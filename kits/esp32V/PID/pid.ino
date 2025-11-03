#include <Wire.h>
#include "MPU6050.h"

MPU6050 mpu;

// Motor pins & channels
int motorPins[4] = {13, 12, 14, 27};   // M1, M2, M3, M4
int motorChannels[4] = {0, 1, 2, 3};

// PWM settings
int freq = 400;
int resolution = 12;  // 0-4095
int minPWM = 1000;
int maxPWM = 2000;
int baseThrottle = 1500;

// PID coefficients
float Kp_pitch = 5.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 5.0, Ki_roll  = 0.0, Kd_roll  = 0.0;
float Kp_yaw   = 1.0, Ki_yaw   = 0.0, Kd_yaw   = 0.0;

// PID variables
float pitchError=0, rollError=0, yawError=0;
float pitchPrevError=0, rollPrevError=0, yawPrevError=0;
float pitchIntegral=0, rollIntegral=0, yawIntegral=0;

// Desired angles
float desiredPitch = 0;
float desiredRoll  = 0;
float desiredYaw   = 0;

// Sensor raw
int16_t ax, ay, az, gx, gy, gz;
float pitch=0, roll=0, yaw=0;

// Complementary filter alpha
float alpha = 0.98;

unsigned long lastTime;

float radToDeg(float rad){ return rad*180.0/3.14159265; }

void setup() {
  Serial.begin(115200);
  Wire.begin();

  mpu.initialize();
  // if(!mpu.testConnection()){
  //   Serial.println("MPU6050 connection failed!");
  //   while(1);
  // }

  // Setup motor PWM
  for(int i=0;i<4;i++){
    ledcSetup(motorChannels[i], freq, resolution);
    ledcAttachPin(motorPins[i], motorChannels[i]);
    int duty = map(baseThrottle, 1000, 2000, 0, 4095);
    ledcWrite(motorChannels[i], duty);
  }

  delay(2000); // ESC init
  lastTime = millis();
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime)/1000.0;
  lastTime = now;

  // Read sensor
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // --- Accelerometer angles ---
  float pitchAcc = radToDeg(atan2(ax, sqrt(ay*ay + az*az)));
  float rollAcc  = radToDeg(atan2(ay, sqrt(ax*ax + az*az)));

  // --- Gyro deg/s ---
  float gyroX = gx/131.0;
  float gyroY = gy/131.0;
  float gyroZ = gz/131.0;

  // --- Complementary filter for pitch & roll ---
  pitch = alpha*(pitch + gyroX*dt) + (1-alpha)*pitchAcc;
  roll  = alpha*(roll  + gyroY*dt) + (1-alpha)*rollAcc;
  yaw  += gyroZ*dt;

  // --- PID calculations ---
  pitchError = desiredPitch - pitch;
  pitchIntegral += pitchError*dt;
  float pitchDerivative = (pitchError - pitchPrevError)/dt;
  float pitchPID = Kp_pitch*pitchError + Ki_pitch*pitchIntegral + Kd_pitch*pitchDerivative;
  pitchPrevError = pitchError;

  rollError = desiredRoll - roll;
  rollIntegral += rollError*dt;
  float rollDerivative = (rollError - rollPrevError)/dt;
  float rollPID = Kp_roll*rollError + Ki_roll*rollIntegral + Kd_roll*rollDerivative;
  rollPrevError = rollError;

  yawError = desiredYaw - yaw;
  yawIntegral += yawError*dt;
  float yawDerivative = (yawError - yawPrevError)/dt;
  float yawPID = Kp_yaw*yawError + Ki_yaw*yawIntegral + Kd_yaw*yawDerivative;
  yawPrevError = yawError;

  // --- Compute motor PWM ---
  int motorPWM[4];
  motorPWM[0] = baseThrottle + pitchPID - rollPID + yawPID; // M1 Front CW
  motorPWM[1] = baseThrottle + pitchPID + rollPID - yawPID; // M2 Left CCW
  motorPWM[2] = baseThrottle - pitchPID - rollPID - yawPID; // M3 Right CCW
  motorPWM[3] = baseThrottle - pitchPID + rollPID + yawPID; // M4 Back CW

  // Constrain and write PWM
  for(int i=0;i<4;i++){
    motorPWM[i] = constrain(motorPWM[i], minPWM, maxPWM);
    int duty = map(motorPWM[i], 1000, 2000, 0, 4095);
    ledcWrite(motorChannels[i], duty);
  }

  // --- Serial output ---
  Serial.print("Pitch: "); Serial.print(pitch,2);
  Serial.print(" | Roll: "); Serial.print(roll,2);
  Serial.print(" | Yaw: "); Serial.print(yaw,2);
  Serial.print(" | Motors: ");
  for(int i=0;i<4;i++){
    Serial.print(motorPWM[i]);
    if(i<3) Serial.print(", ");
  }
  Serial.println();

  delay(10); // ~100 Hz
}
