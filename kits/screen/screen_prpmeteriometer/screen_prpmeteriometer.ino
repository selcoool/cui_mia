#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Joystick 1
#define JOY1_X 33
#define JOY1_Y 32
#define JOY1_B 27

// Joystick 2
#define JOY2_X 35
#define JOY2_Y 34
#define JOY2_B 25

void setup() {
  Serial.begin(115200);

  pinMode(JOY1_B, INPUT_PULLUP);
  pinMode(JOY2_B, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed!");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void loop() {
  // Đọc joystick 1
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);
  bool joy1Btn = digitalRead(JOY1_B) == LOW;

  // Đọc joystick 2
  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);
  bool joy2Btn = digitalRead(JOY2_B) == LOW;

  // Hiển thị lên màn hình
  display.clearDisplay();
  display.setCursor(0, 0);

    display.println(); // Dòng trống

    display.println("Connected");
  display.println("Joystick 1:");
  display.print("X: "); display.print(joy1X);
  display.print("  Y: "); display.println(joy1Y);
  display.print("Btn: ");
  display.println(joy1Btn ? "Pressed" : "Released");

  display.println(); // Dòng trống

  display.println("Joystick 2:");
  display.print("X: "); display.print(joy2X);
  display.print("  Y: "); display.println(joy2Y);
  display.print("Btn: ");
  display.println(joy2Btn ? "Pressed" : "Released");

  display.display();

  delay(100); // Cập nhật mỗi 100ms
}
