/**
 * @file config_panel.cpp
 * @brief Fault configuration form and execution control panel implementation
 */

#include "config_panel.h"
#include "../model/param_validator.h"

#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QRegularExpression>

namespace faulter {

ConfigPanel::ConfigPanel(FaultConfig* config, QWidget* parent)
    : QWidget(parent)
    , m_config(config)
{
    setupUi();
    connect(m_config, &FaultConfig::configChanged, this, &ConfigPanel::onModelChanged);
    updateSpinboxesFromModel();
    onValidateInputs();
    updateControlStates();
}

void ConfigPanel::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(10);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto* container = new QWidget(scrollArea);
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(2, 2, 8, 2);
    containerLayout->setSpacing(12);

    // ── 1. Fault Injection Mechanism Card ──────────────────────────────
    auto* glitchGroup = new QGroupBox("FAULT INJECTION MECHANISM", container);
    auto* glitchLayout = new QGridLayout(glitchGroup);
    glitchLayout->setContentsMargins(10, 14, 10, 10);
    glitchLayout->setHorizontalSpacing(8);
    glitchLayout->setVerticalSpacing(10);
    glitchLayout->setColumnStretch(0, 0);
    glitchLayout->setColumnStretch(1, 1);
    glitchLayout->setColumnStretch(2, 0);
    glitchLayout->setColumnMinimumWidth(2, 56);

    auto* typeLbl = new QLabel("Fault Mechanism:", glitchGroup);
    typeLbl->setObjectName("fieldLabel");
    glitchLayout->addWidget(typeLbl, 0, 0);

    m_glitchTypeCombo = new QComboBox(glitchGroup);
    m_glitchTypeCombo->addItem("Voltage Crowbar (Short Rail to GND)", GLITCH_TYPE_VOLTAGE);
    m_glitchTypeCombo->addItem("Clock Glitching (RP2040 Reserved)", GLITCH_TYPE_CLOCK);
    m_glitchTypeCombo->setItemData(1, false, Qt::UserRole - 1);
    glitchLayout->addWidget(m_glitchTypeCombo, 0, 1, 1, 2);

    containerLayout->addWidget(glitchGroup);

    // ── 2. Automated Parameter Sweep (Brute Force Search) ───────────────
    auto* sweepGroup = new QGroupBox("PARAMETER SWEEP (BRUTE-FORCE SEARCH)", container);
    auto* sweepOuterLayout = new QVBoxLayout(sweepGroup);
    sweepOuterLayout->setContentsMargins(10, 14, 10, 10);
    sweepOuterLayout->setSpacing(8);

    m_sweepEnableCheck = new QCheckBox("Enable Automated 2D Parameter Sweep", sweepGroup);
    sweepOuterLayout->addWidget(m_sweepEnableCheck);

    m_sweepContainer = new QWidget(sweepGroup);
    auto* sweepLayout = new QVBoxLayout(m_sweepContainer);
    sweepLayout->setContentsMargins(0, 4, 0, 0);
    sweepLayout->setSpacing(10);

    // ── Row 1: Pulse Width Range ──────────────────────────────────────
    auto* widthBox = new QFrame(m_sweepContainer);
    widthBox->setStyleSheet("QFrame { background-color: #161b22; border: 1px solid #30363d; border-radius: 6px; }");
    auto* widthVLayout = new QVBoxLayout(widthBox);
    widthVLayout->setContentsMargins(8, 8, 8, 8);
    widthVLayout->setSpacing(6);

    auto* widthTopRow = new QHBoxLayout();
    auto* widthHeader = new QLabel("PULSE WIDTH RANGE", widthBox);
    widthHeader->setStyleSheet("color: #58a6ff; font-size: 11px; font-weight: 700; text-transform: uppercase;");
    widthTopRow->addWidget(widthHeader);
    widthTopRow->addStretch(1);

    auto* wUnitLbl = new QLabel("Unit:", widthBox);
    wUnitLbl->setStyleSheet("color: #8b949e; font-size: 11px;");
    widthTopRow->addWidget(wUnitLbl);

    m_sweepWidthUnitCombo = new QComboBox(widthBox);
    m_sweepWidthUnitCombo->addItems({"ns", "µs", "ms"});
    m_sweepWidthUnitCombo->setFixedWidth(68);
    widthTopRow->addWidget(m_sweepWidthUnitCombo);
    widthVLayout->addLayout(widthTopRow);

    auto* widthGridLayout = new QGridLayout();
    widthGridLayout->setContentsMargins(0, 0, 0, 0);
    widthGridLayout->setHorizontalSpacing(8);
    widthGridLayout->setVerticalSpacing(4);

    auto* wStartLbl = new QLabel("Start Width", widthBox);
    wStartLbl->setObjectName("fieldLabel");
    auto* wEndLbl = new QLabel("End Width", widthBox);
    wEndLbl->setObjectName("fieldLabel");
    auto* wStepLbl = new QLabel("Step (+Δ)", widthBox);
    wStepLbl->setObjectName("fieldLabel");
    wStepLbl->setStyleSheet("color: #3fb950; font-weight: bold; font-size: 11px;");

    widthGridLayout->addWidget(wStartLbl, 0, 0);
    widthGridLayout->addWidget(wEndLbl, 0, 1);
    widthGridLayout->addWidget(wStepLbl, 0, 2);

