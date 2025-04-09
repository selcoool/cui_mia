#include <WiFi.h>

const char *ssid = "ESP32_AP";  // Tên Wi-Fi của bạn
const char *password = "123456789";  // Mật khẩu Wi-Fi của bạn

void setup() {
  Serial.begin(115200);

  // Bắt đầu làm Access Point (AP)
  WiFi.softAP(ssid, password, 3, false, 1);  // channel 3, không bảo mật, chỉ 1 kết nối

  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Mạng Wi-Fi đã tạo và có thể kết nối từ các thiết bị khác
}