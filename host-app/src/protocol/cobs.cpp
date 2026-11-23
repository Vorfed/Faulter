/**
 * @file cobs.cpp
 * @brief Host application COBS encode/decode implementation
 */

#include "cobs.h"

namespace faulter {

std::vector<uint8_t> cobsEncode(const std::vector<uint8_t>& src) {
    std::vector<uint8_t> dst;
    if (src.empty()) {
        dst.push_back(1);
        return dst;
    }

    dst.resize(src.size() + (src.size() / 254) + 2);

    size_t read_idx  = 0;
    size_t write_idx = 0;
    size_t code_idx  = write_idx++;
    uint8_t code     = 1;

    while (read_idx < src.size()) {
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
    dst.resize(write_idx);
    return dst;
}

bool cobsDecode(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst) {
    dst.clear();
    if (src.empty()) {
        return false;
    }

    size_t read_idx = 0;
    while (read_idx < src.size()) {
        uint8_t code = src[read_idx++];
        if (code == 0x00) {
            return false;
        }

        for (uint8_t i = 1; i < code; ++i) {
            if (read_idx >= src.size()) {
                return false;
            }
            dst.push_back(src[read_idx++]);
        }

        if (code < 0xFF && read_idx < src.size()) {
            dst.push_back(0x00);
        }
    }
    return true;
}

} // namespace faulter
