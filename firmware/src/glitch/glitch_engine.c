/**
 * @file glitch_engine.c
 * @brief PIO-based deterministic glitch engine implementation
 */

#include "glitch_engine.h"
#include "../config.h"
#include "../../shared/protocol_defs.h"
#include "glitch.pio.h"

#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

#include <string.h>

/* ── Internal state ──────────────────────────────────────────────────── */

static PIO   s_pio        = NULL;
static uint  s_sm         = 0;
static uint  s_offset     = 0;
static uint  s_glitch_pin = 0;
static uint  s_monitor_pin = 0;
static bool  s_monitor_en = false;
static bool  s_initialized = false;

/** Volatile abort flag checked between burst repetitions. */
static volatile bool s_abort_requested = false;
static glitch_poll_fn s_poll_fn = NULL;
static glitch_pulse_fn s_pulse_fn = NULL;

void glitch_engine_set_poll_callback(glitch_poll_fn fn) {
    s_poll_fn = fn;
}

void glitch_engine_set_pulse_callback(glitch_pulse_fn fn) {
    s_pulse_fn = fn;
}

/* ── Helpers ─────────────────────────────────────────────────────────── */

/**
 * @brief Convert nanoseconds to PIO cycle counts.
 *
 * The PIO jmp-x-- loop takes 1 cycle per iteration.  A count of 0 still
 * produces one loop iteration (the fall-through), so the minimum is 1 cycle.
 * We subtract 1 because the loop executes (count + 1) times total
 * (count iterations + 1 fall-through).
 *
 * If ns == 0 we return 0 (the PIO will do 1 cycle = 8 ns, which is the
 * minimum achievable).
 */
static inline uint32_t ns_to_cycles(uint32_t ns) {
    if (ns == 0) {
        return 0;
    }
    uint32_t cycles = ns / PIO_CYCLE_NS;
    if (cycles == 0) {
        cycles = 1;
    }
    /* Subtract 1 because jmp x-- executes (x+1) times */
    return cycles - 1;
}

/**
 * @brief Convert PIO cycle counts back to nanoseconds for telemetry.
 *
 * Inverse of ns_to_cycles: the loop ran (count + 1) iterations.
 */
static inline uint32_t cycles_to_ns(uint32_t count) {
    return (count + 1) * PIO_CYCLE_NS;
}

/* ── Public API ──────────────────────────────────────────────────────── */

int glitch_engine_init(uint32_t glitch_pin, uint32_t monitor_pin, bool monitor_en) {
    s_pio = GLITCH_PIO_INSTANCE;
    s_sm  = GLITCH_SM_INDEX;

    /* Load PIO program */
    if (!pio_can_add_program(s_pio, &glitch_engine_program)) {
        return ERR_HARDWARE_FAULT;
    }
    s_offset = pio_add_program(s_pio, &glitch_engine_program);

    /* Configure glitch output pin via PIO helper from .pio file */
    glitch_engine_program_init(s_pio, s_sm, s_offset, glitch_pin);
    s_glitch_pin = glitch_pin;

    /* Configure monitor pin as plain GPIO output */
    s_monitor_pin = monitor_pin;
    s_monitor_en  = monitor_en;
    if (monitor_en) {
        gpio_init(monitor_pin);
        gpio_set_dir(monitor_pin, GPIO_OUT);
        gpio_put(monitor_pin, 0);
    }

    s_abort_requested = false;
    s_initialized = true;
    return ERR_NONE;
}

int glitch_engine_fire(uint32_t delay_ns, uint32_t pulse_ns,
                       glitch_result_t *result) {
    if (!s_initialized) {
        return ERR_HARDWARE_FAULT;
    }

    uint32_t delay_count = ns_to_cycles(delay_ns);
    uint32_t pulse_count = ns_to_cycles(pulse_ns);

    /* Clear any stale IRQ flag */
    pio_interrupt_clear(s_pio, 0);

    /* Raise monitor pin for the whole glitch window */
    if (s_monitor_en) {
        gpio_put(s_monitor_pin, 1);
    }

    /* Capture start time */
    uint64_t t_start = time_us_64();

    /* Push delay count — PIO will start counting down immediately */
    pio_sm_put_blocking(s_pio, s_sm, delay_count);

    /* Push pulse width count */
    pio_sm_put_blocking(s_pio, s_sm, pulse_count);

    /* Wait for PIO IRQ 0 (glitch complete) */
    while (!pio_interrupt_get(s_pio, 0)) {
        tight_loop_contents();
    }
    pio_interrupt_clear(s_pio, 0);

    uint64_t t_end = time_us_64();

    /* Lower monitor pin */
    if (s_monitor_en) {
        gpio_put(s_monitor_pin, 0);
    }

    /* Fill result with computed timing (PIO is deterministic, so we know
     * the exact cycle counts that were executed). */
    if (result) {
        result->actual_delay_ns = cycles_to_ns(delay_count);
        result->actual_pulse_ns = cycles_to_ns(pulse_count);
        result->aborted = false;
    }

    (void)t_start;
    (void)t_end;

    return ERR_NONE;
}

