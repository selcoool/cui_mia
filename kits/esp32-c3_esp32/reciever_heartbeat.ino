#define LED_PIN 8

#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;  // Timeout nếu không nhận dữ liệu

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔌 Client %u connected from %s\n", client_num, webSocket.remoteIP(client_num).toString().c_str());
      clientConnected = true;
      lastDataTime = millis();
      break;

    case WStype_TEXT:
      Serial.printf("📩 Received: %s\n", payload);
      lastDataTime = millis();
      break;

    case WStype_DISCONNECTED:
      Serial.printf("❌ Client %u disconnected\n", client_num);
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.softAP(ssid, password);
  Serial.println("✅ AP Started");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Gửi ping mỗi 1 giây, chờ pong 0.5 giây, nếu 1 lần không phản hồi thì ngắt kết nối
  webSocket.enableHeartbeat(1000, 500, 1);
}

void loop() {
  webSocket.loop();

  if (clientConnected) {
    if (millis() - lastDataTime > timeoutNoData) {
      Serial.println("⚠️ Không nhận dữ liệu từ client trong 5 giây. Tắt LED.");
      clientConnected = false;
      digitalWrite(LED_PIN, LOW);
    } else {
      digitalWrite(LED_PIN, HIGH);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}
