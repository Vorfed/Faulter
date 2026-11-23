/**
 * @file warning_banner.h
 * @brief Safety status warning banner
 */

#pragma once

#include <QWidget>
#include <QTimer>
#include "../../../shared/protocol_defs.h"

namespace faulter {

class WarningBanner : public QWidget {
    Q_OBJECT

public:
    explicit WarningBanner(QWidget* parent = nullptr);
    ~WarningBanner() override;

    void setStatus(uint8_t state);

signals:
    void abortRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onPulseTimer();

private:
    uint8_t m_state = STATE_IDLE;
    QTimer* m_pulseTimer = nullptr;
    float   m_pulseAlpha = 1.0f;
    bool    m_pulseUp = false;
};

} // namespace faulter
