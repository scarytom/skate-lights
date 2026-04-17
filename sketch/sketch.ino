#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <SingleFileDrive.h>

#define SAMPLE_FILE "samples6.csv"
#define SAMPLE_BUFFER_SIZE 600
uint8_t activeBufferIdx = 0;
float samples[2][SAMPLE_BUFFER_SIZE][3];

#define I2C_ADDRESS 0x18
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define LED_COUNT 18
Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_GRB + NEO_KHZ800);

// these might be modified during program execution
int pixelChangeIntervalMillis = 50;  // how many millis between pixel changes
int dataSampleIntervalMillis = 100;  // how many millis between data samples

float average_acceleration = 0.0;
float previous_magnitude = 9.81;
float jerk = 0.0;
uint8_t flash_countdown = 0;  // >0 means we're in a flash effect
#define FLASH_DURATION 6       // frames of flash (~300ms at 50ms interval)
#define JERK_THRESHOLD 8.0     // p99 is ~10-11, so 8 catches top ~2% of events

// Tilt tracking (per-axis)
float tilt_x = 0.0;  // smoothed X axis, range roughly ±20
float tilt_y = 0.0;  // smoothed Y axis

// Motion state enum — thresholds from data analysis
// Rolling avg: still<0.5 (38-68%), cruise 0.5-2.0 (25-49%), active 2.0-3.5 (7-13%), intense>3.5 (~0.1%)
#define STATE_STILL   0
#define STATE_CRUISE  1
#define STATE_ACTIVE  2
#define STATE_INTENSE 3
uint8_t motion_state = STATE_STILL;

bool sleepMode = false;
#define MODE_COUNT 7
uint8_t mode = 0;

void setup() {
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);

  strip.begin();

  Serial.begin(115200);
  // while (!Serial) delay(10);

  Serial.println("start");

  if (!lis.begin(I2C_ADDRESS)) {
    Serial.println("Couldnt find accelerometer");
    while (1) yield();
  }
  Serial.println("accelerometer found!");
  
  // lis.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8, or 16
  // lis.setPerformanceMode(LIS3DH_MODE_NORMAL); // LOW_POWER, NORMAL, or HIGH_RESOLUTION
  // lis.setDataRate(LIS3DH_DATARATE_50_HZ); // 1, 20, 25, 50, 100, 200, 400, POWERDOWN, LOWPOWER_5HZ, LOWPOWER_1K6HZ

  LittleFS.begin();

  applyMode();
}

void loop() {
  static bool buttonStateHandled = false;
  static PinStatus previousButtonState = HIGH;
  static unsigned long previousButtonChangeTime = 0; // when did the button last change state
  static unsigned long previousPixelChangeTime  = 0; // when did we last change a pixel
  static unsigned long previousDataSampleTime   = 0; // when did we last sample data

  unsigned long currentTime = millis();

  PinStatus buttonState = digitalRead(PIN_BUTTON);
  if (buttonState != previousButtonState) {
    previousButtonState = buttonState;
    previousButtonChangeTime = currentTime;
    buttonStateHandled = false;
  }

  if (buttonState == LOW && !buttonStateHandled && currentTime - previousButtonChangeTime > 50) {
    buttonStateHandled = true;
    mode = (mode + 1) % MODE_COUNT;
    applyMode();
  }

  if (mode == 0) return;

  if (currentTime - previousPixelChangeTime > pixelChangeIntervalMillis) {
    changePixel();
    previousPixelChangeTime = currentTime;
  }

  if (currentTime - previousDataSampleTime > dataSampleIntervalMillis) {
    sampleData();
    previousDataSampleTime = currentTime;

    // Data-driven thresholds (from analysis of 142k samples):
    // rolling avg p50≈0.5-1.8, p90≈2.5-3.2, p95≈3.3-3.6, max≈7.5
    if (average_acceleration > 3.5) {
      motion_state = STATE_INTENSE;
      pixelChangeIntervalMillis = 20;
    } else if (average_acceleration > 2.0) {
      motion_state = STATE_ACTIVE;
      pixelChangeIntervalMillis = 40;
    } else if (average_acceleration > 0.5) {
      motion_state = STATE_CRUISE;
      pixelChangeIntervalMillis = 80;
    } else {
      motion_state = STATE_STILL;
      pixelChangeIntervalMillis = 200;
    }
  }
}

