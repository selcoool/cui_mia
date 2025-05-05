#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  Wire.begin();

  byte status = mpu.begin();
  if (status != 0) {
    Serial.println("❌ Lỗi kết nối MPU6050");
    while (1);
  }

  Serial.println("✅ MPU6050 kết nối thành công!");
  Serial.println("📐 Đang hiệu chỉnh gyro...");
  delay(1000);
  mpu.calcGyroOffsets();
  Serial.println("✅ Đã hiệu chỉnh xong!");
}

void loop() {
  mpu.update();

  Serial.print("📊 Acc X: "); Serial.print(mpu.getAccX());
  Serial.print(" | Y: "); Serial.print(mpu.getAccY());
  Serial.print(" | Z: "); Serial.println(mpu.getAccZ());

  Serial.print("🎯 Gyro X: "); Serial.print(mpu.getGyroX());
  Serial.print(" | Y: "); Serial.print(mpu.getGyroY());
  Serial.print(" | Z: "); Serial.println(mpu.getGyroZ());

  delay(500);
}
