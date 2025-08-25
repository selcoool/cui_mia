#include "WiFi.h"
#include "esp_wifi.h"
#include "esp_log.h"

void setup() {
  Serial.begin(115200);

  // Init Wi-Fi
  WiFi.mode(WIFI_MODE_AP);

  // Set custom MAC address
  uint8_t customMac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, customMac);
  if (err == ESP_OK) Serial.println("MAC set successfully");

  // Config AP
  wifi_config_t apConfig = {0};
  strcpy((char*)apConfig.ap.ssid, "ESP32C5_5GHz");
  apConfig.ap.ssid_len = strlen("ESP32C5_5GHz");
  apConfig.ap.channel = 36;
  apConfig.ap.authmode = WIFI_AUTH_WPA2_PSK;
  strcpy((char*)apConfig.ap.password, "12345678");
  apConfig.ap.max_connection = 4;

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &apConfig);
  esp_wifi_start();

  // Print MAC
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP, mac);
  Serial.printf("Current MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void loop() {
  // Không làm gì cả
}
