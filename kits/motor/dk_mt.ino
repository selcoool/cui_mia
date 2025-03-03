#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";   // Tên WiFi
const char* password = "12345678"; // Mật khẩu WiFi

WebServer server(80);
WebSocketsServer webSocket(81);

#define LED_PIN 2         // Chân GPIO điều khiển LED (nếu cần)
#define ENA 5             // Chân PWM điều khiển tốc độ động cơ
#define IN1 18            // Chân điều khiển động cơ 1
#define IN2 19            // Chân điều khiển động cơ 1
#define IN3 21            // Chân điều khiển động cơ 2
#define IN4 22            // Chân điều khiển động cơ 2
#define POT_PIN 34        // Chân đọc giá trị từ Potentiometer

#define PWM_CHANNEL 0     // Kênh PWM
#define PWM_FREQ 5000     // Tần số PWM (Hz)
#define PWM_RESOLUTION 8  // Độ phân giải (8-bit, giá trị từ 0-255)

#define FORWARD 8
#define BACKWARD 2
#define LEFT 4
#define RIGHT 6
#define STOP 5




bool ledState = false;   // LED mặc định tắt
int brightness = 127;    // Độ sáng LED mặc định (nếu LED bật)
int speed = 100;         // Tốc độ di chuyển mặc định


// Hàm điều khiển động cơ
void moveCar(int motorDirection) {
 if (motorDirection == FORWARD) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (motorDirection == BACKWARD) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }else if (motorDirection == LEFT) {
      digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
   } 
   else if (motorDirection == RIGHT) {
       digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
   }

    else if (motorDirection == STOP) {
       digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
   }

   
   else {  // Dừng động cơ
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}


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
    // Hàm gửi giá trị tốc độ từ slider (nếu cần)
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
            ontouchstart="sendCommand('UP');" 
            onmousedown="sendCommand('UP');">⬆️</button>
    <div></div>
    
    <!-- Nút trái -->
    <button class="btn" 
            ontouchstart="sendCommand('LEFT');" 
            onmousedown="sendCommand('LEFT');">⬅️</button>
    <!-- Nút chính giữa: STOP -->
    <button class="btn" 
            ontouchstart="sendCommand('STOP');" 
            onmousedown="sendCommand('STOP');">⏹</button>
    <!-- Nút phải -->
    <button class="btn" 
            ontouchstart="sendCommand('RIGHT');" 
            onmousedown="sendCommand('RIGHT');">➡️</button>
    
    <div></div>
    <!-- Nút dưới -->
    <button class="btn" 
            ontouchstart="sendCommand('DOWN');" 
            onmousedown="sendCommand('DOWN');">⬇️</button>
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

// Hàm xử lý trang chủ (giao diện web)
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

// Hàm xử lý các sự kiện WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;

    // Xử lý lệnh thay đổi tốc độ (nếu sử dụng slider)
    if (message.startsWith("speed,")) {
      speed = message.substring(6).toInt();
      Serial.print("Tốc độ mới: ");
      Serial.println(speed);
    }
    // Xử lý lệnh di chuyển
    else if (message == "UP") {
      moveCar(FORWARD);
    }
    else if (message == "DOWN") {
      moveCar(BACKWARD);
    }
    else if (message == "LEFT") {
      moveCar(LEFT);
    }
    else if (message == "RIGHT") {
      moveCar(RIGHT);
    }
    // Xử lý lệnh dừng (STOP) từ nút chính giữa
    else if (message == "STOP") {
      moveCar(STOP);
    }
    // Xử lý nút tăng tốc
    else if (message == "BOOST") {
      speed += 50;
      if (speed > 255) speed = 255;
      Serial.print("Tăng tốc lên: ");
      Serial.println(speed);
    }
    // Xử lý nút bật/tắt LED
    else if (message == "TOGGLE_LED") {
      ledState = !ledState;
      ledcWrite(PWM_CHANNEL, ledState ? brightness : 0);
      Serial.println(ledState ? "LED BẬT" : "LED TẮT");
    }
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

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT_PIN, INPUT);

  // Cấu hình PWM cho chân ENA
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // Mặc định tắt LED
  
  Serial.println("ESP32 đã sẵn sàng!");
}

void loop() {
  int potValue = analogRead(POT_PIN);  // Đọc giá trị từ potentiometer (0-4095 trên ESP32)
  int motorSpeed = map(potValue, 0, 4095, 0, 255);  // Chuyển đổi thành giá trị PWM (0-255)
  
  server.handleClient();
  webSocket.loop();

  if (Serial.available()) {  // Kiểm tra nếu có dữ liệu từ Serial
    String command = Serial.readStringUntil('\n');  // Đọc toàn bộ lệnh
    command.trim();  // Xóa khoảng trắng thừa

    int commandInt = command.toInt();  // Chuyển thành số nguyên
    Serial.println(commandInt);  // In ra để kiểm tra
    moveCar(commandInt);
  }
  
  ledcWrite(PWM_CHANNEL, motorSpeed);  // Điều chỉnh tốc độ động cơ
  delay(100);  // Chờ một chút trước khi đọc lại giá trị
}
