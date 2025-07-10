#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 15
#define LED 2

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);  // Mặc định tắt LED

    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa init failed!");
        while (1);
    }
    Serial.println("✅ LoRa receiver ready");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String received = "";
        while (LoRa.available()) {
            received += (char)LoRa.read();
        }

        Serial.println("📩 Nhận được: " + received);

        // ⚡ Bật LED khi nhận được gói
        digitalWrite(LED, HIGH);
        delay(100);  // Giữ sáng 100ms
        digitalWrite(LED, LOW);

         // Phân tích dữ liệu
        int key1 = getValue(received, "key1").toInt();
        int key2 = getValue(received, "key2").toInt();
     

        int joy1X = getValue(received, "joy1X").toInt();
        int joy1Y = getValue(received, "joy1Y").toInt();
       

        int joy2X = getValue(received, "joy2X").toInt();
        int joy2Y = getValue(received, "joy2Y").toInt();
  

        // Hiển thị dữ liệu
        Serial.println("🔹 Dữ liệu nhận được:");
        Serial.printf("➡ Key1: %d, Key2: %d\n", key1, key2);
        Serial.printf("🎮 Joystick 1 - X: %d, Y: %d\n", joy1X, joy1Y);
        Serial.printf("🎮 Joystick 2 - X: %d, Y: %d\n", joy2X, joy2Y);


    }

    
}


// 🔎 Hàm tách giá trị từ chuỗi key:value
String getValue(String data, String key) {
    int startIndex = data.indexOf(key + ":");
    if (startIndex == -1) return "";  
    startIndex += key.length() + 1;
    int endIndex = data.indexOf(" ", startIndex);
    if (endIndex == -1) endIndex = data.length();
    return data.substring(startIndex, endIndex);
}

