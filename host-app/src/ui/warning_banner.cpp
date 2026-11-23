/**
 * @file warning_banner.cpp
 * @brief Safety status warning banner implementation
 */

#include "warning_banner.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFont>

namespace faulter {

WarningBanner::WarningBanner(QWidget* parent)
    : QWidget(parent)
    , m_pulseTimer(new QTimer(this))
{
    setFixedHeight(42);
    setVisible(false);

    m_pulseTimer->setInterval(80);  // ~12 FPS animation is smooth enough
    connect(m_pulseTimer, &QTimer::timeout, this, &WarningBanner::onPulseTimer);
}

WarningBanner::~WarningBanner() {
    m_pulseTimer->stop();
}

void WarningBanner::setStatus(uint8_t state) {
    // Skip redundant updates — during infinite burst this gets called on every
    // timer tick and each call triggers update()/repaint which starves the
    // event loop.
    if (m_state == state) return;

    m_state = state;
    if (state == STATE_ARMED || state == STATE_PWR_CYCLING || state == STATE_FIRING) {
        setVisible(true);
        if (state == STATE_FIRING || state == STATE_PWR_CYCLING) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        if (!m_pulseTimer->isActive()) {
            m_pulseTimer->start();
        }
    } else {
        m_pulseTimer->stop();
        setVisible(false);
        setCursor(Qt::ArrowCursor);
    }
    update();
}

void WarningBanner::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_state == STATE_FIRING || m_state == STATE_PWR_CYCLING) {
            emit abortRequested();
        }
    }
    QWidget::mousePressEvent(event);
}

void WarningBanner::onPulseTimer() {
    if (m_state == STATE_FIRING || m_state == STATE_PWR_CYCLING) {
        if (m_pulseUp) {
            m_pulseAlpha += 0.10f;
            if (m_pulseAlpha >= 1.0f) {
                m_pulseAlpha = 1.0f;
                m_pulseUp = false;
            }
        } else {
            m_pulseAlpha -= 0.10f;
            if (m_pulseAlpha <= 0.45f) {
                m_pulseAlpha = 0.45f;
                m_pulseUp = true;
            }
        }
        update();
    }
}

void WarningBanner::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bgColor;
    QColor borderColor;
    QColor textColor;
    QString text;

    if (m_state == STATE_ARMED) {
        bgColor = QColor(0x27, 0x1d, 0x0c);
        borderColor = QColor(0xd2, 0x99, 0x22);
        textColor = QColor(0xe3, 0xb3, 0x41);
        text = "⚠  SYSTEM ARMED — Voltage glitching armed. Hardware ready to trigger upon FIRE command.";
    } else if (m_state == STATE_PWR_CYCLING) {
        int alpha = static_cast<int>(255 * m_pulseAlpha);
        bgColor = QColor(0x35, 0x13, 0x17, alpha);
        borderColor = QColor(0xda, 0x36, 0x33);
        textColor = QColor(0xff, 0xff, 0xff);
        text = "FAULT INJECTION IN PROGRESS: Power-cycling target board...";
    } else if (m_state == STATE_FIRING) {
        int alpha = static_cast<int>(255 * m_pulseAlpha);
        bgColor = QColor(0x40, 0x14, 0x19, alpha);
        borderColor = QColor(0xda, 0x36, 0x33);
        textColor = QColor(0xff, 0xff, 0xff);
        text = "FAULT INJECTION ACTIVE — Firing glitch loop! [ CLICK HERE OR 'STOP BURST' TO HALT ]";
    } else {
        return;
    }

    QRect r = rect().adjusted(2, 2, -2, -2);
    p.setBrush(bgColor);
    p.setPen(QPen(borderColor, 1.5));
    p.drawRoundedRect(r, 6, 6);

    QFont font("Segoe UI", 11, QFont::Bold);
    p.setFont(font);
    p.setPen(textColor);
    p.drawText(r, Qt::AlignCenter, text);
}

} // namespace faulter
