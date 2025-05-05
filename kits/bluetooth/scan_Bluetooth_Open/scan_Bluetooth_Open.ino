#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

BluetoothSerial BT;
bool scanning = false;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 15000; // 15 giây

void btCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    Serial.print("Device found: ");
    for (int i = 0; i < 6; i++) {
      Serial.print(param->disc_res.bda[i], HEX);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }

  if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      Serial.println("Discovery stopped.");
      scanning = false;
    } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
      Serial.println("Discovery started.");
      scanning = true;
    }
  }
}

void setup() {
  Serial.begin(115200);  // Mở Serial Monitor
  Serial.println("Initializing Bluetooth...");

  if (!BT.begin("ESP32_Scanner")) {
    Serial.println("Bluetooth initialization failed!");
    return;
  }

  esp_bt_gap_register_callback(btCallback);
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);  // Bắt đầu quét
  scanning = true;
  lastScanTime = millis();
}

void loop() {
  if (!scanning && millis() - lastScanTime >= SCAN_INTERVAL) {
    Serial.println("Restarting scan...");
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);  // Khởi động lại quét
    scanning = true;
    lastScanTime = millis();
  }
  delay(100);  // Thời gian trễ giữa các lần quét
}
