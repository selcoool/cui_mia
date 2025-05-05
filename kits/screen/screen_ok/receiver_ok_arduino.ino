#include <SPI.h>
#include <LoRa.h>

#define SS 10    // Chân CS (NSS) cho LoRa
#define RST 9    // Chân Reset cho LoRa
#define DIO0 2   // Chân DIO0 cho LoRa
#define LED 5    // Chân LED

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
        Serial.print("➡ Key1: "); Serial.print(key1);
        Serial.print(", Key2: "); Serial.print(key2);
        Serial.print(", Key3: "); Serial.println(key3);

        Serial.print("🎮 Joystick 1 - X: "); Serial.print(joy1X);
        Serial.print(", Y: "); Serial.print(joy1Y);
        Serial.print(", Button: "); Serial.println(joy1B ? "Pressed" : "Released");

        Serial.print("🎮 Joystick 2 - X: "); Serial.print(joy2X);
        Serial.print(", Y: "); Serial.print(joy2Y);
        Serial.print(", Button: "); Serial.println(joy2B ? "Pressed" : "Released");

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