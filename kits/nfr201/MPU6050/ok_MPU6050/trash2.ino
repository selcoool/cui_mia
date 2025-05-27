#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

float pitch, roll;

// PID cho Pitch
float Kp_pitch = 2.0;
float Ki_pitch = 0.05;
float Kd_pitch = 0.5;
float errorPitch, previousErrorPitch = 0, integralPitch = 0;

// PID cho Roll
float Kp_roll = 2.0;
float Ki_roll = 0.05;
float Kd_roll = 0.5;
float errorRoll, previousErrorRoll = 0, integralRoll = 0;

// Motor pins
int motor1Pin = 3;
int motor2Pin = 5;
int motor3Pin = 6;
int motor4Pin = 9;

// Thời gian
unsigned long previousTime = 0;

// Joystick values
int joy1X = 2048, joy1Y = 2048, joy2X = 2048, joy2Y = 2048;

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
}

void loop() {
  readJoystickSerial(); // Đọc giá trị từ joystick

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

  // Target từ joystick
  float targetPitch = map(joy1Y, 0, 4095, 10, -10);
  float targetRoll  = map(joy1X, 0, 4095, -10, 10);
  int baseSpeed = map(joy2Y, 0, 4095, 80, 180);
  baseSpeed = constrain(baseSpeed, 0, 255);

  // PID Pitch
  errorPitch = targetPitch - pitch;
  integralPitch += errorPitch * deltaTime;
  float derivativePitch = (errorPitch - previousErrorPitch) / deltaTime;
  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // PID Roll
  errorRoll = targetRoll - roll;
  integralRoll += errorRoll * deltaTime;
  float derivativeRoll = (errorRoll - previousErrorRoll) / deltaTime;
  float pidRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // Tính tốc độ từng motor
  int motorSpeed1 = constrain(baseSpeed + pidPitch + pidRoll, 0, 255);
  int motorSpeed2 = constrain(baseSpeed + pidPitch - pidRoll, 0, 255);
  int motorSpeed3 = constrain(baseSpeed - pidPitch + pidRoll, 0, 255);
  int motorSpeed4 = constrain(baseSpeed - pidPitch - pidRoll, 0, 255);

  analogWrite(motor1Pin, motorSpeed1);
  analogWrite(motor2Pin, motorSpeed2);
  analogWrite(motor3Pin, motorSpeed3);
  analogWrite(motor4Pin, motorSpeed4);

  // In thông tin
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.print(roll);
  Serial.print(" || M1: "); Serial.print(motorSpeed1);
  Serial.print(" | M2: "); Serial.print(motorSpeed2);
  Serial.print(" | M3: "); Serial.print(motorSpeed3);
  Serial.print(" | M4: "); Serial.println(motorSpeed4);

  delay(20); // Cập nhật 50Hz
}

void readJoystickSerial() {
  static String input = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      int values[4];
      int index = 0;
      char *token = strtok((char *)input.c_str(), ",");
      while (token != NULL && index < 4) {
        values[index++] = atoi(token);
        token = strtok(NULL, ",");
      }

      if (index == 4) {
        joy1X = values[0];
        joy1Y = values[1];
        joy2X = values[2];
        joy2Y = values[3];
      }
      input = "";
    } else {
      input += c;
    }
  }
}
