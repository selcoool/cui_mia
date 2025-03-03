#include <SPI.h>
#include <LoRa.h>

#define SS    5    // Chân NSS (CS) của module LoRa
#define RST   14   // Chân RESET của module LoRa
#define DIO0  26   // Chân DIO0 của module LoRa

// Định nghĩa chân cho 2 nút
const int buttonPin1 = 15;  // Nút 1: dùng để gửi lệnh "LED1 TOGGLE"
const int buttonPin2 = 4;   // Nút 2: dùng để gửi lệnh "LED2 TOGGLE"

// Biến lưu trạng thái nút để debounce
bool lastButtonState1 = HIGH;
bool lastButtonState2 = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin1, INPUT_PULLUP);  // Nút dùng nội bộ pull-up
  pinMode(buttonPin2, INPUT_PULLUP);
  
  // Cấu hình module LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {  // Sử dụng tần số 433MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
}

void loop() {
  bool currentButtonState1 = digitalRead(buttonPin1);
  bool currentButtonState2 = digitalRead(buttonPin2);
  
  // Nếu nút 1 chuyển từ HIGH (không nhấn) sang LOW (nhấn)
  if (lastButtonState1 == HIGH && currentButtonState1 == LOW) {
    LoRa.beginPacket();
    LoRa.print("LED1 TOGGLE");
    LoRa.endPacket();
    Serial.println("Sent: LED1 TOGGLE");
    delay(50); // Debounce đơn giản
  }
  
  // Nếu nút 2 chuyển từ HIGH sang LOW
  if (lastButtonState2 == HIGH && currentButtonState2 == LOW) {
    LoRa.beginPacket();
    LoRa.print("LED2 TOGGLE");
    LoRa.endPacket();
    Serial.println("Sent: LED2 TOGGLE");
    delay(50);
  }
  
  lastButtonState1 = currentButtonState1;
  lastButtonState2 = currentButtonState2;
}
