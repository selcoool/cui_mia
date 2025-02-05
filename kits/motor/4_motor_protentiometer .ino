#define ENA 5      // Chân PWM điều khiển tốc độ động cơ
#define IN1 18     // Chân điều khiển động cơ 1
#define IN2 19     // Chân điều khiển động cơ 1
#define IN3 21     // Chân điều khiển động cơ 2
#define IN4 22     // Chân điều khiển động cơ 2
#define POT_PIN 34 // Chân đọc giá trị từ Potentiometer

void setup() {
  // Cấu hình các chân điều khiển động cơ
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT_PIN, INPUT);
}

void loop() {
  int potValue = analogRead(POT_PIN);  // Đọc giá trị từ potentiometer (0-1023 trên Arduino UNO)
  int motorSpeed = map(potValue, 0, 1023, 0, 255);  // Chuyển đổi thành giá trị PWM (0-255)

  // Điều khiển động cơ 1 quay theo chiều kim đồng hồ
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Điều khiển động cơ 2 quay theo chiều kim đồng hồ
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // Điều khiển tốc độ động cơ 1 thông qua PWM
  analogWrite(ENA, motorSpeed);  // Điều chỉnh tốc độ động cơ 1 (PWM)

  // Điều khiển tốc độ động cơ 2 thông qua PWM (nếu có cần điều chỉnh cho động cơ 2)
  // analogWrite(ENB, motorSpeed);  // Nếu có một chân PWM riêng cho động cơ 2, sử dụng analogWrite cho nó

  delay(100);  // Chờ một chút trước khi đọc lại giá trị
}

