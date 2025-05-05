#include "BluetoothSerial.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

BluetoothSerial BT;
bool scanning = false;
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 15000; // 15 giây

uint8_t current_bda[6]; // Lưu địa chỉ MAC gần nhất

void printDeviceInfo(esp_bt_gap_cb_param_t *param) {
  // In địa chỉ MAC
  char bda_str[18];
  sprintf(bda_str, "%02x:%02x:%02x:%02x:%02x:%02x",
          param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
          param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

  String deviceName = "Unknown";
  int8_t rssi = 0;
  uint32_t cod = 0;

  for (int i = 0; i < param->disc_res.num_prop; i++) {
    esp_bt_gap_dev_prop_t *p = &param->disc_res.prop[i];

    if (p->type == ESP_BT_GAP_DEV_PROP_BDNAME) {
      char *name = (char *)p->val;
      deviceName = String(name);
    }

    else if (p->type == ESP_BT_GAP_DEV_PROP_RSSI) {
      rssi = *(int8_t *)p->val;
    }

    else if (p->type == ESP_BT_GAP_DEV_PROP_COD) {
      cod = *(uint32_t *)p->val;
    }
  }

  // In tất cả thông tin trên 1 dòng
  Serial.print("Name: "); Serial.print(deviceName);
  Serial.print(" | MAC: "); Serial.print(bda_str);
  Serial.print(" | RSSI: "); Serial.print(rssi);
  Serial.print(" | COD: 0x"); Serial.println(cod, HEX);
}

void btCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    memcpy(current_bda, param->disc_res.bda, sizeof(current_bda));
    printDeviceInfo(param);

    // Nếu muốn chắc ăn, có thể yêu cầu lại tên:
    esp_bt_gap_read_remote_name(current_bda);
  }

  else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      Serial.println("Discovery stopped.");
      scanning = false;
    } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
      Serial.println("Discovery started.");
      scanning = true;
    }
  }

  else if (event == ESP_BT_GAP_READ_REMOTE_NAME_EVT) {
    if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
      Serial.print("Remote name (fetched): ");
      Serial.println((char*)param->read_rmt_name.rmt_name);
    } else {
      Serial.println("Failed to get remote name.");
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
