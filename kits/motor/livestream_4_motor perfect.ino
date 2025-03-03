#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"


#define ENA 12      // Chân PWM điều khiển tốc độ động cơ
#define IN1 13     // Chân điều khiển động cơ 1
#define IN2 15     // Chân điều khiển động cơ 1
#define IN3 14     // Chân điều khiển động cơ 2
#define IN4 2     // Chân điều khiển động cơ 2
#define POT_PIN 34 // Chân đọc giá trị từ Potentiometer


#define PWM_CHANNEL 0  // Kênh PWM
#define PWM_FREQ 5000  // Tần số PWM (Hz)
#define PWM_RESOLUTION 8  // Độ phân giải (8-bit, giá trị từ 0-255)


#define FORWARD "upBtn"
#define FORBACK "downBtn"
#define LEFT "leftBtn"
#define RIGHT "rightBtn"
#define STOP "stopBtn"



// *Cấu hình Access Point*
const char* ssid = "ESP32-CAM";
const char* password = "12345678";

// *Khởi tạo server*
WebServer server(80);
WebSocketsServer webSocket(3001);

// *Khai báo trước hàm xử lý sự kiện WebSocket*
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

// *Cấu hình camera*
camera_config_t config;


// *Giao diện HTML*
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM Live Stream</title>
    <style>
        body {
            text-align: center; font-family: Arial, sans-serif;
            background: #222; color: #fff;
        }
        h2 { color: #4CAF50; }
        img {
            max-width: 100%; border: 2px solid #4CAF50;
            border-radius: 10px; margin-bottom: 20px;
        }
        
        /* Bố cục nút điều hướng */
        .controls {
            display: grid;
            grid-template-columns: 60px 60px 60px;
            grid-template-rows: 60px 60px 60px;
            gap: 10px;
            justify-content: center;
            align-items: center;
            margin-top: 20px;
        }

        /* Định dạng chung cho nút */
        button {
            background-color: #4CAF50; color: white; border: none;
            padding: 10px; cursor: pointer; border-radius: 5px;
            font-size: 16px; width: 60px; height: 60px;
            display: flex; align-items: center; justify-content: center;
        }
        button:hover { background-color: #45a049; }

        /* Định vị trí các nút trong Grid */
        #upBtn { grid-column: 2; grid-row: 1; }  /* Nút lên */
        #leftBtn { grid-column: 1; grid-row: 2; }  /* Nút trái */
        #stopBtn { grid-column: 2; grid-row: 2; background: red; }  /* Nút dừng */
        #rightBtn { grid-column: 3; grid-row: 2; }  /* Nút phải */
        #downBtn { grid-column: 2; grid-row: 3; }  /* Nút xuống */

    </style>
</head>
<body>
    <h2>📸 Truyền hình trực tiếp từ ESP32-CAM</h2>
    <img id="videoStream" src="" alt="Đang tải hình ảnh..." />

    <div class="controls">
        <button id="upBtn">🔼</button>
        <button id="leftBtn">◀️</button>
        <button id="stopBtn">⏹️</button>
        <button id="rightBtn">▶️</button>
        <button id="downBtn">🔽</button>
    </div>

    <script>
        const videoStream = document.getElementById("videoStream");
        const buttons = document.querySelectorAll("button");
        // const socket = new WebSocket("ws://" + window.location.hostname + ":3001");

        const socket = new WebSocket("ws://" + "192.168.4.1" + ":3001");

        socket.onopen = () => console.log("🔗 Kết nối WebSocket thành công!");
        socket.onerror = (error) => console.error("❌ Lỗi WebSocket:", error);
        socket.onclose = () => console.warn("⚠️ Mất kết nối WebSocket!");

        socket.onmessage = (event) => {
            if (event.data === "button_pressed") {
                alert("✅ ESP32-CAM đã nhận được sự kiện!");
            } else {
                const blob = new Blob([event.data], { type: "image/jpeg" });
                videoStream.src = URL.createObjectURL(blob);
            }
        };

        buttons.forEach(btn => {
            btn.onclick = () => {
                if (socket.readyState === WebSocket.OPEN) {
                    socket.send(btn.id);
                    console.log(`📩 Đã gửi sự kiện: ${btn.id}`);
                } else {
                    console.warn("⚠️ WebSocket chưa kết nối!");
                }
            };
        });
    </script>
</body>
</html>
)rawliteral";



void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32-CAM WebSocket Server (AP Mode)");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(POT_PIN, INPUT);

  // Cấu hình PWM cho chân ENA
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL);

  // *Khởi tạo Access Point*
  WiFi.softAP(ssid, password);
  Serial.println("✅ Access Point đã khởi động!");
  Serial.print("📶 Địa chỉ IP: ");
  Serial.println(WiFi.softAPIP());

  // *Cấu hình camera*
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 15;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("❌ Lỗi: Không thể khởi tạo camera!");
    return;
  }

   sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);
  Serial.println("Camera init OK");

  // *Gửi giao diện HTML khi truy cập địa chỉ IP*
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  // *Khởi động WebSocket server*
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // *Khởi động Web Server*
  server.begin();
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // *Chụp ảnh từ camera*
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Không thể chụp ảnh từ camera!");
    return;
  }

  // *Gửi ảnh đến tất cả client WebSocket*
  webSocket.broadcastBIN(fb->buf, fb->len);
  // Serial.println("📤 Đã gửi ảnh tới tất cả client!");

  // *Giải phóng bộ nhớ*
  esp_camera_fb_return(fb);
}



void moveCar(const char* motorDirection) {  // Đổi từ char -> const char*
  Serial.print("Di chuyển: ");
  Serial.println(motorDirection);

  if (strcmp(motorDirection, FORWARD) == 0) {  // So sánh chuỗi bằng strcmp
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (strcmp(motorDirection, FORBACK) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else if (strcmp(motorDirection, LEFT) == 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else if (strcmp(motorDirection, RIGHT) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (strcmp(motorDirection, STOP) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  } else {
    Serial.println("⚠️ Lệnh không hợp lệ");
  }
}






// *Xử lý sự kiện WebSocket*
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("🔗 Client %u đã kết nối!\n", num);
            webSocket.sendTXT(num, "Xin chào từ ESP32-CAM!");
            break;

        case WStype_DISCONNECTED:
            Serial.printf("❌ Client %u đã ngắt kết nối!\n", num);
            break;

        case WStype_TEXT:
            if (strcmp((char *)payload, "upBtn") == 0) {
                Serial.println("📩 Nhận được sự kiện nhấn nút từ client upBtn!");
                moveCar("upBtn");
                webSocket.sendTXT(num, "button_pressed");
            }
             else if (strcmp((char *)payload, "downBtn") == 0) {
                Serial.println("📩 Nhận được sự kiện nhấn nút từ client! downBtn");
                webSocket.sendTXT(num, "button_pressed");
            }
             else if (strcmp((char *)payload, "leftBtn") == 0) {
                Serial.println("📩 Nhận được sự kiện nhấn nút từ client! leftBtn");
                webSocket.sendTXT(num, "button_pressed");
            }
             else if (strcmp((char *)payload, "rightBtn") == 0) {
                Serial.println("📩 Nhận được sự kiện nhấn nút từ client! rightBtn");
                webSocket.sendTXT(num, "button_pressed");
            }
             else if (strcmp((char *)payload, "stopBtn") == 0) {
                Serial.println("📩 Nhận được sự kiện nhấn nút từ client stopBtn! ");
                webSocket.sendTXT(num, "button_pressed");
            }
            break;

        default:
            break;
    }
}

