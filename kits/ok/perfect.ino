#include <SPI.h>
#include <LoRa.h>

// ==== Pin cấu hình ====
#define LED_PIN 2
#define SS    5
#define RST   14
#define DIO0  15

int motorPin1 = 12;
int motorPin2 = 13;
int motorPin3 = 25;
int motorPin4 = 26;

// ==== Joystick ====
int joy1Y = 2048, joy1X = 2048, joy2X = 2048;

// ==== Throttle ====
int currentThrottle = 100;
int baseThrottle = 100;

// ==== Trạng thái ====
bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 1000;

// ==== Hàm tách giá trị từ chuỗi LoRa ====
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
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // PWM
  ledcSetup(0, 1000, 8); ledcAttachPin(motorPin1, 0);
  ledcSetup(1, 1000, 8); ledcAttachPin(motorPin2, 1);
  ledcSetup(2, 1000, 8); ledcAttachPin(motorPin3, 2);
  ledcSetup(3, 1000, 8); ledcAttachPin(motorPin4, 3);

  // LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    joy1Y = getValue(received, "joy1Y").toInt();
    joy1X = getValue(received, "joy1X").toInt();
    joy2X = getValue(received, "joy2X").toInt();

    clientConnected = true;
    lastDataTime = millis();
    digitalWrite(LED_PIN, HIGH);
  }

  // Timeout nếu mất tín hiệu
  if (clientConnected && millis() - lastDataTime > timeoutNoData) {
    clientConnected = false;
    digitalWrite(LED_PIN, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    return;
  }

  if (!clientConnected) return;

  // ==== Tăng/Giảm throttle qua joy2X ====
  int joyMid = 2048;
  int delta = map(joy2X, 0, 4095, -5, 5);
  if (abs(joy2X - joyMid) > 200) {
    currentThrottle += delta;
    currentThrottle = constrain(currentThrottle, 80, 230);
  }
  baseThrottle = currentThrottle;

  // ==== Điều khiển cơ bản ====
  int forward = map(joy1Y, 0, 4095, -50, 50);
  int turn    = map(joy1X, 0, 4095, -30, 30);

  int m1 = constrain(baseThrottle + forward + turn, 0, 255);
  int m2 = constrain(baseThrottle + forward - turn, 0, 255);
  int m3 = constrain(baseThrottle - forward - turn, 0, 255);
  int m4 = constrain(baseThrottle - forward + turn, 0, 255);

  // ==== Gửi ra PWM ====
  ledcWrite(0, m1);
  ledcWrite(1, m2);
  ledcWrite(2, m3);
  ledcWrite(3, m4);

  // Debug
  Serial.print("Throttle: "); Serial.print(baseThrottle);
  Serial.print(" | M1:"); Serial.print(m1);
  Serial.print(" M2:"); Serial.print(m2);
  Serial.print(" M3:"); Serial.print(m3);
  Serial.print(" M4:"); Serial.println(m4);

  // delay(20);
}
