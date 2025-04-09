#include <Wire.h>
#include <MPU6050.h>

// Chân PWM động cơ
int motorPin1 = 14;
int motorPin2 = 12;
int motorPin3 = 15;
int motorPin4 = 13;

// Chân joystick
int joyX1 = 34; // Pitch
int joyY1 = 35; // Roll
int joyX2 = 32; // Yaw
int joyY2 = 33; // Throttle

// PID cho Pitch và Roll
float Kp_pitch = 15.0, Ki_pitch = 0.0, Kd_pitch = 5.0;
float Kp_roll = 15.0, Ki_roll = 0.0, Kd_roll = 5.0;

float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// Biến MPU6050
int16_t ax, ay, az, gx, gy, gz;
float pitch, roll;

// Biến động cơ
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

// Khởi tạo MPU6050
MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Cấu hình PWM
  for (int i = 0; i < 4; i++) {
    ledcSetup(i, 1000, 8);
  }
  
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);
}

void loop() {
  // Đọc giá trị Joystick
  int pitchInput = map(analogRead(joyX1), 0, 4095, -30, 30);
  int rollInput = map(analogRead(joyY1), 0, 4095, -30, 30);
  int yawInput = map(analogRead(joyX2), 0, 4095, -20, 20);
  int throttle = map(analogRead(joyY2), 0, 4095, 50, 255);

  // Đọc dữ liệu từ MPU6050
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Tính toán góc Pitch và Roll
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // PID cho Pitch
  float errorPitch = -pitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - previousErrorPitch;
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // PID cho Roll
  float errorRoll = -roll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - previousErrorRoll;
  float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // Tính toán tốc độ động cơ
  motorSpeed1 = constrain(throttle + pitchInput + rollInput - yawInput + pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed2 = constrain(throttle - pitchInput + rollInput + yawInput - pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed3 = constrain(throttle + pitchInput - rollInput + yawInput + pidOutputPitch - pidOutputRoll, 0, 255);
  motorSpeed4 = constrain(throttle - pitchInput - rollInput - yawInput - pidOutputPitch - pidOutputRoll, 0, 255);

  // Debug thông tin
  Serial.print("Joystick: Pitch="); Serial.print(pitchInput);
  Serial.print(" Roll="); Serial.print(rollInput);
  Serial.print(" Yaw="); Serial.print(yawInput);
  Serial.print(" Throttle="); Serial.println(throttle);

  Serial.print("MPU: Pitch="); Serial.print(pitch);
  Serial.print(" Roll="); Serial.println(roll);

  Serial.print("Motor: M1="); Serial.print(motorSpeed1);
  Serial.print(" M2="); Serial.print(motorSpeed2);
  Serial.print(" M3="); Serial.print(motorSpeed3);
  Serial.print(" M4="); Serial.println(motorSpeed4);

  // Cập nhật PWM động cơ
  ledcWrite(0, motorSpeed1);
  ledcWrite(1, motorSpeed2);
  ledcWrite(2, motorSpeed3);
  ledcWrite(3, motorSpeed4);

  delay(20);
}
