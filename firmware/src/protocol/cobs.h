/**
 * @file cobs.h
 * @brief COBS (Consistent Overhead Byte Stuffing) encode/decode
 *
 * No dynamic allocation.  Operates on caller-supplied fixed buffers.
 */

#ifndef FAULTER_COBS_H
#define FAULTER_COBS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @brief COBS-encode a buffer.
 *
 * @param dst        Output buffer (must be at least src_len + ceil(src_len/254) + 1 bytes).
 * @param dst_size   Size of the output buffer.
 * @param src        Input data to encode (may contain zero bytes).
 * @param src_len    Length of input data.
 * @return Number of bytes written to dst (excluding the trailing 0x00 delimiter),
 *         or 0 on error (dst too small).
 */
size_t cobs_encode(uint8_t *dst, size_t dst_size,
                   const uint8_t *src, size_t src_len);

/**
 * @brief COBS-decode a buffer.
 *
 * @param dst        Output buffer (must be at least src_len bytes).
 * @param dst_size   Size of the output buffer.
 * @param src        COBS-encoded input (without the trailing 0x00 delimiter).
 * @param src_len    Length of encoded input.
 * @return Number of decoded bytes written to dst, or 0 on error.
 */
size_t cobs_decode(uint8_t *dst, size_t dst_size,
                   const uint8_t *src, size_t src_len);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_COBS_H */
