/**
 * @file telemetry_console.cpp
 * @brief Real-time hardware telemetry dashboard and event activity console implementation
 */

#include "telemetry_console.h"
#include "../../../shared/protocol_defs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTime>
#include <QScrollBar>

namespace faulter {

TelemetryConsole::TelemetryConsole(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setDeviceState(STATE_IDLE);
}

void TelemetryConsole::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(14);

    // ── 1. Top Digital Telemetry Dashboard (4 Metric Cards) ───────────
    auto* meterLayout = new QHBoxLayout();
    meterLayout->setSpacing(10);

    auto createCard = [](const QString& label, QLabel*& valLabel, const QString& initVal, const QString& color) {
        auto* card = new QFrame();
        card->setStyleSheet(QString(
            "QFrame { background-color: #161b22; border: 1px solid #30363d; border-radius: 8px; padding: 10px; }"
            "QLabel { background: transparent; }"
        ));
        auto* l = new QVBoxLayout(card);
        l->setContentsMargins(4, 4, 4, 4);
        l->setSpacing(4);

        auto* title = new QLabel(label, card);
        title->setStyleSheet("color: #8b949e; font-size: 11px; font-weight: 700; text-transform: uppercase;");
        l->addWidget(title);

        valLabel = new QLabel(initVal, card);
        valLabel->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800; font-family: Monospace;").arg(color));
        l->addWidget(valLabel);

        return card;
    };

    meterLayout->addWidget(createCard("PULSES FIRED", m_pulseCountVal, "0", "#58a6ff"), 1);
    meterLayout->addWidget(createCard("ACTUAL PULSE WIDTH", m_lastPulseVal, "-- ns", "#3fb950"), 1);
    meterLayout->addWidget(createCard("ACTUAL DELAY", m_lastDelayVal, "-- ns", "#d29922"), 1);
    meterLayout->addWidget(createCard("HARDWARE STATE", m_stateVal, "IDLE", "#8b949e"), 1);

    rootLayout->addLayout(meterLayout);

    // ── 2. Real-Time Activity Log Console ──────────────────────────────
    auto* logGroup = new QGroupBox("HARDWARE EVENT && TELEMETRY LOG", this);
    auto* logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(12, 16, 12, 12);
    logLayout->setSpacing(8);

    m_logView = new QPlainTextEdit(logGroup);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Monospace", 10));
    m_logView->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #0d1117;"
        "  color: #c9d1d9;"
        "  border: 1px solid #30363d;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  line-height: 1.4;"
        "}"
    );
    logLayout->addWidget(m_logView, 1);

    // Bottom Controls for Log
    auto* logBottom = new QHBoxLayout();
    logBottom->setSpacing(8);

    auto* note = new QLabel("Log shows real-time sequence execution, burst counts, and timing verification.", logGroup);
    note->setStyleSheet("color: #6d7f99; font-size: 11px;");
    logBottom->addWidget(note, 1);

    m_clearBtn = new QPushButton("Clear Log", logGroup);
    m_clearBtn->setMinimumWidth(110);
    connect(m_clearBtn, &QPushButton::clicked, this, &TelemetryConsole::clearLog);
    logBottom->addWidget(m_clearBtn);

    logLayout->addLayout(logBottom);
    rootLayout->addWidget(logGroup, 1);

    logInfo("Faulter Activity Console initialized. Ready.");
}

void TelemetryConsole::recordPulse(uint32_t actualDelayNs, uint32_t actualPulseNs) {
    m_totalPulses++;
    m_pulseCountVal->setText(QString::number(m_totalPulses));
    m_lastPulseVal->setText(QString("%1 ns").arg(actualPulseNs));
    m_lastDelayVal->setText(QString("%1 ns").arg(actualDelayNs));

    // During high-rate bursts, log initial pulses and every 50 pulses to prevent event-loop congestion
    if (m_totalPulses <= 5 || (m_totalPulses % 50 == 0)) {
        logSuccess(QString("Glitch Pulse #%1 executed: Delay = %2 ns, Pulse = %3 ns")
                       .arg(m_totalPulses).arg(actualDelayNs).arg(actualPulseNs));
    }
}

void TelemetryConsole::updateTimingReadout(uint32_t actualDelayNs, uint32_t actualPulseNs) {
    if (actualPulseNs > 0) {
        m_lastPulseVal->setText(QString("%1 ns").arg(actualPulseNs));
    }
    m_lastDelayVal->setText(QString("%1 ns").arg(actualDelayNs));
}

void TelemetryConsole::setDeviceState(uint8_t state) {
    // Guard: skip if the state hasn't changed — avoids expensive setStyleSheet()
    // CSS reparse on every timer tick during infinite burst.
    if (state == m_lastState) return;
    m_lastState = state;

    switch (state) {
        case STATE_IDLE:
            m_stateVal->setText("IDLE");
            m_stateVal->setStyleSheet("color: #8b949e; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
        case STATE_ARMED:
            m_stateVal->setText("ARMED");
            m_stateVal->setStyleSheet("color: #d29922; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
        case STATE_PWR_CYCLING:
            m_stateVal->setText("PWR CYCLE");
            m_stateVal->setStyleSheet("color: #f85149; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
        case STATE_FIRING:
            m_stateVal->setText("FIRING");
            m_stateVal->setStyleSheet("color: #f85149; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
        case STATE_ERROR:
            m_stateVal->setText("ERROR");
            m_stateVal->setStyleSheet("color: #f85149; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
        default:
            m_stateVal->setText("UNKNOWN");
            m_stateVal->setStyleSheet("color: #6e7681; font-size: 20px; font-weight: 800; font-family: Monospace;");
            break;
    }
}

void TelemetryConsole::resetStats() {
    m_totalPulses = 0;
    m_pulseCountVal->setText("0");
    m_lastPulseVal->setText("-- ns");
    m_lastDelayVal->setText("-- ns");
    setDeviceState(STATE_IDLE);
}

void TelemetryConsole::appendLog(const QString& category, const QString& msg, const QString& colorHex) {
    QString timeStr = QTime::currentTime().toString("HH:mm:ss.zzz");
    QString line = QString("<span style='color: #6e7681;'>[%1]</span> <span style='color: %2; font-weight: bold;'>[%3]</span> %4")
                       .arg(timeStr, colorHex, category, msg);

    m_logView->appendHtml(line);
    m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
}

void TelemetryConsole::logInfo(const QString& msg) {
    appendLog("INFO", msg, "#58a6ff");
}

void TelemetryConsole::logSuccess(const QString& msg) {
    appendLog("SUCCESS", msg, "#3fb950");
}

void TelemetryConsole::logWarning(const QString& msg) {
    appendLog("WARNING", msg, "#d29922");
}

void TelemetryConsole::logError(const QString& msg) {
    appendLog("ERROR", msg, "#f85149");
}

void TelemetryConsole::clearLog() {
    m_logView->clear();
    logInfo("Log cleared.");
}

} // namespace faulter
