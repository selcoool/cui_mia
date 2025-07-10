#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

// Motor pins
#define MOTOR1 3   // Front Left
#define MOTOR2 5   // Front Right
#define MOTOR3 6   // Rear Right
#define MOTOR4 9   // Rear Left

// PID constants
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 2.0, Ki_roll  = 0.0, Kd_roll  = 0.0;

// PID state variables
float prevErrorPitch = 0, integralPitch = 0;
float prevErrorRoll = 0, integralRoll = 0;

// Output smoothing
float alpha = 0.2;
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;

// Base speed
int baseThrottle = 130;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets();

  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  Serial.println("Pitch\tRoll\tM1\tM2\tM3\tM4");
}

void loop() {
  mpu.update();

  float pitch = mpu.getAngleX();
  float roll  = mpu.getAngleY();

  // === PID pitch ===
  float errorPitch = 0 - pitch;
  integralPitch += errorPitch;
  float dPitch = errorPitch - prevErrorPitch;
  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * dPitch;
  prevErrorPitch = errorPitch;

  // === PID roll ===
  float errorRoll = 0 - roll;
  integralRoll += errorRoll;
  float dRoll = errorRoll - prevErrorRoll;
  float pidRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * dRoll;
  prevErrorRoll = errorRoll;

  // === Motor mixing ===
  int m1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255);  // Front Left
  int m2 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255);  // Front Right
  int m3 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255);  // Rear Right
  int m4 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255);  // Rear Left

  // === Smoothing ===
  filteredM1 = (1 - alpha) * filteredM1 + alpha * m1;
  filteredM2 = (1 - alpha) * filteredM2 + alpha * m2;
  filteredM3 = (1 - alpha) * filteredM3 + alpha * m3;
  filteredM4 = (1 - alpha) * filteredM4 + alpha * m4;

  analogWrite(MOTOR1, (int)filteredM1);
  analogWrite(MOTOR2, (int)filteredM2);
  analogWrite(MOTOR3, (int)filteredM3);
  analogWrite(MOTOR4, (int)filteredM4);

  // === Debug output ===
  // Dành cho Serial Monitor
  Serial.print("Pitch: "); Serial.print(pitch, 2);
  Serial.print(" | Roll: "); Serial.print(roll, 2);
  Serial.print(" | M1: "); Serial.print((int)filteredM1);
  Serial.print(" | M2: "); Serial.print((int)filteredM2);
  Serial.print(" | M3: "); Serial.print((int)filteredM3);
  Serial.print(" | M4: "); Serial.println((int)filteredM4);

  // Dành cho Serial Plotter
  Serial.print(pitch); Serial.print("\t");
  Serial.print(roll); Serial.print("\t");
  Serial.print((int)filteredM1); Serial.print("\t");
  Serial.print((int)filteredM2); Serial.print("\t");
  Serial.print((int)filteredM3); Serial.print("\t");
  Serial.println((int)filteredM4);

  delay(20);  // 50Hz
}
