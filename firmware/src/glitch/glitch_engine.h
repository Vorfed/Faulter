/**
 * @file glitch_engine.h
 * @brief PIO-based deterministic glitch engine API
 *
 * Provides functions to initialize the PIO glitch hardware and execute
 * voltage glitch pulses with nanosecond-resolution timing.
 */

#ifndef FAULTER_GLITCH_ENGINE_H
#define FAULTER_GLITCH_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Result of a glitch execution. */
typedef struct {
    uint32_t actual_delay_ns;   /**< Measured delay in nanoseconds. */
    uint32_t actual_pulse_ns;   /**< Measured pulse width in nanoseconds. */
    bool     aborted;           /**< True if the glitch was aborted. */
} glitch_result_t;

/**
 * @brief Initialize the PIO glitch engine.
 *
 * Loads the PIO program, configures the state machine, and sets up the
 * glitch output GPIO pin.
 *
 * @param glitch_pin    GPIO pin number for glitch output.
 * @param monitor_pin   GPIO pin number for monitor output (scope trigger).
 * @param monitor_en    Whether to enable the monitor pin.
 * @return 0 on success, ERR_HARDWARE_FAULT on failure.
 */
int glitch_engine_init(uint32_t glitch_pin, uint32_t monitor_pin, bool monitor_en);

/**
 * @brief Execute a single glitch pulse.
 *
 * Pushes delay and pulse width cycle counts into the PIO FIFO and waits
 * for the IRQ indicating completion.  Measures actual timing via hardware
 * timer.
 *
 * @param delay_ns   Desired delay in nanoseconds before glitch pulse.
 * @param pulse_ns   Desired pulse width in nanoseconds.
 * @param result     Output: measured timing result.
 * @return 0 on success, nonzero on error.
 */
int glitch_engine_fire(uint32_t delay_ns, uint32_t pulse_ns,
                       glitch_result_t *result);

/**
 * @brief Execute a burst of repeated glitch pulses (or continuous infinite burst).
 *
 * @param delay_ns          Delay before each pulse (ns).
 * @param pulse_ns          Pulse width (ns).
 * @param repeat_count      Number of pulses per burst.
 * @param repeat_delay_ns   Inter-pulse gap within burst (ns).
 * @param infinite_burst    Whether to repeat bursts continuously until aborted.
 * @param burst_gap_ms      Pause between bursts in milliseconds.
 * @param result            Output: timing of the last executed pulse.
 * @return 0 on success, nonzero on error or abort.
 */
int glitch_engine_fire_burst(uint32_t delay_ns, uint32_t pulse_ns,
                             uint32_t repeat_count, uint32_t repeat_delay_ns,
                             bool infinite_burst, uint32_t burst_gap_ms,
                             glitch_result_t *result);

/**
 * @brief Request abort of an in-progress glitch.
 *
 * Sets an internal flag that is checked between repeated pulses.
 * A single-shot pulse cannot be aborted once the PIO has started.
 */
void glitch_engine_abort(void);

typedef void (*glitch_poll_fn)(void);
void glitch_engine_set_poll_callback(glitch_poll_fn fn);

typedef void (*glitch_pulse_fn)(uint32_t delay_ns, uint32_t pulse_ns);
void glitch_engine_set_pulse_callback(glitch_pulse_fn fn);

/**
 * @brief Reconfigure the glitch output pin.
 *
 * Only call when the engine is idle (not mid-glitch).
 *
 * @param glitch_pin   New GPIO pin number.
 * @param monitor_pin  New monitor GPIO pin number.
 * @param monitor_en   Whether to enable the monitor pin.
 * @return 0 on success.
 */
int glitch_engine_set_pins(uint32_t glitch_pin, uint32_t monitor_pin, bool monitor_en);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_GLITCH_ENGINE_H */
