#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>

// ================= ESP-NOW =================
#define CHANNELS 8

typedef struct {
  uint16_t ch[CHANNELS];
} PPMData;

PPMData receivedData;
unsigned long lastRecvTime = 0;

// callback nhận dữ liệu
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  lastRecvTime = millis();
}

// ================= MPU6050 =================
MPU6050 mpu;
int16_t ax, ay, az, gx, gy, gz;
int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// ================= MOTOR =================
#define M1 2
#define M2 3
#define M3 4
#define M4 5

int freq = 400;
int resolution = 10;
int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// ================= PID =================
float Kp_pitch = 0.3, Ki_pitch = 0.01, Kd_pitch = 0.05;
float Kp_roll  = 0.3, Ki_roll  = 0.01, Kd_roll  = 0.05;
float Kp_yaw   = 0.3, Ki_yaw   = 0.005, Kd_yaw   = 0.01;

float prevErrorPitch = 0, integralPitch = 0;
float prevErrorRoll  = 0, integralRoll  = 0;
float prevErrorYaw   = 0, integralYaw   = 0;

// ================= FILTER =================
float pitch = 0, roll = 0, yaw = 0;
unsigned long lastTime;

// ================= CALIB =================
void calibrateMPU() {
  Serial.println("Calibrating MPU...");
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for(int i=0;i<500;i++){
    mpu.getRotation(&gx,&gy,&gz);
    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;
    delay(5);
  }

  gx_offset = gx_sum / 500;
  gy_offset = gy_sum / 500;
  gz_offset = gz_sum / 500;

  Serial.println("Done!");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("RX READY");
  Serial.println(WiFi.macAddress());

  Wire.begin(21,20);
  mpu.initialize();

  if(!mpu.testConnection()) {
    Serial.println("MPU FAIL");
    while(1);
  }

  calibrateMPU();

  pitch = 0; roll = 0; yaw = 0;

  // PWM
  ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
  ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
  ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
  ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

  lastTime = millis();
}

// ================= LOOP =================
void loop() {
  // ===== FAILSAFE =====
  if (millis() - lastRecvTime > 500) {
    ledcWrite(chM1,0);
    ledcWrite(chM2,0);
    ledcWrite(chM3,0);
    ledcWrite(chM4,0);
    Serial.println("NO SIGNAL");
    delay(100);
    return;
  }

  // ===== READ REMOTE =====
  int throttle = map(receivedData.ch[2], 1000, 2000, 0, 255);
  throttle = constrain(throttle, 0, 255);

  int targetPitch = map(receivedData.ch[1], 1000, 2000, -20, 20);
  int targetRoll  = -map(receivedData.ch[0], 1000, 2000, -20, 20);
  int targetYaw   = map(receivedData.ch[3], 1000, 2000, -180, 180); // assume channel 3 for yaw

  // ===== MPU READ =====
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

  if (ax == 0 && ay == 0 && az == 0) {
    Serial.println("MPU no data");
    return;
  }

  float gyroX = (gx - gx_offset)/131.0;
  float gyroY = (gy - gy_offset)/131.0;
  float gyroZ = (gz - gz_offset)/131.0;

  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
  float accRoll  = atan2(ay, az) * 180 / PI;

  unsigned long now = millis();
  float dt = (now - lastTime)/1000.0;
  lastTime = now;

  // ===== FILTER =====
  pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
  roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;
  yaw   += gyroZ*dt; // simple integration for yaw

  // ===== LIMIT =====
  pitch = constrain(pitch, -45, 45);
  roll  = constrain(roll,  -45, 45);

  // ===== PID =====
  // Pitch
  float errorPitch = targetPitch - pitch;
  integralPitch += errorPitch * dt;
  integralPitch = constrain(integralPitch,-50,50);
  float dPitch = (errorPitch - prevErrorPitch)/dt;
  float outPitch = Kp_pitch*errorPitch + Ki_pitch*integralPitch + Kd_pitch*dPitch;
  prevErrorPitch = errorPitch;

  // Roll
  float errorRoll = targetRoll - roll;
  integralRoll += errorRoll * dt;
  integralRoll = constrain(integralRoll,-50,50);
  float dRoll = (errorRoll - prevErrorRoll)/dt;
  float outRoll = Kp_roll*errorRoll + Ki_roll*integralRoll + Kd_roll*dRoll;
  prevErrorRoll = errorRoll;

  // Yaw
  // float errorYaw = targetYaw - yaw;
  // integralYaw += errorYaw * dt;
  // integralYaw = constrain(integralYaw,-180,180);
  // float dYaw = (errorYaw - prevErrorYaw)/dt;
  // float outYaw = Kp_yaw*errorYaw + Ki_yaw*integralYaw + Kd_yaw*dYaw;
  // prevErrorYaw = errorYaw;

  float outYaw =0;

  // ===== CHECK NaN =====
  if (isnan(outPitch)) outPitch = 0;
  if (isnan(outRoll))  outRoll  = 0;
  if (isnan(outYaw))   outYaw   = 0;

  // // ===== MIX (X quad) =====
  // int m1 = throttle + outPitch + outRoll - outYaw;
  // int m2 = throttle + outPitch - outRoll + outYaw;
  // int m3 = throttle - outPitch + outRoll + outYaw;
  // int m4 = throttle - outPitch - outRoll - outYaw;

  // m1 = constrain(m1,0,255);
  // m2 = constrain(m2,0,255);
  // m3 = constrain(m3,0,255);
  // m4 = constrain(m4,0,255);


  //   int m1 = throttle ;
  // int m2 = throttle ;
  // int m3 = throttle ;
  // int m4 = throttle ;

  // int m1 =throttle - outPitch ;
  // int m2 = throttle + outPitch ;
  // int m3 =  throttle + outPitch ;
  // int m4 = throttle - outPitch ;


  // int m1 =throttle - outRoll ;
  // int m2 = throttle - outRoll ;
  // int m3 =  throttle + outRoll ;
  // int m4 = throttle + outRoll ;

  int m1 =throttle - outPitch - outRoll ;
  int m2 = throttle + outPitch - outRoll ;
  int m3 =  throttle + outPitch + outRoll ;
  int m4 = throttle - outPitch + outRoll ;

    m1 = constrain(m1,0,255);
  m2 = constrain(m2,0,255);
  m3 = constrain(m3,0,255);
  m4 = constrain(m4,0,255);


  if (throttle <= 0) m1=m2=m3=m4=0;

  // ===== OUTPUT =====
  ledcWrite(chM1,m1);
  ledcWrite(chM2,m2);
  ledcWrite(chM3,m3);
  ledcWrite(chM4,m4);

  // ===== DEBUG =====
  Serial.print("THR: "); Serial.print(throttle);
  Serial.print(" P: "); Serial.print(pitch,2);
  Serial.print(" R: "); Serial.print(roll,2);
  Serial.print(" Y: "); Serial.print(yaw,2);
  Serial.print(" | M: "); Serial.print(m1); Serial.print(" "); Serial.print(m2);
  Serial.print(" "); Serial.print(m3); Serial.print(" "); Serial.println(m4);

  delay(10);
}
