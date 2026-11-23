/**
 * @file dark_theme.h
 * @brief Professional dark instrumentation styling and custom control style
 */

#pragma once

#include <QApplication>
#include <QProxyStyle>
#include <QString>

namespace faulter {

class InstrumentStyle : public QProxyStyle {
    Q_OBJECT
public:
    explicit InstrumentStyle(QStyle* baseStyle = nullptr);

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                       QPainter* painter, const QWidget* widget = nullptr) const override;

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option,
                         SubControl subControl, const QWidget* widget = nullptr) const override;
};

class DarkTheme {
public:
    static void apply(QApplication& app);
    static QString getStyleSheet();
};

} // namespace faulter
