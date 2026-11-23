/**
 * @file serial_backend.cpp
 * @brief Cross-platform asynchronous serial communication backend implementation
 */

#include "serial_backend.h"

#include <QDir>
#include <QSocketNotifier>
#include <QDebug>

#if defined(Q_OS_UNIX)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

namespace faulter {

SerialBackend::SerialBackend(QObject* parent)
    : QObject(parent)
{
}

SerialBackend::~SerialBackend() {
    closePort();
}

QStringList SerialBackend::enumeratePorts() {
    QStringList ports;

#if defined(Q_OS_UNIX)
    // Look in /dev for ttyACM* (Pico USB CDC) and ttyUSB*
    QDir devDir("/dev");
    QStringList filters;
    filters << "ttyACM*" << "ttyUSB*" << "tty.usbmodem*";
    QStringList entries = devDir.entryList(filters, QDir::System);
    for (const QString& entry : entries) {
        ports << devDir.absoluteFilePath(entry);
    }

    // Also check /dev/serial/by-id
    QDir serialById("/dev/serial/by-id");
    if (serialById.exists()) {
        QStringList idEntries = serialById.entryList(QDir::Files | QDir::System);
        for (const QString& id : idEntries) {
            ports << serialById.absoluteFilePath(id);
        }
    }
#endif

    ports.removeDuplicates();
    return ports;
}

bool SerialBackend::openPort(const QString& portName, int baudRate) {
    closePort();

#if defined(Q_OS_UNIX)
    m_fd = ::open(portName.toUtf8().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        emit errorOccurred(QString("Failed to open %1: %2").arg(portName, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    struct termios tty;
    if (tcgetattr(m_fd, &tty) != 0) {
        emit errorOccurred(QString("tcgetattr error: %1").arg(QString::fromLocal8Bit(strerror(errno))));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    // Set raw mode: no parity, 8 data bits, 1 stop bit, no flow control
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= (CREAD | CLOCAL);

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    speed_t speed = B921600;
    if (baudRate == 115200) speed = B115200;
#ifdef B921600
    else if (baudRate == 921600) speed = B921600;
#endif

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        emit errorOccurred(QString("tcsetattr error: %1").arg(QString::fromLocal8Bit(strerror(errno))));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    // Assert DTR and RTS so USB CDC devices (e.g. Raspberry Pi Pico) know the host is active
    int mcs = 0;
    if (ioctl(m_fd, TIOCMGET, &mcs) == 0) {
        mcs |= (TIOCM_DTR | TIOCM_RTS);
        ioctl(m_fd, TIOCMSET, &mcs);
    }

    // Flush any stale/garbage bytes left in the serial buffers
    tcflush(m_fd, TCIOFLUSH);

    m_portName = portName;
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &SerialBackend::onSocketReadActivated);

    emit connectionChanged(true);
    return true;
#else
    Q_UNUSED(portName);
    Q_UNUSED(baudRate);
    emit errorOccurred("OS not supported for direct serial port");
    return false;
#endif
}

void SerialBackend::closePort() {
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }

#if defined(Q_OS_UNIX)
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
#endif

    if (!m_portName.isEmpty()) {
        m_portName.clear();
        emit connectionChanged(false);
    }
}

bool SerialBackend::isOpen() const {
#if defined(Q_OS_UNIX)
    return m_fd >= 0;
#else
    return false;
#endif
}

bool SerialBackend::writeData(const QByteArray& data) {
    return writeData(reinterpret_cast<const uint8_t*>(data.constData()), data.size());
}

bool SerialBackend::writeData(const uint8_t* data, size_t len) {
#if defined(Q_OS_UNIX)
    if (m_fd < 0 || data == nullptr || len == 0) return false;

    size_t written = 0;
    while (written < len) {
        ssize_t res = ::write(m_fd, data + written, len - written);
        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(500);
                continue;
            }
            emit errorOccurred(QString("Serial write error: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            return false;
        }
        written += static_cast<size_t>(res);
    }
    return true;
#else
    Q_UNUSED(data);
    Q_UNUSED(len);
    return false;
#endif
}

void SerialBackend::onSocketReadActivated(int socket) {
#if defined(Q_OS_UNIX)
    Q_UNUSED(socket);
    uint8_t buffer[256];
    ssize_t bytesRead = ::read(m_fd, buffer, sizeof(buffer));
    if (bytesRead > 0) {
        emit dataReceived(QByteArray(reinterpret_cast<const char*>(buffer), static_cast<int>(bytesRead)));
    } else if (bytesRead < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        emit errorOccurred(QString("Serial read error: %1").arg(QString::fromLocal8Bit(strerror(errno))));
        closePort();
    }
#endif
}

} // namespace faulter
