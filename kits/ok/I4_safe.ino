#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

// ==== LoRa config ====
#define SS    5
#define RST   14
#define DIO0  26

// ==== Motor PWM pins ====
int motorPin1 = 12;
int motorPin2 = 13;
int motorPin3 = 25;
int motorPin4 = 27;

// ==== LED báo trạng thái ====
#define LED_PIN 2

// ==== MPU6050 ====
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch, roll;

// Kalman filter
float kalmanPitch = 0, kalmanRoll = 0;
float uncertaintyPitch = 2, uncertaintyRoll = 2;

// ==== PID ====
float Kp_pitch = 1.0, Ki_pitch = 0.0, Kd_pitch = 0.4;
float Kp_roll  = 1.0, Ki_roll  = 0.0, Kd_roll  = 0.4;
float previousErrorPitch = 0, previousErrorRoll = 0;
float integralPitch = 0, integralRoll = 0;

// ==== Motor ====
int motorSpeed1 = 0, motorSpeed2 = 0, motorSpeed3 = 0, motorSpeed4 = 0;
float alpha = 0.1;
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;
int currentThrottle = 100;

// ==== Joystick ====
int joy1X = 2048, joy1Y = 2048, joy2X = 2048, joy2Y = 2048;

// ==== Timeout ====
bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 2000;

// ==== LED nhấp nháy ====
unsigned long previousMillis = 0;
const long interval = 500;
bool ledState = false;

// ===== Kalman Filter =====
void kalman_1d(float& state, float& uncertainty, float gyroRate, float measuredAngle) {
  float dt = 0.02;
  float rate = gyroRate;
  state += dt * rate;
  uncertainty += 0.02;

  float K = uncertainty / (uncertainty + 0.1);
  state += K * (measuredAngle - state);
  uncertainty *= (1 - K);
}

// ==== Lấy giá trị joystick ====
String getValue(String data, String key) {
  int idx = data.indexOf(key + "=");
  if (idx == -1) return "0";
  int endIdx = data.indexOf(",", idx);
  if (endIdx == -1) endIdx = data.length();
  return data.substring(idx + key.length() + 1, endIdx);
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 không kết nối!");
    while (1);
  }

  pinMode(LED_PIN, OUTPUT);
  ledcSetup(0, 500, 8);
  ledcSetup(1, 500, 8);
  ledcSetup(2, 500, 8);
  ledcSetup(3, 500, 8);
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Lỗi khởi tạo LoRa!");
    while (1);
  }
  Serial.println("LoRa khởi động thành công!");
}

// ==== Loop chính ====
void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    clientConnected = true;
    lastDataTime = millis();

    joy1Y = getValue(received, "joy1Y").toInt();
    joy1X = getValue(received, "joy1X").toInt();
    joy2Y = getValue(received, "joy2Y").toInt();
    joy2X = getValue(received, "joy2X").toInt();
  }

  if (clientConnected) {
    if (millis() - lastDataTime > timeoutNoData) {
      Serial.println("⚠️ Mất kết nối. Tắt động cơ.");
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      ledcWrite(2, 0);
      ledcWrite(3, 0);
      return;
    }

    // LED nhấp nháy
    if (millis() - previousMillis > interval) {
      previousMillis = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }

    // Đọc MPU6050
    mpu.getAcceleration(&ax, &ay, &az);
    mpu.getRotation(&gx, &gy, &gz);

    float rawPitch = 0, rawRoll = 0;
    float denPitch = sqrt(ax * ax + az * az);
    float denRoll = sqrt(ay * ay + az * az);

    if (denPitch > 0.0001 && denRoll > 0.0001) {
      rawPitch = atan2(ay, denPitch) * 180.0 / PI;
      rawRoll = atan2(-ax, denRoll) * 180.0 / PI;
    }

    kalman_1d(kalmanPitch, uncertaintyPitch, gx / 131.0, rawPitch);
    kalman_1d(kalmanRoll, uncertaintyRoll, gy / 131.0, rawRoll);

    pitch = kalmanPitch;
    roll = kalmanRoll;

    // Điều khiển joystick
    float pitchTarget = map(joy1Y, 0, 4095, -30, 30);
    float rollTarget = map(joy1X, 0, 4095, -30, 30);

    int joyMid = 2048;
    int delta = map(joy2X, 0, 4095, -5, 5);
    if (abs(joy2X - joyMid) > 200) {
      currentThrottle += delta;
      currentThrottle = constrain(currentThrottle, 80, 230);
    }
    int baseThrottle = currentThrottle;

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

    // Mixing motor
    motorSpeed1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255);
    motorSpeed2 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255);
    motorSpeed3 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255);
    motorSpeed4 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255);

    // Smoothing
    filteredM1 = (1 - alpha) * filteredM1 + alpha * motorSpeed1;
    filteredM2 = (1 - alpha) * filteredM2 + alpha * motorSpeed2;
    filteredM3 = (1 - alpha) * filteredM3 + alpha * motorSpeed3;
    filteredM4 = (1 - alpha) * filteredM4 + alpha * motorSpeed4;

    // Output PWM
    ledcWrite(0, (int)filteredM1);
    ledcWrite(1, (int)filteredM2);
    ledcWrite(2, (int)filteredM3);
    ledcWrite(3, (int)filteredM4);

    // Debug
    Serial.print("P:"); Serial.print(pitch, 1);
    Serial.print(" R:"); Serial.print(roll, 1);
    Serial.print(" T:"); Serial.print(baseThrottle);
    Serial.print(" M1:"); Serial.print((int)filteredM1);
    Serial.print(" M2:"); Serial.print((int)filteredM2);
    Serial.print(" M3:"); Serial.print((int)filteredM3);
    Serial.print(" M4:"); Serial.println((int)filteredM4);

    delay(20);
  } else {
    // Không có kết nối
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    digitalWrite(LED_PIN, LOW);
  }
}