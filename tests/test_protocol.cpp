/**
 * @file test_protocol.cpp
 * @brief Comprehensive unit tests for COBS, CRC-16, Protocol, and Validation
 */

#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

#include "../shared/protocol_defs.h"
#include "../host-app/src/protocol/cobs.h"
#include "../host-app/src/protocol/crc16.h"
#include "../host-app/src/model/param_validator.h"

// Firmware C headers to test cross-compatibility
extern "C" {
#include "../firmware/src/protocol/cobs.h"
#include "../firmware/src/protocol/crc16.h"
#include "../firmware/src/protocol/protocol.h"
}

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while(0)

bool test_crc16() {
    std::cout << "[TEST] CRC-16/CCITT verification... ";

    // Test vector: "123456789" -> 0x29B1 for CRC-16/CCITT (poly 0x1021, init 0xFFFF)
    const uint8_t test_vec[] = "123456789";
    uint16_t crc_c = crc16_ccitt(test_vec, 9);
    uint16_t crc_cpp = faulter::computeCrc16(test_vec, 9);

    TEST_ASSERT(crc_c == 0x29B1, "C crc16 matches standard test vector 0x29B1");
    TEST_ASSERT(crc_cpp == 0x29B1, "C++ crc16 matches standard test vector 0x29B1");
    TEST_ASSERT(crc_c == crc_cpp, "C and C++ implementations yield identical CRC");

    std::cout << "PASSED (CRC = 0x" << std::hex << crc_c << std::dec << ")\n";
    return true;
}

bool test_cobs_roundtrip() {
    std::cout << "[TEST] COBS encode/decode roundtrip... ";

    // Test cases with various patterns of zeros and nonzero bytes
    std::vector<std::vector<uint8_t>> test_cases = {
        { 0x01, 0x02, 0x03, 0x04 },
        { 0x00 },
        { 0x00, 0x00 },
        { 0x11, 0x22, 0x00, 0x33, 0x44 },
        { 0x00, 0x11, 0x00, 0x22, 0x00 },
        { 0x01, 0x00 },
        { 0x00, 0x01 }
    };

    // Add a 255-byte test case
    std::vector<uint8_t> large_vec;
    for (int i = 0; i < 255; ++i) large_vec.push_back(static_cast<uint8_t>(i % 256));
    test_cases.push_back(large_vec);

    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto& original = test_cases[i];

        // C++ version
        std::vector<uint8_t> encoded_cpp = faulter::cobsEncode(original);
        // Verify no 0x00 in encoded stream
        for (uint8_t b : encoded_cpp) {
            TEST_ASSERT(b != 0x00, "COBS encoded stream must never contain 0x00");
        }
        std::vector<uint8_t> decoded_cpp;
        bool ok = faulter::cobsDecode(encoded_cpp, decoded_cpp);
        TEST_ASSERT(ok, "C++ COBS decode must succeed");
        TEST_ASSERT(decoded_cpp == original, "Decoded data must match original");

        // C firmware version
        uint8_t c_enc[512];
        uint8_t c_dec[512];
        size_t c_enc_len = cobs_encode(c_enc, sizeof(c_enc), original.data(), original.size());
        TEST_ASSERT(c_enc_len > 0, "C COBS encode must return > 0");

        for (size_t j = 0; j < c_enc_len; ++j) {
            TEST_ASSERT(c_enc[j] != 0x00, "C COBS encoded stream must never contain 0x00");
        }

        size_t c_dec_len = cobs_decode(c_dec, sizeof(c_dec), c_enc, c_enc_len);
        TEST_ASSERT(c_dec_len == original.size(), "C decoded length must match original");
        TEST_ASSERT(std::memcmp(c_dec, original.data(), original.size()) == 0,
                    "C decoded bytes must match original");

        // Cross check: C++ decodes C, C decodes C++
        std::vector<uint8_t> cross_from_c;
        std::vector<uint8_t> c_enc_vec(c_enc, c_enc + c_enc_len);
        TEST_ASSERT(faulter::cobsDecode(c_enc_vec, cross_from_c), "C++ must decode C-encoded stream");
        TEST_ASSERT(cross_from_c == original, "Cross decoded from C must match");

        size_t cross_from_cpp_len = cobs_decode(c_dec, sizeof(c_dec), encoded_cpp.data(), encoded_cpp.size());
        TEST_ASSERT(cross_from_cpp_len == original.size(), "C must decode C++-encoded stream");
        TEST_ASSERT(std::memcmp(c_dec, original.data(), original.size()) == 0, "Cross decoded from C++ must match");
    }

    std::cout << "PASSED (" << test_cases.size() << " vectors)\n";
    return true;
}

