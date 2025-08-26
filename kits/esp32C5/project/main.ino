#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "control_page.h"
#include "settings_page.h"
#include "product_page.h" // thêm product

const char* ssid = "ESP32_WIFI";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2
bool ledState = false;
int speed = 100;

// --- HTTP ---
void handleControl() { server.send(200, "text/html", CONTROL_page); }

void handleSettings() {
  String page = SETTINGS_page;
  page.replace("%AP_NAME%", ssid);
  page.replace("%LED%", ledState ? "BẬT" : "TẮT");
  server.send(200, "text/html", page);
}

void handleProduct() {
  server.send(200, "text/html", PRODUCT_page);
}

// --- WebSocket ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type != WStype_TEXT) return;
  String msg = (char*)payload;

  if(msg.startsWith("speed,")){
    speed = msg.substring(6).toInt();
    webSocket.broadcastTXT("speed," + String(speed));
  }
  else if(msg == "BOOST"){
    speed += 50; if(speed > 255) speed = 255;
    webSocket.broadcastTXT("speed," + String(speed));
  }
  else if(msg == "TOGGLE_LED"){
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
  else if(msg == "UP" || msg == "DOWN" || msg == "LEFT" || msg == "RIGHT" || msg == "STOP"){
    // TODO: motor
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", handleControl);
  server.on("/settings", handleSettings);
  server.on("/product", handleProduct); // thêm product
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// --- Loop ---
void loop() {
  server.handleClient();
  webSocket.loop();
}
