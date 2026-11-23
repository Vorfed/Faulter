# Faulter — PicoGlitcher Control Suite

A professional-grade, end-user-facing hardware fault-injection tool for the Raspberry Pi Pico (RP2040), featuring nanosecond-precision PIO timing and a modern dark-themed C++/Qt desktop instrumentation GUI.

---

## Architecture Overview

```
Faulter/
├── CMakeLists.txt          # Top-level orchestrator build file
├── README.md               # Project documentation and guide
├── shared/                 # Shared wire protocol definitions
│   └── protocol_defs.h     # C/C++ cross-compatible protocol constants
├── docs/                   # Specifications & analysis
│   ├── analysis.md         # Phase 0 reference codebase analysis & design decisions
│   └── protocol.md         # Binary COBS packet protocol specification v1.0
├── firmware/               # Raspberry Pi Pico (RP2040) firmware
│   ├── CMakeLists.txt      # Pico SDK CMake build configuration
│   ├── pico_sdk_import.cmake
│   └── src/
│       ├── config.h        # Default pin and clock configurations
│       ├── main.c          # Firmware entry point & USB CDC packet dispatcher
│       ├── glitch/         # PIO deterministic glitch engine (8 ns resolution)
│       │   ├── glitch.pio  # Low-level PIO assembly countdown and pulse driver
│       │   ├── glitch_engine.h
│       │   └── glitch_engine.c
│       ├── power/          # Configurable target power-cycling control
│       │   ├── power_cycle.h
│       │   └── power_cycle.c
│       ├── protocol/       # COBS framing, CRC-16, and packet serializer/parser
│       │   ├── cobs.h / cobs.c
│       │   ├── crc16.h / crc16.c
│       │   └── protocol.h / protocol.c
│       └── state/          # State machine (IDLE, ARMED, FIRING, ERROR)
│           ├── state_machine.h
│           └── state_machine.c
├── host-app/               # Desktop GUI Application (C++17 + Qt6/Qt5)
│   ├── CMakeLists.txt      # Desktop application CMake build file
│   └── src/
│       ├── main.cpp        # Application entry point
│       ├── model/          # Domain model & parameter validation
│       │   ├── fault_config.h / fault_config.cpp
│       │   └── param_validator.h / param_validator.cpp
│       ├── protocol/       # Host protocol layer & packet handling
│       │   ├── cobs.h / cobs.cpp
│       │   ├── crc16.h / crc16.cpp
│       │   └── host_protocol.h / host_protocol.cpp
│       ├── serial/         # Asynchronous cross-platform serial backend
│       │   ├── serial_backend.h
│       │   └── serial_backend.cpp
│       └── ui/             # Dark instrumentation UI & live telemetry dashboard
│           ├── dark_theme.h / dark_theme.cpp
│           ├── telemetry_console.h / telemetry_console.cpp
│           ├── config_panel.h / config_panel.cpp
│           ├── warning_banner.h / warning_banner.cpp
│           └── main_window.h / main_window.cpp
├── tests/                  # Automated unit test suite
│   ├── CMakeLists.txt
│   └── test_protocol.cpp   # CRC-16, COBS, protocol roundtrip, parameter validator
└── PicoGlitcher/           # Legacy open-source reference (reference only)
```

---

## Technical Specifications

| Feature | Specification |
|---|---|
| **Target Microcontroller** | Raspberry Pi Pico (RP2040) |
| **Glitch Mechanism** | Voltage crowbar pulse (shorting target VCC via external MOSFET) |
| **Timing Resolution** | **8 nanoseconds** (1 cycle at default 125 MHz system clock) |
| **Timing Jitter** | **0 ns** — completely offloaded to PIO state machine, immune to CPU interrupts |
| **Post-Trigger Delay** | 0 to 4,294,967,295 nanoseconds |
| **Glitch Pulse Width** | 8 to 4,294,967,295 nanoseconds (minimum 1 PIO cycle) |
| **Burst Mode** | 1 to 65,535 pulses per fire command with configurable inter-pulse gap |
| **Infinite Continuous Burst** | Continuous loop execution until user sends STOP / ABORT command |
| **Inter-Burst Gap / Timeout** | 0 to 60,000 ms pause between consecutive bursts in infinite/loop mode |
| **Target Power Cycling** | Fully configurable power-down duration in milliseconds |
| **Serial Protocol** | COBS-framed binary packets with CRC-16/CCITT checksums at 921,600 baud |
| **Desktop UI Framework** | C++17 with Qt6 (Widgets + real-time instrumentation layout) |
| **Visual Design** | Custom dark cyber-instrumentation telemetry dashboard & terminal log |

