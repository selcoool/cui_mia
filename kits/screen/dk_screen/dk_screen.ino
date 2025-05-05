#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pin setup
#define BUTTON_UP 4
#define BUTTON_DOWN 5
#define BUTTON_SELECT 12
#define LED_PIN 2

// Menu
int menuIndex = 0;
const int menuItemCount = 5;

bool ledState = false;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
unsigned long blinkDuration = 0;

bool lastState[3] = {HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[3] = {0, 0, 0};
const unsigned long debounceDelay = 50;

void setup() {
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (1);
  }

  display.clearDisplay();
  drawMenu();
}

void loop() {
  handleButton(BUTTON_UP, 0);   // Nút điều khiển lên
  handleButton(BUTTON_DOWN, 1); // Nút điều khiển xuống
  handleButton(BUTTON_SELECT, 2); // Nút chọn

  if (isBlinking && millis() - blinkStartTime >= blinkDuration) {
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    isBlinking = false;
    drawMenu();
  }
}

void handleButton(int buttonPin, int index) {
  bool reading = digitalRead(buttonPin);
  if (reading == LOW && lastState[index] == HIGH) {
    unsigned long now = millis();
    if (now - lastDebounceTime[index] > debounceDelay) {
      if (buttonPin == BUTTON_UP) {
        menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;  // Điều khiển lên
      } else if (buttonPin == BUTTON_DOWN) {
        menuIndex = (menuIndex + 1) % menuItemCount;  // Điều khiển xuống
      } else if (buttonPin == BUTTON_SELECT) {
        switch (menuIndex) {
          case 0: ledState = false; isBlinking = false; digitalWrite(LED_PIN, LOW); break;
          case 1: ledState = true; isBlinking = false; digitalWrite(LED_PIN, HIGH); break;
          case 2: ledState = true; isBlinking = true; blinkDuration = 5000; blinkStartTime = millis(); digitalWrite(LED_PIN, HIGH); break;
          case 3: ledState = true; isBlinking = true; blinkDuration = 10000; blinkStartTime = millis(); digitalWrite(LED_PIN, HIGH); break;
          case 4: ledState = true; isBlinking = true; blinkDuration = 20000; blinkStartTime = millis(); digitalWrite(LED_PIN, HIGH); break;
        }
      }
      drawMenu();
      lastDebounceTime[index] = now;
    }
  }
  lastState[index] = reading;
}

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.println(menuIndex == 0 ? "> LED OFF" : "  LED OFF");
  display.setCursor(0, 12);
  display.println(menuIndex == 1 ? "> LED ON" : "  LED ON");
  display.setCursor(0, 24);
  display.println(menuIndex == 2 ? "> BLINK 5s" : "  BLINK 5s");
  display.setCursor(0, 36);
  display.println(menuIndex == 3 ? "> BLINK 10s" : "  BLINK 10s");
  display.setCursor(0, 48);
  display.println(menuIndex == 4 ? "> BLINK 20s" : "  BLINK 20s");

  display.setCursor(0, 58);
  display.print("LED: ");
  if (ledState) {
    display.print("ON");
    if (isBlinking) display.print(" (Timer)");
  } else {
    display.print("OFF");
  }

  display.display();
}