    m_sweepWidthStartSpin = new QDoubleSpinBox(widthBox);
    m_sweepWidthStartSpin->setDecimals(3);
    m_sweepWidthStartSpin->setRange(0.001, 1000000.0);
    m_sweepWidthStartSpin->setValue(5.0);
    m_sweepWidthStartSpin->setSingleStep(1.0);
    m_sweepWidthStartSpin->setSuffix(" ns");
    m_sweepWidthStartSpin->setMinimumWidth(80);
    widthGridLayout->addWidget(m_sweepWidthStartSpin, 1, 0);

    m_sweepWidthEndSpin = new QDoubleSpinBox(widthBox);
    m_sweepWidthEndSpin->setDecimals(3);
    m_sweepWidthEndSpin->setRange(0.001, 1000000.0);
    m_sweepWidthEndSpin->setValue(100.0);
    m_sweepWidthEndSpin->setSingleStep(1.0);
    m_sweepWidthEndSpin->setSuffix(" ns");
    m_sweepWidthEndSpin->setMinimumWidth(80);
    widthGridLayout->addWidget(m_sweepWidthEndSpin, 1, 1);

    m_sweepWidthStepSpin = new QDoubleSpinBox(widthBox);
    m_sweepWidthStepSpin->setDecimals(3);
    m_sweepWidthStepSpin->setRange(0.001, 100000.0);
    m_sweepWidthStepSpin->setValue(5.0);
    m_sweepWidthStepSpin->setSingleStep(1.0);
    m_sweepWidthStepSpin->setSuffix(" ns");
    m_sweepWidthStepSpin->setMinimumWidth(80);
    widthGridLayout->addWidget(m_sweepWidthStepSpin, 1, 2);

    widthVLayout->addLayout(widthGridLayout);
    sweepLayout->addWidget(widthBox);

    // ── Row 2: Delay / Timeout Sweep Range ────────────────────────────
    auto* delayBox = new QFrame(m_sweepContainer);
    delayBox->setStyleSheet("QFrame { background-color: #161b22; border: 1px solid #30363d; border-radius: 6px; }");
    auto* delayVLayout = new QVBoxLayout(delayBox);
    delayVLayout->setContentsMargins(8, 8, 8, 8);
    delayVLayout->setSpacing(6);

    auto* delayTopRow = new QHBoxLayout();
    auto* delayHeader = new QLabel("DELAY / TIMEOUT RANGE", delayBox);
    delayHeader->setStyleSheet("color: #d29922; font-size: 11px; font-weight: 700; text-transform: uppercase;");
    delayTopRow->addWidget(delayHeader);
    delayTopRow->addStretch(1);

    auto* dUnitLbl = new QLabel("Unit:", delayBox);
    dUnitLbl->setStyleSheet("color: #8b949e; font-size: 11px;");
    delayTopRow->addWidget(dUnitLbl);

    m_sweepDelayUnitCombo = new QComboBox(delayBox);
    m_sweepDelayUnitCombo->addItems({"ns", "µs", "ms"});
    m_sweepDelayUnitCombo->setFixedWidth(68);
    delayTopRow->addWidget(m_sweepDelayUnitCombo);
    delayVLayout->addLayout(delayTopRow);

    auto* delayGridLayout = new QGridLayout();
    delayGridLayout->setContentsMargins(0, 0, 0, 0);
    delayGridLayout->setHorizontalSpacing(8);
    delayGridLayout->setVerticalSpacing(4);

    auto* dStartLbl = new QLabel("Start Delay", delayBox);
    dStartLbl->setObjectName("fieldLabel");
    auto* dEndLbl = new QLabel("End Delay", delayBox);
    dEndLbl->setObjectName("fieldLabel");
    auto* dStepLbl = new QLabel("Step (+Δ)", delayBox);
    dStepLbl->setObjectName("fieldLabel");
    dStepLbl->setStyleSheet("color: #3fb950; font-weight: bold; font-size: 11px;");

    delayGridLayout->addWidget(dStartLbl, 0, 0);
    delayGridLayout->addWidget(dEndLbl, 0, 1);
    delayGridLayout->addWidget(dStepLbl, 0, 2);

    m_sweepDelayStartSpin = new QDoubleSpinBox(delayBox);
    m_sweepDelayStartSpin->setDecimals(3);
    m_sweepDelayStartSpin->setRange(0.0, 1000000000.0);
    m_sweepDelayStartSpin->setValue(0.0);
    m_sweepDelayStartSpin->setSingleStep(10.0);
    m_sweepDelayStartSpin->setSuffix(" ns");
    m_sweepDelayStartSpin->setMinimumWidth(80);
    delayGridLayout->addWidget(m_sweepDelayStartSpin, 1, 0);

    m_sweepDelayEndSpin = new QDoubleSpinBox(delayBox);
    m_sweepDelayEndSpin->setDecimals(3);
    m_sweepDelayEndSpin->setRange(0.0, 1000000000.0);
    m_sweepDelayEndSpin->setValue(500.0);
    m_sweepDelayEndSpin->setSingleStep(10.0);
    m_sweepDelayEndSpin->setSuffix(" ns");
    m_sweepDelayEndSpin->setMinimumWidth(80);
    delayGridLayout->addWidget(m_sweepDelayEndSpin, 1, 1);

    m_sweepDelayStepSpin = new QDoubleSpinBox(delayBox);
    m_sweepDelayStepSpin->setDecimals(3);
    m_sweepDelayStepSpin->setRange(0.001, 100000.0);
    m_sweepDelayStepSpin->setValue(10.0);
    m_sweepDelayStepSpin->setSingleStep(1.0);
    m_sweepDelayStepSpin->setSuffix(" ns");
    m_sweepDelayStepSpin->setMinimumWidth(80);
    delayGridLayout->addWidget(m_sweepDelayStepSpin, 1, 2);

