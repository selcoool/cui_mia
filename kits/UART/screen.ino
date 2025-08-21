#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1   // Không dùng reset pin
#define SDA_PIN 21         // ESP32 SDA
#define SCL_PIN 22         // ESP32 SCL

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN); // Khởi tạo I2C

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Địa chỉ I2C 0x3C
    Serial.println(F("SSD1306 not found!"));
    for(;;); // Dừng chương trình
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Hello ESP32!");
  display.display();
}

void loop() {
  // Hiển thị text từ Serial
  if (Serial.available()) {
    String text = Serial.readStringUntil('\n');
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(text);
    display.display();
  }
}
