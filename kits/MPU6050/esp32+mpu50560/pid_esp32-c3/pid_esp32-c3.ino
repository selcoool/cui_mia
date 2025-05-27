#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;


// Định nghĩa chân PWM cho động cơ
int motorPin1 = 2; // Chân PWM cho động cơ 1
int motorPin2 = 3; // Chân PWM cho động cơ 2
int motorPin3 = 6; // Chân PWM cho động cơ 3
int motorPin4 = 7; // Chân PWM cho động cơ 4


// Các tham số PID cho pitch và roll
float Kp_pitch = 5.0;  // Hệ số P cho pitch
float Ki_pitch = 0.0;   // Hệ số I cho pitch
float Kd_pitch = 5.0;   // Hệ số D cho pitch
float Kp_roll = 5.0;   // Hệ số P cho roll
float Ki_roll = 0.0;    // Hệ số I cho roll
float Kd_roll = 5.0;    // Hệ số D cho roll

float previousErrorPitch = 0;
float integralPitch = 0;
float previousErrorRoll = 0;
float integralRoll = 0;

// Các biến cho cảm biến MPU6050
int16_t ax, ay, az;  // Gia tốc (thay kiểu dữ liệu thành int16_t)
int16_t gx, gy, gz;  // Tốc độ quay (thay kiểu dữ liệu thành int16_t)
float pitch, roll;   // Góc nghiêng (vẫn giữ kiểu dữ liệu float)

void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5); // SDA = 4, SCL = 5 for ESP32-C3
  Serial.println("Initializing MPU6050...");

  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful.");
  } else {
    Serial.println("MPU6050 connection failed.");
  }


  // Cấu hình các kênh PWM với tần số 1000Hz và độ phân giải 8 bit
  ledcSetup(0, 1000, 8);  // Kênh 0, tần số 1000Hz, độ phân giải 8 bit
  ledcSetup(1, 1000, 8);  // Kênh 1, tần số 1000Hz, độ phân giải 8 bit
  ledcSetup(2, 1000, 8);  // Kênh 2, tần số 1000Hz, độ phân giải 8 bit
  ledcSetup(3, 1000, 8);  // Kênh 3, tần số 1000Hz, độ phân giải 8 bit

  // Gắn các chân PWM vào các kênh PWM
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);
}

void loop() {
  // int16_t ax, ay, az;
  // int16_t gx, gy, gz;
   mpu.getAcceleration(&ax, &ay, &az);  // Đọc gia tốc
  mpu.getRotation(&gx, &gy, &gz);      // Đọc tốc độ quay

  // Tính toán góc Pitch và Roll từ gia tốc
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // In giá trị góc để theo dõi
  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print("\tRoll: ");
  Serial.println(roll);

   // Tính toán lỗi PID cho Pitch
  float errorPitch = 0 - pitch;  // Lỗi giữa góc mục tiêu và góc hiện tại
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - previousErrorPitch;
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // Tính toán lỗi PID cho Roll
  float errorRoll = 0 - roll;  // Lỗi giữa góc mục tiêu và góc hiện tại
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - previousErrorRoll;
  float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // Điều chỉnh tốc độ động cơ dựa trên kết quả PID
  int motorSpeed1 = constrain(128 + pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed2 = constrain(128 - pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed3 = constrain(128 + pidOutputPitch - pidOutputRoll, 0, 255);
  int motorSpeed4 = constrain(128 - pidOutputPitch - pidOutputRoll, 0, 255);

  // In giá trị PWM ra Serial Monitor để theo dõi
// In giá trị PWM ra Serial Monitor để theo dõi
Serial.print("Motor Speed 1: "); Serial.print(motorSpeed1); Serial.print(" | ");
Serial.print("Motor Speed 2: "); Serial.print(motorSpeed2); Serial.print(" | ");
Serial.print("Motor Speed 3: "); Serial.print(motorSpeed3); Serial.print(" | ");
Serial.print("Motor Speed 4: "); Serial.println(motorSpeed4);

  // Cập nhật tốc độ động cơ bằng PWM
  ledcWrite(0, motorSpeed1);  // Điều khiển động cơ 1
  ledcWrite(1, motorSpeed2);  // Điều khiển động cơ 2
  ledcWrite(2, motorSpeed3);  // Điều khiển động cơ 3
  ledcWrite(3, motorSpeed4);  // Điều khiển động cơ 4

  // mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Serial.print("Accel: ");
  // Serial.print("X="); Serial.print(ax);
  // Serial.print(" Y="); Serial.print(ay);
  // Serial.print(" Z="); Serial.println(az);

  // Serial.print("Gyro: ");
  // Serial.print("X="); Serial.print(gx);
  // Serial.print(" Y="); Serial.print(gy);
  // Serial.print(" Z="); Serial.println(gz);

  delay(500);
}
