#include <Arduino.h>
#include <Wire.h>
#include <MPU9250.h>
#include <Adafruit_BMP280.h>

#define SDA_PIN 8
#define SCL_PIN 9

MPU9250 mpu;
Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("INIT GY-91...");

  if (!mpu.setup(0x68)) {
    Serial.println("MPU9250 FAIL");
    while (1);
  }

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 FAIL");
    while (1);
  }

  Serial.println("ALL OK 🚀");
}

void loop() {
  mpu.update();

  Serial.print("ACC: ");
  Serial.print(mpu.getAccX()); Serial.print(" ");
  Serial.print(mpu.getAccY()); Serial.print(" ");
  Serial.println(mpu.getAccZ());

  Serial.print("GYRO: ");
  Serial.print(mpu.getGyroX()); Serial.print(" ");
  Serial.print(mpu.getGyroY()); Serial.print(" ");
  Serial.println(mpu.getGyroZ());

  Serial.print("ALT: ");
  Serial.println(bmp.readAltitude(1013.25));

  Serial.println("--------------------");

  delay(200);
}