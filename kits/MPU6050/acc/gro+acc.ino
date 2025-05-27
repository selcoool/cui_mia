#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA = GPIO21, SCL = GPIO22 (ESP32 mặc định)

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

   mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);    // ±250 °/s → hệ số 131.0
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);    // ±2g      → hệ số 16384.0

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected!");
}

void loop() {
  int16_t ax_raw, ay_raw, az_raw;
  int16_t gx_raw, gy_raw, gz_raw;

  // Lấy dữ liệu acc và gyro
  mpu.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);

  // Chuyển đổi acc từ raw sang g (±2g → 16384 LSB/g)
  float ax = ax_raw / 16384.0;
  float ay = ay_raw / 16384.0;
  float az = az_raw / 16384.0;

  // Chuyển đổi gyro từ raw sang độ/giây (±250°/s → 131 LSB/°/s)
  float gx = gx_raw / 131.0;
  float gy = gy_raw / 131.0;
  float gz = gz_raw / 131.0;

  // Hiển thị kết quả
  Serial.print("ACC (g) -> X: "); Serial.print(ax, 2);
  Serial.print(" | Y: "); Serial.print(ay, 2);
  Serial.print(" | Z: "); Serial.print(az, 2);

  Serial.print(" || GYRO (°/s) -> X: "); Serial.print(gx, 2);
  Serial.print(" | Y: "); Serial.print(gy, 2);
  Serial.print(" | Z: "); Serial.println(gz, 2);

  delay(500);
}