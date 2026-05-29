#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>


#define CHANNELS 8

volatile uint32_t lastPacketTime = 0;

typedef struct {
  uint16_t ch[CHANNELS];
} PPMData;

PPMData receivedData;

// ===== callback nhận =====
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));

  for (int i = 0; i < CHANNELS; i++) {

    receivedData.ch[i];
  Serial.print("CH");
  Serial.print(i + 1);
  Serial.print(":");
  Serial.print(receivedData.ch[i]);
  Serial.print(" ");
}
Serial.println();

 lastPacketTime = millis();
}

// ======================================================
// ===================== Calibration =============================
// ======================================================


float GyroOffsetRoll = 0;
float GyroOffsetPitch = 0;
float GyroOffsetYaw = 0;

float AccAngleRoll = 0;
float AccAnglePitch = 0;


// ======================================================
// ===================== LOOP TIMER ======================
// ======================================================

uint32_t loopTimer = 0;
float dt = 0.004;

// ======================================================
// ===================== ARM =============================
// ======================================================

bool armed = false;

// ======================================================
// ===================== PWM =============================
// ======================================================


float RateRoll, RatePitch, RateYaw;
float AccX, AccY, AccZ;
float AngleRoll, AnglePitch;



// ======================================================
// ================= GYRO CALIBRATION ===================
// ======================================================

void calibrateGyro() {

  Serial.println("CALIBRATING GYRO...");

  for (int i = 0; i < 2000; i++) {

    Wire.beginTransmission(0x68);
    Wire.write(0x43);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)0x68,
                     (uint8_t)6);

    int16_t gx =
      Wire.read() << 8 | Wire.read();

    int16_t gy =
      Wire.read() << 8 | Wire.read();

    int16_t gz =
      Wire.read() << 8 | Wire.read();

    GyroOffsetRoll += gx / 65.5;
    GyroOffsetPitch += gy / 65.5;
    GyroOffsetYaw += gz / 65.5;

    delay(2);
  }

  GyroOffsetRoll /= 2000;
  GyroOffsetPitch /= 2000;
  GyroOffsetYaw /= 2000;

  Serial.println("GYRO DONE");
}



// float LoopTimer;
void gyro_signals() {

  // đọc accel
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  Wire.requestFrom(0x68, 6);

  int16_t AccXLSB =
    Wire.read() << 8 | Wire.read();

  int16_t AccYLSB =
    Wire.read() << 8 | Wire.read();

  int16_t AccZLSB =
    Wire.read() << 8 | Wire.read();

  // đọc gyro
  Wire.beginTransmission(0x68);
  Wire.write(0x43);
  Wire.endTransmission(false);

  Wire.requestFrom(0x68, 6);

  int16_t GyroX =
    Wire.read() << 8 | Wire.read();

  int16_t GyroY =
    Wire.read() << 8 | Wire.read();

  int16_t GyroZ =
    Wire.read() << 8 | Wire.read();

  RateRoll =
    ((float)GyroX / 65.5)
    - GyroOffsetRoll;

  RatePitch =
    ((float)GyroY / 65.5)
    - GyroOffsetPitch;

  RateYaw =
    ((float)GyroZ / 65.5)
    - GyroOffsetYaw;

  AccX = (float)AccXLSB / 4096.0;
  AccY = (float)AccYLSB / 4096.0;
  AccZ = (float)AccZLSB / 4096.0;

  float rollDenom =
    sqrt(AccX * AccX + AccZ * AccZ);

  float pitchDenom =
    sqrt(AccY * AccY + AccZ * AccZ);

  if (rollDenom < 0.0001)
    rollDenom = 0.0001;

  if (pitchDenom < 0.0001)
    pitchDenom = 0.0001;

  AccAngleRoll =
    atan2(AccY, rollDenom)
    * 57.2958;

  AccAnglePitch =
    -atan2(AccX, pitchDenom)
    * 57.2958;

  AngleRoll =
    0.98 *
    (AngleRoll + RateRoll * dt)
    +
    0.02 * AccAngleRoll;

  AnglePitch =
    0.98 *
    (AnglePitch + RatePitch * dt)
    +
    0.02 * AccAnglePitch;
}



// void gyro_signals(void) {
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1A);
//   Wire.write(0x05);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1C);
//   Wire.write(0x10);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x3B);
//   Wire.endTransmission(); 
//   Wire.requestFrom(0x68,6);
//   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1B); 
//   Wire.write(0x8);
//   Wire.endTransmission();                                                   
//   Wire.beginTransmission(0x68);
//   Wire.write(0x43);
//   Wire.endTransmission();
//   Wire.requestFrom(0x68,6);
//   int16_t GyroX=Wire.read()<<8 | Wire.read();
//   int16_t GyroY=Wire.read()<<8 | Wire.read();
//   int16_t GyroZ=Wire.read()<<8 | Wire.read();
//   // RateRoll=(float)GyroX/65.5;
//   // RatePitch=(float)GyroY/65.5;
//   // RateYaw=(float)GyroZ/65.5;

