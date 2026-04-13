#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;

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

      // Parse dữ liệu
      {
        String data = String((char*)payload);
        if (data.startsWith("J1:X=") && data.indexOf(",Y=") != -1 && data.indexOf(",J2:X=") != -1) {
          int x1 = data.substring(data.indexOf("J1:X=") + 5, data.indexOf(",Y=")).toInt();
          int y1 = data.substring(data.indexOf(",Y=") + 3, data.indexOf(",J2:X=")).toInt();
          int x2 = data.substring(data.indexOf("J2:X=") + 5, data.lastIndexOf(",Y=")).toInt();
          int y2 = data.substring(data.lastIndexOf(",Y=") + 3).toInt();

          Serial.printf("🕹 J1: (%d, %d), J2: (%d, %d)\n", x1, y1, x2, y2);
        } else {
          Serial.println("⚠️ Invalid data format");
        }
      }
      break;

    case WStype_DISCONNECTED:
      Serial.printf("❌ Client %u disconnected\n", client_num);
      clientConnected = false;
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  Serial.println("✅ AP Started");
  Serial.println(WiFi.softAPIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  webSocket.enableHeartbeat(1000, 500, 1); // ping mỗi 1s, timeout nếu 1 lần không trả lời
}

void loop() {
  webSocket.loop();

  // Nếu cần timeout khi không có dữ liệu
  if (clientConnected && (millis() - lastDataTime > timeoutNoData)) {
    Serial.println("⚠️ Không nhận dữ liệu từ client trong 5 giây.");
    clientConnected = false;
  }
}
