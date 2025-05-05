#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

// Motor pins (PWM)
#define MOTOR1 3  // Front Left
#define MOTOR2 5  // Front Right
#define MOTOR3 6  // Rear Left
#define MOTOR4 9  // Rear Right

// PID values
float Kp = 2.0, Ki = 0.0, Kd = 1.0;
float errorPitch, errorRoll;
float prevErrorPitch = 0, prevErrorRoll = 0;
float integralPitch = 0, integralRoll = 0;

unsigned long lastTime;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  mpu.begin();
  mpu.calcGyroOffsets();

  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  lastTime = millis();
}

void loop() {
  mpu.update();

  float pitch = mpu.getAngleX();
  float roll = mpu.getAngleY();

  // PID: pitch
  errorPitch = 0 - pitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - prevErrorPitch;
  float pidPitch = Kp * errorPitch + Ki * integralPitch + Kd * derivativePitch;
  prevErrorPitch = errorPitch;

  // PID: roll
  errorRoll = 0 - roll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - prevErrorRoll;
  float pidRoll = Kp * errorRoll + Ki * integralRoll + Kd * derivativeRoll;
  prevErrorRoll = errorRoll;

  // Base speed (hover)
  int baseSpeed = 130;  // Tùy chỉnh theo động cơ và trọng lượng

  // Tính tốc độ từng động cơ
  int m1 = constrain(baseSpeed + pidPitch + pidRoll, 0, 255);
  int m2 = constrain(baseSpeed + pidPitch - pidRoll, 0, 255);
  int m3 = constrain(baseSpeed - pidPitch + pidRoll, 0, 255);
  int m4 = constrain(baseSpeed - pidPitch - pidRoll, 0, 255);

  analogWrite(MOTOR1, m1);
  analogWrite(MOTOR2, m2);
  analogWrite(MOTOR3, m3);
  analogWrite(MOTOR4, m4);

  // Debug
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.print(roll);
  Serial.print(" | Motors: "); Serial.print(m1); Serial.print(", "); Serial.print(m2); Serial.print(", ");
  Serial.print(m3); Serial.print(", "); Serial.println(m4);

  delay(10); // Delay nhỏ để ổn định
}
