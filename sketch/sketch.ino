#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>
#include <SingleFileDrive.h>

#define I2C_ADDRESS 0x18
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define LED_COUNT 10
Adafruit_NeoPixel strip(LED_COUNT, PIN_EXTERNAL_NEOPIXELS, NEO_GRB + NEO_KHZ800);


unsigned long previousPixelChangeTime = 0;    // when did we last change a pixel
int           pixelChangeIntervalMillis = 50; // how many millis between pixel changes

unsigned long previousDataSampleTime = 0;     // when did we last sample data
int           dataSampleIntervalMillis = 500; // how many millis between data samples

unsigned long previousDataWriteTime = 0;      // when did we last write data
int           dataWriteIntervalMillis = 500;  // how many millis between data writes


void setup() {
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
  singleFileDrive.begin("littlefsfile.csv", "Data Recorder.csv");
}

void changePixel() {

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
  if (okay-to-write) {
      noInterrupts();
      File f = LittleFS.open("littlefsfile.csv", "a");
      f.printf("%d,%d,%d\n", data1, data2, data3);
      f.close();
      interrupts();
  }
}

void loop() {
  unsigned long currentTime = millis();

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
