#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>

// ================= RECEIVER DATA =================
typedef struct {
  uint16_t ch[4];
} Data;

Data data;

// ================= SIGNAL =================
unsigned long lastRX = 0;

// ================= MOTOR PINS =================
#define M1 3
#define M2 6
#define M3 4
#define M4 5

// ================= MPU9250 =================
#define MPU_ADDR 0x68

float AccX, AccY, AccZ;
float roll, pitch;

float rollOffset = 0;
float pitchOffset = 0;

// ================= PID =================
float kp = 1.3;
float ki = 0.0;
float kd = 0.4;

float iRoll = 0;
float iPitch = 0;

float lastRollErr = 0;
float lastPitchErr = 0;

// ================= ESP NOW =================
void onReceive(const uint8_t *mac, const uint8_t *incoming, int len) {

  if(len != sizeof(Data)) return;

  memcpy(&data, incoming, sizeof(data));

  lastRX = millis();
}

// ================= MOTOR =================
void motorWrite(int pin, int value) {

  value = constrain(value, 0, 255);

  analogWrite(pin, value);
}

// ================= STOP ALL =================
void stopMotors() {

  motorWrite(M1, 0);
  motorWrite(M2, 0);
  motorWrite(M3, 0);
  motorWrite(M4, 0);
}

// ================= MPU9250 RAW =================
void readMPU() {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14, (bool)true);

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  Wire.read();
  Wire.read();

  Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();

  AccX = ax / 16384.0;
  AccY = ay / 16384.0;
  AccZ = az / 16384.0;

  roll =
      atan2(-AccX, sqrt(AccY * AccY + AccZ * AccZ)) * 57.2958;

  pitch =
      atan2(AccY, AccZ) * 57.2958;

  roll -= rollOffset;
  pitch -= pitchOffset;
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  // ================= MOTOR =================
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);

  stopMotors();

  // ================= WIFI =================
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {

    Serial.println("ESP NOW ERROR");

    while (true);
  }

  esp_now_register_recv_cb(onReceive);

  // ================= MPU =================
  Wire.begin();

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(1000);

  // ================= CALIB =================
  Serial.println("CALIB MPU");

  float sr = 0;
  float sp = 0;

  for (int i = 0; i < 500; i++) {

    readMPU();

    sr += roll;
    sp += pitch;

    delay(5);
  }

  rollOffset = sr / 500.0;
  pitchOffset = sp / 500.0;

  Serial.println("READY");

  lastRX = millis();
}

// ================= LOOP =================
void loop() {

  // ================= FAILSAFE =================
  if (millis() - lastRX > 300) {

    stopMotors();

    iRoll = 0;
    iPitch = 0;

    lastRollErr = 0;
    lastPitchErr = 0;

    Serial.println("NO SIGNAL");

    delay(20);

    return;
  }

  // ================= MPU =================
  readMPU();

  // ================= CHANNEL =================
  float throttle =
      map(data.ch[2], 1000, 2000, 0, 255);

  float rollSet =
      map(data.ch[0], 1000, 2000, -20, 20);

  float pitchSet =
      map(data.ch[1], 1000, 2000, -20, 20);

  float yawSet =
      map(data.ch[3], 1000, 2000, -60, 60);

  // ================= PID =================
  float rollErr = rollSet - roll;
  float pitchErr = pitchSet - pitch;

  iRoll += rollErr;
  iPitch += pitchErr;

  iRoll = constrain(iRoll, -100, 100);
  iPitch = constrain(iPitch, -100, 100);

  float dRoll = rollErr - lastRollErr;
  float dPitch = pitchErr - lastPitchErr;

  lastRollErr = rollErr;
  lastPitchErr = pitchErr;

  float rollOut =
      kp * rollErr +
      ki * iRoll +
      kd * dRoll;

  float pitchOut =
      kp * pitchErr +
      ki * iPitch +
      kd * dPitch;

  float yawOut = yawSet * 0.2;

  // ================= X MIXER =================
  int m1 =
      throttle +
      pitchOut +
      rollOut -
      yawOut;

  int m2 =
      throttle +
      pitchOut -
      rollOut +
      yawOut;

  int m3 =
      throttle -
      pitchOut -
      rollOut -
      yawOut;

  int m4 =
      throttle -
      pitchOut +
      rollOut +
      yawOut;

  // ================= LIMIT =================
  m1 = constrain(m1, 0, 255);
  m2 = constrain(m2, 0, 255);
  m3 = constrain(m3, 0, 255);
  m4 = constrain(m4, 0, 255);

  // ================= ARM CHECK =================
  if (throttle < 10) {

    stopMotors();

    Serial.println("DISARMED");

    delay(10);

    return;
  }

  // ================= OUTPUT =================
  motorWrite(M1, m1);
  motorWrite(M2, m2);
  motorWrite(M3, m3);
  motorWrite(M4, m4);

  // ================= DEBUG =================
  Serial.printf(
      "R:%.2f P:%.2f THR:%.0f M:%d %d %d %d\n",
      roll,
      pitch,
      throttle,
      m1,
      m2,
      m3,
      m4);

  delay(10);
}






// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>

// uint8_t rxMAC[] = {0x14,0x63,0x93,0x6E,0x77,0xB4};

// typedef struct {
//   uint16_t ch[4];
// } Data;

// Data data;

// #define CH1 0
// #define CH2 1
// #define CH3 3
// #define CH4 4

// int center[4];

// uint16_t mapStick(int v, int c){
//   if(abs(v-c) < 80) return 1500;
//   if(v < c) return map(v, 0, c, 1000, 1500);
//   return map(v, c, 4095, 1500, 2000);
// }

// void onSend(const uint8_t*, esp_now_send_status_t status){
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "TX OK" : "TX FAIL");
// }

// void setup(){
//   Serial.begin(115200);
//   WiFi.mode(WIFI_STA);

//   esp_now_init();
//   esp_now_register_send_cb(onSend);

//   esp_now_peer_info_t peer{};
//   memcpy(peer.peer_addr, rxMAC, 6);
//   peer.channel = 0;
//   peer.encrypt = false;
//   esp_now_add_peer(&peer);

//   Serial.println("CALIB...");

//   long s[4]={0};
//   for(int i=0;i<200;i++){
//     s[0]+=analogRead(CH1);
//     s[1]+=analogRead(CH2);
//     s[2]+=analogRead(CH3);
//     s[3]+=analogRead(CH4);
//     delay(5);
//   }

//   for(int i=0;i<4;i++) center[i]=s[i]/200;

//   Serial.println("READY");
// }

// void loop(){
//   int r[4];
//   r[0]=analogRead(CH1);
//   r[1]=analogRead(CH2);
//   r[2]=analogRead(CH3);
//   r[3]=analogRead(CH4);

//   for(int i=0;i<4;i++){
//     data.ch[i]=mapStick(r[i], center[i]);
//   }

//   esp_now_send(rxMAC,(uint8_t*)&data,sizeof(data));

//   Serial.printf("TX %d %d %d %d\n",
//     data.ch[0],data.ch[1],data.ch[2],data.ch[3]);

//   delay(20);
// };