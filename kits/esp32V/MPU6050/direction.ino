#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Thời gian
unsigned long timer;

// Góc
float pitch = 0.0;
float roll  = 0.0;
float yaw   = 0.0;

// Hệ số Complementary Filter
const float alpha = 0.98;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Khởi tạo I2C ESP32
  Wire.begin(21, 22);   // SDA = 21, SCL = 22
  Wire.setClock(100000); // 100 kHz

  Serial.println("Scanning I2C bus...");
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  if(count == 0) Serial.println("No I2C devices found!");

  // Khởi tạo MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  Serial.println("MPU6050 initialized!");

  timer = millis();
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  // Đọc dữ liệu raw từ MPU6050
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Chuyển gyro sang deg/s
  float gx_dps = gx / 131.0;
  float gy_dps = gy / 131.0;
  float gz_dps = gz / 131.0;

  // Chuyển accel sang g
  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // Tính pitch & roll từ accel
  float pitchAcc = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180.0 / PI;
  float rollAcc  = atan2(-ax_g, az_g) * 180.0 / PI;

  // Delta time
  float dt = (millis() - timer) / 1000.0;
  timer = millis();

  // Complementary filter
  pitch = alpha * (pitch + gx_dps * dt) + (1 - alpha) * pitchAcc;
  roll  = alpha * (roll  + gy_dps * dt) + (1 - alpha) * rollAcc;

  // Yaw từ gyro Z
  yaw += gz_dps * dt;

  // In ra Serial
  Serial.print("Pitch: "); Serial.print(pitch, 2);
  Serial.print(" | Roll: "); Serial.print(roll, 2);
  Serial.print(" | Yaw: "); Serial.println(yaw, 2);

  delay(20); // ~50Hz
}
