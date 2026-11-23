# Faulter Serial Protocol Specification v1.0

> **Project:** Faulter — PicoGlitcher Control Suite  
> **Date:** 2026-11-14  
> **Status:** DRAFT — awaiting confirmation before implementation

---

## 1. Overview

The Faulter protocol is a binary, COBS-framed, request/response protocol
between the host application (desktop PC) and the Faulter firmware (RP2040).
It runs over USB CDC serial.

### 1.1 Design Goals

- **Reliable framing** — COBS encoding guarantees zero bytes never appear in
  the payload, so `0x00` is an unambiguous frame delimiter. No desync issues.
- **Acknowledged** — every command gets an explicit ACK or NAK response.
- **Versioned** — protocol version negotiation on connect.
- **Extensible** — parameter IDs and command opcodes leave room for future
  glitch types (clock glitch, EMFI) without protocol changes.
- **Telemetry-capable** — firmware reports actual measured timing after each
  glitch execution.

### 1.2 Physical Layer

| Property | Value |
|----------|-------|
| Interface | USB CDC (TinyUSB) |
| Baud rate | 921600 (high speed for low-latency telemetry) |
| Flow control | None (USB CDC handles flow control at the USB level) |

---

## 2. Framing: COBS

Every frame on the wire looks like:

```
[COBS-encoded payload bytes] [0x00]
```

- The payload (described in Section 3) is COBS-encoded before transmission.
- A single `0x00` byte terminates the frame.
- Maximum decoded payload size: **64 bytes** (sufficient for all V1 commands).
- Receiver discards any frame whose decoded payload exceeds 64 bytes.

### 2.1 Why COBS

| Alternative | Reason rejected |
|------------|-----------------|
| Raw ASCII text (like reference) | Ambiguous parsing, no checksums, poor performance |
| SLIP | Requires escaping two byte values; variable expansion ratio |
| Length-prefixed | Requires knowing length before frame start; harder to resync |
| COBS | Fixed 1-byte overhead per 254 bytes; guaranteed `0x00` delimiter; simple to implement; constant-time encode/decode |

---

## 3. Packet Structure

After COBS decoding, every packet has this structure:

```
Offset  Size    Field
──────  ────    ─────
0       1       Packet Type (see Section 4)
1       1       Sequence Number (0–255, wrapping)
2       N       Payload (0–60 bytes, type-dependent)
2+N     2       CRC-16/CCITT (big-endian, over bytes 0..2+N-1)
```

### 3.1 Sequence Numbers

- Host increments its sequence number (0–255, wrapping) for each command sent.
- Firmware responses echo back the sequence number of the command they are
  responding to.
- Unsolicited firmware messages (e.g., state change notifications) use
  sequence number `0xFF`.

### 3.2 CRC-16/CCITT

- Polynomial: `0x1021`
- Initial value: `0xFFFF`
- Computed over all bytes before the CRC field (type + seq + payload).
- Big-endian: MSB first on the wire.
- Receiver must validate CRC and discard frames with CRC mismatch.

---

## 4. Packet Types

### 4.1 Host → Firmware (Commands)

| Type | Hex | Name | Payload | Description |
|------|-----|------|---------|-------------|
| `0x01` | `CMD_PING` | Ping | `[proto_version_major: u8, proto_version_minor: u8]` | Handshake / version check |
| `0x02` | `CMD_SET_PARAM` | Set Parameter | `[param_id: u8, value: u32-LE]` | Set a configuration parameter |
| `0x03` | `CMD_GET_PARAM` | Get Parameter | `[param_id: u8]` | Query a parameter's current value |
| `0x04` | `CMD_ARM` | Arm | (empty) | Validate config and enter ARMED state |
| `0x05` | `CMD_FIRE` | Fire | (empty) | Execute the glitch (must be ARMED) |
| `0x06` | `CMD_ABORT` | Abort | (empty) | Cancel armed state or abort active glitch |
| `0x07` | `CMD_GET_STATUS` | Get Status | (empty) | Query firmware state + last result |
| `0x08` | `CMD_RESET` | Reset | (empty) | Reset all parameters to defaults, return to IDLE |

### 4.2 Firmware → Host (Responses)

