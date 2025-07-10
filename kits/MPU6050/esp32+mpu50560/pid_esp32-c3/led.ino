#define LED_PIN 8  // Đổi thành 2 nếu LED tích hợp nằm ở GPIO2

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // Bật LED
  delay(500);
  digitalWrite(LED_PIN, LOW);  // Tắt LED
  delay(500);
}
