#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

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

PID pidR = {7.0, 0.01, 0.08, 0, 0};
PID pidP = {3.0, 0.01, 0.08, 0, 0};
PID pidY = {2.0, 0.0, 0.02, 0, 0};

// ================= TIME =================
unsigned long lastT = 0;

// ================= READ =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read() << 8) | Wire.read();
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

  // lấy góc ban đầu làm ZERO
  float axf = ax / 1500.0 / 16384.0;
  float ayf = ay / 1500.0 / 16384.0;
  float azf = az / 1500.0 / 16384.0;

  roll0  = atan2(ayf, azf) * 57.2958;
  pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

  Serial.println("CALIB DONE");
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

  calibrate();
}

// ================= LOOP =================
void loop(){

  unsigned long now = micros();
  float dt = (now - lastT) * 1e-6;
  lastT = now;
  if(dt <= 0 || dt > 0.05) dt = 0.01;

  // ===== RAW =====
  float gx = (read16(0x43) - gx_o) / 131.0;
  float gy = (read16(0x45) - gy_o) / 131.0;
  float gz = (read16(0x47) - gz_o) / 131.0;

  float ax = read16(0x3B) / 16384.0;
  float ay = read16(0x3D) / 16384.0;
  float az = read16(0x3F) / 16384.0;

  // ===== ACC ANGLE =====
  float accRoll  = atan2(ay, az) * 57.2958 - roll0;
  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

  // ===== GYRO FILTER =====
  gz_f = gz_f * 0.9 + gz * 0.1;

  // ===== BIAS AUTO =====
  gz_bias = gz_bias * 0.999 + gz_f * 0.001;

  float gz_corr = gz_f - gz_bias;

  if(fabs(gz_corr) < 0.02) gz_corr = 0;

  // ===== ANGLES =====
  roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
  pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;

  yaw += gz_corr * dt;

  if(yaw > 180) yaw -= 360;
  if(yaw < -180) yaw += 360;

  // ===== SETPOINT ZERO =====
  float rollSet = 0;
  float pitchSet = 0;
  float yawSet = 0;

  // ===== PID =====
  float rOut = compute(pidR, rollSet, roll, dt);
  float pOut = compute(pidP, pitchSet, pitch, dt);
  float yOut = compute(pidY, yawSet, yaw, dt);

  rOut = constrain(rOut, -40, 40);
  pOut = constrain(pOut, -40, 40);
  yOut = constrain(yOut, -25, 25);

  // ===== DEBUG =====
  Serial.print("R:"); Serial.print(roll);
  Serial.print(" P:"); Serial.print(pitch);
  Serial.print(" Y:"); Serial.println(yaw);

  delay(5);
}