| Type | Hex | Name | Payload | Description |
|------|-----|------|---------|-------------|
| `0x81` | `RSP_PONG` | Pong | `[proto_major: u8, proto_minor: u8, fw_major: u8, fw_minor: u8, fw_patch: u8, capabilities: u16-LE]` | Version + capabilities response |
| `0x82` | `RSP_ACK` | Acknowledge | `[cmd_type: u8]` | Command accepted |
| `0x83` | `RSP_NAK` | Negative Ack | `[cmd_type: u8, error_code: u8]` | Command rejected (see error codes) |
| `0x84` | `RSP_PARAM_VALUE` | Parameter Value | `[param_id: u8, value: u32-LE]` | Response to GET_PARAM |
| `0x85` | `RSP_STATUS` | Status | `[state: u8, last_error: u8, last_delay_ns: u32-LE, last_pulse_ns: u32-LE]` | Current state + telemetry |
| `0x86` | `RSP_GLITCH_DONE` | Glitch Complete | `[result: u8, actual_delay_ns: u32-LE, actual_pulse_ns: u32-LE]` | Unsolicited: glitch finished |

---

## 5. Parameter IDs

### 5.1 Voltage Glitch Parameters

| ID | Hex | Name | Type | Unit | Range | Default | Description |
|----|-----|------|------|------|-------|---------|-------------|
| `0x01` | `PARAM_GLITCH_TYPE` | Glitch Type | `u32` | enum | 0–1 | 0 | 0 = Voltage, 1 = Clock (reserved) |
| `0x02` | `PARAM_DELAY_NS` | Delay | `u32` | nanoseconds | 0 – 4,294,967,295 | 0 | Post-fire delay before glitch pulse |
| `0x03` | `PARAM_PULSE_NS` | Pulse Width | `u32` | nanoseconds | 8 – 4,294,967,295 | 100 | Duration of glitch pulse |
| `0x04` | `PARAM_REPEAT_COUNT` | Repeat Count | `u32` | count | 1 – 65,535 | 1 | Number of pulses per fire command |
| `0x05` | `PARAM_REPEAT_DELAY_NS` | Inter-Repeat Delay | `u32` | nanoseconds | 0 – 4,294,967,295 | 1000 | Gap between repeated pulses |
| `0x06` | `PARAM_PWR_CYCLE_EN` | Power Cycle Enable | `u32` | bool | 0–1 | 0 | Enable auto power-cycle before glitch |
| `0x07` | `PARAM_PWR_CYCLE_OFF_MS` | Power Cycle Off Time | `u32` | milliseconds | 1 – 10,000 | 50 | How long target stays powered off |
| `0x08` | `PARAM_GLITCH_PIN` | Glitch Output Pin | `u32` | GPIO# | 0–28 | 19 | Which GPIO drives the glitch MOSFET |
| `0x09` | `PARAM_PWR_CYCLE_PIN` | Power Cycle Pin | `u32` | GPIO# | 0–28 | 10 | Which GPIO controls power cycling |
| `0x0A` | `PARAM_MONITOR_PIN` | Monitor Output Pin | `u32` | GPIO# | 0–28 | 9 | Oscilloscope monitor output (HIGH during glitch window) |
| `0x0B` | `PARAM_MONITOR_EN` | Monitor Enable | `u32` | bool | 0–1 | 1 | Enable/disable monitor pin output |
| `0x0C` | `PARAM_BURST_INFINITE` | Infinite Continuous Burst | `u32` | bool | 0–1 | 0 | Loop burst execution indefinitely until ABORT |
| `0x0D` | `PARAM_BURST_GAP_MS` | Inter-Burst Gap / Timeout | `u32` | milliseconds | 0–60,000 | 100 | Delay in ms between bursts during infinite execution |

### 5.2 Reserved Parameter ID Ranges

| Range | Purpose |
|-------|---------|
| `0x01–0x1F` | Core glitch parameters |
| `0x20–0x3F` | Trigger parameters (future) |
| `0x40–0x5F` | Clock glitch parameters (future) |
| `0x60–0x7F` | Sweep/automation parameters (future) |
| `0x80–0xFF` | Vendor/user-defined |

---

## 6. Firmware State Machine