bool test_protocol_packets() {
    std::cout << "[TEST] Protocol packet serialization and parsing... ";

    // Test PONG builder and parser
    uint8_t frame_buf[FAULTER_MAX_FRAME_SIZE + 4];
    size_t frame_len = protocol_build_pong(frame_buf, sizeof(frame_buf), 0x42);
    TEST_ASSERT(frame_len > 0, "protocol_build_pong returned > 0");
    TEST_ASSERT(frame_buf[frame_len - 1] == FAULTER_FRAME_DELIMITER, "Trailing delimiter must be 0x00");

    // Decode COBS
    uint8_t dec_buf[128];
    size_t dec_len = cobs_decode(dec_buf, sizeof(dec_buf), frame_buf, frame_len - 1);
    TEST_ASSERT(dec_len >= PACKET_MIN_SIZE, "Decoded size must be >= PACKET_MIN_SIZE");

    packet_t pkt;
    int err = protocol_parse_packet(dec_buf, dec_len, &pkt);
    TEST_ASSERT(err == ERR_NONE, "protocol_parse_packet returned ERR_NONE");
    TEST_ASSERT(pkt.type == PKT_RSP_PONG, "Packet type must be PKT_RSP_PONG");
    TEST_ASSERT(pkt.seq == 0x42, "Sequence number must be 0x42");
    TEST_ASSERT(pkt.payload_len == 7, "PONG payload length must be 7");
    TEST_ASSERT(pkt.payload[0] == FAULTER_PROTO_VERSION_MAJOR, "Proto major version matches");

    // Test STATUS builder and parser
    frame_len = protocol_build_status(frame_buf, sizeof(frame_buf), 0x10,
                                     STATE_ARMED, ERR_NONE, 1250, 48);
    TEST_ASSERT(frame_len > 0, "protocol_build_status returned > 0");
    dec_len = cobs_decode(dec_buf, sizeof(dec_buf), frame_buf, frame_len - 1);
    err = protocol_parse_packet(dec_buf, dec_len, &pkt);
    TEST_ASSERT(err == ERR_NONE, "STATUS parse ok");
    TEST_ASSERT(pkt.type == PKT_RSP_STATUS, "Packet type is STATUS");
    TEST_ASSERT(pkt.payload[0] == STATE_ARMED, "State in payload is ARMED");

    // Test corrupted CRC detection
    dec_buf[dec_len - 1] ^= 0xFF; // Corrupt last CRC byte
    err = protocol_parse_packet(dec_buf, dec_len, &pkt);
    TEST_ASSERT(err == ERR_CRC_MISMATCH, "Tampered frame must return ERR_CRC_MISMATCH");

    std::cout << "PASSED\n";
    return true;
}

bool test_param_validation() {
    std::cout << "[TEST] Parameter validation rules... ";

    // Pulse width
    auto res = faulter::ParamValidator::validatePulse(4);
    TEST_ASSERT(res.isValid, "4 ns pulse is valid (minimum 1 PIO cycle @ 250 MHz)");

    res = faulter::ParamValidator::validatePulse(3);
    TEST_ASSERT(!res.isValid, "3 ns pulse must be rejected (< 4 ns)");

    res = faulter::ParamValidator::validatePulse(100);
    TEST_ASSERT(res.isValid, "100 ns pulse is valid");

    // Repeat count
    res = faulter::ParamValidator::validateRepeatCount(1);
    TEST_ASSERT(res.isValid, "Repeat count 1 is valid");

    res = faulter::ParamValidator::validateRepeatCount(0);
    TEST_ASSERT(!res.isValid, "Repeat count 0 must be rejected");

    res = faulter::ParamValidator::validateRepeatCount(65535);
    TEST_ASSERT(res.isValid, "Repeat count 65535 is valid");

    res = faulter::ParamValidator::validateRepeatCount(65536);
    TEST_ASSERT(!res.isValid, "Repeat count 65536 must be rejected (> 65535)");

    // Burst gap
    res = faulter::ParamValidator::validateBurstGap(100);
    TEST_ASSERT(res.isValid, "100 ms burst gap is valid");

    res = faulter::ParamValidator::validateBurstGap(60000);
    TEST_ASSERT(res.isValid, "60000 ms burst gap is valid");

    res = faulter::ParamValidator::validateBurstGap(60001);
    TEST_ASSERT(!res.isValid, "60001 ms burst gap must be rejected (> 60000 ms)");

    // Power cycle off time
    res = faulter::ParamValidator::validatePwrCycleOff(50);
    TEST_ASSERT(res.isValid, "50 ms power off is valid");

    res = faulter::ParamValidator::validatePwrCycleOff(0);
    TEST_ASSERT(!res.isValid, "0 ms power off must be rejected");

    // GPIO pins
    res = faulter::ParamValidator::validatePin(19, "Glitch");
    TEST_ASSERT(res.isValid, "GPIO 19 is valid");

    res = faulter::ParamValidator::validatePin(29, "Glitch");
    TEST_ASSERT(!res.isValid, "GPIO 29 must be rejected (max 28)");

    std::cout << "PASSED\n";
    return true;
}

int main() {
    std::cout << "==================================================\n";
    std::cout << "   Faulter Control Suite — Unit Test Suite\n";
    std::cout << "==================================================\n";

    bool all_passed = true;
    all_passed &= test_crc16();
    all_passed &= test_cobs_roundtrip();
    all_passed &= test_protocol_packets();
    all_passed &= test_param_validation();

    std::cout << "==================================================\n";
    if (all_passed) {
        std::cout << "   ALL UNIT TESTS PASSED SUCCESSFULLY! (4/4)\n";
        std::cout << "==================================================\n";
        return 0;
    } else {
        std::cerr << "   SOME TESTS FAILED!\n";
        std::cout << "==================================================\n";
        return 1;
    }
}
