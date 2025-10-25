#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

bool ledState = false;
int brightness = 127;
int speed = 100;

// ---------- Trang chính ----------
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Điều Khiển</title>
  <script>
    var socket;
    var socketReady = false;

    function init() {
      socket = new WebSocket("ws://" + window.location.hostname + ":81/");
      socket.onopen = function() {
        console.log("✅ WebSocket đã kết nối!");
        socketReady = true;
        document.getElementById("status").innerText = "🟢 Đã kết nối WebSocket";
      };
      socket.onerror = function(err) {
        console.error("❌ Lỗi WebSocket:", err);
      };
      socket.onclose = function() {
        console.warn("⚠️ WebSocket đã đóng");
        socketReady = false;
        document.getElementById("status").innerText = "🔴 Mất kết nối WebSocket";
      };
      socket.onmessage = function(event) {
        console.log("📩 Nhận từ ESP32:", event.data);
        if (event.data.startsWith("speed:")) {
          document.getElementById("speedValue").innerText = event.data.split(":")[1];
        }
      };
    }

    function sendCommand(command) {
      if (socketReady) {
        socket.send(command);
      } else {
        alert("⚠️ WebSocket chưa sẵn sàng, vui lòng đợi...");
      }
    }

    function sendSpeed(value) {
      document.getElementById("speedValue").innerText = value;
      sendCommand("speed," + value);
    }
  </script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    .btn { padding: 20px; font-size: 24px; margin: 5px; cursor: pointer; }
    .grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      width: 300px;
      margin: auto;
    }
  </style>
</head>
<body onload="init()">
  <h1>ESP32 Điều Khiển 4 Hướng</h1>
  <p id="status">🟡 Đang kết nối WebSocket...</p>

  <div class="grid">
    <div></div>
    <button class="btn" ontouchstart="sendCommand('UP');" onclick="sendCommand('UP');">⬆️</button>
    <div></div>

    <button class="btn" ontouchstart="sendCommand('LEFT');" onclick="sendCommand('LEFT');">⬅️</button>
    <button class="btn" ontouchstart="sendCommand('STOP');" onclick="sendCommand('STOP');">⏹</button>
    <button class="btn" ontouchstart="sendCommand('RIGHT');" onclick="sendCommand('RIGHT');">➡️</button>

    <div></div>
    <button class="btn" ontouchstart="sendCommand('DOWN');" onclick="sendCommand('DOWN');">⬇️</button>
    <div></div>
  </div>

  <br>
  <button class="btn" onclick="sendCommand('BOOST')">🚀 Tăng tốc</button>
  <button class="btn" onclick="sendCommand('TOGGLE_LED')">💡 Bật/Tắt Đèn</button>

  <br><br>
  <p>Tốc độ: <span id="speedValue">100</span></p>
  <input id="slider" type="range" min="0" max="255" value="100" oninput="sendSpeed(this.value)">

  <br><br>
  <a href="/info"><button class="btn">ℹ️ Thông tin</button></a>
  <a href="/about"><button class="btn">📄 Giới thiệu</button></a>
</body>
</html>
)rawliteral";

// ---------- Trang /info ----------
const char INFO_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head><meta charset="UTF-8"><title>Thông tin hệ thống</title></head>
<body>
  <h2>Thông tin hệ thống</h2>
  <p>Địa chỉ IP: %IP%</p>
  <p>Tốc độ hiện tại: %SPEED%</p>
  <a href="/"><button>🔙 Quay về Trang chính</button></a>
</body>
</html>
)rawliteral";

// ---------- Trang /about ----------
const char ABOUT_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head><meta charset="UTF-8"><title>Giới thiệu</title></head>
<body>
  <h2>Giới thiệu</h2>
  <p>Dự án điều khiển ESP32 bằng WebSocket và giao diện Web.</p>
  <a href="/"><button>🔙 Quay về Trang chính</button></a>
</body>
</html>
)rawliteral";

// ---------- WebSocket ----------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;
    Serial.print("[WebSocket] Nhận lệnh: ");
    Serial.println(message);

    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      Serial.print("Tốc độ mới: ");
      Serial.println(speed);
      webSocket.broadcastTXT("speed:" + String(speed));
    }
    else if (message == "UP" || message == "DOWN" || message == "LEFT" || message == "RIGHT") {
      Serial.print("Di chuyển: ");
      Serial.println(message);
    }
    else if (message == "STOP") {
      Serial.println("Dừng di chuyển");
    }
    else if (message == "BOOST") {
      speed += 50;
      if (speed > 255) speed = 255;
      Serial.print("Tăng tốc lên: ");
      Serial.println(speed);
      webSocket.broadcastTXT("speed:" + String(speed));
    }
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);
      Serial.println(ledState ? "LED BẬT" : "LED TẮT");
    }
  }
}

// ---------- Trang xử lý ----------
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleInfo() {
  String page = INFO_page;
  page.replace("%IP%", WiFi.softAPIP().toString());
  page.replace("%SPEED%", String(speed));
  server.send(200, "text/html", page);
}

void handleAbout() {
  server.send(200, "text/html", ABOUT_page);
}

// ---------- setup ----------
void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);

  Serial.println("WiFi Access Point đã khởi động!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/info", handleInfo);
  server.on("/about", handleAbout);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  Serial.println("ESP32 đã sẵn sàng!");
}

// ---------- loop ----------
void loop() {
  server.handleClient();
  webSocket.loop();
}
