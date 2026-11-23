/**
 * @file dark_theme.cpp
 * @brief Professional dark instrumentation styling implementation — Dark Graphite / Obsidian Palette
 */

#include "dark_theme.h"

#include <QPalette>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <QStyleFactory>

namespace faulter {

InstrumentStyle::InstrumentStyle(QStyle* baseStyle)
    : QProxyStyle(baseStyle ? baseStyle : QStyleFactory::create("Fusion"))
{
}

void InstrumentStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                    QPainter* painter, const QWidget* widget) const
{
    if (element == PE_IndicatorSpinUp || element == PE_IndicatorSpinDown) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        bool isEnabled = (option->state & State_Enabled);
        bool isHovered = (option->state & State_MouseOver);
        bool isPressed = (option->state & State_Sunken);

        QColor arrowColor = isEnabled ? (isPressed ? QColor(0x58, 0xa6, 0xff) : (isHovered ? QColor(0xf0, 0xf6, 0xfc) : QColor(0x8b, 0x94, 0x9e)))
                                      : QColor(0x48, 0x4f, 0x58);

        QRect r = option->rect;
        int cx = r.center().x();
        int cy = r.center().y();

        QPainterPath path;
        if (element == PE_IndicatorSpinUp) {
            path.moveTo(cx - 4, cy + 2);
            path.lineTo(cx + 4, cy + 2);
            path.lineTo(cx, cy - 3);
            path.closeSubpath();
        } else {
            path.moveTo(cx - 4, cy - 2);
            path.lineTo(cx + 4, cy - 2);
            path.lineTo(cx, cy + 3);
            path.closeSubpath();
        }

        painter->setBrush(arrowColor);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->restore();
        return;
    }

    if (element == PE_IndicatorArrowDown) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        bool isEnabled = (option->state & State_Enabled);
        QColor arrowColor = isEnabled ? QColor(0x8b, 0x94, 0x9e) : QColor(0x48, 0x4f, 0x58);

        QRect r = option->rect;
        int cx = r.center().x();
        int cy = r.center().y();

        QPainterPath path;
        path.moveTo(cx - 4, cy - 2);
        path.lineTo(cx + 4, cy - 2);
        path.lineTo(cx, cy + 3);
        path.closeSubpath();

        painter->setBrush(arrowColor);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->restore();
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

QRect InstrumentStyle::subControlRect(ComplexControl control, const QStyleOptionComplex* option,
                                      SubControl subControl, const QWidget* widget) const
{
    QRect r = QProxyStyle::subControlRect(control, option, subControl, widget);

    if (control == CC_SpinBox) {
        if (subControl == SC_SpinBoxUp) {
            QRect frameRect = option->rect;
            int btnW = 20;
            return QRect(frameRect.right() - btnW, frameRect.top() + 1, btnW, frameRect.height() / 2 - 1);
        } else if (subControl == SC_SpinBoxDown) {
            QRect frameRect = option->rect;
            int btnW = 20;
            int midY = frameRect.top() + frameRect.height() / 2;
            return QRect(frameRect.right() - btnW, midY, btnW, frameRect.bottom() - midY);
        }
    }

    return r;
}

