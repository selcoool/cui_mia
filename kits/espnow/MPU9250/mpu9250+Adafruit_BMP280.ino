#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_BMP280.h>

// ================= PIN =================
#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

// ================= MOTOR (LED PWM) =================
#define M1_PIN 4
#define M2_PIN 5
#define M3_PIN 6
#define M4_PIN 3

// ================= BMP =================
Adafruit_BMP280 bmp;
float seaLevel = 1013.25;

// ================= RC =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= STATE =================
float roll=0, pitch=0, yaw=0;
float alt=0, altFiltered=0;

// offsets
float ax_o, ay_o, az_o;
float gx_o, gy_o, gz_o;

unsigned long lastMicros;

// ================= PID =================
struct PID {
  float kp, ki, kd;
  float i, last;
};

PID pidR = {1.4,0,0.04,0,0};
PID pidP = {1.4,0,0.04,0,0};
PID pidY = {2.0,0,0.0,0,0};
PID pidA = {2.0,0.05,0.8,0,0};

// ================= MPU =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  if(Wire.available()<2) return 0;
  return (Wire.read()<<8)|Wire.read();
}

void writeReg(uint8_t reg, uint8_t val){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// ================= PID =================
float compute(PID &p, float set, float in, float dt){
  float e = set - in;
  p.i += e * dt;
  float d = (e - p.last)/dt;
  p.last = e;
  return p.kp*e + p.ki*p.i + p.kd*d;
}

// ================= CALIB =================
void calibrate(){
  long ax=0,ay=0,az=0,gx=0,gy=0,gz=0;

  for(int i=0;i<500;i++){
    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);

    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    delay(2);
  }

  ax_o = ax/500.0;
  ay_o = ay/500.0;
  az_o = az/500.0;

  gx_o = gx/500.0;
  gy_o = gy/500.0;
  gz_o = gz/500.0;
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len){
  memcpy(&rx,data,sizeof(rx));
}

// ================= MOTOR PWM =================
void motorInit(){
  ledcSetup(0, 200, 10); ledcAttachPin(M1_PIN,0);
  ledcSetup(1, 200, 10); ledcAttachPin(M2_PIN,1);
  ledcSetup(2, 200, 10); ledcAttachPin(M3_PIN,2);
  ledcSetup(3, 200, 10); ledcAttachPin(M4_PIN,3);
}

// ================= SET MOTOR =================
void setMotor(int ch, int val){
  ledcWrite(ch, constrain(val,0,255));
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  Wire.setPins(SDA_PIN,SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  writeReg(0x6B,0x00);

  if(!bmp.begin(0x76)){
    Serial.println("BMP FAIL");
    while(1);
  }

  motorInit();
  calibrate();

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  lastMicros = micros();
}

// ================= LOOP =================
void loop(){

  float dt = (micros()-lastMicros)/1000000.0;
  lastMicros = micros();

  // ================= MPU =================
  float ax = read16(0x3B) - ax_o;
  float ay = read16(0x3D) - ay_o;
  float az = read16(0x3F) - az_o;

  float gx = (read16(0x43) - gx_o)/131.0;
  float gy = (read16(0x45) - gy_o)/131.0;
  float gz = (read16(0x47) - gz_o)/131.0;

  // ================= ANGLE =================
  float rollAcc  = atan2(ay,az)*57.3;
  float pitchAcc = atan2(-ax,sqrt(ay*ay+az*az))*57.3;

  roll  = 0.96*(roll + gx*dt) + 0.04*rollAcc;
  pitch = 0.96*(pitch + gy*dt) + 0.04*pitchAcc;
  yaw   += gz*dt;

  // ================= ALT FILTER =================
  alt = bmp.readAltitude(seaLevel);
  altFiltered = 0.9*altFiltered + 0.1*alt;

  // ================= RC INPUT =================
  bool arm = rx.ch[4] > 1500;

  float throttle = rx.ch[0];

  float rollSet  = (rx.ch[3]-1500)/10.0;
  float pitchSet = (rx.ch[2]-1500)/10.0;
  float yawSet   = (rx.ch[1]-1500)/10.0;

  // ================= MOTOR =================
  float m1=0,m2=0,m3=0,m4=0;

  if(arm){

    float rOut = compute(pidR, rollSet, roll, dt);
    float pOut = compute(pidP, pitchSet, pitch, dt);
    float yOut = compute(pidY, yawSet, yaw, dt);

    // ALTITUDE → CHỈ TRIM NHẸ (QUAN TRỌNG)
    float altOut = compute(pidA, 100.0, altFiltered, dt);
    altOut = constrain(altOut, -15, 15);

    throttle += altOut;

    // ================= MIXER X =================
    m1 = throttle + pOut + rOut - yOut;
    m2 = throttle + pOut - rOut + yOut;
    m3 = throttle - pOut - rOut - yOut;
    m4 = throttle - pOut + rOut + yOut;
  }

  setMotor(0,m1);
  setMotor(1,m2);
  setMotor(2,m3);
  setMotor(3,m4);

  // ================= DEBUG =================
  Serial.print("R:"); Serial.print(roll);
  Serial.print(" P:"); Serial.print(pitch);
  Serial.print(" Y:"); Serial.print(yaw);
  Serial.print(" ALT:"); Serial.print(altFiltered);
  Serial.print(" ARM:"); Serial.print(arm);

  Serial.print(" M1:"); Serial.print(m1);
  Serial.print(" M2:"); Serial.print(m2);
  Serial.print(" M3:"); Serial.print(m3);
  Serial.print(" M4:"); Serial.println(m4);

  delay(5);
}
// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Adafruit_BMP280.h>

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// // ================= MOTOR =================
// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3

// // ================= BMP =================
// Adafruit_BMP280 bmp;
// float seaLevel = 1013.25;

// // ================= RX =================
// typedef struct {
//   uint16_t ch[8];
// } PPMData;

// PPMData rx;

// // ================= STATE =================
// float roll=0, pitch=0, yaw=0;
// float alt=0, altFiltered=0;

// float ax_o=0, ay_o=0, az_o=0;
// float gx_o=0, gy_o=0, gz_o=0;

// unsigned long lastMicros;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {1.3,0,0.04,0,0};
// PID pidP = {1.3,0,0.04,0,0};
// PID pidY = {2.0,0,0.0,0,0};
// PID pidA = {2.0,0.3,1.0,0,0}; // altitude PID

// // ================= I2C =================
// void writeReg(uint8_t reg, uint8_t val){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.write(val);
//   Wire.endTransmission();
// }

// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   if(Wire.available()<2) return 0;
//   return (Wire.read()<<8) | Wire.read();
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 200, 10); ledcAttachPin(M1_PIN,0);
//   ledcSetup(1, 200, 10); ledcAttachPin(M2_PIN,1);
//   ledcSetup(2, 200, 10); ledcAttachPin(M3_PIN,2);
//   ledcSetup(3, 200, 10); ledcAttachPin(M4_PIN,3);
// }

