/**
 * @file param_validator.h
 * @brief Fault injection parameter validation logic
 */

#pragma once

#include <QString>
#include <cstdint>
#include "../../../shared/protocol_defs.h"

namespace faulter {

struct ValidationResult {
    bool isValid = true;
    QString errorMessage;

    static ValidationResult ok() { return {true, QString()}; }
    static ValidationResult error(const QString& msg) { return {false, msg}; }
};

class ParamValidator {
public:
    static ValidationResult validateDelay(uint32_t delayNs);
    static ValidationResult validatePulse(uint32_t pulseNs);
    static ValidationResult validateRepeatCount(uint32_t repeatCount);
    static ValidationResult validateRepeatDelay(uint32_t repeatDelayNs);
    static ValidationResult validateBurstGap(uint32_t gapMs);
    static ValidationResult validatePwrCycleOff(uint32_t offMs);
    static ValidationResult validatePin(uint32_t pin, const QString& pinName);
};

} // namespace faulter