//   RateRoll=((float)GyroX/65.5) - GyroOffsetRoll;
// RatePitch=((float)GyroY/65.5) - GyroOffsetPitch;
// RateYaw=((float)GyroZ/65.5) - GyroOffsetYaw;
//   AccX=(float)AccXLSB/4096;
//   AccY=(float)AccYLSB/4096;
//   AccZ=(float)AccZLSB/4096;


// //  
//   // AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
//   // AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);

// float rollDenom =
//   sqrt(AccX * AccX + AccZ * AccZ);

// float pitchDenom =
//   sqrt(AccY * AccY + AccZ * AccZ);

// // chống chia 0
// if (rollDenom < 0.0001)
//   rollDenom = 0.0001;

// if (pitchDenom < 0.0001)
//   pitchDenom = 0.0001;

// AccAngleRoll =
//   atan2(
//     AccY,
//     rollDenom
//   ) * 57.2958;

// AccAnglePitch =
//   -atan2(
//     AccX,
//     pitchDenom
//   ) * 57.2958;

// AngleRoll =
//   0.98 *
//   (AngleRoll + RateRoll * dt)
//   +
//   0.02 * AccAngleRoll;

// AnglePitch =
//   0.98 *
//   (AnglePitch + RatePitch * dt)
//   +
//   0.02 * AccAnglePitch;
   

// }


// ======================================================
// ===================== FAILSAFE ========================
// ======================================================

void failsafe() {

  if (millis() - lastPacketTime > 500) {

    receivedData.ch[0] = 1000;
    receivedData.ch[1] = 1500;
    receivedData.ch[2] = 1500;
    receivedData.ch[3] = 1500;

    armed = false;
  }
}

// ======================================================
// ===================== ARMING ==========================
// ======================================================

void handleArm() {

  // ARM
  if (receivedData.ch[0] > 1050
    //  &&
    //   receivedData.ch[1] > 1900
    ) {

    armed = true;
  }

  // DISARM
  if (receivedData.ch[0] < 1050 
    // &&
    //   receivedData.ch[1] < 1100
    
    ) {

    armed = false;
  }
}

// =====================================================
// ================= PID ================================
// =====================================================

float ErrorRoll, ErrorPitch, ErrorYaw;

float InputRoll, InputPitch, InputYaw;
float Throttle;

float PR = 0.0;
float IR = 0.0;
float DR = 0.0;

float PP = 0.0;
float IP = 0.0;
float DP = 0.0;

float PY = 0.0;
float IY = 0.0;
float DY = 0.0;

float PrevErrR = 0;
float PrevErrP = 0;
float PrevErrY = 0;

float PrevIntR = 0;
float PrevIntP = 0;
float PrevIntY = 0;

float PIDOut[3];

void pidEquation(
  float error,
  float P,
  float I,
  float D,
  float &prevErr,
  float &prevInt
) {

  float Pterm = P * error;

  float Iterm =
    // prevInt +
    // I * (error + prevErr) * 0.004 * 0.5;

     prevInt +
    I * (error + prevErr) * dt * 0.5;

  Iterm = constrain(Iterm, -400, 400);

  float Dterm =
    // D * (error - prevErr) / 0.004;
     D * (error - prevErr) / dt;


  float output = Pterm + Iterm + Dterm;

  output = constrain(output, -400, 400);

  prevErr = error;
  prevInt = Iterm;

  PIDOut[0] = output;
  PIDOut[1] = error;
  PIDOut[2] = Iterm;
}


