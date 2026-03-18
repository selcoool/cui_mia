#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ===== MOTOR PINS =====
int motorPin1 = 2; // M1
int motorPin2 = 3; // M2
int motorPin3 = 4; // M3
int motorPin4 = 5; // M4

// ===== PID =====
float Kp_pitch = 2.0, Ki_pitch = 0.02, Kd_pitch = 0.5;
float Kp_roll  = 2.0, Ki_roll  = 0.02, Kd_roll  = 0.5;

float prevErrorPitch = 0, integralPitch = 0;
float prevErrorRoll  = 0, integralRoll  = 0;

// ===== MPU DATA =====
int16_t ax, ay, az;
int16_t gx, gy, gz;
float pitch = 0, roll = 0;

// ===== TIME =====
unsigned long lastTime = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 20);

  Serial.println("Init MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU OK");
  } else {
    Serial.println("MPU FAIL");
  }

  // PWM
  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);

  ledcAttachPin(motorPin1, 0);
  ledcAttachPin(motorPin2, 1);
  ledcAttachPin(motorPin3, 2);
  ledcAttachPin(motorPin4, 3);

  lastTime = millis();
}

// ===== LOOP =====
void loop() {

  // ===== TIME =====
  float dt = (millis() - lastTime) / 1000.0;
  lastTime = millis();
  if (dt <= 0) dt = 0.01;

  // ===== READ MPU =====
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float accTotal = sqrt(ax * ax + ay * ay + az * az);
  if (accTotal == 0) return;

  // ===== ANGLE =====
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // chống NaN
  if (isnan(pitch) || isnan(roll)) return;

  // ===== TARGET =====
  float targetPitch = 0;
  float targetRoll  = 0;

  // ===== PID PITCH =====
  float errorPitch = targetPitch - pitch;

  integralPitch += errorPitch * dt;
  integralPitch = constrain(integralPitch, -50, 50);

  float dPitch = (errorPitch - prevErrorPitch) / dt;

  float outPitch =
    Kp_pitch * errorPitch +
    Ki_pitch * integralPitch +
    Kd_pitch * dPitch;

  prevErrorPitch = errorPitch;

  // ===== PID ROLL =====
  float errorRoll = targetRoll - roll;

  integralRoll += errorRoll * dt;
  integralRoll = constrain(integralRoll, -50, 50);

  float dRoll = (errorRoll - prevErrorRoll) / dt;

  float outRoll =
    Kp_roll * errorRoll +
    Ki_roll * integralRoll +
    Kd_roll * dRoll;

  prevErrorRoll = errorRoll;

  // ===== LIMIT PID =====
  outPitch = constrain(outPitch, -100, 100);
  outRoll  = constrain(outRoll,  -100, 100);

  // ===== BASE THROTTLE =====
  int throttle = 130; // giữ nhẹ

  // ===== MIX =====
  // int m1 = throttle + (-outPitch - outRoll);
  // int m2 = throttle + ( outPitch - outRoll);
  // int m3 = throttle + ( outPitch + outRoll);
  // int m4 = throttle + (-outPitch + outRoll);


  //   int m1 = throttle - outPitch ;
  // int m2 = throttle -  outPitch ;
  // int m3 = throttle + outPitch;
  // int m4 = throttle + outPitch;

  //   int m1 = throttle - outRoll ;
  // int m2 = throttle +  outRoll ;
  // int m3 = throttle + outRoll;
  // int m4 = throttle - outRoll;

  int m1 = throttle  + ( - outPitch - outRoll)  ;
  int m2 = throttle +( -  outPitch  + outRoll);
  int m3 = throttle + (outPitch + outRoll) ;
  int m4 = throttle + (outPitch - outRoll);

  // ===== LIMIT =====
  m1 = constrain(m1, 0, 255);
  m2 = constrain(m2, 0, 255);
  m3 = constrain(m3, 0, 255);
  m4 = constrain(m4, 0, 255);

  // ===== OUTPUT =====
  ledcWrite(0, m1);
  ledcWrite(1, m2);
  ledcWrite(2, m3);
  ledcWrite(3, m4);

  // ===== DEBUG =====
  Serial.print("P:"); Serial.print(pitch, 2);
  Serial.print(" R:"); Serial.print(roll, 2);
  Serial.print(" | M:");
  Serial.print(m1); Serial.print(" ");
  Serial.print(m2); Serial.print(" ");
  Serial.print(m3); Serial.print(" ");
  Serial.println(m4);

  delay(5); // loop nhanh
}