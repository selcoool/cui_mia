#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ==== Định nghĩa chân ====
#define LED_PIN 2
#define MOTOR1 3
#define MOTOR2 5
#define MOTOR3 6
#define MOTOR4 9
#define VOLTAGE_PIN A0

RF24 radio(7, 8);  // CE, CSN

const byte address[6] = "00001";
unsigned long lastReceiveTime = 0;
const unsigned long timeout = 500; // 500ms
#define LOW_VOLTAGE 8.0

int joyY = 2048;
int joyX = 2048;
int throttle = 0;

unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long lastPrintTime = 0;  // Thời gian in thông tin lần cuối

// ==== Tách giá trị từ chuỗi LoRa ====
String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

// ==== Đọc điện áp từ chia áp ====
float readVoltage() {
  int raw = analogRead(VOLTAGE_PIN);
  float voltage = (raw / 1023.0) * 5.0;
  return voltage * 2.0;  // Chia áp 1:1
}

void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  bool signalReceived = false;

  // Khai báo tốc độ motor mặc định
  int m1 = 0, m2 = 0, m3 = 0, m4 = 0;

  if (radio.available()) {
    char text[32] = {0};
    radio.read(&text, sizeof(text));
    lastReceiveTime = millis();
    signalReceived = true;

    String msg = String(text);
    joyY = getValue(msg, "q").toInt();
    joyX = getValue(msg, "w").toInt();
    throttle = getValue(msg, "e").toInt();

    int mappedThrottle = map(throttle, 0, 4095, 0, 255);
    int pitchOffset = map(joyY, 0, 4095, -50, 50);
    int rollOffset  = map(joyX, 0, 4095, -50, 50);

    m1 = constrain(mappedThrottle + pitchOffset + rollOffset, 0, 255);
    m2 = constrain(mappedThrottle + pitchOffset - rollOffset, 0, 255);
    m3 = constrain(mappedThrottle - pitchOffset - rollOffset, 0, 255);
    m4 = constrain(mappedThrottle - pitchOffset + rollOffset, 0, 255);

    analogWrite(MOTOR1, m1);
    analogWrite(MOTOR2, m2);
    analogWrite(MOTOR3, m3);
    analogWrite(MOTOR4, m4);
  }

  float batteryVoltage = readVoltage();

  // ✅ Trường hợp có tín hiệu
  if (millis() - lastReceiveTime <= timeout) {
    if (batteryVoltage < LOW_VOLTAGE) {
      // ⚠️ Có tín hiệu nhưng pin yếu → LED chớp
      if (millis() - lastBlinkTime > 300) {
        lastBlinkTime = millis();
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
    } else {
      // ✅ Có tín hiệu + pin khỏe → LED sáng
      digitalWrite(LED_PIN, HIGH);
    }
  } else {
    // ❌ Mất tín hiệu → tắt motor & tắt LED
    analogWrite(MOTOR1, 0);
    analogWrite(MOTOR2, 0);
    analogWrite(MOTOR3, 0);
    analogWrite(MOTOR4, 0);
    digitalWrite(LED_PIN, LOW);
  }

  // 🖨 In thông tin mỗi 500ms
  if (millis() - lastPrintTime >= 500) {
    lastPrintTime = millis();
    Serial.print("Dien ap pin: ");
    Serial.print(batteryVoltage, 2);
    Serial.print(" V | Toc do: ");
    Serial.println(throttle);

    Serial.print("PWM MOTOR1: ");
    Serial.print(m1);
    Serial.print(" | MOTOR2: ");
    Serial.print(m2);
    Serial.print(" | MOTOR3: ");
    Serial.print(m3);
    Serial.print(" | MOTOR4: ");
    Serial.println(m4);
  }

  delay(30);
}
