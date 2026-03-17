#include <Arduino.h>

// Chân PWM cho từng motor
int motor1Pin = 2;
int motor2Pin = 3;
int motor3Pin = 4;
int motor4Pin = 5;

// Tốc độ motor 0-255
int motorSpeed1 = 100;  // motor 1 chậm
int motorSpeed2 = 120;  // motor 2 nhanh hơn
int motorSpeed3 = 180;  // motor 3 nhanh hơn nữa
int motorSpeed4 = 120;  // motor 4 tối đa

void setup() {
  Serial.begin(115200);
  pinMode(motor1Pin, OUTPUT);
  pinMode(motor2Pin, OUTPUT);
  pinMode(motor3Pin, OUTPUT);
  pinMode(motor4Pin, OUTPUT);
  Serial.println("✅ 4 Motor PWM Test - Tốc độ khác nhau");
}

void loop() {
  // Gửi PWM cho từng motor
  analogWrite(motor1Pin, motorSpeed1);
  analogWrite(motor2Pin, motorSpeed2);
  analogWrite(motor3Pin, motorSpeed3);
  analogWrite(motor4Pin, motorSpeed4);

  // In ra Serial
  Serial.print("📊 Motor Speeds -> ");
  Serial.print("M1: "); Serial.print(motorSpeed1);
  Serial.print(" | M2: "); Serial.print(motorSpeed2);
  Serial.print(" | M3: "); Serial.print(motorSpeed3);
  Serial.print(" | M4: "); Serial.println(motorSpeed4);

  delay(500); // 0.5s
}
