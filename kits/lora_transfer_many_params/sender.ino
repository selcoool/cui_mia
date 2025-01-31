#include <SPI.h>
#include <LoRa.h>

#define SS 5        // Chân NSS (CS)
#define RST 14      // Chân RESET
#define DIO0 26     // Chân DIO0

const int potentiometerPin = 34;  // Chân ADC kết nối với potentiometer
const int joystickXPin = 35;      // Chân ADC kết nối với joystick trục X
const int joystickYPin = 32;      // Chân ADC kết nối với joystick trục Y

String targetId = "Node2";  // ID của thiết bị nhận

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Tần số LoRa: 433MHz
    Serial.println("Không khởi tạo được LoRa!");
    while (true);
  }
  Serial.println("LoRa đã khởi tạo xong!");
}

void loop() {
  // Đọc giá trị potentiometer
  int potValue = analogRead(potentiometerPin);
  int brightness = map(potValue, 0, 4095, 0, 255);

  // Đọc giá trị joystick
  int joystickX = analogRead(joystickXPin);
  int joystickY = analogRead(joystickYPin);

  joystickX = map(joystickX, 0, 4095, -100, 100); // Chuyển về giá trị -100 đến 100
  joystickY = map(joystickY, 0, 4095, -100, 100);

  // Truyền giá trị analog của potentiometer
  String potMessage = "POT:" + String(brightness);
  LoRa.beginPacket();
  LoRa.print(potMessage);
  LoRa.endPacket();
  Serial.println("Gửi: " + potMessage);

  delay(500); // Gửi mỗi 500ms

  // Truyền giá trị joystick
  String joystickMessage = "JOY:" + String(joystickX) + "," + String(joystickY);
  LoRa.beginPacket();
  LoRa.print(joystickMessage);
  LoRa.endPacket();
  Serial.println("Gửi: " + joystickMessage);

  delay(500); // Gửi mỗi 500ms
}
