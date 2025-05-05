#include <BluetoothSerial.h>  // Thư viện BluetoothSerial cho ESP32

BluetoothSerial SerialBT;  // Tạo đối tượng BluetoothSerial

void setup() {
  // Khởi tạo giao tiếp Serial cho console
  Serial.begin(115200);  

  // Khởi tạo Bluetooth
  if (!SerialBT.begin("ESP32_BT_Device")) {  // Đặt tên cho thiết bị Bluetooth của bạn
    Serial.println("Bluetooth init failed!");
    while (1);
  }
  Serial.println("Bluetooth initialized!");

  // Lấy địa chỉ MAC Bluetooth và hiển thị
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);  // Lấy địa chỉ MAC Bluetooth
  Serial.printf("Bluetooth MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 mac[0], mac[1], mac[2],
                 mac[3], mac[4], mac[5]);
}

void loop() {
  // Kiểm tra nếu có dữ liệu gửi qua Bluetooth
  if (SerialBT.available()) {
    char incomingChar = SerialBT.read();
    Serial.print(incomingChar);  // Hiển thị dữ liệu nhận được từ Bluetooth
  }
}
