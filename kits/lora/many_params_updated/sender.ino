#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 4
#define DIO0 26
#define LED 2  

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

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

        // Phân tích dữ liệu
        int key1 = getValue(received, "key1").toInt();
        int key2 = getValue(received, "key2").toInt();
        String key3 = getValue(received, "key3");

        int joy1X = getValue(received, "joy1X").toInt();
        int joy1Y = getValue(received, "joy1Y").toInt();
        bool joy1B = getValue(received, "joy1B") == "1";  

        int joy2X = getValue(received, "joy2X").toInt();
        int joy2Y = getValue(received, "joy2Y").toInt();
        bool joy2B = getValue(received, "joy2B") == "1";  

        // Hiển thị dữ liệu
        Serial.println("🔹 Dữ liệu nhận được:");
        Serial.printf("➡ Key1: %d, Key2: %d, Key3: %s\n", key1, key2, key3.c_str());
        Serial.printf("🎮 Joystick 1 - X: %d, Y: %d, Button: %s\n", joy1X, joy1Y, joy1B ? "Pressed" : "Released");
        Serial.printf("🎮 Joystick 2 - X: %d, Y: %d, Button: %s\n", joy2X, joy2Y, joy2B ? "Pressed" : "Released");

        // Điều khiển LED
        if (key3 == "ON") {
            digitalWrite(LED, HIGH);
            Serial.println("💡 LED bật!");
        } else if (key3 == "OFF") {
            digitalWrite(LED, LOW);
            Serial.println("💡 LED tắt!");
        }
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
