#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>

// ================= PWM (giống analogWrite) =================
#define PWM_FREQ 1000
#define PWM_RES  8

#define M1 2  // front left
#define M2 3  // front right
#define M3 4  // rear right
#define M4 5  // rear left

int chM1=0, chM2=1, chM3=2, chM4=3;

// ================= ESP-NOW =================
#define CHANNELS 8
typedef struct { uint16_t ch[CHANNELS]; } PPMData;

PPMData receivedData;
unsigned long lastRecvTime=0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&receivedData,incomingData,sizeof(receivedData));
  lastRecvTime=millis();
}

// ================= MPU =================
MPU6050 mpu;
int16_t ax,ay,az,gx,gy,gz;
int16_t gx_offset=0, gy_offset=0;

// ================= PID =================
float Kp_pitch = 0.8;
float Ki_pitch = 0.02;
float Kd_pitch = 0.08;

float Kp_roll  = 0.8;
float Ki_roll  = 0.02;
float Kd_roll  = 0.08;

float prevErrorPitch=0, integralPitch=0;
float prevErrorRoll =0, integralRoll =0;

// ================= FILTER =================
float pitch=0, roll=0;
unsigned long lastTime;

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  Wire.begin(21,20);
  mpu.initialize();

  // PWM setup
  ledcSetup(chM1,PWM_FREQ,PWM_RES); ledcAttachPin(M1,chM1);
  ledcSetup(chM2,PWM_FREQ,PWM_RES); ledcAttachPin(M2,chM2);
  ledcSetup(chM3,PWM_FREQ,PWM_RES); ledcAttachPin(M3,chM3);
  ledcSetup(chM4,PWM_FREQ,PWM_RES); ledcAttachPin(M4,chM4);

  lastTime=millis();
}

// ================= LOOP =================
void loop(){

  // FAILSAFE
  if(millis()-lastRecvTime>500){
    ledcWrite(chM1,0);
    ledcWrite(chM2,0);
    ledcWrite(chM3,0);
    ledcWrite(chM4,0);
    return;
  }

  // ===== RC =====
  int throttle = map(receivedData.ch[2],1000,2000,0,255);

  int targetPitch = map(receivedData.ch[1],1000,2000,-20,20);
  int targetRoll  = -map(receivedData.ch[0],1000,2000,-20,20);

  // ===== MPU =====
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

  float gyroX = (gx-gx_offset)/131.0;
  float gyroY = (gy-gy_offset)/131.0;

  float accPitch = atan2(-ax, sqrt(ay*ay+az*az))*180/PI;
  float accRoll  = atan2(ay, az)*180/PI;

  float dt=(millis()-lastTime)/1000.0;
  lastTime=millis();

  if(dt <= 0) return;

  // FILTER
  pitch = 0.98*(pitch+gyroX*dt)+0.02*accPitch;
  roll  = 0.98*(roll +gyroY*dt)+0.02*accRoll;

  // ===== PID =====

  // Pitch
  float errorPitch = targetPitch - pitch;
  integralPitch += errorPitch * dt;
  integralPitch = constrain(integralPitch,-50,50);

  float dPitch = (errorPitch - prevErrorPitch)/dt;

  float outPitch = 
      Kp_pitch * errorPitch +
      Ki_pitch * integralPitch +
      Kd_pitch * dPitch;

  prevErrorPitch = errorPitch;

  // Roll
  float errorRoll = targetRoll - roll;
  integralRoll += errorRoll * dt;
  integralRoll = constrain(integralRoll,-50,50);

  float dRoll = (errorRoll - prevErrorRoll)/dt;

  float outRoll = 
      Kp_roll * errorRoll +
      Ki_roll * integralRoll +
      Kd_roll * dRoll;

  prevErrorRoll = errorRoll;

  // LIMIT PID
  outPitch = constrain(outPitch,-80,80);
  outRoll  = constrain(outRoll,-80,80);

  // ===== MIX (M1 M2 / M4 M3) =====
  int m1 = throttle + (-outPitch - outRoll);
  int m2 = throttle + ( outPitch - outRoll);
  int m3 = throttle + ( outPitch + outRoll);
  int m4 = throttle + (-outPitch + outRoll);

  // CLAMP
  m1 = constrain(m1,0,255);
  m2 = constrain(m2,0,255);
  m3 = constrain(m3,0,255);
  m4 = constrain(m4,0,255);

  if(throttle==0) m1=m2=m3=m4=0;

  // OUTPUT
  ledcWrite(chM1,m1);
  ledcWrite(chM2,m2);
  ledcWrite(chM3,m3);
  ledcWrite(chM4,m4);

  // DEBUG
  Serial.print("P:");Serial.print(pitch,2);
  Serial.print(" R:");Serial.print(roll,2);
  Serial.print(" M:");
  Serial.print(m1);Serial.print(" ");
  Serial.print(m2);Serial.print(" ");
  Serial.print(m3);Serial.print(" ");
  Serial.println(m4);

  delay(10);
}



// #include <Arduino.h>
// #include <esp_now.h>
// #include <WiFi.h>
// #include <Wire.h>
// #include <MPU6050.h>

// // ================= PWM giống analogWrite =================
// #define PWM_FREQ 1000   // 1kHz (mạnh hơn 400Hz)
// #define PWM_RES  8      // 0–255

// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5

// int chM1=0, chM2=1, chM3=2, chM4=3;

// // ================= ESP-NOW =================
// #define CHANNELS 8
// typedef struct { uint16_t ch[CHANNELS]; } PPMData;

// PPMData receivedData;
// unsigned long lastRecvTime=0;

// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
//   memcpy(&receivedData,incomingData,sizeof(receivedData));
//   lastRecvTime=millis();
// }

// // ================= MPU =================
// MPU6050 mpu;
// int16_t ax,ay,az,gx,gy,gz;
// int16_t gx_offset=0, gy_offset=0;

// // ================= PID =================
// float Kp=0.6, Kd=0.05;
// float prevPitch=0, prevRoll=0;

// float pitch=0, roll=0;
// unsigned long lastTime;

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(OnDataRecv);

//   Wire.begin(21,20);
//   mpu.initialize();

//   // PWM setup (giống analogWrite)
//   ledcSetup(chM1,PWM_FREQ,PWM_RES); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2,PWM_FREQ,PWM_RES); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3,PWM_FREQ,PWM_RES); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4,PWM_FREQ,PWM_RES); ledcAttachPin(M4,chM4);

//   lastTime=millis();
// }

// // ================= LOOP =================
// void loop(){

//   // FAILSAFE
//   if(millis()-lastRecvTime>500){
//     ledcWrite(chM1,0);
//     ledcWrite(chM2,0);
//     ledcWrite(chM3,0);
//     ledcWrite(chM4,0);
//     return;
//   }

//   // ===== RC =====
//   int throttle = map(receivedData.ch[2],1000,2000,0,255);

//   int targetPitch = map(receivedData.ch[1],1000,2000,-20,20);
//   int targetRoll  = -map(receivedData.ch[0],1000,2000,-20,20);

//   // ===== MPU =====
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   float gyroX = (gx-gx_offset)/131.0;
//   float gyroY = (gy-gy_offset)/131.0;

//   float accPitch = atan2(-ax, sqrt(ay*ay+az*az))*180/PI;
//   float accRoll  = atan2(ay, az)*180/PI;

//   float dt=(millis()-lastTime)/1000.0;
//   lastTime=millis();

//   pitch = 0.98*(pitch+gyroX*dt)+0.02*accPitch;
//   roll  = 0.98*(roll +gyroY*dt)+0.02*accRoll;

//   // ===== PID =====
//   float ePitch = targetPitch - pitch;
//   float eRoll  = targetRoll  - roll;

//   float dPitch = (ePitch-prevPitch)/dt;
//   float dRoll  = (eRoll-prevRoll)/dt;

//   float outPitch = Kp*ePitch + Kd*dPitch;
//   float outRoll  = Kp*eRoll  + Kd*dRoll;

//   prevPitch=ePitch;
//   prevRoll=eRoll;

//   // LIMIT
//   outPitch = constrain(outPitch,-80,80);
//   outRoll  = constrain(outRoll,-80,80);

//   // ===== MIX (M1 M2 / M4 M3) =====
//   int m1 = throttle + (-outPitch - outRoll);
//   int m2 = throttle + ( outPitch - outRoll);
//   int m3 = throttle + ( outPitch + outRoll);
//   int m4 = throttle + (-outPitch + outRoll);

//   // CLAMP
//   m1 = constrain(m1,0,255);
//   m2 = constrain(m2,0,255);
//   m3 = constrain(m3,0,255);
//   m4 = constrain(m4,0,255);

//   if(throttle==0) m1=m2=m3=m4=0;

