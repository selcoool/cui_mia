#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Chân PWM cho động cơ (dùng chân PWM trên Arduino Nano: 3, 5, 6, 9, 10, 11)
int motorPin1 = 3, motorPin2 = 5, motorPin3 = 6, motorPin4 = 9;

float ax, ay, az, pitch, roll;
int basePWM = 150;  // Tốc độ trung bình
int minPWM = 110, maxPWM = 255;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 không kết nối!");
    while (1);  // Dừng nếu không kết nối được
  }
  Serial.println("✅ MPU6050 đã kết nối!");

  // Cấu hình chân PWM
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

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

  // Giới hạn PWM trong khoảng từ 0 đến 255
  motor_truoc_trai = constrain(motor_truoc_trai, minPWM, maxPWM);
  motor_truoc_phai = constrain(motor_truoc_phai, minPWM, maxPWM);
  motor_sau_trai = constrain(motor_sau_trai, minPWM, maxPWM);
  motor_sau_phai = constrain(motor_sau_phai, minPWM, maxPWM);

  // Cập nhật PWM động cơ
  setMotorsPWM(motor_truoc_trai, motor_truoc_phai, motor_sau_trai, motor_sau_phai);

  // 📊 Xuất dữ liệu lên Serial Monitor
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print("\tRoll: "); Serial.print(roll);
  Serial.print("\tmotor_truoc_trai: "); Serial.print(motor_truoc_trai);
  Serial.print("\tmotor_truoc_phai: "); Serial.print(motor_truoc_phai);
  Serial.print("\tmotor_sau_trai: "); Serial.print(motor_sau_trai);
  Serial.print("\tmotor_sau_phai: "); Serial.println(motor_sau_phai);

  delay(50);  // Dừng 50ms trước khi đọc lại
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
  analogWrite(motorPin1, pwm1);
  analogWrite(motorPin2, pwm2);
  analogWrite(motorPin3, pwm3);
  analogWrite(motorPin4, pwm4);
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
