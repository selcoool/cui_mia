#include <Wire.h>
#include "MPU6050.h"
#include "sbus.h"
#include <HardwareSerial.h>

MPU6050 mpu;

// Motor pins & channels
int motorPins[4] = {12, 13, 25, 26};   // M1, M2, M3, M4
int motorChannels[4] = {0, 1, 2, 3};

// PWM settings
int freq = 400;
int resolution = 12;  // 0-4095
int minPWM = 1000;
int maxPWM = 2000;

// PID coefficients
float Kp_pitch = 3.0, Ki_pitch = 0.0, Kd_pitch = 0.0;
float Kp_roll  = 3.0, Ki_roll  = 0.0, Kd_roll  = 0.0;
float Kp_yaw   = 3.0, Ki_yaw   = 0.0, Kd_yaw   = 0.0;

// PID variables
float pitchError=0, rollError=0, yawError=0;
float pitchPrevError=0, rollPrevError=0, yawPrevError=0;
float pitchIntegral=0, rollIntegral=0, yawIntegral=0;

// Desired angles & throttle (set từ SBUS)
float desiredPitch = 0;
float desiredRoll  = 0;
float desiredYaw   = 0;
float desiredThrottle = 1000; // safe idle

// Sensor raw
int16_t ax, ay, az, gx, gy, gz;
float pitch=0, roll=0, yaw=0;

// Complementary filter alpha
float alpha = 0.98;

unsigned long lastTime;

// SBUS setup
#define RX1_PIN 16
HardwareSerial SerialSBUS(1);
bfs::SbusRx sbus(&SerialSBUS, RX1_PIN, -1, true);

float radToDeg(float rad){ return rad*180.0/3.14159265; }

void setup() {
  Serial.begin(115200);
  Wire.begin();

  mpu.initialize();

  // Setup motor PWM
  for(int i=0;i<4;i++){
    ledcSetup(motorChannels[i], freq, resolution);
    ledcAttachPin(motorPins[i], motorChannels[i]);
    int duty = map(desiredThrottle, 1000, 2000, 0, 4095);
    ledcWrite(motorChannels[i], duty);
  }

  // SBUS UART
  SerialSBUS.begin(100000, SERIAL_8E2, RX1_PIN, -1);
  sbus.Begin();

  delay(2000); // ESC init
  lastTime = millis();
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastTime)/1000.0;
  lastTime = now;

  // --- SBUS read ---
  bool sbusActive = sbus.Read();
  if(sbusActive){
    const bfs::SbusData &data = sbus.data();

    // Map kênh SBUS sang góc / throttle
    desiredThrottle = map(data.ch[2], 172, 1811, 1000, 2000); // kênh gas
    desiredPitch    = map(data.ch[1], 172, 1811, -10, 10);     // kênh pitch
    desiredRoll     = map(data.ch[0], 172, 1811, -10, 10);     // kênh roll
    desiredYaw      = map(data.ch[3], 172, 1811, -30, 30);     // kênh yaw
  } else {
    // mất tín hiệu SBUS → giữ safe throttle
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
  float gyroZ = gz/131.0;

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
  motorPWM[0] = desiredThrottle + pitchPID + rollPID - yawPID; // M1 Front CW
  motorPWM[1] = desiredThrottle + pitchPID - rollPID + yawPID; // M2 Left CCW
  motorPWM[2] = desiredThrottle - pitchPID - rollPID - yawPID; // M3 Right CCW
  motorPWM[3] = desiredThrottle - pitchPID + rollPID + yawPID; // M4 Back CW

  // Constrain motor PWM strictly between minPWM and maxPWM
  for(int i=0; i<4; i++){
    if( i!=1){
            motorPWM[i] = constrain(motorPWM[i], minPWM, maxPWM);

    // Map to 0-4095 for ESP32 LEDC
    int duty = map(motorPWM[i], minPWM, maxPWM, 0, 4095);
    ledcWrite(motorChannels[i], duty);
    }
   
  }

  // --- Optional: anti-windup ---
  for(int i=0; i<4; i++){
    if(motorPWM[i] == minPWM || motorPWM[i] == maxPWM){
        pitchIntegral -= pitchError*dt;
        rollIntegral  -= rollError*dt;
        yawIntegral   -= yawError*dt;
    }
  }

  // --- Serial output ---
  Serial.print("Throttle: "); Serial.print(desiredThrottle);
  Serial.print(" | Pitch: "); Serial.print(desiredPitch,2);
  Serial.print(" | Roll: "); Serial.print(desiredRoll,2);
  Serial.print(" | Yaw: "); Serial.print(desiredYaw,2);
  Serial.print(" | Motors: ");
  for(int i=0;i<4;i++){
      Serial.print(motorPWM[i]);
      if(i<3) Serial.print(", ");
  }
  Serial.println();

  delay(10); // ~100 Hz
}
