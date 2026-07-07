# 🎧 Vibroacoustic Headphones

> An ESP32-powered vibroacoustic headphone system that converts low-frequency audio (40–120 Hz) into synchronized haptic feedback using three miniature coin vibration motors.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Language](https://img.shields.io/badge/Language-C/C++-green)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 📖 Overview

Vibroacoustic Headphones is an embedded systems project designed to enhance the traditional listening experience by converting low-frequency audio into synchronized tactile feedback.

The ESP32 continuously analyzes the audio signal from the headphone speaker, detects bass frequencies between **40 Hz and 120 Hz**, and drives three miniature coin vibration motors with varying PWM intensity. As a result, users can physically feel the bass in addition to hearing it, creating a more immersive audio experience.

---

## ✨ Features

- 🎵 Real-time low-frequency frequency detection
- 🎧 Three integrated coin vibration motors
- ⚡ Dynamic PWM vibration control
- 🔋 Battery-powered portable system
- 📡 ESP32-based embedded processing
- 🎚️ Frequency range: **40 Hz – 120 Hz**
- 🧩 Compact integration inside Bluetooth headphones

---

# 🛠 Hardware Used

| Component | Quantity |
|-----------|---------:|
| ESP32 Development Board | 1 |
| Bluetooth Headphones | 1 |
| Coin Vibration Motors | 3 |
| XL6009 Boost Converter | 1 |
| 3.7 V Li-ion Battery | 1 |
| Power Switch | 1 |
| Connecting Wires | As Required |

---

# 💻 Software

- Arduino IDE
- ESP32 Board Package

---

# ⚙️ Working Principle

```text
Audio Source
      │
      ▼
Bluetooth Headphones
      │
      ▼
Speaker Output
      │
      ▼
ESP32
(Frequency Detection)
      │
      ▼
PWM Mapping
      │
      ▼
Three Coin Vibration Motors
      │
      ▼
Haptic Feedback
```

The ESP32 continuously samples the incoming audio signal, detects frequencies within **40–120 Hz**, maps them to PWM values, and drives the vibration motors accordingly.

---

# 📂 Project Structure

```text
Vibroacoustic-Headphones
│
├── Arduino_Code/
│   └── VibroacousticHeadphone.ino
│
├── Images/
│   ├── Prototype.jpg
│   ├── Wiring.jpg
│   └── Demo.gif
│
├── Documentation/
│
├── README.md
│
└── LICENSE
```

---

# 🚀 Getting Started

1. Clone this repository.

```bash
git clone https://github.com/yourusername/Vibroacoustic-Headphones.git
```

2. Open the Arduino sketch.

3. Install the ESP32 board package.

4. Connect the ESP32.

5. Upload the firmware.

6. Power the system.

7. Play bass audio (40–120 Hz) and experience synchronized vibration feedback.

---

# 📸 Project Images

Add project images inside the **Images** folder.

Example:

- Prototype
- Internal Wiring
- Headphone Assembly
- Final Setup

---

# 🎥 Demonstration

Add your demo video or GIF here.

Example:

```
Images/Demo.gif
```

---

# 🎯 Applications

- 🎵 Immersive Music Experience
- 🎮 Gaming
- 🥽 Virtual Reality
- ♿ Accessibility Research
- 🔬 Haptic Research
- 📚 Embedded Systems Education

---

# 🔮 Future Improvements

- Stereo vibration mapping
- DSP-based audio processing
- Mobile app integration
- Custom PCB
- Rechargeable battery management
- Adaptive vibration intensity

---

# 🤝 Contributing

Contributions are welcome.

1. Fork the repository
2. Create a new branch
3. Commit your changes
4. Open a Pull Request

---

# 📄 License

This project is licensed under the MIT License.

---

# 👨‍💻 Author

**Rakesh Sivan**

Electronics and Communication Engineering (ECE)

Passionate about Embedded Systems, IoT, Wearable Technology, and Artificial Intelligence.

⭐ If you like this project, consider giving it a star.
