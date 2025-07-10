#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Chân PWM điều khiển động cơ
// Định nghĩa chân PWM cho động cơ
int motorPin1 = 2; // Chân PWM cho động cơ 1
int motorPin2 = 3; // Chân PWM cho động cơ 2
int motorPin3 = 6; // Chân PWM cho động cơ 3
int motorPin4 = 7; // Chân PWM cho động cơ 4

// PID parameters
float Kp_pitch = 5.0, Ki_pitch = 0.0, Kd_pitch = 5.0;
float Kp_roll = 5.0, Ki_roll = 0.0, Kd_roll = 5.0;

float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// Biến đọc từ MPU6050
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch, roll;

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5); // SDA = GPIO4, SCL = GPIO5 (ESP32-C3)

  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful.");
  } else {
    Serial.println("MPU6050 connection failed.");
  }

  // Thiết lập kênh PWM
  ledcSetup(0, 1000, 8); // kênh 0
  ledcSetup(1, 1000, 8); // kênh 1
  ledcSetup(2, 1000, 8); // kênh 2
  ledcSetup(3, 1000, 8); // kênh 3

  // Gán chân vào kênh
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);
}

void loop() {
  // Đọc dữ liệu từ MPU6050
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Tính pitch và roll (đơn vị độ)
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // PID cho pitch
  float errorPitch = 0 - pitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - previousErrorPitch;
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // PID cho roll
  float errorRoll = 0 - roll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - previousErrorRoll;
  float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // Tính tốc độ động cơ (giới hạn từ 0–255)
  int motorSpeed1 = constrain(128 + pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed2 = constrain(128 - pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed3 = constrain(128 + pidOutputPitch - pidOutputRoll, 0, 255);
  int motorSpeed4 = constrain(128 - pidOutputPitch - pidOutputRoll, 0, 255);

  // Ghi ra Serial Plotter (tab-separated format)
  Serial.print("Pitch:"); Serial.print(pitch); Serial.print("\t");
  Serial.print("Roll:"); Serial.print(roll); Serial.print("\t");
  Serial.print("Motor1:"); Serial.print(motorSpeed1); Serial.print("\t");
  Serial.print("Motor2:"); Serial.print(motorSpeed2); Serial.print("\t");
  Serial.print("Motor3:"); Serial.print(motorSpeed3); Serial.print("\t");
  Serial.print("Motor4:"); Serial.println(motorSpeed4);

  // Ghi tín hiệu PWM đến động cơ
  ledcWrite(0, motorSpeed1);
  ledcWrite(1, motorSpeed2);
  ledcWrite(2, motorSpeed3);
  ledcWrite(3, motorSpeed4);

  delay(100); // Giảm xuống để Serial Plotter vẽ mượt hơn
}
