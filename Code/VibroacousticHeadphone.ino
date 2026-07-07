/*
  ============================================================================
  Bluetooth Vibroacoustic Headphone
  ----------------------------------------------------------------------------
  Hardware   : ESP32 (Arduino Core 3.x)
  Headphone  : boAt Rockerz 512 ANC (Bluetooth speaker, tapped differentially)
  Audio In   : RH+ -> GPIO34 (ADC1_CH6)
               RH- -> GPIO35 (ADC1_CH7)
  Vibration  : LEFT  MOTOR -> GPIO25
               RIGHT MOTOR -> GPIO26
               HEAD  MOTOR -> GPIO27
  ----------------------------------------------------------------------------
  Pipeline:
    1. Sample differential audio (RH+ - RH-) at 4000 Hz, 256 samples.
    2. Remove DC, apply Hamming window, run forward FFT, get magnitude.
    3. Find strongest valid bass peak (reject weak / impossible peaks).
    4. Stabilize the detected frequency via a 10-sample majority vote.
    5. Calibrate the stabilized FFT frequency into an estimated real
       bass frequency using an experimentally derived lookup table.
    6. Convert the estimated frequency into a PWM duty cycle (NOT based
       on FFT magnitude) and drive all three vibration motors identically.
    7. Print full debug information every cycle.
  ============================================================================
*/

#include <arduinoFFT.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// ---- ADC input pins (differential speaker tap) ----
static const uint8_t PIN_RH_PLUS  = 34;
static const uint8_t PIN_RH_MINUS = 35;

// ---- Vibration motor output pins ----
static const uint8_t PIN_MOTOR_LEFT  = 25;
static const uint8_t PIN_MOTOR_RIGHT = 26;
static const uint8_t PIN_MOTOR_HEAD  = 27;

// ---- PWM (LEDC) configuration ----
static const uint32_t PWM_FREQ_HZ   = 5000;   // Motor drive PWM frequency
static const uint8_t  PWM_RESOLUTION_BITS = 8; // 8-bit -> 0..255 duty

// ---- FFT configuration ----
static const uint16_t FFT_SAMPLES     = 256;
static const double   SAMPLING_FREQ_HZ = 4000.0;
static const uint32_t SAMPLE_PERIOD_US = (uint32_t)(1000000.0 / SAMPLING_FREQ_HZ);

// ---- Peak detection thresholds ----
static const double MIN_VALID_MAGNITUDE = 800.0;  // Ignore weak peaks
static const double MIN_VALID_FREQ_HZ   = 20.0;   // Reject sub-audible/DC leakage
static const double MAX_VALID_FREQ_HZ   = 500.0;  // Reject implausible bass peaks

// ---- Stability / majority voting ----
static const uint8_t HISTORY_SIZE = 10;
static const double  FREQ_MATCH_TOLERANCE_HZ = 6.0; // Bucket width for "same" frequency
static const double  MIN_CONFIDENCE_PERCENT  = 50.0;

// ---- Calibration ----
static const double CALIBRATION_RAW_TOLERANCE_HZ = 8.0; // Match window for raw->actual lookup
static const double CALIBRATION_CROSSOVER_HZ     = 130.0; // Above this, trust raw FFT directly

// ============================================================================
// STRUCTS
// ============================================================================

// Experimental calibration point: detected (raw FFT) frequency -> actual bass frequency
struct CalibrationPoint {
  double rawFreq;
  double actualFreq;
};

// PWM mapping point: actual bass frequency -> motor duty percentage
struct PWMPoint {
  double freqHz;
  double dutyPercent;
};

// Result of the peak search stage
struct PeakResult {
  double frequency;
  double magnitude;
  bool   valid;
};

// Result of the stabilization stage
struct StabilityResult {
  double stableFreq;
  double confidencePercent;
};

// ============================================================================
// CALIBRATION TABLE (experimental, order matters for ambiguous raw values)
// ============================================================================
static const CalibrationPoint CALIBRATION_TABLE[] = {
  {140.6,  30.0},
  {125.0,  40.0},
  {156.3,  50.0},
  {187.5,  60.0},
  {203.1,  70.0},
  {234.4,  80.0},
  {265.6,  90.0},
  {125.0, 100.0},
  {125.0, 110.0},
  {125.0, 120.0},
  {296.9, 100.0}   // Secondary harmonic of 100 Hz
};
static const uint8_t CALIBRATION_TABLE_SIZE =
    sizeof(CALIBRATION_TABLE) / sizeof(CALIBRATION_TABLE[0]);

