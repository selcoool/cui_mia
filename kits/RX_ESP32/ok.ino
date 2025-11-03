#include <Wire.h>
#include "MPU6050.h"
#include "sbus.h"

MPU6050 mpu;

// Motor pins & channels
int motorPins[4] = {13, 12, 14, 27};
int motorChannels[4] = {0, 1, 2, 3};

// PWM settings
int freq = 400;
int resolution = 12;  // 0-4095
int minPWM = 1000;
int maxPWM = 2000;
int baseThrottle = 1500;

// PID coefficients
float Kp_pitch = 3.0, Ki_pitch = 0.0003, Kd_pitch = 0.3;
float Kp_roll  = 3.0, Ki_roll  = 0.0003, Kd_roll  = 0.3;
float Kp_yaw   = 3.0, Ki_yaw   = 0.0003, Kd_yaw   = 0.1;

// PID variables
float pitchError=0, rollError=0, yawError=0;
float pitchPrevError=0, rollPrevError=0, yawPrevError=0;
float pitchIntegral=0, rollIntegral=0, yawIntegral=0;

// Desired angles & throttle (set từ SBUS)
float desiredPitch = 0;
float desiredRoll  = 0;
float desiredYaw   = 0;
float desiredThrottle = 1500;

// Sensor raw
int16_t ax, ay, az, gx, gy, gz;
float pitch=0, roll=0, yaw=0;

// Complementary filter alpha
float alpha = 0.98;
unsigned long lastTime;

// SBUS setup
#include <HardwareSerial.h>
#define RX1_PIN 16
HardwareSerial SerialSBUS(1);
bfs::SbusRx sbus(&SerialSBUS, RX1_PIN, -1, true);

float radToDeg(float rad){ return rad*180.0/3.14159265; }

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  // Motor setup
  for(int i=0;i<4;i++){
    ledcSetup(motorChannels[i], freq, resolution);
    ledcAttachPin(motorPins[i], motorChannels[i]);
    ledcWrite(motorChannels[i], map(baseThrottle,1000,2000,0,4095));
  }

  // SBUS UART
  SerialSBUS.begin(100000, SERIAL_8E2, RX1_PIN, -1);
  sbus.Begin();

  lastTime = millis();
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime)/1000.0;
  lastTime = now;

  // --- Read SBUS --- 
  if(sbus.Read()){
    const bfs::SbusData &data = sbus.data();
    // Map kênh SBUS sang góc / throttle
    desiredThrottle = map(data.ch[2], 172, 1811, 1000, 2000); // kênh gas
    desiredPitch    = map(data.ch[1], 172, 1811, -10, 10);     // kênh pitch
    desiredRoll     = map(data.ch[0], 172, 1811, -10, 10);     // kênh roll
    desiredYaw      = map(data.ch[3], 172, 1811, -30, 30);     // kênh yaw
  }

  // --- MPU6050 ---
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
  float pitchAcc = radToDeg(atan2(ax, sqrt(ay*ay + az*az)));
  float rollAcc  = radToDeg(atan2(ay, sqrt(ax*ax + az*az)));
  float gyroX = gx/131.0;
  float gyroY = gy/131.0;
  float gyroZ = gz/131.0;

  pitch = alpha*(pitch + gyroX*dt) + (1-alpha)*pitchAcc;
  roll  = alpha*(roll  + gyroY*dt) + (1-alpha)*rollAcc;
  yaw  += gyroZ*dt;

  // --- PID ---
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

  // --- Motor mixing ---
  int motorPWM[4];
  motorPWM[0] = desiredThrottle + pitchPID - rollPID + yawPID;
  motorPWM[1] = desiredThrottle + pitchPID + rollPID - yawPID;
  motorPWM[2] = desiredThrottle - pitchPID - rollPID - yawPID;
  motorPWM[3] = desiredThrottle - pitchPID + rollPID + yawPID;

  for(int i=0;i<4;i++){
    motorPWM[i] = constrain(motorPWM[i], minPWM, maxPWM);
    ledcWrite(motorChannels[i], map(motorPWM[i],1000,2000,0,4095));
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

  delay(10);
}

