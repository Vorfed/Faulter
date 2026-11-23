/**
 * @file config.h
 * @brief Faulter firmware hardware configuration constants
 *
 * Default pin assignments, system clock, and PIO instance selection.
 */

#ifndef FAULTER_CONFIG_H
#define FAULTER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * System Clock
 * ====================================================================== */

/** System clock frequency in Hz (RP2040 default). */
#define SYS_CLOCK_HZ           125000000u

/** PIO cycle time in nanoseconds at 125 MHz. */
#define PIO_CYCLE_NS           8u

/* ======================================================================
 * PIO Configuration
 * ====================================================================== */

/** Which PIO instance to use for the glitch engine. */
#define GLITCH_PIO_INSTANCE    pio0

/** State machine index within the PIO instance. */
#define GLITCH_SM_INDEX        0

/* ======================================================================
 * Default Pin Assignments
 * ====================================================================== */

/** GPIO pin that drives the glitch MOSFET. */
#define PIN_GLITCH_DEFAULT     19

/** GPIO pin that controls target power cycling. */
#define PIN_PWR_CYCLE_DEFAULT  10

/** GPIO pin for oscilloscope monitor output (HIGH during glitch window). */
#define PIN_MONITOR_DEFAULT    9

/* ======================================================================
 * USB CDC Configuration
 * ====================================================================== */

/** Baud rate for USB CDC serial (informational — USB CDC ignores this). */
#define USB_CDC_BAUD           921600

/* ======================================================================
 * Buffer Sizes
 * ====================================================================== */

/** Maximum size for serial RX frame accumulation buffer. */
#define RX_BUF_SIZE            128

/** Maximum size for serial TX frame buffer. */
#define TX_BUF_SIZE            128

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_CONFIG_H */
