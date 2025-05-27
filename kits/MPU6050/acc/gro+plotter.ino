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

  // In header tên biến cho Serial Plotter (in 1 lần)
  Serial.println("gx\tgy\tgz");
}

void loop() {
  int16_t gx_raw, gy_raw, gz_raw;

  mpu.getRotation(&gx_raw, &gy_raw, &gz_raw);

  float gx = gx_raw / 131.0;
  float gy = gy_raw / 131.0;
  float gz = gz_raw / 131.0;

  // In dữ liệu thuần số cho Serial Plotter
  Serial.print(gx, 3); Serial.print("\t");
  Serial.print(gy, 3); Serial.print("\t");
  Serial.println(gz, 3);

  // In dòng mô tả đẹp cho Serial Monitor (cách dòng số)
  Serial.print("GYRO (°/s) -> X: "); Serial.print(gx, 2);
  Serial.print(" | Y: "); Serial.print(gy, 2);
  Serial.print(" | Z: "); Serial.println(gz, 2);

  delay(200);
}
