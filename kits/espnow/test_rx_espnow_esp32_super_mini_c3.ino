#include <esp_now.h>
#include <WiFi.h>

// Hàm callback khi nhận dữ liệu
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  Serial.print("Received bytes: ");
  Serial.println(len);

  Serial.print("Data: ");
  for(int i=0;i<len;i++){
    Serial.print((char)incomingData[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(3000); // Cho Serial Monitor kịp mở

  Serial.println("BOOT OK");

  // Bật chế độ WiFi STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  // Khởi tạo ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP NOW INIT FAIL");
    return;
  }

  Serial.println("ESP-NOW RX READY");

  // Đăng ký callback nhận dữ liệu
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // RX không cần loop gì cả
}