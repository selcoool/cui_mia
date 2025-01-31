#include <SPI.h>
#include <LoRa.h>

#define SS 5        // Chân NSS (CS)
#define RST 14      // Chân RESET
#define DIO0 26     // Chân DIO0

String targetId = "Node2";  // ID của thiết bị này (Node2)

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
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Nhận được gói tin!");

    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
       Serial.println(LoRa.read());
        Serial.println((char)LoRa.read());
       
    }

    Serial.println("Dữ liệu nhận: " + receivedMessage);

    int separatorIndex = receivedMessage.indexOf(':');
    if (separatorIndex != -1) {
      String senderId = receivedMessage.substring(0, separatorIndex);  // ID của thiết bị gửi
      String messageContent = receivedMessage.substring(separatorIndex + 1);  // Nội dung gói tin

      if (senderId == "Node1") {
        Serial.println("Đã nhận từ Node1!");
        Serial.println("Nội dung: " + messageContent);
      } else {
        Serial.println("Thiết bị gửi không phải Node1.");
      }
    } else {
      Serial.println("Không tìm thấy ID trong dữ liệu!");
    }
  }
}