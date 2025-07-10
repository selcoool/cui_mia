#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "ESP32-AP";
const char* password = "12345678";

WebSocketsClient webSocket;

// Chân analog joystick 1
const int joy1X = 34;  // A0
const int joy1Y = 35;  // A1

// Chân analog joystick 2
const int joy2X = 32;  // A2
const int joy2Y = 33;  // A3

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("✅ Connected to WebSocket server!");
      break;
    case WStype_DISCONNECTED:
      Serial.println("❌ Disconnected from server");
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("📶 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi!");

  webSocket.begin("192.168.4.1", 81, "/"); // Địa chỉ IP của server
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  int x1 = analogRead(joy1X);
  int y1 = analogRead(joy1Y);
  int x2 = analogRead(joy2X);
  int y2 = analogRead(joy2Y);

  String message = "J1:X1=" + String(x1) + ",Y1=" + String(y1) +
                   " | J2:X2=" + String(x2) + ",Y=2" + String(y2);
  webSocket.sendTXT(message);

  Serial.println("📤 Sent: " + message);
  delay(200);
}