void motorMix() {

  float DesiredRoll =
    0.10 * (receivedData.ch[3] - 1500);

  float DesiredPitch =
    0.10 * (receivedData.ch[2] - 1500);

  float DesiredYaw =
    0.15 * (receivedData.ch[1] - 1500);

  // ErrorRoll =
  //   DesiredRoll - RateRoll;

  // ErrorPitch =
  //   DesiredPitch - RatePitch;

  ErrorRoll = DesiredRoll - AngleRoll;
ErrorPitch = DesiredPitch - AnglePitch;

  ErrorYaw =
    DesiredYaw - RateYaw;

  // ROLL
  pidEquation(
    ErrorRoll,
    PR,
    IR,
    DR,
    PrevErrR,
    PrevIntR
  );

  InputRoll = PIDOut[0];

  // PITCH
  pidEquation(
    ErrorPitch,
    PP,
    IP,
    DP,
    PrevErrP,
    PrevIntP
  );

  InputPitch = PIDOut[0];

  // YAW
  pidEquation(
    ErrorYaw,
    PY,
    IY,
    DY,
    PrevErrY,
    PrevIntY
  );

  InputYaw = PIDOut[0];

  Throttle = receivedData.ch[0];

  // X QUADCOPTER MIX
  int m1 =
    Throttle
    - InputRoll
    - InputPitch
    - InputYaw;

  int m2 =
    Throttle
    + InputRoll
    - InputPitch
    + InputYaw;

  int m3 =
    Throttle
    + InputRoll
    + InputPitch
    - InputYaw;

  int m4 =
    Throttle
    - InputRoll
    + InputPitch
    + InputYaw;

  m1 = constrain(m1, 1000, 2000);
  m2 = constrain(m2, 1000, 2000);
  m3 = constrain(m3, 1000, 2000);
  m4 = constrain(m4, 1000, 2000);

  if (!armed) {
    m1 = 1000;
    m2 = 1000;
    m3 = 1000;
    m4 = 1000;

    PrevIntR = 0;
    PrevIntP = 0;
    PrevIntY = 0;

    PrevErrR = 0;
    PrevErrP = 0;
    PrevErrY = 0;
  }


  // writeMotor(M1_CH, m1);
  // writeMotor(M2_CH, m2);
  // writeMotor(M3_CH, m3);
  // writeMotor(M4_CH, m4);

  // DEBUG
  static uint32_t lastPrint = 0;

  if (millis() - lastPrint > 100) {

    lastPrint = millis();

    Serial.println("---------------");

    //   Serial.print(" THR: ");
    // Serial.print(receivedData.ch[0]);

    // Serial.print(" YAW: ");
    // Serial.print(receivedData.ch[1]);

    // Serial.print(" ROLL: ");
    // Serial.print(receivedData.ch[3]);

    // Serial.print(" PITCH: ");
    // Serial.println(receivedData.ch[2]);

  

    // Serial.print("GYRO ROLL: ");
    // Serial.print(RateRoll);

    // Serial.print(" PITCH: ");
    // Serial.print(RatePitch);

    // Serial.print(" YAW: ");
    // Serial.println(RateYaw);

    Serial.print("M1: ");
    Serial.print(m1);

    Serial.print(" M2: ");
    Serial.print(m2);

    Serial.print(" M3: ");
    Serial.print(m3);

    Serial.print(" M4: ");
    Serial.println(m4);
  }
}

void setupMPU6050() {

  // wake up MPU6050
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  // DLPF
  Wire.beginTransmission(0x68);
  Wire.write(0x1A);
  Wire.write(0x05);
  Wire.endTransmission();

  // Gyro ±500 deg/s
  Wire.beginTransmission(0x68);
  Wire.write(0x1B);
  Wire.write(0x08);
  Wire.endTransmission();

  // Accel ±8g
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);

  delay(1000);

  // I2C
  Wire.begin();
  Wire.setClock(400000);



  // // WAKE MPU6050
  // Wire.beginTransmission(0x68);
  // Wire.write(0x6B);
  // Wire.write(0x00);
  // Wire.endTransmission();

  setupMPU6050();

delay(100);

    calibrateGyro();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

   loopTimer = micros();

}

void loop() {

  while (micros() - loopTimer < 4000);

  dt =
    (micros() - loopTimer)
    / 1000000.0;

  // chống dt = 0
  if (dt <= 0.000001)
    dt = 0.000001;

  loopTimer = micros();

    gyro_signals();
    failsafe();
    handleArm();
    motorMix();
  // Serial.print("Acceleration X [g]= ");
  // Serial.print(AccX);
  // Serial.print(" Acceleration Y [g]= ");
  // Serial.print(AccY);
  // Serial.print(" Acceleration Z [g]= ");
  // Serial.println(AccZ);
  // delay(50);
}




// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Wire.h>


// #define CHANNELS 8

// volatile uint32_t lastPacketTime = 0;

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData receivedData;

// // ===== callback nhận =====
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));

//   for (int i = 0; i < CHANNELS; i++) {

//     receivedData.ch[i];
//   // Serial.print("CH");
//   // Serial.print(i + 1);
//   // Serial.print(":");
//   // Serial.print(receivedData.ch[i]);
//   // Serial.print(" ");
// }
// Serial.println();

//  lastPacketTime = millis();
// }


// float RateRoll, RatePitch, RateYaw;
// float AccX, AccY, AccZ;
// float AngleRoll, AnglePitch;
// // float LoopTimer;
// void gyro_signals(void) {
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1A);
//   Wire.write(0x05);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1C);
//   Wire.write(0x10);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x3B);
//   Wire.endTransmission(); 
//   Wire.requestFrom(0x68,6);
//   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1B); 
//   Wire.write(0x8);
//   Wire.endTransmission();                                                   
//   Wire.beginTransmission(0x68);
//   Wire.write(0x43);
//   Wire.endTransmission();
//   Wire.requestFrom(0x68,6);
//   int16_t GyroX=Wire.read()<<8 | Wire.read();
//   int16_t GyroY=Wire.read()<<8 | Wire.read();
//   int16_t GyroZ=Wire.read()<<8 | Wire.read();
//   RateRoll=(float)GyroX/65.5;
//   RatePitch=(float)GyroY/65.5;
//   RateYaw=(float)GyroZ/65.5;
//   AccX=(float)AccXLSB/4096;
//   AccY=(float)AccYLSB/4096;
//   AccZ=(float)AccZLSB/4096;
//   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
//   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
// }


