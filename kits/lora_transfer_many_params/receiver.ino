#include <SPI.h>
#include <LoRa.h>

#define SS 5        // Chân NSS (CS)
#define RST 14      // Chân RESET
#define DIO0 26     // Chân DIO0

const int ledPin = 13;  // Chân điều khiển LED (hoặc relay)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Tần số LoRa: 433MHz
    Serial.println("Không khởi tạo được LoRa!");
    while (true);
  }
  Serial.println("LoRa đã khởi tạo xong!");

  pinMode(ledPin, OUTPUT); // Chân LED là OUTPUT
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String message = LoRa.readString();
    Serial.println("Nhận được: " + message);

    // Phân tích dữ liệu nhận được
    if (message.startsWith("LED:")) {
      String command = message.substring(4); // Lấy phần sau "LED:"
      if (command == "ON") {
        digitalWrite(ledPin, HIGH); // Bật LED
        Serial.println("Đèn bật");
      } else if (command == "OFF") {
        digitalWrite(ledPin, LOW); // Tắt LED
        Serial.println("Đèn tắt");
      }
    } else if (message.startsWith("POT:")) {
      String value = message.substring(4); // Lấy phần sau "POT:"
      int brightness = value.toInt(); // Chuyển thành số nguyên
      Serial.println("Độ sáng nhận được: " + String(brightness));
    } else if (message.startsWith("JOY:")) {
      String joystickData = message.substring(4); // Lấy phần sau "JOY:"
      int commaIndex = joystickData.indexOf(',');
      int joystickX = joystickData.substring(0, commaIndex).toInt();
      int joystickY = joystickData.substring(commaIndex + 1).toInt();
      Serial.println("Joystick X: " + String(joystickX) + ", Y: " + String(joystickY));
    }
  }
}
