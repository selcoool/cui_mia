#include <vector>
#include <stdio.h>
#include "WiFi.h"
#include "wifi_conf.h"
#include "wifi_structures.h"
#include "wifi_util.h"

// Cấu trúc lưu thông tin mạng Wi-Fi
typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint channel;
} WiFiScanResult;

std::vector<WiFiScanResult> scan_results;

// Hàm callback xử lý kết quả từng mạng Wi-Fi
rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  if (scan_result->scan_complete) return RTW_SUCCESS;

  rtw_scan_result_t *record = &scan_result->ap_details;
  record->SSID.val[record->SSID.len] = 0;  // null-terminated

  WiFiScanResult result;
  result.ssid = (record->SSID.len > 0) ? String((const char*)record->SSID.val) : "<ẨN>";
  result.channel = record->channel;
  result.rssi = record->signal_strength;
  memcpy(result.bssid, record->BSSID.octet, 6);

  char bssid_str[18];
  snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           result.bssid[0], result.bssid[1], result.bssid[2],
           result.bssid[3], result.bssid[4], result.bssid[5]);
  result.bssid_str = String(bssid_str);

  scan_results.push_back(result);
  return RTW_SUCCESS;
}

// Quét Wi-Fi
void scanWiFi() {
  Serial.println("🔧 Khởi động WiFi stack...");
  WiFi.begin((char*)""); // Chỉ khởi tạo Wi-Fi

  delay(500);

  Serial.println("🔍 Quét Wi-Fi...");
  scan_results.clear();

  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);

    Serial.print("✅ Tìm thấy ");
    Serial.print(scan_results.size());
    Serial.println(" mạng Wi-Fi.\n");

    for (size_t i = 0; i < scan_results.size(); i++) {
      String band = (scan_results[i].channel >= 36) ? "5 GHz" : "2.4 GHz";
      Serial.println("📶 SSID   : " + scan_results[i].ssid);
      Serial.println("🔗 BSSID  : " + scan_results[i].bssid_str);
      Serial.println("📡 Kênh   : " + String(scan_results[i].channel));
      Serial.println("📉 RSSI   : " + String(scan_results[i].rssi) + " dBm");
      Serial.println("🛰️ Băng tần: " + band);
      Serial.println("-------------------------------------");
    }
  } else {
    Serial.println("❌ Quét Wi-Fi thất bại!");
  }
}

// Hàm tìm theo MAC
void findWiFiByMAC(String mac) {
  bool found = false;
  for (auto &net : scan_results) {
    if (net.bssid_str.equalsIgnoreCase(mac)) {
      Serial.println("✅ Thông tin mạng Wi-Fi tìm thấy:");
      Serial.println("📶 SSID   : " + net.ssid);
      Serial.println("🔗 BSSID  : " + net.bssid_str);
      Serial.println("📡 Kênh   : " + String(net.channel));
      Serial.println("📉 RSSI   : " + String(net.rssi) + " dBm");
      Serial.println("-------------------------------------");
      found = true;
      break;
    }
  }
  if (!found) {
    Serial.println("❌ Không tìm thấy Wi-Fi với MAC: " + mac);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  scanWiFi();

  Serial.println("\nNhập địa chỉ MAC (ví dụ: 5C:92:5E:38:00:68) để tra cứu thông tin Wi-Fi:");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // loại bỏ khoảng trắng
    if (input.length() > 0) {
      findWiFiByMAC(input);
      Serial.println("\nNhập tiếp địa chỉ MAC khác nếu muốn:");
    }
  }
}