void setup1(void) {

}

void loop1() {
  uint32_t bufferIdx = rp2040.fifo.pop();
//   writeData(bufferIdx);
}

void applyMode() {
  Serial.printf("entering mode %d\n", mode);
  if (mode == 0) {
    digitalWrite(PIN_EXTERNAL_POWER, LOW);
    strip.setBrightness(0);
  // lis.setPerformanceMode(LOW_POWER);
  // lis.setDataRate(POWERDOWN);
    // allow USB in sleep mode
    singleFileDrive.begin(SAMPLE_FILE, "data.csv");
  }
  if (mode == 1) {
    strip.setBrightness(64);
    digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  // lis.setPerformanceMode(LIS3DH_MODE_NORMAL);
  // lis.setDataRate(LIS3DH_DATARATE_50_HZ);
    singleFileDrive.end();
  }
  if (mode == 2) {
    strip.setBrightness(128);
  }
  if (mode == 3) {
    strip.setBrightness(255);
  }
  if (mode == 4) {
    strip.setBrightness(64);
  }
  if (mode == 5) {
    strip.setBrightness(128);
  }
  if (mode == 6) {
    strip.setBrightness(255);
  }
}

void changePixel() {
  // Impact flash overrides everything — white flash on big jerk events
  if (flash_countdown > 0) {
    impactFlash();
    flash_countdown--;
    strip.show();
    return;
  }

  if (mode >= 1 && mode <= 3) {
    // Modes 1-3: tilt-reactive rainbow + theatre chase
    tiltRainbow();
    applyTheatreChase();
  } else {
    // Modes 4-6: motion-reactive pulse
    motionPulse();
  }
  strip.show();
}

void sampleData() {
  static uint16_t sampleIdx = 0;

  sensors_event_t event;
  lis.getEvent(&event);
  samples[activeBufferIdx][sampleIdx][0] = event.acceleration.x;
  samples[activeBufferIdx][sampleIdx][1] = event.acceleration.y;
  samples[activeBufferIdx][sampleIdx][2] = event.acceleration.z;

  sampleIdx++;

  if (sampleIdx == SAMPLE_BUFFER_SIZE) {
    Serial.println("switching sample buffer");
    uint8_t fullBufferIdx = activeBufferIdx;
    activeBufferIdx = (activeBufferIdx + 1) % 2;
    sampleIdx = 0;
    rp2040.fifo.push(fullBufferIdx);
  }

  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  updateRollingAverage(x, y, z);

  // Jerk detection — sudden magnitude change triggers flash
  float mag = sqrt(x * x + y * y + z * z);
  jerk = abs(mag - previous_magnitude);
  previous_magnitude = mag;
  if (jerk > JERK_THRESHOLD) {
    flash_countdown = FLASH_DURATION;
  }

  // Smooth tilt tracking (exponential moving average, alpha=0.3)
  tilt_x = tilt_x * 0.7 + x * 0.3;
  tilt_y = tilt_y * 0.7 + y * 0.3;
}

void updateRollingAverage(float x, float y, float z) {
  static uint8_t idx = 0;
  static float buffer[10] = {0};

  float in_magnitude = abs(sqrt(x * x + y * y + z * z) - 9.81);
  float out_magnitude = buffer[idx];
  buffer[idx] = in_magnitude;

  idx++;
  if (idx == 10) {
    idx = 0;
  }

  average_acceleration = (average_acceleration * 10.0 - out_magnitude + in_magnitude) / 10.0;
}

void writeData(uint8_t bufferIdx) {
  Serial.println("writing data to flash");
  noInterrupts();
  File f = LittleFS.open(SAMPLE_FILE, "a");
  for(uint16_t idx = 0; idx < SAMPLE_BUFFER_SIZE; idx++) {
    f.printf("%f,%f,%f\n", samples[bufferIdx][idx][0], samples[bufferIdx][idx][1], samples[bufferIdx][idx][2]);
  }
  f.close();
  interrupts();
}

