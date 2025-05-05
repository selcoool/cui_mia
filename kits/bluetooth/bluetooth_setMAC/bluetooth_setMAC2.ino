#include "Arduino.h"
#include "BluetoothSerial.h" // Thư viện BluetoothSerial để sử dụng Bluetooth Classic

BluetoothSerial SerialBT;

// Đây là địa chỉ MAC mà bạn MUỐN Bluetooth có
#define TARGET_BT_MAC {0x12, 0x1B, 0x44, 0x11, 0x3A, 0xBA}

void setup() {
  Serial.begin(115200);

  // Gán địa chỉ bạn muốn Bluetooth có
  uint8_t target_bt_mac[6] = TARGET_BT_MAC;

  // Tạo base MAC bằng cách giảm byte cuối đi 2
  uint8_t base_mac[6];
  memcpy(base_mac, target_bt_mac, 6);
  base_mac[5] -= 2;

  // Set base MAC
  esp_err_t err = esp_base_mac_addr_set(base_mac);
  if (err == ESP_OK) {
    Serial.println("Base MAC address set successfully.");
  } else {
    Serial.println("Failed to set base MAC address.");
  }

  // Khởi tạo Bluetooth
  if (!SerialBT.begin("ESP32_Device")) {
    Serial.println("Bluetooth init failed.");
    return;
  }

  Serial.println("Bluetooth initialized and ready to connect!");

  // In ra địa chỉ MAC thực tế của Bluetooth
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  Serial.print("Current Bluetooth MAC address: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(":");
    Serial.printf("%02X", mac[i]);
  }
  Serial.println();
}

void loop() {
  SerialBT.println("Hello from ESP32 via Bluetooth!");
  delay(1000);
}
