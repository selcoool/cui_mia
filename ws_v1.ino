#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "soc/rtc_cntl_reg.h" //disable brownout problems

// Cấu hình cho ESP32 Camera (AI Thinker Camera)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid = "D0806";     // Thay đổi tên mạng Wi-Fi của bạn
const char* password = "INSIDESD0806";  // Thay đổi mật khẩu Wi-Fi của bạn
const char* websockets_server_host = "192.168.0.17";  // Địa chỉ IP của WebSocket server
const uint16_t websockets_server_port = 3001;

using namespace websockets;
WebsocketsClient client;

// Callback khi nhận được tin nhắn từ server WebSocket
void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Received Message: ");
  Serial.println(message.data());
}

// Hàm khởi tạo camera
esp_err_t init_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Cấu hình kích thước và chất lượng hình ảnh
  config.frame_size = FRAMESIZE_VGA;   // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 15;            // 10-63, số thấp hơn cho chất lượng cao hơn
  config.fb_count = 2;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init FAIL: 0x%x", err);
    return err;
  }
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);
  Serial.println("Camera init OK");
  
  return ESP_OK;
}

// Hàm khởi tạo Wi-Fi
esp_err_t init_wifi() {
  WiFi.begin(ssid, password);
  Serial.println("WiFi init ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi OK");

  client.onMessage(onMessageCallback);  // Đăng ký callback cho tin nhắn
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/");  // Kết nối đến WebSocket server
  if (!connected) {
    Serial.println("WS connect failed!");
    return ESP_FAIL;
  }

  Serial.println("WS OK");
  client.send("Hello from ESP32 camera stream!");  // Gửi thông điệp ban đầu
  return ESP_OK;
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  init_camera();
  init_wifi();
}

void loop() {
  if (client.available()) {
    camera_fb_t *fb = esp_camera_fb_get();  // Lấy ảnh từ camera
    if (!fb) {
      Serial.println("Image capture failed");
      esp_camera_fb_return(fb);
      ESP.restart();  // Khởi động lại nếu không lấy được ảnh
    }
    client.sendBinary((const char*) fb->buf, fb->len);  // Gửi dữ liệu ảnh qua WebSocket
    Serial.println("Image sent");
    esp_camera_fb_return(fb);  // Trả lại buffer cho camera
    client.poll();  // Lắng nghe sự kiện WebSocket
  }
}