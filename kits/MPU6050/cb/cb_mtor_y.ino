#include <Wire.h>
#include <MPU6050.h>

// Khai báo chân motor
const int motorPin1 = 25; // Motor 1
const int motorPin2 = 26; // Motor 2

// PID parameters
float Kp = 2.0;
float Ki = 0.0;
float Kd = 0.0;

float previousError = 0;
float integral = 0;

// MPU6050
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch;

// Kalman filter
float kalmanPitch = 0;
float uncertaintyPitch = 2;
const float t = 0.02; // 20ms

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }

  // PWM setup
  ledcSetup(0, 1000, 8); // 1kHz, 8-bit
  ledcSetup(1, 1000, 8);
  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
}

// Kalman filter đơn giản cho 1 chiều
void kalman_1d(float& state, float& uncertainty, float input, float measurement) {
  state = state + t * input;
  uncertainty += t * t * 16; // process noise (4^2)
  float gain = uncertainty / (uncertainty + 9); // measurement noise (3^2)
  state = state + gain * (measurement - state);
  uncertainty *= (1 - gain);
}

void loop() {
  // Đọc dữ liệu từ MPU6050
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  float rawPitch = 0;
  float denominator = sqrt(ax * ax + az * az);
  if (denominator > 0.0001) {
    rawPitch = atan2(ay, denominator) * 180.0 / PI;
  }

  // Kalman filter
  kalman_1d(kalmanPitch, uncertaintyPitch, gx / 131.0, rawPitch);
  pitch = kalmanPitch;

  // PID controller
  float error = 0 - pitch;
  integral += error;
  integral = constrain(integral, -400, 400);
  float derivative = error - previousError;
  previousError = error;

  float pidOutput = Kp * error + Ki * integral + Kd * derivative;

  // Tính tốc độ motor
  int speed1 = constrain(128 + pidOutput, 0, 255);
  int speed2 = constrain(128 - pidOutput, 0, 255);

  ledcWrite(0, speed1);
  ledcWrite(1, speed2);

  // Serial Monitor (nếu cần)
  Serial.print("Pitch: "); Serial.print(pitch);
  Serial.print("\tM1: "); Serial.print(speed1);
  Serial.print("\tM2: "); Serial.println(speed2);

  delay(20); // loop every 20ms
}
