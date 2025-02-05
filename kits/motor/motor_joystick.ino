#define ENA 5      // Chân PWM điều khiển tốc độ động cơ
#define IN1 18     // Chân điều khiển động cơ (IN1)
#define IN2 19     // Chân điều khiển động cơ (IN2)
#define JOY_X_PIN 34 // Chân đọc giá trị từ trục X của Joystick (X-axis)
#define JOY_Y_PIN 35 // Chân đọc giá trị từ trục Y của Joystick (Y-axis)

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(JOY_X_PIN, INPUT);
  pinMode(JOY_Y_PIN, INPUT);
  Serial.begin(115200);  // Để giám sát giá trị từ joystick
}

void loop() {
  // Đọc giá trị từ Joystick (0-4095)
  int joyX = analogRead(JOY_X_PIN); // Trục X để điều khiển hướng
  int joyY = analogRead(JOY_Y_PIN); // Trục Y để điều khiển tốc độ

  // Kiểm tra nếu joystick không di chuyển (giá trị gần trung tâm)
  if (abs(joyX - 2970) < 300 && abs(joyY - 3120) < 300) {  // Khi joystick ở gần trung tâm
    // Dừng động cơ khi không có tác động
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);  // Dừng động cơ
    analogWrite(ENA, 0);      // Dừng PWM (tốc độ = 0)
  }
  else {
    // Điều khiển chiều quay của động cơ bằng trục X
    if (joyX < 1500) { // Nếu joystick di chuyển sang trái
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    } else if (joyX > 2500) { // Nếu joystick di chuyển sang phải
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } else { // Nếu joystick ở gần trung tâm, giữ trạng thái dừng
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }

    // Điều khiển tốc độ động cơ thông qua PWM từ trục Y
    int motorSpeed = map(joyY, 0, 4095, 0, 255); // Chuyển giá trị Y thành giá trị PWM
    analogWrite(ENA, motorSpeed);  // Điều chỉnh tốc độ động cơ
  }

  // Hiển thị giá trị trên Serial Monitor
  Serial.print("Joy X: ");
  Serial.print(joyX);
  Serial.print(" | Joy Y: ");
  Serial.println(joyY);

  delay(100);  // Chờ một chút trước khi đọc lại giá trị
}
