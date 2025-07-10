#include <WiFi.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);

bool clientConnected = false;
unsigned long lastDataTime = 0;
const unsigned long timeoutNoData = 5000;  // 5 giây

// 🔎 Hàm lấy giá trị từ key=value
String getValue(String data, String key) {
  int startIndex = data.indexOf(key + "=");
  if (startIndex == -1) return "";
  startIndex += key.length() + 1;
  int endIndex = data.indexOf(",", startIndex);
  if (endIndex == -1) endIndex = data.length();
  return data.substring(startIndex, endIndex);
}

void webSocketEvent(uint8_t client_num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("🔌 Client %u connected from %s\n", client_num, webSocket.remoteIP(client_num).toString().c_str());
      clientConnected = true;
      lastDataTime = millis();
      break;

    case WStype_TEXT: {
      String data = String((char*)payload);
      Serial.printf("📩 Received: %s\n", data.c_str());
      lastDataTime = millis();

      // Lấy J1 và J2 từ chuỗi
      String j1 = data.substring(data.indexOf("J1:"), data.indexOf("J2:"));
      String j2 = data.substring(data.indexOf("J2:"));

      int x1 = getValue(j1, "X").toInt();
      int y1 = getValue(j1, "Y").toInt();
      int x2 = getValue(j2, "X").toInt();
      int y2 = getValue(j2, "Y").toInt();

      Serial.printf("🕹 J1: (%d, %d), J2: (%d, %d)\n", x1, y1, x2, y2);
      break;
    }

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
  webSocket.enableHeartbeat(1000, 500, 1);
}

void loop() {
  webSocket.loop();

  if (clientConnected && (millis() - lastDataTime > timeoutNoData)) {
    Serial.println("⚠️ Không nhận dữ liệu trong 5 giây.");
    clientConnected = false;
  }
}
