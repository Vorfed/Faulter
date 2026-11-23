/**
 * @file protocol_defs.h
 * @brief Faulter Protocol Definitions — Shared between firmware and host
 * @version 1.0.0
 *
 * This header defines the wire-protocol constants used by both the RP2040
 * firmware and the desktop host application. It is plain C with C++ guards
 * so it compiles in either toolchain.
 *
 * See docs/protocol.md for the full protocol specification.
 */

#ifndef FAULTER_PROTOCOL_DEFS_H
#define FAULTER_PROTOCOL_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ======================================================================
 * Protocol Version
 * ====================================================================== */

#define FAULTER_PROTO_VERSION_MAJOR  1
#define FAULTER_PROTO_VERSION_MINOR  0

/* ======================================================================
 * Framing Constants
 * ====================================================================== */

#define FAULTER_FRAME_DELIMITER      0x00
#define FAULTER_MAX_PAYLOAD_SIZE     64
/** Max COBS-encoded size: payload + ceil(payload/254) + delimiter */
#define FAULTER_MAX_FRAME_SIZE       (FAULTER_MAX_PAYLOAD_SIZE + 2 + 1)
#define FAULTER_BAUD_RATE            921600

/* ======================================================================
 * Packet Types — Host → Firmware (Commands)  0x01–0x7F
 * ====================================================================== */

#define PKT_CMD_PING          0x01
#define PKT_CMD_SET_PARAM     0x02
#define PKT_CMD_GET_PARAM     0x03
#define PKT_CMD_ARM           0x04
#define PKT_CMD_FIRE          0x05
#define PKT_CMD_ABORT         0x06
#define PKT_CMD_GET_STATUS    0x07
#define PKT_CMD_RESET         0x08

/* ======================================================================
 * Packet Types — Firmware → Host (Responses)  0x80–0xFF
 * ====================================================================== */

#define PKT_RSP_PONG          0x81
#define PKT_RSP_ACK           0x82
#define PKT_RSP_NAK           0x83
#define PKT_RSP_PARAM_VALUE   0x84
#define PKT_RSP_STATUS        0x85
#define PKT_RSP_GLITCH_DONE   0x86

/* ======================================================================
 * Parameter IDs
 * ====================================================================== */

/* Core glitch parameters (0x01–0x1F) */
#define PARAM_GLITCH_TYPE         0x01
#define PARAM_DELAY_NS            0x02
#define PARAM_PULSE_NS            0x03
#define PARAM_REPEAT_COUNT        0x04
#define PARAM_REPEAT_DELAY_NS     0x05
#define PARAM_PWR_CYCLE_EN        0x06
#define PARAM_PWR_CYCLE_OFF_MS    0x07
#define PARAM_GLITCH_PIN          0x08
#define PARAM_PWR_CYCLE_PIN       0x09
#define PARAM_MONITOR_PIN         0x0A
#define PARAM_MONITOR_EN          0x0B
#define PARAM_BURST_INFINITE      0x0C  /* 0 = fixed count, 1 = infinite continuous burst */
#define PARAM_BURST_GAP_MS        0x0D  /* Pause / gap between bursts in milliseconds */

/* Trigger parameters — reserved for future (0x20–0x3F) */
/* Clock glitch parameters — reserved for future (0x40–0x5F) */

#define PARAM_ID_MAX              0x0D  /* Highest valid param */

/* ======================================================================
 * Glitch Types
 * ====================================================================== */

#define GLITCH_TYPE_VOLTAGE   0
#define GLITCH_TYPE_CLOCK     1   /* Reserved for future */

/* ======================================================================
 * Firmware State Machine States
 * ====================================================================== */

#define STATE_IDLE            0x00
#define STATE_ARMED           0x01
#define STATE_PWR_CYCLING     0x02
#define STATE_FIRING          0x03
#define STATE_ERROR           0x04

/* ======================================================================
 * Error Codes
 * ====================================================================== */

#define ERR_NONE              0x00
#define ERR_UNKNOWN_CMD       0x01
#define ERR_INVALID_STATE     0x02
#define ERR_INVALID_PARAM     0x03
#define ERR_VALUE_OUT_OF_RANGE 0x04
#define ERR_CRC_MISMATCH      0x05
#define ERR_FRAME_TOO_LONG    0x06
#define ERR_NOT_CONFIGURED    0x07
#define ERR_HARDWARE_FAULT    0x08
#define ERR_PROTO_VERSION     0x09
#define ERR_ABORTED           0x0A

/* ======================================================================
 * Capability Bits (reported in RSP_PONG)
 * ====================================================================== */

#define CAP_VOLTAGE_GLITCH    (1u << 0)
#define CAP_CLOCK_GLITCH      (1u << 1)
#define CAP_TRIGGER_GPIO      (1u << 2)
#define CAP_TRIGGER_UART      (1u << 3)
#define CAP_POWER_CYCLE       (1u << 4)
#define CAP_MONITOR_PIN       (1u << 5)
#define CAP_BURST_MODE        (1u << 6)

/** V1 firmware capabilities */
#define FAULTER_V1_CAPABILITIES  \
    (CAP_VOLTAGE_GLITCH | CAP_POWER_CYCLE | CAP_MONITOR_PIN | CAP_BURST_MODE)

/* ======================================================================
 * Glitch Result Codes (in RSP_GLITCH_DONE)
 * ====================================================================== */

#define GLITCH_RESULT_OK      0x00
#define GLITCH_RESULT_ABORTED 0x01
#define GLITCH_RESULT_ERROR   0x02

/* ======================================================================
 * Sequence Number for Unsolicited Messages
 * ====================================================================== */

#define SEQ_UNSOLICITED       0xFF

/* ======================================================================
 * Default Parameter Values
 * ====================================================================== */

#define DEFAULT_GLITCH_TYPE       GLITCH_TYPE_VOLTAGE
#define DEFAULT_DELAY_NS          0
#define DEFAULT_PULSE_NS          100
#define DEFAULT_REPEAT_COUNT      1
#define DEFAULT_REPEAT_DELAY_NS   1000
#define DEFAULT_PWR_CYCLE_EN      0
#define DEFAULT_PWR_CYCLE_OFF_MS  50
#define DEFAULT_GLITCH_PIN        19
#define DEFAULT_PWR_CYCLE_PIN     10
#define DEFAULT_MONITOR_PIN       9
#define DEFAULT_MONITOR_EN        1
#define DEFAULT_BURST_INFINITE    0
#define DEFAULT_BURST_GAP_MS      100

/* ======================================================================
 * Parameter Validation Ranges
 * ====================================================================== */

#define MIN_PULSE_NS              8     /* 1 PIO cycle at 125 MHz */
#define MAX_GPIO_PIN              28
#define MAX_PWR_CYCLE_OFF_MS      10000
#define MAX_REPEAT_COUNT          65535
#define MAX_BURST_GAP_MS          60000 /* 60 seconds max gap */

/* ======================================================================
 * Packet Structure Helpers
 * ====================================================================== */

/** Minimum packet size: type(1) + seq(1) + crc(2) = 4 bytes */
#define PACKET_HEADER_SIZE        2
#define PACKET_CRC_SIZE           2
#define PACKET_MIN_SIZE           (PACKET_HEADER_SIZE + PACKET_CRC_SIZE)

/* ======================================================================
 * CRC-16/CCITT Parameters
 * ====================================================================== */

#define CRC16_POLYNOMIAL          0x1021
#define CRC16_INITIAL             0xFFFF

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_PROTOCOL_DEFS_H */
