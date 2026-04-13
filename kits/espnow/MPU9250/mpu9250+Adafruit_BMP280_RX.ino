#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_BMP280.h>

// ================= PIN =================
#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

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
float altF=0;

// ================= OFFSET =================
float ax_o=0, ay_o=0, az_o=0;
float gx_o=0, gy_o=0, gz_o=0;

// ================= PID =================
struct PID {
  float kp, ki, kd;
  float i, last;
};

PID pidR = {3.0, 0.01, 0.08, 0, 0};
PID pidP = {3.0, 0.01, 0.08, 0, 0};
PID pidY = {2.0, 0.0, 0.0, 0, 0};

// ================= MPU READ =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read()<<8) | Wire.read();
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
  p.i = constrain(p.i, -25, 25);

  float d = (e - p.last) / dt;
  p.last = e;

  return p.kp*e + p.ki*p.i + p.kd*d;
}

// ================= CALIBRATION FIXED =================
void calibrate(){
  long ax=0,ay=0,az=0,gx=0,gy=0,gz=0;

  Serial.println("KEEP DRONE FLAT...");

  for(int i=0;i<1500;i++){
    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);

    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    delay(2);
  }

  ax_o = ax/1500.0;
  ay_o = ay/1500.0;
  az_o = (az/1500.0) - 16384;   // 🔥 gravity fix

  gx_o = gx/1500.0;
  gy_o = gy/1500.0;
  gz_o = gz/1500.0;

  Serial.println("CAL DONE");
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len){
  memcpy(&rx, data, sizeof(rx));
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

// ================= SETUP =================
unsigned long lastT=0;

void setup(){
  Serial.begin(115200);

  Wire.setPins(SDA_PIN,SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  writeReg(0x6B,0x00);

  bmp.begin(0x76);

  motorInit();
  calibrate();

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
}

// ================= LOOP =================
void loop(){

  float dt = (micros() - lastT) / 1000000.0;
  lastT = micros();

  // ================= MPU =================
  float ax = (read16(0x3B) - ax_o) / 16384.0;
  float ay = (read16(0x3D) - ay_o) / 16384.0;
  float az = (read16(0x3F) - az_o) / 16384.0;

  float gx = (read16(0x43) - gx_o) / 131.0;
  float gy = (read16(0x45) - gy_o) / 131.0;
  float gz = (read16(0x47) - gz_o) / 131.0;

  // ================= ANGLE =================
  float rollAcc  = atan2(ay, az) * 57.3;
  float pitchAcc = atan2(-ax, sqrt(ay*ay + az*az)) * 57.3;

  // 🔥 STRONGER FILTER (VERY IMPORTANT)
  roll  = 0.985 * (roll + gx * dt) + 0.015 * rollAcc;
  pitch = 0.985 * (pitch + gy * dt) + 0.015 * pitchAcc;

  // yaw drift only (no mag)
  yaw += gz * dt;
  if(fabs(gz) < 0.02) yaw *= 0.999;

  // ================= ALT =================
  altF = 0.9 * altF + 0.1 * bmp.readAltitude(seaLevel);

  // ================= RC =================
  bool arm = rx.ch[4] > 1500;

  float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

  float rollSet  = (rx.ch[3] - 1500) / 10.0;
  float pitchSet = (rx.ch[2] - 1500) / 10.0;
  float yawSet   = (rx.ch[1] - 1500) / 20.0;

  float m1=0,m2=0,m3=0,m4=0;

  if(arm){

    // 🔥 deadband chống rung
    if(abs(roll) < 0.8) roll = 0;
    if(abs(pitch) < 0.8) pitch = 0;

    float rOut = compute(pidR, rollSet, roll, dt);
    float pOut = compute(pidP, pitchSet, pitch, dt);
    float yOut = compute(pidY, yawSet, yaw, dt);

    rOut = constrain(rOut, -35, 35);
    pOut = constrain(pOut, -35, 35);
    yOut = constrain(yOut, -25, 25);

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
  Serial.print(" ALT:"); Serial.print(altF);
  Serial.print(" ARM:"); Serial.print(arm);

  Serial.print(" M1:"); Serial.print(m1);
  Serial.print(" M2:"); Serial.print(m2);
  Serial.print(" M3:"); Serial.print(m3);
  Serial.print(" M4:"); Serial.println(m4);

  delay(5);
}