void DarkTheme::apply(QApplication& app) {
    app.setStyle(new InstrumentStyle(QStyleFactory::create("Fusion")));

    // Generate crisp arrow assets for QSS ComboBox
    QPixmap arrowNormal(12, 8);
    arrowNormal.fill(Qt::transparent);
    {
        QPainter p(&arrowNormal);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.moveTo(2, 2);
        path.lineTo(10, 2);
        path.lineTo(6, 7);
        path.closeSubpath();
        p.setBrush(QColor(0x8b, 0x94, 0x9e));
        p.setPen(Qt::NoPen);
        p.drawPath(path);
    }
    arrowNormal.save("/tmp/faulter_combo_arrow.png");

    QPixmap arrowGray(12, 8);
    arrowGray.fill(Qt::transparent);
    {
        QPainter p(&arrowGray);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.moveTo(2, 2);
        path.lineTo(10, 2);
        path.lineTo(6, 7);
        path.closeSubpath();
        p.setBrush(QColor(0x48, 0x4f, 0x58));
        p.setPen(Qt::NoPen);
        p.drawPath(path);
    }
    arrowGray.save("/tmp/faulter_combo_arrow_disabled.png");

    // Generate crisp checkmark asset for QSS CheckBox
    QPixmap checkMark(14, 14);
    checkMark.fill(Qt::transparent);
    {
        QPainter p(&checkMark);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor(0xe6, 0xed, 0xf3), 2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.drawLine(3, 7, 6, 10);
        p.drawLine(6, 10, 11, 4);
    }
    checkMark.save("/tmp/faulter_check_mark.png");

    QFont defaultFont("Segoe UI", 10);
    defaultFont.setStyleHint(QFont::SansSerif);
    app.setFont(defaultFont);

    QPalette p;
    p.setColor(QPalette::Window, QColor(0x0d, 0x11, 0x17));
    p.setColor(QPalette::WindowText, QColor(0xe6, 0xed, 0xf3));
    p.setColor(QPalette::Base, QColor(0x16, 0x1b, 0x22));
    p.setColor(QPalette::AlternateBase, QColor(0x1c, 0x21, 0x28));
    p.setColor(QPalette::ToolTipBase, QColor(0x21, 0x26, 0x2d));
    p.setColor(QPalette::ToolTipText, QColor(0xe6, 0xed, 0xf3));
    p.setColor(QPalette::Text, QColor(0xe6, 0xed, 0xf3));
    p.setColor(QPalette::Button, QColor(0x21, 0x26, 0x2d));
    p.setColor(QPalette::ButtonText, QColor(0xe6, 0xed, 0xf3));
    p.setColor(QPalette::BrightText, QColor(0x58, 0xa6, 0xff));
    p.setColor(QPalette::Link, QColor(0x58, 0xa6, 0xff));
    p.setColor(QPalette::Highlight, QColor(0x1f, 0x6f, 0xeb));
    p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x6e, 0x76, 0x81));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x6e, 0x76, 0x81));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x6e, 0x76, 0x81));
    app.setPalette(p);

    app.setStyleSheet(getStyleSheet());
}

