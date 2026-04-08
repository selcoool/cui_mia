#include <Arduino.h>

#define PPM_PIN 34       // GPIO nhận tín hiệu PPM
#define CHANNELS 8       // Số kênh PPM

volatile uint16_t channels[CHANNELS];
volatile uint8_t channelIndex = 0;
volatile uint32_t lastRise = 0;

void IRAM_ATTR ppmISR() {
  uint32_t now = micros();
  uint32_t pulse = now - lastRise;
  lastRise = now;

  if (pulse > 3000) {  // sync pulse, reset kênh
    channelIndex = 0;
  } else {
    if (channelIndex < CHANNELS) {
      channels[channelIndex] = pulse; // lưu giá trị microseconds
      channelIndex++;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmISR, RISING);
}

void loop() {
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 100) { // in ra 10 lần/giây
    lastPrint = millis();
    Serial.print("Channels: ");
    for (uint8_t i = 0; i < CHANNELS; i++) {
      Serial.print(channels[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
}