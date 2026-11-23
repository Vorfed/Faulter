/**
 * @file telemetry_console.h
 * @brief Real-time hardware telemetry dashboard and event activity console
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <cstdint>

namespace faulter {

class TelemetryConsole : public QWidget {
    Q_OBJECT

public:
    explicit TelemetryConsole(QWidget* parent = nullptr);

    void recordPulse(uint32_t actualDelayNs, uint32_t actualPulseNs);
    void updateTimingReadout(uint32_t actualDelayNs, uint32_t actualPulseNs);
    void setDeviceState(uint8_t state);
    void resetStats();

    void logInfo(const QString& msg);
    void logSuccess(const QString& msg);
    void logWarning(const QString& msg);
    void logError(const QString& msg);

public slots:
    void clearLog();

private:
    void setupUi();
    void appendLog(const QString& category, const QString& msg, const QString& colorHex);

    // Digital readout meters
    QLabel* m_pulseCountVal = nullptr;
    QLabel* m_lastPulseVal = nullptr;
    QLabel* m_lastDelayVal = nullptr;
    QLabel* m_stateVal = nullptr;

    // Terminal log
    QPlainTextEdit* m_logView = nullptr;
    QPushButton*    m_clearBtn = nullptr;

    uint64_t m_totalPulses = 0;
    uint8_t  m_lastState = 0xFF; // tracks last state to avoid redundant setStyleSheet calls
};

} // namespace faulter
