#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(3000); // delay để Serial Monitor kịp khởi động

  // Chế độ STA (station)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // ngắt nếu đang kết nối WiFi

  // In MAC address ra Serial
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // không cần làm gì thêm
}


// esp32 c3 super mini:   18:8B:0E:92:66:34

// esp32 dev : 14:2B:2F:EC:17:B8