// ============================================================================
// PWM MAPPING TABLE (monotonic in frequency, used for linear interpolation)
// Rescaled so the full 30-120 Hz bass range drives motors between 40% and
// 90% duty (instead of the original 15%-90% range).
// ============================================================================
static const PWMPoint PWM_TABLE[] = {
  { 30.0, 40.0},
  { 40.0, 45.6},
  { 50.0, 51.1},
  { 60.0, 56.7},
  { 70.0, 62.2},
  { 80.0, 67.8},
  { 90.0, 73.3},
  {100.0, 78.9},
  {110.0, 84.4},
  {120.0, 90.0}
};
static const uint8_t PWM_TABLE_SIZE = sizeof(PWM_TABLE) / sizeof(PWM_TABLE[0]);

// ============================================================================
// GLOBAL STATE
// ============================================================================

ArduinoFFT<double> FFT = ArduinoFFT<double>();

double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];

double   freqHistory[HISTORY_SIZE];
uint8_t  historyIndex = 0;
bool     historyFilled = false;

double stableFrequencyPrev = 0.0; // Last accepted stable frequency (held on low confidence)

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================
void   sampleAudio();
void   computeFFT();
PeakResult findPeak();
void   updateHistory(double freq);
StabilityResult majorityVote();
double calculateConfidence(double modeFreq, uint8_t matchCount, uint8_t sampleCount);
double calibrateFrequency(double stableFreq);
double calculatePWM(double estimatedFreq);
void   driveMotors(uint8_t pwmValue);
void   printResults(double rawFreq, double stableFreq, double estimatedFreq,
                     double confidence, uint8_t pwmValue);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  // ---- ADC configuration ----
  analogReadResolution(12);              // 0..4095
  analogSetAttenuation(ADC_11db);        // Full ~0-3.3V range
  pinMode(PIN_RH_PLUS, INPUT);
  pinMode(PIN_RH_MINUS, INPUT);

  // ---- Motor PWM configuration (ESP32 Arduino Core 3.x pin-based LEDC API) ----
  ledcAttach(PIN_MOTOR_LEFT,  PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttach(PIN_MOTOR_RIGHT, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttach(PIN_MOTOR_HEAD,  PWM_FREQ_HZ, PWM_RESOLUTION_BITS);

  // ---- Initialize history buffer ----
  for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
    freqHistory[i] = 0.0;
  }

  Serial.println(F("Bluetooth Vibroacoustic Headphone - Initialized"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  // 1. Acquire audio samples
  sampleAudio();

  // 2. Run FFT pipeline (DC removal, windowing, forward FFT, magnitude)
  computeFFT();

  // 3. Locate strongest valid bass peak
  PeakResult peak = findPeak();

  // 4. Update rolling history with the raw detected frequency (0 if invalid)
  double rawFreqForHistory = peak.valid ? peak.frequency : 0.0;
  updateHistory(rawFreqForHistory);

  // 5. Majority vote across history -> stable frequency + confidence
  // 5. Majority vote across history -> stable frequency + confidence
StabilityResult stability = majorityVote();

double stableFrequency;

if (!peak.valid) {
    // No valid FFT peak detected -> stop motors
    stableFrequency = 0.0;
}
else if (stability.confidencePercent < MIN_CONFIDENCE_PERCENT) {
    // Low confidence -> keep previous frequency
    stableFrequency = stableFrequencyPrev;
}
else {
    // Good detection
    stableFrequency = stability.stableFreq;
    stableFrequencyPrev = stableFrequency;
}

  // 6. Calibrate stable frequency into an estimated real bass frequency
  double estimatedFrequency = calibrateFrequency(stableFrequency);

  // 7. Compute PWM duty purely from estimated frequency
  double dutyPercent = calculatePWM(estimatedFrequency);
  uint8_t pwmValue = (uint8_t)constrain((dutyPercent / 100.0) * 255.0, 0, 255);

  // 8. Drive all three motors identically
  driveMotors(pwmValue);

  // 9. Debug output
  printResults(peak.valid ? peak.frequency : 0.0, stableFrequency,
               estimatedFrequency, stability.confidencePercent, pwmValue);

  delay(20); // Small pacing delay between acquisition cycles
}

// ============================================================================
// sampleAudio()
// Reads the differential speaker signal (RH+ - RH-) at a fixed sampling
// rate and fills the FFT input buffers. Imaginary part is zeroed.
// ============================================================================
void sampleAudio() {
  uint32_t nextSampleTime = micros();

  for (uint16_t i = 0; i < FFT_SAMPLES; i++) {
    // Wait until the scheduled sample time to keep a steady sampling rate
    while ((int32_t)(micros() - nextSampleTime) < 0) {
      // busy-wait for precise timing
    }
    nextSampleTime += SAMPLE_PERIOD_US;

    int rhPlus  = analogRead(PIN_RH_PLUS);
    int rhMinus = analogRead(PIN_RH_MINUS);

    // Differential audio sample
    double audio = (double)(rhPlus - rhMinus);

    vReal[i] = audio;
    vImag[i] = 0.0;
  }
}

// ============================================================================
// computeFFT()
// Applies DC removal, Hamming windowing, forward FFT, and converts the
// complex result to magnitude, all in-place on vReal/vImag.
// ============================================================================
void computeFFT() {
 FFT.dcRemoval(vReal, FFT_SAMPLES);
  FFT.windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, FFT_SAMPLES);
}

// ============================================================================
// findPeak()
// Scans the magnitude spectrum (excluding DC bin) for the strongest bin,
// then validates it against magnitude and frequency-range constraints.
// ============================================================================
PeakResult findPeak() {
  PeakResult result;
  result.frequency = 0.0;
  result.magnitude = 0.0;
  result.valid = false;

  double binResolutionHz = SAMPLING_FREQ_HZ / (double)FFT_SAMPLES;

  // Only the first half of the spectrum is meaningful (Nyquist)
  uint16_t usableBins = FFT_SAMPLES / 2;

  double bestMagnitude = 0.0;
  uint16_t bestBin = 0;

  // Skip bin 0 (DC) explicitly
  for (uint16_t bin = 1; bin < usableBins; bin++) {
    if (vReal[bin] > bestMagnitude) {
      bestMagnitude = vReal[bin];
      bestBin = bin;
    }
  }

  if (bestBin == 0) {
    return result; // Nothing found
  }

  double candidateFreq = bestBin * binResolutionHz;

  // Reject weak peaks
  if (bestMagnitude < MIN_VALID_MAGNITUDE) {
    return result;
  }

  // Reject impossible peaks (out of plausible bass range)
  if (candidateFreq < MIN_VALID_FREQ_HZ || candidateFreq > MAX_VALID_FREQ_HZ) {
    return result;
  }

  result.frequency = candidateFreq;
  result.magnitude = bestMagnitude;
  result.valid = true;
  return result;
}

// ============================================================================
// updateHistory()
// Pushes the latest detected raw frequency into the circular history buffer
// used for majority-vote stabilization.
// ============================================================================
void updateHistory(double freq) {
  freqHistory[historyIndex] = freq;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyIndex == 0) {
    historyFilled = true;
  }
}

