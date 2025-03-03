#define ENA 5      // Chân PWM điều khiển tốc độ động cơ
#define IN1 18     // Chân điều khiển động cơ 1
#define IN2 19     // Chân điều khiển động cơ 1
#define IN3 21     // Chân điều khiển động cơ 2
#define IN4 22     // Chân điều khiển động cơ 2
#define POT_PIN 34 // Chân đọc giá trị từ Potentiometer

#define PWM_CHANNEL 0  // Kênh PWM
#define PWM_FREQ 5000  // Tần số PWM (Hz)
#define PWM_RESOLUTION 8  // Độ phân giải (8-bit, giá trị từ 0-255)


#define FORWARD 8
#define FORBACK 2
#define LEFT 4
#define RIGHT 6
#define STOP 5

void setup() {
  Serial.begin(115200);

  // Cấu hình các chân điều khiển động cơ
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT_PIN, INPUT);

  // Cấu hình PWM cho chân ENA
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL);
}

void moveCar(int motorDirection) {
  if (motorDirection == FORWARD) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (motorDirection == FORBACK) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }else if (motorDirection == LEFT) {
      digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
   } 
   else if (motorDirection == RIGHT) {
       digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
   }

    else if (motorDirection == STOP) {
       digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
   }

   
   else {  // Dừng động cơ
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

void loop() {
  int potValue = analogRead(POT_PIN);  // Đọc giá trị từ potentiometer (0-4095 trên ESP32)
  int motorSpeed = map(potValue, 0, 4095, 0, 255);  // Chuyển đổi thành giá trị PWM (0-255)

  if (Serial.available()) {  // Kiểm tra nếu có dữ liệu từ Serial
    String command = Serial.readStringUntil('\n');  // Đọc toàn bộ lệnh
    command.trim();  // Xóa khoảng trắng thừa

    int commandInt = command.toInt();  // Chuyển thành số nguyên
    Serial.println(commandInt);  // In ra để kiểm tra
    moveCar(commandInt);
  }

  ledcWrite(PWM_CHANNEL, motorSpeed);  // Điều chỉnh tốc độ động cơ
  delay(100);  // Chờ một chút trước khi đọc lại giá trị
}
