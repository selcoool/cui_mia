#include <Arduino.h>
#include <Wire.h>
#include "MPU6050.h"

// --- MPU6050 ---
MPU6050 mpu(0x68);
#define MPU_INT_PIN 12
volatile bool mpuReady = false;

// --- Motor pins ---
#define MOTOR1_PIN 5
#define MOTOR2_PIN 6
#define MOTOR3_PIN 3
#define MOTOR4_PIN 4

// --- PWM setup ---
int pwmFreq = 500;        // Hz
int pwmResolution = 10;   // 10-bit → 0-1023
int ch1 = 0, ch2 = 1, ch3 = 2, ch4 = 3;
int pwmMax = (1 << pwmResolution) - 1;

// --- Motor speeds ---
int motorSpeed[4] = {0,0,0,0};

// --- PID parameters ---
float Kp=1.0, Ki=0.0, Kd=0.1;
float Kp_yaw=1.0, Ki_yaw=0.0, Kd_yaw=0.05;

// --- PID variables ---
float rollSet=0, pitchSet=0, yawSet=0;
float rollInput=0, pitchInput=0, yawInput=0;
float rollOutput=0, pitchOutput=0, yawOutput=0;
float rollErrorPrev=0, pitchErrorPrev=0, yawErrorPrev=0;
float rollIntegral=0, pitchIntegral=0, yawIntegral=0;

// --- User yaw command ---
float yawCommand = 0; // chỉ thay đổi khi người dùng gửi lệnh

// --- Interrupt ---
void IRAM_ATTR mpuDataReady() { mpuReady = true; }

// --- Set motor PWM ---
void setMotor(int ch, int speed){
  speed = constrain(speed,0,pwmMax);
  ledcWrite(ch,speed);
}

// --- Stop all motors ---
void stopAllMotors(){
  for(int i=0;i<4;i++) motorSpeed[i]=0;
  setMotor(ch1,0); setMotor(ch2,0);
  setMotor(ch3,0); setMotor(ch4,0);
}

// --- Simple PID ---
float computePID(float setpoint, float input, float &integral, float &prevError, float KpVal, float KiVal, float KdVal){
  float error = setpoint - input;
  integral += error * 0.01;           // dt=10ms
  float derivative = (error - prevError)/0.01;
  prevError = error;
  return KpVal*error + KiVal*integral + KdVal*derivative;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- I2C ---
  Wire.begin(11,10);      
  Wire.setClock(400000);

  Serial.println("Init MPU6050...");
  mpu.initialize();
  if(!mpu.testConnection()) Serial.println("MPU ERROR!");
  else Serial.println("MPU OK!");

  // --- Enable Data Ready Interrupt ---
  mpu.setIntDataReadyEnabled(true);

  // --- Motor PWM ---
  ledcSetup(ch1,pwmFreq,pwmResolution);
  ledcSetup(ch2,pwmFreq,pwmResolution);
  ledcSetup(ch3,pwmFreq,pwmResolution);
  ledcSetup(ch4,pwmFreq,pwmResolution);
  ledcAttachPin(MOTOR1_PIN,ch1);
  ledcAttachPin(MOTOR2_PIN,ch2);
  ledcAttachPin(MOTOR3_PIN,ch3);
  ledcAttachPin(MOTOR4_PIN,ch4);

  stopAllMotors();

  // --- INT pin ---
  pinMode(MPU_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), mpuDataReady, RISING);

  Serial.println("Drone ready. Send yaw command (-180 to 180) via Serial.");
}

void loop() {
  // --- đọc lệnh yaw từ Serial ---
  if(Serial.available()){
    int t = Serial.parseInt();
    if(t >= -180 && t <= 180) yawCommand = t;
  }

  if(mpuReady){
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

    // --- Normalize accel to angles ---
    rollInput = atan2(ay,az) * 57.3;
    pitchInput = atan2(-ax,az) * 57.3;

    // --- Yaw: tích hợp gyro (chỉ khi có lệnh) ---
    float dt = 0.01;
    yawInput += 0; // không tự tích lũy
    yawOutput = computePID(yawCommand, yawInput, yawIntegral, yawErrorPrev, Kp_yaw, Ki_yaw, Kd_yaw);

    // --- PID Roll/Pitch ---
    rollOutput = computePID(rollSet, rollInput, rollIntegral, rollErrorPrev, Kp, Ki, Kd);
    pitchOutput = computePID(pitchSet, pitchInput, pitchIntegral, pitchErrorPrev, Kp, Ki, Kd);

    // --- Motor mixing (4 motors + yaw) ---
    motorSpeed[0] = constrain(512 + rollOutput - pitchOutput + yawOutput, 0, pwmMax); // M1
    motorSpeed[1] = constrain(512 - rollOutput - pitchOutput - yawOutput, 0, pwmMax); // M2
    motorSpeed[2] = constrain(512 - rollOutput + pitchOutput + yawOutput, 0, pwmMax); // M3
    motorSpeed[3] = constrain(512 + rollOutput + pitchOutput - yawOutput, 0, pwmMax); // M4

    setMotor(ch1, motorSpeed[0]);
    setMotor(ch2, motorSpeed[1]);
    setMotor(ch3, motorSpeed[2]);
    setMotor(ch4, motorSpeed[3]);

    // --- Debug ---
    Serial.print("ACC: "); Serial.print(ax); Serial.print(", "); Serial.print(ay); Serial.print(", "); Serial.print(az);
    Serial.print(" | Roll="); Serial.print(rollInput); Serial.print(" Pitch="); Serial.print(pitchInput); Serial.print(" YawCmd="); Serial.print(yawCommand);
    Serial.print(" | M1="); Serial.print(motorSpeed[0]);
    Serial.print(" M2="); Serial.print(motorSpeed[1]);
    Serial.print(" M3="); Serial.print(motorSpeed[2]);
    Serial.print(" M4="); Serial.println(motorSpeed[3]);

    mpuReady = false;
  }
}