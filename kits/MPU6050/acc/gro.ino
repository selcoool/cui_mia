#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA = GPIO21, SCL = GPIO22 trên ESP32

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected!");
}

void loop() {
  int16_t gx_raw, gy_raw, gz_raw;

  // Chỉ lấy dữ liệu gyro
  mpu.getRotation(&gx_raw, &gy_raw, &gz_raw);

  // Chuyển đổi từ raw → độ/giây (LSB = 131.0 ở ±250°/s)
  float gx = gx_raw / 131.0;
  float gy = gy_raw / 131.0;
  float gz = gz_raw / 131.0;

  // Hiển thị dữ liệu gyro
  Serial.print("GYRO (°/s) -> X: "); Serial.print(gx, 2);
  Serial.print(" | Y: "); Serial.print(gy, 2);
  Serial.print(" | Z: "); Serial.println(gz, 2);

  delay(200);
}
