
VCC	3.3V
GND	GND
MISO	GPIO 19
MOSI	GPIO 23
SCK	GPIO 18
CS (Chip Select)	GPIO 5


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

  // List the first 10 files on the SD card
  Serial.println("Listing files on SD card:");

  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory!");
    return;
  }

  root.rewindDirectory();
  int fileCount = 0;
  File entry = root.openNextFile();
  
  while (entry && fileCount < 10) {
    Serial.print("File name: ");
    Serial.println(entry.name());
    entry = root.openNextFile();
    fileCount++;
  }

  if (fileCount == 0) {
    Serial.println("No files found on the SD card.");
  }
}

void loop() {
  // Nothing to do in the loop
}
