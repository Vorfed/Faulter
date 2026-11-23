/**
 * @file host_protocol.h
 * @brief High-level host protocol manager for Faulter with Virtual Hardware Simulation
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <vector>
#include "../serial/serial_backend.h"
#include "../model/fault_config.h"
#include "../../../shared/protocol_defs.h"

namespace faulter {

class HostProtocol : public QObject {
    Q_OBJECT

public:
    static const QString VIRTUAL_PORT_NAME;

    explicit HostProtocol(QObject* parent = nullptr);
    ~HostProtocol() override;

    bool connectToPort(const QString& portName, int baudRate = 921600);
    void disconnectPort();
    bool isConnected() const;
    bool isSimulation() const { return m_isSimulation; }
    uint8_t simState() const { return m_simState; }
    QString portName() const;

    // Commands
    void sendPing();
    void sendSetParam(uint8_t paramId, uint32_t value);
    void sendGetParam(uint8_t paramId);
    void sendArm();
    void sendFire();
    void sendAbort();
    void sendGetStatus();
    void sendReset();
    void syncAllParams(const FaultConfig& config);

signals:
    void connectionStateChanged(bool connected);
    void pongReceived(uint8_t protoMajor, uint8_t protoMinor,
                      uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch,
                      uint16_t caps);
    void ackReceived(uint8_t cmdType);
    void nakReceived(uint8_t cmdType, uint8_t errorCode);
    void paramValueReceived(uint8_t paramId, uint32_t value);
    void statusReceived(uint8_t state, uint8_t lastError, uint32_t lastDelayNs, uint32_t lastPulseNs);
    void glitchCompleted(uint8_t result, uint32_t actualDelayNs, uint32_t actualPulseNs);
    void protocolError(const QString& message);

private slots:
    void onSerialDataReceived(const QByteArray& chunk);
    void onSerialConnectionChanged(bool connected);
    void onSerialError(const QString& err);
    void onSimulatedGlitchDone();

private:
    void processFrame(const std::vector<uint8_t>& frame);
    void sendPacket(uint8_t type, const uint8_t* payload, size_t payloadLen);

    SerialBackend* m_serial = nullptr;
    std::vector<uint8_t> m_rxBuffer;
    uint8_t m_seq = 0;

    bool m_isSimulation = false;
    uint8_t m_simState = STATE_IDLE;
    uint32_t m_simDelayNs = 0;
    uint32_t m_simPulseNs = 100;
    bool m_simInfinite = false;
    uint32_t m_simGapMs = 100;
    QTimer* m_simTimer = nullptr;
};

} // namespace faulter