    delayVLayout->addLayout(delayGridLayout);
    sweepLayout->addWidget(delayBox);

    // ── Row 3: Step Interval / Pause ──────────────────────────────────
    auto* gapRow = new QHBoxLayout();
    gapRow->setSpacing(8);
    auto* gapLbl2 = new QLabel("Step Interval / Pause:", m_sweepContainer);
    gapLbl2->setObjectName("fieldLabel");
    gapRow->addWidget(gapLbl2);

    m_sweepStepGapSpin = new QDoubleSpinBox(m_sweepContainer);
    m_sweepStepGapSpin->setDecimals(3);
    m_sweepStepGapSpin->setRange(0.001, 60000.0);
    m_sweepStepGapSpin->setValue(50.0);
    m_sweepStepGapSpin->setSingleStep(1.0);
    m_sweepStepGapSpin->setMinimumWidth(100);
    gapRow->addWidget(m_sweepStepGapSpin);

    m_sweepStepGapUnitCombo = new QComboBox(m_sweepContainer);
    m_sweepStepGapUnitCombo->addItems({"µs", "ms", "s"});
    m_sweepStepGapUnitCombo->setCurrentIndex(1); // default "ms"
    m_sweepStepGapUnitCombo->setFixedWidth(68);
    gapRow->addWidget(m_sweepStepGapUnitCombo);

    gapRow->addStretch(1);
    sweepLayout->addLayout(gapRow);

    // ── Row 4: Status ─────────────────────────────────────────────────
    m_sweepStatusLabel = new QLabel("Ready. Sweep advances width; timeout steps once on wrap.", m_sweepContainer);
    m_sweepStatusLabel->setStyleSheet("color: #8b949e; font-size: 11px; font-weight: 600; padding-top: 4px;");
    m_sweepStatusLabel->setWordWrap(true);
    sweepLayout->addWidget(m_sweepStatusLabel);

    m_sweepContainer->setVisible(true);
    sweepOuterLayout->addWidget(m_sweepContainer);
    containerLayout->addWidget(sweepGroup);

    // ── 3. Burst Execution & Repetition Card ───────────────────────────
    auto* burstGroup = new QGroupBox("BURST EXECUTION && REPETITION", container);
    auto* burstOuterLayout = new QVBoxLayout(burstGroup);
    burstOuterLayout->setContentsMargins(10, 14, 10, 10);
    burstOuterLayout->setSpacing(8);

    m_infiniteCheck = new QCheckBox("Enable Continuous Burst Execution (Loop until STOP)", burstGroup);
    burstOuterLayout->addWidget(m_infiniteCheck);

    auto* burstGridWidget = new QWidget(burstGroup);
    auto* burstLayout = new QGridLayout(burstGridWidget);
    burstLayout->setContentsMargins(0, 4, 0, 0);
    burstLayout->setHorizontalSpacing(8);
    burstLayout->setVerticalSpacing(10);
    burstLayout->setColumnStretch(0, 0);
    burstLayout->setColumnStretch(1, 1);
    burstLayout->setColumnStretch(2, 0);
    burstLayout->setColumnMinimumWidth(2, 56);

    // Row 0: Post-Trigger Delay
    auto* delayLbl = new QLabel("Post-Trigger Delay:", burstGridWidget);
    delayLbl->setObjectName("fieldLabel");
    burstLayout->addWidget(delayLbl, 0, 0);

    m_delaySpin = new QSpinBox(burstGridWidget);
    m_delaySpin->setRange(0, 1000000);
    m_delaySpin->setValue(0);
    burstLayout->addWidget(m_delaySpin, 0, 1);

    m_delayUnitCombo = new QComboBox(burstGridWidget);
    m_delayUnitCombo->addItems({"ns", "µs", "ms"});
    m_delayUnitCombo->setFixedWidth(68);
    burstLayout->addWidget(m_delayUnitCombo, 0, 2);

    // Row 1: Pulse Width (T)
    auto* pulseLbl = new QLabel("Pulse Width (T):", burstGridWidget);
    pulseLbl->setObjectName("fieldLabel");
    burstLayout->addWidget(pulseLbl, 1, 0);

    m_pulseSpin = new QSpinBox(burstGridWidget);
    m_pulseSpin->setRange(MIN_PULSE_NS, 1000000);
    m_pulseSpin->setValue(100);
    burstLayout->addWidget(m_pulseSpin, 1, 1);

    m_pulseUnitCombo = new QComboBox(burstGridWidget);
    m_pulseUnitCombo->addItems({"ns", "µs", "ms"});
    m_pulseUnitCombo->setFixedWidth(68);
    burstLayout->addWidget(m_pulseUnitCombo, 1, 2);

    // Row 2: Step Interval / Pause
    m_burstGapLabel = new QLabel("Step Interval / Pause:", burstGridWidget);
    m_burstGapLabel->setObjectName("fieldLabel");
    burstLayout->addWidget(m_burstGapLabel, 2, 0);

    m_burstGapSpin = new QSpinBox(burstGridWidget);
    m_burstGapSpin->setRange(0, MAX_BURST_GAP_MS);
    m_burstGapSpin->setValue(100);
    burstLayout->addWidget(m_burstGapSpin, 2, 1);

    m_burstGapUnit = new QLabel("ms", burstGridWidget);
    m_burstGapUnit->setStyleSheet("color: #8b949e; font-weight: bold; padding-left: 4px;");
    burstLayout->addWidget(m_burstGapUnit, 2, 2);

