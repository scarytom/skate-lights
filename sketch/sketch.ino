#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <SingleFileDrive.h>

#define I2C_ADDRESS 0x18
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define LED_COUNT 10
Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_GRB + NEO_KHZ800);

// these might be modified during program execution
int pixelChangeIntervalMillis = 50;  // how many millis between pixel changes
int dataSampleIntervalMillis = 500;  // how many millis between data samples
int dataWriteIntervalMillis = 10000; // how many millis between data writes

bool sleepMode = true;

void setup() {
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);

  strip.begin();
  strip.setBrightness(50);

  Serial.begin(115200);
  while (!Serial) delay(10);

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
}

void loop() {
  static bool buttonStateHandled = false;
  static PinStatus previousButtonState = HIGH;
  static unsigned long previousButtonChangeTime = 0; // when did the button last change state
  static unsigned long previousPixelChangeTime  = 0; // when did we last change a pixel
  static unsigned long previousDataSampleTime   = 0; // when did we last sample data
  static unsigned long previousDataWriteTime    = 0; // when did we last write data

  unsigned long currentTime = millis();

  PinStatus buttonState = digitalRead(PIN_BUTTON);
  if (buttonState != previousButtonState) {
    previousButtonState = buttonState;
    previousButtonChangeTime = currentTime;
    buttonStateHandled = false;
  }

  if (buttonState == LOW && !buttonStateHandled && currentTime - previousButtonChangeTime > 50) {
    buttonStateHandled = true;
    handleButtonPress();
  }

  if (sleepMode) return;

  if (currentTime - previousPixelChangeTime > pixelChangeIntervalMillis) {
    changePixel();
    previousPixelChangeTime = currentTime;
  }

  if (currentTime - previousDataSampleTime > dataSampleIntervalMillis) {
    sampleData();
    previousDataSampleTime = currentTime;
  }

  if (currentTime - previousDataWriteTime > dataWriteIntervalMillis) {
    writeData();
    previousDataWriteTime = currentTime;
  }
}

void handleButtonPress() {
  sleepMode = !sleepMode;
  if (sleepMode) {
    digitalWrite(PIN_EXTERNAL_POWER, LOW);

    // allow USB in sleep mode
    singleFileDrive.begin("littlefsfile.csv", "Data Recorder.csv");
  } else {
    digitalWrite(PIN_EXTERNAL_POWER, HIGH);
    singleFileDrive.end();
  }
}

void changePixel() {
  pixelChangeIntervalMillis = 50;
  rainbow();
  // solid(strip.Color(127, 0, 0));
  applyTheatreChase();

  strip.show();
}

void sampleData() {
  sensors_event_t event;
  lis.getEvent(&event);
  Serial.print("\t\tX: "); Serial.print(event.acceleration.x);
  Serial.print(" \tY: "); Serial.print(event.acceleration.y);
  Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
  Serial.println(" m/s^2 ");
}

void writeData() {
  bool okayToWrite = true;
  if (okayToWrite) {
      noInterrupts();
      File f = LittleFS.open("littlefsfile.csv", "a");
      f.printf("%d,%d,%d\n", 0.1, 0.2, 0.3);
      f.close();
      interrupts();
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
