#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

// ==== LoRa config ====
#define SS    5
#define RST   14
#define DIO0  15

// ==== Motor PWM pins ====
int motorPin1 = 12;
int motorPin2 = 13;  // Sửa từ 14 -> 13 để tránh xung đột RST LoRa
int motorPin3 = 25;
int motorPin4 = 26;

// ==== MPU6050 ====
MPU6050 mpu;

// PID Parameters
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll = 2.0, Ki_roll = 0.0, Kd_roll = 0.0;
float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// Kalman Filter
float kalmanPitch = 0, kalmanRoll = 0;
float uncertaintyPitch = 2, uncertaintyRoll = 2;
float t = 0.02;  // 20ms loop

// IMU raw data
int16_t ax, ay, az, gx, gy, gz;
float pitch, roll;

// Joystick values (updated from LoRa)
int joy1Y = 2048, joy1X = 2048, joy2Y = 2048, joy2X = 2048;

// Motor speeds
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

unsigned long lastPacketTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 failed!");
    while (1);
  }

  // PWM setup
  ledcSetup(0, 1000, 8); ledcAttachPin(motorPin1, 0);
  ledcSetup(1, 1000, 8); ledcAttachPin(motorPin2, 1);
  ledcSetup(2, 1000, 8); ledcAttachPin(motorPin3, 2);
  ledcSetup(3, 1000, 8); ledcAttachPin(motorPin4, 3);

  // LoRa init
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ LoRa failed!");
    while (1);
  }
  Serial.println("✅ LoRa ready");
}

void kalman_1d(float& state, float& uncertainty, float rate, float measurement) {
  state += t * rate;
  uncertainty += t * t * 16;
  float gain = uncertainty / (uncertainty + 9);
  state += gain * (measurement - state);
  uncertainty *= (1 - gain);
}

String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

void loop() {
  // ==== LoRa Packet ====
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    joy1Y = getValue(received, "joy1Y").toInt();  // tiến/lùi
    joy1X = getValue(received, "joy1X").toInt();  // trái/phải
    joy2Y = getValue(received, "joy2Y").toInt();  // throttle
    joy2X = getValue(received, "joy2X").toInt();  // chưa dùng

    lastPacketTime = millis();  // Cập nhật thời gian nhận gói gần nhất
  }

  // Failsafe: Nếu mất tín hiệu hơn 1000ms thì reset joystick
  if (millis() - lastPacketTime > 1000) {
    joy2Y = 0; // Giảm throttle về 0
  }

  // ==== MPU6050 ====
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  float rawPitch = 0, rawRoll = 0;
  float denPitch = sqrt(ax * ax + az * az);
  float denRoll  = sqrt(ay * ay + az * az);

  if (denPitch > 0.0001 && denRoll > 0.0001) {
    rawPitch = atan2(ay, denPitch) * 180.0 / PI;
    rawRoll  = atan2(-ax, denRoll) * 180.0 / PI;
  }

  kalman_1d(kalmanPitch, uncertaintyPitch, gx / 131.0, rawPitch);
  kalman_1d(kalmanRoll, uncertaintyRoll, gy / 131.0, rawRoll);

  pitch = kalmanPitch;
  roll = kalmanRoll;

  // ==== Điều khiển bằng joystick ====
  float pitchTarget = map(joy1Y, 0, 4095, -30, 30);
  float rollTarget  = map(joy1X, 0, 4095, -30, 30);
  int baseThrottle  = map(joy2Y, 0, 4095, 80, 230);
  baseThrottle = constrain(baseThrottle, 80, 230);

  // ==== PID Pitch ====
  float errorPitch = pitchTarget - pitch;
  float dPitch = errorPitch - previousErrorPitch;
  integralPitch += errorPitch;
  integralPitch = constrain(integralPitch, -400, 400);

  // ==== PID Roll ====
  float errorRoll = rollTarget - roll;
  float dRoll = errorRoll - previousErrorRoll;
  integralRoll += errorRoll;
  integralRoll = constrain(integralRoll, -400, 400);

  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * dPitch;
  float pidRoll  = Kp_roll  * errorRoll  + Ki_roll  * integralRoll  + Kd_roll  * dRoll;

  previousErrorPitch = errorPitch;
  previousErrorRoll = errorRoll;

  // ==== Motor Mixing ====
  motorSpeed1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255); // Front Left
  motorSpeed2 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255); // Front Right
  motorSpeed3 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255); // Rear Right
  motorSpeed4 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255); // Rear Left

  // PWM Output
  ledcWrite(0, motorSpeed1);
  ledcWrite(1, motorSpeed2);
  ledcWrite(2, motorSpeed3);
  ledcWrite(3, motorSpeed4);

  // Debug
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" Roll: "); Serial.print(roll);
  Serial.print(" T: "); Serial.print(baseThrottle);
  Serial.print(" M1: "); Serial.print(motorSpeed1);
  Serial.print(" M2: "); Serial.print(motorSpeed2);
  Serial.print(" M3: "); Serial.print(motorSpeed3);
  Serial.print(" M4: "); Serial.println(motorSpeed4);

  delay(20);
}
