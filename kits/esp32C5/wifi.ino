#include "WiFi.h"

// SSID và mật khẩu AP
const char* ssid = "ESP32C5_5GHz";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Chế độ AP
  WiFi.mode(WIFI_MODE_AP);

  // Phát AP 5GHz (channel 36)
  if (WiFi.softAP(ssid, password, 36)) {
    Serial.print("AP 5GHz đang chạy với SSID: ");
    Serial.println(ssid);
  } else {
    Serial.println("❌ Không thể khởi tạo AP");
  }

  // In MAC tự động của AP
  Serial.print("MAC hiện tại: ");
  Serial.println(WiFi.softAPmacAddress());
}

void loop() {
  // Không làm gì
}