// void failsafe() {

//   if (millis() - lastPacketTime > 500) {

//     receivedData.ch[0] = 1500;
//     receivedData.ch[1] = 1500;
//     receivedData.ch[2] = 1500;
//     receivedData.ch[3] = 1500;
//   }
// }

// // =====================================================
// // ================= PID ================================
// // =====================================================

// float ErrorRoll, ErrorPitch, ErrorYaw;

// float InputRoll, InputPitch, InputYaw;
// float Throttle;

// float PR = 0.6;
// float IR = 3.5;
// float DR = 0.03;

// float PP = 0.6;
// float IP = 3.5;
// float DP = 0.03;

// float PY = 2.0;
// float IY = 12.0;
// float DY = 0.0;

// float PrevErrR = 0;
// float PrevErrP = 0;
// float PrevErrY = 0;

// float PrevIntR = 0;
// float PrevIntP = 0;
// float PrevIntY = 0;

// float PIDOut[3];

// void pidEquation(
//   float error,
//   float P,
//   float I,
//   float D,
//   float &prevErr,
//   float &prevInt
// ) {

//   float Pterm = P * error;

//   float Iterm =
//     prevInt +
//     I * (error + prevErr) * 0.004 * 0.5;

//   Iterm = constrain(Iterm, -400, 400);

//   float Dterm =
//     D * (error - prevErr) / 0.004;

//   float output = Pterm + Iterm + Dterm;

//   output = constrain(output, -400, 400);

//   prevErr = error;
//   prevInt = Iterm;

//   PIDOut[0] = output;
//   PIDOut[1] = error;
//   PIDOut[2] = Iterm;
// }


// void motorMix() {

//   float DesiredRoll =
//     0.10 * (receivedData.ch[3] - 1500);

//   float DesiredPitch =
//     0.10 * (receivedData.ch[2] - 1500);

//   float DesiredYaw =
//     0.15 * (receivedData.ch[1] - 1500);

//   ErrorRoll =
//     DesiredRoll - RateRoll;

//   ErrorPitch =
//     DesiredPitch - RatePitch;

//   ErrorYaw =
//     DesiredYaw - RateYaw;

//   // ROLL
//   pidEquation(
//     ErrorRoll,
//     PR,
//     IR,
//     DR,
//     PrevErrR,
//     PrevIntR
//   );

//   InputRoll = PIDOut[0];

//   // PITCH
//   pidEquation(
//     ErrorPitch,
//     PP,
//     IP,
//     DP,
//     PrevErrP,
//     PrevIntP
//   );

//   InputPitch = PIDOut[0];

//   // YAW
//   pidEquation(
//     ErrorYaw,
//     PY,
//     IY,
//     DY,
//     PrevErrY,
//     PrevIntY
//   );

//   InputYaw = PIDOut[0];

//   Throttle = receivedData.ch[0];

//   // X QUADCOPTER MIX
//   int m1 =
//     Throttle
//     - InputRoll
//     - InputPitch
//     - InputYaw;

//   int m2 =
//     Throttle
//     + InputRoll
//     - InputPitch
//     + InputYaw;

//   int m3 =
//     Throttle
//     + InputRoll
//     + InputPitch
//     - InputYaw;

//   int m4 =
//     Throttle
//     - InputRoll
//     + InputPitch
//     + InputYaw;

//   m1 = constrain(m1, 1000, 2000);
//   m2 = constrain(m2, 1000, 2000);
//   m3 = constrain(m3, 1000, 2000);
//   m4 = constrain(m4, 1000, 2000);

//   // writeMotor(M1_CH, m1);
//   // writeMotor(M2_CH, m2);
//   // writeMotor(M3_CH, m3);
//   // writeMotor(M4_CH, m4);

//   // DEBUG
//   static uint32_t lastPrint = 0;

//   if (millis() - lastPrint > 100) {

//     lastPrint = millis();

//     Serial.println("---------------");

//     //   Serial.print(" THR: ");
//     // Serial.print(receivedData.ch[0]);

//     // Serial.print(" YAW: ");
//     // Serial.print(receivedData.ch[1]);

//     // Serial.print(" ROLL: ");
//     // Serial.print(receivedData.ch[3]);

//     // Serial.print(" PITCH: ");
//     // Serial.println(receivedData.ch[2]);

  

//     // Serial.print("GYRO ROLL: ");
//     // Serial.print(RateRoll);

//     // Serial.print(" PITCH: ");
//     // Serial.print(RatePitch);

//     // Serial.print(" YAW: ");
//     // Serial.println(RateYaw);

//     Serial.print("M1: ");
//     Serial.print(m1);

//     Serial.print(" M2: ");
//     Serial.print(m2);

