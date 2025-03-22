#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 14
#define DIO0 26

#define JOY1_X 35
#define JOY1_Y 34
#define JOY1_B 25

#define JOY2_X 33
#define JOY2_Y 32
#define JOY2_B 27

#define BTN3 4  // Nút nhấn toggle
#define LED 2   // Đèn LED trên GPIO2

bool btnState = false;  // Trạng thái ON/OFF
bool lastButtonState = HIGH;  // Lưu trạng thái trước đó của nút
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;  // Thời gian chống rung nút (50ms)

void setup() {
    Serial.begin(115200);
    pinMode(JOY1_B, INPUT_PULLUP);
    pinMode(JOY2_B, INPUT_PULLUP);
    pinMode(BTN3, INPUT_PULLUP);
    pinMode(LED, OUTPUT);  // Cấu hình chân LED là OUTPUT

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
    int joy1B = !digitalRead(JOY1_B);  

    int joy2X = analogRead(JOY2_X);
    int joy2Y = analogRead(JOY2_Y);
    int joy2B = !digitalRead(JOY2_B);

    // Xử lý toggle nút BTN3
    bool buttonRead = digitalRead(BTN3);
    if (buttonRead == LOW && lastButtonState == HIGH) {
        unsigned long currentTime = millis();
        if (currentTime - lastDebounceTime > debounceDelay) {
            btnState = !btnState;  // Đảo trạng thái ON/OFF
            digitalWrite(LED, btnState ? HIGH : LOW);  // Bật/Tắt đèn
            lastDebounceTime = currentTime;
        }
    }
    lastButtonState = buttonRead;

    // Chuyển trạng thái nút thành chuỗi
    String key3 = btnState ? "ON" : "OFF";

    // Tạo thông điệp gửi LoRa
    String message = "key3:" + key3 +
                     " joy1X:" + String(joy1X) + " joy1Y:" + String(joy1Y) + " joy1B:" + String(joy1B) +
                     " joy2X:" + String(joy2X) + " joy2Y:" + String(joy2Y) + " joy2B:" + String(joy2B);

    // Gửi dữ liệu qua LoRa
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();

    Serial.println("📤 Đã gửi: " + message);
    delay(500);  // Gửi mỗi 500ms
}
