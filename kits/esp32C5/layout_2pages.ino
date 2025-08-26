#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";   // Tên AP
const char* password = "12345678"; // Mật khẩu AP

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2
bool ledState = false;
int speed = 100;

// --- Trang điều khiển ---
const char CONTROL_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Điều Khiển</title>
<script>
var socket;
function init() {
  socket = new WebSocket("ws://" + window.location.hostname + ":81/");
  socket.onmessage = function(event) {
    if(event.data.startsWith("speed,")){
      document.getElementById("speedValue").innerText = event.data.split(",")[1];
    }
  };
}
function sendCommand(cmd){ socket.send(cmd); }
function sendSpeed(val){ socket.send("speed," + val); }
</script>
<style>
body { font-family: Arial; text-align: center; }
.btn { padding: 20px; font-size: 24px; margin: 5px; cursor: pointer; }
.grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; width: 300px; margin: auto; }
</style>
</head>
<body onload="init()">
<h2>ESP32 Điều Khiển 4 Hướng</h2>
<div class="grid">
  <div></div>
  <button class="btn" onmousedown="sendCommand('UP')" onmouseup="sendCommand('STOP')">⬆️</button>
  <div></div>
  <button class="btn" onmousedown="sendCommand('LEFT')" onmouseup="sendCommand('STOP')">⬅️</button>
  <button class="btn" onmousedown="sendCommand('STOP')">⏹</button>
  <button class="btn" onmousedown="sendCommand('RIGHT')" onmouseup="sendCommand('STOP')">➡️</button>
  <div></div>
  <button class="btn" onmousedown="sendCommand('DOWN')" onmouseup="sendCommand('STOP')">⬇️</button>
  <div></div>
</div>
<br>
<button class="btn" onclick="sendCommand('BOOST')">🚀 Tăng tốc</button>
<button class="btn" onclick="sendCommand('TOGGLE_LED')">💡 Bật/Tắt LED</button>
<p>Tốc độ: <span id="speedValue">100</span></p>
<input type="range" min="0" max="255" value="100" oninput="sendSpeed(this.value)">
<br><br>
<a href="/settings">➡️ Trang thông tin</a>
</body>
</html>
)rawliteral";

// --- Trang thông tin / settings ---
const char SETTINGS_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Info</title>
<style>body{font-family:Arial;text-align:center;}</style>
</head>
<body>
<h2>Thông tin ESP32</h2>
<p>ESP32 đang phát WiFi AP: <strong>%AP_NAME%</strong></p>
<p>Trạng thái LED: <strong>%LED%</strong></p>
<br>
<a href="/">⬅️ Quay lại trang điều khiển</a>
</body>
</html>
)rawliteral";

// --- Xử lý HTTP ---
void handleControl() { server.send(200, "text/html", CONTROL_page); }

void handleSettings() {
  String page = SETTINGS_page;
  page.replace("%AP_NAME%", ssid);
  page.replace("%LED%", ledState ? "BẬT" : "TẮT");
  server.send(200, "text/html", page);
}

// --- Xử lý WebSocket ---
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
    // TODO: Thêm code motor nếu cần
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);

  // WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Web server
  server.on("/", handleControl);
  server.on("/settings", handleSettings);
  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("ESP32 sẵn sàng!");
}

// --- Loop ---
void loop() {
  server.handleClient();
  webSocket.loop();
}
