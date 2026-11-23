/**
 * @file config_panel.h
 * @brief Fault configuration form and execution control panel
 */

#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "../model/fault_config.h"

namespace faulter {

class ConfigPanel : public QWidget {
    Q_OBJECT

public:
    explicit ConfigPanel(FaultConfig* config, QWidget* parent = nullptr);

    /** Update UI lock state according to device state. */
    void setDeviceState(uint8_t state);

    /** Set connection status to enable/disable controls. */
    void setConnected(bool connected);

    /** Set sweep progress status string. */
    void setSweepStatus(const QString& status);

    /** Set whether an automated 2D parameter sweep is currently running. */
    void setSweeping(bool sweeping);

signals:
    void armRequested();
    void fireRequested();
    void abortRequested();
    void resetRequested();

private slots:
    void onModelChanged();
    void onUiChanged();
    void onValidateInputs();
    void onFireButtonClicked();

private:
    void setupUi();
    void updateSpinboxesFromModel();
    void updateControlStates();
    uint64_t getNsValue(QSpinBox* spin, QComboBox* unitCombo);
    void setNsValue(uint32_t ns, QSpinBox* spin, QComboBox* unitCombo);
    uint64_t getNsValueFromDouble(QDoubleSpinBox* spin, QComboBox* unitCombo);
    void setDoubleNsValue(uint32_t ns, QDoubleSpinBox* spin, QComboBox* unitCombo);
    double getStepGapMs() const;
    void setStepGapMs(double ms);

    FaultConfig* m_config = nullptr;
    bool m_updatingFromModel = false;
    bool m_updatingFromUi = false;
    bool m_connected = false;
    bool m_isSweeping = false;
    uint8_t m_deviceState = STATE_IDLE;
    uint8_t m_lastVisualState = 0xFF; // tracks last state used for button styling

    // Glitch controls
    QComboBox* m_glitchTypeCombo = nullptr;
    QSpinBox*  m_delaySpin = nullptr;
    QComboBox* m_delayUnitCombo = nullptr;
    QSpinBox*  m_pulseSpin = nullptr;
    QComboBox* m_pulseUnitCombo = nullptr;

    // Burst controls
    QCheckBox* m_infiniteCheck = nullptr;
    QSpinBox*  m_burstGapSpin = nullptr;
    QLabel*    m_burstGapLabel = nullptr;
    QLabel*    m_burstGapUnit = nullptr;

    // Power cycling controls
    QCheckBox* m_pwrCycleCheck = nullptr;
    QSpinBox*  m_pwrCycleOffSpin = nullptr;

    // Pin controls
    QSpinBox*  m_glitchPinSpin = nullptr;
    QSpinBox*  m_pwrCyclePinSpin = nullptr;
    QSpinBox*  m_monitorPinSpin = nullptr;
    QCheckBox* m_monitorEnCheck = nullptr;

    // Control buttons
    QPushButton* m_armBtn = nullptr;
    QPushButton* m_fireBtn = nullptr;
    QPushButton* m_abortBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;

    // Automated parameter sweep (brute-force) controls
    QCheckBox*       m_sweepEnableCheck = nullptr;
    QWidget*         m_sweepContainer = nullptr;
    QDoubleSpinBox*  m_sweepWidthStartSpin = nullptr;
    QDoubleSpinBox*  m_sweepWidthEndSpin = nullptr;
    QDoubleSpinBox*  m_sweepWidthStepSpin = nullptr;
    QComboBox*       m_sweepWidthUnitCombo = nullptr;
    QDoubleSpinBox*  m_sweepDelayStartSpin = nullptr;
    QDoubleSpinBox*  m_sweepDelayEndSpin = nullptr;
    QDoubleSpinBox*  m_sweepDelayStepSpin = nullptr;
    QComboBox*       m_sweepDelayUnitCombo = nullptr;
    QDoubleSpinBox*  m_sweepStepGapSpin = nullptr;
    QComboBox*       m_sweepStepGapUnitCombo = nullptr;
    QLabel*          m_sweepStatusLabel = nullptr;

    // Inline validation error message
    QLabel* m_errorLabel = nullptr;
};

} // namespace faulter
