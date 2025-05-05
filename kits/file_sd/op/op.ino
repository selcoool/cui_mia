#include <SPI.h>
#include <SD.h>

#define SD_CS 5  // Chip select for SD card

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");
  
  // Try to open the root directory
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory.");
    return;
  }
  
  // List files
  while (true) {
    File file = root.openNextFile();
    if (!file) {
      break;  // No more files
    }
    Serial.print("File: ");
    Serial.print(file.name());
    Serial.print(" Size: ");
    Serial.println(file.size());
    file.close();
  }
  
  root.close();
}

void loop() {
  // Empty loop, just testing SD initialization
}
