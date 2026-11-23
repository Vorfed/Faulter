/**
 * @file main_window.h
 * @brief Faulter desktop host application main window
 */

#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#include "../model/fault_config.h"
#include "../protocol/host_protocol.h"
#include "telemetry_console.h"
#include "config_panel.h"
#include "warning_banner.h"

namespace faulter {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Public accessors for test harnesses
    HostProtocol* protocol() const { return m_protocol; }
    FaultConfig* config() const { return m_config; }
    ConfigPanel* configPanel() const { return m_configPanel; }
    TelemetryConsole* telemetryConsole() const { return m_telemetryConsole; }

    void applyCommandLineArgs(const QString& port, const QStringList& pulseArgs, const QStringList& delayArgs, double gapMs);

public slots:
    void onRefreshPorts();
    void onConnectClicked();
    void onSweepStep();

private slots:
    void onConnectionStateChanged(bool connected);
    void onPongReceived(uint8_t protoMajor, uint8_t protoMinor,
                        uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch,
                        uint16_t caps);
    void onAckReceived(uint8_t cmdType);
    void onNakReceived(uint8_t cmdType, uint8_t errorCode);
    void onStatusReceived(uint8_t state, uint8_t lastError, uint32_t lastDelayNs, uint32_t lastPulseNs);
    void onGlitchCompleted(uint8_t result, uint32_t actualDelayNs, uint32_t actualPulseNs);
    void onProtocolError(const QString& errorMsg);

    void onArmRequested();
    void onFireRequested();
    void onAbortRequested();
    void onResetRequested();
    void onConfigChanged();
    void onPollTimer();

private:
    void setupUi();
    void setupConnections();
    void updateConnectionControls(bool connected);

    FaultConfig*        m_config = nullptr;
    HostProtocol*       m_protocol = nullptr;

    // Top Header Widgets
    QComboBox*          m_portCombo = nullptr;
    QPushButton*        m_refreshBtn = nullptr;
    QPushButton*        m_connectBtn = nullptr;
    QLabel*             m_statusLed = nullptr;

    // Main View Widgets
    WarningBanner*      m_warningBanner = nullptr;
    ConfigPanel*        m_configPanel = nullptr;
    TelemetryConsole*   m_telemetryConsole = nullptr;

    // Periodic telemetry poll
    QTimer*             m_pollTimer = nullptr;
    QTimer*             m_burstProgressTimer = nullptr;
    uint8_t             m_currentState = STATE_IDLE;

    // Automated 2D parameter sweep (brute-force) engine
    bool                m_isSweeping = false;
    bool                m_sweepWaitingForCompletion = false;
    bool                m_sweepCompleted = false;
    uint32_t            m_currentSweepDelay = 0;
    uint32_t            m_currentSweepWidth = 0;
    uint64_t            m_sweepStepCount = 0;
};

} // namespace faulter
