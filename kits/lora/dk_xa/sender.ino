#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 26

#define JOY1_X 34
#define JOY1_Y 35
#define JOY1_B 32

#define JOY2_X 33
#define JOY2_Y 25
#define JOY2_B 4

#define BTN1 27
#define BTN2 14
#define BTN3 12

void setup() {
    Serial.begin(115200);

    pinMode(JOY1_B, INPUT_PULLUP);
    pinMode(JOY2_B, INPUT_PULLUP);
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    pinMode(BTN3, INPUT_PULLUP);

    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa init failed!");
        while (1);
    }
    Serial.println("✅ LoRa transmitter ready");
}

void loop() {
    // Đọc giá trị joystick
    int joy1X = analogRead(JOY1_X);
    int joy1Y = analogRead(JOY1_Y);
    int joy1B = !digitalRead(JOY1_B);  // Nút bấm nhấn = 1, thả = 0

    int joy2X = analogRead(JOY2_X);
    int joy2Y = analogRead(JOY2_Y);
    int joy2B = !digitalRead(JOY2_B);

    // Đọc trạng thái nút bấm
    int key1 = digitalRead(BTN1);
    int key2 = digitalRead(BTN2);
    String key3 = digitalRead(BTN3) ? "OFF" : "ON";  // Nút nhấn giữ bật

    // Tạo thông điệp gửi
    String message = "key1:" + String(key1) + " key2:" + String(key2) + " key3:" + key3 +
                     " joy1X:" + String(joy1X) + " joy1Y:" + String(joy1Y) + " joy1B:" + String(joy1B) +
                     " joy2X:" + String(joy2X) + " joy2Y:" + String(joy2Y) + " joy2B:" + String(joy2B);

    // Gửi dữ liệu qua LoRa
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();

    Serial.println("📤 Đã gửi: " + message);
    delay(500);  // Gửi mỗi 500ms
}
