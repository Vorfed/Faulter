/**
 * @file protocol.c
 * @brief Faulter protocol packet serializer and parser implementation
 */

#include "protocol.h"
#include "cobs.h"
#include "crc16.h"

#include <string.h>

int protocol_parse_packet(const uint8_t *frame, size_t frame_len, packet_t *pkt) {
    if (frame == NULL || pkt == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (frame_len < PACKET_MIN_SIZE) {
        return ERR_FRAME_TOO_LONG;
    }

    size_t data_len = frame_len - PACKET_CRC_SIZE;
    uint16_t expected_crc = (uint16_t)((frame[data_len] << 8) | frame[data_len + 1]);
    uint16_t computed_crc = crc16_ccitt(frame, data_len);

    if (expected_crc != computed_crc) {
        return ERR_CRC_MISMATCH;
    }

    pkt->type = frame[0];
    pkt->seq  = frame[1];
    pkt->payload_len = data_len - PACKET_HEADER_SIZE;

    if (pkt->payload_len > FAULTER_MAX_PAYLOAD_SIZE) {
        return ERR_FRAME_TOO_LONG;
    }

    if (pkt->payload_len > 0) {
        memcpy(pkt->payload, &frame[PACKET_HEADER_SIZE], pkt->payload_len);
    }

    return ERR_NONE;
}

size_t protocol_build_frame(uint8_t *out_buf, size_t out_max,
                            uint8_t type, uint8_t seq,
                            const uint8_t *payload, size_t payload_len) {
    if (out_buf == NULL || (payload == NULL && payload_len > 0)) {
        return 0;
    }
    if (payload_len > FAULTER_MAX_PAYLOAD_SIZE) {
        return 0;
    }

    /* Assemble raw un-encoded packet: [type][seq][payload...][crc_h][crc_l] */
    uint8_t raw[FAULTER_MAX_PAYLOAD_SIZE + PACKET_HEADER_SIZE + PACKET_CRC_SIZE];
    raw[0] = type;
    raw[1] = seq;

    if (payload_len > 0) {
        memcpy(&raw[2], payload, payload_len);
    }

    size_t data_len = PACKET_HEADER_SIZE + payload_len;
    uint16_t crc = crc16_ccitt(raw, data_len);
    raw[data_len]     = (uint8_t)(crc >> 8);
    raw[data_len + 1] = (uint8_t)(crc & 0xFF);
    size_t raw_len = data_len + PACKET_CRC_SIZE;

    /* COBS encode into out_buf */
    size_t encoded_len = cobs_encode(out_buf, out_max, raw, raw_len);
    if (encoded_len == 0 || encoded_len + 1 > out_max) {
        return 0;
    }

    /* Append delimiter */
    out_buf[encoded_len] = FAULTER_FRAME_DELIMITER;
    return encoded_len + 1;
}

size_t protocol_build_pong(uint8_t *buf, size_t max_len, uint8_t seq) {
    /* [proto_major, proto_minor, fw_major, fw_minor, fw_patch, caps_l, caps_h] */
    uint16_t caps = FAULTER_V1_CAPABILITIES;
    uint8_t payload[7] = {
        FAULTER_PROTO_VERSION_MAJOR,
        FAULTER_PROTO_VERSION_MINOR,
        1, 0, 0, /* Firmware v1.0.0 */
        (uint8_t)(caps & 0xFF),
        (uint8_t)(caps >> 8)
    };
    return protocol_build_frame(buf, max_len, PKT_RSP_PONG, seq, payload, sizeof(payload));
}

size_t protocol_build_ack(uint8_t *buf, size_t max_len, uint8_t seq, uint8_t cmd_type) {
    uint8_t payload[1] = { cmd_type };
    return protocol_build_frame(buf, max_len, PKT_RSP_ACK, seq, payload, sizeof(payload));
}

size_t protocol_build_nak(uint8_t *buf, size_t max_len, uint8_t seq,
                          uint8_t cmd_type, uint8_t err_code) {
    uint8_t payload[2] = { cmd_type, err_code };
    return protocol_build_frame(buf, max_len, PKT_RSP_NAK, seq, payload, sizeof(payload));
}

size_t protocol_build_param_value(uint8_t *buf, size_t max_len, uint8_t seq,
                                  uint8_t param_id, uint32_t value) {
    uint8_t payload[5];
    payload[0] = param_id;
    payload[1] = (uint8_t)(value & 0xFF);
    payload[2] = (uint8_t)((value >> 8) & 0xFF);
    payload[3] = (uint8_t)((value >> 16) & 0xFF);
    payload[4] = (uint8_t)((value >> 24) & 0xFF);
    return protocol_build_frame(buf, max_len, PKT_RSP_PARAM_VALUE, seq, payload, sizeof(payload));
}

size_t protocol_build_status(uint8_t *buf, size_t max_len, uint8_t seq,
                             uint8_t state, uint8_t last_error,
                             uint32_t last_delay_ns, uint32_t last_pulse_ns) {
    uint8_t payload[10];
    payload[0] = state;
    payload[1] = last_error;
    payload[2] = (uint8_t)(last_delay_ns & 0xFF);
    payload[3] = (uint8_t)((last_delay_ns >> 8) & 0xFF);
    payload[4] = (uint8_t)((last_delay_ns >> 16) & 0xFF);
    payload[5] = (uint8_t)((last_delay_ns >> 24) & 0xFF);
    payload[6] = (uint8_t)(last_pulse_ns & 0xFF);
    payload[7] = (uint8_t)((last_pulse_ns >> 8) & 0xFF);
    payload[8] = (uint8_t)((last_pulse_ns >> 16) & 0xFF);
    payload[9] = (uint8_t)((last_pulse_ns >> 24) & 0xFF);
    return protocol_build_frame(buf, max_len, PKT_RSP_STATUS, seq, payload, sizeof(payload));
}

size_t protocol_build_glitch_done(uint8_t *buf, size_t max_len,
                                  uint8_t result, uint32_t actual_delay_ns,
                                  uint32_t actual_pulse_ns) {
    uint8_t payload[9];
    payload[0] = result;
    payload[1] = (uint8_t)(actual_delay_ns & 0xFF);
    payload[2] = (uint8_t)((actual_delay_ns >> 8) & 0xFF);
    payload[3] = (uint8_t)((actual_delay_ns >> 16) & 0xFF);
    payload[4] = (uint8_t)((actual_delay_ns >> 24) & 0xFF);
    payload[5] = (uint8_t)(actual_pulse_ns & 0xFF);
    payload[6] = (uint8_t)((actual_pulse_ns >> 8) & 0xFF);
    payload[7] = (uint8_t)((actual_pulse_ns >> 16) & 0xFF);
    payload[8] = (uint8_t)((actual_pulse_ns >> 24) & 0xFF);
    return protocol_build_frame(buf, max_len, PKT_RSP_GLITCH_DONE, SEQ_UNSOLICITED,
                                payload, sizeof(payload));
}
