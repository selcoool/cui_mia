#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

BluetoothSerial BT;
bool scanning = false;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 15000; // 15 giây giữa các lần scan

void btCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    char bda_str[18];
    sprintf(bda_str, "%02x:%02x:%02x:%02x:%02x:%02x",
            param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
            param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);
    Serial.print("Found device: ");
    Serial.println(bda_str);
  } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
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
