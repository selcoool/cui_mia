#include <Wire.h>
#include <MPU6050_light.h>

// MPU6050
MPU6050 mpu(Wire);

// Motor PWM pins
#define MOTOR1 3
#define MOTOR2 5
#define MOTOR3 6
#define MOTOR4 9

// PID constants (tách riêng)
float Kp_pitch = 2.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll = 2.0, Ki_roll = 0.0, Kd_roll = 0.0;

// PID variables
float errorPitch, errorRoll;
float prevErrorPitch = 0, prevErrorRoll = 0;
float integralPitch = 0, integralRoll = 0;

// PWM filter
float filteredM1 = 0, filteredM2 = 0, filteredM3 = 0, filteredM4 = 0;
float alpha = 0.2;

// Base throttle
int baseThrottle = 130;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.begin();
  mpu.calcGyroOffsets(); // Giữ máy phẳng khi chạy dòng này

  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  Serial.println("Pitch\tRoll\tM1\tM2\tM3\tM4");  // Tiêu đề cho Plotter
}

void loop() {
  mpu.update();

  float pitch = mpu.getAngleX();
  float roll  = mpu.getAngleY();

  // === PID PITCH ===
  errorPitch = 0 - pitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - prevErrorPitch;
  float pidPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  prevErrorPitch = errorPitch;

  // === PID ROLL ===
  errorRoll = 0 - roll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - prevErrorRoll;
  float pidRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  prevErrorRoll = errorRoll;

  // === Motor Mix ===
  int m1 = constrain(baseThrottle + pidPitch + pidRoll, 0, 255);
  int m2 = constrain(baseThrottle + pidPitch - pidRoll, 0, 255);
  int m3 = constrain(baseThrottle - pidPitch + pidRoll, 0, 255);
  int m4 = constrain(baseThrottle - pidPitch - pidRoll, 0, 255);

  // === Filter ===
  filteredM1 = (1 - alpha) * filteredM1 + alpha * m1;
  filteredM2 = (1 - alpha) * filteredM2 + alpha * m2;
  filteredM3 = (1 - alpha) * filteredM3 + alpha * m3;
  filteredM4 = (1 - alpha) * filteredM4 + alpha * m4;

  // === Output to motors ===
  analogWrite(MOTOR1, (int)filteredM1);
  analogWrite(MOTOR2, (int)filteredM2);
  analogWrite(MOTOR3, (int)filteredM3);
  analogWrite(MOTOR4, (int)filteredM4);

  // === Serial Plotter ===
  Serial.print(pitch); Serial.print("\t");
  Serial.print(roll);  Serial.print("\t");
  Serial.print((int)filteredM1); Serial.print("\t");
  Serial.print((int)filteredM2); Serial.print("\t");
  Serial.print((int)filteredM3); Serial.print("\t");
  Serial.println((int)filteredM4);

  delay(20);  // ~50Hz update rate
}
