#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <SingleFileDrive.h>

#define SAMPLE_FILE "samples6.csv"
#define SAMPLE_BUFFER_SIZE 600
uint8_t activeBufferIdx = 0;
bool readyToWrite = false;
float samples[2][SAMPLE_BUFFER_SIZE][3];

#define I2C_ADDRESS 0x18
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define LED_COUNT 18
Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_GRB + NEO_KHZ800);

// these might be modified during program execution
int pixelChangeIntervalMillis = 50;  // how many millis between pixel changes
int dataSampleIntervalMillis = 100;  // how many millis between data samples

float average_acceleration = 0.0;

bool sleepMode = false;
#define MODE_COUNT 7;
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

    if (average_acceleration > 6.0) {
      pixelChangeIntervalMillis = 25;
    } else if (average_acceleration > 4.0) {
      pixelChangeIntervalMillis = 50;
    } else if (average_acceleration > 2.0) {
      pixelChangeIntervalMillis = 75;
    } else  if (average_acceleration > 1.0) {
      pixelChangeIntervalMillis = 100;
    } else {
      pixelChangeIntervalMillis = 250;
    }
  }
}

void setup1(void) {

}

void loop1() {
  if (readyToWrite) {
    writeData();
  }
  delay(10000);
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
  rainbow();
  // solid(strip.Color(127, 0, 0));

  if (mode < 4) {
    applyTheatreChase();
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
    activeBufferIdx = (activeBufferIdx + 1) % 2;
    sampleIdx = 0;
    readyToWrite = true;
  }

  updateRollingAverage(event.acceleration.x, event.acceleration.y, event.acceleration.z);
}

void updateRollingAverage(float x, float y, float z) {
  static uint8_t idx = 0;
  static float buffer[10];

  float in_magnitude = abs(sqrt(x * x + y * y + z * z) - 9.81);
  float out_magnitude = buffer[idx];
  buffer[idx] = in_magnitude;

  idx++;
  if (idx == 10) {
    idx = 0;
  }

  average_acceleration = (average_acceleration * 10.0 - out_magnitude + in_magnitude) / 10.0;
}

void writeData() {
  Serial.println("writing data to flash");
  uint8_t bufferIdx = (activeBufferIdx + 1) % 2;
  noInterrupts();
  File f = LittleFS.open(SAMPLE_FILE, "a");
  for(uint16_t idx = 0; idx < SAMPLE_BUFFER_SIZE; idx++) {
    f.printf("%f,%f,%f\n", samples[bufferIdx][idx][0], samples[bufferIdx][idx][1], samples[bufferIdx][idx][2]);
  }
  f.close();
  interrupts();
  readyToWrite = false;
}

#define THEATRE_GAP 4
void applyTheatreChase() {
  static uint8_t pixelSet = 0;
  for(uint16_t pixelIdx = 0; pixelIdx < LED_COUNT; pixelIdx++) {
    if ((pixelIdx + pixelSet) % THEATRE_GAP != 0) {
      strip.setPixelColor(pixelIdx, strip.Color(0, 0, 0));
    }
  }

  if (pixelSet == 0) {
    pixelSet = THEATRE_GAP - 1;
  } else {
    pixelSet = pixelSet - 1;
  }
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

  if (cyclePosition < RAINBOW_STEP) {
    cyclePosition = RAINBOW_STATES - RAINBOW_STEP;
  } else {
    cyclePosition = cyclePosition - RAINBOW_STEP;
  }
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
