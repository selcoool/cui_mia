#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

#define SDA_PIN 8
#define SCL_PIN 9
#define INT_PIN 7

volatile bool mpuReady = false;

void IRAM_ATTR mpuISR() {
  mpuReady = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Init MPU6050...");

  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 FAIL");
    while (1);
  }

  Serial.println("✔ MPU6050 OK");

  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), mpuISR, RISING);

  Serial.println("INT READY");
}

void loop() {

  if (mpuReady) {
    mpuReady = false;

    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180 / 3.14159;
    float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180 / 3.14159;

    Serial.print("Pitch: ");
    Serial.print(pitch);
    Serial.print(" | Roll: ");
    Serial.println(roll);
  }
}