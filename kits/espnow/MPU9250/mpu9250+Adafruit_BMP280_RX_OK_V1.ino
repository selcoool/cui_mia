

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <math.h>


// ================= PIN =================
#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

#define M1_PIN 4
#define M2_PIN 5
#define M3_PIN 6
#define M4_PIN 3


int trimM1 = 0;
int trimM2 = 0;
int trimM3 = 5;
int trimM4 = 5;

// ================= RX =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= FAILSAFE =================
unsigned long lastRX = 0;
bool rxFail = true;
bool prevFail = true;

// ================= ARM =================
bool lastArm = false;

// ================= STATE =================
float roll = 0, pitch = 0, yaw = 0;
float roll0 = 0, pitch0 = 0;

// ================= GYRO =================
float gx_o = 0, gy_o = 0, gz_o = 0;
float gz_bias = 0;
float gz_f = 0;

// ================= PID =================
struct PID {
  float kp, ki, kd;
  float i, last;
};

PID pidR = {4.5, 0.01, 0.08, 0, 0};
PID pidP = {4.5, 0.01, 0.08, 0, 0};
PID pidY = {0.0, 0.0, 0.02, 0, 0};

// ================= TIME =================
unsigned long lastT = 0;

// ================= READ MPU =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read()<<8) | Wire.read();
}

// ================= MOTOR =================
void motorInit(){
  ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
  ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
  ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
  ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
}

void setMotor(int ch, float val){
  ledcWrite(ch, constrain((int)val, 0, 255));
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len){
  memcpy(&rx, data, sizeof(rx));
  lastRX = millis();
}

// ================= CALIB =================
void calibrate(){
  long gx=0, gy=0, gz=0;
  long ax=0, ay=0, az=0;

  Serial.println("CALIB... KEEP STILL");

  for(int i=0;i<1500;i++){
    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);

    delay(2);
  }

  gx_o = gx / 1500.0;
  gy_o = gy / 1500.0;
  gz_o = gz / 1500.0;

  float axf = ax / 1500.0 / 16384.0;
  float ayf = ay / 1500.0 / 16384.0;
  float azf = az / 1500.0 / 16384.0;

  roll0  = atan2(ayf, azf) * 57.2958;
  pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

  gz_bias = gz_o / 131.0;

  Serial.println("CAL DONE");
}

// ================= PID =================
float compute(PID &p, float set, float in, float dt){
  float e = set - in;

  p.i += e * dt;
  p.i = constrain(p.i, -30, 30);

  float d = (e - p.last) / dt;
  p.last = e;

  return p.kp*e + p.ki*p.i + p.kd*d;
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  motorInit();
  calibrate();

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  lastRX = millis();
}

// ================= LOOP =================
void loop(){

  float dt = (micros() - lastT) * 1e-6;
  lastT = micros();
  if(dt <= 0 || dt > 0.05) dt = 0.01;

  // ================= FAILSAFE =================
  rxFail = (millis() - lastRX > 400);

  if(rxFail){
    setMotor(0,0);
    setMotor(1,0);
    setMotor(2,0);
    setMotor(3,0);

    roll = pitch = yaw = 0;
    gz_bias = 0;

    pidR.i = pidP.i = pidY.i = 0;

    prevFail = true;
    return;
  }

  if(prevFail && !rxFail){
    yaw = 0;
    gz_bias = 0;
    pidR.i = pidP.i = pidY.i = 0;
    prevFail = false;
  }

  // ================= MPU =================
  float gx = (read16(0x43) - gx_o) / 131.0;
  float gy = (read16(0x45) - gy_o) / 131.0;
  float gz = (read16(0x47) - gz_o) / 131.0;

  float ax = read16(0x3B) / 16384.0;
  float ay = read16(0x3D) / 16384.0;
  float az = read16(0x3F) / 16384.0;

  float accRoll  = atan2(ay, az) * 57.2958 - roll0;
  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

  // ================= FILTER =================
  gz_f = gz_f * 0.9 + gz * 0.1;

  // ================= ARM =================
  bool arm = rx.ch[4] > 1600;

  // 🔥 ARM → CALIB
  if(arm && !lastArm){
    Serial.println("ARM -> CALIB");

    calibrate();

    roll = pitch = yaw = 0;
    gz_bias = 0;
    gz_f = 0;

    pidR.i = pidP.i = pidY.i = 0;
    pidR.last = pidP.last = pidY.last = 0;

    delay(500);
  }

  lastArm = arm;

  // ================= AUTO BIAS =================
  if(arm && fabs(gz) < 1.0){
    gz_bias = gz_bias * 0.999 + gz * 0.001;
  }

  float gz_corr = gz_f - gz_bias;
  if(fabs(gz_corr) < 0.02) gz_corr = 0;

  // ================= ANGLE =================
  roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
  pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;
  yaw += gz_corr * dt;

  if(yaw > 180) yaw -= 360;
  if(yaw < -180) yaw += 360;

  // ================= RC =================
  float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

  float rollSet  = (rx.ch[3] - 1500) / 10.0;
  float pitchSet = (rx.ch[2] - 1500) / 10.0;
  float yawSet   = (rx.ch[1] - 1500) / 20.0;

  float m1=0,m2=0,m3=0,m4=0;

  if(arm){
    float rOut = compute(pidR, rollSet, roll, dt);
    float pOut = compute(pidP, pitchSet, pitch, dt);
    float yOut = compute(pidY, yawSet, yaw, dt);

    rOut = constrain(rOut,-40,40);
    pOut = constrain(pOut,-40,40);
    yOut = constrain(yOut,-25,25);

    m1 = throttle + pOut + rOut - yOut;
    m2 = throttle + pOut - rOut + yOut;
    m3 = throttle - pOut - rOut - yOut;
    m4 = throttle - pOut + rOut + yOut;

    // m1 = throttle + pOut + rOut ;
    // m2 = throttle + pOut - rOut ;
    // m3 = throttle - pOut - rOut;
    // m4 = throttle - pOut + rOut ;

    // m1 = throttle + pOut  + trimM1;
    // m2 = throttle + pOut  + trimM2;
    // m3 = throttle - pOut  + trimM3;
    // m4 = throttle - pOut  + trimM4;


//     m1 = throttle + pOut + rOut - yOut + trimM1;
// m2 = throttle + pOut - rOut + yOut + trimM2;
// m3 = throttle - pOut - rOut - yOut + trimM3;
// m4 = throttle - pOut + rOut + yOut + trimM4;


 
  }

  setMotor(0,m1);
  setMotor(1,m2);
  setMotor(2,m3);
  setMotor(3,m4);

  // ================= DEBUG =================
  Serial.print("R:"); Serial.print(roll);
  Serial.print(" P:"); Serial.print(pitch);
  Serial.print(" Y:"); Serial.print(yaw);
  Serial.print(" ARM:"); Serial.print(arm);

  //  Serial.print(" pppppppppp:"); Serial.print( rx.ch[5]);
  Serial.print(" FAIL:"); Serial.print(rxFail);

  Serial.print(" M1:"); Serial.print(m1);
  Serial.print(" M2:"); Serial.print(m2);
  Serial.print(" M3:"); Serial.print(m3);
  Serial.print(" M4:"); Serial.println(m4);
}
