/**
 * @file fault_config.h
 * @brief Domain model storing glitch and hardware configuration
 */

#pragma once

#include <QObject>
#include <cstdint>
#include "../../../shared/protocol_defs.h"

namespace faulter {

class FaultConfig : public QObject {
    Q_OBJECT

    Q_PROPERTY(uint32_t glitchType READ glitchType WRITE setGlitchType NOTIFY configChanged)
    Q_PROPERTY(uint32_t delayNs READ delayNs WRITE setDelayNs NOTIFY configChanged)
    Q_PROPERTY(uint32_t pulseNs READ pulseNs WRITE setPulseNs NOTIFY configChanged)
    Q_PROPERTY(uint32_t repeatCount READ repeatCount WRITE setRepeatCount NOTIFY configChanged)
    Q_PROPERTY(uint32_t repeatDelayNs READ repeatDelayNs WRITE setRepeatDelayNs NOTIFY configChanged)
    Q_PROPERTY(bool pwrCycleEnabled READ pwrCycleEnabled WRITE setPwrCycleEnabled NOTIFY configChanged)
    Q_PROPERTY(uint32_t pwrCycleOffMs READ pwrCycleOffMs WRITE setPwrCycleOffMs NOTIFY configChanged)
    Q_PROPERTY(uint32_t glitchPin READ glitchPin WRITE setGlitchPin NOTIFY configChanged)
    Q_PROPERTY(uint32_t pwrCyclePin READ pwrCyclePin WRITE setPwrCyclePin NOTIFY configChanged)
    Q_PROPERTY(uint32_t monitorPin READ monitorPin WRITE setMonitorPin NOTIFY configChanged)
    Q_PROPERTY(bool monitorEnabled READ monitorEnabled WRITE setMonitorEnabled NOTIFY configChanged)
    Q_PROPERTY(bool burstInfinite READ burstInfinite WRITE setBurstInfinite NOTIFY configChanged)
    Q_PROPERTY(uint32_t burstGapMs READ burstGapMs WRITE setBurstGapMs NOTIFY configChanged)

public:
    explicit FaultConfig(QObject* parent = nullptr);

    uint32_t glitchType() const { return m_glitchType; }
    void setGlitchType(uint32_t val);

    uint32_t delayNs() const { return m_delayNs; }
    void setDelayNs(uint32_t val);

    uint32_t pulseNs() const { return m_pulseNs; }
    void setPulseNs(uint32_t val);

    uint32_t repeatCount() const { return m_repeatCount; }
    void setRepeatCount(uint32_t val);

    uint32_t repeatDelayNs() const { return m_repeatDelayNs; }
    void setRepeatDelayNs(uint32_t val);

    bool burstInfinite() const { return m_burstInfinite; }
    void setBurstInfinite(bool val);

    uint32_t burstGapMs() const { return m_burstGapMs; }
    void setBurstGapMs(uint32_t val);

    bool pwrCycleEnabled() const { return m_pwrCycleEnabled; }
    void setPwrCycleEnabled(bool val);

    uint32_t pwrCycleOffMs() const { return m_pwrCycleOffMs; }
    void setPwrCycleOffMs(uint32_t val);

    uint32_t glitchPin() const { return m_glitchPin; }
    void setGlitchPin(uint32_t val);

    uint32_t pwrCyclePin() const { return m_pwrCyclePin; }
    void setPwrCyclePin(uint32_t val);

    uint32_t monitorPin() const { return m_monitorPin; }
    void setMonitorPin(uint32_t val);

    bool monitorEnabled() const { return m_monitorEnabled; }
    void setMonitorEnabled(bool val);

    // Brute-force parameter sweep configuration
    bool sweepEnabled() const { return m_sweepEnabled; }
    void setSweepEnabled(bool val);

    uint32_t sweepWidthStartNs() const { return m_sweepWidthStartNs; }
    void setSweepWidthStartNs(uint32_t val);

    uint32_t sweepWidthEndNs() const { return m_sweepWidthEndNs; }
    void setSweepWidthEndNs(uint32_t val);

    uint32_t sweepWidthStepNs() const { return m_sweepWidthStepNs; }
    void setSweepWidthStepNs(uint32_t val);

    uint32_t sweepDelayStartNs() const { return m_sweepDelayStartNs; }
    void setSweepDelayStartNs(uint32_t val);

    uint32_t sweepDelayEndNs() const { return m_sweepDelayEndNs; }
    void setSweepDelayEndNs(uint32_t val);

    uint32_t sweepDelayStepNs() const { return m_sweepDelayStepNs; }
    void setSweepDelayStepNs(uint32_t val);

    double sweepStepGapMs() const { return m_sweepStepGapMs; }
    void setSweepStepGapMs(double val);

    /** Reset all parameters to factory defaults. */
    void resetToDefaults();

signals:
    void configChanged();

private:
    uint32_t m_glitchType = DEFAULT_GLITCH_TYPE;
    uint32_t m_delayNs = DEFAULT_DELAY_NS;
    uint32_t m_pulseNs = DEFAULT_PULSE_NS;
    uint32_t m_repeatCount = DEFAULT_REPEAT_COUNT;
    uint32_t m_repeatDelayNs = DEFAULT_REPEAT_DELAY_NS;
    bool     m_pwrCycleEnabled = (DEFAULT_PWR_CYCLE_EN != 0);
    uint32_t m_pwrCycleOffMs = DEFAULT_PWR_CYCLE_OFF_MS;
    uint32_t m_glitchPin = DEFAULT_GLITCH_PIN;
    uint32_t m_pwrCyclePin = DEFAULT_PWR_CYCLE_PIN;
    uint32_t m_monitorPin = DEFAULT_MONITOR_PIN;
    bool     m_monitorEnabled = (DEFAULT_MONITOR_EN != 0);
    bool     m_burstInfinite = (DEFAULT_BURST_INFINITE != 0);
    uint32_t m_burstGapMs = DEFAULT_BURST_GAP_MS;

    // Brute-force sweep defaults
    bool     m_sweepEnabled = false;
    uint32_t m_sweepWidthStartNs = 5;
    uint32_t m_sweepWidthEndNs = 100;
    uint32_t m_sweepWidthStepNs = 5;
    uint32_t m_sweepDelayStartNs = 0;
    uint32_t m_sweepDelayEndNs = 500;
    uint32_t m_sweepDelayStepNs = 10;
    double   m_sweepStepGapMs = 50.0;
};

} // namespace faulter
