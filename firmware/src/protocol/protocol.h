/**
 * @file protocol.h
 * @brief Faulter protocol packet serializer and parser
 */

#ifndef FAULTER_PROTOCOL_H
#define FAULTER_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../shared/protocol_defs.h"

/** Parsed packet representation. */
typedef struct {
    uint8_t type;
    uint8_t seq;
    uint8_t payload[FAULTER_MAX_PAYLOAD_SIZE];
    size_t  payload_len;
} packet_t;

/**
 * @brief Parse and validate a raw decoded frame (before COBS encode / after COBS decode).
 *
 * Verifies minimum size and CRC-16.
 *
 * @param frame     Decoded frame buffer.
 * @param frame_len Length of decoded frame.
 * @param pkt       Output parsed packet.
 * @return ERR_NONE on success, error code on failure.
 */
int protocol_parse_packet(const uint8_t *frame, size_t frame_len, packet_t *pkt);

/**
 * @brief Build a framed COBS packet ready to be sent over serial (including trailing 0x00).
 *
 * Takes unencoded packet components (type, seq, payload), appends CRC-16,
 * COBS-encodes, and appends the 0x00 delimiter.
 *
 * @param out_buf     Destination buffer (must be at least FAULTER_MAX_FRAME_SIZE).
 * @param out_max     Size of destination buffer.
 * @param type        Packet type opcode.
 * @param seq         Sequence number.
 * @param payload     Payload bytes (or NULL if payload_len == 0).
 * @param payload_len Payload length in bytes.
 * @return Total bytes written to out_buf (including 0x00 delimiter), or 0 on error.
 */
size_t protocol_build_frame(uint8_t *out_buf, size_t out_max,
                            uint8_t type, uint8_t seq,
                            const uint8_t *payload, size_t payload_len);

/* ── Convenience packet builders ─────────────────────────────────────── */

size_t protocol_build_pong(uint8_t *buf, size_t max_len, uint8_t seq);

size_t protocol_build_ack(uint8_t *buf, size_t max_len, uint8_t seq, uint8_t cmd_type);

size_t protocol_build_nak(uint8_t *buf, size_t max_len, uint8_t seq,
                          uint8_t cmd_type, uint8_t err_code);

size_t protocol_build_param_value(uint8_t *buf, size_t max_len, uint8_t seq,
                                  uint8_t param_id, uint32_t value);

size_t protocol_build_status(uint8_t *buf, size_t max_len, uint8_t seq,
                             uint8_t state, uint8_t last_error,
                             uint32_t last_delay_ns, uint32_t last_pulse_ns);

size_t protocol_build_glitch_done(uint8_t *buf, size_t max_len,
                                  uint8_t result, uint32_t actual_delay_ns,
                                  uint32_t actual_pulse_ns);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_PROTOCOL_H */
