<div align="center">

# Morse Code Transceiver — ATmega164P

**A bidirectional (logical full-duplex) Morse code transceiver built on the ATmega164P microcontroller.**

Compose ASCII messages via hardware DIP switches, transmit them as optical Morse signals,
and simultaneously receive & decode external Morse transmissions — all in real time.

</div>

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [System Architecture](#system-architecture)
- [Hardware & Pin Mapping](#hardware--pin-mapping)
- [Technical Specifications](#technical-specifications)
- [Morse Timing Reference](#morse-timing-reference)
- [SOS Emergency Detection](#sos-emergency-detection)
- [Getting Started](#getting-started)
- [Repository Structure](#repository-structure)
- [Author](#author)
- [License](#license)

---

## Overview

This embedded system implements a complete Morse code transceiver with two parallel data paths:

| Path   | Direction              | Method                                                          |
|--------|------------------------|-----------------------------------------------------------------|
| **TX** | User → LEDs            | FSM-driven composition + blocking transmission via `delay_ms()` |
| **RX** | External signal → Buffer | Asynchronous ISR sampling at ~50 Hz via Timer0 overflow       |

The user composes messages character-by-character using 7-bit DIP switches, stores them into a local buffer, and triggers optical transmission. Meanwhile, an incoming Morse signal on PORTC is continuously sampled, decoded, and stored into a circular receive buffer — completely independent of the transmission path.

---

## Key Features

- **Logical Full-Duplex** — Transmit and receive Morse messages concurrently; the ISR-based receiver runs in the background while the main loop handles composition and transmission.
- **3-State Finite State Machine** — Clean control flow through `IDLE → COMPOSING → TRANSMITTING` with hardware button transitions.
- **Asynchronous Signal Sampling** — Timer0 overflow ISR fires every ~20 ms, classifying pulse durations into dots (2–8 ticks) and dashes (12–20 ticks).
- **Circular Receive Buffer** — Incoming decoded characters are stored in a ring buffer (`buffer_rx`) with automatic wrap-around, preventing overflow.
- **SOS Distress Detection** — A sequential parser monitors decoded characters for the S-O-S pattern, triggering a 10-second transmission lockout and synchronized LED alarm flashing.
- **Real-Time Input Editing** — SAVE appends the current DIP switch value; DELETE pops the last character (stack behavior).
- **Full A–Z + 0–9 Coverage** — Both the encoder and decoder support the complete ITU Morse code alphabet and numerals.

---

## System Architecture

### State Machine

```
                        MOD (PA0)
              ┌──────────────────────┐
              v                      |
          +--------+            +-----------+
  ------->|  IDLE  |---MOD----->| COMPOSING |<---+
          +--------+   (PA0)   +-----------+    |
              ^                  |    |    |     |
              |            SAVE (PA1) | DELETE   |
              |                  |    | (PA2)    |
              |                  +----+---------+
              |                  |
              |           TRANSMIT (PD7)
              |           & SOS not active
              |                  |
              |                  v
              |          +--------------+
              +----------| TRANSMITTING |
            auto-return  +--------------+
           (TX complete)
```

### Data Flow

```
+-------------------------------------------------------------+
|                        MAIN LOOP                            |
|                                                             |
|  DIP Switches --> buffer_tx[] --> trimite_caracter() --> LEDs (TX)  |
|   (PORTD 0:6)      (SAVE/DEL)     pulse_morse()      PB0, PB1    |
+-------------------------------------------------------------+
                           || concurrent
+-------------------------------------------------------------+
|                   TIMER0 ISR (~20 ms)                       |
|                                                             |
|  PORTC (0:1) --> contor_20ms --> decodeaza_morse() --> buffer_rx[]  |
|  (external in)   pulse classify   dot/dash -> ASCII   (circular)   |
|                                        |                           |
|                                   SOS detector                     |
|                                   (sequential S->O->S match)       |
+-------------------------------------------------------------+
```

---

## Hardware & Pin Mapping

| Port / Pin    | Direction | Function                                                     |
|:--------------|:----------|:-------------------------------------------------------------|
| **PA0**       | Input PU  | MOD / Cancel button                                          |
| **PA1**       | Input PU  | SAVE button — append character to TX buffer                  |
| **PA2**       | Input PU  | DELETE button — pop last character from TX buffer            |
| **PB0**       | Output    | Dot LED (also used for SOS alarm flashing)                   |
| **PB1**       | Output    | Dash LED (also used for SOS alarm flashing)                  |
| **PB2**       | Output    | IDLE state indicator LED                                     |
| **PB3**       | Output    | COMPOSING state indicator LED                                |
| **PB4**       | Output    | TRANSMITTING state indicator LED                             |
| **PC0, PC1**  | Input PU  | External Morse signal input (sampled asynchronously via ISR) |
| **PD0 – PD6** | Input PU  | 7-bit DIP switches for ASCII character binary input          |
| **PD7**       | Input PU  | TRANSMIT trigger button                                      |

> **PU** = Internal pull-up resistor enabled. Buttons/switches are active-low.

---

## Technical Specifications

| Parameter              | Value                        |
|:-----------------------|:-----------------------------|
| Microcontroller        | Microchip ATmega164P         |
| Core Clock             | 10.000000 MHz                |
| Language               | Embedded C                   |
| Development Tool       | CodeVisionAVR / AVR Studio 4 |
| Timer0 Prescaler       | 1024                         |
| Timer0 Preload (TCNT0) | `0x3C` (60)                  |
| ISR Period             | ~20.07 ms (~49.8 Hz)         |
| Data Stack Size        | 256 bytes                    |
| TX/RX Buffer Size      | 20 characters                |

---

## Morse Timing Reference

All timing is derived from `T_BASE = 100 ms`:

| Element               | Duration    | ISR Ticks (~20 ms) |
|:----------------------|:------------|:-------------------|
| Dot (`.`)             | 1T = 100 ms | 2 – 8              |
| Dash (`-`)            | 3T = 300 ms | 12 – 20            |
| Intra-character pause | 1T = 100 ms | —                  |
| Inter-character pause | 3T = 300 ms | ~15                |
| Inter-word pause      | 7T = 700 ms | ~35                |
| End-of-message pause  | >2 s        | >100               |

### Supported Characters

```
A  .-      N  -.      0  -----
B  -...    O  ---     1  .----
C  -.-.    P  .--.    2  ..---
D  -..     Q  --.-    3  ...--
E  .       R  .-.     4  ....-
F  ..-.    S  ...     5  .....
G  --.     T  -       6  -....
H  ....    U  ..-     7  --...
I  ..      V  ...-    8  ---..
J  .---    W  .--     9  ----.
K  -.-     X  -..-
L  .-..    Y  -.--
M  --      Z  --..
```

---

## SOS Emergency Detection

The receiver includes a real-time sequential parser that monitors decoded characters for the **S-O-S** distress pattern:

1. Each decoded character is checked against the expected SOS sequence.
2. Upon detecting a complete S-O-S sequence:
   - **Transmission is locked** (`SOS_activ = 1`) — the TRANSMIT button is ignored.
   - **Both LEDs (PB0, PB1) flash synchronously** at ~2 Hz (250 ms ON / 250 ms OFF).
   - The lockout persists for **10 seconds** (500 x 20 ms ISR ticks).
3. After 10 seconds, the system automatically returns to normal operation.

---

## Getting Started

### Prerequisites

- **Hardware**: ATmega164P development board, LEDs, push-buttons, DIP switches, wiring as per the [pin mapping](#hardware--pin-mapping)
- **Software**: [CodeVisionAVR](http://www.hpinfotech.ro) or AVR Studio 4
- **Programmer**: Any AVR ISP programmer (USBasp, AVRISP mkII, etc.)

### Build & Flash

1. **Clone the repository**
   ```bash
   git clone https://github.com/gaedarius3/Morse-Code-Transceiver-ATmega164P.git
   cd Morse-Code-Transceiver-ATmega164P
   ```

2. **Open the project** in CodeVisionAVR
   - Open `src/main.c`
   - Set the target chip to **ATmega164P** and clock to **10 MHz**

3. **Build** — compile the project (F7 or Build -> Build All)

4. **Flash** — program the ATmega164P via your ISP programmer

### Usage

| Action                  | How                                                      |
|-------------------------|----------------------------------------------------------|
| Enter compose mode      | Press **MOD** (PA0)                                      |
| Set a character         | Configure 7-bit ASCII on the DIP switches (PD0–PD6)     |
| Save character          | Press **SAVE** (PA1)                                     |
| Delete last character   | Press **DELETE** (PA2)                                   |
| Transmit message        | Press **TRANSMIT** (PD7)                                 |
| Cancel / return to IDLE | Press **MOD** (PA0) again                                |

---

## Repository Structure

```
Morse-Code-Transceiver-ATmega164P/
├── src/
│   └── main.c                              # Core transceiver firmware (FSM + ISR)
├── docs/
│   └── Morse_Transceiver_Documentation.pdf # Detailed architecture & lab report
├── .gitignore                              # Ignored build & temporary files
├── LICENSE                                 # MIT License
└── README.md                               # This file
```

---

## Author

**Gae Darius Andrei**
*Student — University Politehnica of Bucharest, Faculty of ETTI*

[GitHub](https://github.com/gaedarius3) • [LinkedIn](https://www.linkedin.com/in/darius-andrei-gae-277320380)

---

## License

This project is licensed under the [MIT License](LICENSE).