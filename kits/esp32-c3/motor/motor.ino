#define LED_PIN 8
#define MOTOR_PIN 2

#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;  // Timeout nếu không nhận dữ liệu

unsigned long previousMillis = 0;
const long interval = 500;  // Thời gian chớp tắt LED (ms)
bool ledState = LOW;

int motorSpeed = 0;  // Giá trị PWM 0-255

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length);

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup PWM cho motor ở pin 2
  ledcSetup(0, 5000, 8);      // channel 0, 5kHz, 8 bit
  ledcAttachPin(MOTOR_PIN, 0);
  ledcWrite(0, 0);            // bắt đầu với tốc độ 0

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
      Serial.println("⚠️ Không nhận dữ liệu từ client trong 5 giây. Tắt LED và motor.");
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
      ledState = LOW;
      motorSpeed = 0;
      ledcWrite(0, motorSpeed);
    } else {
      // Chớp tắt LED mỗi 500ms
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
      }
      // Cập nhật PWM motor dựa trên motorSpeed đã set khi nhận dữ liệu
      ledcWrite(0, motorSpeed);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    ledState = LOW;
    motorSpeed = 0;
    ledcWrite(0, motorSpeed);
  }
}

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔌 Client %u connected from %s\n", client_num, webSocket.remoteIP(client_num).toString().c_str());
      clientConnected = true;
      lastDataTime = millis();
      break;

    case WStype_TEXT: {
      payload[length] = '\0'; // Đảm bảo chuỗi kết thúc
      String msg = (char*)payload;
      Serial.printf("📩 Received: %s\n", msg.c_str());
      lastDataTime = millis();

      // Lấy giá trị J1:X từ chuỗi
      int j1x_index = msg.indexOf("J1:X=");
      if (j1x_index != -1) {
        int start = j1x_index + 5; // vị trí bắt đầu số sau "J1:X="
        int end = msg.indexOf(',', start);
        if (end == -1) end = msg.length();
        String valStr = msg.substring(start, end);
        int rawValue = valStr.toInt();

        // Map giá trị 1900-2100 về 0-255
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
      ledcWrite(0, motorSpeed);
      break;

    default:
      break;
  }
}
