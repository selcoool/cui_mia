#include <SPI.h>
#include <LoRa.h>

#define SS 5        // Chân NSS (CS)
#define RST 14      // Chân RESET
#define DIO0 26     // Chân DIO0

int ledPin = 2; // Chân điều khiển đèn LED (hoặc relay)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {  // Tần số LoRa: 433MHz
    Serial.println("Không khởi tạo được LoRa!");
    while (true);
  }
  Serial.println("LoRa đã khởi tạo xong!");

  pinMode(ledPin, OUTPUT);  // Cài đặt chân điều khiển đèn LED
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String message = LoRa.readString();
    Serial.println("Nhận được tin nhắn: " + message);

    if (message.indexOf("ON") != -1) {
      digitalWrite(ledPin, HIGH);  // Bật đèn
      Serial.println("Đèn đã bật");
    } else if (message.indexOf("OFF") != -1) {
      digitalWrite(ledPin, LOW);  // Tắt đèn
      Serial.println("Đèn đã tắt");
    }
  }
}