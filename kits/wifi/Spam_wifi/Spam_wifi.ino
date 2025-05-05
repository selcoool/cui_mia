#include <WiFi.h>
#include "esp_wifi.h"

// Tên Wi-Fi và mật khẩu của bạn
const char *ssid = "ESP32_AP";  
const char *password = "123456789";  

void setup() {
  Serial.begin(115200);

  // Đặt một địa chỉ MAC giả cho WiFi AP
  uint8_t macAddress[6] = {0x5C, 0x92, 0x5E, 0x38, 0x00, 0x68}; // Địa chỉ MAC tùy chọn
  
  // Khởi động WiFi AP trước khi thay đổi địa chỉ MAC
  WiFi.mode(WIFI_AP);
  
  // Thiết lập địa chỉ MAC cho WiFi AP
  esp_wifi_set_mac(WIFI_IF_AP, macAddress);

  // Bắt đầu làm Access Point (AP)
  WiFi.softAP(ssid, password, 3, false, 1);  // channel 3, không bảo mật, chỉ 1 kết nối

  // In ra địa chỉ MAC của AP
  Serial.print("WiFi AP is up with custom MAC address: ");
  Serial.println(WiFi.softAPmacAddress());

  // In ra địa chỉ IP của AP
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Mạng Wi-Fi đã tạo và có thể kết nối từ các thiết bị khác
}
