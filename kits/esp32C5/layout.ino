#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";   // Tên WiFi
const char* password = "12345678"; // Mật khẩu WiFi

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2   // Chân GPIO điều khiển LED (D2 trên ESP32 DevKit)

bool ledState = false;   // LED mặc định tắt
int speed = 100;         // Tốc độ mặc định

// --- Giao diện web ---
const char MAIN_page[] PROGMEM = R"rawliteral(
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
        if (event.data.startsWith("speed,")) {
          document.getElementById("speedValue").innerText = event.data.split(",")[1];
        }
      };
    }
    function sendCommand(command) {
      socket.send(command);
    }
    function sendSpeed(value) {
      socket.send("speed," + value);
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
  <div class="grid">
    <div></div>
    <!-- Nút trên -->
    <button class="btn" 
            onmousedown="sendCommand('UP')" onmouseup="sendCommand('STOP')"
            ontouchstart="sendCommand('UP')" ontouchend="sendCommand('STOP')">⬆️</button>
    <div></div>
    
    <!-- Nút trái -->
    <button class="btn" 
            onmousedown="sendCommand('LEFT')" onmouseup="sendCommand('STOP')"
            ontouchstart="sendCommand('LEFT')" ontouchend="sendCommand('STOP')">⬅️</button>
    
    <!-- Nút chính giữa: STOP -->
    <button class="btn" 
            onmousedown="sendCommand('STOP')" 
            ontouchstart="sendCommand('STOP')">⏹</button>
    
    <!-- Nút phải -->
    <button class="btn" 
            onmousedown="sendCommand('RIGHT')" onmouseup="sendCommand('STOP')"
            ontouchstart="sendCommand('RIGHT')" ontouchend="sendCommand('STOP')">➡️</button>
    
    <div></div>
    <!-- Nút dưới -->
    <button class="btn" 
            onmousedown="sendCommand('DOWN')" onmouseup="sendCommand('STOP')"
            ontouchstart="sendCommand('DOWN')" ontouchend="sendCommand('STOP')">⬇️</button>
    <div></div>
  </div>
  <br>
  <!-- Các nút khác: tăng tốc và bật/tắt LED -->
  <button class="btn" onclick="sendCommand('BOOST')">🚀 Tăng tốc</button>
  <button class="btn" onclick="sendCommand('TOGGLE_LED')">💡 Bật/Tắt Đèn</button>
  <br><br>
  <p>Tốc độ: <span id="speedValue">100</span></p>
  <input id="slider" type="range" min="0" max="255" value="100" oninput="sendSpeed(this.value)">
</body>
</html>
)rawliteral";

// --- Xử lý request HTTP ---
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

// --- Xử lý WebSocket ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;

    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      Serial.print("Tốc độ mới: ");
      Serial.println(speed);
      webSocket.broadcastTXT("speed," + String(speed));
    }
    else if (message == "UP" || message == "DOWN" || message == "LEFT" || message == "RIGHT") {
      Serial.print("Di chuyển: ");
      Serial.println(message);
      // TODO: Thêm code điều khiển motor
    }
    else if (message == "STOP") {
      Serial.println("Stop di chuyển");
      // TODO: Dừng motor
    }
    else if (message == "BOOST") {
      speed += 50;
      if (speed > 255) speed = 255;
      Serial.print("Tăng tốc lên: ");
      Serial.println(speed);
      webSocket.broadcastTXT("speed," + String(speed));
    }
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.println(ledState ? "LED BẬT" : "LED TẮT");
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WiFi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("WiFi Access Point started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  // Web server
  server.on("/", handleRoot);
  server.begin();
  
  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Mặc định tắt LED
  
  Serial.println("ESP32 đã sẵn sàng!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
