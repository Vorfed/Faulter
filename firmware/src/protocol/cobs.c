/**
 * @file cobs.c
 * @brief COBS encode/decode implementation
 *
 * Reference: Cheshire & Baker, "Consistent Overhead Byte Stuffing", 1999.
 */

#include "cobs.h"

size_t cobs_encode(uint8_t *dst, size_t dst_size,
                   const uint8_t *src, size_t src_len) {
    if (dst == NULL || src == NULL) {
        return 0;
    }

    /* Worst case encoded size: src_len + ceil(src_len/254) + 1 */
    size_t worst = src_len + (src_len / 254) + 1;
    if (dst_size < worst) {
        return 0;
    }

    size_t read_idx  = 0;
    size_t write_idx = 0;
    size_t code_idx  = write_idx++;  /* Reserve space for first code byte */
    uint8_t code     = 1;

    while (read_idx < src_len) {
        if (src[read_idx] == 0x00) {
            dst[code_idx] = code;
            code_idx = write_idx++;
            code = 1;
            read_idx++;
        } else {
            dst[write_idx++] = src[read_idx++];
            code++;
            if (code == 0xFF) {
                dst[code_idx] = code;
                code_idx = write_idx++;
                code = 1;
            }
        }
    }
    dst[code_idx] = code;

    return write_idx;
}

size_t cobs_decode(uint8_t *dst, size_t dst_size,
                   const uint8_t *src, size_t src_len) {
    if (dst == NULL || src == NULL || src_len == 0) {
        return 0;
    }

    size_t read_idx  = 0;
    size_t write_idx = 0;

    while (read_idx < src_len) {
        uint8_t code = src[read_idx++];
        if (code == 0x00) {
            return 0;  /* Zero byte in COBS stream is invalid */
        }

        for (uint8_t i = 1; i < code; i++) {
            if (read_idx >= src_len) {
                return 0;  /* Truncated */
            }
            if (write_idx >= dst_size) {
                return 0;  /* Output buffer overflow */
            }
            dst[write_idx++] = src[read_idx++];
        }

        /* If code < 0xFF, the original data had a zero byte here
         * (unless we've reached the end of the input). */
        if (code < 0xFF && read_idx < src_len) {
            if (write_idx >= dst_size) {
                return 0;
            }
            dst[write_idx++] = 0x00;
        }
    }

    return write_idx;
}
