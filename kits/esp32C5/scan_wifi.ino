#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("🔍 Bắt đầu quét Wi-Fi...");
  
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("❌ Không tìm thấy mạng nào");
  } else {
    Serial.printf("✅ Đã tìm thấy %d mạng Wi-Fi:\n\n", n);
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      String bssid = WiFi.BSSIDstr(i);
      int32_t rssi = WiFi.RSSI(i);
      int32_t channel = WiFi.channel(i);
      wifi_auth_mode_t auth = WiFi.encryptionType(i);
      String authStr;
      switch(auth) {
        case WIFI_AUTH_OPEN: authStr = "OPEN"; break;
        case WIFI_AUTH_WEP: authStr = "WEP"; break;
        case WIFI_AUTH_WPA_PSK: authStr = "WPA_PSK"; break;
        case WIFI_AUTH_WPA2_PSK: authStr = "WPA2_PSK"; break;
        case WIFI_AUTH_WPA_WPA2_PSK: authStr = "WPA/WPA2_PSK"; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: authStr = "WPA2_ENTERPRISE"; break;
        default: authStr = "UNKNOWN"; break;
      }

      String band = (channel >= 36) ? "5 GHz" : "2.4 GHz";

      Serial.println("-------------------------------------");
      Serial.println("📶 SSID    : " + ssid);
      Serial.println("🔗 BSSID   : " + bssid);
      Serial.println("📡 Kênh    : " + String(channel));
      Serial.println("📉 RSSI    : " + String(rssi) + " dBm");
      Serial.println("🔒 Bảo mật : " + authStr);
      Serial.println("🛰️ Băng tần: " + band);
    }
    Serial.println("-------------------------------------");
  }
}

void loop() {
  // Không cần làm gì
}
