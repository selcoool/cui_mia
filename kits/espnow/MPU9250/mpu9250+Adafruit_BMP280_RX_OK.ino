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

// ================= RC =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= FAILSAFE =================
unsigned long lastRX = 0;
bool rxFail = true;
bool prevFail = true;

// ================= STATE =================
float roll = 0, pitch = 0;
float yaw = 0;

// ================= GYRO =================
float gx_o=0, gy_o=0, gz_o=0;
float gz_bias = 0;

// ================= PID =================
struct PID {
  float kp, ki, kd;
  float i, last;
};

PID pidR = {3.0, 0.01, 0.08, 0, 0};
PID pidP = {3.0, 0.01, 0.08, 0, 0};
PID pidY = {2.0, 0.0, 0.0, 0, 0};

// ================= MPU =================
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

  Serial.println("GYRO CALIB...");

  for(int i=0;i<1500;i++){
    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);
    delay(2);
  }

  gx_o = gx/1500.0;
  gy_o = gy/1500.0;
  gz_o = gz/1500.0;

  gz_bias = gz_o / 131.0;

  Serial.println("CAL DONE");
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

// ================= SETUP =================
unsigned long lastT = 0;

void setup(){
  Serial.begin(115200);

  Wire.setPins(SDA_PIN,SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
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

  float dt = (micros() - lastT) / 1000000.0;
  lastT = micros();

  // ================= FAILSAFE =================
  rxFail = (millis() - lastRX > 400);

  if(rxFail){

    setMotor(0,0);
    setMotor(1,0);
    setMotor(2,0);
    setMotor(3,0);

    roll = 0;
    pitch = 0;
    yaw = 0;

    gz_bias = 0; // 🔥 IMPORTANT RESET

    pidR.i = pidP.i = pidY.i = 0;

    prevFail = true;
    return;
  }

  // ================= RECONNECT =================
  if(prevFail && !rxFail){

    yaw = 0;
    gz_bias = 0;   // 🔥 RESET BIAS

    pidR.i = pidP.i = pidY.i = 0;

    prevFail = false;
  }

  // ================= MPU =================
  float gx = (read16(0x43) - gx_o) / 131.0;
  float gy = (read16(0x45) - gy_o) / 131.0;
  float gz = (read16(0x47) - gz_o) / 131.0;

  // ================= AUTO BIAS LEARN =================
  if (!rxFail) {
    gz_bias = gz_bias * 0.999 + gz * 0.001;
  }

  // ================= YAW FIX CORE =================
  float gz_corr = gz - gz_bias;

  if (fabs(gz_corr) < 0.03) gz_corr = 0;

  yaw += gz_corr * dt;

  // ================= ANGLE =================
  roll  = 0.98 * roll + gx * dt;
  pitch = 0.98 * pitch + gy * dt;

  // ================= RC =================
  bool arm = rx.ch[4] > 1500;

  float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

  float rollSet  = (rx.ch[3] - 1500) / 10.0;
  float pitchSet = (rx.ch[2] - 1500) / 10.0;
  float yawSet   = (rx.ch[1] - 1500) / 20.0;

  float m1=0,m2=0,m3=0,m4=0;

  if(arm){

    float rOut = compute(pidR, rollSet, roll, dt);
    float pOut = compute(pidP, pitchSet, pitch, dt);
    float yOut = compute(pidY, yawSet, yaw, dt);

    rOut = constrain(rOut,-35,35);
    pOut = constrain(pOut,-35,35);
    yOut = constrain(yOut,-25,25);

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
  Serial.print(" BIAS:"); Serial.print(gz_bias);
  Serial.print(" ARM:"); Serial.print(arm);
  Serial.print(" FAIL:"); Serial.println(rxFail);
}