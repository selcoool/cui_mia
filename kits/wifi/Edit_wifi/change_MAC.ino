#include <WiFi.h>
#include "esp_wifi.h"

void setup() {
  Serial.begin(115200);

  // Đặt một địa chỉ MAC giả cho WiFi AP
  uint8_t macAddress[6] = {0x5C, 0x92, 0x5E, 0x38, 0x00, 0x68}; // Địa chỉ MAC tùy chọn
  
  // Khởi động WiFi AP trước khi thay đổi địa chỉ MAC
  WiFi.mode(WIFI_AP);
  
  // Thiết lập địa chỉ MAC cho WiFi AP
  esp_wifi_set_mac(WIFI_IF_AP, macAddress);

  // Bắt đầu AP với tên và mật khẩu
  WiFi.softAP("MyESP32", "password");

  // In ra địa chỉ MAC của AP
  Serial.print("WiFi AP is up with custom MAC address: ");
  Serial.println(WiFi.softAPmacAddress());
}

void loop() {
  // Chạy mã của bạn
}
