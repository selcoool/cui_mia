#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA = GPIO21, SCL = GPIO22 (ESP32 mặc định)

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  // Kiểm tra kết nối
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected!");
}

void loop() {
  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

  // Chuyển đổi từ raw sang đơn vị "g" (±2g = 16384 LSB/g)
  float ax = ax_raw / 16384.0;
  float ay = ay_raw / 16384.0;
  float az = az_raw / 16384.0;

  // Tính tổng gia tốc (dạng vector)
  float totalAcc = sqrt(ax * ax + ay * ay + az * az);


//   // IN DỮ LIỆU DÀNH CHO SERIAL PLOTTER (chỉ số, cách tab)
  Serial.print(ax, 3); Serial.print("\t");
  Serial.print(ay, 3); Serial.print("\t");
  Serial.print(az, 3); Serial.print("\t");
  Serial.println(totalAcc, 3);


  Serial.print("Acc X: "); Serial.print(ax, 3); Serial.print(" g, ");
  Serial.print("Y: "); Serial.print(ay, 3); Serial.print(" g, ");
  Serial.print("Z: "); Serial.print(az, 3); Serial.print(" g, ");
  Serial.print(" | Total: "); Serial.print(totalAcc, 3); Serial.println(" g");

  delay(500);
}