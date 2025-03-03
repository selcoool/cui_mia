#define ENA 5      // Chân PWM điều khiển tốc độ động cơ
#define IN1 18     // Chân điều khiển động cơ
#define IN2 19     // Chân điều khiển động cơ
#define POT_PIN 34 // Chân đọc giá trị từ Potentiometer

void setup() {
    Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(POT_PIN, INPUT);
}

void loop() {
  int potValue = analogRead(POT_PIN); // Đọc giá trị từ potentiometer (0-4095)
  int motorSpeed = map(potValue, 0, 4095, 0, 255); // Chuyển đổi thành giá trị PWM (0-255)

  // Điều khiển động cơ quay theo chiều kim đồng hồ
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Điều khiển tốc độ động cơ thông qua PWM
  analogWrite(ENA, motorSpeed);  // Điều chỉnh tốc độ động cơ (PWM)

  // delay(100);  // Chờ một chút trước khi đọc lại giá trị
}
