#include <SPI.h>
#include <LoRa.h>

#define SS    5    // Chân NSS (CS) của module LoRa
#define RST   14   // Chân RESET của module LoRa
#define DIO0  26   // Chân DIO0 của module LoRa

// Định nghĩa chân cho 2 LED
const int ledPin1 = 2;  // LED1: ví dụ LED nối với chân 2
const int ledPin2 = 4;  // LED2: ví dụ LED nối với chân 4

// Biến lưu trạng thái LED
bool ledState1 = false;
bool ledState2 = false;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  
  // Cấu hình module LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String message = LoRa.readString();
    Serial.print("Received: ");
    Serial.println(message);
    
    // Nếu nhận được lệnh "LED1 TOGGLE"
    if (message.indexOf("LED1 TOGGLE") != -1) {
      ledState1 = !ledState1;
      digitalWrite(ledPin1, ledState1 ? HIGH : LOW);
      Serial.println(ledState1 ? "LED1 ON" : "LED1 OFF");
    }
    
    // Nếu nhận được lệnh "LED2 TOGGLE"
    if (message.indexOf("LED2 TOGGLE") != -1) {
      ledState2 = !ledState2;
      digitalWrite(ledPin2, ledState2 ? HIGH : LOW);
      Serial.println(ledState2 ? "LED2 ON" : "LED2 OFF");
    }
  }
}
