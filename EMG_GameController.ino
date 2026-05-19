// ============================================================
// EMG Game Controller - 3 FORCE LEVELS
// Final Year Project: Real-time Video Game Control via EMG
// Hardware: Arduino UNO R4 Minima + Muscle BioAmp Shield
//
// GESTURES (left hand only):
//   Relax              → command 0 → REST
//   Soft squeeze       → command 1 → LEFT
//   Medium squeeze     → command 2 → RIGHT
//   Hard squeeze       → command 3 → JUMP
//
// HOW IT WORKS:
//   After calibration, the system knows your resting level.
//   It then sets 3 thresholds above that:
//     Low threshold  = soft flex detected  → LEFT
//     Mid threshold  = medium flex         → RIGHT
//     High threshold = hard flex           → JUMP
//   The harder you squeeze, the bigger the envelope value.
//   We check which "zone" the envelope falls into.
// ============================================================

#define SAMPLE_RATE          500
#define BAUD_RATE            115200
#define INPUT_PIN1           A0
#define INPUT_PIN2           A2
#define CH1_LED              13
#define CH2_LED              11
#define BUTTON_PIN           4
#define STATUS_LED           8
#define BUFFER_SIZE          32
#define OUTPUT_INTERVAL_MS   20
#define CALIBRATION_SAMPLES  500
#define DEAD_ZONE            8

// ============================================================
// THRESHOLD MULTIPLIERS
// These define the 3 force zones above your resting baseline.
//
// Example: if your resting baseline = 10
//   LOW_MULT  1.8 → threshold_low  = 18  (soft flex)
//   MID_MULT  3.5 → threshold_mid  = 35  (medium flex)
//   HIGH_MULT 6.0 → threshold_high = 60  (hard flex)
//
// If gestures feel wrong, adjust these:
//   Soft flex triggering JUMP? → increase HIGH_MULT
//   Hard flex only triggering LEFT? → decrease all multipliers
// ============================================================
#define LOW_MULT   1.8    // Soft squeeze   → LEFT
#define MID_MULT   3.5    // Medium squeeze → RIGHT
#define HIGH_MULT  6.0    // Hard squeeze   → JUMP

// Circular buffers
int circular_buffer1[BUFFER_SIZE];
int data_index1 = 0, sum1 = 0;
int circular_buffer2[BUFFER_SIZE];
int data_index2 = 0, sum2 = 0;

// 3 thresholds — set automatically during calibration
int threshold_low  = 20;
int threshold_mid  = 40;
int threshold_high = 70;

// Button debounce
int ledState = LOW;
int buttonState = 0;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Timing
unsigned long lastOutputTime = 0;
int lastCommand = -1;


// ============================================================
// CALIBRATION
// Measures your resting signal for 3 seconds.
// Sets 3 thresholds at different multipliers above baseline.
// ============================================================
void calibrate() {
  Serial.println("CAL_START");
  Serial.println("RELAX your muscles for 3 seconds...");

  long baseline_sum = 0;

  for (int i = 0; i < CALIBRATION_SAMPLES * 3; i++) {
    delayMicroseconds(2000);

    int raw1 = analogRead(INPUT_PIN1);
    int sig1 = (int)EMGFilter1((float)raw1);
    int env1 = getEnvelope1(abs(sig1));
    baseline_sum += env1;

    if (i % 500 == 0) {
      digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    }
  }

  int baseline = (int)(baseline_sum / (CALIBRATION_SAMPLES * 3));

  // Set the 3 force thresholds
  threshold_low  = max((int)(baseline * LOW_MULT),  DEAD_ZONE * 2);
  threshold_mid  = max((int)(baseline * MID_MULT),  DEAD_ZONE * 4);
  threshold_high = max((int)(baseline * HIGH_MULT), DEAD_ZONE * 8);

  // Send thresholds to Python for display
  Serial.print("T_LOW:");  Serial.print(threshold_low);
  Serial.print(",T_MID:"); Serial.print(threshold_mid);
  Serial.print(",T_HIGH:"); Serial.println(threshold_high);
  Serial.println("CAL_DONE");

  // Reset buffers
  memset(circular_buffer1, 0, sizeof(circular_buffer1));
  memset(circular_buffer2, 0, sizeof(circular_buffer2));
  sum1 = 0; sum2 = 0;
  data_index1 = 0; data_index2 = 0;

  digitalWrite(STATUS_LED, HIGH);
}


// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(INPUT_PIN1, INPUT);
  pinMode(INPUT_PIN2, INPUT);
  pinMode(CH1_LED, OUTPUT);
  pinMode(CH2_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(STATUS_LED, OUTPUT);

  calibrate();
}


// ============================================================
// MAIN LOOP
// ============================================================
void loop() {

  // Button debounce
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) ledState = !ledState;
    }
  }
  lastButtonState = reading;
  digitalWrite(STATUS_LED, ledState);

  // 500Hz sampling timer
  static unsigned long past = 0;
  unsigned long present = micros();
  unsigned long interval = present - past;
  past = present;

  static long timer = 0;
  timer -= interval;

  if (timer < 0) {
    timer += 1000000 / SAMPLE_RATE;

    // Read and filter Channel 1 (left hand)
    int sensor_value1 = analogRead(INPUT_PIN1);
    int signal1       = (int)EMGFilter1((float)sensor_value1);
    int envelope1     = getEnvelope1(abs(signal1));

    // LED brightness shows force level
    digitalWrite(CH1_LED, envelope1 > threshold_low ? HIGH : LOW);

    // Send command at 50Hz
    unsigned long now = millis();
    if (now - lastOutputTime >= OUTPUT_INTERVAL_MS) {
      lastOutputTime = now;

      // --------------------------------------------------------
      // 3-LEVEL FORCE GESTURE CLASSIFICATION
      //
      // We check from HIGH to LOW so the highest zone wins.
      // envelope1 is your current muscle signal strength.
      //
      //   envelope >= threshold_high  → HARD flex   → command 3 (JUMP)
      //   envelope >= threshold_mid   → MEDIUM flex → command 2 (RIGHT)
      //   envelope >= threshold_low   → SOFT flex   → command 1 (LEFT)
      //   below all thresholds        → REST        → command 0
      // --------------------------------------------------------
      int command = 0;

      if (envelope1 >= threshold_high) {
        command = 3;   // HARD squeeze → JUMP
      } else if (envelope1 >= threshold_mid) {
        command = 2;   // MEDIUM squeeze → RIGHT
      } else if (envelope1 >= threshold_low) {
        command = 1;   // SOFT squeeze → LEFT
      } else {
        command = 0;   // REST
      }

      // Only send when command changes
      if (command != lastCommand) {
        Serial.println(command);
        lastCommand = command;
      }
    }
  }
}


// ============================================================
// ENVELOPE DETECTION
// ============================================================
int getEnvelope1(int abs_emg) {
  sum1 -= circular_buffer1[data_index1];
  sum1 += abs_emg;
  circular_buffer1[data_index1] = abs_emg;
  data_index1 = (data_index1 + 1) % BUFFER_SIZE;
  return (sum1 / BUFFER_SIZE) * 2;
}

int getEnvelope2(int abs_emg) {
  sum2 -= circular_buffer2[data_index2];
  sum2 += abs_emg;
  circular_buffer2[data_index2] = abs_emg;
  data_index2 = (data_index2 + 1) % BUFFER_SIZE;
  return (sum2 / BUFFER_SIZE) * 2;
}


// ============================================================
// BAND-PASS BUTTERWORTH FILTER (74.5 - 149.5 Hz, Order 4)
// ============================================================
float EMGFilter1(float input) {
  float output = input;
  { static float z1, z2;
    float x = output - 0.05159732f*z1 - 0.36347401f*z2;
    output = 0.01856301f*x + 0.03712602f*z1 + 0.01856301f*z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - (-0.53945795f)*z1 - 0.39764934f*z2;
    output = x - 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - 0.47319594f*z1 - 0.70744137f*z2;
    output = x + 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - (-1.00211112f)*z1 - 0.74520226f*z2;
    output = x - 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  return output;
}

float EMGFilter2(float input2) {
  float output = input2;
  { static float z1, z2;
    float x = output - 0.05159732f*z1 - 0.36347401f*z2;
    output = 0.01856301f*x + 0.03712602f*z1 + 0.01856301f*z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - (-0.53945795f)*z1 - 0.39764934f*z2;
    output = x - 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - 0.47319594f*z1 - 0.70744137f*z2;
    output = x + 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  { static float z1, z2;
    float x = output - (-1.00211112f)*z1 - 0.74520226f*z2;
    output = x - 2.0f*z1 + z2;
    z2 = z1; z1 = x; }
  return output;
}
