/**
 * @file main.cpp
 * @brief Faulter desktop host application entry point
 */

#include <QApplication>
#include <QRegularExpression>
#include "ui/dark_theme.h"
#include "ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Faulter");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Faulter");
    app.setWindowIcon(QIcon(":/icons/faulter.png"));

    faulter::DarkTheme::apply(app);

    faulter::MainWindow window;

    // Support PicoGlitcher controller.py style CLI arguments:
    // e.g. ./faulter /dev/ttyACM0 -p 5 100 5 -d 0 500 10
    QString port;
    QStringList pulseArgs;
    QStringList delayArgs;
    double gapMs = 50.0;

    const QStringList rawArgs = app.arguments();
    for (int i = 1; i < rawArgs.size(); ++i) {
        QString arg = rawArgs[i];
        if (arg == "-p" || arg == "--pulse") {
            while (i + 1 < rawArgs.size() && !rawArgs[i + 1].startsWith("-") && pulseArgs.size() < 3) {
                QString nextVal = rawArgs[++i];
                QStringList parts = nextVal.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                pulseArgs.append(parts);
            }
        } else if (arg == "-d" || arg == "--delay") {
            while (i + 1 < rawArgs.size() && !rawArgs[i + 1].startsWith("-") && delayArgs.size() < 3) {
                QString nextVal = rawArgs[++i];
                QStringList parts = nextVal.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                delayArgs.append(parts);
            }
        } else if (arg == "--gap" && i + 1 < rawArgs.size()) {
            gapMs = rawArgs[++i].toDouble();
        } else if (!arg.startsWith("-") && port.isEmpty()) {
            port = arg;
        }
    }

    if (gapMs <= 0.0) gapMs = 50.0;

    if (!port.isEmpty() || !pulseArgs.isEmpty() || !delayArgs.isEmpty()) {
        window.applyCommandLineArgs(port, pulseArgs, delayArgs, gapMs);
    }

    window.show();

    return app.exec();
}
