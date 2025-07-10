#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

// ==== LoRa config ====
#define LED_PIN 2
#define SS    5
#define RST   14
#define DIO0  15

// ==== Motor PWM pins ====
int motorPin1 = 12;
int motorPin2 = 13;
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

// Smoothed speeds
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;
float alpha = 0.2;  // smoothing factor

// Throttle giữ giá trị liên tục
int currentThrottle = 100;


bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;  // Timeout nếu không nhận dữ liệu

unsigned long previousMillis = 0;
const long interval = 500;  // Thời gian chớp tắt LED (ms)
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

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

// Kalman Filter 1D
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

  if (clientConnected) {
    if (millis() - lastDataTime > timeoutNoData) {
      Serial.println("⚠️ Không nhận dữ liệu từ client trong 5 giây. Tắt LED và motor.");
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
      ledState = LOW;
      motorSpeed = 0;
      ledcWrite(0, motorSpeed);
    } else {
      // Chớp tắt LED mỗi 500ms
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
      // Cập nhật PWM motor dựa trên motorSpeed đã set khi nhận dữ liệu
      ledcWrite(0, motorSpeed);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    ledState = LOW;
    motorSpeed = 0;
    ledcWrite(0, motorSpeed);
  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // Parse giá trị joystick
    joy1Y = getValue(received, "joy1Y").toInt();  // tiến/lùi
    joy1X = getValue(received, "joy1X").toInt();  // trái/phải
    joy2Y = getValue(received, "joy2Y").toInt();  // cất/hạ cánh
    joy2X = getValue(received, "joy2X").toInt();  // tăng/giảm throttle
  }

  // ==== Đọc IMU ====
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

  // ==== Joystick Điều Khiển ====
  float pitchTarget = map(joy1Y, 0, 4095, -30, 30); // Tiến / Lùi
  float rollTarget  = map(joy1X, 0, 4095, -30, 30); // Trái / Phải

  // Điều chỉnh throttle bằng joy2X
  int joyMid = 2048;
  int delta = map(joy2X, 0, 4095, -5, 5);  // thay đổi nhỏ theo mỗi vòng lặp
  if (abs(joy2X - joyMid) > 200) {
    currentThrottle += delta;
    currentThrottle = constrain(currentThrottle, 80, 230);
  }
  int baseThrottle = currentThrottle;

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
  motorSpeed1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255);
  motorSpeed2 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255);
  motorSpeed3 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255);
  motorSpeed4 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255);

  // ==== ']'
  filteredM1 = (1 - alpha) * filteredM1 + alpha * motorSpeed1;
  filteredM2 = (1 - alpha) * filteredM2 + alpha * motorSpeed2;
  filteredM3 = (1 - alpha) * filteredM3 + alpha * motorSpeed3;
  filteredM4 = (1 - alpha) * filteredM4 + alpha * motorSpeed4;

  // ==== PWM Output ====
  ledcWrite(0, (int)filteredM1);
  ledcWrite(1, (int)filteredM2);
  ledcWrite(2, (int)filteredM3);
  ledcWrite(3, (int)filteredM4);

  // Debug
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" Roll: "); Serial.print(roll);
  Serial.print(" Throttle: "); Serial.print(baseThrottle);
Serial.print("M1:"); Serial.print((int)filteredM1); Serial.print("\t");
Serial.print("M2:"); Serial.print((int)filteredM2); Serial.print("\t");
Serial.print("M3:"); Serial.print((int)filteredM3); Serial.print("\t");
Serial.print("M4:"); Serial.println((int)filteredM4);
  delay(20);
}