```
                    ┌──────────────────────────┐
                    │                          │
       ┌────────────▼───────────┐              │
       │         IDLE           │◄─── RESET    │
       │  (accepting config)    │              │
       └────────────┬───────────┘              │
                    │ CMD_ARM                   │
                    │ (validates params)         │
       ┌────────────▼───────────┐              │
       │        ARMED           │──── ABORT ───┘
       │  (ready to fire)       │
       └────────────┬───────────┘
                    │ CMD_FIRE
                    │
       ┌────────────▼───────────┐
       │       FIRING           │──── ABORT ───┐
       │  (executing glitch)    │              │
       └────────────┬───────────┘              │
                    │ (complete)                │
       ┌────────────▼───────────┐              │
       │         IDLE           │◄─────────────┘
       │  (RSP_GLITCH_DONE      │
       │   sent with telemetry) │
       └────────────────────────┘
```

### 6.1 State Values

| Value | Name | Description |
|-------|------|-------------|
| `0x00` | `STATE_IDLE` | Default. Accepts parameter changes. |
| `0x01` | `STATE_ARMED` | Parameters validated and locked. Waiting for FIRE. |
| `0x02` | `STATE_PWR_CYCLING` | Power-cycling target (if enabled). |
| `0x03` | `STATE_FIRING` | Glitch pulse(s) in progress. |
| `0x04` | `STATE_ERROR` | An error occurred. RESET or ABORT to recover. |

### 6.2 State Transition Rules

- `CMD_SET_PARAM` / `CMD_GET_PARAM`: only valid in `IDLE`.
- `CMD_ARM`: only valid in `IDLE`. Validates all parameters. NAK if invalid.
- `CMD_FIRE`: only valid in `ARMED`. Starts glitch sequence.
- `CMD_ABORT`: valid in `ARMED` or `FIRING`. Returns to `IDLE`.
- `CMD_RESET`: valid in any state. Returns to `IDLE` with default params.
- `CMD_PING` / `CMD_GET_STATUS`: valid in any state.

---

## 7. Error Codes

| Code | Hex | Name | Description |
|------|-----|------|-------------|
| `0x00` | `ERR_NONE` | No error | — |
| `0x01` | `ERR_UNKNOWN_CMD` | Unknown command | Unrecognized packet type |
| `0x02` | `ERR_INVALID_STATE` | Invalid state | Command not valid in current state |
| `0x03` | `ERR_INVALID_PARAM` | Invalid parameter | Unknown parameter ID |
| `0x04` | `ERR_VALUE_OUT_OF_RANGE` | Value out of range | Parameter value outside valid range |
| `0x05` | `ERR_CRC_MISMATCH` | CRC mismatch | Frame CRC check failed |
| `0x06` | `ERR_FRAME_TOO_LONG` | Frame too long | Decoded frame exceeds 64 bytes |
| `0x07` | `ERR_NOT_CONFIGURED` | Not configured | ARM attempted but required params not set |
| `0x08` | `ERR_HARDWARE_FAULT` | Hardware fault | GPIO or PIO initialization failed |
| `0x09` | `ERR_PROTO_VERSION` | Protocol mismatch | Host/firmware protocol version incompatible |
| `0x0A` | `ERR_ABORTED` | Aborted | Operation was aborted by user |

---

## 8. Capabilities Bitfield (RSP_PONG)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | `CAP_VOLTAGE_GLITCH` | Supports voltage glitching |
| 1 | `CAP_CLOCK_GLITCH` | Supports clock glitching |
| 2 | `CAP_TRIGGER_GPIO` | Supports GPIO trigger |
| 3 | `CAP_TRIGGER_UART` | Supports UART pattern trigger |
| 4 | `CAP_POWER_CYCLE` | Supports target power cycling |
| 5 | `CAP_MONITOR_PIN` | Supports monitor output pin |
| 6 | `CAP_BURST_MODE` | Supports multi-pulse burst |
| 7–15 | Reserved | — |

V1 firmware will report: `CAP_VOLTAGE_GLITCH | CAP_POWER_CYCLE | CAP_MONITOR_PIN | CAP_BURST_MODE` = `0x0071`

---

## 9. Timing & Performance

### 9.1 Glitch Generation via PIO

The RP2040 PIO state machine will handle the critical timing path:

