#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

float pitch, roll;

// Tham số PID cho Pitch
float Kp_pitch = 2.0;
float Ki_pitch = 0.05;
float Kd_pitch = 0.5;
float errorPitch, previousErrorPitch = 0, integralPitch = 0;

// Tham số PID cho Roll
float Kp_roll = 2.0;
float Ki_roll = 0.05;
float Kd_roll = 0.5;
float errorRoll, previousErrorRoll = 0, integralRoll = 0;

int motor1Pin = 3;
int motor2Pin = 5;
int motor3Pin = 6;
int motor4Pin = 9;

const float MAX_TILT_ANGLE = 15.0; // Giới hạn nghiêng tối đa cho phép (độ)
const int ledWarningPin = 13;      // LED cảnh báo nếu vượt quá giới hạn

unsigned long previousTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected successfully");
  } else {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  pinMode(motor1Pin, OUTPUT);
  pinMode(motor2Pin, OUTPUT);
  pinMode(motor3Pin, OUTPUT);
  pinMode(motor4Pin, OUTPUT);
  pinMode(ledWarningPin, OUTPUT);
}

void loop() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - previousTime) / 1000.0;
  previousTime = currentTime;

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  if ((ax == 0 && ay == 0 && az == 0) || isnan(ax) || isnan(ay) || isnan(az)) {
    Serial.println("Invalid sensor data!");
    return;
  }

  float denominator_pitch = sqrt((float)ax * ax + (float)az * az);
  float denominator_roll = sqrt((float)ay * ay + (float)az * az);

  if (denominator_pitch != 0) {
    pitch = atan2((float)ay, denominator_pitch) * 180.0 / PI;
  } else {
    pitch = 0;
  }

  if (denominator_roll != 0) {
    roll = atan2((float)-ax, denominator_roll) * 180.0 / PI;
  } else {
    roll = 0;
  }

  // ======= Giới hạn độ nghiêng =======
  if (abs(pitch) > MAX_TILT_ANGLE || abs(roll) > MAX_TILT_ANGLE) {
    analogWrite(motor1Pin, 0);
    analogWrite(motor2Pin, 0);
    analogWrite(motor3Pin, 0);
    analogWrite(motor4Pin, 0);

    digitalWrite(ledWarningPin, HIGH); // Bật LED cảnh báo
    Serial.println("!! WARNING: Tilt angle exceeded. Motors stopped.");
    delay(200);
    return;
  } else {
    digitalWrite(ledWarningPin, LOW); // Tắt LED nếu an toàn
  }

  // ----- PID cho Pitch -----
  float targetPitch = 0;
  errorPitch = targetPitch - pitch;
  integralPitch += errorPitch * deltaTime;
  float derivativePitch = (errorPitch - previousErrorPitch) / deltaTime;
  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // ----- PID cho Roll -----
  float targetRoll = 0;
  errorRoll = targetRoll - roll;
  integralRoll += errorRoll * deltaTime;
  float derivativeRoll = (errorRoll - previousErrorRoll) / deltaTime;
  float pidRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // ----- Điều khiển motor -----
  int baseSpeed = 128;
  int motorSpeed1 = constrain(baseSpeed + pidPitch + pidRoll, 0, 255);
  int motorSpeed2 = constrain(baseSpeed + pidPitch - pidRoll, 0, 255);
  int motorSpeed3 = constrain(baseSpeed - pidPitch + pidRoll, 0, 255);
  int motorSpeed4 = constrain(baseSpeed - pidPitch - pidRoll, 0, 255);

  analogWrite(motor1Pin, motorSpeed1);
  analogWrite(motor2Pin, motorSpeed2);
  analogWrite(motor3Pin, motorSpeed3);
  analogWrite(motor4Pin, motorSpeed4);

  // ----- In thông tin -----
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.print(roll);
  Serial.print(" || M1: "); Serial.print(motorSpeed1);
  Serial.print(" | M2: "); Serial.print(motorSpeed2);
  Serial.print(" | M3: "); Serial.print(motorSpeed3);
  Serial.print(" | M4: "); Serial.println(motorSpeed4);

  delay(50);
}
