#include <esp_now.h>
#include <WiFi.h>

// Pins joystick
#define JOY1_X 33
#define JOY1_Y 32
#define JOY1_B 27
#define JOY2_X 35
#define JOY2_Y 34
#define JOY2_B 25

// MAC RX (ESP32-C3)
uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// Struct dữ liệu
typedef struct {
  int16_t joy1X;
  int16_t joy1Y;
  bool joy1Btn;
  int16_t joy2X;
  int16_t joy2Y;
  bool joy2Btn;
} JoystickData;

JoystickData joystick;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Cấu hình pins
  pinMode(JOY1_B, INPUT_PULLUP);
  pinMode(JOY2_B, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP NOW INIT FAIL");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Add peer failed");
    return;
  }

  Serial.println("TX READY");
}

void loop() {
  // Đọc joystick 1
  joystick.joy1X = analogRead(JOY1_X);
  joystick.joy1Y = analogRead(JOY1_Y);
  joystick.joy1Btn = digitalRead(JOY1_B) == LOW;

  // Đọc joystick 2
  joystick.joy2X = analogRead(JOY2_X);
  joystick.joy2Y = analogRead(JOY2_Y);
  joystick.joy2Btn = digitalRead(JOY2_B) == LOW;

  // Gửi dữ liệu qua ESP-NOW
  esp_now_send(receiverMAC, (uint8_t *)&joystick, sizeof(joystick));

  delay(50); // gửi ~20 lần/giây (có thể giảm delay để tăng tốc)
}