
Option 1: OLED Display (SSD1306, 128x64)
Connections (I2C - 4 pins):

OLED	ESP32
VCC	3.3V
GND	GND
SCL	GPIO22
SDA	GPIO21

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // I2C address
  display.clearDisplay();

  display.setTextSize(1);         // Cỡ chữ nhỏ
  display.setTextColor(WHITE);    // Màu chữ trắng
  display.setCursor(0, 0);

  display.println("Dieu khien:");
  display.println("Cap canh");
  display.println("Ha canh");
  display.println("");
  display.println("Tien  Lui");
  display.println("Trai  Phai");

  display.display();
}

void loop() {
  // Không cần gì thêm nếu chỉ hiển thị tĩnh
}
