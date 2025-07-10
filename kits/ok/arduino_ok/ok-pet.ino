#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <MPU6050_light.h>

// ==== Pin định nghĩa ====
#define MOTOR1 3
#define MOTOR2 5
#define MOTOR3 6
#define MOTOR4 9

#define SS   10
#define RST  7
#define DIO0 2

MPU6050 mpu(Wire);

// ==== PID ====
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 2.0, Ki_roll  = 0.0, Kd_roll  = 0.0;
float prevErrorPitch = 0, integralPitch = 0;
float prevErrorRoll  = 0, integralRoll  = 0;

// ==== LoRa Timeout ====
bool clientConnected = false;
unsigned long lastReceiveTime = 0;
const unsigned long timeoutMs = 1000;

// ==== Joystick ====
int joy1Y = 2048, joy1X = 2048, joy2X = 2048;
int currentThrottle = 130;

// ==== Smoothing ====
float alpha = 0.2;
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;

// ==== Tách chuỗi ====
String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets();

  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed");
    while (1);
  }
  Serial.println("LoRa ready");
}

void loop() {
  // ==== Nhận LoRa ====
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) received += (char)LoRa.read();

    joy1Y = getValue(received, "joy1Y").toInt();
    joy1X = getValue(received, "joy1X").toInt();
    joy2X = getValue(received, "joy2X").toInt();

    // Tăng/giảm throttle khi lệch khỏi giữa
    if (abs(joy2X - 2048) > 200) {
      int delta = map(joy2X, 0, 4095, -5, 5);
      currentThrottle += delta;
      currentThrottle = constrain(currentThrottle, 80, 230);
    }

    lastReceiveTime = millis();
    clientConnected = true;
  }

  // ==== Timeout mất kết nối ====
  if (clientConnected && millis() - lastReceiveTime > timeoutMs) {
    clientConnected = false;
    analogWrite(MOTOR1, 0);
    analogWrite(MOTOR2, 0);
    analogWrite(MOTOR3, 0);
    analogWrite(MOTOR4, 0);
    Serial.println("LoRa timeout – motors off");
    return;
  }

  if (!clientConnected) return;

  // ==== MPU6050 ====
  mpu.update();
  float pitch = mpu.getAngleX();
  float roll  = mpu.getAngleY();

  // ==== PID ====
  float errorPitch = 0 - pitch;
  integralPitch += errorPitch;
  float dPitch = errorPitch - prevErrorPitch;
  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * dPitch;
  prevErrorPitch = errorPitch;

  float errorRoll = 0 - roll;
  integralRoll += errorRoll;
  float dRoll = errorRoll - prevErrorRoll;
  float pidRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * dRoll;
  prevErrorRoll = errorRoll;

  // ==== Joystick offset ====
  int pitchOffset = map(joy1Y, 0, 4095, -30, 30);
  int rollOffset  = map(joy1X, 0, 4095, -30, 30);

  // ==== Tổng hợp motor ====
  int m1 = constrain(currentThrottle + pitchOffset + rollOffset + pidPitch + pidRoll, 0, 255);
  int m2 = constrain(currentThrottle + pitchOffset - rollOffset + pidPitch - pidRoll, 0, 255);
  int m3 = constrain(currentThrottle - pitchOffset - rollOffset - pidPitch - pidRoll, 0, 255);
  int m4 = constrain(currentThrottle - pitchOffset + rollOffset - pidPitch + pidRoll, 0, 255);

  // ==== Smoothing ====
  filteredM1 = (1 - alpha) * filteredM1 + alpha * m1;
  filteredM2 = (1 - alpha) * filteredM2 + alpha * m2;
  filteredM3 = (1 - alpha) * filteredM3 + alpha * m3;
  filteredM4 = (1 - alpha) * filteredM4 + alpha * m4;

  analogWrite(MOTOR1, (int)filteredM1);
  analogWrite(MOTOR2, (int)filteredM2);
  analogWrite(MOTOR3, (int)filteredM3);
  analogWrite(MOTOR4, (int)filteredM4);

  // ==== Debug ====
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" Roll: "); Serial.print(roll);
  Serial.print(" Throttle: "); Serial.print(currentThrottle);
  Serial.print(" M1: "); Serial.print((int)filteredM1);
  Serial.print(" M2: "); Serial.print((int)filteredM2);
  Serial.print(" M3: "); Serial.print((int)filteredM3);
  Serial.print(" M4: "); Serial.println((int)filteredM4);

  delay(20);  // 50Hz
}
