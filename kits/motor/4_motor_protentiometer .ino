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
  int potValue = analogRead(POT_PIN);
  int motorSpeed = map(potValue, 0, 4095, 0, 255);

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    int commandInt = command.toInt();
    Serial.println(commandInt);
    moveCar(commandInt, motorSpeed);
  }

  delay(100);
}
