#include <Arduino.h>
#include <Wire.h>

#define SDA 8
#define SCL 9

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);

  Serial.println("READ MPU REGISTER...");
}

uint8_t readReg(uint8_t reg) {
  Wire.beginTransmission(0x68);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 1);
  return Wire.read();
}

void loop() {

  uint8_t whoami = readReg(0x75);

  Serial.print("WHO_AM_I: ");
  Serial.println(whoami, HEX);

  delay(1000);
}

// | Value | Meaning                           |
// | ----- | --------------------------------- |
// | 0x71  | OK (MPU9250 đúng chuẩn)           |
// | 0x70  | ⚠ lệch / chip variant / bus issue |
// | 0x00  | chết hoặc sleep                   |
// | 0xFF  | fake / không connect              |