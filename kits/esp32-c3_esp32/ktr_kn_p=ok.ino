#define LED_PIN 8

#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
bool newDataReceived = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;  // 5 giây không nhận dữ liệu

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.softAP(ssid, password);
  Serial.println("✅ AP Started");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔌 Client %u connected from %s\n", client_num, webSocket.remoteIP(client_num).toString().c_str());
      clientConnected = true;
      lastDataTime = millis();
      break;

    case WStype_TEXT:
      Serial.printf("📩 Received raw: %s\n", payload);

      if (length > 0 && payload[0] != '\0') {
        String msg = String((char*)payload);
        msg.trim();

        if (msg.length() > 0) {
          Serial.println("✅ Đã nhận được dữ liệu hợp lệ từ client.");
          lastDataTime = millis();
          newDataReceived = true;
        } else {
          Serial.println("⚠️ Đã nhận nhưng nội dung rỗng.");
        }
      } else {
        Serial.println("⚠️ Không nhận được dữ liệu.");
      }
      break;

    case WStype_DISCONNECTED:
      Serial.printf("❌ Client %u disconnected\n", client_num);
      clientConnected = false;
      newDataReceived = false;
      break;

    default:
      break;
  }
}

void loop() {
  webSocket.loop();

  if (clientConnected) {
    if (millis() - lastDataTime > timeoutNoData) {
      Serial.println("⚠️ Không nhận dữ liệu từ client trong 5 giây. Tắt LED.");
      clientConnected = false;
      newDataReceived = false;
      digitalWrite(LED_PIN, LOW);
    } else if (newDataReceived) {
      // Nháy LED nếu vừa nhận dữ liệu hợp lệ
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}
