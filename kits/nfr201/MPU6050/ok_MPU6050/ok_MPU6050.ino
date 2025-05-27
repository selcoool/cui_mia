#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

float pitch, roll;
float Kp = 2.0;  // Hệ số tỉ lệ
float Kd = 0.5;  // Hệ số đạo hàm
float previousErrorPitch = 0, previousErrorRoll = 0;

int motor1Pin = 3;
int motor2Pin = 5;
int motor3Pin = 6;
int motor4Pin = 9;

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
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Kiểm tra dữ liệu hợp lệ
  if ((ax == 0 && ay == 0 && az == 0) || isnan(ax) || isnan(ay) || isnan(az)) {
    Serial.println("Invalid sensor data!");
    return;
  }

  // Tính toán Pitch và Roll an toàn
  float denominator_pitch = sqrt((float)ax * ax + (float)az * az);
  float denominator_roll = sqrt((float)ay * ay + (float)az * az);

  if (denominator_pitch != 0 && !isnan(denominator_pitch)) {
    pitch = atan2((float)ay, denominator_pitch) * 180.0 / PI;
  } else {
    pitch = 0;
  }

  if (denominator_roll != 0 && !isnan(denominator_roll)) {
    roll = atan2((float)-ax, denominator_roll) * 180.0 / PI;
  } else {
    roll = 0;
  }

  // PID đơn giản
  float targetPitch = 0, targetRoll = 0;
  float errorPitch = targetPitch - pitch;
  float errorRoll = targetRoll - roll;

  float pidOutputPitch = Kp * errorPitch + Kd * (errorPitch - previousErrorPitch);
  float pidOutputRoll = Kp * errorRoll + Kd * (errorRoll - previousErrorRoll);

  previousErrorPitch = errorPitch;
  previousErrorRoll = errorRoll;

  int baseSpeed = 128;

  // Tính tốc độ động cơ
  int motorSpeed1 = constrain(baseSpeed + pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed2 = constrain(baseSpeed + pidOutputPitch - pidOutputRoll, 0, 255);
  int motorSpeed3 = constrain(baseSpeed - pidOutputPitch + pidOutputRoll, 0, 255);
  int motorSpeed4 = constrain(baseSpeed - pidOutputPitch - pidOutputRoll, 0, 255);

  analogWrite(motor1Pin, motorSpeed1);
  analogWrite(motor2Pin, motorSpeed2);
  analogWrite(motor3Pin, motorSpeed3);
  analogWrite(motor4Pin, motorSpeed4);

  // Hiển thị dữ liệu
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print(" | Roll: "); Serial.print(roll);
  Serial.print(" || Motor1: "); Serial.print(motorSpeed1);
  Serial.print(" | Motor2: "); Serial.print(motorSpeed2);
  Serial.print(" | Motor3: "); Serial.print(motorSpeed3);
  Serial.print(" | Motor4: "); Serial.println(motorSpeed4);

  delay(100); // Delay nhỏ để giảm nhiễu
}
