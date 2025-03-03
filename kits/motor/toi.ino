const int motorPin1 = 2;   // IN1 của L298N
const int motorPin2 = 14;  // IN2 của L298N
const int motorPin3 = 15;  // IN3 của L298N
const int motorPin4 = 13;  // IN4 của L298N

#define ENA 12      // Chân PWM điều khiển tốc độ động cơ

void setup() {
  Serial.begin(115200);

  // Cấu hình các chân điều khiển động cơ
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  pinMode(ENA, OUTPUT);  // Chân ENA cần được cấu hình để điều khiển PWM

  // Khởi tạo PWM cho ENA
  ledcSetup(0, 5000, 8);  // Tần số 5000 Hz, độ phân giải 8 bit
  ledcAttachPin(ENA, 0);  // Gắn chân PWM vào kênh 0
}

void loop() {

 
      // Chạy động cơ về phía trước
      digitalWrite(motorPin1, HIGH);
      digitalWrite(motorPin2, LOW);
      digitalWrite(motorPin3, HIGH);
      digitalWrite(motorPin4, LOW);
      ledcWrite(0, 255);  // Tốc độ tối đa


  delay(100);  // Chờ một chút trước khi đọc lại giá trị

}


