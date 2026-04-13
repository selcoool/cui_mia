#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define CHANNELS 8

typedef struct {
  uint16_t ch[CHANNELS];
} PPMData;

PPMData receivedData;

// ===== callback nhận =====
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  for (int i = 0; i < CHANNELS; i++) {
  Serial.print("CH");
  Serial.print(i + 1);
  Serial.print(":");
  Serial.print(receivedData.ch[i]);
  Serial.print(" ");
}
Serial.println();
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("RX READY");

  // 👉 In MAC để kiểm tra
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}
