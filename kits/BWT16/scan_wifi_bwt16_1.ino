#include <vector>
#include <stdio.h>  // cần để dùng snprintf
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
  record->SSID.val[record->SSID.len] = 0;  // Đảm bảo chuỗi null-terminated

  WiFiScanResult result;

  // Kiểm tra SSID có bị ẩn không
  if (record->SSID.len > 0) {
    result.ssid = String((const char*)record->SSID.val);
  } else {
    result.ssid = "<ẨN>";
  }

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

// Quét và in ra danh sách mạng
void scanWiFi() {
  Serial.println("🔧 Khởi động WiFi stack...");
  WiFi.begin((char*)"");  // Khởi tạo WiFi ở chế độ station

  delay(500);

  Serial.println("🔍 Bắt đầu quét Wi-Fi...");
  scan_results.clear();

  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);  // Đợi scan hoàn tất

    Serial.print("✅ Đã tìm thấy ");
    Serial.print(scan_results.size());
    Serial.println(" mạng Wi-Fi:\n");

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
    Serial.println("❌ Không thể quét Wi-Fi!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  scanWiFi();
}

void loop() {
  // Không làm gì cả
}
