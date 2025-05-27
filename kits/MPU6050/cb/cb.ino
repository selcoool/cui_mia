#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }
  Serial.println("MPU6050 connected successfully");
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Chuyển đổi từ raw sang đơn vị gia tốc g
  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // Tính góc nghiêng (đơn vị độ)
  float pitch = atan2(ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180 / PI;
  float roll  = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180 / PI;

  // Hiển thị dữ liệu
  Serial.print("Accel X: "); Serial.print(ax_g);
  Serial.print(" | Y: "); Serial.print(ay_g);
  Serial.print(" | Z: "); Serial.print(az_g);

  Serial.print(" || Gyro X: "); Serial.print(gx / 131.0);
  Serial.print(" | Y: "); Serial.print(gy / 131.0);
  Serial.print(" | Z: "); Serial.print(gz / 131.0);

  Serial.print(" || Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.println(roll);

  delay(500);
}
