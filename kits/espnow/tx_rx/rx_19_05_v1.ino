#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"

typedef struct {
  uint16_t ch[4];
} Data;

volatile Data rxData;

uint32_t lastRx = 0;
uint8_t lostCount = 0;
bool lost = false;

#define FAILSAFE_MS 180

// ===== CALLBACK =====
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len)
#endif
{
  if (len != sizeof(Data)) return;

  memcpy((void*)&rxData, data, sizeof(Data));
  lastRx = millis();
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);

  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.println("================================");
  Serial.println("      ESP-NOW RX READY");
  Serial.println("================================");
}

void loop() {

  // ===== FAILSAFE =====
  if (millis() - lastRx > FAILSAFE_MS) {
    if (lostCount < 10) lostCount++;
  } else {
    lostCount = 0;
  }

  lost = (lostCount >= 3);

  uint16_t out[4];

  if (lost) {
    out[0] = out[1] = out[2] = out[3] = 1500;
  } else {
    for (int i = 0; i < 4; i++) {
      out[i] = rxData.ch[i];
    }
  }

  // ===== PRINT TERMINAL (10Hz) =====
  static uint32_t lastPrint = 0;

  if (millis() - lastPrint > 100) {
    lastPrint = millis();

    Serial.print("[ESP-NOW] ");

    Serial.print("CH0: "); Serial.print(out[0]);
    Serial.print(" | CH1: "); Serial.print(out[1]);
    Serial.print(" | CH2: "); Serial.print(out[2]);
    Serial.print(" | CH3: "); Serial.print(out[3]);

    Serial.print(" | STATUS: ");
    Serial.print(lost ? "LOST" : "OK");

    Serial.print(" | RSS(ms): ");
    Serial.println(millis() - lastRx);
  }
}