int glitch_engine_fire_burst(uint32_t delay_ns, uint32_t pulse_ns,
                             uint32_t repeat_count, uint32_t repeat_delay_ns,
                             bool infinite_burst, uint32_t burst_gap_ms,
                             glitch_result_t *result) {
    if (!s_initialized) {
        return ERR_HARDWARE_FAULT;
    }

    s_abort_requested = false;

    glitch_result_t last_result;
    memset(&last_result, 0, sizeof(last_result));

    uint32_t pulses_per_burst = (repeat_count > 0) ? repeat_count : 1;

    while (!s_abort_requested) {
        if (s_poll_fn) {
            s_poll_fn();
        }
        if (s_abort_requested) break;

        for (uint32_t i = 0; i < pulses_per_burst; i++) {
            if (s_poll_fn) {
                s_poll_fn();
            }
            if (s_abort_requested) {
                if (result) {
                    *result = last_result;
                    result->aborted = true;
                }
                return ERR_ABORTED;
            }

            int err = glitch_engine_fire(delay_ns, pulse_ns, &last_result);
            if (err != ERR_NONE) {
                if (result) {
                    *result = last_result;
                }
                return err;
            }

            /* Report pulse telemetry to host so meters update live */
            if (s_pulse_fn) {
                s_pulse_fn(last_result.actual_delay_ns, last_result.actual_pulse_ns);
            }

            /* Inter-pulse delay within burst */
            if (i + 1 < pulses_per_burst && repeat_delay_ns > 0) {
                sleep_us(repeat_delay_ns / 1000);
            }
        }

        if (!infinite_burst) {
            break;
        }

        /* Inter-burst gap / timeout — polled every 1ms for sub-millisecond abort latency */
        if (burst_gap_ms > 0) {
            for (uint32_t ms = 0; ms < burst_gap_ms; ms++) {
                if (s_poll_fn) {
                    s_poll_fn();
                }
                if (s_abort_requested) break;
                sleep_ms(1);
            }
            if (s_poll_fn) {
                s_poll_fn();
            }
        } else {
            /* Even with 0 gap, yield briefly to poll abort */
            if (s_poll_fn) {
                s_poll_fn();
            }
        }
    }

    if (result) {
        *result = last_result;
        if (s_abort_requested) result->aborted = true;
    }
    return s_abort_requested ? ERR_ABORTED : ERR_NONE;
}

void glitch_engine_abort(void) {
    s_abort_requested = true;
}

int glitch_engine_set_pins(uint32_t glitch_pin, uint32_t monitor_pin, bool monitor_en) {
    if (!s_initialized) {
        return ERR_HARDWARE_FAULT;
    }

    /* Disable SM, remove old program, re-init with new pin */
    pio_sm_set_enabled(s_pio, s_sm, false);
    pio_remove_program(s_pio, &glitch_engine_program, s_offset);

    /* Ensure old glitch pin is LOW and released */
    gpio_put(s_glitch_pin, 0);

    /* Reload */
    if (!pio_can_add_program(s_pio, &glitch_engine_program)) {
        return ERR_HARDWARE_FAULT;
    }
    s_offset = pio_add_program(s_pio, &glitch_engine_program);
    glitch_engine_program_init(s_pio, s_sm, s_offset, glitch_pin);
    s_glitch_pin = glitch_pin;

    /* Update monitor */
    if (s_monitor_en && monitor_pin != s_monitor_pin) {
        gpio_put(s_monitor_pin, 0);
    }
    s_monitor_pin = monitor_pin;
    s_monitor_en  = monitor_en;
    if (monitor_en) {
        gpio_init(monitor_pin);
        gpio_set_dir(monitor_pin, GPIO_OUT);
        gpio_put(monitor_pin, 0);
    }

    return ERR_NONE;
}
