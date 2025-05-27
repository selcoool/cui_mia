#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RF24.h>

// NRF24L01 pin cấu hình
#define CE_PIN 14
#define CSN_PIN 5
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// Joystick pin cấu hình
#define JOY2_X 35
#define JOY2_Y 34
#define JOY1_X 33
#define JOY1_Y 32

// OLED cấu hình
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // Khởi động OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Not found screen OLED");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  delay(500);

  // Khởi động NRF24L01
  bool radioStarted = radio.begin();
  bool chipConnected = radio.isChipConnected();

  display.clearDisplay();
  display.setCursor(0, 0);

  if (radioStarted && chipConnected) {
    Serial.println("✅ NRF24L01 initiated and connected OK");
    display.println("NRF24:OK");
  } else {
    Serial.println(" NRF24L01 has error and cannot connected !");
    display.println("NRF24:  Lỗi!");

    if (!radioStarted) {
      Serial.println(" radio.begin() thất bại");
      display.println("radio.begin() ");
    }

    if (!chipConnected) {
      Serial.println("Cann't connect to chip NRF");
      display.println("chip NRF ");
    }

    display.display();
    while (1); // Dừng lại nếu lỗi
  }

  // Thiết lập cấu hình truyền
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.stopListening();

  display.display();

  // In cấu hình NRF ra Serial
  radio.printDetails();
  delay(1000);
}

void loop() {
  // Đọc joystick
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);
  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);

  // Soạn dữ liệu
  char text[64];
  snprintf(text, sizeof(text), "q:%d w:%d e:%d f:%d", joy1X, joy1Y, joy2X, joy2Y);

  // Gửi dữ liệu qua NRF
  bool success = radio.write(text, strlen(text));

  // Hiển thị lên OLED
  display.clearDisplay();
  display.setCursor(0, 0);


  display.print("Jk 1:");
  display.print("X: "); display.print(joy1X);
  display.print(",Y: "); display.println(joy1Y);

  display.print("Jk 2:");
  display.print("X: "); display.print(joy2X);
  display.print(",Y: "); display.println(joy2Y);

  display.setCursor(0, 55);
  display.println(success ? "Sent Ok" : "Not yet sent");

  display.display();

  // Serial debug
  Serial.print("Gửi dữ liệu: ");
  Serial.println(text);
  Serial.println(success ?  "Gửi thành công!" : "Gửi thất bại!");
  
  delay(500);
}
