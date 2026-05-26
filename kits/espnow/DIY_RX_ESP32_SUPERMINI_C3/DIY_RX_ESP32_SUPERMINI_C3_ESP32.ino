#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>

// ================= DATA =================
typedef struct {
  uint16_t ch[4];
} Data;

Data data;

// ================= SIGNAL =================
unsigned long lastRX = 0;
bool rxFail = false;

// ================= MOTOR =================
#define M1 3
#define M2 6
#define M3 4
#define M4 5

// ================= MPU =================
#define MPU_ADDR 0x68

float AccX, AccY, AccZ;
float roll, pitch;
float rollOffset = 0;
float pitchOffset = 0;

// ================= PID =================
float kp = 1.3;
float kd = 0.4;

float iRoll = 0;
float iPitch = 0;

float lastRollErr = 0;
float lastPitchErr = 0;

// ================= MOTOR SMOOTH =================
float m1_f = 0, m2_f = 0, m3_f = 0, m4_f = 0;
float ramp = 8.0; // càng cao càng nhanh

// ================= FAILSAFE =================
void stopMotors() {
  analogWrite(M1, 0);
  analogWrite(M2, 0);
  analogWrite(M3, 0);
  analogWrite(M4, 0);
}

// ================= ESP NOW =================
void onReceive(const uint8_t *mac, const uint8_t *incoming, int len) {
  if (len != sizeof(Data)) return;

  memcpy(&data, incoming, sizeof(data));

  // clamp nhẹ chống spike
  for (int i = 0; i < 4; i++) {
    if (data.ch[i] < 900) data.ch[i] = 900;
    if (data.ch[i] > 2100) data.ch[i] = 2100;
  }

  lastRX = millis();
}

// ================= MPU =================
void readMPU() {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 14, true);

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  Wire.read(); Wire.read();
  Wire.read(); Wire.read();
  Wire.read(); Wire.read();
  Wire.read(); Wire.read();

  AccX = ax / 16384.0;
  AccY = ay / 16384.0;
  AccZ = az / 16384.0;

  roll =
      atan2(-AccX, sqrt(AccY * AccY + AccZ * AccZ)) * 57.2958
      - rollOffset;

  pitch =
      atan2(AccY, AccZ) * 57.2958
      - pitchOffset;
}

// ================= MOTOR WRITE SMOOTH =================
void smoothMotor(float target1, float target2, float target3, float target4) {

  m1_f += (target1 - m1_f) * ramp * 0.01;
  m2_f += (target2 - m2_f) * ramp * 0.01;
  m3_f += (target3 - m3_f) * ramp * 0.01;
  m4_f += (target4 - m4_f) * ramp * 0.01;

  analogWrite(M1, constrain(m1_f, 0, 255));
  analogWrite(M2, constrain(m2_f, 0, 255));
  analogWrite(M3, constrain(m3_f, 0, 255));
  analogWrite(M4, constrain(m4_f, 0, 255));
}

// ================= MAP FLOAT =================
float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) /
         (in_max - in_min) + out_min;
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);

  stopMotors();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    while (true);
  }

  esp_now_register_recv_cb(onReceive);

  Wire.begin();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(1000);

  Serial.println("CALIB...");

  float sr = 0, sp = 0;

  for (int i = 0; i < 300; i++) {
    readMPU();
    sr += roll;
    sp += pitch;
    delay(5);
  }

  rollOffset = sr / 300.0;
  pitchOffset = sp / 300.0;

  Serial.println("READY");
  lastRX = millis();
}

// ================= LOOP =================
void loop() {

  // ================= FAILSAFE (FIXED) =================
  rxFail = (millis() - lastRX > 250);

  if (rxFail) {
    stopMotors();
    iRoll = 0;
    iPitch = 0;

    if (millis() % 500 < 20)
      Serial.println("NO SIGNAL");

    return;
  }

  readMPU();

  bool armed = data.ch[0] > 1550;

  float throttle = armed ?
    fmap(data.ch[0], 1550, 2000, 0, 255) : 0;

  float rollSet  = fmap(data.ch[3], 1000, 2000, -20, 20);
  float pitchSet = fmap(data.ch[2], 1000, 2000, -20, 20);
  float yawSet   = fmap(data.ch[1], 1000, 2000, -60, 60);

  if (abs(rollSet) < 1) rollSet = 0;
  if (abs(pitchSet) < 1) pitchSet = 0;
  if (abs(yawSet) < 2) yawSet = 0;

  float rollErr = rollSet - roll;
  float pitchErr = pitchSet - pitch;

  iRoll += rollErr;
  iPitch += pitchErr;

  iRoll = constrain(iRoll, -100, 100);
  iPitch = constrain(iPitch, -100, 100);

  float rollOut =
      kp * rollErr + kd * (rollErr - lastRollErr);

  float pitchOut =
      kp * pitchErr + kd * (pitchErr - lastPitchErr);

  lastRollErr = rollErr;
  lastPitchErr = pitchErr;

  float yawOut = yawSet * 0.2;

  float m1 = throttle + pitchOut + rollOut - yawOut;
  float m2 = throttle + pitchOut - rollOut + yawOut;
  float m3 = throttle - pitchOut - rollOut - yawOut;
  float m4 = throttle - pitchOut + rollOut + yawOut;

  if (!armed) {
    stopMotors();
     Serial.println("DISARMED");
    return;
  }

  smoothMotor(m1, m2, m3, m4);

  Serial.printf(
    "CH0:%d THR:%.0f R:%.2f P:%.2f M:%.0f %.0f %.0f %.0f\n",
    data.ch[0],
    throttle,
    roll,
    pitch,
    m1_f, m2_f, m3_f, m4_f
  );
}




// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Wire.h>

// // ================= RECEIVER DATA =================
// typedef struct {
//   uint16_t ch[4];
// } Data;

// volatile bool newPacket = false;

// Data data;
// Data lastData;

// // ================= SIGNAL =================
// unsigned long lastRX = 0;

// // ================= MOTOR PINS =================
// #define M1 3
// #define M2 6
// #define M3 4
// #define M4 5

// // ================= MPU9250 =================
// #define MPU_ADDR 0x68

// float AccX, AccY, AccZ;
// float roll, pitch;

// float rollOffset = 0;
// float pitchOffset = 0;

// // ================= PID =================
// float kp = 1.3;
// float ki = 0.0;
// float kd = 0.4;

// float iRoll = 0;
// float iPitch = 0;

// float lastRollErr = 0;
// float lastPitchErr = 0;

// // ================= FAILSAFE =================
// void setFailsafe() {

//   data.ch[0] = 1500; // throttle idle
//   data.ch[1] = 1500;
//   data.ch[2] = 1500;
//   data.ch[3] = 1500;
// }

// // ================= MOTOR =================
// void motorWrite(int pin, int value) {

//   value = constrain(value, 0, 255);

//   analogWrite(pin, value);
// }

// void stopMotors() {

//   motorWrite(M1, 0);
//   motorWrite(M2, 0);
//   motorWrite(M3, 0);
//   motorWrite(M4, 0);
// }

// // ================= FLOAT MAP =================
// float fmap(float x,
//            float in_min,
//            float in_max,
//            float out_min,
//            float out_max) {

//   return (x - in_min) *
//          (out_max - out_min) /
//          (in_max - in_min) +
//          out_min;
// }

// // ================= ESP NOW =================
// void onReceive(const uint8_t *mac,
//                const uint8_t *incoming,
//                int len) {

//   if(len != sizeof(Data)) return;

//   Data temp;

//   memcpy(&temp, incoming, sizeof(temp));

//   // ================= VALIDATE =================
//   bool invalid =
//       temp.ch[0] < 900  || temp.ch[0] > 2100 ||
//       temp.ch[1] < 900  || temp.ch[1] > 2100 ||
//       temp.ch[2] < 900  || temp.ch[2] > 2100 ||
//       temp.ch[3] < 900  || temp.ch[3] > 2100;

//   if(invalid) {

//     Serial.println("INVALID DATA");

//     return;
//   }

//   // ================= FILTER JUMP =================
//   if(abs((int)temp.ch[0] - (int)lastData.ch[0]) > 400) return;
//   if(abs((int)temp.ch[1] - (int)lastData.ch[1]) > 400) return;
//   if(abs((int)temp.ch[2] - (int)lastData.ch[2]) > 400) return;
//   if(abs((int)temp.ch[3] - (int)lastData.ch[3]) > 400) return;

//   memcpy(&data, &temp, sizeof(data));
//   memcpy(&lastData, &temp, sizeof(lastData));

//   lastRX = millis();

//   newPacket = true;
// }

// // ================= MPU9250 =================
// void readMPU() {

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x3B);
//   Wire.endTransmission(false);

//   Wire.requestFrom((uint8_t)MPU_ADDR,
//                    (uint8_t)14,
//                    (bool)true);

//   if(Wire.available() < 14) return;

//   int16_t ax = Wire.read() << 8 | Wire.read();
//   int16_t ay = Wire.read() << 8 | Wire.read();
//   int16_t az = Wire.read() << 8 | Wire.read();

//   Wire.read();
//   Wire.read();

//   Wire.read();
//   Wire.read();
//   Wire.read();
//   Wire.read();
//   Wire.read();
//   Wire.read();

//   AccX = ax / 16384.0;
//   AccY = ay / 16384.0;
//   AccZ = az / 16384.0;

//   roll =
//       atan2(
//         -AccX,
//         sqrt(AccY * AccY + AccZ * AccZ)
//       ) * 57.2958;

//   pitch =
//       atan2(AccY, AccZ) * 57.2958;

//   roll -= rollOffset;
//   pitch -= pitchOffset;
// }

// // ================= SETUP =================
// void setup() {

//   Serial.begin(115200);

//   // ================= MOTOR =================
//   pinMode(M1, OUTPUT);
//   pinMode(M2, OUTPUT);
//   pinMode(M3, OUTPUT);
//   pinMode(M4, OUTPUT);

//   stopMotors();

