#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "MPU6050.h"

/* ===================== ESP-NOW DATA ===================== */
typedef struct __attribute__((packed)) {
  int16_t joyX;
  int16_t joyY;
  int16_t joyZ;
  int16_t throttle;
  uint8_t button1;   // ARM
  uint8_t button2;   // DISARM
  uint32_t timestamp;
} ControlPacket;

volatile ControlPacket rxData;
volatile bool newPacket = false;
unsigned long lastPacketTime = 0;

// MAC transmitter
uint8_t txMac[] = {0x14,0x2B,0x2F,0xEC,0x17,0xB8};

/* ===================== MPU6050 ===================== */
MPU6050 mpu(0x68);
#define MPU_INT_PIN 12
volatile bool mpuReady = false;

/* ===================== MOTOR ===================== */
#define MOTOR1_PIN 5
#define MOTOR2_PIN 6
#define MOTOR3_PIN 3
#define MOTOR4_PIN 4

int pwmFreq = 500;
int pwmResolution = 10;
int ch1 = 0, ch2 = 1, ch3 = 2, ch4 = 3;
int pwmMax = (1 << pwmResolution) - 1;

int motorSpeed[4] = {0,0,0,0};

/* ===================== PID ===================== */
float Kp = 1.3;
float Ki = 0.0;
float Kd = 0.08;

float rollSet = 0, pitchSet = 0;
float rollInput = 0, pitchInput = 0;
float rollOutput = 0, pitchOutput = 0;
float rollErrorPrev = 0, pitchErrorPrev = 0;
float rollIntegral = 0, pitchIntegral = 0;

/* ===================== STATE ===================== */
bool armed = false;
int baseThrottle = 0;

/* ===================== INTERRUPT ===================== */
void IRAM_ATTR mpuDataReady() {
  mpuReady = true;
}

/* ===================== MOTOR CONTROL ===================== */
void setMotor(int ch, int speed) {
  speed = constrain(speed, 0, pwmMax);
  ledcWrite(ch, speed);
}

void stopAllMotors() {
  for (int i = 0; i < 4; i++) motorSpeed[i] = 0;
  setMotor(ch1, 0);
  setMotor(ch2, 0);
  setMotor(ch3, 0);
  setMotor(ch4, 0);
}

/* ===================== PID ===================== */
float computePID(float setpoint, float input, float &integral, float &prevError) {
  float error = setpoint - input;
  integral += error * 0.01;
  float derivative = (error - prevError) / 0.01;
  prevError = error;
  return Kp * error + Ki * integral + Kd * derivative;
}

/* ===================== ESP-NOW CALLBACK ===================== */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (memcmp(mac, txMac, 6) != 0) return;  // lọc MAC transmitter
  if (len != sizeof(ControlPacket)) return;

  memcpy((void*)&rxData, incomingData, sizeof(ControlPacket));
  newPacket = true;
  lastPacketTime = millis();
}

/* ===================== SETUP ===================== */
void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- I2C MPU6050 ---
  Wire.begin(11, 10);   // SDA, SCL
  Wire.setClock(400000);

  Serial.println("Init MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) Serial.println("MPU ERROR!");
  else Serial.println("MPU OK!");

  mpu.setIntDataReadyEnabled(true);
  pinMode(MPU_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MPU_INT_PIN), mpuDataReady, RISING);

  // --- MOTOR PWM ---
  ledcSetup(ch1, pwmFreq, pwmResolution);
  ledcSetup(ch2, pwmFreq, pwmResolution);
  ledcSetup(ch3, pwmFreq, pwmResolution);
  ledcSetup(ch4, pwmFreq, pwmResolution);

  ledcAttachPin(MOTOR1_PIN, ch1);
  ledcAttachPin(MOTOR2_PIN, ch2);
  ledcAttachPin(MOTOR3_PIN, ch3);
  ledcAttachPin(MOTOR4_PIN, ch4);

  stopAllMotors();

  // --- ESP-NOW ---
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_ps(WIFI_PS_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (1);
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("DRONE FC RECEIVER READY ✅");
}

/* ===================== LOOP ===================== */
void loop() {

  // --- FAILSAFE ---
  if (millis() - lastPacketTime > 300) {
    armed = false;
    stopAllMotors();
    return;
  }

  // --- RX DATA ---
  if (newPacket) {
    newPacket = false;

    rollSet  = map(rxData.joyX, -500, 500, -15, 15);
    pitchSet = map(rxData.joyY, -500, 500, -15, 15);

    baseThrottle = map(rxData.throttle, 1000, 2000, 0, pwmMax);

     Serial.print(" THR="); Serial.print(rxData.throttle);
    Serial.print(" Roll="); Serial.print(rxData.joyX);
    Serial.print(" Pitch="); Serial.println(rxData.joyY);

    if (rxData.button1) armed = true;
    if (rxData.button2) armed = false;
  }

  if (!armed) {
    stopAllMotors();
    return;
  }

  // --- MPU6050 ---
  if (mpuReady) {
    mpuReady = false;

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    rollInput  = atan2(ay, az) * 57.3;
    pitchInput = atan2(-ax, az) * 57.3;

    rollOutput  = computePID(rollSet, rollInput, rollIntegral, rollErrorPrev);
    pitchOutput = computePID(pitchSet, pitchInput, pitchIntegral, pitchErrorPrev);

    motorSpeed[0] = baseThrottle + rollOutput - pitchOutput;
    motorSpeed[1] = baseThrottle - rollOutput - pitchOutput;
    motorSpeed[2] = baseThrottle - rollOutput + pitchOutput;
    motorSpeed[3] = baseThrottle + rollOutput + pitchOutput;

    setMotor(ch1, motorSpeed[0]);
    setMotor(ch2, motorSpeed[1]);
    setMotor(ch3, motorSpeed[2]);
    setMotor(ch4, motorSpeed[3]);

    // --- DEBUG ---
    Serial.print("ARM="); Serial.print(armed);
    // Serial.print(" THR="); Serial.print(baseThrottle);
    // Serial.print(" Roll="); Serial.print(rollInput);
    // Serial.print(" Pitch="); Serial.println(pitchInput);
  }
}