    burstOuterLayout->addWidget(burstGridWidget);
    containerLayout->addWidget(burstGroup);

    // ── 4. Target Power Cycling Card ───────────────────────────────────
    auto* pwrGroup = new QGroupBox("TARGET POWER CYCLING", container);
    auto* pwrLayout = new QGridLayout(pwrGroup);
    pwrLayout->setContentsMargins(10, 14, 10, 10);
    pwrLayout->setHorizontalSpacing(8);
    pwrLayout->setVerticalSpacing(10);
    pwrLayout->setColumnStretch(0, 0);
    pwrLayout->setColumnStretch(1, 1);
    pwrLayout->setColumnStretch(2, 0);
    pwrLayout->setColumnMinimumWidth(2, 56);

    m_pwrCycleCheck = new QCheckBox("Enable Target Power-Cycling", pwrGroup);
    pwrLayout->addWidget(m_pwrCycleCheck, 0, 0, 1, 3);

    auto* offLbl = new QLabel("Power-Off Duration:", pwrGroup);
    offLbl->setObjectName("fieldLabel");
    pwrLayout->addWidget(offLbl, 1, 0);

    m_pwrCycleOffSpin = new QSpinBox(pwrGroup);
    m_pwrCycleOffSpin->setRange(1, MAX_PWR_CYCLE_OFF_MS);
    m_pwrCycleOffSpin->setValue(50);
    pwrLayout->addWidget(m_pwrCycleOffSpin, 1, 1);

    auto* msLbl = new QLabel("ms", pwrGroup);
    msLbl->setStyleSheet("color: #6d7f99; font-weight: bold; padding-left: 4px;");
    pwrLayout->addWidget(msLbl, 1, 2);

    containerLayout->addWidget(pwrGroup);

    // ── 4. Hardware GPIO Mapping Card ──────────────────────────────────
    auto* pinGroup = new QGroupBox("HARDWARE GPIO ROUTING", container);
    auto* pinLayout = new QGridLayout(pinGroup);
    pinLayout->setContentsMargins(10, 14, 10, 10);
    pinLayout->setHorizontalSpacing(8);
    pinLayout->setVerticalSpacing(10);
    pinLayout->setColumnStretch(0, 0);
    pinLayout->setColumnStretch(1, 1);
    pinLayout->setColumnStretch(2, 0);
    pinLayout->setColumnMinimumWidth(2, 56);

    auto* gPinLbl = new QLabel("Glitch Pulse Pin:", pinGroup);
    gPinLbl->setObjectName("fieldLabel");
    pinLayout->addWidget(gPinLbl, 0, 0);
    m_glitchPinSpin = new QSpinBox(pinGroup);
    m_glitchPinSpin->setRange(0, MAX_GPIO_PIN);
    m_glitchPinSpin->setValue(DEFAULT_GLITCH_PIN);
    pinLayout->addWidget(m_glitchPinSpin, 0, 1);
    auto* gDesc = new QLabel("(MOSFET Gate)", pinGroup);
    gDesc->setStyleSheet("color: #8b949e; font-size: 11px;");
    pinLayout->addWidget(gDesc, 0, 2);

    auto* pPinLbl = new QLabel("Power Relay Pin:", pinGroup);
    pPinLbl->setObjectName("fieldLabel");
    pinLayout->addWidget(pPinLbl, 1, 0);
    m_pwrCyclePinSpin = new QSpinBox(pinGroup);
    m_pwrCyclePinSpin->setRange(0, MAX_GPIO_PIN);
    m_pwrCyclePinSpin->setValue(DEFAULT_PWR_CYCLE_PIN);
    pinLayout->addWidget(m_pwrCyclePinSpin, 1, 1);
    auto* pDesc = new QLabel("(Target VCC Cut)", pinGroup);
    pDesc->setStyleSheet("color: #8b949e; font-size: 11px;");
    pinLayout->addWidget(pDesc, 1, 2);

    auto* mPinLbl = new QLabel("Scope Trigger Pin:", pinGroup);
    mPinLbl->setObjectName("fieldLabel");
    pinLayout->addWidget(mPinLbl, 2, 0);
    m_monitorPinSpin = new QSpinBox(pinGroup);
    m_monitorPinSpin->setRange(0, MAX_GPIO_PIN);
    m_monitorPinSpin->setValue(DEFAULT_MONITOR_PIN);
    pinLayout->addWidget(m_monitorPinSpin, 2, 1);
    auto* mDesc = new QLabel("(Logic Monitor)", pinGroup);
    mDesc->setStyleSheet("color: #8b949e; font-size: 11px;");
    pinLayout->addWidget(mDesc, 2, 2);

    m_monitorEnCheck = new QCheckBox("Drive Scope Monitor Output", pinGroup);
    m_monitorEnCheck->setChecked(true);
    m_monitorEnCheck->setToolTip(
        "Drives the Scope Trigger Pin (GP2) HIGH for the exact duration of the glitch window.\n"
        "Connect oscilloscope Channel 1 or External Trigger here to capture pulses cleanly."
    );
    pinLayout->addWidget(m_monitorEnCheck, 3, 0, 1, 3);

    auto* monDesc = new QLabel(
        "Asserts pin HIGH during glitch window to trigger oscilloscope / logic analyzer.", pinGroup);
    monDesc->setStyleSheet("color: #8b949e; font-size: 11px; padding-left: 24px; margin-top: -4px;");
    monDesc->setWordWrap(true);
    pinLayout->addWidget(monDesc, 4, 0, 1, 3);

    containerLayout->addWidget(pinGroup);

