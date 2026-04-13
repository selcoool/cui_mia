#define LED_PIN 8

#define MOTOR_PIN_1 2
#define MOTOR_PIN_2 3
#define MOTOR_PIN_3 6
#define MOTOR_PIN_4 7

#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;

unsigned long previousMillis = 0;
const long interval = 500;
bool ledState = LOW;

const int pwmFreq = 5000;
const int pwmResolution = 8;

// PWM channels cho 4 motor
const int chMotor1 = 0;
const int chMotor2 = 1;
const int chMotor3 = 2;
const int chMotor4 = 3;

int motorSpeed = 0;  // Giá trị PWM 0-255

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length);

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup PWM cho 4 motor pins
  ledcSetup(chMotor1, pwmFreq, pwmResolution);
  ledcAttachPin(MOTOR_PIN_1, chMotor1);

  ledcSetup(chMotor2, pwmFreq, pwmResolution);
  ledcAttachPin(MOTOR_PIN_2, chMotor2);

  ledcSetup(chMotor3, pwmFreq, pwmResolution);
  ledcAttachPin(MOTOR_PIN_3, chMotor3);

  ledcSetup(chMotor4, pwmFreq, pwmResolution);
  ledcAttachPin(MOTOR_PIN_4, chMotor4);

  // Khởi động PWM 0 (tắt motor)
  ledcWrite(chMotor1, 0);
  ledcWrite(chMotor2, 0);
  ledcWrite(chMotor3, 0);
  ledcWrite(chMotor4, 0);

  WiFi.softAP(ssid, password);
  Serial.println("✅ AP Started");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  webSocket.enableHeartbeat(1000, 500, 1);
}

void loop() {
  webSocket.loop();

  if (clientConnected) {
    if (millis() - lastDataTime > timeoutNoData) {
      Serial.println("⚠️ Không nhận dữ liệu trong 5 giây, tắt motor và LED");
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
      ledState = LOW;

      motorSpeed = 0;
      updateAllMotors(motorSpeed);
    } else {
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
      updateAllMotors(motorSpeed);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    ledState = LOW;

    motorSpeed = 0;
    updateAllMotors(motorSpeed);
  }
}

void updateAllMotors(int speed) {
  ledcWrite(chMotor1, speed);
  ledcWrite(chMotor2, speed);
  ledcWrite(chMotor3, speed);
  ledcWrite(chMotor4, speed);
}

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔌 Client %u connected\n", client_num);
      clientConnected = true;
      lastDataTime = millis();
      break;

    case WStype_TEXT: {
      payload[length] = '\0';
      String msg = (char*)payload;
      Serial.printf("📩 Received: %s\n", msg.c_str());
      lastDataTime = millis();

      int j1x_index = msg.indexOf("J1:X=");
      if (j1x_index != -1) {
        int start = j1x_index + 5;
        int end = msg.indexOf(',', start);
        if (end == -1) end = msg.length();
        int rawValue = msg.substring(start, end).toInt();

        motorSpeed = map(rawValue, 1900, 2100, 0, 255);
        motorSpeed = constrain(motorSpeed, 0, 255);
        Serial.printf("Motor speed set to %d\n", motorSpeed);
      }
      break;
    }

    case WStype_DISCONNECTED:
      Serial.printf("❌ Client %u disconnected\n", client_num);
      clientConnected = false;
      motorSpeed = 0;
      updateAllMotors(motorSpeed);
      break;

    default:
      break;
  }
}
