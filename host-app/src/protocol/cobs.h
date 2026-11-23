/**
 * @file cobs.h
 * @brief Host application COBS encode/decode functions (C++17)
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace faulter {

/**
 * @brief Encode a byte sequence with COBS.
 *
 * @param src Input unencoded bytes.
 * @return COBS-encoded byte vector (without trailing 0x00 delimiter).
 */
std::vector<uint8_t> cobsEncode(const std::vector<uint8_t>& src);

/**
 * @brief Decode a COBS-encoded byte sequence.
 *
 * @param src Encoded bytes (excluding trailing 0x00).
 * @param[out] dst Output decoded bytes.
 * @return True on success, false if corrupted.
 */
bool cobsDecode(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst);

} // namespace faulter
