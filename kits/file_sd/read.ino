#include <SPI.h>
#include <SD.h>

#define SD_CS 5  // Chip Select Pin for SD card

void setup() {
  Serial.begin(115200);
  
  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");
  
  // Open the WAV file on the SD card
  File audioFile = SD.open("/audio.wav");
  if (!audioFile) {
    Serial.println("Failed to open audio file!");
    return;
  }

  // Read and print the first few bytes of the file
  byte buffer[32];
  int bytesRead = audioFile.read(buffer, sizeof(buffer));
  Serial.print("Bytes read: ");
  Serial.println(bytesRead);
  
  // Print first few bytes
  for (int i = 0; i < bytesRead; i++) {
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  audioFile.close();
}

void loop() {
  // Nothing to do here
}