QString DarkTheme::getStyleSheet() {
    return QString(R"(
        /* ── Base Window & Typography ────────────────────────────────────── */
        QWidget {
            background-color: #0d1117;
            color: #e6edf3;
            font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
            font-size: 13px;
        }

        QMainWindow {
            background-color: #090d12;
        }

        /* ── Top Header Bar ──────────────────────────────────────────────── */
        QFrame#topHeaderBar {
            background-color: #161b22;
            border-bottom: 1px solid #30363d;
            padding: 8px 16px;
        }

        QLabel#brandTitle {
            font-size: 17px;
            font-weight: 800;
            color: #f0f6fc;
            letter-spacing: 1.5px;
        }

        QLabel#brandSubtitle {
            font-size: 11px;
            color: #8b949e;
            font-weight: 500;
        }

        /* ── Group Boxes / Cards ─────────────────────────────────────────── */
        QGroupBox {
            background-color: #161b22;
            border: 1px solid #30363d;
            border-radius: 8px;
            margin-top: 22px;
            padding: 16px 12px 12px 12px;
            font-weight: 700;
            font-size: 11px;
            color: #c9d1d9;
            letter-spacing: 0.8px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 14px;
            padding: 2px 8px;
            background-color: #21262d;
            border: 1px solid #30363d;
            border-radius: 4px;
            color: #c9d1d9;
        }

        /* ── Labels ──────────────────────────────────────────────────────── */
        QLabel {
            background: transparent;
            color: #c9d1d9;
            font-size: 12px;
        }

        QLabel#fieldLabel {
            font-weight: 600;
            color: #8b949e;
        }

        /* ── Input Fields: SpinBox, LineEdit, ComboBox ───────────────────── */
        QLineEdit, QSpinBox, QComboBox {
            background-color: #0d1117;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 4px 8px;
            color: #f0f6fc;
            selection-background-color: #1f6feb;
            selection-color: #ffffff;
            min-height: 26px;
            font-size: 13px;
        }

        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid #58a6ff;
            background-color: #161b22;
        }

        QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled {
            background-color: #161b22;
            color: #484f58;
            border: 1px solid #21262d;
        }

        QSpinBox {
            padding-right: 22px;
        }

        QSpinBox::up-button, QSpinBox::down-button {
            width: 20px;
            background-color: #21262d;
            border-left: 1px solid #30363d;
        }

        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #30363d;
        }

        QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {
            background-color: #58a6ff;
        }

        QComboBox {
            background-color: #0d1117;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 3px 6px;
            padding-right: 22px;
            color: #f0f6fc;
            selection-background-color: #1f6feb;
            selection-color: #ffffff;
            min-height: 26px;
            font-size: 13px;
        }

        QComboBox:focus {
            border: 1px solid #58a6ff;
            background-color: #161b22;
        }

        QComboBox:disabled {
            background-color: #161b22;
            color: #484f58;
            border: 1px solid #21262d;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 18px;
            border-left: 1px solid #30363d;
            background-color: #21262d;
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }

        QComboBox::down-arrow {
            image: url(/tmp/faulter_combo_arrow.png);
            width: 10px;
            height: 7px;
        }

        QComboBox::down-arrow:disabled {
            image: url(/tmp/faulter_combo_arrow_disabled.png);
            width: 10px;
            height: 7px;
        }

        QComboBox QAbstractItemView {
            background-color: #161b22;
            border: 1px solid #30363d;
            border-radius: 6px;
            selection-background-color: #1f6feb;
            selection-color: #ffffff;
            color: #f0f6fc;
            padding: 4px;
        }

        /* ── Buttons ─────────────────────────────────────────────────────── */
        QPushButton {
            background-color: #21262d;
            border: 1px solid #30363d;
            border-radius: 6px;
            padding: 7px 14px;
            font-weight: 600;
            color: #e6edf3;
            min-height: 22px;
        }

        QPushButton:hover {
            background-color: #30363d;
            border-color: #8b949e;
            color: #ffffff;
        }

        QPushButton:pressed {
            background-color: #161b22;
        }

        QPushButton:disabled {
            background-color: #161b22;
            border-color: #21262d;
            color: #484f58;
        }

        /* Action Buttons */
        QPushButton#armButton {
            background-color: #21262d;
            border: 1px solid #58a6ff;
            color: #58a6ff;
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 0.5px;
            min-height: 32px;
        }

        QPushButton#armButton:hover {
            background-color: #1f6feb;
            color: #ffffff;
        }

        QPushButton#fireButton {
            background-color: #490202;
            border: 1px solid #da3633;
            color: #ff7b72;
            font-size: 14px;
            font-weight: 800;
            letter-spacing: 0.8px;
            min-height: 32px;
        }

        QPushButton#fireButton:hover {
            background-color: #b62324;
            color: #ffffff;
        }

        QPushButton#fireButton:disabled {
            background-color: #211517;
            border-color: #3c1e1e;
            color: #6e4040;
        }

        QPushButton#abortButton {
            background-color: #341a04;
            border: 1px solid #d29922;
            color: #e3b341;
            font-size: 12px;
            font-weight: 700;
            min-height: 28px;
        }

        QPushButton#abortButton:hover {
            background-color: #9e6a03;
            color: #ffffff;
        }

        QPushButton#resetButton {
            background-color: #21262d;
            border: 1px solid #30363d;
            color: #8b949e;
            font-size: 12px;
            min-height: 28px;
        }

        QPushButton#resetButton:hover {
            background-color: #30363d;
            color: #f0f6fc;
        }

        /* ── Checkboxes ──────────────────────────────────────────────────── */
        QCheckBox {
            color: #e6edf3;
            spacing: 8px;
            font-weight: 500;
        }

        QCheckBox:disabled {
            color: #6e7681;
        }

        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #30363d;
            border-radius: 4px;
            background-color: #161b22;
        }

        QCheckBox::indicator:hover {
            border-color: #8b949e;
            background-color: #21262d;
        }

        QCheckBox::indicator:checked {
            background-color: #21262d;
            border: 1px solid #8b949e;
            image: url(/tmp/faulter_check_mark.png);
        }

        QCheckBox::indicator:checked:hover {
            background-color: #30363d;
            border-color: #c9d1d9;
        }

        QCheckBox::indicator:disabled {
            background-color: #0d1117;
            border: 1px solid #21262d;
        }

        /* ── Status Bar ──────────────────────────────────────────────────── */
        QStatusBar {
            background-color: #090d12;
            border-top: 1px solid #21262d;
            color: #8b949e;
            font-size: 12px;
            padding: 4px 8px;
        }

        /* ── Scrollbars & Splitters ──────────────────────────────────────── */
        QSplitter::handle:horizontal {
            background-color: #21262d;
            border-left: 1px solid #161b22;
            border-right: 1px solid #161b22;
            width: 6px;
        }
        QSplitter::handle:horizontal:hover {
            background-color: #58a6ff;
        }
        QSplitter::handle:horizontal:pressed {
            background-color: #1f6feb;
        }

        QScrollBar:vertical {
            background: #0d1117;
            width: 8px;
            margin: 0;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical {
            background: #21262d;
            min-height: 20px;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical:hover {
            background: #30363d;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");
}

} // namespace faulter