//     Serial.print(" M3: ");
//     Serial.print(m3);

//     Serial.print(" M4: ");
//     Serial.println(m4);
//   }
// }


// void setup() {
//   Serial.begin(115200);

//   delay(1000);

//   // I2C
//   Wire.begin();
//   Wire.setClock(400000);

//   // WAKE MPU6050
//   Wire.beginTransmission(0x68);
//   Wire.write(0x6B);
//   Wire.write(0x00);
//   Wire.endTransmission();

//   delay(100);


//   WiFi.mode(WIFI_STA);

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP-NOW init failed");
//     return;
//   }

//   esp_now_register_recv_cb(OnDataRecv);

// }

// void loop() {

//   // Serial.print(receivedData.ch[0]);

//     gyro_signals();
//     motorMix();
//   // Serial.print("Acceleration X [g]= ");
//   // Serial.print(AccX);
//   // Serial.print(" Acceleration Y [g]= ");
//   // Serial.print(AccY);
//   // Serial.print(" Acceleration Z [g]= ");
//   // Serial.println(AccZ);
//   // delay(50);
// }































// // include <Arduino.h>
// // #include <WiFi.h>
// // #include <esp_now.h>
// // #include <Wire.h>

// // // =====================================================
// // // ================= ESP NOW RX =========================
// // // =====================================================

// // #define CHANNELS 8

// // typedef struct {
// //   uint16_t ch[CHANNELS];
// // } PPMData;

// // PPMData receivedData;

// // // =====================================================
// // // ================= MPU6050 ============================
// // // =====================================================

// // float RateRoll, RatePitch, RateYaw;
// // float AccX, AccY, AccZ;
// // float AngleRoll, AnglePitch;

// // // =====================================================
// // // ================= MOTOR PWM ==========================
// // // =====================================================

// // // ESC pins
// // #define M1_PIN 25
// // #define M2_PIN 26
// // #define M3_PIN 27
// // #define M4_PIN 14

// // // LEDC channels
// // #define M1_CH 0
// // #define M2_CH 1
// // #define M3_CH 2
// // #define M4_CH 3

// // // PWM config
// // #define PWM_FREQ 50
// // #define PWM_RESOLUTION 16

// // // 50Hz = 20ms
// // // 1000us -> ~3276
// // // 2000us -> ~6553

// // uint32_t usToDuty(uint16_t us) {
// //   return map(us, 1000, 2000, 3276, 6553);
// // }

// // void writeMotor(uint8_t channel, uint16_t us) {
// //   us = constrain(us, 1000, 2000);
// //   ledcWrite(channel, usToDuty(us));
// // }

// // // =====================================================
// // // ================= PID ================================
// // // =====================================================

// // float ErrorRoll, ErrorPitch, ErrorYaw;

// // float InputRoll, InputPitch, InputYaw;
// // float Throttle;

// // float PR = 0.6;
// // float IR = 3.5;
// // float DR = 0.03;

// // float PP = 0.6;
// // float IP = 3.5;
// // float DP = 0.03;

// // float PY = 2.0;
// // float IY = 12.0;
// // float DY = 0.0;

// // float PrevErrR = 0;
// // float PrevErrP = 0;
// // float PrevErrY = 0;

// // float PrevIntR = 0;
// // float PrevIntP = 0;
// // float PrevIntY = 0;

// // float PIDOut[3];

// // void pidEquation(
// //   float error,
// //   float P,
// //   float I,
// //   float D,
// //   float &prevErr,
// //   float &prevInt
// // ) {

// //   float Pterm = P * error;

// //   float Iterm =
// //     prevInt +
// //     I * (error + prevErr) * 0.004 * 0.5;

// //   Iterm = constrain(Iterm, -400, 400);

// //   float Dterm =
// //     D * (error - prevErr) / 0.004;

// //   float output = Pterm + Iterm + Dterm;

// //   output = constrain(output, -400, 400);

// //   prevErr = error;
// //   prevInt = Iterm;

// //   PIDOut[0] = output;
// //   PIDOut[1] = error;
// //   PIDOut[2] = Iterm;
// // }

// // // =====================================================
// // // ================= ESP NOW CALLBACK ===================
// // // =====================================================

// // volatile uint32_t lastPacketTime = 0;


// // // void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
// // //   memcpy(&receivedData, incomingData, sizeof(receivedData));

// // //   for (int i = 0; i < CHANNELS; i++) {

// // //     receivedData.ch[i];

// // //   }
// // // Serial.println();
// // // }


// // void OnDataRecv(
// //   const uint8_t *mac,
// //   const uint8_t *incomingData,
// //   int len
// // ) {

// //   if (len != sizeof(PPMData)) {
// //     Serial.println("INVALID PACKET");
// //     return;
// //   }

// //   memcpy(&receivedData, incomingData, sizeof(receivedData));

// //   lastPacketTime = millis();
// // }

// // // =====================================================
// // // ================= MPU6050 READ =======================
// // // =====================================================

