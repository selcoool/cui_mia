#include <SPI.h>
#include <LoRa.h>

#define SS 5        // Chân NSS (CS)
#define RST 14      // Chân RESET
#define DIO0 26     // Chân DIO0

String nodeId = "Node1";  // ID của thiết bị gửi
String targetId = "Node2"; // ID của thiết bị nhận

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
  String message = targetId + ":Hello from XXXXXXXXXXXXXXX";

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.println("Đã gửi đến " + targetId + ": " + message);

  delay(5000);  // Gửi mỗi 2 giây
}