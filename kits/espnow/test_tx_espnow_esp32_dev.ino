#include <esp_now.h>
#include <WiFi.h>

// MAC của RX (ESP32-C3)
uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// Callback khi gửi dữ liệu
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("TX START");

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP NOW INIT FAIL");
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
  char msg[] = "HELLO ESP32 C3";

  // Gửi dữ liệu
  esp_now_send(receiverMAC, (uint8_t *)msg, sizeof(msg));

  Serial.println("SEND");

  delay(1000);
}