//   // ================= FAILSAFE =================
//   setFailsafe();

//   memcpy(&lastData, &data, sizeof(data));

//   // ================= WIFI =================
//   WiFi.mode(WIFI_STA);

//   // QUAN TRỌNG
//   WiFi.setSleep(false);

//   delay(200);

//   // ================= ESP NOW =================
//   if(esp_now_init() != ESP_OK) {

//     Serial.println("ESP NOW ERROR");

//     while(true);
//   }

//   esp_now_register_recv_cb(onReceive);

//   // ================= MPU =================
//   Wire.begin();

//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0);
//   Wire.endTransmission(true);

//   delay(1000);

//   // ================= CALIB =================
//   Serial.println("CALIB MPU");

//   float sr = 0;
//   float sp = 0;

//   for(int i=0;i<500;i++) {

//     readMPU();

//     sr += roll;
//     sp += pitch;

//     delay(5);
//   }

//   rollOffset = sr / 500.0;
//   pitchOffset = sp / 500.0;

//   Serial.println("READY");

//   lastRX = millis();
// }

// // ================= LOOP =================
// void loop() {

//   // ================= FAILSAFE =================
//   if(millis() - lastRX > 500) {

//     setFailsafe();

//     stopMotors();

//     iRoll = 0;
//     iPitch = 0;

//     lastRollErr = 0;
//     lastPitchErr = 0;

//     Serial.println("NO SIGNAL");

//     delay(20);

//     return;
//   }

//   // ================= REQUIRE PACKET =================
//   if(!newPacket) {

//     delay(2);

//     return;
//   }

//   newPacket = false;

//   // ================= MPU =================
//   readMPU();

//   // ==================================================
//   // THROTTLE FIX
//   // TX:
//   // 1500 = idle
//   // 2000 = full
//   // ==================================================

//   float throttle = 0;

//   if(data.ch[0] < 1550){

//       throttle = 0;
//   }
//   else{

//       throttle = fmap(
//           data.ch[0],
//           1550,
//           2000,
//           0,
//           255
//       );
//   }

//   // ================= OTHER CHANNEL =================
//   float yawSet =
//       fmap(data.ch[1], 1000, 2000, -60, 60);

//   float rollSet =
//       fmap(data.ch[3], 1000, 2000, -20, 20);

//   float pitchSet =
//       fmap(data.ch[2], 1000, 2000, -20, 20);

//   // ================= DEADZONE =================
//   if(abs(yawSet) < 2) yawSet = 0;
//   if(abs(rollSet) < 1) rollSet = 0;
//   if(abs(pitchSet) < 1) pitchSet = 0;

//   // ================= DEBUG =================
//   Serial.printf(
//       "CH:%d %d %d %d | THR:%.0f YAW:%.2f ROLL:%.2f PITCH:%.2f\n",
//       data.ch[0],
//       data.ch[1],
//       data.ch[2],
//       data.ch[3],
//       throttle,
//       yawSet,
//       rollSet,
//       pitchSet
//   );

//   // ================= PID =================
//   float rollErr = rollSet - roll;
//   float pitchErr = pitchSet - pitch;

//   iRoll += rollErr;
//   iPitch += pitchErr;

//   iRoll = constrain(iRoll, -100, 100);
//   iPitch = constrain(iPitch, -100, 100);

//   float dRoll = rollErr - lastRollErr;
//   float dPitch = pitchErr - lastPitchErr;

//   lastRollErr = rollErr;
//   lastPitchErr = pitchErr;

//   float rollOut =
//       kp * rollErr +
//       ki * iRoll +
//       kd * dRoll;

//   float pitchOut =
//       kp * pitchErr +
//       ki * iPitch +
//       kd * dPitch;

//   float yawOut = yawSet * 0.2;

//   // ================= MIXER =================
//   int m1 =
//       throttle +
//       pitchOut +
//       rollOut -
//       yawOut;

//   int m2 =
//       throttle +
//       pitchOut -
//       rollOut +
//       yawOut;

//   int m3 =
//       throttle -
//       pitchOut -
//       rollOut -
//       yawOut;

//   int m4 =
//       throttle -
//       pitchOut +
//       rollOut +
//       yawOut;

//   // ================= LIMIT =================
//   m1 = constrain(m1, 0, 255);
//   m2 = constrain(m2, 0, 255);
//   m3 = constrain(m3, 0, 255);
//   m4 = constrain(m4, 0, 255);

//   // ================= DISARM =================
//   if(throttle <= 0) {

//     stopMotors();

//     Serial.println("DISARMED");

//     delay(10);

//     return;
//   }

//   // ================= OUTPUT =================
//   motorWrite(M1, m1);
//   motorWrite(M2, m2);
//   motorWrite(M3, m3);
//   motorWrite(M4, m4);

//   // ================= MOTOR DEBUG =================
//   Serial.printf(
//       "R:%.2f P:%.2f | M:%d %d %d %d\n",
//       roll,
//       pitch,
//       m1,
//       m2,
//       m3,
//       m4
//   );

//   delay(10);
// }