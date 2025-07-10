#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// LoRa pins
#define SS 5
#define RST 14
#define DIO0 26

int batteryPin = 39 ;  // Chân ADC để đọc điện áp pin

// Joystick 2
#define JOY2_X 35
#define JOY2_Y 34
#define JOY2_B 25

// Joystick 1
#define JOY1_X 33
#define JOY1_Y 32
#define JOY1_B 27

// Button & LED
#define BTN3 4
#define LED 2

bool btnState = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

float referenceVoltage = 3.3;  // Điện áp tham chiếu

void setup() {
  Serial.begin(115200);
  
  // Joystick button input
  pinMode(JOY1_B, INPUT_PULLUP);
  pinMode(JOY2_B, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  analogReadResolution(12); // Độ phân giải 12 bit (giá trị từ 0 đến 4095)

  // LoRa setup
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa init failed!");
    while (1);
  }
  Serial.println("✅ LoRa transmitter ready");

  // OLED setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Lora is OK");
}

void loop() {

  // Đọc giá trị từ chân ADC để tính toán điện áp pin
  int batteryValue = analogRead(batteryPin); // Đọc giá trị ADC
  float voltage = (batteryValue * referenceVoltage) / 4095.0;  // Tính điện áp từ giá trị ADC
  
  // Kiểm tra giá trị ADC
  Serial.print("ADC value: ");
  Serial.println(batteryValue);  // In giá trị ADC để kiểm tra

  // Cập nhật màn hình OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Hiển thị điện áp pin
  display.print("Voltage: ");
  display.print(voltage, 2);  // In điện áp với 2 chữ số thập phân
  display.println("V");

  // Đọc giá trị joystick
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);
  int joy1B = !digitalRead(JOY1_B);

  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);
  int joy2B = !digitalRead(JOY2_B);

  // Xử lý nút nhấn với debounce
  bool buttonRead = digitalRead(BTN3);
  if (buttonRead == LOW && lastButtonState == HIGH) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay) {
      btnState = !btnState;
      digitalWrite(LED, btnState ? HIGH : LOW);
      lastDebounceTime = currentTime;
    }
  }
  lastButtonState = buttonRead;

  String key3 = btnState ? "ON" : "OFF";

  // Hiển thị dữ liệu joystick lên OLED
  display.setCursor(0, 16);
  display.setTextSize(1);
  display.print("J1:X=");
  display.print(joy1X);
  display.print(" Y=");
  display.print(joy1Y);
  display.print(" B=");
  display.println(joy1B ? "1" : "0");

  display.print("J2:X=");
  display.print(joy2X);
  display.print(" Y=");
  display.print(joy2Y);
  display.print(" B=");
  display.println(joy2B ? "1" : "0");

  display.print("BTN3: ");
  display.println(key3);

  display.display(); // Cập nhật màn hình


  // Tạo thông điệp để gửi qua LoRa
  String message = "joy1Y:" + String(joy1Y) + " joy1X:" + String(joy1X) + " joy2X:" + String(joy2X);

  if (LoRa.beginPacket()) {
    LoRa.print(message);  // Send message
    LoRa.endPacket();  // End LoRa packet
    Serial.println("📤 Đã gửi: " + message);
  
    // Update OLED with success message
    display.setCursor(0, 48);
    display.println("Lora is OK");
    display.display();
  } else {
    display.setCursor(0, 48);
    display.println("Lora is Not Ok");
    display.display();
  }

  delay(500);  // Delay giữa các lần đọc và gửi
}
