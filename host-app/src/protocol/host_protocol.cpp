/**
 * @file host_protocol.cpp
 * @brief High-level host protocol manager implementation with Virtual Hardware Simulation
 */

#include "host_protocol.h"
#include "cobs.h"
#include "crc16.h"

#include <QDebug>
#include <cstring>

namespace faulter {

const QString HostProtocol::VIRTUAL_PORT_NAME = "Virtual Pico (Hardware Simulation)";

HostProtocol::HostProtocol(QObject* parent)
    : QObject(parent)
    , m_serial(new SerialBackend(this))
    , m_simTimer(new QTimer(this))
{
    connect(m_serial, &SerialBackend::dataReceived, this, &HostProtocol::onSerialDataReceived);
    connect(m_serial, &SerialBackend::connectionChanged, this, &HostProtocol::onSerialConnectionChanged);
    connect(m_serial, &SerialBackend::errorOccurred, this, &HostProtocol::onSerialError);
    connect(m_simTimer, &QTimer::timeout, this, &HostProtocol::onSimulatedGlitchDone);
}

HostProtocol::~HostProtocol() {
    disconnectPort();
}

bool HostProtocol::connectToPort(const QString& portName, int baudRate) {
    disconnectPort();

    if (portName == VIRTUAL_PORT_NAME) {
        m_isSimulation = true;
        m_simState = STATE_IDLE;
        emit connectionStateChanged(true);
        // Emulate immediate Pong response
        QTimer::singleShot(20, this, [this]() {
            emit pongReceived(FAULTER_PROTO_VERSION_MAJOR, FAULTER_PROTO_VERSION_MINOR,
                              1, 0, 0, FAULTER_V1_CAPABILITIES);
        });
        return true;
    }

    m_isSimulation = false;
    m_rxBuffer.clear();
    return m_serial->openPort(portName, baudRate);
}

void HostProtocol::disconnectPort() {
    if (m_isSimulation) {
        m_isSimulation = false;
        m_simState = STATE_IDLE;
        emit connectionStateChanged(false);
        return;
    }

    m_serial->closePort();
    m_rxBuffer.clear();
}

bool HostProtocol::isConnected() const {
    return m_isSimulation || m_serial->isOpen();
}

QString HostProtocol::portName() const {
    if (m_isSimulation) return VIRTUAL_PORT_NAME;
    return m_serial->currentPortName();
}

void HostProtocol::sendPacket(uint8_t type, const uint8_t* payload, size_t payloadLen) {
    if (!isConnected()) {
        emit protocolError("Cannot send packet: Not connected");
        return;
    }

    if (m_isSimulation) {
        // Virtual hardware packet handling
        switch (type) {
            case PKT_CMD_PING:
                emit pongReceived(FAULTER_PROTO_VERSION_MAJOR, FAULTER_PROTO_VERSION_MINOR,
                                  1, 0, 0, FAULTER_V1_CAPABILITIES);
                break;
            case PKT_CMD_SET_PARAM:
                if (payload && payloadLen >= 5) {
                    uint8_t paramId = payload[0];
                    uint32_t val = (uint32_t)payload[1] | ((uint32_t)payload[2] << 8) |
                                   ((uint32_t)payload[3] << 16) | ((uint32_t)payload[4] << 24);
                    if (paramId == PARAM_DELAY_NS) m_simDelayNs = val;
                    if (paramId == PARAM_PULSE_NS) m_simPulseNs = val;
                    if (paramId == PARAM_BURST_INFINITE) m_simInfinite = (val != 0);
                    if (paramId == PARAM_BURST_GAP_MS) {
                        m_simGapMs = val;
                        if (m_simTimer->isActive()) {
                            m_simTimer->setInterval(std::max(50u, m_simGapMs));
                        }
                    }
                }
                emit ackReceived(PKT_CMD_SET_PARAM);
                break;
            case PKT_CMD_ARM:
                m_simState = STATE_ARMED;
                emit ackReceived(PKT_CMD_ARM);
                break;
            case PKT_CMD_FIRE:
                m_simState = STATE_FIRING;
                emit ackReceived(PKT_CMD_FIRE);
                if (m_simInfinite) {
                    m_simTimer->start(std::max(50u, m_simGapMs));
                } else {
                    QTimer::singleShot(10, this, &HostProtocol::onSimulatedGlitchDone);
                }
                break;
            case PKT_CMD_ABORT:
                m_simState = STATE_IDLE;
                m_simTimer->stop();
                emit ackReceived(PKT_CMD_ABORT);
                emit glitchCompleted(GLITCH_RESULT_ABORTED, 0, 0);
                break;
            case PKT_CMD_RESET:
                m_simState = STATE_IDLE;
                m_simTimer->stop();
                emit ackReceived(PKT_CMD_RESET);
                break;
            case PKT_CMD_GET_STATUS:
                emit statusReceived(m_simState, ERR_NONE, m_simDelayNs, m_simPulseNs);
                break;
            default:
                emit ackReceived(type);
                break;
        }
        return;
    }

    std::vector<uint8_t> raw;
    raw.push_back(type);
    raw.push_back(m_seq++);

    if (payload && payloadLen > 0) {
        raw.insert(raw.end(), payload, payload + payloadLen);
    }

    uint16_t crc = computeCrc16(raw);
    raw.push_back(static_cast<uint8_t>(crc >> 8));
    raw.push_back(static_cast<uint8_t>(crc & 0xFF));

    std::vector<uint8_t> encoded = cobsEncode(raw);
    encoded.push_back(FAULTER_FRAME_DELIMITER);

    m_serial->writeData(encoded.data(), encoded.size());
}

void HostProtocol::onSimulatedGlitchDone() {
    if (!m_isSimulation) return;
    if (!m_simInfinite) {
        m_simState = STATE_IDLE;
    }

    uint32_t actualDelay = (m_simDelayNs / 8) * 8;
    uint32_t actualPulse = std::max(8u, (m_simPulseNs / 8) * 8);

    emit glitchCompleted(GLITCH_RESULT_OK, actualDelay, actualPulse);
}

void HostProtocol::sendPing() {
    uint8_t payload[2] = { FAULTER_PROTO_VERSION_MAJOR, FAULTER_PROTO_VERSION_MINOR };
    sendPacket(PKT_CMD_PING, payload, sizeof(payload));
}

void HostProtocol::sendSetParam(uint8_t paramId, uint32_t value) {
    uint8_t payload[5];
    payload[0] = paramId;
    payload[1] = static_cast<uint8_t>(value & 0xFF);
    payload[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    payload[3] = static_cast<uint8_t>((value >> 16) & 0xFF);
    payload[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
    sendPacket(PKT_CMD_SET_PARAM, payload, sizeof(payload));
}

void HostProtocol::sendGetParam(uint8_t paramId) {
    uint8_t payload[1] = { paramId };
    sendPacket(PKT_CMD_GET_PARAM, payload, sizeof(payload));
}

void HostProtocol::sendArm() {
    sendPacket(PKT_CMD_ARM, nullptr, 0);
}

void HostProtocol::sendFire() {
    sendPacket(PKT_CMD_FIRE, nullptr, 0);
}

void HostProtocol::sendAbort() {
    // Send 3 consecutive idempotent ABORT packets to guarantee 100% arrival reliability
    // over USB CDC even when high-rate telemetry or serial streaming is active.
    sendPacket(PKT_CMD_ABORT, nullptr, 0);
    if (!m_isSimulation) {
        sendPacket(PKT_CMD_ABORT, nullptr, 0);
        sendPacket(PKT_CMD_ABORT, nullptr, 0);
    }
}

void HostProtocol::sendGetStatus() {
    sendPacket(PKT_CMD_GET_STATUS, nullptr, 0);
}

void HostProtocol::sendReset() {
    sendPacket(PKT_CMD_RESET, nullptr, 0);
}

void HostProtocol::syncAllParams(const FaultConfig& config) {
    sendSetParam(PARAM_GLITCH_TYPE, config.glitchType());
    sendSetParam(PARAM_DELAY_NS, config.delayNs());
    sendSetParam(PARAM_PULSE_NS, config.pulseNs());
    sendSetParam(PARAM_REPEAT_COUNT, config.repeatCount());
    sendSetParam(PARAM_REPEAT_DELAY_NS, config.repeatDelayNs());
    sendSetParam(PARAM_PWR_CYCLE_EN, config.pwrCycleEnabled() ? 1 : 0);
    sendSetParam(PARAM_PWR_CYCLE_OFF_MS, config.pwrCycleOffMs());
    sendSetParam(PARAM_GLITCH_PIN, config.glitchPin());
    sendSetParam(PARAM_PWR_CYCLE_PIN, config.pwrCyclePin());
    sendSetParam(PARAM_MONITOR_PIN, config.monitorPin());
    sendSetParam(PARAM_MONITOR_EN, config.monitorEnabled() ? 1 : 0);
    sendSetParam(PARAM_BURST_INFINITE, config.burstInfinite() ? 1 : 0);
    sendSetParam(PARAM_BURST_GAP_MS, config.burstGapMs());
}

void HostProtocol::onSerialDataReceived(const QByteArray& chunk) {
    for (int i = 0; i < chunk.size(); ++i) {
        uint8_t byte = static_cast<uint8_t>(chunk.at(i));
        if (byte == FAULTER_FRAME_DELIMITER) {
            if (!m_rxBuffer.empty()) {
                processFrame(m_rxBuffer);
                m_rxBuffer.clear();
            }
        } else {
            if (m_rxBuffer.size() < 256) {
                m_rxBuffer.push_back(byte);
            } else {
                m_rxBuffer.clear();
            }
        }
    }
}

void HostProtocol::processFrame(const std::vector<uint8_t>& frame) {
    std::vector<uint8_t> decoded;
    if (!cobsDecode(frame, decoded)) {
        emit protocolError("COBS decode error");
        return;
    }

    if (decoded.size() < PACKET_MIN_SIZE) {
        emit protocolError("Frame too short");
        return;
    }

    size_t dataLen = decoded.size() - PACKET_CRC_SIZE;
    uint16_t expectedCrc = (static_cast<uint16_t>(decoded[dataLen]) << 8) | decoded[dataLen + 1];
    uint16_t computedCrc = computeCrc16(decoded.data(), dataLen);

    if (expectedCrc != computedCrc) {
        emit protocolError(QString("CRC mismatch: exp 0x%1 got 0x%2").arg(expectedCrc, 4, 16, QChar('0')).arg(computedCrc, 4, 16, QChar('0')));
        return;
    }

    uint8_t type = decoded[0];
    const uint8_t* payload = &decoded[PACKET_HEADER_SIZE];
    size_t payloadLen = dataLen - PACKET_HEADER_SIZE;

    switch (type) {
        case PKT_RSP_PONG: {
            if (payloadLen >= 7) {
                uint16_t caps = static_cast<uint16_t>(payload[5]) | (static_cast<uint16_t>(payload[6]) << 8);
                emit pongReceived(payload[0], payload[1], payload[2], payload[3], payload[4], caps);
            }
            break;
        }
        case PKT_RSP_ACK: {
            if (payloadLen >= 1) {
                emit ackReceived(payload[0]);
            }
            break;
        }
        case PKT_RSP_NAK: {
            if (payloadLen >= 2) {
                emit nakReceived(payload[0], payload[1]);
            }
            break;
        }
        case PKT_RSP_PARAM_VALUE: {
            if (payloadLen >= 5) {
                uint32_t val = static_cast<uint32_t>(payload[1]) |
                               (static_cast<uint32_t>(payload[2]) << 8) |
                               (static_cast<uint32_t>(payload[3]) << 16) |
                               (static_cast<uint32_t>(payload[4]) << 24);
                emit paramValueReceived(payload[0], val);
            }
            break;
        }
        case PKT_RSP_STATUS: {
            if (payloadLen >= 10) {
                uint32_t d = static_cast<uint32_t>(payload[2]) |
                             (static_cast<uint32_t>(payload[3]) << 8) |
                             (static_cast<uint32_t>(payload[4]) << 16) |
                             (static_cast<uint32_t>(payload[5]) << 24);
                uint32_t p = static_cast<uint32_t>(payload[6]) |
                             (static_cast<uint32_t>(payload[7]) << 8) |
                             (static_cast<uint32_t>(payload[8]) << 16) |
                             (static_cast<uint32_t>(payload[9]) << 24);
                emit statusReceived(payload[0], payload[1], d, p);
            }
            break;
        }
        case PKT_RSP_GLITCH_DONE: {
            if (payloadLen >= 9) {
                uint32_t d = static_cast<uint32_t>(payload[1]) |
                             (static_cast<uint32_t>(payload[2]) << 8) |
                             (static_cast<uint32_t>(payload[3]) << 16) |
                             (static_cast<uint32_t>(payload[4]) << 24);
                uint32_t p = static_cast<uint32_t>(payload[5]) |
                             (static_cast<uint32_t>(payload[6]) << 8) |
                             (static_cast<uint32_t>(payload[7]) << 16) |
                             (static_cast<uint32_t>(payload[8]) << 24);
                emit glitchCompleted(payload[0], d, p);
            }
            break;
        }
        default:
            emit protocolError(QString("Unknown packet opcode: 0x%1").arg(type, 2, 16, QChar('0')));
            break;
    }
}

void HostProtocol::onSerialConnectionChanged(bool connected) {
    emit connectionStateChanged(connected);
    if (connected) {
        sendPing();
    }
}

void HostProtocol::onSerialError(const QString& err) {
    emit protocolError(err);
}

} // namespace faulter
