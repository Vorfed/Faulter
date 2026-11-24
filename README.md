<p align="center">
  <img src="assets/logo.png" alt="Faulter Logo" width="380"/>
</p>

<h1 align="center">Faulter</h1>

<p align="center">
  <strong>A hardware fault-injection learning project built around the Raspberry Pi Pico (RP2040)</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Hardware-RP2040%20(Raspberry%20Pi%20Pico)-blue?style=flat-square" alt="Hardware"/>
  <img src="https://img.shields.io/badge/Host%20GUI-C%2B%2B17%20%2F%20Qt6%20%26%20Qt5-blueviolet?style=flat-square" alt="GUI"/>
  <img src="https://img.shields.io/badge/Protocol-COBS%20%2B%20CRC--16-orange?style=flat-square" alt="Protocol"/>
  <img src="https://img.shields.io/badge/Status-Early%2C%20Active%20Development-yellow?style=flat-square" alt="Status"/>
</p>

---

## What is Faulter?

Faulter is a personal learning project: a voltage fault-injection (glitching) tool built around a standard, low-cost **Raspberry Pi Pico (RP2040)**, an external glitch MOSFET, and a desktop control GUI. I'm building it to understand how voltage fault injection works at the hardware level, and to have a cheap, reproducible bench setup for experimenting on my own development boards.

