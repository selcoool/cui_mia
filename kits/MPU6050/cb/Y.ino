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
  Serial.println("MPU6050 connected");
}

void loop() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // In gia tốc trục Y
  Serial.print("Accel Y (g): ");
  Serial.println(ay_g);

  // Tính góc Roll (quanh trục Y)
  float roll = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180 / PI;

  Serial.print("Roll (degree): ");
  Serial.println(roll);

  delay(500);
}
