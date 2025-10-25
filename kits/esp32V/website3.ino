#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

const char* ssid = "ESP32_WIFI";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);
Preferences preferences;

#define LED_PIN 2
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

bool ledState = false;
int brightness = 127;
int speed = 100;

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 Điều Khiển</title>
  <script>
    var socket;
    function init() {
      socket = new WebSocket("ws://" + window.location.hostname + ":81/");
      socket.onopen = function() {
        console.log("WebSocket đã kết nối!");
      };
      socket.onmessage = function(event) {
        console.log("Nhận từ ESP32: " + event.data);
        if (event.data.startsWith("speed:")) {
          var val = event.data.split(":")[1];
          document.getElementById("speedValue").innerText = val;
          document.getElementById("speedSlider").value = val;
        }
        else if (event.data.startsWith("brightness:")) {
          var val = event.data.split(":")[1];
          document.getElementById("brightnessValue").innerText = val;
          document.getElementById("brightnessSlider").value = val;
        }
        else if (event.data.startsWith("ledState:")) {
          var state = event.data.split(":")[1];
          document.getElementById("ledStateBtn").innerText = state == "1" ? "💡 Tắt Đèn" : "💡 Bật Đèn";
        }
      };
    }

    function sendCommand(command) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
      } else {
        alert("WebSocket chưa sẵn sàng!");
      }
    }

    function sendSpeed(value) {
      document.getElementById("speedValue").innerText = value;
      sendCommand("speed," + value);
    }

    function sendBrightness(value) {
      document.getElementById("brightnessValue").innerText = value;
      sendCommand("brightness," + value);
    }

    function saveConfig() {
      sendCommand("SAVE");
      alert("Đã lưu cấu hình!");
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
  <button id="ledStateBtn" class="btn" onclick="sendCommand('TOGGLE_LED')">💡 Bật Đèn</button>
  <br><br>

  <p>Tốc độ: <span id="speedValue">100</span></p>
  <input id="speedSlider" type="range" min="0" max="255" value="100" oninput="sendSpeed(this.value)" />
  
  <br><br>

  <p>Độ sáng LED: <span id="brightnessValue">127</span></p>
  <input id="brightnessSlider" type="range" min="0" max="255" value="127" oninput="sendBrightness(this.value)" />

  <br><br>

  <button class="btn" onclick="saveConfig()">💾 Lưu cấu hình</button>
  <button class="btn" onclick="sendCommand('RESET')">🔄 Reset cấu hình</button>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // Gửi dữ liệu hiện tại cho client mới kết nối
    webSocket.sendTXT(num, "speed:" + String(speed));
    webSocket.sendTXT(num, "brightness:" + String(brightness));
    webSocket.sendTXT(num, "ledState:" + String(ledState ? 1 : 0));
  }
  else if (type == WStype_TEXT) {
    String message = (char*)payload;
    Serial.println("[WebSocket] Nhận: " + message);

    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      // Chỉ cập nhật giá trị, không lưu
      webSocket.broadcastTXT("speed:" + String(speed));
    }
    else if (message.startsWith("brightness,")) {
      brightness = message.substring(11).toInt();
      if (ledState) {
        ledcWrite(PWM_CHANNEL, brightness);
      }
      webSocket.broadcastTXT("brightness:" + String(brightness));
    }
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);
      webSocket.broadcastTXT("ledState:" + String(ledState ? 1 : 0));
    }
    else if (message == "SAVE") {
      // Lưu Preferences khi nhấn nút Lưu
      preferences.putInt("speed", speed);
      preferences.putInt("brightness", brightness);
      preferences.putBool("ledState", ledState);
      Serial.println("Đã lưu cấu hình!");
    }
    else if (message == "RESET") {
      preferences.clear();
      speed = 100;
      brightness = 127;
      ledState = false;
      ledcWrite(PWM_CHANNEL, 0);

      preferences.putInt("speed", speed);
      preferences.putInt("brightness", brightness);
      preferences.putBool("ledState", ledState);

      webSocket.broadcastTXT("speed:" + String(speed));
      webSocket.broadcastTXT("brightness:" + String(brightness));
      webSocket.broadcastTXT("ledState:0");

      Serial.println("Đã reset cấu hình về mặc định");
    }
    else if (message == "BOOST") {
      speed += 50;
      if (speed > 255) speed = 255;
      webSocket.broadcastTXT("speed:" + String(speed));
    }
    else if (message == "UP" || message == "DOWN" || message == "LEFT" || message == "RIGHT") {
      Serial.printf("Di chuyển: %s\n", message.c_str());
    }
    else if (message == "STOP") {
      Serial.println("Dừng di chuyển");
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);

  Serial.println("WiFi Access Point đã khởi động!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.softAPIP());

  preferences.begin("my-app", false);

  // Load giá trị đã lưu hoặc mặc định
  speed = preferences.getInt("speed", 100);
  brightness = preferences.getInt("brightness", 127);
  ledState = preferences.getBool("ledState", false);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);

  server.on("/", handleRoot);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("ESP32 đã sẵn sàng!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
