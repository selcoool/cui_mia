#include <SPI.h>
#include <LoRa.h>

// Motor pins
#define MOTOR1 3
#define MOTOR2 5
#define MOTOR3 6
#define MOTOR4 9

// LoRa pins
#define SS   10
#define RST  7
#define DIO0 2

int joyY = 2048;
int joyX = 2048;
int throttle = 130;

String getValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return "";
  start += key.length() + 1;
  int end = data.indexOf(" ", start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(MOTOR3, OUTPUT);
  pinMode(MOTOR4, OUTPUT);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa failed!");
    while (1);
  }
  Serial.println("LoRa ready");
}

void loop() {
  // Nhận dữ liệu LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) received += (char)LoRa.read();

    joyY = getValue(received, "joy1Y").toInt(); // tiến/lùi
    joyX = getValue(received, "joy1X").toInt(); // trái/phải
    throttle = getValue(received, "joy2X").toInt(); // tốc độ tổng

    // map throttle từ 0–4095 về 0–255
    int mappedThrottle = map(throttle, 0, 4095, 0, 255);

    // Tạo offset theo hướng
    int pitchOffset = map(joyY, 0, 4095, -50, 50);
    int rollOffset  = map(joyX, 0, 4095, -50, 50);

    int m1 = constrain(mappedThrottle + pitchOffset + rollOffset, 0, 255); // Front Left
    int m2 = constrain(mappedThrottle + pitchOffset - rollOffset, 0, 255); // Front Right
    int m3 = constrain(mappedThrottle - pitchOffset - rollOffset, 0, 255); // Rear Right
    int m4 = constrain(mappedThrottle - pitchOffset + rollOffset, 0, 255); // Rear Left

    analogWrite(MOTOR1, m1);
    analogWrite(MOTOR2, m2);
    analogWrite(MOTOR3, m3);
    analogWrite(MOTOR4, m4);

    Serial.print("Throttle: "); Serial.print(mappedThrottle);
    Serial.print(" | M1: "); Serial.print(m1);
    Serial.print(" M2: "); Serial.print(m2);
    Serial.print(" M3: "); Serial.print(m3);
    Serial.print(" M4: "); Serial.println(m4);
  }
}
