// [env:esp32-c3-devkitm-1]
// platform = espressif32
// board = esp32-c3-devkitm-1
// framework = arduino

// monitor_speed = 115200
// lib_deps = 
//     RF24
//     https://github.com/bolderflight/sbus.git
//      electroniccats/MPU6050 @ ^0.6.0
// build_flags =
//                 -DARDUINO_USB_CDC_ON_BOOT=1
//                 -DARDUINO_USB_MODE=1


#include <Wire.h>
#include "MPU6050.h"
#include "sbus.h"
#include <HardwareSerial.h>

MPU6050 mpu;

// --- Motor Pins / Channels ---
int motorPins[4] = {4, 5, 8, 9};       
int motorChannels[4] = {0, 1, 2, 3};   
int freq = 400;                         
int resolution = 12;                    
int minPWM = 1000;
int maxPWM = 2000;

// --- PID Constants ---
float Kp_pitch = 3.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 3.0, Ki_roll  = 0.0, Kd_roll  = 0.0;
float Kp_yaw   = 3.0, Ki_yaw   = 0.0, Kd_yaw   = 0.0;

// --- PID variables ---
float pitchError=0, rollError=0, yawError=0;
float pitchPrevError=0, rollPrevError=0, yawPrevError=0;
float pitchIntegral=0, rollIntegral=0, yawIntegral=0;
float pitchDerivativePrev=0, rollDerivativePrev=0, yawDerivativePrev=0;

// --- Desired setpoints ---
float desiredPitch = 0;
float desiredRoll  = 0;
float desiredYaw   = 0;
float desiredThrottle = 1000;

// --- MPU6050 readings ---
int16_t ax, ay, az, gx, gy, gz;
float pitch=0, roll=0, yaw=0;

// --- Complementary filter alpha ---
float alpha = 0.98;
unsigned long lastTime;

// --- Arming ---
bool isArmed = false;

// --- SBUS ---
#define RX1_PIN 3
HardwareSerial SerialSBUS(1);
bfs::SbusRx sbus(&SerialSBUS, RX1_PIN, -1, true);

// --- GyroZ bias ---
float gyroZ_bias = 0;
int biasSamples = 200;

// --- Utility ---
float radToDeg(float rad){ return rad*180.0/3.14159265; }

