#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

int motorPin1 = 3, motorPin2 = 5, motorPin3 = 6, motorPin4 = 9;

float ax, ay, az, pitch, roll;
int basePWM = 150;
int minPWM = 110, maxPWM = 255;

int errorCount = 0;
const int maxErrorThreshold = 50;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 không kết nối!");
    while (1);
  }

  Serial.println("✅ MPU6050 đã kết nối!");

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  calibrateMotors();
  setMotorsPWM(basePWM, basePWM, basePWM, basePWM);
}

void loop() {
  if (!updateMPU()) {
    errorCount++;
    Serial.println("⚠️ Dữ liệu MPU không hợp lệ, bỏ qua vòng lặp.");
    if (errorCount >= maxErrorThreshold) {
      Serial.println("🔁 Quá nhiều lỗi liên tục, đang reset hệ thống...");
      delay(1000);
      asm volatile ("  jmp 0");  // Reset phần mềm cho Arduino Uno/Nano
    }
    delay(10);
    return;
  }

  errorCount = 0; // Reset bộ đếm nếu dữ liệu hợp lệ

  int correctionFactor = 2;
  int motor_truoc_trai = basePWM - (roll + pitch) * correctionFactor;
  int motor_truoc_phai = basePWM + (roll - pitch) * correctionFactor;
  int motor_sau_trai = basePWM - (roll - pitch) * correctionFactor;
  int motor_sau_phai = basePWM + (roll + pitch) * correctionFactor;

  motor_truoc_trai = constrain(motor_truoc_trai, minPWM, maxPWM);
  motor_truoc_phai = constrain(motor_truoc_phai, minPWM, maxPWM);
  motor_sau_trai = constrain(motor_sau_trai, minPWM, maxPWM);
  motor_sau_phai = constrain(motor_sau_phai, minPWM, maxPWM);

  setMotorsPWM(motor_truoc_trai, motor_truoc_phai, motor_sau_trai, motor_sau_phai);

  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print("\tRoll: "); Serial.print(roll);
  Serial.print("\tM1: "); Serial.print(motor_truoc_trai);
  Serial.print("\tM2: "); Serial.print(motor_truoc_phai);
  Serial.print("\tM3: "); Serial.print(motor_sau_trai);
  Serial.print("\tM4: "); Serial.println(motor_sau_phai);

  delay(50);
}

void calibrateMotors() {
  for (int pwm = minPWM; pwm <= maxPWM; pwm += 5) {
    setMotorsPWM(pwm, pwm, pwm, pwm);
    delay(20);
  }
}

void setMotorsPWM(int pwm1, int pwm2, int pwm3, int pwm4) {
  analogWrite(motorPin1, pwm1);
  analogWrite(motorPin2, pwm2);
  analogWrite(motorPin3, pwm3);
  analogWrite(motorPin4, pwm4);
}

bool updateMPU() {
  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

  ax = ax_raw / 16384.0;
  ay = ay_raw / 16384.0;
  az = az_raw / 16384.0;

  float magnitude = sqrt(ax * ax + ay * ay + az * az);

  if (magnitude < 0.1 || isnan(magnitude)) {
    return false;  // Dữ liệu lỗi → bỏ qua
  }

  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  return true;
}
