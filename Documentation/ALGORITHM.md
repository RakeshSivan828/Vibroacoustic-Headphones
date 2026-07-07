# 🧠 Algorithm Design

This document explains the algorithms used in the **Vibroacoustic Headphones** project and the purpose of each processing stage.

---

# 📌 System Processing Pipeline

```text
Audio Signal
      │
      ▼
Differential Audio Sampling
      │
      ▼
Fast Fourier Transform (FFT)
      │
      ▼
Peak Detection
      │
      ▼
Majority Voting
      │
      ▼
Frequency Calibration
      │
      ▼
PWM Mapping
      │
      ▼
Motor Control
      │
      ▼
Vibroacoustic Feedback
```

---

# 1️⃣ Differential Audio Sampling

## Objective

The ESP32 samples the audio signal directly from the headphone speaker output using two ADC inputs.

```text
Audio = RH+ − RH−
```

## Why This Algorithm?

The speaker output is differential rather than single-ended. Measuring the voltage difference improves signal quality and reduces common-mode noise, allowing more reliable frequency analysis.

---

# 2️⃣ Fast Fourier Transform (FFT)

## Objective

The sampled audio signal is converted from the **time domain** into the **frequency domain** using the ArduinoFFT library.

The processing sequence is:

- DC Offset Removal
- Hamming Window
- FFT Computation
- Magnitude Calculation

## Why This Algorithm?

The vibration motors respond to bass frequencies rather than signal amplitude. FFT makes it possible to identify the dominant frequency present in the incoming audio signal.

---

# 3️⃣ Peak Detection

## Objective

After performing the FFT, the algorithm searches for the strongest valid frequency peak.

The detected frequency is accepted only if it falls within the configured frequency range.

## Why This Algorithm?

FFT output often contains harmonics and electrical noise. Peak detection filters out unwanted components and keeps only meaningful bass frequencies.

---

# 4️⃣ Majority Voting Algorithm

## Objective

The detected frequencies are stored inside a rolling history buffer.

The frequency that appears most frequently is selected as the final output.

## Why This Algorithm?

FFT measurements naturally fluctuate between adjacent frequency bins. Majority voting smooths these fluctuations and provides stable vibration feedback without sudden jumps.

---

# 5️⃣ Frequency Calibration

## Objective

Raw FFT frequencies are converted into calibrated frequencies using a lookup table.

Example:

| FFT Frequency | Calibrated Frequency |
|--------------:|---------------------:|
|140.6 Hz|30 Hz|
|156.3 Hz|50 Hz|
|203.1 Hz|70 Hz|
|265.6 Hz|90 Hz|

## Why This Algorithm?

FFT frequency resolution depends on sampling parameters and often introduces measurement error. Calibration compensates for these deviations and improves frequency accuracy.

---

# 6️⃣ PWM Mapping

## Objective

The calibrated frequency is mapped to a PWM duty cycle.

Example:

| Frequency | PWM Duty Cycle |
|-----------|---------------:|
|30 Hz|40%|
|40 Hz|45.6%|
|60 Hz|56.7%|
|80 Hz|67.8%|
|100 Hz|78.9%|
|120 Hz|90%|

Linear interpolation is used between mapping points to generate smooth transitions.

## Why This Algorithm?

Different bass frequencies produce different tactile sensations. PWM mapping allows the vibration intensity to increase progressively as the detected bass frequency increases.

---

# 7️⃣ Vibration Motor Control

## Objective

The calculated PWM value is applied simultaneously to all three coin vibration motors.

```text
Motor 1
Motor 2
Motor 3
```

## Why This Algorithm?

Driving all three motors together provides a uniform vibroacoustic sensation throughout the headphone structure, creating a more immersive user experience.

---

# 🔄 Overall Algorithm

```text
Initialize ESP32
       │
       ▼
Read Audio Samples
       │
       ▼
Remove DC Offset
       │
       ▼
Apply Hamming Window
       │
       ▼
Perform FFT
       │
       ▼
Detect Dominant Frequency
       │
       ▼
Is Frequency Valid?
       │
   ┌───┴────┐
   │        │
 No         Yes
   │        │
Stop     Majority Voting
Motors        │
              ▼
     Frequency Calibration
              │
              ▼
        PWM Calculation
              │
              ▼
     Drive Three Coin Motors
              │
              ▼
        Repeat Continuously
```

---

# ✅ Advantages

- Differential audio acquisition improves signal quality.
- FFT provides efficient real-time frequency analysis.
- Peak detection removes noise and unwanted harmonics.
- Majority voting stabilizes frequency estimation.
- Frequency calibration improves measurement accuracy.
- PWM interpolation produces smooth vibration transitions.
- Low computational complexity enables real-time execution on the ESP32.

---

# 📌 Summary

This project combines differential audio sampling, Fast Fourier Transform (FFT), peak detection, majority voting, frequency calibration, PWM mapping, and real-time motor control to convert low-frequency audio into synchronized tactile feedback.

The complete processing pipeline enables users to **hear and feel** bass frequencies simultaneously, creating an immersive vibroacoustic listening experience using an ESP32-based embedded system.
