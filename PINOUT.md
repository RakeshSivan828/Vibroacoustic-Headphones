# 📌 ESP32 Pin Connections

| ESP32 Pin | Function |
|-----------|----------|
| GPIO34 | Audio Input |
| GPIO25 | Coin Motor 1 |
| GPIO26 | Coin Motor 2 |
| GPIO27 | Coin Motor 3 |
| VIN | 5V Input |
| GND | Ground |

---

# Power Connections

| Device | Connection |
|---------|------------|
| Battery | XL6009 Input |
| XL6009 Output | ESP32 VIN |
| Common Ground | All Components |

---

# Signal Flow

```text
Speaker Output

↓

ESP32 ADC

↓

FFT

↓

PWM

↓

Motors
```