// // void gyro_signals() {

// //   // ACCEL CONFIG
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x1C);
// //   Wire.write(0x10);
// //   Wire.endTransmission();

// //   // READ ACC
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x3B);
// //   Wire.endTransmission();

// //   Wire.requestFrom((uint8_t)0x68, (uint8_t)6, true);

// //   int16_t AccXLSB =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t AccYLSB =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t AccZLSB =
// //     Wire.read() << 8 | Wire.read();

// //   // GYRO CONFIG
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x1B);
// //   Wire.write(0x08);
// //   Wire.endTransmission();

// //   // READ GYRO
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x43);
// //   Wire.endTransmission();

// //   Wire.requestFrom((uint8_t)0x68, (uint8_t)6, true);

// //   int16_t GyroX =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t GyroY =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t GyroZ =
// //     Wire.read() << 8 | Wire.read();

// //   // SCALE
// //   RateRoll  = (float)GyroX / 65.5;
// //   RatePitch = (float)GyroY / 65.5;
// //   RateYaw   = (float)GyroZ / 65.5;

// //   AccX = (float)AccXLSB / 4096;
// //   AccY = (float)AccYLSB / 4096;
// //   AccZ = (float)AccZLSB / 4096;

// //   AngleRoll =
// //     atan(
// //       AccY /
// //       sqrt(AccX * AccX + AccZ * AccZ)
// //     ) * 57.2958;

// //   AnglePitch =
// //     -atan(
// //       AccX /
// //       sqrt(AccY * AccY + AccZ * AccZ)
// //     ) * 57.2958;
// // }

// // // =====================================================
// // // ================= FAILSAFE ===========================
// // // =====================================================

// // // void failsafe() {

// // //   if (millis() - lastPacketTime > 500) {

// // //     receivedData[0] = 1500;
// // //     receivedData[1] = 1500;
// // //     receivedData[2] = 1000;
// // //     receivedData[3] = 1500;
// // //   }
// // // }

// // // =====================================================
// // // ================= MOTOR MIX ==========================
// // // =====================================================

// // void motorMix() {

// //   float DesiredRoll =
// //     0.10 * (receivedData.ch[0] - 1500);

// //   float DesiredPitch =
// //     0.10 * (receivedData.ch[1] - 1500);

// //   float DesiredYaw =
// //     0.15 * (receivedData.ch[3] - 1500);

// //   ErrorRoll =
// //     DesiredRoll - RateRoll;

// //   ErrorPitch =
// //     DesiredPitch - RatePitch;

// //   ErrorYaw =
// //     DesiredYaw - RateYaw;

// //   // ROLL
// //   pidEquation(
// //     ErrorRoll,
// //     PR,
// //     IR,
// //     DR,
// //     PrevErrR,
// //     PrevIntR
// //   );

// //   InputRoll = PIDOut[0];

// //   // PITCH
// //   pidEquation(
// //     ErrorPitch,
// //     PP,
// //     IP,
// //     DP,
// //     PrevErrP,
// //     PrevIntP
// //   );

// //   InputPitch = PIDOut[0];

// //   // YAW
// //   pidEquation(
// //     ErrorYaw,
// //     PY,
// //     IY,
// //     DY,
// //     PrevErrY,
// //     PrevIntY
// //   );

// //   InputYaw = PIDOut[0];

// //   Throttle = receivedData.ch[2];

// //   // X QUADCOPTER MIX
// //   int m1 =
// //     Throttle
// //     - InputRoll
// //     - InputPitch
// //     - InputYaw;

// //   int m2 =
// //     Throttle
// //     + InputRoll
// //     - InputPitch
// //     + InputYaw;

// //   int m3 =
// //     Throttle
// //     + InputRoll
// //     + InputPitch
// //     - InputYaw;

// //   int m4 =
// //     Throttle
// //     - InputRoll
// //     + InputPitch
// //     + InputYaw;

// //   m1 = constrain(m1, 1000, 2000);
// //   m2 = constrain(m2, 1000, 2000);
// //   m3 = constrain(m3, 1000, 2000);
// //   m4 = constrain(m4, 1000, 2000);

// //   // writeMotor(M1_CH, m1);
// //   // writeMotor(M2_CH, m2);
// //   // writeMotor(M3_CH, m3);
// //   // writeMotor(M4_CH, m4);

// //   // DEBUG
// //   static uint32_t lastPrint = 0;

// //   if (millis() - lastPrint > 100) {

// //     lastPrint = millis();

// //     Serial.println("---------------");

// //     Serial.print("CH1: ");
// //     Serial.print(receivedData.ch[0]);

// //     Serial.print(" CH2: ");
// //     Serial.print(receivedData.ch[1]);

// //     Serial.print(" THR: ");
// //     Serial.print(receivedData.ch[2]);

// //     Serial.print(" YAW: ");
// //     Serial.println(receivedData.ch[3]);