    // Validation Error Message
    m_errorLabel = new QLabel(container);
    m_errorLabel->setStyleSheet("color: #f85149; background-color: #211517; border: 1px solid #da3633; border-radius: 6px; padding: 6px 10px; font-weight: bold;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    containerLayout->addWidget(m_errorLabel);

    containerLayout->addStretch(1);
    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea, 1);

    // ── 5. Fixed Emergency & Execution Action Dock ─────────────────────
    auto* actionDock = new QFrame(this);
    actionDock->setStyleSheet("QFrame { background-color: #161b22; border-top: 1px solid #30363d; padding-top: 8px; }");
    auto* dockLayout = new QVBoxLayout(actionDock);
    dockLayout->setContentsMargins(0, 4, 0, 0);
    dockLayout->setSpacing(8);

    auto* fireRow = new QHBoxLayout();
    fireRow->setSpacing(8);

    m_armBtn = new QPushButton("ARM GLITCH", actionDock);
    m_armBtn->setObjectName("armButton");
    m_armBtn->setCursor(Qt::PointingHandCursor);

    m_fireBtn = new QPushButton("FIRE BURST", actionDock);
    m_fireBtn->setObjectName("fireButton");
    m_fireBtn->setEnabled(false);
    m_fireBtn->setCursor(Qt::PointingHandCursor);

    fireRow->addWidget(m_armBtn, 1);
    fireRow->addWidget(m_fireBtn, 1);
    dockLayout->addLayout(fireRow);

    auto* subRow = new QHBoxLayout();
    subRow->setSpacing(8);

    m_abortBtn = new QPushButton("🛑 STOP / ABORT", actionDock);
    m_abortBtn->setObjectName("abortButton");
    m_abortBtn->setCursor(Qt::PointingHandCursor);

    m_resetBtn = new QPushButton("Reset Defaults", actionDock);
    m_resetBtn->setObjectName("resetButton");
    m_resetBtn->setCursor(Qt::PointingHandCursor);

    subRow->addWidget(m_abortBtn, 1);
    subRow->addWidget(m_resetBtn, 1);
    dockLayout->addLayout(subRow);

    rootLayout->addWidget(actionDock);

    // Signal mappings
    connect(m_glitchTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigPanel::onUiChanged);
    connect(m_delaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_delayUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigPanel::onUiChanged);
    connect(m_pulseSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_pulseUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigPanel::onUiChanged);
    connect(m_infiniteCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && m_sweepEnableCheck->isChecked()) {
            m_sweepEnableCheck->setChecked(false);
        }
        if (!checked && (m_deviceState == STATE_FIRING || m_deviceState == STATE_PWR_CYCLING)) {
            emit abortRequested();
        }
        onUiChanged();
        updateControlStates();
    });
    connect(m_burstGapSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_pwrCycleCheck, &QCheckBox::toggled, this, &ConfigPanel::onUiChanged);
    connect(m_pwrCycleOffSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_glitchPinSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_pwrCyclePinSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_monitorPinSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_monitorEnCheck, &QCheckBox::toggled, this, &ConfigPanel::onUiChanged);

    // Sweep widget signals
    connect(m_sweepEnableCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && m_infiniteCheck->isChecked()) {
            m_infiniteCheck->setChecked(false);
        }
        onUiChanged();
        updateControlStates();
    });

    auto updateWidthSuffix = [this]() {
        QString u = " " + m_sweepWidthUnitCombo->currentText();
        m_sweepWidthStartSpin->setSuffix(u);
        m_sweepWidthEndSpin->setSuffix(u);
        m_sweepWidthStepSpin->setSuffix(u);
    };
    connect(m_sweepWidthUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updateWidthSuffix]() {
        updateWidthSuffix();
        onUiChanged();
    });

    auto updateDelaySuffix = [this]() {
        QString u = " " + m_sweepDelayUnitCombo->currentText();
        m_sweepDelayStartSpin->setSuffix(u);
        m_sweepDelayEndSpin->setSuffix(u);
        m_sweepDelayStepSpin->setSuffix(u);
    };
    connect(m_sweepDelayUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updateDelaySuffix]() {
        updateDelaySuffix();
        onUiChanged();
    });

    connect(m_sweepStepGapUnitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConfigPanel::onUiChanged);

    connect(m_sweepWidthStartSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepWidthEndSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepWidthStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepDelayStartSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepDelayEndSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepDelayStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);
    connect(m_sweepStepGapSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onUiChanged);

    connect(m_armBtn, &QPushButton::clicked, this, [this]() {
        if (!m_isSweeping && m_deviceState != STATE_FIRING) {
            if (m_connected) {
                m_armBtn->setEnabled(false); // prevent rapid duplicate clicks
            }
            emit armRequested();
        }
    });
    connect(m_fireBtn, &QPushButton::clicked, this, &ConfigPanel::onFireButtonClicked);
    connect(m_abortBtn, &QPushButton::clicked, this, &ConfigPanel::abortRequested);
    connect(m_resetBtn, &QPushButton::clicked, this, &ConfigPanel::resetRequested);
}

uint64_t ConfigPanel::getNsValue(QSpinBox* spin, QComboBox* unitCombo) {
    uint64_t val = static_cast<uint64_t>(spin->value());
    int unit = unitCombo->currentIndex();
    if (unit == 1) val *= 1000ULL;
    else if (unit == 2) val *= 1000000ULL;
    return val;
}

