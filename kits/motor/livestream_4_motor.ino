#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "esp_camera.h"

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
        body { text-align: center; font-family: Arial, sans-serif; background: #222; color: #fff; }
        h2 { color: #4CAF50; }
        img { max-width: 100%; border: 2px solid #4CAF50; border-radius: 10px; }
        button {
            background-color: #4CAF50; color: white; border: none;
            padding: 10px 20px; margin-top: 10px; cursor: pointer;
            border-radius: 5px; font-size: 16px;
        }
        button:hover { background-color: #45a049; }
    </style>
</head>
<body>
    <h2>📸 Truyền hình trực tiếp từ ESP32-CAM</h2>
    <img id="videoStream" src="" alt="Đang tải hình ảnh..." />
    <br>
    <button id="upBtn">upBtn</button>
     <button id="downBtn">downBtn</button>
      <button id="leftBtn">leftBtn</button>
       <button id="rightBtn">rightBtn</button>
        <button id="stopBtn">stopBtn</button>

    <script>
        const videoStream = document.getElementById("videoStream");
        const upBtn = document.getElementById("upBtn");
        const downBtn = document.getElementById("downBtn");
        const leftBtn = document.getElementById("leftBtn");
        const rightBtn = document.getElementById("rightBtn");
        const stopBtn = document.getElementById("stopBtn");
        const socket = new WebSocket("ws://" + window.location.hostname + ":3001");

        socket.onopen = () => {
            console.log("🔗 Kết nối WebSocket thành công!");
        };

        socket.onmessage = (event) => {
            if (event.data === "button_pressed") {
                alert("✅ ESP32-CAM đã nhận được sự kiện!");
            } else {
                const blob = new Blob([event.data], { type: "image/jpeg" });
                videoStream.src = URL.createObjectURL(blob);
            }
        };

        upBtn.onclick = () => {
            socket.send("upBtn");
            console.log("📩 Đã gửi sự kiện nhấn nút!");
        };

          downBtn.onclick = () => {
            socket.send("downBtn");
            console.log("📩 Đã gửi sự kiện nhấn nút!");
        };

          leftBtn.onclick = () => {
            socket.send("leftBtn");
            console.log("📩 Đã gửi sự kiện nhấn nút!");
        };

        rightBtn.onclick = () => {
            socket.send("rightBtn");
            console.log("📩 Đã gửi sự kiện nhấn nút!");
        };

          stopBtn.onclick = () => {
            socket.send("stopBtn");
            console.log("📩 Đã gửi sự kiện nhấn nút!");
        };

          
        

        socket.onerror = (error) => {
            console.error("❌ Lỗi WebSocket:", error);
        };
    </script>
</body>
</html>
)rawliteral";



void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32-CAM WebSocket Server (AP Mode)");

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