void calibrateGyroZ() {
  long sum = 0;
  for (int i = 0; i < biasSamples; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sum += gz;
    delay(5);
  }
  gyroZ_bias = sum / (float)biasSamples / 131.0;
  Serial.print("GyroZ bias: "); Serial.println(gyroZ_bias, 4);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(7,6);               
  mpu.initialize();

  // --- Calibrate GyroZ to reduce yaw drift ---
  calibrateGyroZ();

  // --- Setup motors ---
  for(int i=0;i<4;i++){
    ledcSetup(motorChannels[i], freq, resolution);
    ledcAttachPin(motorPins[i], motorChannels[i]);
    ledcWrite(motorChannels[i], map(minPWM, 1000, 2000, 0, 4095));
  }

  // --- Setup SBUS ---
  SerialSBUS.begin(100000, SERIAL_8E2, RX1_PIN, -1);
  sbus.Begin();

  delay(2000);
  lastTime = millis();
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime)/1000.0;
  if (dt <= 0 || dt > 1) dt = 0.01;
  lastTime = now;

  // --- SBUS read ---
  bool sbusActive = sbus.Read();
  if(sbusActive){
    const bfs::SbusData &data = sbus.data();
    desiredThrottle = map(data.ch[2], 172, 1811, 1000, 2000);
    desiredPitch    = map(data.ch[1], 172, 1811, -10, 10);
    desiredRoll     = map(data.ch[0], 172, 1811, -10, 10);
    desiredYaw      = map(data.ch[3], 172, 1811, -30, 30);

    // --- ARM/DISARM ---
    if(data.ch[4] > 1000){ 
      if(!isArmed){
        isArmed = true;
        pitchIntegral = rollIntegral = yawIntegral = 0;
        pitchPrevError = rollPrevError = yawPrevError = 0;
        pitchDerivativePrev = rollDerivativePrev = yawDerivativePrev = 0;
        Serial.println(">> ARMED");
      }
    } else { 
      if(isArmed){
        isArmed = false;
        Serial.println(">> DISARMED");
      }
    }
  } else {
    desiredThrottle = 1000;
    desiredPitch = 0;
    desiredRoll = 0;
    desiredYaw = 0;
  }

  // --- Read MPU6050 ---
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  float pitchAcc = radToDeg(atan2(ax, sqrt(ay*ay + az*az)));
  float rollAcc  = radToDeg(atan2(ay, sqrt(ax*ax + az*az)));
  float gyroX = gx/131.0;
  float gyroY = gy/131.0;
  float gyroZ = gz/131.0 - gyroZ_bias; // <-- trừ bias

  pitch = alpha*(pitch + gyroX*dt) + (1-alpha)*pitchAcc;
  roll  = alpha*(roll  + gyroY*dt) + (1-alpha)*rollAcc;
  yaw  += gyroZ*dt;

  if (isnan(pitch)) pitch = 0;
  if (isnan(roll)) roll = 0;
  if (isnan(yaw)) yaw = 0;

  // --- Reset PID if throttle low ---
  if (desiredThrottle < 1100) {
    pitchIntegral = rollIntegral = yawIntegral = 0;
    pitchPrevError = rollPrevError = yawPrevError = 0;
    pitchDerivativePrev = rollDerivativePrev = yawDerivativePrev = 0;
  }

  // --- PID calculation ---
  pitchError = desiredPitch - pitch;
  pitchIntegral += pitchError*dt;
  float pitchDerivative = (pitchError - pitchPrevError)/dt;
  pitchDerivative = 0.7*pitchDerivativePrev + 0.3*pitchDerivative;
  pitchPrevError = pitchError;
  pitchDerivativePrev = pitchDerivative;
  float pitchPID = Kp_pitch*pitchError + Ki_pitch*pitchIntegral + Kd_pitch*pitchDerivative;

  rollError = desiredRoll - roll;
  rollIntegral += rollError*dt;
  float rollDerivative = (rollError - rollPrevError)/dt;
  rollDerivative = 0.7*rollDerivativePrev + 0.3*rollDerivative;
  rollPrevError = rollError;
  rollDerivativePrev = rollDerivative;
  float rollPID = Kp_roll*rollError + Ki_roll*rollIntegral + Kd_roll*rollDerivative;

  yawError = desiredYaw - yaw;
  yawIntegral += yawError*dt;
  float yawDerivative = (yawError - yawPrevError)/dt;
  yawDerivative = 0.7*yawDerivativePrev + 0.3*yawDerivative;
  yawPrevError = yawError;
  yawDerivativePrev = yawDerivative;
  float yawPID = Kp_yaw*yawError + Ki_yaw*yawIntegral + Kd_yaw*yawDerivative;

  // --- Clamp PID ---
  pitchPID = constrain(pitchPID, -400, 400);
  rollPID  = constrain(rollPID, -400, 400);
  yawPID   = constrain(yawPID, -400, 400);

  // --- Dynamic throttle clamp ---
  float maxPID = max(max(abs(pitchPID), abs(rollPID)), abs(yawPID));
  float throttleLimited = constrain(desiredThrottle, minPWM, maxPWM - maxPID);

  // --- Motor mixing ---
  int motorPWM[4];
  motorPWM[0] = throttleLimited + pitchPID + rollPID - yawPID;
  motorPWM[1] = throttleLimited + pitchPID - rollPID + yawPID;
  motorPWM[2] = throttleLimited - pitchPID - rollPID - yawPID;
  motorPWM[3] = throttleLimited - pitchPID + rollPID + yawPID;

  // --- Apply to motors ---
  for (int i=0; i<4; i++){
    if(!isArmed){
      motorPWM[i] = minPWM;
    } else {
      if (motorPWM[i] > maxPWM || motorPWM[i] < minPWM){
        pitchIntegral -= pitchError*dt;
        rollIntegral  -= rollError*dt;
        yawIntegral   -= yawError*dt;
      }
    }
    motorPWM[i] = constrain(motorPWM[i], minPWM, maxPWM);
    int duty = map(motorPWM[i], 1000, 2000, 0, 4095);
    ledcWrite(motorChannels[i], duty);
  }

  // --- Debug output ---
  Serial.print(isArmed ? "[ARMED] " : "[DISARMED] ");
  Serial.print("Throttle: "); Serial.print(desiredThrottle,2);
  Serial.print(" | PitchPID: "); Serial.print(pitchPID,1);
  Serial.print(" | RollPID: "); Serial.print(rollPID,1);
  Serial.print(" | YawPID: "); Serial.print(yawPID,1);
  Serial.print(" | Motors: ");
  for (int i=0; i<4; i++) {
    Serial.print(motorPWM[i]);
    if (i<3) Serial.print(", ");
  }
  Serial.println();

  delay(10); // ~100Hz loop
}