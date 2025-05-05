#include <SPI.h>
#include <SD.h>

#define SD_CS 5  // Chip Select pin for SD card

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor
  }

  Serial.println("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("Initialization failed!");
    return;
  }
  Serial.println("Initialization successful!");

  // Create or open a file on the SD card
  File dataFile = SD.open("/test.txt", FILE_WRITE);

  if (dataFile) {
    Serial.println("Writing to file...");
    dataFile.println("Hello, ESP32 and SD card!");
    dataFile.close();  // Close the file
    Serial.println("Done writing to file.");
  } else {
    Serial.println("Error opening file!");
  }
}

void loop() {
  // Nothing to do in the loop
}
