#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"

// Wi-Fi credentials
const char* ssid = "D0806";
const char* password = "INSIDESD0806";

// WebSocket server IP (thay đổi theo địa chỉ server của bạn)
const char* webSocketServer = "192.168.0.17"; // IP của WebSocket server Node.js

WebSocketsClient webSocket;

// Camera configuration
camera_config_t config;

void setup() {
  Serial.begin(115200);

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Thiết lập cấu hình cho camera ESP32
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
  config.jpeg_quality = 15; // Chất lượng thấp để truyền tải nhanh hơn
  config.fb_count = 2;

  // Khởi tạo camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera initialization failed!");
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

  // Kết nối tới WebSocket server
  webSocket.begin(webSocketServer, 3001, "/");
  webSocket.onEvent(webSocketEvent); // Đăng ký sự kiện WebSocket
}

void loop() {
  webSocket.loop(); // Kiểm tra sự kiện WebSocket

  // Chụp và gửi hình ảnh mỗi 100ms
  // static unsigned long lastTime = 0;
  // if (millis() - lastTime > 100) { // Cập nhật hình ảnh mỗi 100ms
  //   lastTime = millis();

    // Chụp khung hình từ camera
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Gửi khung hình qua WebSocket
    webSocket.sendBIN(fb->buf, fb->len);
    Serial.println("Sent image to server");

    // Trả lại bộ nhớ cho khung hình
    esp_camera_fb_return(fb);
  // }
}

// Hàm xử lý sự kiện WebSocket
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // Khi kết nối thành công
    Serial.println("Connected to WebSocket server successfully!");
    
    // Gửi tin nhắn xác nhận
    String message = "ESP32 client: Connection successful!";
    webSocket.sendTXT(message);  
    Serial.println("Sent message to server: " + message);
  } 
  else if (type == WStype_DISCONNECTED) {
    Serial.println("Disconnected from WebSocket server.");
  }
  else if (type == WStype_ERROR) {
    Serial.println("WebSocket error occurred.");
  }
  else if (type == WStype_TEXT) {
    // Xử lý tin nhắn từ server nếu có
    String receivedMessage = String((char*)payload);
    Serial.println("Received message from server: " + receivedMessage);
  }
}
