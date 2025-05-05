#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

BluetoothSerial BT;
bool scanning = false;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 15000;

uint8_t current_bda[6]; // Địa chỉ MAC tạm thời lưu lại

void btCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    // Lưu địa chỉ MAC để dùng sau
    memcpy(current_bda, param->disc_res.bda, sizeof(current_bda));

    // Gửi yêu cầu lấy tên thiết bị
    esp_bt_gap_read_remote_name(current_bda);
  } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      Serial.println("Discovery stopped.");
      scanning = false;
    } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
      Serial.println("Discovery started.");
      scanning = true;
    }
  } else if (event == ESP_BT_GAP_READ_REMOTE_NAME_EVT) {
    if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
      Serial.print("Device: ");
      if (param->read_rmt_name.rmt_name != NULL) {
        Serial.print((char*)param->read_rmt_name.rmt_name);
      } else {
        Serial.print("Name not available");
      }

      // In địa chỉ MAC đã lưu trước đó
      char bda_str[18];
      sprintf(bda_str, "%02x:%02x:%02x:%02x:%02x:%02x",
              current_bda[0], current_bda[1], current_bda[2],
              current_bda[3], current_bda[4], current_bda[5]);

      Serial.print(" | MAC: ");
      Serial.println(bda_str);
    } else {
      Serial.println("Failed to get device name.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Bluetooth Classic Scan...");

  if (!BT.begin("ESP32_Scanner")) {
    Serial.println("Bluetooth init failed!");
    return;
  }

  esp_bt_gap_register_callback(btCallback);
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
  scanning = true;
  lastScanTime = millis();
}

void loop() {
  if (!scanning && millis() - lastScanTime >= SCAN_INTERVAL) {
    Serial.println("Restarting scan...");
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    scanning = true;
    lastScanTime = millis();
  }
  delay(100);
}
