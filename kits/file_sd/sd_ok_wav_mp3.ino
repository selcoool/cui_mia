#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

#define SD_CS 5        // Chip select cho thẻ SD
#define I2S_BCLK 26    // Bit clock cho MAX98357A
#define I2S_LRC  25    // Left/right clock
#define I2S_DIN  22    // Data input (DIN)

// Audio output & file source
AudioOutputI2S *out;
AudioFileSourceSD *file;
AudioGeneratorMP3 *mp3;
AudioGeneratorWAV *wav;

// Tên file cần phát (đặt đúng tên file trong thẻ nhớ)
const char *filename = "/audio.wav"; // Hoặc "/audio.wav"

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Khởi tạo thẻ SD
  Serial.println("🔌 Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card init failed!");
    while (true);
  }
  Serial.println("✅ SD card ready.");

  // Kiểm tra file có tồn tại không
  if (!SD.exists(filename)) {
    Serial.printf("❌ File %s not found!\n", filename);
    while (true);
  }

  // I2S output cho MAX98357A
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  out->SetGain(0.8);

  file = new AudioFileSourceSD(filename);

  // Phát theo định dạng
  if (strstr(filename, ".mp3")) {
    mp3 = new AudioGeneratorMP3();
    if (mp3->begin(file, out)) {
      Serial.println("🎵 MP3 playback started!");
    } else {
      Serial.println("❌ MP3 begin failed.");
    }
  } else if (strstr(filename, ".wav")) {
    wav = new AudioGeneratorWAV();
    if (wav->begin(file, out)) {
      Serial.println("🎵 WAV playback started!");
    } else {
      Serial.println("❌ WAV begin failed.");
    }
  } else {
    Serial.println("❌ Unsupported file type!");
  }
}

void loop() {
  if (mp3 && mp3->isRunning()) {
    mp3->loop();
  } else if (wav && wav->isRunning()) {
    wav->loop();
  } else {
    Serial.println("✅ Playback finished.");
    delay(2000);
    // Optional: restart playback
    // file->close(); file = new AudioFileSourceSD(filename);
    // mp3->begin(file, out); // hoặc wav->begin(...) tuỳ loại
  }
}