// //     Serial.print("GYRO ROLL: ");
// //     Serial.print(RateRoll);

// //     Serial.print(" PITCH: ");
// //     Serial.print(RatePitch);

// //     Serial.print(" YAW: ");
// //     Serial.println(RateYaw);

// //     Serial.print("M1: ");
// //     Serial.print(m1);

// //     Serial.print(" M2: ");
// //     Serial.print(m2);

// //     Serial.print(" M3: ");
// //     Serial.print(m3);

// //     Serial.print(" M4: ");
// //     Serial.println(m4);
// //   }
// // }

// // // =====================================================
// // // ================= SETUP ==============================
// // // =====================================================

// // void setup() {

// //   Serial.begin(115200);

// //   delay(1000);

// //   // I2C
// //   Wire.begin();
// //   Wire.setClock(400000);

// //   // WAKE MPU6050
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x6B);
// //   Wire.write(0x00);
// //   Wire.endTransmission();

// //   delay(100);

// //   // WIFI
// //   WiFi.mode(WIFI_STA);

// //   Serial.print("RX MAC: ");
// //   Serial.println(WiFi.macAddress());

// //   // ESP NOW
// //   if (esp_now_init() != ESP_OK) {

// //     Serial.println("ESP NOW INIT FAILED");

// //     while (1);
// //   }

// //   esp_now_register_recv_cb(OnDataRecv);

// //   // PWM SETUP
// //   ledcSetup(M1_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M2_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M3_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M4_CH, PWM_FREQ, PWM_RESOLUTION);

// //   ledcAttachPin(M1_PIN, M1_CH);
// //   ledcAttachPin(M2_PIN, M2_CH);
// //   ledcAttachPin(M3_PIN, M3_CH);
// //   ledcAttachPin(M4_PIN, M4_CH);

// //   // ARM ESC
// //   writeMotor(M1_CH, 1000);
// //   writeMotor(M2_CH, 1000);
// //   writeMotor(M3_CH, 1000);
// //   writeMotor(M4_CH, 1000);

// //   Serial.println("DRONE RX READY");
// // }

// // // =====================================================
// // // ================= LOOP ===============================
// // // =====================================================

// // void loop() {

// //   gyro_signals();

// //   // failsafe();

// //   motorMix();

// //   delay(4);
// // }











// // #include <Arduino.h>
// // #include <WiFi.h>
// // #include <esp_now.h>
// // #include <Wire.h>

// // // ================= RECEIVER =================
// // #define MAX_CHANNELS 16

// // typedef struct {
// //   uint16_t ch[MAX_CHANNELS];
// // } PPMData;

// // PPMData receiverData;
// // float ReceiverValue[8] = {1500,1500,1000,1500,0,0,0,0};

// // uint32_t lastPacket = 0;
// // uint32_t lastPrint = 0;
// // bool systemReady = false;

// // // ================= ESP-NOW =================
// // void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

// //   // FIX: chống data lỗi gây crash / rác
// //   if (len != sizeof(PPMData)) return;

// //   memcpy(&receiverData, incomingData, sizeof(receiverData));
// //   lastPacket = millis();

// //   for (int i = 0; i < 8; i++) {
// //     ReceiverValue[i] = receiverData.ch[i];
// //   }
// // }

// // // // ================= GYRO (RAW MPU6050) =================
// // float RateRoll = 0, RatePitch = 0, RateYaw = 0;

// // void gyro_signals() {
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x3B);
// //   Wire.endTransmission();
// //   Wire.requestFrom(0x68, 6);

// //   int16_t Ax = Wire.read() << 8 | Wire.read();
// //   int16_t Ay = Wire.read() << 8 | Wire.read();
// //   int16_t Az = Wire.read() << 8 | Wire.read();

// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x43);
// //   Wire.endTransmission();
// //   Wire.requestFrom(0x68, 6);

// //   int16_t Gx = Wire.read() << 8 | Wire.read();
// //   int16_t Gy = Wire.read() << 8 | Wire.read();
// //   int16_t Gz = Wire.read() << 8 | Wire.read();

// //   RateRoll  = Gx / 65.5;
// //   RatePitch = Gy / 65.5;
// //   RateYaw   = Gz / 65.5;
// // }

// // // ================= PID =================
// // float ErrorRoll, ErrorPitch, ErrorYaw;
// // float InputRoll, InputPitch, InputYaw, InputThrottle;

// // float PR = 0.6, IR = 3.5, DR = 0.03;
// // float PP = 0.6, IP = 3.5, DP = 0.03;
// // float PY = 2.0, IY = 12,   DY = 0;

// // float PrevErrR=0, PrevErrP=0, PrevErrY=0;
// // float PrevIntR=0, PrevIntP=0, PrevIntY=0;

// // float PID[3];

// // void pid(float error, float P, float I, float D,
// //          float &prevErr, float &prevInt) {

// //   float Pterm = P * error;
// //   float Iterm = prevInt + I * (error + prevErr) * 0.004 / 2;
// //   float Dterm = D * (error - prevErr) / 0.004;

