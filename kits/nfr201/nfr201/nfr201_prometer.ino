#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);  // CE, CSN cho Arduino Nano

// Địa chỉ của mô-đun nhận
const byte address[6] = "00001";  

// Định nghĩa các chân analog và digital
#define JOY1_X A0
#define JOY1_Y A1
#define JOY1_B 2

#define JOY2_X A2
#define JOY2_Y A3
#define JOY2_B 3

#define BTN1 4
#define BTN2 5
#define BTN3 6

void setup() {
  Serial.begin(115200);  // Khởi tạo giao tiếp Serial

  // Cài đặt chân joystick và nút nhấn
  pinMode(JOY1_B, INPUT_PULLUP);
  pinMode(JOY2_B, INPUT_PULLUP);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  radio.begin();  // Khởi tạo nRF24L01
  radio.openWritingPipe(address);  // Đặt địa chỉ để gửi dữ liệu
  radio.setPALevel(RF24_PA_HIGH);  // Cài đặt công suất phát
  Serial.println("✅ nRF24L01 sender ready");
}

void loop() {
  // Đọc giá trị từ joystick và nút nhấn
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);
  int joy1B = !digitalRead(JOY1_B); // Nút bấm nhấn = 1, thả = 0

  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);
  int joy2B = !digitalRead(JOY2_B);

  // Đọc trạng thái các nút bấm
  int key1 = digitalRead(BTN1);
  int key2 = digitalRead(BTN2);
  String key3 = digitalRead(BTN3) ? "OFF" : "ON";  // Nút nhấn giữ bật

  // Tạo thông điệp để gửi
  String message = 
                   " joy1X:" + String(joy1X) + " joy1Y:" + String(joy1Y) + " joy1B:" + String(joy1B) +
                   " joy2X:" + String(joy2X) + " joy2Y:" + String(joy2Y) + " joy2B:" + String(joy2B);

  // Gửi thông điệp qua nRF24L01
  radio.write(message.c_str(), message.length());
  Serial.println("📤 Đã gửi: " + message);

  delay(500);  // Gửi mỗi 500ms
}
