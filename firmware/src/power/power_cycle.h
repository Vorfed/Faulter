/**
 * @file power_cycle.h
 * @brief Target board power cycling control
 */

#ifndef FAULTER_POWER_CYCLE_H
#define FAULTER_POWER_CYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the power cycle GPIO.
 *
 * Configures the pin as output and sets it HIGH (target powered).
 *
 * @param pin GPIO pin number.
 */
void power_cycle_init(uint32_t pin);

/**
 * @brief Execute a power cycle sequence.
 *
 * Pulls the power cycle pin LOW for off_duration_ms, then restores it HIGH.
 *
 * @param pin             GPIO pin number.
 * @param off_duration_ms Duration in milliseconds to hold power rail LOW.
 */
void power_cycle_execute(uint32_t pin, uint32_t off_duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_POWER_CYCLE_H */
