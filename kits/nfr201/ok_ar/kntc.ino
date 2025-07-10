#include <SPI.h>
#include <LoRa.h>

// CE = 10, RST = 9, DIO0 = 2
#define SS_PIN    10
#define RST_PIN   7
#define DIO0_PIN  2

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver");

  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String message = "";

    while (LoRa.available()) {
      message += (char)LoRa.read();
    }

    Serial.print("Da nhan: ");
    Serial.println(message);
  }
}
