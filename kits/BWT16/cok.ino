#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Cấu hình Wi-Fi ở chế độ station (bắt buộc để scan hoạt động)
  char dummy_ssid[] = "dummy";
  char dummy_pass[] = "12345678";

  Serial.println("🔧 Bật Wi-Fi ở chế độ station để quét...");

  // Kết nối vào một SSID giả để bật Wi-Fi
  WiFi.begin(dummy_ssid, dummy_pass);  // không cần kết nối thành công

  delay(3000);  // đợi Wi-Fi khởi động xong

  Serial.println("🔍 Bắt đầu quét Wi-Fi...");

  WiFi.disconnect();  // ngắt khỏi SSID giả (nếu cần)
  delay(500);

  int n = WiFi.scanNetworks();

  if (n <= 0) {
    Serial.println("❌ Không tìm thấy mạng Wi-Fi nào.");
  } else {
    Serial.print("✅ Tìm thấy ");
    Serial.print(n);
    Serial.println(" mạng Wi-Fi:\n");

    for (int i = 0; i < n; ++i) {
      Serial.print("📶 SSID: ");
      Serial.println(WiFi.SSID(i));

      Serial.print("📡 RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");

      Serial.println("---------------------------");
    }
  }
}

void loop() {
  // không cần làm gì
}
