#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 4
#define DIO0 26
#define LED 2  

int motorPin1 = 14;
int motorPin2 = 12;
int motorPin3 = 15;
int motorPin4 = 13;

MPU6050 mpu;

float Kp_pitch = 15.0;
float Ki_pitch = 0.0;
float Kd_pitch = 5.0;
float Kp_roll = 15.0;
float Ki_roll = 0.0;
float Kd_roll = 5.0;

float previousErrorPitch = 0;
float integralPitch = 0;
float previousErrorRoll = 0;
float integralRoll = 0;

int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch, roll;
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa init failed!");
        while (1);
    }
    Serial.println("✅ LoRa receiver ready");

    Wire.begin();
    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed");
        while (1);
    }

    ledcSetup(0, 1000, 8);
    ledcSetup(1, 1000, 8);
    ledcSetup(2, 1000, 8);
    ledcSetup(3, 1000, 8);

    ledcAttachPin(motorPin1, 0);
    ledcAttachPin(motorPin2, 1);
    ledcAttachPin(motorPin3, 2);
    ledcAttachPin(motorPin4, 3);
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String received = "";
        while (LoRa.available()) {
            received += (char)LoRa.read();
        }
        Serial.println("📩 Nhận được: " + received);

        int joy1X = getValue(received, "joy1X").toInt();
        int joy1Y = getValue(received, "joy1Y").toInt();
        bool joy1B = getValue(received, "joy1B") == "1";

        mpu.getAcceleration(&ax, &ay, &az);
        mpu.getRotation(&gx, &gy, &gz);

        pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
        roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

        float errorPitch = 0 - pitch;
        integralPitch += errorPitch;
        float derivativePitch = errorPitch - previousErrorPitch;
        float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
        previousErrorPitch = errorPitch;

        float errorRoll = 0 - roll;
        integralRoll += errorRoll;
        float derivativeRoll = errorRoll - previousErrorRoll;
        float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
        previousErrorRoll = errorRoll;

        motorSpeed1 = constrain(128 + pidOutputPitch + pidOutputRoll, 0, 255);
        motorSpeed2 = constrain(128 - pidOutputPitch + pidOutputRoll, 0, 255);
        motorSpeed3 = constrain(128 + pidOutputPitch - pidOutputRoll, 0, 255);
        motorSpeed4 = constrain(128 - pidOutputPitch - pidOutputRoll, 0, 255);

        Serial.printf("Motor Speeds -> 1: %d, 2: %d, 3: %d, 4: %d\n", motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4);

        ledcWrite(0, motorSpeed1);
        ledcWrite(1, motorSpeed2);
        ledcWrite(2, motorSpeed3);
        ledcWrite(3, motorSpeed4);
    }
}

String getValue(String data, String key) {
    int startIndex = data.indexOf(key + ":");
    if (startIndex == -1) return "";
    startIndex += key.length() + 1;
    int endIndex = data.indexOf(" ", startIndex);
    if (endIndex == -1) endIndex = data.length();
    return data.substring(startIndex, endIndex);
}
