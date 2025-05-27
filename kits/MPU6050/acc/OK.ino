#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA = 21, SCL = 22 for ESP32

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected!");
}

void loop() {
  int16_t ax_raw, ay_raw, az_raw;
  int16_t gx_raw, gy_raw, gz_raw;

  mpu.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);

  float AccX = ax_raw / 16384.0;
  float AccY = ay_raw / 16384.0;
  float AccZ = az_raw / 16384.0;

  float RateRoll = gx_raw / 131.0;
  float RatePitch = gy_raw / 131.0;
  float RateYaw = gz_raw / 131.0;

  float AngleRoll = atan2(AccY, sqrt(AccX * AccX + AccZ * AccZ)) * 57.2958;
  float AnglePitch = -atan2(AccX, sqrt(AccY * AccY + AccZ * AccZ)) * 57.2958;

  Serial.print("Acc -> X: "); Serial.print(AccX, 2);
  Serial.print(" | Y: "); Serial.print(AccY, 2);
  Serial.print(" | Z: "); Serial.print(AccZ, 2);
  
  Serial.print(" || Gyro -> Roll: "); Serial.print(RateRoll, 2);
  Serial.print(" | Pitch: "); Serial.print(RatePitch, 2);
  Serial.print(" | Yaw: "); Serial.print(RateYaw, 2);

  Serial.print(" || Angles -> Roll: "); Serial.print(AngleRoll, 2);
  Serial.print(" | Pitch: "); Serial.println(AnglePitch, 2);

  delay(500);
}
