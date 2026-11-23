/**
 * @file param_validator.cpp
 * @brief Fault injection parameter validation implementation
 */

#include "param_validator.h"

namespace faulter {

ValidationResult ParamValidator::validateDelay(uint32_t delayNs) {
    Q_UNUSED(delayNs);
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validatePulse(uint32_t pulseNs) {
    if (pulseNs < MIN_PULSE_NS) {
        return ValidationResult::error(QString("Pulse width must be >= %1 ns (1 PIO clock cycle)").arg(MIN_PULSE_NS));
    }
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validateRepeatCount(uint32_t repeatCount) {
    if (repeatCount < 1 || repeatCount > MAX_REPEAT_COUNT) {
        return ValidationResult::error(QString("Repeat count must be between 1 and %1").arg(MAX_REPEAT_COUNT));
    }
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validateRepeatDelay(uint32_t repeatDelayNs) {
    Q_UNUSED(repeatDelayNs);
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validateBurstGap(uint32_t gapMs) {
    if (gapMs > MAX_BURST_GAP_MS) {
        return ValidationResult::error(QString("Burst gap must be <= %1 ms").arg(MAX_BURST_GAP_MS));
    }
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validatePwrCycleOff(uint32_t offMs) {
    if (offMs < 1 || offMs > MAX_PWR_CYCLE_OFF_MS) {
        return ValidationResult::error(QString("Power-off duration must be between 1 and %1 ms").arg(MAX_PWR_CYCLE_OFF_MS));
    }
    return ValidationResult::ok();
}

ValidationResult ParamValidator::validatePin(uint32_t pin, const QString& pinName) {
    if (pin > MAX_GPIO_PIN) {
        return ValidationResult::error(QString("%1 GPIO pin must be between 0 and %2").arg(pinName).arg(MAX_GPIO_PIN));
    }
    return ValidationResult::ok();
}

} // namespace faulter