---

## Cloning the Repository

Because Faulter bundles the Raspberry Pi Pico SDK as a submodule for standalone firmware compilation, clone recursively:

```bash
git clone --recurse-submodules https://github.com/<username>/Faulter.git
```

If you already cloned without submodules, initialize them recursively:
```bash
git submodule update --init --recursive
```

---

## Building the Desktop Host Application

### Prerequisites
- Modern C++ compiler supporting C++17 (`gcc` >= 9, `clang` >= 10, or MSVC 2019+)
- CMake >= 3.16
- Qt6 (or Qt5) development packages: `Widgets`, `Core`, `Gui`

### Quick Build
```bash
# Build from project root
cmake -B build -S . -DBUILD_HOST=ON -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

# Launch the desktop application
./build/host-app/faulter
```

Or build the host application directly:
```bash
cd host-app
cmake -B build -S .
cmake --build build -j$(nproc)
./build/faulter
```

---

## Running Automated Unit Tests

Faulter includes a standalone unit test suite that validates CRC-16 test vectors, COBS framing roundtrips, packet serialization/deserialization, and parameter boundary validation:

```bash
cd tests
cmake -B build -S .
cmake --build build
./build/run_tests
```

Expected output:
```text
==================================================
   Faulter Control Suite — Unit Test Suite
==================================================
[TEST] CRC-16/CCITT verification... PASSED (CRC = 0x29b1)
[TEST] COBS encode/decode roundtrip... PASSED (8 vectors)
[TEST] Protocol packet serialization and parsing... PASSED
[TEST] Parameter validation rules... PASSED
==================================================
   ALL UNIT TESTS PASSED SUCCESSFULLY! (4/4)
==================================================
```

---

## Building the RP2040 Firmware

### Prerequisites
- ARM GCC toolchain (`arm-none-eabi-gcc` and `arm-none-eabi-newlib`)
- CMake >= 3.13
- Bundled Pico SDK submodule (automatically detected inside `firmware/lib/pico-sdk`) or external `PICO_SDK_PATH`

### Firmware Compilation
```bash
cd firmware
cmake -B build -S .
cmake --build build -j$(nproc)
```

The build produces:
- `faulter_fw.uf2` — Drag and drop onto the Pico in BOOTSEL mode
- `faulter_fw.elf` / `faulter_fw.bin` / `faulter_fw.hex`

---

## Default Hardware Pinout

| Pin | Default GPIO | Role | Description |
|---|---|---|---|
| **Glitch Output** | GPIO 19 | Output | High-speed active-HIGH pulse to glitch MOSFET gate |
| **Power Cycling** | GPIO 10 | Output | Active-LOW relay/switch to power-cycle target board |
| **Scope Monitor** | GPIO 9 | Output | Active-HIGH during entire glitch window for scope trigger |

All GPIO pin assignments can be reconfigured dynamically at runtime from the Desktop UI.

---

## Safety & Operational Workflow

To protect target hardware and prevent accidental misfires during security research:
1. **Connect**: Connect to the Pico over USB CDC serial port or select the Virtual Pico Simulator for offline testing.
2. **Configure**: Adjust pulse width, delay, parameter sweep ranges, infinite burst toggle, step pause interval, and power cycling in the configuration panel.
3. **Arm**: Click **ARM GLITCH**. The host validates all parameters and locks them in hardware. Parameter inputs are disabled to prevent mid-flight changes. The **Safety Warning Banner** illuminates in bold amber.
4. **Fire**: Click **FIRE BURST** (or **START BRUTE SWEEP** in sweep mode). The hardware executes the glitch sequence with nanosecond precision via the PIO state machine. Live pulse counts and execution logs stream into the **Telemetry Dashboard & Activity Terminal**.
5. **Abort**: Click **STOP / ABORT** at any time to immediately disarm the hardware or halt an ongoing sweep / continuous burst sequence.
