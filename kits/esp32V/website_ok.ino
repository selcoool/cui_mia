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
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Điều Khiển</title>
  <script>
    var socket;

    function init() {
      socket = new WebSocket("ws://" + window.location.hostname + ":81/");
      socket.onopen = function() {
        console.log("WebSocket đã kết nối!");
        // Yêu cầu dữ liệu từ ESP32
        socket.send("GET_STATE");
      };
      socket.onmessage = function(event) {
        console.log("Nhận từ ESP32: " + event.data);
        if (event.data.startsWith("speed:")) {
          const value = event.data.split(":")[1];
          document.getElementById("speed").value = value;
        } else if (event.data.startsWith("brightness:")) {
          const value = event.data.split(":")[1];
          document.getElementById("brightness").value = value;
        } else if (event.data.startsWith("ledState:")) {
          const state = event.data.split(":")[1];
          document.getElementById("ledState").innerText = state == "1" ? "💡 Tắt Đèn" : "💡 Bật Đèn";
        }
      };
    }

    function saveChanges() {
      const speed = document.getElementById("speed").value;
      const brightness = document.getElementById("brightness").value;
      socket.send("speed," + speed);
      socket.send("brightness," + brightness);
      alert("Đã gửi và lưu thay đổi.");
    }

    function toggleLED() {
      socket.send("TOGGLE_LED");
    }

    function resetConfig() {
      if (confirm("Bạn có chắc muốn reset về mặc định?")) {
        socket.send("RESET");
      }
    }
  </script>
  <style>
    body { font-family: Arial; text-align: center; }
    input[type='number'] { font-size: 20px; padding: 5px; width: 80px; }
    .btn { font-size: 20px; padding: 10px 20px; margin: 10px; }
  </style>
</head>
<body onload="init()">
  <h1>⚙️ Cấu Hình ESP32</h1>

  <p>
    Tốc độ: <input type="number" id="speed" min="0" max="255" value="100"> <br><br>
    Độ sáng LED: <input type="number" id="brightness" min="0" max="255" value="127">
  </p>

  <button class="btn" onclick="saveChanges()">💾 Lưu</button>
  <button id="ledState" class="btn" onclick="toggleLED()">💡 Bật Đèn</button>
  <button class="btn" onclick="resetConfig()">🔄 Reset</button>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;
    Serial.println("[WebSocket] Nhận: " + message);

    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      preferences.putInt("speed", speed);
      webSocket.broadcastTXT("speed:" + String(speed));
    }
    else if (message.startsWith("brightness,")) {
      brightness = message.substring(11).toInt();
      preferences.putInt("brightness", brightness);
      if (ledState) {
        ledcWrite(PWM_CHANNEL, brightness);
      }
      webSocket.broadcastTXT("brightness:" + String(brightness));
    }
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      preferences.putBool("ledState", ledState);
      ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);
      webSocket.broadcastTXT("ledState:" + String(ledState));
    }
    else if (message == "RESET") {
      preferences.clear();
      speed = 100;
      brightness = 127;
      ledState = false;
      preferences.putInt("speed", speed);
      preferences.putInt("brightness", brightness);
      preferences.putBool("ledState", ledState);
      ledcWrite(PWM_CHANNEL, 0);
      webSocket.broadcastTXT("speed:" + String(speed));
      webSocket.broadcastTXT("brightness:" + String(brightness));
      webSocket.broadcastTXT("ledState:0");
    }
    else if (message == "GET_STATE") {
      webSocket.sendTXT(num, "speed:" + String(speed));
      webSocket.sendTXT(num, "brightness:" + String(brightness));
      webSocket.sendTXT(num, "ledState:" + String(ledState));
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  preferences.begin("my-config", false);

  // Load giá trị lưu trữ
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
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
