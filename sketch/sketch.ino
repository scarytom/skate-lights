#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <SingleFileDrive.h>

#define SAMPLE_FILE "samples6.csv"
#define SAMPLE_BUFFER_SIZE 600
uint8_t active_buffer_idx = 0;
float samples[2][SAMPLE_BUFFER_SIZE][3];

#define I2C_ADDRESS 0x18
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define LED_COUNT 18
Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_GRB + NEO_KHZ800);

#define DATA_SAMPLE_INTERVAL_MILLIS 100

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
const int pixel_interval_by_state[] = {200, 80, 40, 20};

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
  static bool button_state_handled = false;
  static PinStatus previous_button_state = HIGH;
  static unsigned long previous_button_change_time = 0; // when did the button last change state
  static unsigned long previous_pixel_change_time  = 0; // when did we last change a pixel
  static unsigned long previous_data_sample_time   = 0; // when did we last sample data

  unsigned long current_time = millis();

  PinStatus button_state = digitalRead(PIN_BUTTON);
  if (button_state != previous_button_state) {
    previous_button_state = button_state;
    previous_button_change_time = current_time;
    button_state_handled = false;
  }

  if (button_state == LOW && !button_state_handled && current_time - previous_button_change_time > 50) {
    button_state_handled = true;
    mode = (mode + 1) % MODE_COUNT;
    applyMode();
  }

  if (mode == 0) return;

  if (current_time - previous_pixel_change_time > pixel_interval_by_state[motion_state]) {
    changePixel();
    previous_pixel_change_time = current_time;
  }

  if (current_time - previous_data_sample_time > DATA_SAMPLE_INTERVAL_MILLIS) {
    sampleData();
    previous_data_sample_time = current_time;

    // Data-driven thresholds (from analysis of 142k samples):
    // rolling avg p50≈0.5-1.8, p90≈2.5-3.2, p95≈3.3-3.6, max≈7.5
    if (average_acceleration > 3.5) {
      motion_state = STATE_INTENSE;
    } else if (average_acceleration > 2.0) {
      motion_state = STATE_ACTIVE;
    } else if (average_acceleration > 0.5) {
      motion_state = STATE_CRUISE;
    } else {
      motion_state = STATE_STILL;
    }
  }
}

void setup1(void) {

}

void loop1() {
  uint32_t buffer_idx = rp2040.fifo.pop();
//   writeData(buffer_idx);
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
  static uint16_t sample_idx = 0;

  sensors_event_t event;
  lis.getEvent(&event);
  samples[active_buffer_idx][sample_idx][0] = event.acceleration.x;
  samples[active_buffer_idx][sample_idx][1] = event.acceleration.y;
  samples[active_buffer_idx][sample_idx][2] = event.acceleration.z;

  sample_idx++;

  if (sample_idx == SAMPLE_BUFFER_SIZE) {
    Serial.println("switching sample buffer");
    uint8_t full_buffer_idx = active_buffer_idx;
    active_buffer_idx = (active_buffer_idx + 1) % 2;
    sample_idx = 0;
    rp2040.fifo.push(full_buffer_idx);
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

void writeData(uint8_t buffer_idx) {
  Serial.println("writing data to flash");
  noInterrupts();
  File f = LittleFS.open(SAMPLE_FILE, "a");
  for(uint16_t idx = 0; idx < SAMPLE_BUFFER_SIZE; idx++) {
    f.printf("%f,%f,%f\n", samples[buffer_idx][idx][0], samples[buffer_idx][idx][1], samples[buffer_idx][idx][2]);
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
  static uint8_t pulse_phase = 0;

  // Pulse brightness oscillates; speed depends on motion state
  uint8_t pulse_step = 3 + motion_state * 5;  // still=3, cruise=8, active=13, intense=18
  pulse_phase += pulse_step;
  // Triangle wave: 0→255→0
  uint8_t brightness = pulse_phase < 128 ? pulse_phase * 2 : (255 - pulse_phase) * 2;

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
  static uint8_t pixel_set = 0;
  for(uint16_t pixel_idx = 0; pixel_idx < LED_COUNT; pixel_idx++) {
    if ((pixel_idx + pixel_set) % THEATRE_GAP != 0) {
      strip.setPixelColor(pixel_idx, strip.Color(0, 0, 0));
    }
  }

  pixel_set = (pixel_set + 1) % THEATRE_GAP;
}

void solid(uint32_t colour) {
  for(uint16_t pixel_idx = 0; pixel_idx < LED_COUNT; pixel_idx++) {
    strip.setPixelColor(pixel_idx, colour);
  }
}

#define RAINBOW_STATES 765 // 255 states * 3 colours
#define RAINBOW_STEP 5
void rainbow() {
  static uint16_t cycle_position = 0;

  for(uint16_t pixel_idx = 0; pixel_idx < LED_COUNT; pixel_idx++) {
    strip.setPixelColor(pixel_idx, calculateRainbowColour(pixel_idx * RAINBOW_STEP + cycle_position));
  }

  cycle_position = (cycle_position + RAINBOW_STEP) % RAINBOW_STATES;
}

// --- Tilt-reactive rainbow: board tilt shifts the hue offset ---
void tiltRainbow() {
  static uint16_t cycle_position = 0;

  // Map tilt_x (-20..+20) to a hue offset (0..RAINBOW_STATES)
  // This makes the rainbow "slide" along the strip as you lean
  int16_t tilt_offset = (int16_t)(tilt_x * 19);  // ~±380 range from ±20

  // Speed scales with motion state
  uint16_t step = 2 + motion_state * 3;  // still=2, cruise=5, active=8, intense=11

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    uint16_t pos = (i * 5 + cycle_position + tilt_offset + RAINBOW_STATES) % RAINBOW_STATES;
    strip.setPixelColor(i, calculateRainbowColour(pos));
  }

  cycle_position = (cycle_position + step) % RAINBOW_STATES;
}

uint32_t calculateRainbowColour(uint16_t position) {
  uint8_t phase = (position % RAINBOW_STATES) / 255; // 0=RG; 1=GB; 2=RB
  uint8_t increasing_component = position % 255;
  uint8_t decreasing_component = 255 - increasing_component;

  if (phase == 0) {
    return strip.Color(decreasing_component, increasing_component, 0);
  }
  if (phase == 1) {
    return strip.Color(0, decreasing_component, increasing_component);
  }
  return strip.Color(increasing_component, 0, decreasing_component);
}
