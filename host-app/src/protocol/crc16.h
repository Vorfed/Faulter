/**
 * @file crc16.h
 * @brief CRC-16/CCITT for host application
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace faulter {

/**
 * @brief Compute CRC-16/CCITT over buffer (poly 0x1021, init 0xFFFF).
 */
uint16_t computeCrc16(const uint8_t* data, size_t len);

inline uint16_t computeCrc16(const std::vector<uint8_t>& data) {
    return computeCrc16(data.data(), data.size());
}

} // namespace faulter
