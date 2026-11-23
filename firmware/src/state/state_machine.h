/**
 * @file state_machine.h
 * @brief Faulter firmware state machine and parameter management
 */

#ifndef FAULTER_STATE_MACHINE_H
#define FAULTER_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../protocol/protocol.h"
#include "../../shared/protocol_defs.h"

/** Callback type for sending serialized response frames over serial. */
typedef void (*protocol_tx_fn)(const uint8_t *frame, size_t len);

/** All configuration parameters for a glitch setup. */
typedef struct {
    uint32_t glitch_type;
    uint32_t delay_ns;
    uint32_t pulse_ns;
    uint32_t repeat_count;
    uint32_t repeat_delay_ns;
    uint32_t pwr_cycle_en;
    uint32_t pwr_cycle_off_ms;
    uint32_t glitch_pin;
    uint32_t pwr_cycle_pin;
    uint32_t monitor_pin;
    uint32_t monitor_en;
    uint32_t burst_infinite;
    uint32_t burst_gap_ms;
} fault_params_t;

/**
 * @brief Initialize the state machine with default parameters.
 *
 * @param tx_fn Function to call when transmitting response frames.
 */
void state_machine_init(protocol_tx_fn tx_fn);

/**
 * @brief Process an incoming parsed command packet.
 *
 * Handles state transitions, executes actions, and transmits responses via tx_fn.
 *
 * @param pkt Parsed packet.
 */
void state_machine_process_packet(const packet_t *pkt);

/**
 * @brief Get current firmware state.
 *
 * @return One of STATE_IDLE, STATE_ARMED, STATE_PWR_CYCLING, STATE_FIRING, STATE_ERROR.
 */
uint8_t state_machine_get_state(void);

/**
 * @brief Reset parameters to factory defaults and transition to IDLE.
 */
void state_machine_reset_defaults(void);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_STATE_MACHINE_H */