// //   Iterm = constrain(Iterm, -400, 400);

// //   float out = Pterm + Iterm + Dterm;
// //   out = constrain(out, -400, 400);

// //   prevErr = error;
// //   prevInt = Iterm;

// //   PID[0] = out;
// //   PID[1] = error;
// //   PID[2] = Iterm;
// // }

// // // ================= FAILSAFE =================
// // void failsafe() {
// //   if (millis() - lastPacket > 200) {
// //     ReceiverValue[0] = 1500;
// //     ReceiverValue[1] = 1500;
// //     ReceiverValue[2] = 1000;
// //     ReceiverValue[3] = 1500;
// //   }
// // }

// // // ================= MOTOR =================
// // void motor_write(float m1, float m2, float m3, float m4) {
// //   analogWrite(1, constrain(m1, 1000, 2000));
// //   analogWrite(2, constrain(m2, 1000, 2000));
// //   analogWrite(3, constrain(m3, 1000, 2000));
// //   analogWrite(4, constrain(m4, 1000, 2000));
// // }

// // // ================= ESP-NOW INIT =================
// // void espnow_init() {

// //   WiFi.mode(WIFI_STA);
// //   WiFi.disconnect(true, true);
// //   delay(200);

// //   if (esp_now_init() != ESP_OK) {
// //     Serial.println("ESP-NOW FAIL");
// //     return;
// //   }

// //   esp_now_register_recv_cb(onDataRecv);
// // }

// // // ================= SETUP =================
// // void setup() {

// //   Serial.begin(115200);
// //   delay(2000);
// //   Serial.println("\nBOOTING DRONE...");

// //   Wire.begin();
// //   Wire.setClock(400000);

// //   espnow_init();

// //   pinMode(5, OUTPUT);
// //   digitalWrite(5, HIGH);

// //   systemReady = true;

// //   Serial.println("DRONE READY ESP-NOW");
// // }

// // // ================= LOOP =================
// // void loop() {

// //   if (!systemReady) return;

// //   gyro_signals();
// //   failsafe();

// //   // ===== CONTROL =====
// //   float DesiredRoll  = 0.1 * (ReceiverValue[0] - 1500);
// //   float DesiredPitch = 0.1 * (ReceiverValue[1] - 1500);
// //   float DesiredYaw   = 0.15 * (ReceiverValue[3] - 1500);

// //   ErrorRoll  = DesiredRoll - RateRoll;
// //   ErrorPitch = DesiredPitch - RatePitch;
// //   ErrorYaw   = DesiredYaw - RateYaw;

// //   pid(ErrorRoll, PR, IR, DR, PrevErrR, PrevIntR);
// //   InputRoll = PID[0];

// //   pid(ErrorPitch, PP, IP, DP, PrevErrP, PrevIntP);
// //   InputPitch = PID[0];

// //   pid(ErrorYaw, PY, IY, DY, PrevErrY, PrevIntY);
// //   InputYaw = PID[0];

// //   InputThrottle = ReceiverValue[2];

// //   float m1 = InputThrottle - InputRoll - InputPitch - InputYaw;
// //   float m2 = InputThrottle + InputRoll - InputPitch + InputYaw;
// //   float m3 = InputThrottle + InputRoll + InputPitch - InputYaw;
// //   float m4 = InputThrottle - InputRoll + InputPitch + InputYaw;

// //   motor_write(m1, m2, m3, m4);

// //   // ================= CLEAN DEBUG =================
// //   if (millis() - lastPrint > 100) {
// //     lastPrint = millis();

// //     Serial.println("----------------------");
// //     Serial.println("----- DRONE DATA -----");

// //     Serial.print("RX: ");
// //     Serial.print(ReceiverValue[0]); Serial.print(" ");
// //     Serial.print(ReceiverValue[1]); Serial.print(" ");
// //     Serial.print(ReceiverValue[2]); Serial.print(" ");
// //     Serial.println(ReceiverValue[3]);

// //     Serial.print("GYRO: ");
// //     Serial.print(RateRoll); Serial.print(" ");
// //     Serial.print(RatePitch); Serial.print(" ");
// //     Serial.println(RateYaw);

// //     Serial.print("ERR: ");
// //     Serial.print(ErrorRoll); Serial.print(" ");
// //     Serial.print(ErrorPitch); Serial.print(" ");
// //     Serial.println(ErrorYaw);

// //     Serial.print("PID: ");
// //     Serial.print(PID[0]); Serial.print(" ");
// //     Serial.print(PID[1]); Serial.print(" ");
// //     Serial.println(PID[2]);

// //     Serial.print("MOTOR: ");
// //     Serial.print(m1); Serial.print(" ");
// //     Serial.print(m2); Serial.print(" ");
// //     Serial.print(m3); Serial.print(" ");
// //     Serial.println(m4);
// //   }

// //   delay(4);
// // }