It is **not** a finished or validated instrument. Timing accuracy, trigger reliability, and the measurement methodology below are all still being characterized — see the [Status](#project-status) section for what is solid and what isn't yet.

### How Voltage Glitching Works (the idea)

During normal operation, a microcontroller or SoC processes instructions sequentially — fetching opcodes, checking conditions, enforcing access control. The idea behind voltage glitching is that briefly shorting the target's power rail (VCC) to ground, timed precisely relative to the target's execution, can cause transient misbehavior: a dropped instruction, a corrupted register read, or a skipped branch (e.g. a `password_ok` check). Whether and how reliably this happens depends heavily on the specific target, the pulse shape, and the timing — it's not guaranteed, and part of the point of this project is to actually measure that, rather than assume it.

### Why RP2040 & PIO?

Software-timed glitching (`NOP` loops, CPU timers) is limited by interrupt latency and jitter from the CPU core. Instead, Faulter uses the RP2040's **Programmable I/O (PIO)** peripheral to generate the trigger pulse directly in hardware, independent of the CPU:

- Each PIO instruction cycle is 8 ns at 125 MHz, so delays and pulse widths are set in fixed increments of that.
- Because PIO runs outside the CPU, it isn't affected by OS scheduling or USB interrupt timing the way a software loop would be.
- Pulse width and post-trigger delay are configurable from 8 ns up to several seconds.

I haven't independently verified jitter performance across many runs yet — the oscilloscope capture below is a single representative trace, not a statistical characterization.

---

## Oscilloscope Capture

Below is a trace of one glitch pulse, captured on a Digilent Discovery 3 at 125 MHz sampling:

<p align="center">
  <img src="assets/scope.png" alt="Faulter Glitch Pulse Oscilloscope Capture" width="850"/>
</p>

- **Yellow trace (CH1)**: the glitch MOSFET gate drive line (GPIO 19). On trigger, the PIO state machine drives this line high for a configured pulse width (shown here: 100 ns).
- The rise time and ringing shown are from this specific setup (wiring, MOSFET, decoupling) and will vary with a different target or layout — I'm still learning how much of this is me vs. the hardware characteristics.
- A separate Scope Monitor pin (GPIO 9) asserts synchronously with the glitch, so an oscilloscope or logic analyzer can trigger on every pulse.

---

## Features (implemented so far)

- **Parameter sweep**: scans combinations of pulse width and post-trigger delay over a configurable range, with configurable pause between attempts.
- **Continuous burst mode**: repeats the pulse until stopped.
- **Power-cycle control**: drives an external relay/MOSFET to power-cycle the target between attempts.
- **Configurable GPIO mapping** for glitch output, power control, and scope monitor pins, without recompiling firmware.
- **Virtual Pico simulator**: lets me test the GUI, sweep logic, and logging without a physical Pico connected — useful for development, but obviously not a substitute for testing against real hardware.
- **Host↔firmware protocol**: request/response over USB CDC, framed with COBS and checked with CRC-16/CCITT, at 921,600 baud.

---

## Default Hardware Pinout

| Signal | RP2040 GPIO | Default Role | Description |
|---|---|---|---|
| **Glitch Output** | **GPIO 19** | Output (Active-HIGH) | Connects to N-channel MOSFET gate (e.g. IRLML2502) shorting target VCC to GND |
| **Power Cycle** | **GPIO 10** | Output (Active-LOW) | Connects to P-channel MOSFET or relay driving target power supply |
| **Scope Monitor** | **GPIO 9** | Output (Active-HIGH) | Connects to oscilloscope Ch 1 or external trigger for pulse capture |
| **USB CDC** | **USB Type-B / Micro** | Serial comms | 921,600 baud binary telemetry and control link |

> Pin mappings are configurable in the UI at runtime.

---

## Installation & Building

### Prerequisites

- **Host application**: C++17 compiler (`gcc` ≥ 9, `clang` ≥ 10, or MSVC 2019+), CMake ≥ 3.16, Qt6 or Qt5 (`Widgets`, `Core`, `Gui`)
- **Firmware**: ARM GCC toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-newlib`), CMake ≥ 3.13

### 1. Clone the repository

```bash
git clone https://github.com/Vorfed/Faulter.git
cd Faulter
./setup.sh
```

*(Or manually: `git submodule update --init firmware/lib/pico-sdk && git -C firmware/lib/pico-sdk submodule update --init lib/tinyusb`)*

> **Why `./setup.sh`?**
> A plain `git clone --recurse-submodules` pulls down 400+ MB because the upstream Pico SDK bundles wireless/Bluetooth libraries (`mbedtls`, `btstack`, `lwip`, `cyw43-driver`) meant for the Pico W. Faulter targets the plain wired RP2040 and only needs TinyUSB, so `setup.sh` skips the rest — about 70 MB, under 20 seconds.

### 2. Build the desktop host application

```bash
cmake -B build -S . -DBUILD_HOST=ON -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
./build/host-app/faulter
```

### 3. Build the RP2040 firmware

```bash
cd firmware
cmake -B build -S .
cmake --build build -j$(nproc)
```

Produces `faulter_fw.uf2` (flash this), plus `.elf` / `.bin` / `.hex`.

**Flashing:** hold **BOOTSEL** on the Pico while plugging it in via USB, drag `faulter_fw.uf2` onto the `RPI-RP2` drive that appears. It reboots and re-enumerates as a USB CDC serial device.

### 4. Run the test suite

Covers CRC-16 math, COBS encode/decode round-trips, and packet serialization, plus headless Qt GUI component tests:

```bash
ctest --test-dir build --output-on-failure
```

---

## Operational Workflow

```
   ┌─────────────┐       ┌───────────────┐       ┌─────────────┐       ┌─────────────┐
   │ 1. CONNECT  │ ───►  │ 2. CONFIGURE  │ ───►  │   3. ARM    │ ───►  │   4. FIRE   │
   │ USB or Sim  │       │ Width & Delay │       │ Locks State │       │ PIO Trigger │
   └─────────────┘       └───────────────┘       └─────────────┘       └─────────────┘
                                                        ▲                     │
                                                        └───── [STOP/ABORT] ──┘
```

1. **Connect** to the Pico's serial port, or use the Virtual Pico simulator for offline testing.
2. **Configure** pulse width, post-trigger delay, sweep range, or burst mode.
3. **Arm**: parameters are validated and locked before firing.
4. **Fire**: the pulse (or sweep) executes; live telemetry streams to the GUI.
5. **Stop/Abort** at any time to disarm immediately.

---

## ⚠️ Safety & Hardware Damage Warning

> **This can permanently damage hardware if used carelessly.**
>
> - Shorting a power rail repeatedly can cause overcurrent heating, latch-up, and electromigration damage in the target silicon.
> - Wrong pulse widths, missing current limiting, or excess voltage **can destroy your target MCU, SoC, or flash**.
> - Always use a current-limited lab supply with overcurrent protection.
> - Only test on sacrificial/expendable boards — never on anything you need to keep working.
> - I take no responsibility for hardware damaged by using or misusing this project, and you shouldn't assume it's foolproof either.

---

## Project Status

This is an early-stage personal project, not a finished tool. Current state, as I understand it:

**Working / implemented:**
- Firmware and host app build and run; USB communication protocol works (framing, CRC, tests passing).
- PIO generates a pulse with the configured width and delay, observable on the oscilloscope.
- GUI supports connect/configure/arm/fire, sweeps, burst mode, and the simulator.

**Not yet validated / open questions:**
- I haven't rigorously measured trigger jitter or repeatability across many pulses — the single oscilloscope capture above is illustrative, not a characterization.
- I haven't yet achieved a reliable, repeatable fault on a real target (e.g. a bypassed check); so far testing has mostly focused on getting the pulse generation itself correct.
- Decoupling, MOSFET selection, and probe/wiring parasitics likely affect the pulse shape shown above, and I don't yet have a good quantitative handle on how much.

**Next steps:**
- Characterizing timing jitter properly (statistics across many triggers, not a single capture).
- Testing against a known-vulnerable target to validate the whole chain end-to-end.
- Clock glitching support and external trigger qualification, eventually.

Issues, corrections, and advice are very welcome — this is a learning project, and I'd rather get pointed at my mistakes early than later.

---

<p align="center">
  <strong>Repository:</strong> <a href="https://github.com/Vorfed/Faulter">https://github.com/Vorfed/Faulter</a>
</p>
