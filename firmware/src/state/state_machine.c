/**
 * @file state_machine.c
 * @brief Faulter firmware state machine and parameter management implementation
 */

#include "state_machine.h"
#include "../glitch/glitch_engine.h"
#include "../power/power_cycle.h"
#include "../protocol/protocol.h"
#include "../../shared/protocol_defs.h"

#include <string.h>

/* ── Internal State ──────────────────────────────────────────────────── */

static protocol_tx_fn s_tx = NULL;
static uint8_t        s_state = STATE_IDLE;
static uint8_t        s_last_error = ERR_NONE;
static fault_params_t s_params;
static uint32_t       s_last_delay_ns = 0;
static uint32_t       s_last_pulse_ns = 0;

/* Frame transmission scratch buffer */
static uint8_t s_tx_buf[FAULTER_MAX_FRAME_SIZE + 4];

/* ── Helper Functions ────────────────────────────────────────────────── */

static void send_ack(uint8_t seq, uint8_t cmd) {
    if (s_tx) {
        size_t len = protocol_build_ack(s_tx_buf, sizeof(s_tx_buf), seq, cmd);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static void send_nak(uint8_t seq, uint8_t cmd, uint8_t err) {
    s_last_error = err;
    if (s_tx) {
        size_t len = protocol_build_nak(s_tx_buf, sizeof(s_tx_buf), seq, cmd, err);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static void send_pong(uint8_t seq) {
    if (s_tx) {
        size_t len = protocol_build_pong(s_tx_buf, sizeof(s_tx_buf), seq);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static void send_status(uint8_t seq) {
    if (s_tx) {
        size_t len = protocol_build_status(s_tx_buf, sizeof(s_tx_buf), seq,
                                           s_state, s_last_error,
                                           s_last_delay_ns, s_last_pulse_ns);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static void send_param_val(uint8_t seq, uint8_t param_id, uint32_t val) {
    if (s_tx) {
        size_t len = protocol_build_param_value(s_tx_buf, sizeof(s_tx_buf), seq, param_id, val);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static void send_glitch_done(uint8_t result, uint32_t delay_ns, uint32_t pulse_ns) {
    if (s_tx) {
        size_t len = protocol_build_glitch_done(s_tx_buf, sizeof(s_tx_buf), result, delay_ns, pulse_ns);
        if (len > 0) s_tx(s_tx_buf, len);
    }
}

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* ── Parameter Management ────────────────────────────────────────────── */

static int validate_and_set_param(uint8_t param_id, uint32_t val) {
    switch (param_id) {
        case PARAM_GLITCH_TYPE:
            if (val != GLITCH_TYPE_VOLTAGE) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.glitch_type = val;
            break;
        case PARAM_DELAY_NS:
            s_params.delay_ns = val;
            break;
        case PARAM_PULSE_NS:
            if (val == 0) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            if (val < MIN_PULSE_NS) {
                val = MIN_PULSE_NS;
            }
            s_params.pulse_ns = val;
            break;
        case PARAM_REPEAT_COUNT:
            if (val < 1 || val > MAX_REPEAT_COUNT) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.repeat_count = val;
            break;
        case PARAM_REPEAT_DELAY_NS:
            s_params.repeat_delay_ns = val;
            break;
        case PARAM_PWR_CYCLE_EN:
            s_params.pwr_cycle_en = (val != 0) ? 1 : 0;
            break;
        case PARAM_PWR_CYCLE_OFF_MS:
            if (val < 1 || val > MAX_PWR_CYCLE_OFF_MS) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.pwr_cycle_off_ms = val;
            break;
        case PARAM_GLITCH_PIN:
            if (val > MAX_GPIO_PIN) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.glitch_pin = val;
            glitch_engine_set_pins(s_params.glitch_pin, s_params.monitor_pin, s_params.monitor_en);
            break;
        case PARAM_PWR_CYCLE_PIN:
            if (val > MAX_GPIO_PIN) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.pwr_cycle_pin = val;
            power_cycle_init(s_params.pwr_cycle_pin);
            break;
        case PARAM_MONITOR_PIN:
            if (val > MAX_GPIO_PIN) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.monitor_pin = val;
            glitch_engine_set_pins(s_params.glitch_pin, s_params.monitor_pin, s_params.monitor_en);
            break;
        case PARAM_MONITOR_EN:
            s_params.monitor_en = (val != 0) ? 1 : 0;
            glitch_engine_set_pins(s_params.glitch_pin, s_params.monitor_pin, s_params.monitor_en);
            break;
        case PARAM_BURST_INFINITE:
            s_params.burst_infinite = (val != 0) ? 1 : 0;
            break;
        case PARAM_BURST_GAP_MS:
            if (val > MAX_BURST_GAP_MS) {
                return ERR_VALUE_OUT_OF_RANGE;
            }
            s_params.burst_gap_ms = val;
            break;
        default:
            return ERR_INVALID_PARAM;
    }
    return ERR_NONE;
}

static int get_param_value(uint8_t param_id, uint32_t *out_val) {
    switch (param_id) {
        case PARAM_GLITCH_TYPE:      *out_val = s_params.glitch_type; break;
        case PARAM_DELAY_NS:         *out_val = s_params.delay_ns; break;
        case PARAM_PULSE_NS:         *out_val = s_params.pulse_ns; break;
        case PARAM_REPEAT_COUNT:     *out_val = s_params.repeat_count; break;
        case PARAM_REPEAT_DELAY_NS:  *out_val = s_params.repeat_delay_ns; break;
        case PARAM_PWR_CYCLE_EN:     *out_val = s_params.pwr_cycle_en; break;
        case PARAM_PWR_CYCLE_OFF_MS: *out_val = s_params.pwr_cycle_off_ms; break;
        case PARAM_GLITCH_PIN:       *out_val = s_params.glitch_pin; break;
        case PARAM_PWR_CYCLE_PIN:    *out_val = s_params.pwr_cycle_pin; break;
        case PARAM_MONITOR_PIN:      *out_val = s_params.monitor_pin; break;
        case PARAM_MONITOR_EN:       *out_val = s_params.monitor_en; break;
        case PARAM_BURST_INFINITE:   *out_val = s_params.burst_infinite; break;
        case PARAM_BURST_GAP_MS:     *out_val = s_params.burst_gap_ms; break;
        default: return ERR_INVALID_PARAM;
    }
    return ERR_NONE;
}

/* ── Public API ──────────────────────────────────────────────────────── */

void state_machine_reset_defaults(void) {
    s_params.glitch_type       = DEFAULT_GLITCH_TYPE;
    s_params.delay_ns          = DEFAULT_DELAY_NS;
    s_params.pulse_ns          = DEFAULT_PULSE_NS;
    s_params.repeat_count      = DEFAULT_REPEAT_COUNT;
    s_params.repeat_delay_ns   = DEFAULT_REPEAT_DELAY_NS;
    s_params.pwr_cycle_en      = DEFAULT_PWR_CYCLE_EN;
    s_params.pwr_cycle_off_ms  = DEFAULT_PWR_CYCLE_OFF_MS;
    s_params.glitch_pin        = DEFAULT_GLITCH_PIN;
    s_params.pwr_cycle_pin     = DEFAULT_PWR_CYCLE_PIN;
    s_params.monitor_pin       = DEFAULT_MONITOR_PIN;
    s_params.monitor_en        = DEFAULT_MONITOR_EN;
    s_params.burst_infinite    = DEFAULT_BURST_INFINITE;
    s_params.burst_gap_ms      = DEFAULT_BURST_GAP_MS;

    s_state = STATE_IDLE;
    s_last_error = ERR_NONE;

    /* Reinitialize peripherals with defaults */
    glitch_engine_init(s_params.glitch_pin, s_params.monitor_pin, s_params.monitor_en != 0);
    power_cycle_init(s_params.pwr_cycle_pin);
}

static void on_glitch_pulse_executed(uint32_t delay_ns, uint32_t pulse_ns) {
    s_last_delay_ns = delay_ns;
    s_last_pulse_ns = pulse_ns;
    send_glitch_done(GLITCH_RESULT_OK, delay_ns, pulse_ns);
}

void state_machine_init(protocol_tx_fn tx_fn) {
    s_tx = tx_fn;
    state_machine_reset_defaults();
    glitch_engine_set_pulse_callback(on_glitch_pulse_executed);
}

uint8_t state_machine_get_state(void) {
    return s_state;
}

void state_machine_process_packet(const packet_t *pkt) {
    if (pkt == NULL) return;

    /* Global commands valid in any state */
    if (pkt->type == PKT_CMD_PING) {
        send_pong(pkt->seq);
        return;
    }

    if (pkt->type == PKT_CMD_GET_STATUS) {
        send_status(pkt->seq);
        return;
    }

    if (pkt->type == PKT_CMD_RESET) {
        glitch_engine_abort();
        state_machine_reset_defaults();
        send_ack(pkt->seq, PKT_CMD_RESET);
        return;
    }

    /* State-dependent handling */
    switch (s_state) {
        case STATE_IDLE: {
            switch (pkt->type) {
                case PKT_CMD_SET_PARAM: {
                    if (pkt->payload_len < 5) {
                        send_nak(pkt->seq, PKT_CMD_SET_PARAM, ERR_INVALID_PARAM);
                        return;
                    }
                    uint8_t param_id = pkt->payload[0];
                    uint32_t val = read_u32_le(&pkt->payload[1]);
                    int err = validate_and_set_param(param_id, val);
                    if (err == ERR_NONE) {
                        send_ack(pkt->seq, PKT_CMD_SET_PARAM);
                    } else {
                        send_nak(pkt->seq, PKT_CMD_SET_PARAM, (uint8_t)err);
                    }
                    break;
                }
                case PKT_CMD_GET_PARAM: {
                    if (pkt->payload_len < 1) {
                        send_nak(pkt->seq, PKT_CMD_GET_PARAM, ERR_INVALID_PARAM);
                        return;
                    }
                    uint8_t param_id = pkt->payload[0];
                    uint32_t val = 0;
                    int err = get_param_value(param_id, &val);
                    if (err == ERR_NONE) {
                        send_param_val(pkt->seq, param_id, val);
                    } else {
                        send_nak(pkt->seq, PKT_CMD_GET_PARAM, (uint8_t)err);
                    }
                    break;
                }
                case PKT_CMD_ARM: {
                    /* Validate parameters before arming — clamp to minimum PIO cycle if needed */
                    if (s_params.pulse_ns == 0) {
                        send_nak(pkt->seq, PKT_CMD_ARM, ERR_VALUE_OUT_OF_RANGE);
                        return;
                    }
                    if (s_params.pulse_ns < MIN_PULSE_NS) {
                        s_params.pulse_ns = MIN_PULSE_NS;
                    }
                    s_state = STATE_ARMED;
                    send_ack(pkt->seq, PKT_CMD_ARM);
                    break;
                }
                case PKT_CMD_FIRE:
                    send_nak(pkt->seq, PKT_CMD_FIRE, ERR_INVALID_STATE);
                    break;
                case PKT_CMD_ABORT:
                    send_ack(pkt->seq, PKT_CMD_ABORT);
                    break;
                default:
                    send_nak(pkt->seq, pkt->type, ERR_UNKNOWN_CMD);
                    break;
            }
            break;
        }

        case STATE_ARMED: {
            switch (pkt->type) {
                case PKT_CMD_FIRE: {
                    /* Acknowledge fire command immediately */
                    send_ack(pkt->seq, PKT_CMD_FIRE);

                    /* 1. Optional target power cycle */
                    if (s_params.pwr_cycle_en) {
                        s_state = STATE_PWR_CYCLING;
                        power_cycle_execute(s_params.pwr_cycle_pin, s_params.pwr_cycle_off_ms);
                    }

                    /* 2. Execute glitch pulses via PIO engine */
                    s_state = STATE_FIRING;
                    glitch_result_t result;
                    memset(&result, 0, sizeof(result));

                    int err = glitch_engine_fire_burst(
                        s_params.delay_ns,
                        s_params.pulse_ns,
                        s_params.repeat_count,
                        s_params.repeat_delay_ns,
                        s_params.burst_infinite != 0,
                        s_params.burst_gap_ms,
                        &result
                    );

                    s_last_delay_ns = result.actual_delay_ns;
                    s_last_pulse_ns = result.actual_pulse_ns;

                    /* 3. Return to IDLE */
                    s_state = STATE_IDLE;

                    /* 4. Emit final completion / abort status */
                    if (err == ERR_ABORTED || result.aborted) {
                        send_glitch_done(GLITCH_RESULT_ABORTED, s_last_delay_ns, s_last_pulse_ns);
                    } else if (err != ERR_NONE) {
                        s_last_error = (uint8_t)err;
                        send_glitch_done(GLITCH_RESULT_ERROR, s_last_delay_ns, s_last_pulse_ns);
                    } else if (!s_params.burst_infinite) {
                        send_glitch_done(GLITCH_RESULT_OK, s_last_delay_ns, s_last_pulse_ns);
                    }
                    break;
                }
                case PKT_CMD_ABORT:
                    s_state = STATE_IDLE;
                    send_ack(pkt->seq, PKT_CMD_ABORT);
                    break;
                case PKT_CMD_ARM:
                    send_ack(pkt->seq, PKT_CMD_ARM);
                    break;
                case PKT_CMD_SET_PARAM:
                    send_nak(pkt->seq, PKT_CMD_SET_PARAM, ERR_INVALID_STATE);
                    break;
                case PKT_CMD_GET_PARAM: {
                    if (pkt->payload_len < 1) {
                        send_nak(pkt->seq, PKT_CMD_GET_PARAM, ERR_INVALID_PARAM);
                        return;
                    }
                    uint32_t val = 0;
                    int err = get_param_value(pkt->payload[0], &val);
                    if (err == ERR_NONE) {
                        send_param_val(pkt->seq, pkt->payload[0], val);
                    } else {
                        send_nak(pkt->seq, PKT_CMD_GET_PARAM, (uint8_t)err);
                    }
                    break;
                }
                default:
                    send_nak(pkt->seq, pkt->type, ERR_UNKNOWN_CMD);
                    break;
            }
            break;
        }

        case STATE_PWR_CYCLING:
        case STATE_FIRING: {
            if (pkt->type == PKT_CMD_ABORT) {
                glitch_engine_abort();
                s_state = STATE_IDLE;
                send_ack(pkt->seq, PKT_CMD_ABORT);
            } else {
                send_nak(pkt->seq, pkt->type, ERR_INVALID_STATE);
            }
            break;
        }

        case STATE_ERROR: {
            if (pkt->type == PKT_CMD_ABORT || pkt->type == PKT_CMD_RESET) {
                state_machine_reset_defaults();
                send_ack(pkt->seq, pkt->type);
            } else {
                send_nak(pkt->seq, pkt->type, ERR_INVALID_STATE);
            }
            break;
        }
    }
}