```
PIO Program (pseudocode):
    pull block          ; wait for delay count from FIFO
    mov x, osr          ; load delay counter
delay_loop:
    jmp x-- delay_loop  ; count down (1 cycle = 8ns per iteration)
    set pins, 1         ; glitch ON — deterministic, zero jitter
    pull block          ; get pulse width count from FIFO
    mov x, osr          ; load pulse counter
pulse_loop:
    jmp x-- pulse_loop  ; count down
    set pins, 0         ; glitch OFF
    irq 0               ; signal CPU that glitch is complete
```

### 9.2 Timing Resolution

| Metric | Value |
|--------|-------|
| PIO clock | 125 MHz (default `clk_sys`) |
| PIO cycle time | 8 ns |
| Minimum delay | 0 ns (no delay) |
| Minimum pulse width | 8 ns (1 PIO cycle) |
| Maximum single-counter span | ~34.36 seconds (2³² × 8 ns) |
| Jitter | **0 ns** — PIO runs independently of CPU, interrupts, USB |
| Trigger-to-pulse latency | Deterministic (PIO instruction count × 8 ns) |

### 9.3 Telemetry Measurement

The firmware will use a hardware timer (`hardware_timer`) to capture
timestamps at glitch-start and glitch-end, reporting the actual elapsed
time in the `RSP_GLITCH_DONE` response. This lets the host verify that
the PIO achieved the requested timing.

---

## 10. Connection Lifecycle

```
Host                                    Firmware
  │                                        │
  │──── CMD_PING [v1.0] ─────────────────►│
  │◄─── RSP_PONG [v1.0, fw1.0.0, caps] ──│
  │                                        │
  │──── CMD_SET_PARAM [DELAY=1000] ──────►│
  │◄─── RSP_ACK ─────────────────────────│
  │                                        │
  │──── CMD_SET_PARAM [PULSE=500] ───────►│
  │◄─── RSP_ACK ─────────────────────────│
  │                                        │
  │──── CMD_SET_PARAM [PWR_CYCLE_EN=1] ──►│
  │◄─── RSP_ACK ─────────────────────────│
  │                                        │
  │──── CMD_SET_PARAM [PWR_OFF_MS=100] ──►│
  │◄─── RSP_ACK ─────────────────────────│
  │                                        │
  │──── CMD_ARM ─────────────────────────►│
  │◄─── RSP_ACK ─────────────────────────│
  │                                        │
  │──── CMD_FIRE ────────────────────────►│
  │◄─── RSP_ACK ─────────────────────────│
  │     [firmware: pwr cycle → fire PIO]   │
  │◄─── RSP_GLITCH_DONE [ok, 1004ns, 504ns]│
  │                                        │
  │──── CMD_GET_STATUS ──────────────────►│
  │◄─── RSP_STATUS [IDLE, no error, ...] ─│
  │                                        │
  │──── CMD_RESET ───────────────────────►│
  │◄─── RSP_ACK ─────────────────────────│
```

---

## 11. Wire Format Example

**Example:** `CMD_SET_PARAM` setting `PARAM_DELAY_NS` (0x02) to 1000 ns (0x000003E8):

```
Before COBS encoding:
  Type:     0x02 (CMD_SET_PARAM)
  Seq:      0x01
  Payload:  0x02 0xE8 0x03 0x00 0x00  (param_id=0x02, value=1000 LE)
  CRC:      0xXX 0xXX (CRC-16 over preceding 7 bytes)
  Total:    9 bytes

After COBS encoding + delimiter:
  [COBS bytes...] 0x00
```

---

## 12. Implementation Notes

### 12.1 Firmware Side
- COBS decode/encode: ~50 lines of C, no dynamic allocation.
- CRC-16: lookup table (~512 bytes ROM) or bitwise (~20 lines).
- State machine: simple switch-case, all transitions in one function.
- PIO program: ~10 instructions, uses 1 state machine on PIO0.
- USB CDC: handled by TinyUSB (included in Pico SDK via `pico_stdio_usb`).

### 12.2 Host Side
- Serial I/O: platform abstraction over OS serial APIs (or a library like
  `libserialport` / Boost.Asio / Qt's `QSerialPort`).
- COBS + CRC: same implementation as firmware (portable C/C++).
- Timeout: host should time out and retry if no response within 500 ms.
- Reconnect: host should detect USB disconnect and attempt reconnection.
