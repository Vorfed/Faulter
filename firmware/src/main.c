/**
 * @file main.c
 * @brief Faulter firmware entry point for Raspberry Pi Pico (RP2040)
 */

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include "config.h"
#include "protocol/cobs.h"
#include "protocol/protocol.h"
#include "state/state_machine.h"
#include "glitch/glitch_engine.h"
#include "../shared/protocol_defs.h"
#include "tusb.h"

#include <stdio.h>

/* ── Serial Transmission Callback ────────────────────────────────────── */

static void serial_tx_callback(const uint8_t *frame, size_t len) {
    if (frame == NULL || len == 0) return;
    for (size_t i = 0; i < len; i++) {
        putchar_raw((int)frame[i]);
    }
    stdio_flush();
}

/* ── Frame reception buffers ─────────────────────────────────────────── */

static uint8_t s_rx_raw[RX_BUF_SIZE];
static size_t  s_rx_len = 0;
static uint8_t s_decoded_buf[FAULTER_MAX_PAYLOAD_SIZE + PACKET_HEADER_SIZE + PACKET_CRC_SIZE + 4];

/**
 * @brief Non-blocking USB CDC packet poll and dispatcher.
 * Called in main loop and also between bursts to handle immediate ABORT.
 */
static void main_poll_usb(void) {
    tud_task();
    while (true) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            break;
        }

        uint8_t byte = (uint8_t)c;

        if (byte == FAULTER_FRAME_DELIMITER) {
            if (s_rx_len > 0) {
                /* Decode COBS */
                size_t decoded_len = cobs_decode(s_decoded_buf, sizeof(s_decoded_buf),
                                                 s_rx_raw, s_rx_len);
                if (decoded_len >= PACKET_MIN_SIZE) {
                    packet_t pkt;
                    int err = protocol_parse_packet(s_decoded_buf, decoded_len, &pkt);
                    if (err == ERR_NONE) {
                        state_machine_process_packet(&pkt);
                    }
                    /* On CRC mismatch or decode error, frame is corrupt:
                     * quietly drop it. Never transmit a NAK with corrupt
                     * header bytes onto the bus, which corrupts the host. */
                }
                s_rx_len = 0;
            }
        } else {
            if (s_rx_len < sizeof(s_rx_raw)) {
                s_rx_raw[s_rx_len++] = byte;
            } else {
                /* Overflow: reset buffer and wait for next frame delimiter */
                s_rx_len = 0;
            }
        }
    }
}

/* ── Main Loop ───────────────────────────────────────────────────────── */

int main(void) {
    /* Set core voltage to 1.25V for rock-solid 250 MHz high-performance operation */
    vreg_set_voltage(VREG_VOLTAGE_1_25);
    sleep_ms(10);

    /* Overclock RP2040 system clock to 250 MHz (4 ns hardware timing resolution) */
    bool clk_ok = set_sys_clock_khz(250000, false);
    if (!clk_ok) {
        /* Fallback to 125 MHz default if 250 MHz PLL configuration fails */
        set_sys_clock_khz(125000, false);
    }

    /* Initialize default stdio (USB CDC enabled) */
    stdio_init_all();

    /* Disable CRLF translation for pure binary serial communication */
    stdio_set_translate_crlf(&stdio_usb, false);

    /* Initialize state machine and hardware */
    state_machine_init(serial_tx_callback);

    /* Register USB polling callback so glitch loop can check for ABORT packets */
    glitch_engine_set_poll_callback(main_poll_usb);

    while (true) {
        main_poll_usb();
        sleep_us(200);
    }

    return 0;
}