//   // OUTPUT giống analogWrite
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // DEBUG
//   Serial.print("THR:");
//   Serial.print(throttle);
//   Serial.print(" M:");
//   Serial.print(m1);Serial.print(" ");
//   Serial.print(m2);Serial.print(" ");
//   Serial.print(m3);Serial.print(" ");
//   Serial.println(m4);

//   delay(10);
// }
// #include <Arduino.h>
// #include <esp_now.h>
// #include <WiFi.h>
// #include <Wire.h>
// #include <MPU6050.h>

// // ================= ESP-NOW =================
// #define CHANNELS 8

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData receivedData;
// unsigned long lastRecvTime = 0;

// // callback nhận dữ liệu
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));
//   lastRecvTime = millis();
// }

// // ================= MPU6050 =================
// MPU6050 mpu;
// int16_t ax, ay, az, gx, gy, gz;
// int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// // ================= MOTOR =================
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5

// int freq = 400;
// int resolution = 10;
// int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// // ================= PID =================
// float Kp_pitch = 2, Ki_pitch = 0.01, Kd_pitch = 0.05;
// float Kp_roll  = 2, Ki_roll  = 0.01, Kd_roll  = 0.05;
// float Kp_yaw   = 0.3, Ki_yaw   = 0.005, Kd_yaw   = 0.01;

// float prevErrorPitch = 0, integralPitch = 0;
// float prevErrorRoll  = 0, integralRoll  = 0;
// float prevErrorYaw   = 0, integralYaw   = 0;

// // ================= FILTER =================
// float pitch = 0, roll = 0, yaw = 0;
// unsigned long lastTime;

// // ================= CALIB =================
// void calibrateMPU() {
//   Serial.println("Calibrating MPU...");
//   long gx_sum = 0, gy_sum = 0, gz_sum = 0;

//   for(int i=0;i<500;i++){
//     mpu.getRotation(&gx,&gy,&gz);
//     gx_sum += gx;
//     gy_sum += gy;
//     gz_sum += gz;
//     delay(5);
//   }

//   gx_offset = gx_sum / 500;
//   gy_offset = gy_sum / 500;
//   gz_offset = gz_sum / 500;

//   Serial.println("Done!");
// }

// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(OnDataRecv);

//   Serial.println("RX READY");
//   Serial.println(WiFi.macAddress());

//   Wire.begin(21,20);
//   mpu.initialize();

//   if(!mpu.testConnection()) {
//     Serial.println("MPU FAIL");
//     while(1);
//   }

//   calibrateMPU();

//   pitch = 0; roll = 0; yaw = 0;

//   // PWM
//   ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

//   lastTime = millis();
// }

// // ================= LOOP =================
// void loop() {
//   // ===== FAILSAFE =====
//   if (millis() - lastRecvTime > 500) {
//     ledcWrite(chM1,0);
//     ledcWrite(chM2,0);
//     ledcWrite(chM3,0);
//     ledcWrite(chM4,0);
//     Serial.println("NO SIGNAL");
//     delay(100);
//     return;
//   }

//   // ===== READ REMOTE =====
//   int throttle = map(receivedData.ch[2], 1000, 2000, 0, 255);
//   throttle = constrain(throttle, 0, 255);

//   int targetPitch = map(receivedData.ch[1], 1000, 2000, -20, 20);
//   int targetRoll  = -map(receivedData.ch[0], 1000, 2000, -20, 20);
//   int targetYaw   = map(receivedData.ch[3], 1000, 2000, -180, 180); // assume channel 3 for yaw

//   // ===== MPU READ =====
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   if (ax == 0 && ay == 0 && az == 0) {
//     Serial.println("MPU no data");
//     return;
//   }

//   float gyroX = (gx - gx_offset)/131.0;
//   float gyroY = (gy - gy_offset)/131.0;
//   float gyroZ = (gz - gz_offset)/131.0;

//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
//   float accRoll  = atan2(ay, az) * 180 / PI;

//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // ===== FILTER =====
//   pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
//   roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;
//   yaw   += gyroZ*dt; // simple integration for yaw

//   // ===== LIMIT =====
//   pitch = constrain(pitch, -45, 45);
//   roll  = constrain(roll,  -45, 45);

//   // ===== PID =====
//   // Pitch
//   float errorPitch = targetPitch - pitch;
//   integralPitch += errorPitch * dt;
//   integralPitch = constrain(integralPitch,-50,50);
//   float dPitch = (errorPitch - prevErrorPitch)/dt;
//   float outPitch = Kp_pitch*errorPitch + Ki_pitch*integralPitch + Kd_pitch*dPitch;
//   prevErrorPitch = errorPitch;

//   // Roll
//   float errorRoll = targetRoll - roll;
//   integralRoll += errorRoll * dt;
//   integralRoll = constrain(integralRoll,-50,50);
//   float dRoll = (errorRoll - prevErrorRoll)/dt;
//   float outRoll = Kp_roll*errorRoll + Ki_roll*integralRoll + Kd_roll*dRoll;
//   prevErrorRoll = errorRoll;

//   // Yaw
//   // float errorYaw = targetYaw - yaw;
//   // integralYaw += errorYaw * dt;
//   // integralYaw = constrain(integralYaw,-180,180);
//   // float dYaw = (errorYaw - prevErrorYaw)/dt;
//   // float outYaw = Kp_yaw*errorYaw + Ki_yaw*integralYaw + Kd_yaw*dYaw;
//   // prevErrorYaw = errorYaw;

//   float outYaw =0;

//   // ===== CHECK NaN =====
//   if (isnan(outPitch)) outPitch = 0;
//   if (isnan(outRoll))  outRoll  = 0;
//   if (isnan(outYaw))   outYaw   = 0;


//   int m1 =throttle - outPitch - outRoll ;
//   int m2 = throttle + outPitch - outRoll ;
//   int m3 =  throttle + outPitch + outRoll ;
//   int m4 = throttle - outPitch + outRoll ;

//     m1 = constrain(m1,0,255);
//   m2 = constrain(m2,0,255);
//   m3 = constrain(m3,0,255);
//   m4 = constrain(m4,0,255);


//   if (throttle <= 0) m1=m2=m3=m4=0;

//   // ===== OUTPUT =====
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // ===== DEBUG =====
//   Serial.print("THR: "); Serial.print(throttle);
//   Serial.print(" P: "); Serial.print(pitch,2);
//   Serial.print(" R: "); Serial.print(roll,2);
//   Serial.print(" Y: "); Serial.print(yaw,2);
//   Serial.print(" | M: "); Serial.print(m1); Serial.print(" "); Serial.print(m2);
//   Serial.print(" "); Serial.print(m3); Serial.print(" "); Serial.println(m4);

//   delay(10);
// }



// #include <Arduino.h>
// #include <esp_now.h>
// #include <WiFi.h>
// #include <Wire.h>
// #include <MPU6050.h>

// // ================= ESP-NOW =================
// #define CHANNELS 8

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData receivedData;
// unsigned long lastRecvTime = 0;

// // callback nhận dữ liệu
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));
//   lastRecvTime = millis();
// }

// // ================= MPU6050 =================
// MPU6050 mpu;
// int16_t ax, ay, az, gx, gy, gz;
// int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// // ================= MOTOR =================
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5

// int freq = 400;
// int resolution = 10;
// int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// // ================= PID =================
// float Kp_pitch = 2, Ki_pitch = 0.01, Kd_pitch = 0.05;
// float Kp_roll  = 2, Ki_roll  = 0.01, Kd_roll  = 0.05;
// float Kp_yaw   = 0.3, Ki_yaw   = 0.005, Kd_yaw   = 0.01;

// float prevErrorPitch = 0, integralPitch = 0;
// float prevErrorRoll  = 0, integralRoll  = 0;
// float prevErrorYaw   = 0, integralYaw   = 0;

// // ================= FILTER =================
// float pitch = 0, roll = 0, yaw = 0;
// unsigned long lastTime;

// // ================= CALIB =================
// void calibrateMPU() {
//   Serial.println("Calibrating MPU...");
//   long gx_sum = 0, gy_sum = 0, gz_sum = 0;

//   for(int i=0;i<500;i++){
//     mpu.getRotation(&gx,&gy,&gz);
//     gx_sum += gx;
//     gy_sum += gy;
//     gz_sum += gz;
//     delay(5);
//   }

//   gx_offset = gx_sum / 500;
//   gy_offset = gy_sum / 500;
//   gz_offset = gz_sum / 500;

//   Serial.println("Done!");
// }

// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(OnDataRecv);

//   Serial.println("RX READY");
//   Serial.println(WiFi.macAddress());

//   Wire.begin(21,20);
//   mpu.initialize();

//   if(!mpu.testConnection()) {
//     Serial.println("MPU FAIL");
//     while(1);
//   }

//   calibrateMPU();

