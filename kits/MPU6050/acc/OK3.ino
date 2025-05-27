#include <Wire.h>
#include <MPU6050.h>

// Định nghĩa chân PWM cho động cơ
int motorPin1 = 14;
int motorPin2 = 12;
int motorPin3 = 25;
int motorPin4 = 26;

// Khởi tạo đối tượng MPU6050
MPU6050 mpu;

// Tham số PID cho Pitch
float Kp_pitch = 2.0;
float Ki_pitch = 0.0;
float Kd_pitch = 0.0;

// Tham số PID cho Roll
float Kp_roll = 2.0;
float Ki_roll = 0.0;
float Kd_roll = 0.0;

// Biến PID Pitch
float previousErrorPitch = 0;
float integralPitch = 0;

// Biến PID Roll
float previousErrorRoll = 0;
float integralRoll = 0;

// Dữ liệu từ MPU6050
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch, roll;

// Tốc độ các động cơ
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // Thiết lập các kênh PWM
  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  // Gán chân PWM cho kênh PWM
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);
}

void loop() {
  // Đọc dữ liệu từ MPU6050
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Tính toán góc Pitch và Roll, kiểm tra mẫu số
  float denominatorPitch = sqrt(ax * ax + az * az);
  float denominatorRoll  = sqrt(ay * ay + az * az);

  if (denominatorPitch > 0.0001 && denominatorRoll > 0.0001) {
    pitch = atan2(ay, denominatorPitch) * 180.0 / PI;
    roll  = atan2(-ax, denominatorRoll) * 180.0 / PI;
  } else {
    pitch = 0;
    roll = 0;
    Serial.println("Warning: Denominator near zero, skipping pitch/roll calculation");
  }

  // PID cho Pitch
  float errorPitch = 0 - pitch;
  float derivativePitch = errorPitch - previousErrorPitch;

  // PID cho Roll
  float errorRoll = 0 - roll;
  float derivativeRoll = errorRoll - previousErrorRoll;

  // Tính toán tốc độ tạm thời để kiểm tra điều kiện
  float pidTempPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  float pidTempRoll  = Kp_roll  * errorRoll  + Ki_roll  * integralRoll  + Kd_roll  * derivativeRoll;

  int tempM1 = constrain(128 + pidTempPitch + pidTempRoll, 0, 255);
  int tempM2 = constrain(128 - pidTempPitch + pidTempRoll, 0, 255);
  int tempM3 = constrain(128 + pidTempPitch - pidTempRoll, 0, 255);
  int tempM4 = constrain(128 - pidTempPitch - pidTempRoll, 0, 255);

  // Cập nhật tích phân nếu tốc độ nằm trong khoảng an toàn
  if (tempM1 > 0 && tempM1 < 255 &&
      tempM2 > 0 && tempM2 < 255 &&
      tempM3 > 0 && tempM3 < 255 &&
      tempM4 > 0 && tempM4 < 255) {
    integralPitch += errorPitch;
    integralPitch = constrain(integralPitch, -400, 400);

    integralRoll += errorRoll;
    integralRoll = constrain(integralRoll, -400, 400);
  }

  // Tính PID đầu ra
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  float pidOutputRoll  = Kp_roll  * errorRoll  + Ki_roll  * integralRoll  + Kd_roll  * derivativeRoll;

  previousErrorPitch = errorPitch;
  previousErrorRoll = errorRoll;

  // Tính tốc độ động cơ
  motorSpeed1 = constrain(128 + pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed2 = constrain(128 - pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed3 = constrain(128 + pidOutputPitch - pidOutputRoll, 0, 255);
  motorSpeed4 = constrain(128 - pidOutputPitch - pidOutputRoll, 0, 255);

  // Gửi tín hiệu PWM
  ledcWrite(0, motorSpeed1);
  ledcWrite(1, motorSpeed2);
  ledcWrite(2, motorSpeed3);
  ledcWrite(3, motorSpeed4);





    // In dữ liệu có chú thích cho Serial Monitor
// Serial.print("Pitch: "); Serial.print(pitch);
// Serial.print("\tRoll: "); Serial.print(roll);
// Serial.print("\tIntegralPitch: "); Serial.print(integralPitch);
// Serial.print("\tIntegralRoll: "); Serial.print(integralRoll);
Serial.print("\tM1: "); Serial.print(motorSpeed1);
Serial.print("\tM2: "); Serial.print(motorSpeed2);
Serial.print("\tM3: "); Serial.print(motorSpeed3);
Serial.print("\tM4: "); Serial.println(motorSpeed4);

// In dữ liệu dạng số cho Serial Plotter
// Serial.print(pitch); Serial.print("\t");
// Serial.print(roll); Serial.print("\t");
// Serial.print(integralPitch); Serial.print("\t");
// Serial.print(integralRoll); Serial.print("\t");
Serial.print(motorSpeed1); Serial.print("\t");
Serial.print(motorSpeed2); Serial.print("\t");
Serial.print(motorSpeed3); Serial.print("\t");
Serial.println(motorSpeed4);

  delay(20);
}

