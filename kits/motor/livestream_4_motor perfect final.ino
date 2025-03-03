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
    <title>ESP32-CAM Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f4f4f4;
            margin: 0;
            padding: 20px;
        }
        h1 {
            color: #333;
        }
        #videoStream {
            width: 300px;
            height: 290px;
            border: 2px solid #333;
            margin-bottom: 20px;
        }

        /* Điều hướng */
        .controls {
            position: relative;
            width: 150px;
            height: 150px;
            margin: 20px auto;
        }
        .controls button {
            position: absolute;
            width: 50px;
            height: 50px;
            border-radius: 50%;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            border: none;
            background-color: #007BFF;
            color: white;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .controls button:active {
            background-color: #0056b3;
        }

        /* Nút Dừng (Trung tâm) */
        #stop {
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background-color: red;
        }

        /* Nút xung quanh */
        #up {
            top: 0;
            left: 50%;
            transform: translateX(-50%);
        }
        #down {
            bottom: 0;
            left: 50%;
            transform: translateX(-50%);
        }
        #left {
            left: 0;
            top: 50%;
            transform: translateY(-50%);
        }
        #right {
            right: 0;
            top: 50%;
            transform: translateY(-50%);
        }

        /* Slider tốc độ */
        .slider-container {
            margin-top: 20px;
        }
        input[type="range"] {
            width: 50%;
        }
        #speedValue {
            font-weight: bold;
            color: #007BFF;
        }
    </style>
</head>
<body>

    <img id="videoStream" src="" alt="Đang tải video...">

    <!-- Điều hướng -->
    <div class="controls">
        <button id="up">⬆️</button>
        <button id="left">⬅️</button>
        <button id="stop">⏹</button>
        <button id="right">➡️</button>
        <button id="down">⬇️</button>
    </div>

    <button id="captureBtn">📸 Chụp Ảnh</button>
    <button id="toggleLightBtn">💡 Bật/Tắt Đèn</button>

    <div class="slider-container">
        <p>🚀 Tốc độ: <span id="speedValue">50</span></p>
        <input type="range" id="speedSlider" min="0" max="100" value="50">
    </div>

    <script>
        const videoStream = document.getElementById("videoStream");
        const buttons = document.querySelectorAll(".controls button");
        const captureBtn = document.getElementById("captureBtn");
        const toggleLightBtn = document.getElementById("toggleLightBtn");
        const speedSlider = document.getElementById("speedSlider");
        const speedValue = document.getElementById("speedValue");

        // Kết nối WebSocket với ESP32-CAM
        const socket = new WebSocket("ws://" + "192.168.4.1" + ":3001");

        socket.onopen = () => console.log("🔗 Kết nối WebSocket thành công!");
        socket.onerror = (error) => console.error("❌ Lỗi WebSocket:", error);
        socket.onclose = () => console.warn("⚠️ Mất kết nối WebSocket!");

        // Nhận dữ liệu hình ảnh từ ESP32-CAM
        socket.onmessage = (event) => {
            if (event.data === "button_pressed") {
                console.log("✅ ESP32-CAM đã nhận được lệnh!");
            } else {
                const blob = new Blob([event.data], { type: "image/jpeg" });
                videoStream.src = URL.createObjectURL(blob);
            }
        };

        // Xử lý sự kiện nhấn nút điều hướng
        buttons.forEach(btn => {
            btn.onclick = () => {
                if (socket.readyState === WebSocket.OPEN) {
                    socket.send(btn.id);
                    console.log(`📩 Đã gửi lệnh: ${btn.id}`);
                } else {
                    console.warn("⚠️ WebSocket chưa kết nối!");
                }
            };
        });

        // Xử lý sự kiện nhấn nút chụp ảnh
        captureBtn.onclick = () => {
            if (socket.readyState === WebSocket.OPEN) {
                socket.send("capture");
                console.log("📸 Chụp ảnh!");
            }
        };

        // Xử lý sự kiện nhấn nút bật/tắt đèn
        toggleLightBtn.onclick = () => {
            if (socket.readyState === WebSocket.OPEN) {
                socket.send("toggle_light");
                console.log("💡 Bật/Tắt đèn!");
            }
        };

        // Xử lý thay đổi tốc độ
        speedSlider.addEventListener("input", () => {
            speedValue.textContent = speedSlider.value;
        });

        speedSlider.addEventListener("change", () => {
            if (socket.readyState === WebSocket.OPEN) {
                socket.send(`speed:${speedSlider.value}`);
                console.log(`🚀 Đã gửi tốc độ: ${speedSlider.value}`);
            }
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

   sensor_t *s = esp_camera_sensor_get();
if (s != nullptr) {
  s->set_hmirror(s, 1); // Lật ngang
  s->set_vflip(s, 1);   // Lật dọc
  Serial.println("Camera flipped successfully!");
} else {
  Serial.println("Failed to access camera sensor!");
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



void moveCar(const char* motorDirection) {  // Đổi từ char -> const char*
  Serial.print("Di chuyển: ");
  Serial.println(motorDirection);

  if (strcmp(motorDirection, FORWARD) == 0) {  // So sánh chuỗi bằng strcmp
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
     Serial.println("FORWARD");
  } else if (strcmp(motorDirection, FORBACK) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    Serial.println("FORBACK");
  } else if (strcmp(motorDirection, LEFT) == 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    Serial.println("LEFT");
  } else if (strcmp(motorDirection, RIGHT) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
   Serial.println("RIGHT");
  } else if (strcmp(motorDirection, STOP) == 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
       Serial.println(STOP);
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

        case WStype_TEXT: {
            std::string command = std::string((char *)payload);
            Serial.printf("📩 Nhận lệnh: %s\n", command.c_str());

            if (command == "upBtn") {
                moveCar("up");
            } else if (command == "downBtn") {
                moveCar("downBtn");
            } else if (command == "leftBtn") {
                moveCar("leftBtn");
            } else if (command == "rightBtn") {
                moveCar("rightBtn");
            } else if (command == "stopBtn") {
                moveCar("stopBtn");
            } else if (command == "capture") {  
                // capturePhoto();  // Gọi hàm chụp ảnh
                // webSocket.sendTXT(num, "Photo Captured");
                 Serial.println("capture");
            } else if (command == "toggleLight") {
              Serial.println("toggleLight");
                // toggleLight();  // Gọi hàm bật/tắt đèn
                // webSocket.sendTXT(num, "Light Toggled");
            } else if (command.rfind("speed:", 0) == 0) {  // Kiểm tra nếu chuỗi bắt đầu bằng "speed:"
                int speed = std::stoi(command.substr(6));  // Lấy giá trị sau "speed:"
                // setSpeed(speed);

               Serial.println("speed:");
                webSocket.sendTXT(num, ("Speed Set: " + std::to_string(speed)).c_str());
            }

            webSocket.sendTXT(num, "button_pressed");
            break;
        }

        default:
            break;
    }
}