void ConfigPanel::setNsValue(uint32_t ns, QSpinBox* spin, QComboBox* unitCombo) {
    if (ns >= 1000000 && (ns % 1000000 == 0)) {
        unitCombo->setCurrentIndex(2);
        spin->setValue(static_cast<int>(ns / 1000000));
    } else if (ns >= 1000 && (ns % 1000 == 0)) {
        unitCombo->setCurrentIndex(1);
        spin->setValue(static_cast<int>(ns / 1000));
    } else {
        unitCombo->setCurrentIndex(0);
        spin->setValue(static_cast<int>(ns));
    }
}

uint64_t ConfigPanel::getNsValueFromDouble(QDoubleSpinBox* spin, QComboBox* unitCombo) {
    double val = spin->value();
    int unit = unitCombo ? unitCombo->currentIndex() : 0;
    if (unit == 1) val *= 1000.0;        // µs -> ns
    else if (unit == 2) val *= 1000000.0; // ms -> ns
    return static_cast<uint64_t>(std::round(val));
}

void ConfigPanel::setDoubleNsValue(uint32_t ns, QDoubleSpinBox* spin, QComboBox* unitCombo) {
    int unit = unitCombo ? unitCombo->currentIndex() : 0;
    if (unit == 1) { // µs
        spin->setValue(static_cast<double>(ns) / 1000.0);
    } else if (unit == 2) { // ms
        spin->setValue(static_cast<double>(ns) / 1000000.0);
    } else { // ns
        spin->setValue(static_cast<double>(ns));
    }
}

double ConfigPanel::getStepGapMs() const {
    double val = m_sweepStepGapSpin->value();
    int unit = m_sweepStepGapUnitCombo ? m_sweepStepGapUnitCombo->currentIndex() : 1;
    if (unit == 0) { // µs
        return val / 1000.0;
    } else if (unit == 1) { // ms
        return val;
    } else if (unit == 2) { // s
        return val * 1000.0;
    }
    return val;
}

void ConfigPanel::setStepGapMs(double ms) {
    if (!m_sweepStepGapUnitCombo || !m_sweepStepGapSpin) return;
    int currentUnit = m_sweepStepGapUnitCombo->currentIndex();
    if (currentUnit == 0) { // µs
        m_sweepStepGapSpin->setValue(ms * 1000.0);
    } else if (currentUnit == 2) { // s
        m_sweepStepGapSpin->setValue(ms / 1000.0);
    } else { // ms
        m_sweepStepGapSpin->setValue(ms);
    }
}

void ConfigPanel::updateSpinboxesFromModel() {
    if (!m_config) return;
    m_updatingFromModel = true;

    setNsValue(m_config->delayNs(), m_delaySpin, m_delayUnitCombo);
    setNsValue(m_config->pulseNs(), m_pulseSpin, m_pulseUnitCombo);
    m_infiniteCheck->setChecked(m_config->burstInfinite());
    m_burstGapSpin->setValue(m_config->burstGapMs());

    m_pwrCycleCheck->setChecked(m_config->pwrCycleEnabled());
    m_pwrCycleOffSpin->setValue(m_config->pwrCycleOffMs());
    m_pwrCycleOffSpin->setEnabled(m_config->pwrCycleEnabled());

    m_glitchPinSpin->setValue(m_config->glitchPin());
    m_pwrCyclePinSpin->setValue(m_config->pwrCyclePin());
    m_monitorPinSpin->setValue(m_config->monitorPin());
    m_monitorEnCheck->setChecked(m_config->monitorEnabled());

    m_sweepEnableCheck->setChecked(m_config->sweepEnabled());
    setDoubleNsValue(m_config->sweepWidthStartNs(), m_sweepWidthStartSpin, m_sweepWidthUnitCombo);
    setDoubleNsValue(m_config->sweepWidthEndNs(), m_sweepWidthEndSpin, m_sweepWidthUnitCombo);
    setDoubleNsValue(m_config->sweepWidthStepNs(), m_sweepWidthStepSpin, m_sweepWidthUnitCombo);
    setDoubleNsValue(m_config->sweepDelayStartNs(), m_sweepDelayStartSpin, m_sweepDelayUnitCombo);
    setDoubleNsValue(m_config->sweepDelayEndNs(), m_sweepDelayEndSpin, m_sweepDelayUnitCombo);
    setDoubleNsValue(m_config->sweepDelayStepNs(), m_sweepDelayStepSpin, m_sweepDelayUnitCombo);
    setStepGapMs(m_config->sweepStepGapMs());

    m_updatingFromModel = false;
}

void ConfigPanel::onModelChanged() {
    if (!m_updatingFromModel && !m_updatingFromUi) {
        updateSpinboxesFromModel();
    }
}