//   pitch = 0; roll = 0; yaw = 0;

//   // PWM
//   ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

//   lastTime = millis();
// }

// // ================= LOOP =================
// void loop() {
//   // ===== FAILSAFE =====
//   if (millis() - lastRecvTime > 500) {
//     ledcWrite(chM1,0);
//     ledcWrite(chM2,0);
//     ledcWrite(chM3,0);
//     ledcWrite(chM4,0);
//     Serial.println("NO SIGNAL");
//     delay(100);
//     return;
//   }

//   // ===== READ REMOTE =====
//   int throttle = map(receivedData.ch[2], 1000, 2000, 0, 255);
//   throttle = constrain(throttle, 0, 255);

//   int targetPitch = map(receivedData.ch[1], 1000, 2000, -20, 20);
//   int targetRoll  = -map(receivedData.ch[0], 1000, 2000, -20, 20);
//   int targetYaw   = map(receivedData.ch[3], 1000, 2000, -180, 180); // assume channel 3 for yaw

//   // ===== MPU READ =====
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   if (ax == 0 && ay == 0 && az == 0) {
//     Serial.println("MPU no data");
//     return;
//   }

//   float gyroX = (gx - gx_offset)/131.0;
//   float gyroY = (gy - gy_offset)/131.0;
//   float gyroZ = (gz - gz_offset)/131.0;

//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
//   float accRoll  = atan2(ay, az) * 180 / PI;

//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // ===== FILTER =====
//   pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
//   roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;
//   yaw   += gyroZ*dt; // simple integration for yaw

//   // ===== LIMIT =====
//   pitch = constrain(pitch, -45, 45);
//   roll  = constrain(roll,  -45, 45);

//   // ===== PID =====
//   // Pitch
//   float errorPitch = targetPitch - pitch;
//   integralPitch += errorPitch * dt;
//   integralPitch = constrain(integralPitch,-50,50);
//   float dPitch = (errorPitch - prevErrorPitch)/dt;
//   float outPitch = Kp_pitch*errorPitch + Ki_pitch*integralPitch + Kd_pitch*dPitch;
//   prevErrorPitch = errorPitch;

//   // Roll
//   float errorRoll = targetRoll - roll;
//   integralRoll += errorRoll * dt;
//   integralRoll = constrain(integralRoll,-50,50);
//   float dRoll = (errorRoll - prevErrorRoll)/dt;
//   float outRoll = Kp_roll*errorRoll + Ki_roll*integralRoll + Kd_roll*dRoll;
//   prevErrorRoll = errorRoll;

//   // Yaw
//   // float errorYaw = targetYaw - yaw;
//   // integralYaw += errorYaw * dt;
//   // integralYaw = constrain(integralYaw,-180,180);
//   // float dYaw = (errorYaw - prevErrorYaw)/dt;
//   // float outYaw = Kp_yaw*errorYaw + Ki_yaw*integralYaw + Kd_yaw*dYaw;
//   // prevErrorYaw = errorYaw;

//   float outYaw =0;

//   // ===== CHECK NaN =====
//   if (isnan(outPitch)) outPitch = 0;
//   if (isnan(outRoll))  outRoll  = 0;
//   if (isnan(outYaw))   outYaw   = 0;

//   // // ===== MIX (X quad) =====
//   // int m1 = throttle + outPitch + outRoll - outYaw;
//   // int m2 = throttle + outPitch - outRoll + outYaw;
//   // int m3 = throttle - outPitch + outRoll + outYaw;
//   // int m4 = throttle - outPitch - outRoll - outYaw;

//   // m1 = constrain(m1,0,255);
//   // m2 = constrain(m2,0,255);
//   // m3 = constrain(m3,0,255);
//   // m4 = constrain(m4,0,255);


//   //   int m1 = throttle ;
//   // int m2 = throttle ;
//   // int m3 = throttle ;
//   // int m4 = throttle ;

//   // int m1 =throttle - outPitch ;
//   // int m2 = throttle + outPitch ;
//   // int m3 =  throttle + outPitch ;
//   // int m4 = throttle - outPitch ;


//   // int m1 =throttle - outRoll ;
//   // int m2 = throttle - outRoll ;
//   // int m3 =  throttle + outRoll ;
//   // int m4 = throttle + outRoll ;

//   int m1 =throttle - outPitch - outRoll ;
//   int m2 = throttle + outPitch - outRoll ;
//   int m3 =  throttle + outPitch + outRoll ;
//   int m4 = throttle - outPitch + outRoll ;

//     m1 = constrain(m1,0,255);
//   m2 = constrain(m2,0,255);
//   m3 = constrain(m3,0,255);
//   m4 = constrain(m4,0,255);


//   if (throttle <= 0) m1=m2=m3=m4=0;

//   // ===== OUTPUT =====
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // ===== DEBUG =====
//   Serial.print("THR: "); Serial.print(throttle);
//   Serial.print(" P: "); Serial.print(pitch,2);
//   Serial.print(" R: "); Serial.print(roll,2);
//   Serial.print(" Y: "); Serial.print(yaw,2);
//   Serial.print(" | M: "); Serial.print(m1); Serial.print(" "); Serial.print(m2);
//   Serial.print(" "); Serial.print(m3); Serial.print(" "); Serial.println(m4);

//   delay(10);
// }

// #include <Arduino.h>
// #include <esp_now.h>
// #include <WiFi.h>
// #include <Wire.h>
// #include <MPU6050.h>

// // ================= ESP-NOW =================
// #define CHANNELS 8

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData receivedData;
// unsigned long lastRecvTime = 0;

// // callback nhận
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));
//   lastRecvTime = millis();
// }

// // ================= MPU6050 =================
// MPU6050 mpu;
// int16_t ax, ay, az, gx, gy, gz;
// int16_t gx_offset = 0, gy_offset = 0;

// // ================= MOTOR =================
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5

// int freq = 400;
// int resolution = 10;
// int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// // ================= PID =================
// float Kp_pitch = 0.3, Kd_pitch = 0.05;
// float Kp_roll  = 0.3, Kd_roll  = 0.05;

// float prevErrorPitch = 0;
// float prevErrorRoll  = 0;

// // ================= FILTER =================
// float pitch = 0, roll = 0;
// unsigned long lastTime;

// // ================= CALIB =================
// void calibrateMPU() {
//   Serial.println("Calibrating MPU...");
//   long gx_sum = 0, gy_sum = 0;

//   for(int i=0;i<500;i++){
//     mpu.getRotation(&gx,&gy,&gz);
//     gx_sum += gx;
//     gy_sum += gy;
//     delay(5);
//   }

//   gx_offset = gx_sum / 500;
//   gy_offset = gy_sum / 500;

//   Serial.println("Done!");
// }

// // ================= SETUP =================
// void setup() {
//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(OnDataRecv);

//   Serial.println("RX READY");
//   Serial.println(WiFi.macAddress());

//   Wire.begin(21,20);
//   mpu.initialize();

//   if(!mpu.testConnection()) {
//     Serial.println("MPU FAIL");
//     while(1);
//   }

//   calibrateMPU();

//   // reset góc
//   pitch = 0;
//   roll = 0;

//   // PWM
//   ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

//   lastTime = millis();
// }

// // ================= LOOP =================
// void loop() {

//   // ===== FAILSAFE =====
//   if (millis() - lastRecvTime > 500) {
//     ledcWrite(chM1,0);
//     ledcWrite(chM2,0);
//     ledcWrite(chM3,0);
//     ledcWrite(chM4,0);
//     Serial.println("NO SIGNAL");
//     delay(100);
//     return;
//   }

//   // ===== READ REMOTE =====
//   int throttle = map(receivedData.ch[2], 1000, 2000, 0, 400);
//   int targetPitch = map(receivedData.ch[1], 1000, 2000, -20, 20);
//   int targetRoll  = -map(receivedData.ch[0], 1000, 2000, -20, 20);

//   // ===== MPU READ =====
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   float gyroX = (gx - gx_offset)/131.0;
//   float gyroY = (gy - gy_offset)/131.0;

//   // ===== FIX GÓC CHUẨN =====
//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
//   float accRoll  = atan2(ay, az) * 180 / PI;

//   // ===== TIME =====
//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // ===== FILTER =====
//   pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
//   roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;

//   // ===== LIMIT =====
//   pitch = constrain(pitch, -45, 45);
//   roll  = constrain(roll,  -45, 45);

//   // ===== PID =====
//   float errorPitch = targetPitch - pitch;
//   float dPitch = (errorPitch - prevErrorPitch)/dt;
//   float outPitch = Kp_pitch*errorPitch + Kd_pitch*dPitch;
//   prevErrorPitch = errorPitch;

//   float errorRoll = targetRoll - roll;
//   float dRoll = (errorRoll - prevErrorRoll)/dt;
//   float outRoll = Kp_roll*errorRoll + Kd_roll*dRoll;
//   prevErrorRoll = errorRoll;

