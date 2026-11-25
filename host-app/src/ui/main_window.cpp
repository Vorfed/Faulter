/**
 * @file main_window.cpp
 * @brief Faulter desktop host application main window implementation
 */

#include "main_window.h"
#include "../serial/serial_backend.h"

#include <cmath>
#include <QFrame>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

namespace faulter {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_config(new FaultConfig(this))
    , m_protocol(new HostProtocol(this))
    , m_pollTimer(new QTimer(this))
    , m_burstProgressTimer(new QTimer(this))
{
    setWindowTitle("Faulter — Hardware Security Fault Injection Suite [RP2040]");
    setWindowIcon(QIcon(":/icons/faulter.png"));
    resize(1380, 860);

    setupUi();
    setupConnections();

    onRefreshPorts();

    m_pollTimer->setInterval(200); // 5 Hz telemetry query
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimer);
    connect(m_burstProgressTimer, &QTimer::timeout, this, [this]() {
        if (m_currentState == STATE_FIRING && m_config->burstInfinite()) {
            m_telemetryConsole->recordPulse(m_config->delayNs(), m_config->pulseNs());
        }
    });
}

MainWindow::~MainWindow() {
    m_burstProgressTimer->stop();
    m_pollTimer->stop();
    m_protocol->disconnectPort();
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    auto* mainVLayout = new QVBoxLayout(centralWidget);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);

    // ── 1. Top Instrumentation Header Bar ──────────────────────────────
    auto* topHeaderBar = new QFrame(this);
    topHeaderBar->setObjectName("topHeaderBar");
    auto* topLayout = new QHBoxLayout(topHeaderBar);
    topLayout->setContentsMargins(16, 8, 16, 8);
    topLayout->setSpacing(14);

    // Brand Icon, Title & Subtitle
    auto* brandLayout = new QHBoxLayout();
    brandLayout->setSpacing(12);

    auto* logoIconLbl = new QLabel(topHeaderBar);
    logoIconLbl->setAlignment(Qt::AlignCenter);
    QPixmap logoPix(":/icons/faulter.png");
    if (!logoPix.isNull()) {
        logoIconLbl->setPixmap(logoPix.scaled(46, 46, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    brandLayout->addWidget(logoIconLbl);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    auto* titleLbl = new QLabel("FAULTER", topHeaderBar);
    titleLbl->setObjectName("brandTitle");
    auto* subLbl = new QLabel("RP2040 Hardware Security Fault-Injection Suite", topHeaderBar);
    subLbl->setObjectName("brandSubtitle");
    titleLayout->addWidget(titleLbl);
    titleLayout->addWidget(subLbl);
    brandLayout->addLayout(titleLayout);
    topLayout->addLayout(brandLayout);

    topLayout->addSpacing(24);

    // Serial Port Connection Controls
    auto* connBox = new QHBoxLayout();
    connBox->setSpacing(8);

    auto* portLbl = new QLabel("Hardware Target:", topHeaderBar);
    portLbl->setStyleSheet("font-weight: bold; color: #8b9db5;");
    connBox->addWidget(portLbl);

    m_portCombo = new QComboBox(topHeaderBar);
    m_portCombo->setMinimumWidth(260);
    connBox->addWidget(m_portCombo);

    m_refreshBtn = new QPushButton("⟳ Scan", topHeaderBar);
    connBox->addWidget(m_refreshBtn);

    m_connectBtn = new QPushButton("Connect", topHeaderBar);
    m_connectBtn->setMinimumWidth(125);
    m_connectBtn->setStyleSheet("QPushButton { background-color: #21262d; border: 1px solid #388bfd; color: #58a6ff; font-weight: bold; } QPushButton:hover { background-color: #1f6feb; color: #ffffff; }");
    connBox->addWidget(m_connectBtn);

    topLayout->addLayout(connBox);

    // Live Status Badge
    m_statusLed = new QLabel("● Offline", topHeaderBar);
    m_statusLed->setStyleSheet("color: #8b949e; font-weight: bold; padding: 4px 10px; background-color: #161b22; border: 1px solid #30363d; border-radius: 4px;");
    topLayout->addWidget(m_statusLed);

    topLayout->addStretch(1);

    // Quick Hardware Capabilities Tag
    auto* hwTag = new QLabel("Core: RP2040 @ 250 MHz  |  PIO Timing: 4 ns", topHeaderBar);
    hwTag->setStyleSheet("color: #8b949e; font-weight: 600; font-size: 11px; background: #21262d; border: 1px solid #30363d; border-radius: 4px; padding: 4px 8px;");
    topLayout->addWidget(hwTag);

    mainVLayout->addWidget(topHeaderBar);

    // ── 2. Safety Warning Banner ───────────────────────────────────────
    m_warningBanner = new WarningBanner(this);
    mainVLayout->addWidget(m_warningBanner);

    // ── 3. Central Splitter View ───────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("mainSplitter");
    splitter->setHandleWidth(6);
    splitter->setChildrenCollapsible(false);

    // Left Configuration Panel — unconstrained upper bound so user can resize freely
    m_configPanel = new ConfigPanel(m_config, splitter);
    m_configPanel->setMinimumWidth(380);
    m_configPanel->setConnected(false);
    splitter->addWidget(m_configPanel);

    // Right Telemetry Dashboard and Activity Terminal
    m_telemetryConsole = new TelemetryConsole(splitter);
    m_telemetryConsole->setMinimumWidth(450);
    splitter->addWidget(m_telemetryConsole);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({540, 840});

    mainVLayout->addWidget(splitter, 1);
    setCentralWidget(centralWidget);

    // ── 4. Status Bar ──────────────────────────────────────────────────
    statusBar()->showMessage("Faulter ready. Connect to physical Pico or run in Virtual Hardware Simulator.");
}

void MainWindow::setupConnections() {
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    // Protocol signals
    connect(m_protocol, &HostProtocol::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    connect(m_protocol, &HostProtocol::pongReceived, this, &MainWindow::onPongReceived);
    connect(m_protocol, &HostProtocol::ackReceived, this, &MainWindow::onAckReceived);
    connect(m_protocol, &HostProtocol::nakReceived, this, &MainWindow::onNakReceived);
    connect(m_protocol, &HostProtocol::statusReceived, this, &MainWindow::onStatusReceived);
    // QueuedConnection is CRITICAL: the simulation timer emits glitchCompleted
    // synchronously, so with DirectConnection the slot runs inside the timer
    // callback and starves the event loop of mouse/keyboard events.
    // QueuedConnection posts the slot call as a separate event so the event
    // loop can interleave input events between glitch ticks.
    connect(m_protocol, &HostProtocol::glitchCompleted, this, &MainWindow::onGlitchCompleted, Qt::QueuedConnection);
    connect(m_protocol, &HostProtocol::protocolError, this, &MainWindow::onProtocolError);

    // Config panel signals
    connect(m_configPanel, &ConfigPanel::armRequested, this, &MainWindow::onArmRequested);
    connect(m_configPanel, &ConfigPanel::fireRequested, this, &MainWindow::onFireRequested);
    connect(m_configPanel, &ConfigPanel::abortRequested, this, &MainWindow::onAbortRequested);
    connect(m_configPanel, &ConfigPanel::resetRequested, this, &MainWindow::onResetRequested);

    // Warning banner click-to-abort
    connect(m_warningBanner, &WarningBanner::abortRequested, this, &MainWindow::onAbortRequested);

    // Model signal
    connect(m_config, &FaultConfig::configChanged, this, &MainWindow::onConfigChanged);
}

void MainWindow::onRefreshPorts() {
    QString current = m_portCombo->currentText();
    m_portCombo->clear();

    // 1. Add Virtual Simulation Mode first so user can test immediately
    m_portCombo->addItem(HostProtocol::VIRTUAL_PORT_NAME);

    // 2. Add real physical ports
    QStringList realPorts = SerialBackend::enumeratePorts();
    for (const QString& p : realPorts) {
        m_portCombo->addItem(p);
    }

    int idx = m_portCombo->findText(current);
    if (idx >= 0) {
        m_portCombo->setCurrentIndex(idx);
    }
}

void MainWindow::onConnectClicked() {
    if (m_protocol->isConnected()) {
        m_pollTimer->stop();
        m_protocol->disconnectPort();
    } else {
        QString port = m_portCombo->currentText();
        if (port.isEmpty()) {
            QMessageBox::warning(this, "Serial Port", "Please select a serial device or choose Simulation Mode.");
            return;
        }
        if (m_protocol->connectToPort(port)) {
            // onSerialConnectionChanged already triggers sendPing() upon connection
            m_pollTimer->start();
        }
    }
}

void MainWindow::onConnectionStateChanged(bool connected) {
    updateConnectionControls(connected);
}

void MainWindow::updateConnectionControls(bool connected) {
    if (connected) {
        m_connectBtn->setText("Disconnect");
        m_connectBtn->setStyleSheet("QPushButton { background-color: #211517; border: 1px solid #da3633; color: #f85149; font-weight: bold; } QPushButton:hover { background-color: #cf222e; color: #ffffff; }");

        if (m_protocol->isSimulation()) {
            m_statusLed->setText("● Virtual Simulator Connected");
            m_statusLed->setStyleSheet("color: #58a6ff; font-weight: bold; padding: 4px 10px; background-color: #161b22; border: 1px solid #388bfd; border-radius: 4px;");
            statusBar()->showMessage("Connected to Faulter Virtual Hardware Simulator. Ready to Arm & Fire.");
            m_telemetryConsole->logSuccess("Connected to Virtual Hardware Simulator.");
        } else {
            m_statusLed->setText("● Hardware Online");
            m_statusLed->setStyleSheet("color: #3fb950; font-weight: bold; padding: 4px 10px; background-color: #161b22; border: 1px solid #238636; border-radius: 4px;");
            statusBar()->showMessage("Connected to Raspberry Pi Pico (RP2040) over USB CDC.");
            m_telemetryConsole->logSuccess(QString("Connected to Raspberry Pi Pico on %1").arg(m_protocol->portName()));
        }

        m_portCombo->setEnabled(false);
        m_refreshBtn->setEnabled(false);
        m_configPanel->setConnected(true);
    } else {
        m_connectBtn->setText("Connect");
        m_connectBtn->setStyleSheet("QPushButton { background-color: #21262d; border: 1px solid #388bfd; color: #58a6ff; font-weight: bold; } QPushButton:hover { background-color: #1f6feb; color: #ffffff; }");

        m_statusLed->setText("● Offline");
        m_statusLed->setStyleSheet("color: #8b949e; font-weight: bold; padding: 4px 10px; background-color: #161b22; border: 1px solid #30363d; border-radius: 4px;");

        m_portCombo->setEnabled(true);
        m_refreshBtn->setEnabled(true);
        m_configPanel->setConnected(false);

        m_currentState = STATE_IDLE;
        m_warningBanner->setStatus(STATE_IDLE);
        m_configPanel->setDeviceState(STATE_IDLE);
        m_telemetryConsole->setDeviceState(STATE_IDLE);
        m_telemetryConsole->logInfo("Hardware disconnected.");
        statusBar()->showMessage("Disconnected.");
    }
}

void MainWindow::onPongReceived(uint8_t protoMajor, uint8_t protoMinor,
                                uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch,
                                uint16_t caps) {
    Q_UNUSED(caps);
    QString info = QString("Faulter Online  |  Protocol v%1.%2  |  Firmware v%3.%4.%5")
                       .arg(protoMajor).arg(protoMinor)
                       .arg(fwMajor).arg(fwMinor).arg(fwPatch);
    statusBar()->showMessage(info);
    m_telemetryConsole->logInfo(info);
    m_protocol->syncAllParams(*m_config);
}

void MainWindow::onAckReceived(uint8_t cmdType) {
    // During sweep: track internal state but do NOT toggle UI visuals.
    // The sweep engine owns the visual state; flipping buttons on every
    // ARM/FIRE ACK causes distracting flicker at high rates.
    if (m_isSweeping) {
        if (cmdType == PKT_CMD_ARM) {
            m_currentState = STATE_ARMED;
        } else if (cmdType == PKT_CMD_FIRE) {
            m_currentState = STATE_FIRING;
        } else if (cmdType == PKT_CMD_ABORT) {
            m_currentState = STATE_IDLE;
        }
        return; // No UI updates during sweep
    }

    if (cmdType == PKT_CMD_ARM) {
        m_currentState = STATE_ARMED;
        m_warningBanner->setStatus(STATE_ARMED);
        m_configPanel->setDeviceState(STATE_ARMED);
        m_telemetryConsole->setDeviceState(STATE_ARMED);
        m_telemetryConsole->logSuccess("Hardware ARMED and locked. Ready to fire.");
        statusBar()->showMessage("Hardware ARMED and locked. Parameters synchronized. Press FIRE to trigger.");
    } else if (cmdType == PKT_CMD_ABORT) {
        m_currentState = STATE_IDLE;
        m_warningBanner->setStatus(STATE_IDLE);
        m_configPanel->setDeviceState(STATE_IDLE);
        m_telemetryConsole->setDeviceState(STATE_IDLE);
        m_telemetryConsole->logWarning("Hardware disarmed / aborted.");
        statusBar()->showMessage("Hardware disarmed / aborted.");
        // Resume poll timer (was paused during infinite burst)
        if (m_protocol->isConnected() && !m_pollTimer->isActive()) {
            m_pollTimer->start();
        }
    } else if (cmdType == PKT_CMD_RESET) {
        m_currentState = STATE_IDLE;
        m_warningBanner->setStatus(STATE_IDLE);
        m_configPanel->setDeviceState(STATE_IDLE);
        m_telemetryConsole->setDeviceState(STATE_IDLE);
        m_telemetryConsole->logInfo("Hardware reset to defaults.");
        statusBar()->showMessage("Hardware reset to defaults.");
    } else if (cmdType == PKT_CMD_FIRE) {
        m_currentState = STATE_FIRING;
        m_warningBanner->setStatus(STATE_FIRING);
        m_configPanel->setDeviceState(STATE_FIRING);
        m_telemetryConsole->setDeviceState(STATE_FIRING);
        m_telemetryConsole->logWarning("Glitch sequence triggered!");
        statusBar()->showMessage("Glitch firing in progress...");
    }
}

void MainWindow::onNakReceived(uint8_t cmdType, uint8_t errorCode) {
    QString errStr = QString("Command 0x%1 rejected by hardware (Error: 0x%2)")
                         .arg(cmdType, 2, 16, QChar('0'))
                         .arg(errorCode, 2, 16, QChar('0'));

    // During sweep, a NAK means the previous step hasn't finished yet.
    // Log it but do NOT abort the sweep — the completion-driven engine
    // will simply wait for the RSP_GLITCH_DONE before sending the next step.
    if (m_isSweeping) {
        m_telemetryConsole->logWarning(QString("Sweep NAK (non-fatal): %1").arg(errStr));
        return;
    }

    statusBar()->showMessage(errStr, 5000);
    m_telemetryConsole->logError(errStr);
    if (cmdType == PKT_CMD_ARM || cmdType == PKT_CMD_FIRE) {
        m_configPanel->setDeviceState(m_currentState);
    }
    // Note: Do NOT open a modal QMessageBox here, as modal dialogs block the GUI
    // and steal focus, preventing immediate user interaction (requiring double-clicks).
}

void MainWindow::onStatusReceived(uint8_t state, uint8_t lastError, uint32_t lastDelayNs, uint32_t lastPulseNs) {
    Q_UNUSED(lastError);

    // If host is already IDLE (user clicked abort or attack halted), do not allow
    // delayed or in-flight FIRING status packets to resurrect the firing UI state!
    if (m_currentState == STATE_IDLE && (state == STATE_FIRING || state == STATE_PWR_CYCLING)) {
        return;
    }

    if (m_currentState != state && !m_isSweeping) {
        m_currentState = state;
        m_warningBanner->setStatus(state);
        m_configPanel->setDeviceState(state);
        m_telemetryConsole->setDeviceState(state);
    }

    // Keep actual delay & pulse meters updated with hardware telemetry
    if (lastPulseNs > 0 || lastDelayNs > 0) {
        m_telemetryConsole->updateTimingReadout(lastDelayNs, lastPulseNs);
    }
}

void MainWindow::onGlitchCompleted(uint8_t result, uint32_t actualDelayNs, uint32_t actualPulseNs) {
    // 1. If sweeping, the completion drives the next step (not a timer!)
    if (m_isSweeping) {
        m_telemetryConsole->recordPulse(actualDelayNs, actualPulseNs);

        if (!m_sweepWaitingForCompletion) {
            return;
        }
        m_sweepWaitingForCompletion = false;

        if (m_sweepCompleted) {
            m_isSweeping = false;
            m_configPanel->setSweeping(false);
            m_telemetryConsole->logSuccess(
                QString("✔ Brute-force parameter sweep COMPLETED! All coordinates tested (%1 pulses fired).")
                    .arg(m_sweepStepCount));
            onAbortRequested();
            return;
        }

        // Schedule next step after the configured gap
        double gapMs = m_config->sweepStepGapMs();
        int timeoutMs = (gapMs <= 0.0) ? 0 : (gapMs < 1.0 ? 0 : static_cast<int>(std::round(gapMs)));
        QTimer::singleShot(timeoutMs, this, &MainWindow::onSweepStep);
        return;
    }

    // 2. Always record pulse in telemetry meters
    m_telemetryConsole->recordPulse(actualDelayNs, actualPulseNs);

    // 3. If the attack was already stopped or aborted, ignore trailing state updates
    if (m_currentState == STATE_IDLE) {
        return;
    }

    bool isInfinite = m_config->burstInfinite();

    if (!isInfinite || result != GLITCH_RESULT_OK) {
        // Burst ended or was aborted → transition to IDLE
        m_currentState = STATE_IDLE;
        m_warningBanner->setStatus(STATE_IDLE);
        m_configPanel->setDeviceState(STATE_IDLE);
        m_telemetryConsole->setDeviceState(STATE_IDLE);

        if (m_burstProgressTimer->isActive()) {
            m_burstProgressTimer->stop();
        }

        // Resume the status-poll timer (was paused during infinite burst)
        if (m_protocol->isConnected() && !m_pollTimer->isActive()) {
            m_pollTimer->start();
        }

        if (result == GLITCH_RESULT_OK) {
            statusBar()->showMessage(QString("✔ Glitch pulse executed: Actual Delay = %1 ns, Actual Pulse = %2 ns")
                                         .arg(actualDelayNs).arg(actualPulseNs), 6000);
        } else {
            m_telemetryConsole->logWarning("Glitch execution ended or stopped.");
            statusBar()->showMessage("Glitch finished / stopped.", 6000);
        }
    }
}

void MainWindow::onProtocolError(const QString& errorMsg) {
    m_telemetryConsole->logError("Protocol Error: " + errorMsg);
    statusBar()->showMessage("Protocol: " + errorMsg, 4000);
}

void MainWindow::onArmRequested() {
    // Block re-arming while a sweep or firing is in progress
    if (m_isSweeping || m_currentState == STATE_FIRING) {
        return;
    }
    if (!m_protocol->isConnected()) {
        m_configPanel->setConnected(false);
        QMessageBox::information(this, "Connect Required",
            "Please select a target and click 'Connect' (or choose the Virtual Pico Simulator) before arming.");
        return;
    }
    if (m_currentState == STATE_ARMED) {
        // Toggle disarm if already armed
        onAbortRequested();
        return;
    }
    m_telemetryConsole->logInfo(QString("Arming with Delay=%1ns, Width=%2ns, Burst=%3, Infinite=%4, Gap=%5ms")
                                    .arg(m_config->delayNs())
                                    .arg(m_config->pulseNs())
                                    .arg(m_config->repeatCount())
                                    .arg(m_config->burstInfinite() ? "ON" : "OFF")
                                    .arg(m_config->burstGapMs()));
    m_protocol->syncAllParams(*m_config);
    m_protocol->sendArm();
}

void MainWindow::onFireRequested() {
    if (m_currentState == STATE_FIRING || m_isSweeping) {
        return; // Prevent duplicate fire requests
    }

    if (m_config->sweepEnabled()) {
        m_isSweeping = true;
        m_sweepWaitingForCompletion = false;
        m_sweepCompleted = false;
        m_currentSweepDelay = m_config->sweepDelayStartNs();
        m_currentSweepWidth = m_config->sweepWidthStartNs();
        m_sweepStepCount = 0;

        m_currentState = STATE_FIRING;
        m_warningBanner->setStatus(STATE_FIRING);
        m_configPanel->setSweeping(true);
        m_telemetryConsole->setDeviceState(STATE_FIRING);

        m_pollTimer->stop();

        QString msg = QString("Brute-force parameter sweep STARTED: Width [%1..%2 ns, step %3 ns] × Delay [%4..%5 ns, step %6 ns] @ %7 ms interval.")
                          .arg(m_config->sweepWidthStartNs())
                          .arg(m_config->sweepWidthEndNs())
                          .arg(m_config->sweepWidthStepNs())
                          .arg(m_config->sweepDelayStartNs())
                          .arg(m_config->sweepDelayEndNs())
                          .arg(m_config->sweepDelayStepNs())
                          .arg(QString::number(m_config->sweepStepGapMs(), 'g', 4));
        m_telemetryConsole->logWarning(msg);
        statusBar()->showMessage("Brute-force sweep running... Click STOP SWEEP to halt.", 10000);

        // Execute first pulse immediately
        onSweepStep();
        return;
    }

    m_currentState = STATE_FIRING;
    m_warningBanner->setStatus(STATE_FIRING);
    m_configPanel->setDeviceState(STATE_FIRING);
    m_telemetryConsole->setDeviceState(STATE_FIRING);

    if (m_config->burstInfinite()) {
        m_telemetryConsole->logWarning("Infinite continuous burst loop ACTIVE. Click STOP / ABORT to halt.");
        statusBar()->showMessage("Infinite continuous burst running... Click STOP / ABORT to stop.");
        // Pause the 200ms GET_STATUS poll while infinite burst is running.
        // In simulation mode it triggers a synchronous statusReceived cascade
        // on every tick, which adds unnecessary event-loop pressure.
        m_pollTimer->stop();
        m_burstProgressTimer->setInterval(std::max(50u, m_config->burstGapMs()));
        m_burstProgressTimer->start();
    } else {
        m_telemetryConsole->logWarning("FIRE triggered! Glitch output active.");
    }
    m_protocol->sendFire();
}

void MainWindow::onSweepStep() {
    if (!m_isSweeping) {
        return;
    }

    m_sweepStepCount++;

    // Safety: ensure pulse width is at least MIN_PULSE_NS (4 ns) for hardware
    uint32_t pulseWidth = std::max<uint32_t>(MIN_PULSE_NS, m_currentSweepWidth);

    // 1. Program current coordinate to device / simulator
    m_protocol->sendSetParam(PARAM_DELAY_NS, m_currentSweepDelay);
    m_protocol->sendSetParam(PARAM_PULSE_NS, pulseWidth);
    m_protocol->sendArm();
    m_sweepWaitingForCompletion = true;
    m_protocol->sendFire();

    // 2. Immediately record pulse in top dashboard readout meters
    m_telemetryConsole->recordPulse(m_currentSweepDelay, pulseWidth);

    // 3. Update status in configuration panel
    m_configPanel->setSweepStatus(
        QString("Testing: Delay = %1 ns, Width = %2 ns  [Pulse #%3]")
            .arg(m_currentSweepDelay)
            .arg(pulseWidth)
            .arg(m_sweepStepCount));

    // 4. Advance coordinate according to brute-force sweep algorithm:
    //    Width increases by widthStep until width reaches widthEnd.
    //    When width reaches widthEnd: delay/timeout increases by delayStep just once,
    //    and width restarts from widthStart to test again!
    m_currentSweepWidth += m_config->sweepWidthStepNs();
    if (m_currentSweepWidth > m_config->sweepWidthEndNs()) {
        m_currentSweepWidth = m_config->sweepWidthStartNs();
        m_currentSweepDelay += m_config->sweepDelayStepNs();

        if (m_currentSweepDelay > m_config->sweepDelayEndNs()) {
            // Reached end of 2D search grid!
            m_sweepCompleted = true;
        }
    }
}

void MainWindow::onAbortRequested() {
    m_isSweeping = false;
    m_sweepWaitingForCompletion = false;
    m_sweepCompleted = false;
    m_configPanel->setSweeping(false);

    if (m_burstProgressTimer && m_burstProgressTimer->isActive()) {
        m_burstProgressTimer->stop();
    }

    m_telemetryConsole->logWarning("ABORT executed. Glitch output halted.");

    // 1. Immediately transition UI state to IDLE so the user gets instant visual feedback:
    //    The STOP BURST / STOP SWEEP button instantly reverts, controls re-enable,
    //    and banner hides — without waiting for serial round-trip latency or ACKs.
    m_currentState = STATE_IDLE;
    m_warningBanner->setStatus(STATE_IDLE);
    m_configPanel->setDeviceState(STATE_IDLE);
    m_telemetryConsole->setDeviceState(STATE_IDLE);

    // 2. Transmit ABORT to hardware and halt simulation timer
    if (m_protocol) {
        m_protocol->sendAbort();
    }

    // 3. Resume poll timer after hardware settles (500ms delay to prevent serial bus collisions)
    QTimer::singleShot(500, this, [this]() {
        if (m_protocol && m_protocol->isConnected() && m_currentState == STATE_IDLE && !m_pollTimer->isActive()) {
            m_pollTimer->start();
        }
    });

    statusBar()->showMessage("Hardware disarmed / glitch output halted.", 5000);
}

void MainWindow::onResetRequested() {
    m_protocol->sendReset();
    m_config->resetToDefaults();
    m_telemetryConsole->resetStats();
    m_telemetryConsole->logInfo("Parameters and statistics reset to defaults.");
}

void MainWindow::onConfigChanged() {
    if (m_protocol && m_protocol->isConnected()) {
        m_protocol->sendSetParam(PARAM_BURST_GAP_MS, m_config->burstGapMs());
    }
}

void MainWindow::onPollTimer() {
    if (m_protocol->isConnected()) {
        m_protocol->sendGetStatus();
    }
}

void MainWindow::applyCommandLineArgs(const QString& port, const QStringList& pulseArgs, const QStringList& delayArgs, double gapMs) {
    if (!pulseArgs.isEmpty() || !delayArgs.isEmpty()) {
        m_config->setSweepEnabled(true);
    }
    if (pulseArgs.size() >= 3) {
        m_config->setSweepWidthStartNs(pulseArgs[0].toUInt());
        m_config->setSweepWidthEndNs(pulseArgs[1].toUInt());
        m_config->setSweepWidthStepNs(pulseArgs[2].toUInt());
    }
    if (delayArgs.size() >= 3) {
        m_config->setSweepDelayStartNs(delayArgs[0].toUInt());
        m_config->setSweepDelayEndNs(delayArgs[1].toUInt());
        m_config->setSweepDelayStepNs(delayArgs[2].toUInt());
    }
    if (gapMs > 0.0) {
        m_config->setSweepStepGapMs(gapMs);
    }

    if (!port.isEmpty()) {
        int idx = m_portCombo->findText(port);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
            onConnectClicked();
        } else {
            m_portCombo->addItem(port);
            m_portCombo->setCurrentIndex(m_portCombo->count() - 1);
            onConnectClicked();
        }
    }
}

} // namespace faulter
