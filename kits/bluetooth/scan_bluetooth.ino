#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* pBLEScan;

void setup() {
  Serial.begin(115200);  // Bắt đầu monitor Serial
  BLEDevice::init("");   // Khởi tạo thiết bị BLE

  pBLEScan = BLEDevice::getScan();  // Tạo đối tượng quét BLE
  pBLEScan->setActiveScan(true);    // Quét chủ động (sẽ nhận thêm thông tin về thiết bị)
  pBLEScan->setInterval(100);       // Khoảng thời gian giữa các lần quét (ms)
  pBLEScan->setWindow(99);          // Cửa sổ quét (ms)

  Serial.println("Scanning for Bluetooth devices...");
}

void loop() {
  BLEScanResults foundDevices = pBLEScan->start(5); // Quét trong 5 giây
  int count = foundDevices.getCount();
  
  if (count == 0) {
    Serial.println("No devices found");
  } else {
    Serial.print(count);
    Serial.println(" devices found:");

    for (int i = 0; i < count; ++i) {
      BLEAdvertisedDevice advertisedDevice = foundDevices.getDevice(i);
      Serial.print("Device #");
      Serial.print(i + 1);
      Serial.print(": ");
      
      // In tên thiết bị
      Serial.print("Name: ");
      String deviceName = advertisedDevice.getName().c_str();
      if (deviceName == "") {
        deviceName = "Unknown";  // Nếu không có tên, hiển thị Unknown
      }
      Serial.print(deviceName);
      
      // In RSSI (Cường độ tín hiệu)
      Serial.print(", RSSI: ");
      Serial.print(advertisedDevice.getRSSI());  
      Serial.print(" dBm");
      
      // In Địa chỉ MAC
      Serial.print(", Address: ");
      Serial.print(advertisedDevice.getAddress().toString().c_str()); // Địa chỉ MAC (BSSID)
      
      // Kiểm tra và hiển thị UUID dịch vụ nếu có
      if (advertisedDevice.haveServiceUUID()) {
        Serial.print(", Service UUID: ");
        Serial.println(advertisedDevice.getServiceUUID().toString().c_str());  // UUID của dịch vụ quảng bá
      }
      Serial.println();
    }
  }
  delay(2000); // Delay 2 giây trước khi quét lại
}