// void setMotor(int ch, int val){
//   ledcWrite(ch, val);
// }

// // ================= PID =================
// float compute(PID &p, float set, float inp, float dt){
//   float e = set - inp;
//   p.i += e*dt;
//   float d = (e - p.last)/dt;
//   p.last = e;
//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= CALIB =================
// void calibrate(){
//   long ax=0,ay=0,az=0,gx=0,gy=0,gz=0;

//   for(int i=0;i<500;i++){
//     ax+=read16(0x3B);
//     ay+=read16(0x3D);
//     az+=read16(0x3F);

//     gx+=read16(0x43);
//     gy+=read16(0x45);
//     gz+=read16(0x47);

//     delay(2);
//   }

//   ax_o=ax/500.0;
//   ay_o=ay/500.0;
//   az_o=az/500.0;

//   gx_o=gx/500.0;
//   gy_o=gy/500.0;
//   gz_o=gz/500.0;
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac, const uint8_t *data, int len){
//   memcpy(&rx,data,sizeof(rx));
// }

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN,SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   writeReg(0x6B,0x00);

//   if(!bmp.begin(0x76)){
//     Serial.println("BMP FAIL");
//     while(1);
//   }

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);

//   lastMicros = micros();
// }

// // ================= LOOP =================
// void loop(){

//   float dt = (micros()-lastMicros)/1000000.0;
//   lastMicros = micros();

//   // ================= MPU =================
//   float ax = read16(0x3B)-ax_o;
//   float ay = read16(0x3D)-ay_o;
//   float az = read16(0x3F)-az_o;

//   float gx = (read16(0x43)-gx_o)/131.0;
//   float gy = (read16(0x45)-gy_o)/131.0;
//   float gz = (read16(0x47)-gz_o)/131.0;

//   // ================= ANGLE =================
//   float rollAcc  = atan2(ay,az)*57.3;
//   float pitchAcc = atan2(-ax,sqrt(ay*ay+az*az))*57.3;

//   roll  = 0.96*(roll + gx*dt) + 0.04*rollAcc;
//   pitch = 0.96*(pitch + gy*dt) + 0.04*pitchAcc;
//   yaw   += gz*dt;

//   // ================= ALTITUDE =================
//   alt = bmp.readAltitude(seaLevel);
//   altFiltered = 0.9*altFiltered + 0.1*alt;

//   // ================= ARM =================
//   bool arm = rx.ch[4] > 1500;

//   int throttle = rx.ch[0];

//   float rollIn  = (rx.ch[3]-1500)/10.0;
//   float pitchIn = (rx.ch[2]-1500)/10.0;
//   float yawIn   = (rx.ch[1]-1500)/10.0;

//   // altitude setpoint (CH5 optional)
//   float altSet = 100.0;

//   float m1=0,m2=0,m3=0,m4=0;

//   if(arm){

//     float rOut = compute(pidR,rollIn,roll,dt);
//     float pOut = compute(pidP,pitchIn,pitch,dt);
//     float yOut = compute(pidY,yawIn,yaw,dt);
//     float aOut = compute(pidA,altSet,altFiltered,dt);

//     rOut = constrain(rOut,-40,40);
//     pOut = constrain(pOut,-40,40);
//     yOut = constrain(yOut,-40,40);

//     throttle += aOut; // altitude hold

//     // ================= MIXER =================
//     m1 = throttle + pOut + rOut - yOut;
//     m2 = throttle + pOut - rOut + yOut;
//     m3 = throttle - pOut - rOut - yOut;
//     m4 = throttle - pOut + rOut + yOut;
//   }

//   setMotor(0, constrain(m1,0,255));
//   setMotor(1, constrain(m2,0,255));
//   setMotor(2, constrain(m3,0,255));
//   setMotor(3, constrain(m4,0,255));

//   // ================= DEBUG =================
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.print(yaw);
//   Serial.print(" ALT:"); Serial.print(altFiltered);
//   Serial.print(" ARM:"); Serial.print(arm);

//   Serial.print(" M1:"); Serial.print(m1);
//   Serial.print(" M2:"); Serial.print(m2);
//   Serial.print(" M3:"); Serial.print(m3);
//   Serial.print(" M4:"); Serial.println(m4);

//   delay(5);
// }