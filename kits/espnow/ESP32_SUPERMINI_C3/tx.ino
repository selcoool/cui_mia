#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define PPM_PIN 34
#define CHANNELS 8

// 👉 MAC của ESP32 RX (Super Mini)
uint8_t receiverMAC[] = {0x18, 0x8B, 0x0E, 0x92, 0x66, 0x34};

// ===== PPM =====
volatile uint16_t channels[CHANNELS];
volatile uint8_t channelIndex = 0;
volatile uint32_t lastRise = 0;

// struct gửi
typedef struct {
  uint16_t ch[CHANNELS];
} PPMData;

PPMData dataToSend;

// ===== ISR đọc PPM =====
void IRAM_ATTR ppmISR() {
  uint32_t now = micros();
  uint32_t pulse = now - lastRise;
  lastRise = now;

  if (pulse > 3000) {
    channelIndex = 0; // sync
  } else {
    if (channelIndex < CHANNELS) {
      if (pulse > 800 && pulse < 2200) {
        channels[channelIndex] = pulse;
      }
      channelIndex++;
    }
  }
}

// ===== callback gửi =====
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(PPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmISR, RISING);

  WiFi.mode(WIFI_STA);

  // fix channel (tránh lỗi ESP-NOW)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Add peer failed");
    return;
  }

  Serial.println("TX READY");
}

void loop() {
  // copy từ ISR
  noInterrupts();
  for (int i = 0; i < CHANNELS; i++) {
    dataToSend.ch[i] = channels[i];
  }
  uint8_t idx = channelIndex;
  interrupts();

  // ===== DEBUG =====
  Serial.print("TX: ");
  for (int i = 0; i < CHANNELS; i++) {
    Serial.print(dataToSend.ch[i]);
    Serial.print(" ");
  }
  Serial.print("| IDX: ");
  Serial.println(idx);

  // ===== GỬI =====
  esp_now_send(receiverMAC, (uint8_t *)&dataToSend, sizeof(dataToSend));

  delay(50); // ~20Hz
}