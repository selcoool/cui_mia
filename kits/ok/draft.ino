#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

// ==== LoRa config ====
#define SS    5
#define RST   14
#define DIO0  15

// ==== Motor PWM pins ====
int motorPin1 = 25;
int motorPin2 = 26;
int motorPin3 = 27;
int motorPin4 = 33;

// ==== MPU6050 ====
MPU6050 mpu;

// PID Params
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll = 2.0, Ki_roll = 0.0, Kd_roll = 0.0;
float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// Kalman
float kalmanPitch = 0, kalmanRoll = 0;
float uncertaintyPitch = 2, uncertaintyRoll = 2;
float t = 0.02;  // 20ms loop

// IMU data
int16_t ax, ay, az, gx, gy, gz;
float pitch, roll;

// Joystick values (updated from LoRa)
int joy1Y = 2048, joy2X = 2048, joy2Y = 2048;

// Motor speeds
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

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

// Kalman 1D
void kalman_1d(float& state, float& uncertainty, float rate, float measurement) {
  state += t * rate;
  uncertainty += t * t * 16;
  float gain = uncertainty / (uncertainty + 9);
  state += gain * (measurement - state);
  uncertainty *= (1 - gain);
}

// Tách giá trị từ chuỗi
String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

void loop() {
  // Nhận dữ liệu LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // Parse giá trị joystick
    joy1Y = getValue(received, "joy1Y").toInt();
    joy1X = getValue(received, "joy1X").toInt();
    joy2Y = getValue(received, "joy2Y").toInt();
    joy2X = getValue(received, "joy2X").toInt();
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

  // ==== Điều khiển từ Joystick ====
  float pitchTarget = map(joy2Y, 0, 4095, -50, 50);
  float rollTarget  = map(joy2X, 0, 4095, -50, 50);
  int baseThrottle  = map(joy1Y, 0, 4095, 100, 200);
  baseThrottle = constrain(baseThrottle, 80, 230);

  // ==== PID ====
  float errorPitch = pitchTarget - pitch;
  float dPitch = errorPitch - previousErrorPitch;
  integralPitch += errorPitch;
  integralPitch = constrain(integralPitch, -400, 400);

  float errorRoll = rollTarget - roll;
  float dRoll = errorRoll - previousErrorRoll;
  integralRoll += errorRoll;
  integralRoll = constrain(integralRoll, -400, 400);

  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * dPitch;
  float pidRoll  = Kp_roll  * errorRoll  + Ki_roll  * integralRoll  + Kd_roll  * dRoll;

  previousErrorPitch = errorPitch;
  previousErrorRoll = errorRoll;

  // ==== Tính tốc độ động cơ ====
  motorSpeed1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255);
  motorSpeed2 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255);
  motorSpeed3 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255);
  motorSpeed4 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255);

  // PWM output
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