void ConfigPanel::onUiChanged() {
    if (m_updatingFromModel || m_updatingFromUi || !m_config) return;
    m_updatingFromUi = true;

    m_pwrCycleOffSpin->setEnabled(m_pwrCycleCheck->isChecked());

    uint64_t delay = getNsValue(m_delaySpin, m_delayUnitCombo);
    uint64_t pulse = getNsValue(m_pulseSpin, m_pulseUnitCombo);

    m_config->setDelayNs(static_cast<uint32_t>(delay));
    m_config->setPulseNs(static_cast<uint32_t>(pulse));
    m_config->setBurstInfinite(m_infiniteCheck->isChecked());
    m_config->setRepeatCount(1);
    m_config->setRepeatDelayNs(0);
    m_config->setBurstGapMs(m_burstGapSpin->value());
    m_config->setPwrCycleEnabled(m_pwrCycleCheck->isChecked());
    m_config->setPwrCycleOffMs(m_pwrCycleOffSpin->value());
    m_config->setGlitchPin(m_glitchPinSpin->value());
    m_config->setPwrCyclePin(m_pwrCyclePinSpin->value());
    m_config->setMonitorPin(m_monitorPinSpin->value());
    m_config->setMonitorEnabled(m_monitorEnCheck->isChecked());

    m_config->setSweepEnabled(m_sweepEnableCheck->isChecked());
    m_config->setSweepWidthStartNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepWidthStartSpin, m_sweepWidthUnitCombo)));
    m_config->setSweepWidthEndNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepWidthEndSpin, m_sweepWidthUnitCombo)));
    m_config->setSweepWidthStepNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepWidthStepSpin, m_sweepWidthUnitCombo)));
    m_config->setSweepDelayStartNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepDelayStartSpin, m_sweepDelayUnitCombo)));
    m_config->setSweepDelayEndNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepDelayEndSpin, m_sweepDelayUnitCombo)));
    m_config->setSweepDelayStepNs(static_cast<uint32_t>(getNsValueFromDouble(m_sweepDelayStepSpin, m_sweepDelayUnitCombo)));
    m_config->setSweepStepGapMs(getStepGapMs());

    m_updatingFromUi = false;

    onValidateInputs();
}

void ConfigPanel::onValidateInputs() {
    ValidationResult res = ParamValidator::validatePulse(static_cast<uint32_t>(getNsValue(m_pulseSpin, m_pulseUnitCombo)));
    if (!res.isValid) {
        m_errorLabel->setText("⚠ " + res.errorMessage);
        m_errorLabel->setVisible(true);
        m_armBtn->setEnabled(false);
        return;
    }

    m_errorLabel->setVisible(false);
    updateControlStates();
}

void ConfigPanel::setSweepStatus(const QString& status) {
    if (m_sweepStatusLabel) {
        m_sweepStatusLabel->setText(status);
    }
}

void ConfigPanel::setDeviceState(uint8_t state) {
    m_deviceState = state;
    if (!m_isSweeping) {
        updateControlStates();
    }
}

void ConfigPanel::setConnected(bool connected) {
    m_connected = connected;
    updateControlStates();
}

void ConfigPanel::setSweeping(bool sweeping) {
    m_isSweeping = sweeping;
    updateControlStates();
}

void ConfigPanel::onFireButtonClicked() {
    if (m_isSweeping || m_deviceState == STATE_FIRING || m_deviceState == STATE_PWR_CYCLING) {
        m_fireBtn->setEnabled(false); // prevent repeated spam
        emit abortRequested();
    } else if (m_deviceState == STATE_ARMED) {
        m_fireBtn->setEnabled(false); // disable immediately to prevent multi-clicking!
        emit fireRequested();
    } else if (m_deviceState == STATE_IDLE && m_connected && m_sweepEnableCheck->isChecked()) {
        m_fireBtn->setEnabled(false);
        emit fireRequested();
    }
}

