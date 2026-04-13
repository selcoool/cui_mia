#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // ⚡ I2C mặc định (khuyên test trước)
  Wire.begin();

  Serial.println("Starting MPU6050...");

  mpu.initialize();

  // kiểm tra kết nối
  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 NOT CONNECTED!");
    Serial.println("Check:");
    Serial.println("- SDA/SCL wiring");
    Serial.println("- 3.3V power");
    Serial.println("- GND common");
    while (1) delay(10);
  }

  Serial.println("✔ MPU6050 CONNECTED!");
}

void loop() {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  Serial.print("Accel X: "); Serial.print(ax);
  Serial.print(" | Y: "); Serial.print(ay);
  Serial.print(" | Z: "); Serial.println(az);

  Serial.print("Gyro  X: "); Serial.print(gx);
  Serial.print(" | Y: "); Serial.print(gy);
  Serial.print(" | Z: "); Serial.println(gz);

  Serial.println("----------------------");

  delay(500);
}