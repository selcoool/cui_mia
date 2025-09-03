#include "WiFi.h"
#include "esp_wifi.h"
#include "esp_log.h"

void printWiFiInfo() {
  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_AP, &conf);

  Serial.println("========== AP Info ==========");
  Serial.printf("SSID: %s\n", conf.ap.ssid);
  Serial.printf("Password: %s\n", conf.ap.password);
  Serial.printf("Channel: %d\n", conf.ap.channel);
  Serial.printf("Authmode: %d (0=OPEN,3=WPA2,4=WPA3)\n", conf.ap.authmode);
  Serial.printf("Max Connections: %d\n", conf.ap.max_connection);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("=============================");
}

void setup() {
  Serial.begin(115200);

  // Init Wi-Fi ở chế độ AP
  WiFi.mode(WIFI_MODE_AP);

  // Đặt MAC address tùy chỉnh
  uint8_t customMac[6] = {0x5C, 0x92, 0x5E, 0x38, 0x00, 0x68};
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, customMac);
  if (err == ESP_OK) {
    Serial.println("MAC set successfully");
  } else {
    Serial.printf("Failed to set MAC, error: %d\n", err);
  }

  // Cấu hình Access Point
  wifi_config_t apConfig;
  memset(&apConfig, 0, sizeof(apConfig));

  strcpy((char*)apConfig.ap.ssid, "D0806");   // SSID WiFi
  apConfig.ap.ssid_len = 0;  // ESP tự tính
  strcpy((char*)apConfig.ap.password, "12345678");   // mật khẩu WiFi
  apConfig.ap.channel = 3;   // dùng kênh 2.4GHz
  apConfig.ap.authmode = WIFI_AUTH_WPA2_PSK;
  apConfig.ap.max_connection = 4;

  // Áp dụng cấu hình
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &apConfig);
  esp_wifi_start();

  // In thông tin Wi-Fi AP
  delay(500);  // đợi AP khởi động
  printWiFiInfo();
}

void loop() {
  // không làm gì cả
}
