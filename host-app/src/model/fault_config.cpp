/**
 * @file fault_config.cpp
 * @brief Domain model implementation
 */

#include "fault_config.h"

namespace faulter {

FaultConfig::FaultConfig(QObject* parent)
    : QObject(parent)
{
}

void FaultConfig::setGlitchType(uint32_t val) {
    if (m_glitchType != val) {
        m_glitchType = val;
        emit configChanged();
    }
}

void FaultConfig::setDelayNs(uint32_t val) {
    if (m_delayNs != val) {
        m_delayNs = val;
        emit configChanged();
    }
}

void FaultConfig::setPulseNs(uint32_t val) {
    if (m_pulseNs != val) {
        m_pulseNs = val;
        emit configChanged();
    }
}

void FaultConfig::setRepeatCount(uint32_t val) {
    if (m_repeatCount != val) {
        m_repeatCount = val;
        emit configChanged();
    }
}

void FaultConfig::setRepeatDelayNs(uint32_t val) {
    if (m_repeatDelayNs != val) {
        m_repeatDelayNs = val;
        emit configChanged();
    }
}

void FaultConfig::setBurstInfinite(bool val) {
    if (m_burstInfinite != val) {
        m_burstInfinite = val;
        emit configChanged();
    }
}

void FaultConfig::setBurstGapMs(uint32_t val) {
    if (m_burstGapMs != val) {
        m_burstGapMs = val;
        emit configChanged();
    }
}

void FaultConfig::setPwrCycleEnabled(bool val) {
    if (m_pwrCycleEnabled != val) {
        m_pwrCycleEnabled = val;
        emit configChanged();
    }
}

void FaultConfig::setPwrCycleOffMs(uint32_t val) {
    if (m_pwrCycleOffMs != val) {
        m_pwrCycleOffMs = val;
        emit configChanged();
    }
}

void FaultConfig::setGlitchPin(uint32_t val) {
    if (m_glitchPin != val) {
        m_glitchPin = val;
        emit configChanged();
    }
}

void FaultConfig::setPwrCyclePin(uint32_t val) {
    if (m_pwrCyclePin != val) {
        m_pwrCyclePin = val;
        emit configChanged();
    }
}

void FaultConfig::setMonitorPin(uint32_t val) {
    if (m_monitorPin != val) {
        m_monitorPin = val;
        emit configChanged();
    }
}

void FaultConfig::setMonitorEnabled(bool val) {
    if (m_monitorEnabled != val) {
        m_monitorEnabled = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepEnabled(bool val) {
    if (m_sweepEnabled != val) {
        m_sweepEnabled = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepWidthStartNs(uint32_t val) {
    if (m_sweepWidthStartNs != val) {
        m_sweepWidthStartNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepWidthEndNs(uint32_t val) {
    if (m_sweepWidthEndNs != val) {
        m_sweepWidthEndNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepWidthStepNs(uint32_t val) {
    if (m_sweepWidthStepNs != val) {
        m_sweepWidthStepNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepDelayStartNs(uint32_t val) {
    if (m_sweepDelayStartNs != val) {
        m_sweepDelayStartNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepDelayEndNs(uint32_t val) {
    if (m_sweepDelayEndNs != val) {
        m_sweepDelayEndNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepDelayStepNs(uint32_t val) {
    if (m_sweepDelayStepNs != val) {
        m_sweepDelayStepNs = val;
        emit configChanged();
    }
}

void FaultConfig::setSweepStepGapMs(double val) {
    if (val < 0.001) val = 0.001;
    if (m_sweepStepGapMs != val) {
        m_sweepStepGapMs = val;
        emit configChanged();
    }
}

void FaultConfig::resetToDefaults() {
    m_glitchType       = DEFAULT_GLITCH_TYPE;
    m_delayNs          = DEFAULT_DELAY_NS;
    m_pulseNs          = DEFAULT_PULSE_NS;
    m_repeatCount      = DEFAULT_REPEAT_COUNT;
    m_repeatDelayNs    = DEFAULT_REPEAT_DELAY_NS;
    m_pwrCycleEnabled  = (DEFAULT_PWR_CYCLE_EN != 0);
    m_pwrCycleOffMs    = DEFAULT_PWR_CYCLE_OFF_MS;
    m_glitchPin        = DEFAULT_GLITCH_PIN;
    m_pwrCyclePin      = DEFAULT_PWR_CYCLE_PIN;
    m_monitorPin       = DEFAULT_MONITOR_PIN;
    m_monitorEnabled   = (DEFAULT_MONITOR_EN != 0);
    m_burstInfinite    = (DEFAULT_BURST_INFINITE != 0);
    m_burstGapMs       = DEFAULT_BURST_GAP_MS;

    m_sweepEnabled     = false;
    m_sweepWidthStartNs = 5;
    m_sweepWidthEndNs  = 100;
    m_sweepWidthStepNs = 5;
    m_sweepDelayStartNs = 0;
    m_sweepDelayEndNs  = 500;
    m_sweepDelayStepNs = 10;
    m_sweepStepGapMs   = 50.0;

    emit configChanged();
}

} // namespace faulter
