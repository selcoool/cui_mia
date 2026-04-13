#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);

  // Nếu bạn KHÔNG chắc chân, dùng mặc định:
  Wire.begin();

  Serial.println("Starting I2C scan...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found device at 0x");
      Serial.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("❌ No I2C device found");
  } else {
    Serial.println("✔ Done");
  }

  delay(3000);
}