// ============================================================================
// majorityVote()
// Groups the frequency history into tolerance-based buckets, finds the mode
// (most frequent bucket), and returns its representative frequency along
// with the confidence of that vote.
// ============================================================================
StabilityResult majorityVote() {
  StabilityResult result;
  result.stableFreq = 0.0;
  result.confidencePercent = 0.0;

  uint8_t validCount = historyFilled ? HISTORY_SIZE : historyIndex;
  if (validCount == 0) {
    return result;
  }

  double bestRepresentative = 0.0;
  uint8_t bestCount = 0;

  // For each entry, count how many other entries fall within tolerance of it
  for (uint8_t i = 0; i < validCount; i++) {
    double candidate = freqHistory[i];
    if (candidate <= 0.0) {
      continue; // Skip invalid/zero entries (no detection that cycle)
    }

    uint8_t matchCount = 0;
    double sum = 0.0;

    for (uint8_t j = 0; j < validCount; j++) {
      double other = freqHistory[j];
      if (other <= 0.0) continue;
      if (fabs(other - candidate) <= FREQ_MATCH_TOLERANCE_HZ) {
        matchCount++;
        sum += other;
      }
    }

    if (matchCount > bestCount) {
      bestCount = matchCount;
      bestRepresentative = sum / (double)matchCount; // Average within the bucket
    }
  }

  if (bestCount == 0) {
    return result; // No valid detections at all in the window
  }

  result.stableFreq = bestRepresentative;
  result.confidencePercent = calculateConfidence(bestRepresentative, bestCount, validCount);
  return result;
}

