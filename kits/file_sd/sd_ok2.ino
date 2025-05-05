#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

#define SD_CS 5
#define I2S_BCLK 26
#define I2S_LRC 25
#define I2S_DIN 22

AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("🔌 Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card initialization failed!");
    while (true);
  }
  Serial.println("✅ SD card initialized.");

  if (!SD.exists("/audio.wav")) {
    Serial.println("❌ File /audio.wav not found!");
    while (true);
  }

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  out->SetGain(0.8);

  file = new AudioFileSourceSD("/audio.wav");
  wav = new AudioGeneratorWAV();
  if (!wav->begin(file, out)) {
    Serial.println("❌ Failed to start WAV playback. Invalid file?");
    while (true);
  }

  Serial.println("✅ Playback started!");
}

void loop() {
  if (wav->isRunning()) {
    wav->loop();
  } else {
    Serial.println("🎵 Playback finished.");
    delay(1000);
  }
}
