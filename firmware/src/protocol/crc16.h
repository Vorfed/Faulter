/**
 * @file crc16.h
 * @brief CRC-16/CCITT (polynomial 0x1021, initial value 0xFFFF)
 */

#ifndef FAULTER_CRC16_H
#define FAULTER_CRC16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Compute CRC-16/CCITT over a data buffer.
 *
 * Uses a 256-entry lookup table for speed.
 *
 * @param data   Pointer to data bytes.
 * @param len    Number of bytes.
 * @return CRC-16 value.
 */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

/**
 * @brief Incrementally update a CRC-16/CCITT value with one byte.
 *
 * @param crc    Current CRC value (start with CRC16_INITIAL).
 * @param byte   The next data byte.
 * @return Updated CRC value.
 */
uint16_t crc16_ccitt_update(uint16_t crc, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* FAULTER_CRC16_H */