// ============================================================
// Light pattern functions
// ============================================================

// --- Impact flash: bright white burst that fades out ---
void impactFlash() {
  // Intensity fades over FLASH_DURATION frames
  uint8_t brightness = (uint8_t)(255 * flash_countdown / FLASH_DURATION);
  uint32_t color = strip.Color(brightness, brightness, brightness);
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

// --- Motion-reactive pulse: color and speed driven by acceleration ---
void motionPulse() {
  static uint8_t pulsePhase = 0;

  // Pulse brightness oscillates; speed depends on motion state
  uint8_t pulseStep = 3 + motion_state * 5;  // still=3, cruise=8, active=13, intense=18
  pulsePhase += pulseStep;
  // Triangle wave: 0→255→0
  uint8_t brightness = pulsePhase < 128 ? pulsePhase * 2 : (255 - pulsePhase) * 2;

  // Color shifts with motion state
  uint32_t color;
  switch (motion_state) {
    case STATE_STILL:
      // Calm blue breathing
      color = strip.Color(0, 0, brightness);
      break;
    case STATE_CRUISE:
      // Teal/cyan
      color = strip.Color(0, brightness, brightness / 2);
      break;
    case STATE_ACTIVE:
      // Green-yellow energy
      color = strip.Color(brightness / 2, brightness, 0);
      break;
    case STATE_INTENSE:
      // Hot red-orange
      color = strip.Color(brightness, brightness / 3, 0);
      break;
    default:
      color = strip.Color(0, 0, brightness);
  }

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

#define THEATRE_GAP 4
void applyTheatreChase() {
  static uint8_t pixelSet = 0;
  for(uint16_t pixelIdx = 0; pixelIdx < LED_COUNT; pixelIdx++) {
    if ((pixelIdx + pixelSet) % THEATRE_GAP != 0) {
      strip.setPixelColor(pixelIdx, strip.Color(0, 0, 0));
    }
  }

  pixelSet = (pixelSet + 1) % THEATRE_GAP;
}

void solid(uint32_t colour) {
  for(uint16_t pixelIdx = 0; pixelIdx < LED_COUNT; pixelIdx++) {
    strip.setPixelColor(pixelIdx, colour);
  }
}

#define RAINBOW_STATES 765 // 255 states * 3 colours
#define RAINBOW_STEP 5
void rainbow() {
  static uint16_t cyclePosition = 0;

  for(uint16_t pixelIdx = 0; pixelIdx < LED_COUNT; pixelIdx++) {
    strip.setPixelColor(pixelIdx, calculateRainbowColour(pixelIdx * RAINBOW_STEP + cyclePosition));
  }

  cyclePosition = (cyclePosition + RAINBOW_STEP) % RAINBOW_STATES;
}

// --- Tilt-reactive rainbow: board tilt shifts the hue offset ---
void tiltRainbow() {
  static uint16_t cyclePosition = 0;

  // Map tilt_x (-20..+20) to a hue offset (0..RAINBOW_STATES)
  // This makes the rainbow "slide" along the strip as you lean
  int16_t tiltOffset = (int16_t)(tilt_x * 19);  // ~±380 range from ±20

  // Speed scales with motion state
  uint16_t step = 2 + motion_state * 3;  // still=2, cruise=5, active=8, intense=11

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint16_t pos = (i * 5 + cyclePosition + tiltOffset + RAINBOW_STATES) % RAINBOW_STATES;
    strip.setPixelColor(i, calculateRainbowColour(pos));
  }

  cyclePosition = (cyclePosition + step) % RAINBOW_STATES;
}

uint32_t calculateRainbowColour(uint16_t position) {
  uint8_t phase = (position % RAINBOW_STATES) / 255; // 0=RG; 1=GB; 2=RB
  uint8_t increasingComponent = position % 255;
  uint8_t decreasingComponent = 255 - increasingComponent;

  if (phase == 0) {
    return strip.Color(decreasingComponent, increasingComponent, 0);
  }
  if (phase == 1) {
    return strip.Color(0, decreasingComponent, increasingComponent);
  }
  return strip.Color(increasingComponent, 0, decreasingComponent);
}
