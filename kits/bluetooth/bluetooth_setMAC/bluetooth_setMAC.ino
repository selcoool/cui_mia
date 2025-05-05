#include "Arduino.h"
#include "BluetoothSerial.h" // Thư viện BluetoothSerial để sử dụng Bluetooth Classic

BluetoothSerial SerialBT;

#define NEW_MAC_ADDRESS {0x12, 0x1B, 0x44, 0x11, 0x3A, 0x02}  // Địa chỉ MAC hợp lệ khác

void setup() {
  Serial.begin(115200);
  
  // Địa chỉ MAC mới cho Bluetooth
  uint8_t new_mac[6] = NEW_MAC_ADDRESS;

  // Thay đổi địa chỉ MAC cho Bluetooth
  esp_err_t err = esp_base_mac_addr_set(new_mac); // Chỉ cần truyền mảng MAC mới
  if (err == ESP_OK) {
    Serial.println("Bluetooth MAC address set successfully.");
  } else {
    Serial.println("Failed to set Bluetooth MAC address.");
  }

  // Khởi tạo Bluetooth Serial
  if (!SerialBT.begin("ESP32_Device")) {
    Serial.println("An error occurred while initializing Bluetooth.");
    return;
  }

  Serial.println("Bluetooth initialized and ready to connect!");

  // In ra địa chỉ MAC của Bluetooth sau khi thay đổi
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT); // Lấy địa chỉ MAC của Bluetooth
  Serial.print("Current Bluetooth MAC address: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      Serial.print(":");
    }
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

void loop() {
  // Ví dụ gửi dữ liệu qua Bluetooth Serial
  SerialBT.println("Hello from ESP32 via Bluetooth!");
  delay(1000);
}
