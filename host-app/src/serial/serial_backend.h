/**
 * @file serial_backend.h
 * @brief Cross-platform asynchronous serial communication backend
 */

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <memory>

class QSocketNotifier;

namespace faulter {

class SerialBackend : public QObject {
    Q_OBJECT

public:
    explicit SerialBackend(QObject* parent = nullptr);
    ~SerialBackend() override;

    /** Enumerate available serial port names on the system. */
    static QStringList enumeratePorts();

    /** Connect to a named serial port at given baud rate. */
    bool openPort(const QString& portName, int baudRate = 921600);

    /** Close serial port. */
    void closePort();

    /** Whether port is currently open. */
    bool isOpen() const;

    /** Current port name. */
    QString currentPortName() const { return m_portName; }

    /** Send raw bytes. */
    bool writeData(const QByteArray& data);
    bool writeData(const uint8_t* data, size_t len);

signals:
    void dataReceived(const QByteArray& chunk);
    void connectionChanged(bool connected);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onSocketReadActivated(int socket);

private:
    QString m_portName;
    int m_fd = -1;
    QSocketNotifier* m_notifier = nullptr;
};

} // namespace faulter