//   // ===== MIX =====
//   int m1 = throttle + outPitch + outRoll;
//   int m2 = throttle + outPitch - outRoll;
//   int m3 = throttle - outPitch - outRoll;
//   int m4 = throttle - outPitch + outRoll;

//   m1 = constrain(m1,0,1023);
//   m2 = constrain(m2,0,1023);
//   m3 = constrain(m3,0,1023);
//   m4 = constrain(m4,0,1023);

//   // ===== OUTPUT =====
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // ===== DEBUG =====
//   Serial.print("THR: "); Serial.print(throttle);
//   Serial.print(" Pitch: "); Serial.print(pitch);
//   Serial.print(" Roll: "); Serial.print(roll);
//   Serial.print(" | M: ");
//   Serial.print(m1); Serial.print(" ");
//   Serial.print(m2); Serial.print(" ");
//   Serial.print(m3); Serial.print(" ");
//   Serial.println(m4);

//   delay(10);
// }
// #include <Wire.h>
// #include <MPU6050.h>

// // ===== MPU6050 =====
// MPU6050 mpu;
// int16_t ax, ay, az, gx, gy, gz;
// int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// // ===== Motor PWM =====
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5
// int freq = 400;      // Hz
// int resolution = 10; // 0-1023
// int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// // ===== PID Parameters (safe values) =====
// float Kp_pitch = 0.5, Ki_pitch = 0.0, Kd_pitch = 0.1;
// float Kp_roll  = 0.5, Ki_roll  = 0.0, Kd_roll  = 0.1;

// float errorPitch = 0, prevErrorPitch = 0, intPitch = 0;
// float errorRoll  = 0, prevErrorRoll  = 0, intRoll  = 0;

// // ===== Complementary Filter =====
// float pitch = 0, roll = 0;
// unsigned long lastTime;

// void calibrateMPU() {
//   Serial.println("Calibrating MPU6050...");
//   long gx_sum = 0, gy_sum = 0, gz_sum = 0;
//   for(int i=0;i<500;i++){
//     mpu.getRotation(&gx,&gy,&gz);
//     gx_sum += gx; gy_sum += gy; gz_sum += gz;
//     delay(5);
//   }
//   gx_offset = gx_sum / 500;
//   gy_offset = gy_sum / 500;
//   gz_offset = gz_sum / 500;
//   Serial.println("Calibration done!");
// }

// void setup() {
//   Serial.begin(115200);
//   Wire.begin(21,20);

//   // Init MPU6050
//   mpu.initialize();
//   if(!mpu.testConnection()) {
//     Serial.println("MPU6050 failed!");
//     while(1);
//   }

//   calibrateMPU();

//   // Init PWM for motors
//   ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

//   // Motors off
//   ledcWrite(chM1,0);
//   ledcWrite(chM2,0);
//   ledcWrite(chM3,0);
//   ledcWrite(chM4,0);

//   lastTime = millis();
// }

// void loop() {
//   // 1. Read MPU6050
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   // Subtract gyro offsets
//   float gyroX = (gx - gx_offset)/131.0;
//   float gyroY = (gy - gy_offset)/131.0;

//   // Acc angles
//   float accPitch = atan2(ay,az)*180/PI;
//   float accRoll  = atan2(-ax,az)*180/PI;

//   // dt
//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // Complementary Filter
//   pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
//   roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;

//   // 2. PID
//   errorPitch = 0 - pitch;
//   intPitch += errorPitch * dt;
//   float dPitch = (errorPitch - prevErrorPitch)/dt;
//   float outPitch = Kp_pitch*errorPitch + Ki_pitch*intPitch + Kd_pitch*dPitch;
//   prevErrorPitch = errorPitch;

//   errorRoll = 0 - roll;
//   intRoll += errorRoll * dt;
//   float dRoll = (errorRoll - prevErrorRoll)/dt;
//   float outRoll = Kp_roll*errorRoll + Ki_roll*intRoll + Kd_roll*dRoll;
//   prevErrorRoll = errorRoll;

//   // 3. Motor mix
//   int throttle = 120; // safe base
//   int m1 = throttle + outPitch + outRoll;
//   int m2 = throttle + outPitch - outRoll;
//   int m3 = throttle - outPitch - outRoll;
//   int m4 = throttle - outPitch + outRoll;

//   // limit PWM
//   m1 = constrain(m1,0,1023);
//   m2 = constrain(m2,0,1023);
//   m3 = constrain(m3,0,1023);
//   m4 = constrain(m4,0,1023);

//   // 4. Write PWM
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // Debug
//   Serial.print("Pitch: "); Serial.print(pitch);
//   Serial.print(" Roll: "); Serial.print(roll);
//   Serial.print(" | Motors: ");
//   Serial.print(m1); Serial.print(" ");
//   Serial.print(m2); Serial.print(" ");
//   Serial.print(m3); Serial.print(" ");
//   Serial.println(m4);

//   delay(10);
// }

// #include <esp_now.h>
// #include <WiFi.h>

// #define CHANNELS 8

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData data;

// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

//   memcpy(&data,incomingData,sizeof(data));

//   Serial.print("RX CH: ");

//   for(int i=0;i<CHANNELS;i++){
//     Serial.print(data.ch[i]);
//     Serial.print(" ");
//   }

//   Serial.println();
// }

// void setup() {

//   Serial.begin(115200);
//   delay(2000);

//   Serial.println("BOOT OK");

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   Serial.print("MAC: ");
//   Serial.println(WiFi.macAddress());

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_recv_cb(OnDataRecv);

//   Serial.println("ESP-NOW RX READY");
// }

// void loop() {

// }
// void setup() {
//   Serial.begin(115200);
//   pinMode(PPM_PIN, INPUT);
//   attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmISR, RISING);
// }

// void loop() {
//   static uint32_t lastPrint = 0;
//   if (millis() - lastPrint > 100) { // in ra 10 lần/giây
//     lastPrint = millis();
//     Serial.print("Channels: ");
//     for (uint8_t i = 0; i < CHANNELS; i++) {
//       Serial.print(channels[i]);
//       Serial.print(" ");
//     }
//     Serial.println();
//   }
// }
// #include <esp_now.h>
// #include <WiFi.h>

// // Pins joystick (thường bạn có thể kết nối sau)
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// uint8_t receiverMAC[] = {0x18, 0x8B, 0x0E, 0x92, 0x66, 0x34};

// typedef struct {
//   int16_t joy1X;
//   int16_t joy1Y;
//   bool joy1Btn;
//   int16_t joy2X;
//   int16_t joy2Y;
//   bool joy2Btn;
// } JoystickData;

// JoystickData joystick;

// // Callback khi gửi dữ liệu
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 1;  // cố định kênh
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Add peer failed");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {
//   // Giả lập joystick (trong thực tế bạn sẽ đọc analog)
//   joystick.joy1X = 2000;
//   joystick.joy1Y = 2000;
//   joystick.joy1Btn = false;
//   joystick.joy2X = 2000;
//   joystick.joy2Y = 2000;
//   joystick.joy2Btn = false;

//   // In Serial
//   Serial.print("J1 X: "); Serial.print(joystick.joy1X);
//   Serial.print(" Y: "); Serial.print(joystick.joy1Y);
//   Serial.print(" Btn: "); Serial.print(joystick.joy1Btn);

//   Serial.print(" | J2 X: "); Serial.print(joystick.joy2X);
//   Serial.print(" Y: "); Serial.print(joystick.joy2Y);
//   Serial.print(" Btn: "); Serial.println(joystick.joy2Btn);

//   // Gửi dữ liệu
//   esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&joystick, sizeof(joystick));

//   // Nếu fail, delay lâu hơn để tránh bận CPU quá
//   if(result != ESP_OK) {
//     Serial.println("Error sending data, slowing down...");
//     delay(200);  // tăng delay khi fail
//   } else {
//     delay(50);   // bình thường ~20 lần/giây
//   }
// }
// #include <esp_now.h>
// #include <WiFi.h>

// // Joystick pins
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// // MAC RX ESP32-C3 Super Mini
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34}; // thay bằng MAC thực tế

// typedef struct {
//   int16_t joy1X;
//   int16_t joy1Y;
//   bool joy1Btn;
//   int16_t joy2X;
//   int16_t joy2Y;
//   bool joy2Btn;
// } JoystickData;

// JoystickData joystick;
// JoystickData lastJoystick = {0};

// int applyDeadzone(int val, int center, int deadzone=10) {
//   if(abs(val - center) < deadzone) return center;
//   return val;
// }

