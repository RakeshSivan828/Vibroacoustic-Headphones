# 🔧 Hardware Documentation

This document describes the hardware components used in the Vibroacoustic Headphones project.

---

# Hardware Components

| Component | Quantity | Purpose |
|-----------|---------:|---------|
| ESP32 Development Board | 1 | Main microcontroller responsible for signal processing and motor control |
| Bluetooth Headphones | 1 | Audio source and mechanical housing |
| Coin Vibration Motors | 3 | Generate tactile feedback |
| XL6009 Boost Converter | 1 | Converts 3.7 V battery output to a stable 5 V supply |
| 3.7 V Li-ion Battery | 1 | Portable power source |
| Power Switch | 1 | Turns the system ON/OFF |
| Connecting Wires | As Required | Electrical connections |

---

# Component Selection

## ESP32

Chosen because of:

- High processing speed
- Built-in ADC
- PWM outputs
- Low power consumption
- Arduino IDE support

---

## Coin Vibration Motors

Chosen because they are:

- Small
- Lightweight
- Easy to integrate
- Low power
- Silent compared to ERM motors

---

## XL6009 Boost Converter

Provides:

- Stable 5V output
- High efficiency
- Battery operation

---

## Battery

Specifications

- Voltage: 3.7V
- Rechargeable
- Portable
- Lightweight

---

# Power Flow

```text
Battery
    │
    ▼
XL6009
    │
    ▼
ESP32
    │
    ▼
Coin Vibration Motors
```
