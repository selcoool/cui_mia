#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";   // Tên WiFi
const char* password = "12345678"; // Mật khẩu WiFi

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2         // Chân GPIO điều khiển LED
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8  // 8-bit (0-255)

bool ledState = false;   // LED mặc định tắt
int brightness = 127;    // Độ sáng LED mặc định (nếu LED bật)
int speed = 100;         // Tốc độ di chuyển mặc định

// Giao diện web được lưu ở flash (PROGMEM)
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
        // Có thể cập nhật giao diện nếu cần
      };
    }
    // Hàm gửi lệnh qua WebSocket
    function sendCommand(command) {
      socket.send(command);
    }
    // Hàm gửi giá trị tốc độ từ slider
    function sendSpeed(value) {
      socket.send("speed," + value);
    }
  </script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; }
    .btn { padding: 10px 20px; font-size: 20px; margin: 5px; cursor: pointer; }
    .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 5px; width: 200px; margin: auto; }
  </style>
</head>
<body onload="init()">
  <h1>ESP32 Điều Khiển 4 Hướng</h1>
  <!-- Các nút điều hướng sử dụng sự kiện ontouchstart/ontouchend cho mobile và onmousedown/onmouseup cho desktop -->
  <div class="grid">
    <div></div> 
    <button class="btn" 
            ontouchstart="sendCommand('UP');" 
            ontouchend="sendCommand('STOP');"
            onmousedown="sendCommand('UP');"
            onmouseup="sendCommand('STOP');">⬆️</button> 
    <div></div>
    <button class="btn" 
            ontouchstart="sendCommand('LEFT');" 
            ontouchend="sendCommand('STOP');"
            onmousedown="sendCommand('LEFT');"
            onmouseup="sendCommand('STOP');">⬅️</button> 
    <div></div> 
    <button class="btn" 
            ontouchstart="sendCommand('RIGHT');" 
            ontouchend="sendCommand('STOP');"
            onmousedown="sendCommand('RIGHT');"
            onmouseup="sendCommand('STOP');">➡️</button>
    <div></div> 
    <button class="btn" 
            ontouchstart="sendCommand('DOWN');" 
            ontouchend="sendCommand('STOP');"
            onmousedown="sendCommand('DOWN');"
            onmouseup="sendCommand('STOP');">⬇️</button> 
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

void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;
    
    // Xử lý lệnh thay đổi tốc độ (slider)
    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      Serial.print("Tốc độ mới: ");
      Serial.println(speed);
    } 
    // Xử lý lệnh điều hướng (nhận được UP, DOWN, LEFT, RIGHT)
    else if (message == "UP" || message == "DOWN" || message == "LEFT" || message == "RIGHT") {
      Serial.print("Di chuyển: ");
      Serial.println(message);
      // Thêm ở đây các lệnh điều khiển động cơ hoặc chuyển động theo hướng
    }
    // Xử lý lệnh STOP được gửi khi thả nút
    else if (message == "STOP") {
      Serial.println("Stop di chuyển");
      // Thêm ở đây lệnh dừng chuyển động
    }
    // Xử lý nút tăng tốc
    else if (message == "BOOST") {
      speed += 50;
      if (speed > 255) speed = 255;
      Serial.print("Tăng tốc lên: ");
      Serial.println(speed);
    }
    // Xử lý bật/tắt LED
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);
      Serial.println(ledState ? "LED BẬT" : "LED TẮT");
    }

    // (Tùy chọn) Bạn có thể gửi lại dữ liệu về giao diện nếu cần
    // webSocket.broadcastTXT(String(speed));
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  
  Serial.println("WiFi Access Point started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Cấu hình PWM cho LED
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // Mặc định tắt LED

  Serial.println("ESP32 đã sẵn sàng!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
