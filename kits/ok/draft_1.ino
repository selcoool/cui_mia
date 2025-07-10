#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

MPU6050 mpu;

// PID parameters
float Kp = 30.0, Ki = 1.0, Kd = 2.0;
float pitchError, rollError;
float pitchIntegral = 0, rollIntegral = 0;
float pitchLastError = 0, rollLastError = 0;
float pitchOutput, rollOutput;

int motorFL = 16;
int motorFR = 17;
int motorBL = 18;
int motorBR = 19;

int baseSpeed = 1000;
float pitch = 0, roll = 0;

bool isArmed = false;

// LoRa config
#define SS    5
#define RST   14
#define DIO0  15
#define LED_PIN 2

unsigned long lastReceivedTime = 0;
const unsigned long timeoutDuration = 2000;  // 2 giây
unsigned long lastUpdate = 0;
const int LOOP_INTERVAL = 10;  // 10ms

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 không kết nối được");
    while (1);
  }

  // PWM setup
  ledcSetup(0, 400, 16);
  ledcSetup(1, 400, 16);
  ledcSetup(2, 400, 16);
  ledcSetup(3, 400, 16);
  ledcAttachPin(motorFL, 0);
  ledcAttachPin(motorFR, 1);
  ledcAttachPin(motorBL, 2);
  ledcAttachPin(motorBR, 3);
  stopMotors();

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // LoRa init
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Khởi động LoRa thất bại!");
    while (1);
  }
  Serial.println("LoRa đã sẵn sàng. Nhấn 'a' để ARM.");
}

void loop() {
  // Arm từ Serial
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'a') {
      isArmed = true;
      Serial.println("Đã ARM.");
    }
  }

  // Nhận dữ liệu từ LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.print("Đã nhận LoRa: ");
    Serial.println(incoming);

    lastReceivedTime = millis();        // Cập nhật thời gian nhận
    digitalWrite(LED_PIN, HIGH);       // Bật LED báo hiệu
  }

  // Nếu quá timeout mà không có dữ liệu → Tắt LED
  if (millis() - lastReceivedTime > timeoutDuration) {
    digitalWrite(LED_PIN, LOW);
  }

  // Cập nhật PID và motor mỗi 10ms
  if (millis() - lastUpdate >= LOOP_INTERVAL) {
    lastUpdate = millis();

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float accPitch = atan2((float)ax, sqrt(ay * ay + az * az)) * 180 / PI;
    float accRoll = atan2((float)ay, sqrt(ax * ax + az * az)) * 180 / PI;
    float dt = LOOP_INTERVAL / 1000.0;
    pitch = 0.98 * (pitch + gx * dt / 131.0) + 0.02 * accPitch;
    roll = 0.98 * (roll + gy * dt / 131.0) + 0.02 * accRoll;

    float pitchTarget = 0;
    float rollTarget = 0;

    // PID tính toán
    pitchError = pitchTarget - pitch;
    pitchIntegral += pitchError * dt;
    pitchIntegral = constrain(pitchIntegral, -100, 100);
    float pitchDerivative = (pitchError - pitchLastError) / dt;
    pitchLastError = pitchError;
    pitchOutput = Kp * pitchError + Ki * pitchIntegral + Kd * pitchDerivative;

    rollError = rollTarget - roll;
    rollIntegral += rollError * dt;
    rollIntegral = constrain(rollIntegral, -100, 100);
    float rollDerivative = (rollError - rollLastError) / dt;
    rollLastError = rollError;
    rollOutput = Kp * rollError + Ki * rollIntegral + Kd * rollDerivative;

    int speedFL = baseSpeed + pitchOutput + rollOutput;
    int speedFR = baseSpeed + pitchOutput - rollOutput;
    int speedBL = baseSpeed - pitchOutput + rollOutput;
    int speedBR = baseSpeed - pitchOutput - rollOutput;

    speedFL = constrain(speedFL, 0, 4095);
    speedFR = constrain(speedFR, 0, 4095);
    speedBL = constrain(speedBL, 0, 4095);
    speedBR = constrain(speedBR, 0, 4095);

    if (isArmed) {
      ledcWrite(0, speedFL);
      ledcWrite(1, speedFR);
      ledcWrite(2, speedBL);
      ledcWrite(3, speedBR);
    } else {
      stopMotors();
    }

    Serial.print("Pitch: "); Serial.print(pitch, 2);
    Serial.print(" | Roll: "); Serial.print(roll, 2);
    Serial.print(" || FL: "); Serial.print(speedFL);
    Serial.print(" | FR: "); Serial.print(speedFR);
    Serial.print(" | BL: "); Serial.print(speedBL);
    Serial.print(" | BR: "); Serial.println(speedBR);
  }
}

void stopMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
}
