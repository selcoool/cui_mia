#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;
int motorPin1 = 25, motorPin2 = 26, motorPin3 = 27, motorPin4 = 14;

float ax, ay, az, pitch, roll;
int basePWM = 150;  // Tốc độ trung bình
int minPWM = 110, maxPWM = 255;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 không kết nối!");
    while (1);
  }
  Serial.println("✅ MPU6050 đã kết nối!");

  // Cấu hình PWM cho động cơ
  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);

  calibrateMotors();
  setMotorsPWM(basePWM, basePWM, basePWM, basePWM);
}

void loop() {
  updateMPU();

  // 🚀 Điều chỉnh tốc độ động cơ để giữ cân bằng
  int correctionFactor = 2; // Điều chỉnh độ nhạy
  int motor_truoc_trai = basePWM - (roll + pitch) * correctionFactor;
  int motor_truoc_phai = basePWM + (roll - pitch) * correctionFactor;
  int motor_sau_trai = basePWM - (roll - pitch) * correctionFactor;
  int motor_sau_phai = basePWM + (roll + pitch) * correctionFactor;

  setMotorsPWM(motor_truoc_trai, motor_truoc_phai, motor_sau_trai, motor_sau_phai);

  // 📊 Xuất dữ liệu lên Serial Plotter với label tiếng Việt
  Serial.print("Pitch:"); Serial.print(pitch);
  Serial.print("\tRoll:"); Serial.print(roll);
  Serial.print("\tmotor_truoc_trai:"); Serial.print(motor_truoc_trai);
  Serial.print("\tmotor_truoc_phai:"); Serial.print(motor_truoc_phai);
  Serial.print("\tmotor_sau_trai:"); Serial.print(motor_sau_trai);
  Serial.print("\tmotor_sau_phai:"); Serial.println(motor_sau_phai); // Xuống dòng

  delay(50);
}

// 🔧 Hiệu chỉnh động cơ ban đầu
void calibrateMotors() {
  for (int pwm = minPWM; pwm <= maxPWM; pwm += 5) {
    setMotorsPWM(pwm, pwm, pwm, pwm);
    delay(20);
  }
}

// ⚙️ Điều chỉnh tốc độ 4 động cơ
void setMotorsPWM(int pwm1, int pwm2, int pwm3, int pwm4) {
  pwm1 = constrain(pwm1, minPWM, maxPWM);
  pwm2 = constrain(pwm2, minPWM, maxPWM);
  pwm3 = constrain(pwm3, minPWM, maxPWM);
  pwm4 = constrain(pwm4, minPWM, maxPWM);

  ledcWrite(0, pwm1);
  ledcWrite(1, pwm2);
  ledcWrite(2, pwm3);
  ledcWrite(3, pwm4);
}

// 📡 Cập nhật giá trị từ MPU6050
void updateMPU() {
  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);
  ax = ax_raw / 16384.0;
  ay = ay_raw / 16384.0;
  az = az_raw / 16384.0;

  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
}
