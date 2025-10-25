#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Motor pins
#define MOTOR1 18
#define MOTOR2 19
#define MOTOR3 23
#define MOTOR4 5

// PWM setup
const int freq = 1000;
const int resolution = 16;

// PID constants (tinh chỉnh riêng cho từng trục)
float Kp_pitch = 1.2, Ki_pitch = 0.0, Kd_pitch = 0.3;
float Kp_roll  = 1.2, Ki_roll  = 0.0, Kd_roll  = 0.3;
float Kp_yaw   = 1.0, Ki_yaw   = 0.0, Kd_yaw   = 0.2;

// PID variables
float integral_pitch = 0, previous_error_pitch = 0;
float integral_roll  = 0, previous_error_roll  = 0;
float integral_yaw   = 0, previous_error_yaw   = 0;

// Drone angles
float pitch = 0.0;
float roll  = 0.0;
float yaw   = 0.0;

// Target angles (thăng bằng)
const float pitch_target = 0.0;
const float roll_target  = 0.0;
const float yaw_target   = 0.0;

// Complementary filter
const float alpha = 0.98;

// Timer
unsigned long timer;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // I2C
  Wire.begin(21,22);
  Wire.setClock(100000);

  // PWM setup
  ledcSetup(0, freq, resolution);
  ledcSetup(1, freq, resolution);
  ledcSetup(2, freq, resolution);
  ledcSetup(3, freq, resolution);
  
  ledcAttachPin(MOTOR1, 0);
  ledcAttachPin(MOTOR2, 1);
  ledcAttachPin(MOTOR3, 2);
  ledcAttachPin(MOTOR4, 3);

  // Initialize MPU
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  Serial.println("MPU6050 initialized!");

  timer = millis();
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  
  // Read sensor
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert to proper units
  float gx_dps = gx / 131.0;
  float gy_dps = gy / 131.0;
  float gz_dps = gz / 131.0;

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // Calculate angles from accelerometer
  float pitchAcc = atan2(ay_g, sqrt(ax_g*ax_g + az_g*az_g)) * 180/PI;
  float rollAcc  = atan2(-ax_g, az_g) * 180/PI;

  // Invert axes if needed
  rollAcc  = -rollAcc;
  gy_dps   = -gy_dps;

  // Failsafe: nếu MPU bị lỗi
  if (isnan(pitchAcc) || isnan(rollAcc) || isnan(gz_dps)) {
    pitchAcc = pitch_target;
    rollAcc  = roll_target;
    gz_dps   = 0;
  }

  // Delta time
  float dt = (millis() - timer)/1000.0;
  timer = millis();

  // Complementary filter
  pitch = alpha * (pitch + gx_dps * dt) + (1-alpha) * pitchAcc;
  roll  = alpha * (roll + gy_dps * dt) + (1-alpha) * rollAcc;
  yaw  += gz_dps * dt;

  // ===== PID =====
  // Calculate errors
  float error_pitch = pitch_target - pitch;
  float error_roll  = roll_target  - roll;
  float error_yaw   = yaw_target   - yaw;

  // Update integrals
  integral_pitch += error_pitch * dt;
  integral_roll  += error_roll  * dt;
  integral_yaw   += error_yaw   * dt;

  // Calculate derivatives
  float derivative_pitch = (error_pitch - previous_error_pitch) / dt;
  float derivative_roll  = (error_roll  - previous_error_roll)  / dt;
  float derivative_yaw   = (error_yaw   - previous_error_yaw)   / dt;

  // PID outputs
  float out_pitch = Kp_pitch * error_pitch + Ki_pitch * integral_pitch + Kd_pitch * derivative_pitch;
  float out_roll  = Kp_roll  * error_roll  + Ki_roll  * integral_roll  + Kd_roll  * derivative_roll;
  float out_yaw   = Kp_yaw   * error_yaw   + Ki_yaw   * integral_yaw   + Kd_yaw   * derivative_yaw;

  // Store errors for next loop
  previous_error_pitch = error_pitch;
  previous_error_roll  = error_roll;
  previous_error_yaw   = error_yaw;

  // ===== Motor outputs =====
  int baseThrottle = 1200; // 1000-2000
  int ch1 = baseThrottle - out_pitch + out_roll - out_yaw; // MOTOR1
  int ch2 = baseThrottle - out_pitch - out_roll + out_yaw; // MOTOR2
  int ch3 = baseThrottle + out_pitch - out_roll - out_yaw; // MOTOR3
  int ch4 = baseThrottle + out_pitch + out_roll + out_yaw; // MOTOR4

  // Constrain PWM
  ch1 = constrain(ch1, 1000, 2000);
  ch2 = constrain(ch2, 1000, 2000);
  ch3 = constrain(ch3, 1000, 2000);
  ch4 = constrain(ch4, 1000, 2000);

  // Map to 16-bit PWM
  ledcWrite(0, map(ch1, 1000, 2000, 0, 65535));
  ledcWrite(1, map(ch2, 1000, 2000, 0, 65535));
  ledcWrite(2, map(ch3, 1000, 2000, 0, 65535));
  ledcWrite(3, map(ch4, 1000, 2000, 0, 65535));

 Serial.print("Pitch: "); Serial.print(pitch,2);
Serial.print(" | Roll: "); Serial.print(roll,2);
Serial.print(" | Yaw: "); Serial.print(yaw,2);

Serial.print(" | PWM1: "); Serial.print(ch1);
Serial.print(" | PWM2: "); Serial.print(ch2);
Serial.print(" | PWM3: "); Serial.print(ch3);
Serial.print(" | PWM4: "); Serial.println(ch4);

  delay(20); // 50Hz loop
}