#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// NOTE: Khai báo kích thước màn hình OLED và tạo đối tượng màn hình
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// NOTE: Gán chân nút nhấn và LED
#define BUTTON_UP 4
#define BUTTON_DOWN 5
#define BUTTON_SELECT 12
#define LED_PIN 2

// NOTE: Các biến liên quan đến menu chính và điều khiển LED
int menuIndex = 0;
const int menuItemCount = 5;

bool ledState = false;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
unsigned long blinkDuration = 0;

// NOTE: Biến để chống rung nút
bool lastState[3] = {HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[3] = {0, 0, 0};
const unsigned long debounceDelay = 50;

// NOTE: Quản lý trạng thái menu phụ (Hello World)
bool isInHelloWorldMenu = false;
bool isShowingSubContent = false;
int helloWorldSubIndex = 0;
const int helloWorldSubCount = 3;

void setup() {
  // NOTE: Cấu hình chân I/O
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);

  // NOTE: Khởi động màn hình OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed!");
    while (1);
  }

  display.clearDisplay();
  drawMainMenu();  // NOTE: Hiển thị menu chính ban đầu
}

void loop() {
  if (isShowingSubContent) {
    handleReturnButton(BUTTON_SELECT, 2);  // NOTE: Chỉ xử lý SELECT khi đang xem nội dung Hello World
  } else if (isInHelloWorldMenu) {
    handleHelloWorldMenu();  // NOTE: Đang ở menu Hello World
  } else {
    handleMainMenu();  // NOTE: Đang ở menu chính
  }

  // NOTE: Kiểm tra thời gian để tắt LED khi nháy
  if (isBlinking && millis() - blinkStartTime >= blinkDuration) {
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    isBlinking = false;
    drawMainMenu();
  }
}

void handleMainMenu() {
  // NOTE: Nút UP
  handleButton(BUTTON_UP, 0, [] {
    menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
    drawMainMenu();
  });

  // NOTE: Nút DOWN
  handleButton(BUTTON_DOWN, 1, [] {
    menuIndex = (menuIndex + 1) % menuItemCount;
    drawMainMenu();
  });

  // NOTE: Nút SELECT
  handleButton(BUTTON_SELECT, 2, [] {
    switch (menuIndex) {
      case 0: ledState = false; isBlinking = false; digitalWrite(LED_PIN, LOW); break;
      case 1: ledState = true;  isBlinking = false; digitalWrite(LED_PIN, HIGH); break;
      case 2: ledState = true;  isBlinking = true;  blinkDuration = 5000; blinkStartTime = millis(); digitalWrite(LED_PIN, HIGH); break;
      case 3: ledState = true;  isBlinking = true;  blinkDuration = 10000; blinkStartTime = millis(); digitalWrite(LED_PIN, HIGH); break;
      case 4:  // NOTE: Mở menu Hello World
        isInHelloWorldMenu = true;
        helloWorldSubIndex = 0;
        drawHelloWorldMenu();
        return;
    }
    drawMainMenu();
  });
}

void handleHelloWorldMenu() {
  // NOTE: Nút UP trong menu phụ
  handleButton(BUTTON_UP, 0, [] {
    helloWorldSubIndex = (helloWorldSubIndex - 1 + helloWorldSubCount) % helloWorldSubCount;
    drawHelloWorldMenu();
  });

  // NOTE: Nút DOWN trong menu phụ
  handleButton(BUTTON_DOWN, 1, [] {
    helloWorldSubIndex = (helloWorldSubIndex + 1) % helloWorldSubCount;
    drawHelloWorldMenu();
  });

  // NOTE: Nút SELECT trong menu phụ
  handleButton(BUTTON_SELECT, 2, [] {
    if (helloWorldSubIndex == 0) {
      showHelloWorld("Hello World 1");
    } else if (helloWorldSubIndex == 1) {
      showHelloWorld("Hello World 2");
    } else {
      isInHelloWorldMenu = false;
      drawMainMenu();
    }
  });
}

void handleReturnButton(int buttonPin, int index) {
  bool reading = digitalRead(buttonPin);
  if (reading == LOW && lastState[index] == HIGH) {
    unsigned long now = millis();
    if (now - lastDebounceTime[index] > debounceDelay) {
      isShowingSubContent = false;
      drawHelloWorldMenu();  // NOTE: Quay lại menu Hello World
      lastDebounceTime[index] = now;
    }
  }
  lastState[index] = reading;
}

// NOTE: Hàm xử lý nút có chống rung và truyền hành động (callback)
void handleButton(int buttonPin, int index, void (*onPress)()) {
  bool reading = digitalRead(buttonPin);
  if (reading == LOW && lastState[index] == HIGH) {
    unsigned long now = millis();
    if (now - lastDebounceTime[index] > debounceDelay) {
      onPress();
      lastDebounceTime[index] = now;
    }
  }
  lastState[index] = reading;
}

// NOTE: Vẽ giao diện menu chính
void drawMainMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  const char* items[] = {"LED OFF", "LED ON", "BLINK 5s", "BLINK 10s", "Hello world"};
  for (int i = 0; i < menuItemCount; i++) {
    display.setCursor(0, i * 12);
    display.println(i == menuIndex ? (String("> ") + items[i]) : ("  " + String(items[i])));
  }

  display.setCursor(0, 58);
  display.print("LED: ");
  display.print(ledState ? "ON" : "OFF");
  if (ledState && isBlinking) display.print(" (Timer)");

  display.display();
}

// NOTE: Vẽ giao diện menu con Hello World
void drawHelloWorldMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  const char* items[] = {"Hello World 1", "Hello World 2", "Back"};
  for (int i = 0; i < helloWorldSubCount; i++) {
    display.setCursor(0, i * 12);
    display.println(i == helloWorldSubIndex ? (String("> ") + items[i]) : ("  " + String(items[i])));
  }

  display.display();
}

// NOTE: Hiển thị nội dung Hello World
void showHelloWorld(const char* text) {
  isShowingSubContent = true;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println(text);

  display.setCursor(10, 40);
  display.println("Press SELECT to return");
  display.display();
}