// bool isChanged(JoystickData a, JoystickData b) {
//   return a.joy1X != b.joy1X || a.joy1Y != b.joy1Y || a.joy1Btn != b.joy1Btn ||
//          a.joy2X != b.joy2X || a.joy2Y != b.joy2Y || a.joy2Btn != b.joy2Btn;
// }

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   if(esp_now_init() != ESP_OK){
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if(esp_now_add_peer(&peerInfo) != ESP_OK){
//     Serial.println("Add peer failed");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {
//   joystick.joy1X = applyDeadzone(analogRead(JOY1_X), 1965, 10);
//   joystick.joy1Y = applyDeadzone(analogRead(JOY1_Y), 2025, 10);
//   joystick.joy1Btn = digitalRead(JOY1_B) == LOW;

//   joystick.joy2X = applyDeadzone(analogRead(JOY2_X), 1990, 10);
//   joystick.joy2Y = applyDeadzone(analogRead(JOY2_Y), 1940, 10);
//   joystick.joy2Btn = digitalRead(JOY2_B) == LOW;

//   if(isChanged(joystick, lastJoystick)){
//     esp_now_send(receiverMAC, (uint8_t *)&joystick, sizeof(joystick));
//     lastJoystick = joystick;
//   }

//   delay(20);
// }


// #include <esp_now.h>
// #include <WiFi.h>

// // Pins joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// // MAC RX (ESP32-C3)
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// // Struct dữ liệu
// typedef struct {
//   int16_t joy1X;
//   int16_t joy1Y;
//   bool joy1Btn;
//   int16_t joy2X;
//   int16_t joy2Y;
//   bool joy2Btn;
// } JoystickData;

// JoystickData joystick;

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   // Cấu hình pins
//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Add peer failed");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {
//   // Đọc joystick 1
//   joystick.joy1X = analogRead(JOY1_X);
//   joystick.joy1Y = analogRead(JOY1_Y);
//   joystick.joy1Btn = digitalRead(JOY1_B) == LOW;

//   // Đọc joystick 2
//   joystick.joy2X = analogRead(JOY2_X);
//   joystick.joy2Y = analogRead(JOY2_Y);
//   joystick.joy2Btn = digitalRead(JOY2_B) == LOW;

//   // Gửi dữ liệu qua ESP-NOW
//   esp_now_send(receiverMAC, (uint8_t *)&joystick, sizeof(joystick));

//   delay(50); // gửi ~20 lần/giây (có thể giảm delay để tăng tốc)
// }



// #include <esp_now.h>
// #include <WiFi.h>

// // MAC của RX (ESP32-C3)
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// // 18:8B:0E:92:66:34

// // Callback khi gửi dữ liệu
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   Serial.println("TX START");

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Add peer failed");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {
//   char msg[] = "HELLO ESP32 C3";

//   // Gửi dữ liệu
//   esp_now_send(receiverMAC, (uint8_t *)msg, sizeof(msg));

//   Serial.println("SEND");

//   delay(1000);
// }



// #include <esp_now.h>
// #include <WiFi.h>

// uint8_t receiverMAC[] = {0x34,0x85,0x18,0xAA,0xBB,0xCC}; // đổi MAC RX

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {

//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);

//   Serial.println("ESP-NOW TX START");

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Failed to add peer");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {

//   char message[] = "HELLO ESP32 C3";

//   esp_now_send(receiverMAC, (uint8_t *)message, sizeof(message));

//   Serial.println("SEND");

//   delay(1000);
// }














// #include <esp_now.h>
// #include <WiFi.h>

// uint8_t receiverMAC[] = {0x34,0x85,0x18,0xAA,0xBB,0xCC}; // đổi MAC RX

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {

//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);

//   Serial.println("ESP-NOW TX START");

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Failed to add peer");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {

//   char message[] = "HELLO ESP32 C3";

//   esp_now_send(receiverMAC, (uint8_t *)message, sizeof(message));

//   Serial.println("SEND");

//   delay(1000);
// }













// #include <esp_now.h>
// #include <WiFi.h>

// uint8_t receiverMAC[] = {0x34,0x85,0x18,0xAB,0xCD,0xEF};

// void setup() {

//   Serial.begin(115200);
//   WiFi.mode(WIFI_STA);

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error");
//     return;
//   }

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   esp_now_add_peer(&peerInfo);
// }

// void loop() {

//   char msg[] = "HELLO";

//   esp_now_send(receiverMAC, (uint8_t *)msg, sizeof(msg));

//   Serial.println("SEND");

//   delay(1000);
// }
// #include <esp_now.h>
// #include <WiFi.h>

// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

//   char msg[50];
//   memcpy(msg, incomingData, len);

//   Serial.print("Received: ");
//   Serial.println(msg);
// }

// void setup() {

//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Init Failed");
//     return;
//   }

//   esp_now_register_recv_cb(OnDataRecv);
// }

// void loop() {
// }




// #include <Wire.h>
// #include <MPU6050.h>

// // ===== MPU6050 =====
// MPU6050 mpu;
// int16_t ax, ay, az, gx, gy, gz;
// int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

// // ===== Motor PWM =====
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5
// int freq = 400;      // Hz
// int resolution = 10; // 0-1023
// int chM1 = 0, chM2 = 1, chM3 = 2, chM4 = 3;

// // ===== PID Parameters (safe values) =====
// float Kp_pitch = 0.5, Ki_pitch = 0.0, Kd_pitch = 0.1;
// float Kp_roll  = 0.5, Ki_roll  = 0.0, Kd_roll  = 0.1;

// float errorPitch = 0, prevErrorPitch = 0, intPitch = 0;
// float errorRoll  = 0, prevErrorRoll  = 0, intRoll  = 0;

// // ===== Complementary Filter =====
// float pitch = 0, roll = 0;
// unsigned long lastTime;

// void calibrateMPU() {
//   Serial.println("Calibrating MPU6050...");
//   long gx_sum = 0, gy_sum = 0, gz_sum = 0;
//   for(int i=0;i<500;i++){
//     mpu.getRotation(&gx,&gy,&gz);
//     gx_sum += gx; gy_sum += gy; gz_sum += gz;
//     delay(5);
//   }
//   gx_offset = gx_sum / 500;
//   gy_offset = gy_sum / 500;
//   gz_offset = gz_sum / 500;
//   Serial.println("Calibration done!");
// }

// void setup() {
//   Serial.begin(115200);
//   Wire.begin(21,20);

//   // Init MPU6050
//   mpu.initialize();
//   if(!mpu.testConnection()) {
//     Serial.println("MPU6050 failed!");
//     while(1);
//   }

//   calibrateMPU();

//   // Init PWM for motors
//   ledcSetup(chM1, freq, resolution); ledcAttachPin(M1,chM1);
//   ledcSetup(chM2, freq, resolution); ledcAttachPin(M2,chM2);
//   ledcSetup(chM3, freq, resolution); ledcAttachPin(M3,chM3);
//   ledcSetup(chM4, freq, resolution); ledcAttachPin(M4,chM4);

//   // Motors off
//   ledcWrite(chM1,0);
//   ledcWrite(chM2,0);
//   ledcWrite(chM3,0);
//   ledcWrite(chM4,0);

//   lastTime = millis();
// }

// void loop() {
//   // 1. Read MPU6050
//   mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);

//   // Subtract gyro offsets
//   float gyroX = (gx - gx_offset)/131.0;
//   float gyroY = (gy - gy_offset)/131.0;

//   // Acc angles
//   float accPitch = atan2(ay,az)*180/PI;
//   float accRoll  = atan2(-ax,az)*180/PI;

//   // dt
//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // Complementary Filter
//   pitch = 0.98*(pitch + gyroX*dt) + 0.02*accPitch;
//   roll  = 0.98*(roll  + gyroY*dt) + 0.02*accRoll;

//   // 2. PID
//   errorPitch = 0 - pitch;
//   intPitch += errorPitch * dt;
//   float dPitch = (errorPitch - prevErrorPitch)/dt;
//   float outPitch = Kp_pitch*errorPitch + Ki_pitch*intPitch + Kd_pitch*dPitch;
//   prevErrorPitch = errorPitch;

//   errorRoll = 0 - roll;
//   intRoll += errorRoll * dt;
//   float dRoll = (errorRoll - prevErrorRoll)/dt;
//   float outRoll = Kp_roll*errorRoll + Ki_roll*intRoll + Kd_roll*dRoll;
//   prevErrorRoll = errorRoll;

//   // 3. Motor mix
//   int throttle = 0; // safe base
//   int m1 = throttle + outPitch + outRoll;
//   int m2 = throttle + outPitch - outRoll;
//   int m3 = throttle - outPitch - outRoll;
//   int m4 = throttle - outPitch + outRoll;

//   // limit PWM
//   m1 = constrain(m1,0,1023);
//   m2 = constrain(m2,0,1023);
//   m3 = constrain(m3,0,1023);
//   m4 = constrain(m4,0,1023);

//   // 4. Write PWM
//   ledcWrite(chM1,m1);
//   ledcWrite(chM2,m2);
//   ledcWrite(chM3,m3);
//   ledcWrite(chM4,m4);

//   // Debug
//   Serial.print("Pitch: "); Serial.print(pitch);
//   Serial.print(" Roll: "); Serial.print(roll);
//   Serial.print(" | Motors: ");
//   Serial.print(m1); Serial.print(" ");
//   Serial.print(m2); Serial.print(" ");
//   Serial.print(m3); Serial.print(" ");
//   Serial.println(m4);

//   delay(10);
// }

// #include <Arduino.h>
// #define M1 2
// #define M2 3
// #define M3 4
// #define M4 5

// int freq = 1000;
// int resolution = 8;

// void setup() {

//   ledcSetup(0, freq, resolution);
//   ledcAttachPin(M1, 0);

//   ledcSetup(1, freq, resolution);
//   ledcAttachPin(M2, 1);

//   ledcSetup(2, freq, resolution);
//   ledcAttachPin(M3, 2);

//   ledcSetup(3, freq, resolution);
//   ledcAttachPin(M4, 3);
// }

// void loop() {

//   ledcWrite(0, 128);
//   ledcWrite(1, 128);
//   ledcWrite(2, 128);
//   ledcWrite(3, 128);

// }

// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// void setup() {
//   Serial.begin(115200);

//   Wire.begin(21, 20);   // SDA, SCL

//   mpu.initialize();

//   if (mpu.testConnection()) {
//     Serial.println("MPU6050 connected!");
//   } else {
//     Serial.println("MPU6050 connection failed");
//   }
// }

// void loop() {
//   int16_t ax, ay, az;
//   int16_t gx, gy, gz;

//   mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

//   Serial.print("Accel: ");
//   Serial.print(ax); Serial.print(" ");
//   Serial.print(ay); Serial.print(" ");
//   Serial.print(az); Serial.print(" | ");

//   Serial.print("Gyro: ");
//   Serial.print(gx); Serial.print(" ");
//   Serial.print(gy); Serial.print(" ");
//   Serial.println(gz);

//   delay(500);
// }

// #include <Arduino.h>
// #define LED_PIN 8

// void setup() {
//   pinMode(LED_PIN, OUTPUT);
// }

// void loop() {
//   digitalWrite(LED_PIN, HIGH); // bật LED
//   delay(1000);

//   digitalWrite(LED_PIN, LOW);  // tắt LED
//   delay(1000);
// }

// #include <Wire.h>
// #include "MPU6050.h"

// MPU6050 mpu;

// // Motor pins & channels (ESP32)
// // int motorPins[4] = {13, 12, 14, 27};   // M1, M2, M3, M4
// int motorPins[4] = {11, 11, 11, 11};   // M1, M2, M3, M4
// int motorChannels[4] = {0, 1, 2, 3};

// // PWM settings
// int freq = 400;        // Hz
// int resolution = 12;   // 0-4095
// int minPWM = 1000;     // microseconds
// int maxPWM = 2000;     // microseconds
// int baseThrottle = 1500;

// // PID coefficients (tùy chỉnh)
// float Kp_pitch = 5.0, Ki_pitch = 0.0, Kd_pitch = 1.0;
// float Kp_roll  = 5.0, Ki_roll  = 0.0, Kd_roll  = 1.0;

// // PID variables
// float pitchError=0, rollError=0;
// float pitchPrevError=0, rollPrevError=0;
// float pitchIntegral=0, rollIntegral=0;

// // Desired angles (cân bằng)
// float desiredPitch = 0;
// float desiredRoll  = 0;

// // Sensor raw
// int16_t ax, ay, az, gx, gy, gz;
// float pitch=0, roll=0;

// // Complementary filter alpha
// float alpha = 0.98;

// unsigned long lastTime;

// // Chuyển rad -> deg
// float radToDeg(float rad){ return rad*180.0/3.14159265; }

// void setup() {
//   Serial.begin(115200);
//   Wire.begin();

//   Serial.println("🔹 ESP32 + MPU6050 + 4 Motor Test 🔹");

//   mpu.initialize();
//   if(!mpu.testConnection()){
//     Serial.println("❌ MPU6050 connection failed!");
//     while(1);
//   }
//   Serial.println("✅ MPU6050 ready!");

//   // Cấu hình motor PWM
//   for(int i=0;i<4;i++){
//     ledcSetup(motorChannels[i], freq, resolution);
//     ledcAttachPin(motorPins[i], motorChannels[i]);
//     int duty = map(baseThrottle, 1000, 2000, 0, 4095);
//     ledcWrite(motorChannels[i], duty);
//   }

//   delay(2000); // ESC init
//   lastTime = millis();
// }

// void loop() {
//   unsigned long now = millis();
//   float dt = (now - lastTime)/1000.0;
//   lastTime = now;

//   // Đọc dữ liệu MPU6050
//   mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

//   // --- Accelerometer angles ---
//   float pitchAcc = radToDeg(atan2(ax, sqrt(ay*ay + az*az)));
//   float rollAcc  = radToDeg(atan2(ay, sqrt(ax*ax + az*az)));

//   // --- Gyro deg/s ---
//   float gyroX = gx/131.0;
//   float gyroY = gy/131.0;

//   // --- Complementary filter ---
//   pitch = alpha*(pitch + gyroX*dt) + (1-alpha)*pitchAcc;
//   roll  = alpha*(roll  + gyroY*dt) + (1-alpha)*rollAcc;

//   // --- PID calculations ---
//   pitchError = desiredPitch - pitch;
//   pitchIntegral += pitchError*dt;
//   float pitchDerivative = (pitchError - pitchPrevError)/dt;
//   float pitchPID = Kp_pitch*pitchError + Ki_pitch*pitchIntegral + Kd_pitch*pitchDerivative;
//   pitchPrevError = pitchError;

//   rollError = desiredRoll - roll;
//   rollIntegral += rollError*dt;
//   float rollDerivative = (rollError - rollPrevError)/dt;
//   float rollPID = Kp_roll*rollError + Ki_roll*rollIntegral + Kd_roll*rollDerivative;
//   rollPrevError = rollError;

//   // --- Compute motor PWM ---
//   int motorPWM[4];
//   motorPWM[0] = baseThrottle + pitchPID - rollPID; // M1 Front CW
//   motorPWM[1] = baseThrottle + pitchPID + rollPID; // M2 Left CCW
//   motorPWM[2] = baseThrottle - pitchPID - rollPID; // M3 Right CCW
//   motorPWM[3] = baseThrottle - pitchPID + rollPID; // M4 Back CW

//   // Constrain và ghi PWM
//   for(int i=0;i<4;i++){
//     motorPWM[i] = constrain(motorPWM[i], minPWM, maxPWM);
//     int duty = map(motorPWM[i], 1000, 2000, 0, 4095);
//     ledcWrite(motorChannels[i], duty);
//   }

//   // --- Serial debug ---
//   Serial.print("Pitch: "); Serial.print(pitch,2);
//   Serial.print(" | Roll: "); Serial.print(roll,2);
//   Serial.print(" | Motors: ");
//   for(int i=0;i<4;i++){
//     Serial.print(motorPWM[i]);
//     if(i<3) Serial.print(", ");
//   }
//   Serial.println();

//   delay(10); // ~100Hz
// }

// #include <Arduino.h>
// #include <Wire.h>

// void setup() {
//   Wire.begin();
//   Serial.begin(115200);

//   Serial.println("I2C Scanner");
// }

// void loop() {
//   byte error, address;
//   int nDevices = 0;

//   Serial.println("Scanning...");

//   for(address = 1; address < 127; address++) {

//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();

//     if (error == 0) {
//       Serial.print("I2C device found at 0x");
//       Serial.println(address, HEX);
//       nDevices++;
//     }
//   }

//   if (nDevices == 0)
//     Serial.println("No I2C devices found");

//   delay(3000);
// }


// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// void setup() {
//   Serial.begin(115200);
//   Wire.begin();

//   Serial.println("🔹 Arduino Pro Mini + MPU6050 Test 🔹");

//   mpu.initialize();
//   if (!mpu.testConnection()) {
//     Serial.println("❌ MPU6050 not detected!");
//     while (1);
//   }
//   Serial.println("✅ MPU6050 ready!");
// }

// void loop() {
//   int16_t ax, ay, az;
//   int16_t gx, gy, gz;

//   // Đọc giá trị
//   ax = mpu.getAccelerationX();
//   ay = mpu.getAccelerationY();
//   az = mpu.getAccelerationZ();

//   gx = mpu.getRotationX();
//   gy = mpu.getRotationY();
//   gz = mpu.getRotationZ();

//   // In ra Serial
//   Serial.print("Acc: ");
//   Serial.print(ax); Serial.print(", ");
//   Serial.print(ay); Serial.print(", ");
//   Serial.println(az);

//   Serial.print("Gyro: ");
//   Serial.print(gx); Serial.print(", ");
//   Serial.print(gy); Serial.print(", ");
//   Serial.println(gz);

//   Serial.println("-----------------------");

//   delay(100); // 10Hz
// }



// test


// ARDUINO pro mini

// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 7
// #define CSN_PIN 10

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openReadingPipe(0, address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.startListening();

//   Serial.println("✅ RX ready (Arduino Pro Mini)");
//   Serial.println("------------------------------------------------");
// }

// void loop() {
//   if (radio.available()) {
//     JoystickData data;
//     radio.read(&data, sizeof(data));

//     Serial.print("🎮 Joy1: X="); Serial.print(data.joy1X);
//     Serial.print(", Y="); Serial.print(data.joy1Y);
//     Serial.print(" | Btn="); Serial.print(data.joy1Btn);

//     Serial.print(" || Joy2: X="); Serial.print(data.joy2X);
//     Serial.print(", Y="); Serial.print(data.joy2Y);
//     Serial.print(" | Btn="); Serial.println(data.joy2Btn);

//     // RAW DATA
//     Serial.print("📦 RAW DATA: ");
//     Serial.print(data.joy1X); Serial.print(",");
//     Serial.print(data.joy1Y); Serial.print(",");
//     Serial.print(data.joy2X); Serial.print(",");
//     Serial.print(data.joy2Y); Serial.print(",");
//     Serial.print(data.joy1Btn); Serial.print(",");
//     Serial.println(data.joy2Btn);

//     Serial.println("------------------------------------------------");
//   }

//   delay(20); // 50Hz
// }




// ESP32

// #include <SPI.h>
// #include <RF24.h>

// // NRF24 pins on ESP32
// #define CE_PIN 14
// #define CSN_PIN 5

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // Pin joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27

// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openWritingPipe(address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.stopListening();

//   Serial.println("✅ TX ready (ESP32)");
// }

// void loop() {
//   JoystickData data;
//   data.joy1X = analogRead(JOY1_X);
//   data.joy1Y = analogRead(JOY1_Y);
//   data.joy2X = analogRead(JOY2_X);
//   data.joy2Y = analogRead(JOY2_Y);
//   data.joy1Btn = !digitalRead(JOY1_B); // 1 = pressed
//   data.joy2Btn = !digitalRead(JOY2_B);

//   bool ok = radio.write(&data, sizeof(data));

//   Serial.print(ok ? "✅ Sent: " : "❌ Failed: ");
//   Serial.print("Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//   Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);

//   delay(50); // ~20Hz
// }



// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// void setup() {
//   Serial.begin(115200);
//   Wire.begin();
  
//   mpu.initialize();
  
//   if (mpu.testConnection()) {
//     Serial.println("MPU6050 connected!");
//   } else {
//     Serial.println("MPU6050 connection failed!");
//   }
// }

// void loop() {
//   int16_t ax, ay, az;
//   int16_t gx, gy, gz;

//   mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

//   Serial.print("Accel: ");
//   Serial.print(ax); Serial.print(" ");
//   Serial.print(ay); Serial.print(" ");
//   Serial.print(az); Serial.print(" | Gyro: ");
//   Serial.print(gx); Serial.print(" ");
//   Serial.print(gy); Serial.print(" ");
//   Serial.println(gz);

//   delay(500);
// }


// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 7
// #define CSN_PIN 10

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openReadingPipe(0, address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.startListening();

//   Serial.println("✅ RX ready (Arduino Pro Mini)");
//   Serial.println("------------------------------------------------");
// }

// void loop() {
//   if (radio.available()) {
//     JoystickData data;
//     radio.read(&data, sizeof(data));

//     Serial.print("🎮 Joy1: X="); Serial.print(data.joy1X);
//     Serial.print(", Y="); Serial.print(data.joy1Y);
//     Serial.print(" | Btn="); Serial.print(data.joy1Btn);

//     Serial.print(" || Joy2: X="); Serial.print(data.joy2X);
//     Serial.print(", Y="); Serial.print(data.joy2Y);
//     Serial.print(" | Btn="); Serial.println(data.joy2Btn);

//     // RAW DATA
//     Serial.print("📦 RAW DATA: ");
//     Serial.print(data.joy1X); Serial.print(",");
//     Serial.print(data.joy1Y); Serial.print(",");
//     Serial.print(data.joy2X); Serial.print(",");
//     Serial.print(data.joy2Y); Serial.print(",");
//     Serial.print(data.joy1Btn); Serial.print(",");
//     Serial.println(data.joy2Btn);

//     Serial.println("------------------------------------------------");
//   }

//   delay(20); // 50Hz
// }

// #include <Arduino.h>
// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// // Chân PWM cho 4 động cơ
// int motorPin1 = 0;
// int motorPin2 = 1;
// int motorPin3 = 2;
// int motorPin4 = 3;

// // Tham số PID cho pitch và roll
// float Kp_pitch = 5.0, Ki_pitch = 0.0, Kd_pitch = 5.0;
// float Kp_roll  = 5.0, Ki_roll  = 0.0, Kd_roll  = 5.0;

// float previousErrorPitch = 0, integralPitch = 0;
// float previousErrorRoll  = 0, integralRoll  = 0;

// // Biến cảm biến
// int16_t ax, ay, az;
// int16_t gx, gy, gz;
// float pitch, roll;

// void setup() {
//   Serial.begin(115200);

//   // I2C MPU6050
//   Wire.begin(20, 21); // SDA=20, SCL=21
//   Wire.setClock(400000); // 400kHz cho nhanh

//   mpu.initialize();
//   if (mpu.testConnection()) {
//     Serial.println("MPU6050 OK");
//   } else {
//     Serial.println("MPU6050 FAIL");
//   }

//   // Cấu hình PWM cho 4 động cơ
//   ledcSetup(0, 1000, 8); // kênh 0, 1kHz, 8-bit
//   ledcSetup(1, 1000, 8);
//   ledcSetup(2, 1000, 8);
//   ledcSetup(3, 1000, 8);

//   ledcAttachPin(motorPin1, 0);
//   ledcAttachPin(motorPin2, 1);
//   ledcAttachPin(motorPin3, 2);
//   ledcAttachPin(motorPin4, 3);
// }

// void loop() {
//   // Đọc MPU6050
//   mpu.getAcceleration(&ax, &ay, &az);
//   mpu.getRotation(&gx, &gy, &gz);

//   // Tính Pitch & Roll
//   pitch = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;
//   roll  = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0 / PI;

//   Serial.print("Pitch: "); Serial.print(pitch);
//   Serial.print("\tRoll: "); Serial.println(roll);

//   // PID Pitch
//   float errorPitch = 0 - pitch;
//   integralPitch += errorPitch;
//   float derivativePitch = errorPitch - previousErrorPitch;
//   float pidPitch = Kp_pitch*errorPitch + Ki_pitch*integralPitch + Kd_pitch*derivativePitch;
//   previousErrorPitch = errorPitch;

//   // PID Roll
//   float errorRoll = 0 - roll;
//   integralRoll += errorRoll;
//   float derivativeRoll = errorRoll - previousErrorRoll;
//   float pidRoll = Kp_roll*errorRoll + Ki_roll*integralRoll + Kd_roll*derivativeRoll;
//   previousErrorRoll = errorRoll;

//   // Tính PWM động cơ
//   int m1 = constrain(128 + pidPitch + pidRoll, 0, 255);
//   int m2 = constrain(128 - pidPitch + pidRoll, 0, 255);
//   int m3 = constrain(128 + pidPitch - pidRoll, 0, 255);
//   int m4 = constrain(128 - pidPitch - pidRoll, 0, 255);

//   // Gửi PWM
//   ledcWrite(0, m1);
//   ledcWrite(1, m2);
//   ledcWrite(2, m3);
//   ledcWrite(3, m4);

//   Serial.print("Motor1: "); Serial.print(m1);
//   Serial.print(" | Motor2: "); Serial.print(m2);
//   Serial.print(" | Motor3: "); Serial.print(m3);
//   Serial.print(" | Motor4: "); Serial.println(m4);

//   delay(200); // 5Hz
// }



// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 14
// #define CSN_PIN 5

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // Pin joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// #define DEADZONE 247  // vùng chết

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// JoystickData lastData;

// int applyDeadzone(int value) {
//   int center = 2048; // ADC 12bit ESP32 (0-4095)
//   if (abs(value - center) < DEADZONE) return 0;
//   return value - center; // giá trị lệch tâm
// }

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   radio.begin();
//   radio.openWritingPipe(address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.stopListening();

//   Serial.println("✅ TX Ready");
// }

// void loop() {
//   JoystickData data;

//   data.joy1X = applyDeadzone(analogRead(JOY1_X));
//   data.joy1Y = applyDeadzone(analogRead(JOY1_Y));
//   data.joy2X = applyDeadzone(analogRead(JOY2_X));
//   data.joy2Y = applyDeadzone(analogRead(JOY2_Y));
//   data.joy1Btn = !digitalRead(JOY1_B);
//   data.joy2Btn = !digitalRead(JOY2_B);

//   // Chỉ gửi khi có thay đổi
//   if (memcmp(&data, &lastData, sizeof(data)) != 0) {
//     bool ok = radio.write(&data, sizeof(data));
//     lastData = data;
//     Serial.print(ok ? "✅ Sent: " : "❌ Failed: ");
//     Serial.print("Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//     Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);
//   } else {
//     // Khi không thay đổi, vẫn hiển thị Serial
//     Serial.print("📡 No movement | Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//     Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);
//   }

//   delay(20); // 50Hz
// }

// #include <SPI.h>
// #include <RF24.h>

// // NRF24 pins on ESP32
// #define CE_PIN 14
// #define CSN_PIN 5

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // Pin joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27

// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openWritingPipe(address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.stopListening();

//   Serial.println("✅ TX ready (ESP32)");
// }

// void loop() {
//   JoystickData data;
//   data.joy1X = analogRead(JOY1_X);
//   data.joy1Y = analogRead(JOY1_Y);
//   data.joy2X = analogRead(JOY2_X);
//   data.joy2Y = analogRead(JOY2_Y);
//   data.joy1Btn = !digitalRead(JOY1_B); // 1 = pressed
//   data.joy2Btn = !digitalRead(JOY2_B);

//   bool ok = radio.write(&data, sizeof(data));

//   Serial.print(ok ? "✅ Sent: " : "❌ Failed: ");
//   Serial.print("Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//   Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);

//   delay(50); // ~20Hz
// }







// #include <Arduino.h>
// #include <SPI.h>
// #include <RF24.h>

// // ==== NRF24L01 =====
// #define CE_PIN 7
// #define CSN_PIN 10
// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // ==== Struct dữ liệu joystick gửi từ ESP32 ====
// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// // Chân PWM cho từng motor
// int motor1Pin = 3;
// int motor2Pin = 5;
// int motor3Pin = 6;
// int motor4Pin = 9;

// // Tốc độ motor 0-255
// int motorSpeed1 = 120;  
// int motorSpeed2 = 120; 
// int motorSpeed3 = 120;  
// int motorSpeed4 = 120;  

// void setup() {
//   Serial.begin(115200);
//   pinMode(motor1Pin, OUTPUT);
//   pinMode(motor2Pin, OUTPUT);
//   pinMode(motor3Pin, OUTPUT);
//   pinMode(motor4Pin, OUTPUT);
//   Serial.println("✅ 4 Motor PWM Test - Tốc độ khác nhau");
// }

// void loop() {
//   // Gửi PWM cho từng motor
//   analogWrite(motor1Pin, motorSpeed1);
//   analogWrite(motor2Pin, motorSpeed2);
//   analogWrite(motor3Pin, motorSpeed3);
//   analogWrite(motor4Pin, motorSpeed4);

//   // In ra Serial
//   Serial.print("📊 Motor Speeds -> ");
//   Serial.print("M1: "); Serial.print(motorSpeed1);
//   Serial.print(" | M2: "); Serial.print(motorSpeed2);
//   Serial.print(" | M3: "); Serial.print(motorSpeed3);
//   Serial.print(" | M4: "); Serial.println(motorSpeed4);

//   delay(500); // 0.5s
// }






// #include <Arduino.h>

// // Chân PWM cho từng motor
// int motor1Pin = 3;
// int motor2Pin = 5;
// int motor3Pin = 6;
// int motor4Pin = 9;

// // Tốc độ motor 0-255
// int motorSpeed1 = 120;  // motor 1 chậm
// int motorSpeed2 = 120;  // motor 2 nhanh hơn
// int motorSpeed3 = 120;  // motor 3 nhanh hơn nữa
// int motorSpeed4 = 120;  // motor 4 tối đa

// void setup() {
//   Serial.begin(115200);
//   pinMode(motor1Pin, OUTPUT);
//   pinMode(motor2Pin, OUTPUT);
//   pinMode(motor3Pin, OUTPUT);
//   pinMode(motor4Pin, OUTPUT);
//   Serial.println("✅ 4 Motor PWM Test - Tốc độ khác nhau");
// }

// void loop() {
//   // Gửi PWM cho từng motor
//   analogWrite(motor1Pin, motorSpeed1);
//   analogWrite(motor2Pin, motorSpeed2);
//   analogWrite(motor3Pin, motorSpeed3);
//   analogWrite(motor4Pin, motorSpeed4);

//   // In ra Serial
//   Serial.print("📊 Motor Speeds -> ");
//   Serial.print("M1: "); Serial.print(motorSpeed1);
//   Serial.print(" | M2: "); Serial.print(motorSpeed2);
//   Serial.print(" | M3: "); Serial.print(motorSpeed3);
//   Serial.print(" | M4: "); Serial.println(motorSpeed4);

//   delay(500); // 0.5s
// }


// #include <Wire.h>
// #include <MPU6050.h>

// MPU6050 mpu;

// void setup() {
//   Serial.begin(115200);
//   Wire.begin();

//   Serial.println("🔹 Arduino Pro Mini + MPU6050 Test 🔹");

//   mpu.initialize();
//   if (!mpu.testConnection()) {
//     Serial.println("❌ MPU6050 not detected!");
//     while (1);
//   }
//   Serial.println("✅ MPU6050 ready!");
// }

// void loop() {
//   int16_t ax, ay, az;
//   int16_t gx, gy, gz;

//   // Đọc giá trị
//   ax = mpu.getAccelerationX();
//   ay = mpu.getAccelerationY();
//   az = mpu.getAccelerationZ();

//   gx = mpu.getRotationX();
//   gy = mpu.getRotationY();
//   gz = mpu.getRotationZ();

//   // In ra Serial
//   Serial.print("Acc: ");
//   Serial.print(ax); Serial.print(", ");
//   Serial.print(ay); Serial.print(", ");
//   Serial.println(az);

//   Serial.print("Gyro: ");
//   Serial.print(gx); Serial.print(", ");
//   Serial.print(gy); Serial.print(", ");
//   Serial.println(gz);

//   Serial.println("-----------------------");

//   delay(100); // 10Hz
// }



// test


// ARDUINO pro mini

// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 7
// #define CSN_PIN 10

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openReadingPipe(0, address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.startListening();

//   Serial.println("✅ RX ready (Arduino Pro Mini)");
//   Serial.println("------------------------------------------------");
// }

// void loop() {
//   if (radio.available()) {
//     JoystickData data;
//     radio.read(&data, sizeof(data));

//     Serial.print("🎮 Joy1: X="); Serial.print(data.joy1X);
//     Serial.print(", Y="); Serial.print(data.joy1Y);
//     Serial.print(" | Btn="); Serial.print(data.joy1Btn);

//     Serial.print(" || Joy2: X="); Serial.print(data.joy2X);
//     Serial.print(", Y="); Serial.print(data.joy2Y);
//     Serial.print(" | Btn="); Serial.println(data.joy2Btn);

//     // RAW DATA
//     Serial.print("📦 RAW DATA: ");
//     Serial.print(data.joy1X); Serial.print(",");
//     Serial.print(data.joy1Y); Serial.print(",");
//     Serial.print(data.joy2X); Serial.print(",");
//     Serial.print(data.joy2Y); Serial.print(",");
//     Serial.print(data.joy1Btn); Serial.print(",");
//     Serial.println(data.joy2Btn);

//     Serial.println("------------------------------------------------");
//   }

//   delay(20); // 50Hz
// }




// ESP32

// #include <SPI.h>
// #include <RF24.h>

// // NRF24 pins on ESP32
// #define CE_PIN 14
// #define CSN_PIN 5

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // Pin joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27

// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openWritingPipe(address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.stopListening();

//   Serial.println("✅ TX ready (ESP32)");
// }

// void loop() {
//   JoystickData data;
//   data.joy1X = analogRead(JOY1_X);
//   data.joy1Y = analogRead(JOY1_Y);
//   data.joy2X = analogRead(JOY2_X);
//   data.joy2Y = analogRead(JOY2_Y);
//   data.joy1Btn = !digitalRead(JOY1_B); // 1 = pressed
//   data.joy2Btn = !digitalRead(JOY2_B);

//   bool ok = radio.write(&data, sizeof(data));

//   Serial.print(ok ? "✅ Sent: " : "❌ Failed: ");
//   Serial.print("Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//   Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);

//   delay(50); // ~20Hz
// }

