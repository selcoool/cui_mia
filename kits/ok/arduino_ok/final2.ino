#include <SPI.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <RF24.h>

// ==== Định nghĩa chân ====
#define LED_PIN     2
#define MOTOR1      3
#define MOTOR2      5
#define MOTOR3      6
#define MOTOR4      9
#define VOLTAGE_PIN A0
zda
// ==== NRF24L01 ====
#define CE_PIN  7
#define CSN_PIN 8
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// ==== MPU6050 ====
MPU6050 mpu(Wire);

// ==== PID ====
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 2.0, Ki_roll  = 0.0, Kd_roll  = 0.0;
float prevErrorPitch = 0, integralPitch = 0;
float prevErrorRoll  = 0, integralRoll  = 0;

// ==== Joystick & Throttle ====
int joy1Y = 2048, joy1X = 2048, joy2X = 2048;
int currentThrottle = 130;

// ==== Tín hiệu và LED ====
unsigned long lastReceiveTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastPrintTime = 0;
bool clientConnected = false;
bool ledState = false;
const unsigned long timeoutMs = 1000;
#define LOW_VOLTAGE 5.0

// ==== Smoothing ====
float alpha = 0.2;
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;

// ==== Hàm phụ ====
String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

float readVoltage() {
  int raw = analogRead(VOLTAGE_PIN);
  float voltage = (raw / 1023.0) * 5.0;
  return voltage * 2.0;  // Chia áp 1:1
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  mpu.begin();
  mpu.calcGyroOffsets();

  if (!radio.begin()) {
    Serial.println("❌ Không tìm thấy NRF24!");
    while (1);
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  Serial.println("✅ NRF24L01 sẵn sàng nhận dữ liệu...");
}

void loop() {
  // ==== Đọc dữ liệu NRF24 ====
  if (radio.available()) {
    char text[32] = {0};
    radio.read(&text, sizeof(text));
    String received = String(text);

    joy1X = getValue(received, "q").toInt();
    joy1Y = getValue(received, "w").toInt();
    joy2X = getValue(received, "e").toInt();

    if (abs(joy2X - 2048) > 200) {
      int delta = map(joy2X, 0, 4095, -5, 5);
      currentThrottle += delta;
      currentThrottle = constrain(currentThrottle, 80, 230);
    }

    lastReceiveTime = millis();
    clientConnected = true;
  }

  float batteryVoltage = readVoltage();

  // ==== Xử lý LED theo tình trạng kết nối và pin ====
  if (clientConnected && millis() - lastReceiveTime <= timeoutMs) {
    if (batteryVoltage < LOW_VOLTAGE) {
      // ⚠️ Có tín hiệu nhưng pin yếu → LED chớp
      if (millis() - lastBlinkTime > 300) {
        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
    } else {
      digitalWrite(LED_PIN, HIGH); // ✅ Có tín hiệu + pin khỏe
    }
  } else {
    // ❌ Mất tín hiệu → tắt motor & LED
    analogWrite(MOTOR1, 0);
    analogWrite(MOTOR2, 0);
    analogWrite(MOTOR3, 0);
    analogWrite(MOTOR4, 0);
    digitalWrite(LED_PIN, LOW);
    clientConnected = false;
    Serial.println("❌ NRF Timeout – motors off");
    return;
  }

  // ==== MPU PID điều khiển ====
  mpu.update();
  float pitch = mpu.getAngleX();
  float roll  = mpu.getAngleY();

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

  int pitchOffset = map(joy1Y, 0, 4095, -30, 30);
  int rollOffset  = map(joy1X, 0, 4095, -30, 30);

  int m1 = constrain(currentThrottle + pitchOffset + rollOffset + pidPitch + pidRoll, 0, 255);
  int m2 = constrain(currentThrottle + pitchOffset - rollOffset + pidPitch - pidRoll, 0, 255);
  int m3 = constrain(currentThrottle - pitchOffset - rollOffset - pidPitch - pidRoll, 0, 255);
  int m4 = constrain(currentThrottle - pitchOffset + rollOffset - pidPitch + pidRoll, 0, 255);

  filteredM1 = (1 - alpha) * filteredM1 + alpha * m1;
  filteredM2 = (1 - alpha) * filteredM2 + alpha * m2;
  filteredM3 = (1 - alpha) * filteredM3 + alpha * m3;
  filteredM4 = (1 - alpha) * filteredM4 + alpha * m4;

  analogWrite(MOTOR1, (int)filteredM1);
  analogWrite(MOTOR2, (int)filteredM2);
  analogWrite(MOTOR3, (int)filteredM3);
  analogWrite(MOTOR4, (int)filteredM4);

  // ==== In debug mỗi 500ms ====
  if (millis() - lastPrintTime >= 500) {
    lastPrintTime = millis();
    Serial.print("🔋 Điện áp: ");
    Serial.print(batteryVoltage, 2);
    Serial.print("V | Throttle: ");
    Serial.print(currentThrottle);
    Serial.print(" | M1: "); Serial.print((int)filteredM1);
    Serial.print(" M2: "); Serial.print((int)filteredM2);
    Serial.print(" M3: "); Serial.print((int)filteredM3);
    Serial.print(" M4: "); Serial.println((int)filteredM4);
  }

  delay(20); // 50Hz
}
