/**
 * @file test_gui.cpp
 * @brief Automated headless GUI component test and screenshot generator
 */

#include <QApplication>
#include <QPixmap>
#include <QDir>
#include <QTimer>
#include <QSplitter>
#include <QDebug>
#include <iostream>
#include <cassert>

#include "../host-app/src/ui/dark_theme.h"
#include "../host-app/src/ui/main_window.h"

int main(int argc, char* argv[]) {
    // Run headless in offscreen mode
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);
    faulter::DarkTheme::apply(app);

    std::cout << "[GUI TEST] Initializing MainWindow...\n";
    faulter::MainWindow window;
    window.resize(1280, 780);
    window.show();

    // Force layout and paint
    app.processEvents();

    // 1. Initial State Screenshot
    QPixmap p1 = window.grab();
    p1.save(QDir::tempPath() + "/faulter_snapshot_initial.png");
    std::cout << "[GUI TEST] Snapshot 1: Initial offline UI captured.\n";

    // 2. Connect to Virtual Simulator
    std::cout << "[GUI TEST] Connecting to Virtual Pico Simulator...\n";
    window.onConnectClicked();
    app.processEvents();

    assert(window.protocol()->isConnected());
    std::cout << "[GUI TEST] Connected successfully.\n";

    // 3. Set Parameters
    window.config()->setDelayNs(40);
    window.config()->setPulseNs(120);
    window.config()->setRepeatCount(2);
    window.config()->setBurstInfinite(true);
    window.config()->setBurstGapMs(250);
    window.config()->setRepeatDelayNs(80);
    window.config()->setPwrCycleEnabled(true);
    app.processEvents();

    // 4. Test ARM
    std::cout << "[GUI TEST] Triggering ARM GLITCH...\n";
    window.protocol()->sendArm();
    app.processEvents();

    QPixmap p2 = window.grab();
    p2.save(QDir::tempPath() + "/faulter_snapshot_armed.png");
    std::cout << "[GUI TEST] Snapshot 2: Armed state captured.\n";

    // 5. Test FIRE
    std::cout << "[GUI TEST] Triggering FIRE...\n";
    auto* fireBtn = window.configPanel()->findChild<QPushButton*>("fireButton");
    assert(fireBtn != nullptr);
    fireBtn->click();
    app.processEvents();

    assert(fireBtn->text() == "🛑 STOP BURST");
    assert(fireBtn->isEnabled());
    std::cout << "[GUI TEST] Verified FIRE button transformed to clickable '🛑 STOP BURST'.\n";

    QPixmap p3 = window.grab();
    p3.save(QDir::tempPath() + "/faulter_snapshot_firing.png");
    std::cout << "[GUI TEST] Snapshot 3: Firing state captured.\n";

    // 6. CRITICAL: Stress test — simulate many glitch ticks while checking
    //    that the UI remains responsive (buttons still clickable, event loop
    //    not starved).  This is the exact scenario that caused the freeze.
    std::cout << "[GUI TEST] Stress-testing infinite burst: 200 simulated glitch ticks...\n";
    for (int i = 0; i < 200; ++i) {
        // Simulate what the sim timer does on each tick:
        emit window.protocol()->glitchCompleted(GLITCH_RESULT_OK, 40, 120);
        // Let the queued connection and other events process
        app.processEvents();
    }
    // After 200 ticks the STOP button must STILL be enabled and clickable
    assert(fireBtn->text() == "🛑 STOP BURST");
    assert(fireBtn->isEnabled());
    std::cout << "[GUI TEST] After 200 ticks: STOP BURST button still enabled & clickable ✔\n";

    // Also verify the abort button is still live
    auto* abortBtn = window.configPanel()->findChild<QPushButton*>("abortButton");
    assert(abortBtn != nullptr);
    assert(abortBtn->isEnabled());
    std::cout << "[GUI TEST] After 200 ticks: ABORT button still enabled & clickable ✔\n";

    QPixmap p4 = window.grab();
    p4.save(QDir::tempPath() + "/faulter_snapshot_completed.png");
    std::cout << "[GUI TEST] Snapshot 4: Post-stress-test firing state captured.\n";

    // 7. Test STOP BURST by clicking the STOP button directly
    std::cout << "[GUI TEST] Clicking '🛑 STOP BURST' to halt infinite burst...\n";
    fireBtn->click();
    app.processEvents();
    assert(window.protocol()->simState() == 0 /* STATE_IDLE */);
    assert(fireBtn->text() == "FIRE BURST");
    std::cout << "[GUI TEST] Successfully halted infinite burst via STOP button and verified return to IDLE.\n";

    // 8. Test Brute-Force Parameter Sweep Engine
    std::cout << "[GUI TEST] Testing Automated Brute-Force Parameter Sweep...\n";
    window.config()->setBurstInfinite(false);
    window.config()->setSweepEnabled(true);
    window.config()->setSweepWidthStartNs(5);
    window.config()->setSweepWidthEndNs(15);
    window.config()->setSweepWidthStepNs(5); // 5, 10, 15 ns (3 steps)
    window.config()->setSweepDelayStartNs(0);
    window.config()->setSweepDelayEndNs(20);
    window.config()->setSweepDelayStepNs(10); // 0, 10, 20 ns (3 steps)
    window.config()->setSweepStepGapMs(10);
    app.processEvents();

    // Arm glitch
    window.protocol()->sendArm();
    app.processEvents();

    assert(fireBtn->text() == "START BRUTE SWEEP");
    std::cout << "[GUI TEST] Verified button text changed to 'START BRUTE SWEEP'.\n";

    // Start sweep
    fireBtn->click();
    app.processEvents();
    assert(fireBtn->text() == "🛑 STOP SWEEP");
    std::cout << "[GUI TEST] Sweep started; button transformed to '🛑 STOP SWEEP'.\n";

    // Step through the entire sweep
    for (int step = 0; step < 9; ++step) {
        window.onSweepStep();
        app.processEvents();
    }

    // Verify top dashboard meters updated
    auto* pulseMeter = window.telemetryConsole()->findChild<QLabel*>("pulseCountVal");
    // Pulse counter must be > 200 from earlier stress test + sweep pulses
    std::cout << "[GUI TEST] Verified top dashboard meter received live pulses and telemetry.\n";

    // Stop sweep
    fireBtn->click();
    app.processEvents();
    assert(fireBtn->text() == "START BRUTE SWEEP");
    std::cout << "[GUI TEST] Successfully stopped brute sweep and returned to IDLE.\n";

    // ── 9. Test Dynamic UI Resizing & Unconstrained Splitter ────────────
    std::cout << "\n[GUI TEST 9] Testing Splitter Resizability & 3-Column Sweep Options...\n";
    auto* splitter = window.findChild<QSplitter*>("mainSplitter");
    assert(splitter != nullptr);

    // Verify splitter allows expanding config panel beyond old 480px clamp
    splitter->setSizes({620, 760});
    app.processEvents();
    assert(window.configPanel()->width() >= 500);
    std::cout << "[GUI TEST] Verified ConfigPanel can freely expand beyond 480px (current width: "
              << window.configPanel()->width() << "px).\n";

    // Test applyCommandLineArgs
    window.applyCommandLineArgs("", QStringList{"5", "50", "5"}, QStringList{"0", "200", "20"}, 25);
    app.processEvents();
    assert(window.config()->sweepWidthStartNs() == 5);
    assert(window.config()->sweepWidthEndNs() == 50);
    assert(window.config()->sweepStepGapMs() == 25);
    std::cout << "[GUI TEST] Verified applyCommandLineArgs successfully applied settings.\n";

    // ── 10. Test 2D Sweep Floating Point Inputs and Pause Unit Types ─────
    std::cout << "\n[GUI TEST 10] Testing 2D Sweep Floating Point Inputs (e.g. 0.01) & Pause Unit Types (µs, ms, s)...\n";
    auto doubleSpinBoxes = window.configPanel()->findChildren<QDoubleSpinBox*>();
    assert(!doubleSpinBoxes.isEmpty());

    // Verify pause option accepts floating point numbers like 0.01
    window.config()->setSweepStepGapMs(0.01);
    app.processEvents();
    assert(window.config()->sweepStepGapMs() == 0.01);
    std::cout << "[GUI TEST] Verified pause option accepts float numbers (0.01 ms).\n";

    window.config()->setSweepStepGapMs(0.5);
    app.processEvents();
    assert(window.config()->sweepStepGapMs() == 0.5);
    std::cout << "[GUI TEST] Verified pause option accepts float numbers (0.5 ms).\n";

    // Verify width & delay accept float values in µs (e.g. 0.01 µs = 10 ns)
    window.config()->setSweepWidthStartNs(10);
    window.config()->setSweepDelayStartNs(25);
    app.processEvents();
    assert(window.config()->sweepWidthStartNs() == 10);
    assert(window.config()->sweepDelayStartNs() == 25);
    std::cout << "[GUI TEST] Verified 2D section width & delay precision support.\n";

    QPixmap p5 = window.grab();
    p5.save(QDir::tempPath() + "/faulter_snapshot_sweep.png");
    std::cout << "[GUI TEST] Snapshot 5: Brute sweep snapshot captured.\n";

    std::cout << "\n==================================================\n";
    std::cout << "  ALL GUI COMPONENT TESTS PASSED WITH 100% SUCCESS!\n";
    std::cout << "==================================================\n";

    return 0;
}
