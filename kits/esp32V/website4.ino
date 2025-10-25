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
        requestState();
      };
      socket.onmessage = function(event) {
        console.log("Nhận từ ESP32: " + event.data);
        if (event.data.startsWith("speed:")) {
          var val = event.data.split(":")[1];
          updateSpeedUI(val);
        }
        else if (event.data.startsWith("brightness:")) {
          var val = event.data.split(":")[1];
          updateBrightnessUI(val);
        }
        else if (event.data.startsWith("ledState:")) {
          var state = event.data.split(":")[1];
          document.getElementById("ledStateBtn").innerText = state == "1" ? "💡 Tắt Đèn" : "💡 Bật Đèn";
        }
      };
      socket.onclose = function() {
        console.log("WebSocket bị đóng, đang thử kết nối lại...");
        setTimeout(init, 2000);
      };
    }

    function sendCommand(command) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
      } else {
        alert("WebSocket chưa sẵn sàng!");
      }
    }

    function updateSpeed(value) {
      value = parseInt(value);
      if (isNaN(value)) return;
      if (value < 0) value = 0;
      if (value > 255) value = 255;
      updateSpeedUI(value);
      sendCommand("speed," + value);
    }

    function updateBrightness(value) {
      value = parseInt(value);
      if (isNaN(value)) return;
      if (value < 0) value = 0;
      if (value > 255) value = 255;
      updateBrightnessUI(value);
      sendCommand("brightness," + value);
    }

    function updateSpeedUI(value) {
      document.getElementById("speedValue").innerText = value;
      document.getElementById("speedSlider").value = value;
      document.getElementById("speedInput").value = value;
    }

    function updateBrightnessUI(value) {
      document.getElementById("brightnessValue").innerText = value;
      document.getElementById("brightnessSlider").value = value;
      document.getElementById("brightnessInput").value = value;
    }

    function toggleLED() {
      sendCommand("TOGGLE_LED");
    }

    function saveConfig() {
      sendCommand("SAVE");
      alert("Đã lưu cấu hình!");
    }

    function resetConfig() {
      if (confirm("Bạn có chắc muốn reset về mặc định?")) {
        sendCommand("RESET");
      }
    }

    function refreshConfig() {
      requestState();
    }

    function requestState() {
      sendCommand("GET_STATE");
    }
  </script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 20px; }
    .btn { padding: 15px 25px; font-size: 20px; margin: 8px; cursor: pointer; }
    input[type=range] { width: 250px; }
    input[type=number] { width: 60px; font-size: 18px; padding: 5px; margin-left: 10px; }
  </style>
</head>
<body onload="init()">
  <h1>⚙️ Cấu Hình ESP32</h1>

  <p>Tốc độ: <span id="speedValue">100</span></p>
  <input id="speedSlider" type="range" min="0" max="255" value="100" oninput="updateSpeed(this.value)" />
  <input id="speedInput" type="number" min="0" max="255" value="100" oninput="updateSpeed(this.value)" />

  <br><br>

  <p>Độ sáng LED: <span id="brightnessValue">127</span></p>
  <input id="brightnessSlider" type="range" min="0" max="255" value="127" oninput="updateBrightness(this.value)" />
  <input id="brightnessInput" type="number" min="0" max="255" value="127" oninput="updateBrightness(this.value)" />

  <br><br>

  <button class="btn" id="ledStateBtn" onclick="toggleLED()">💡 Bật Đèn</button>
  <br>
  <button class="btn" onclick="saveConfig()">💾 Lưu cấu hình</button>
  <button class="btn" onclick="resetConfig()">🔄 Reset cấu hình</button>
  <button class="btn" onclick="refreshConfig()">🔃 Làm mới</button>

</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // Gửi trạng thái hiện tại cho client mới kết nối
    webSocket.sendTXT(num, "speed:" + String(speed));
    webSocket.sendTXT(num, "brightness:" + String(brightness));
    webSocket.sendTXT(num, "ledState:" + String(ledState ? 1 : 0));
  }
  else if (type == WStype_TEXT) {
    String message = (char*)payload;
    Serial.println("[WebSocket] Nhận: " + message);

    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
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
    else if (message == "GET_STATE") {
      webSocket.sendTXT(num, "speed:" + String(speed));
      webSocket.sendTXT(num, "brightness:" + String(brightness));
      webSocket.sendTXT(num, "ledState:" + String(ledState ? 1 : 0));
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