// ============================================================================
// calculateConfidence()
// Confidence is the proportion of the history window that agrees with the
// winning (mode) frequency bucket, expressed as a percentage.
// ============================================================================
double calculateConfidence(double modeFreq, uint8_t matchCount, uint8_t sampleCount) {
  (void)modeFreq; // Reserved for future weighting; currently unused in formula
  if (sampleCount == 0) return 0.0;
  return (100.0 * (double)matchCount) / (double)sampleCount;
}

// ============================================================================
// calibrateFrequency()
// Converts a stabilized raw FFT frequency into an estimated true bass
// frequency using the experimental calibration table below 130 Hz. Above
// the crossover, the raw FFT frequency is trusted directly.
// ============================================================================
double calibrateFrequency(double stableFreq) {
  if (stableFreq <= 0.0) {
    return 0.0;
  }

  if (stableFreq > CALIBRATION_CROSSOVER_HZ) {
    return stableFreq; // Above crossover: use actual FFT frequency as-is
  }

  // Search calibration table in defined order; first sufficiently close
  // match wins (resolves ambiguous duplicate raw values deterministically).
  for (uint8_t i = 0; i < CALIBRATION_TABLE_SIZE; i++) {
    double diff = fabs(stableFreq - CALIBRATION_TABLE[i].rawFreq);
    if (diff <= CALIBRATION_RAW_TOLERANCE_HZ) {
      return CALIBRATION_TABLE[i].actualFreq;
    }
  }

  // No calibration match found: fall back to the raw stabilized frequency
  return stableFreq;
}

// ============================================================================
// calculatePWM()
// Maps the estimated bass frequency to a motor duty cycle percentage using
// piecewise-linear interpolation over the PWM_TABLE. PWM depends ONLY on
// frequency, never on FFT magnitude.
// ============================================================================
double calculatePWM(double estimatedFreq) {
 if (estimatedFreq < 30.0 || estimatedFreq >=125.0)
    return 0.0;

  // Clamp to table bounds
  if (estimatedFreq <= PWM_TABLE[0].freqHz) {
    return PWM_TABLE[0].dutyPercent;
  }
  if (estimatedFreq >= PWM_TABLE[PWM_TABLE_SIZE - 1].freqHz) {
    return PWM_TABLE[PWM_TABLE_SIZE - 1].dutyPercent;
  }

  // Linear interpolation between the two bracketing table points
  for (uint8_t i = 0; i < PWM_TABLE_SIZE - 1; i++) {
    double f0 = PWM_TABLE[i].freqHz;
    double f1 = PWM_TABLE[i + 1].freqHz;

    if (estimatedFreq >= f0 && estimatedFreq <= f1) {
      double d0 = PWM_TABLE[i].dutyPercent;
      double d1 = PWM_TABLE[i + 1].dutyPercent;
      double ratio = (estimatedFreq - f0) / (f1 - f0);
      return d0 + ratio * (d1 - d0);
    }
  }

  return PWM_TABLE[PWM_TABLE_SIZE - 1].dutyPercent; // Fallback (should not reach here)
}

// ============================================================================
// driveMotors()
// Applies the identical PWM duty value to all three vibration motors.
// ============================================================================
void driveMotors(uint8_t pwmValue) {
  ledcWrite(PIN_MOTOR_LEFT,  pwmValue);
  ledcWrite(PIN_MOTOR_RIGHT, pwmValue);
  ledcWrite(PIN_MOTOR_HEAD,  pwmValue);
}

// ============================================================================
// printResults()
// Prints a formatted debug block with raw, stable, and estimated
// frequencies, confidence, and the applied motor PWM value.
// ============================================================================
void printResults(double rawFreq, double stableFreq, double estimatedFreq,
                   double confidence, uint8_t pwmValue) {
  Serial.println(F("--------------------------------"));
  Serial.print(F("Raw FFT Frequency   : "));
  Serial.print(rawFreq, 1);
  Serial.println(F(" Hz"));

  Serial.print(F("Stable Frequency    : "));
  Serial.print(stableFreq, 1);
  Serial.println(F(" Hz"));

  Serial.print(F("Estimated Frequency : "));
  Serial.print(estimatedFreq, 1);
  Serial.println(F(" Hz"));

  Serial.print(F("Confidence          : "));
  Serial.print(confidence, 1);
  Serial.println(F(" %"));

  Serial.print(F("Motor PWM           : "));
  Serial.print(pwmValue);
  Serial.println(F(" / 255"));

  Serial.println(F("--------------------------------"));
}