void ConfigPanel::updateControlStates() {
    // ── Rock-solid, flicker-free UI state during automated 2D parameter sweep ──
    if (m_isSweeping) {
        // Lock all inputs so user cannot modify settings while sweep is in progress
        m_glitchTypeCombo->setEnabled(false);
        m_delaySpin->setEnabled(false);
        m_delayUnitCombo->setEnabled(false);
        m_pulseSpin->setEnabled(false);
        m_pulseUnitCombo->setEnabled(false);
        m_infiniteCheck->setEnabled(false);
        m_burstGapSpin->setEnabled(false);
        m_pwrCycleCheck->setEnabled(false);
        m_pwrCycleOffSpin->setEnabled(false);
        m_glitchPinSpin->setEnabled(false);
        m_pwrCyclePinSpin->setEnabled(false);
        m_monitorPinSpin->setEnabled(false);
        m_monitorEnCheck->setEnabled(false);

        m_sweepEnableCheck->setEnabled(false);
        m_sweepWidthStartSpin->setEnabled(false);
        m_sweepWidthEndSpin->setEnabled(false);
        m_sweepWidthStepSpin->setEnabled(false);
        m_sweepWidthUnitCombo->setEnabled(false);
        m_sweepDelayStartSpin->setEnabled(false);
        m_sweepDelayEndSpin->setEnabled(false);
        m_sweepDelayStepSpin->setEnabled(false);
        m_sweepDelayUnitCombo->setEnabled(false);
        m_sweepStepGapSpin->setEnabled(false);
        m_sweepStepGapUnitCombo->setEnabled(false);

        m_armBtn->setText("SWEEP RUNNING");
        m_armBtn->setEnabled(false);
        m_armBtn->setStyleSheet("");

        m_fireBtn->setText("🛑 STOP SWEEP");
        m_fireBtn->setEnabled(true);
        m_fireBtn->setStyleSheet(
            "QPushButton#fireButton {"
            "  background-color: #b62324;"
            "  border: 1px solid #da3633;"
            "  color: #ffffff;"
            "  font-size: 13px;"
            "  font-weight: 700;"
            "  min-height: 34px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton#fireButton:hover {"
            "  background-color: #cf222e;"
            "  color: #ffffff;"
            "}"
        );

        m_abortBtn->setEnabled(true);
        m_resetBtn->setEnabled(false);
        return;
    }

    bool isIdle = (m_deviceState == STATE_IDLE);
    bool isArmed = (m_deviceState == STATE_ARMED);
    bool isFiring = (m_deviceState == STATE_FIRING || m_deviceState == STATE_PWR_CYCLING);

    bool isSweep = m_sweepEnableCheck->isChecked();
    bool isInfinite = m_infiniteCheck->isChecked();

    // Inputs remain interactable and configurable whenever device is IDLE
    bool inputsEditable = isIdle;

    m_glitchTypeCombo->setEnabled(inputsEditable);
    m_delaySpin->setEnabled(inputsEditable && !isSweep);
    m_delayUnitCombo->setEnabled(inputsEditable && !isSweep);
    m_pulseSpin->setEnabled(inputsEditable && !isSweep);
    m_pulseUnitCombo->setEnabled(inputsEditable && !isSweep);

    // Mutual exclusivity of generation modes: User can choose either Sweep or Burst mode
    m_sweepEnableCheck->setEnabled(inputsEditable && !isInfinite);
    m_infiniteCheck->setEnabled((inputsEditable || isFiring) && !isSweep);

    // All burst execution settings remain fully settable during continuous infinite burst!
    m_burstGapSpin->setEnabled((inputsEditable || isFiring) && !isSweep);

    m_pwrCycleCheck->setEnabled(inputsEditable);
    m_pwrCycleOffSpin->setEnabled(inputsEditable && m_pwrCycleCheck->isChecked());
    m_glitchPinSpin->setEnabled(inputsEditable);
    m_pwrCyclePinSpin->setEnabled(inputsEditable);
    m_monitorPinSpin->setEnabled(inputsEditable);
    m_monitorEnCheck->setEnabled(inputsEditable);

    // Sweep options are always visible and editable whenever idle
    bool sweepInputsEditable = inputsEditable && !isInfinite;
    m_sweepWidthStartSpin->setEnabled(sweepInputsEditable);
    m_sweepWidthEndSpin->setEnabled(sweepInputsEditable);
    m_sweepWidthStepSpin->setEnabled(sweepInputsEditable);
    m_sweepWidthUnitCombo->setEnabled(sweepInputsEditable);
    m_sweepDelayStartSpin->setEnabled(sweepInputsEditable);
    m_sweepDelayEndSpin->setEnabled(sweepInputsEditable);
    m_sweepDelayStepSpin->setEnabled(sweepInputsEditable);
    m_sweepDelayUnitCombo->setEnabled(sweepInputsEditable);
    m_sweepStepGapSpin->setEnabled(sweepInputsEditable);
    m_sweepStepGapUnitCombo->setEnabled(sweepInputsEditable);

    // Validate current input values
    bool isValid = true;
    if (m_pulseSpin && m_pulseUnitCombo) {
        ValidationResult res = ParamValidator::validatePulse(static_cast<uint32_t>(getNsValue(m_pulseSpin, m_pulseUnitCombo)));
        isValid = res.isValid;
    }

    // ARM button toggles between ARM and DISARM when already armed:
    if (isArmed) {
        m_armBtn->setText("✔ ARMED (Disarm)");
        m_armBtn->setEnabled(true);
        if (m_lastVisualState != STATE_ARMED) {
            m_armBtn->setStyleSheet("QPushButton#armButton { background-color: #9e6a03; border: 1px solid #d29922; color: #ffffff; font-weight: 700; } QPushButton#armButton:hover { background-color: #bb8009; }");
        }
    } else {
        m_armBtn->setText("ARM GLITCH");
        m_armBtn->setEnabled(isIdle && m_connected && isValid);
        if (m_lastVisualState == STATE_ARMED || m_lastVisualState == 0xFF) {
            m_armBtn->setStyleSheet("");
        }
    }

    // MAIN ACTION BUTTON transforms into a high-visibility, clickable STOP button while firing!
    if (isFiring) {
        m_fireBtn->setText(isSweep ? "🛑 STOP SWEEP" : "🛑 STOP BURST");
        m_fireBtn->setEnabled(true);
        if (m_lastVisualState != STATE_FIRING) {
            m_fireBtn->setStyleSheet(
                "QPushButton#fireButton {"
                "  background-color: #b62324;"
                "  border: 1px solid #da3633;"
                "  color: #ffffff;"
                "  font-size: 13px;"
                "  font-weight: 700;"
                "  min-height: 34px;"
                "  border-radius: 6px;"
                "}"
                "QPushButton#fireButton:hover {"
                "  background-color: #cf222e;"
                "  color: #ffffff;"
                "}"
            );
        }
    } else if (isArmed) {
        m_fireBtn->setText(isSweep ? "START BRUTE SWEEP" : "FIRE BURST");
        m_fireBtn->setEnabled(m_connected);
        if (m_lastVisualState == STATE_FIRING || m_lastVisualState == 0xFF) {
            m_fireBtn->setStyleSheet("");
        }
    } else if (isIdle && isSweep) {
        // When sweep attack mode is selected and connected, allow starting sweep directly!
        m_fireBtn->setText("START BRUTE SWEEP");
        m_fireBtn->setEnabled(m_connected);
        if (m_lastVisualState == STATE_FIRING || m_lastVisualState == 0xFF) {
            m_fireBtn->setStyleSheet("");
        }
    } else {
        m_fireBtn->setText(isSweep ? "START BRUTE SWEEP" : "FIRE BURST");
        m_fireBtn->setEnabled(false);
        if (m_lastVisualState == STATE_FIRING || m_lastVisualState == 0xFF) {
            m_fireBtn->setStyleSheet("");
        }
    }

    m_lastVisualState = m_deviceState;

    // Secondary emergency STOP / ABORT button is also always ready:
    m_abortBtn->setEnabled(m_connected && (isArmed || isFiring));
    m_resetBtn->setEnabled(isIdle);
}

} // namespace faulter
