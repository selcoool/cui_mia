const int buttonPin = 15;  // Chân nút
const int ledPin = 2;      // Chân LED

bool lastButtonState = HIGH; // Trạng thái nút lần trước (mặc định HIGH với INPUT_PULLUP)
bool ledState = false;       // Trạng thái LED hiện tại

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP); // Sử dụng nội bộ pull-up cho nút
  pinMode(ledPin, OUTPUT);          // Cấu hình chân LED làm OUTPUT
  digitalWrite(ledPin, ledState);   // Đảm bảo LED ban đầu tắt
}

void loop() {
  bool currentButtonState = digitalRead(buttonPin);

  // Nếu nút chuyển từ không nhấn (HIGH) sang nhấn (LOW)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Toggle trạng thái LED
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    
    // In thông báo trên Serial Monitor
    Serial.println(ledState ? "LED ON" : "LED OFF");
    
    // Chờ một chút để tránh hiện tượng rung (debounce)
    delay(50);
  }

  // Cập nhật trạng thái nút lần trước
  lastButtonState = currentButtonState;
}
