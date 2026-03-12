#include <esp_now.h>
#include <WiFi.h>

// Struct dữ liệu phải giống TX
typedef struct {
  int16_t joy1X;
  int16_t joy1Y;
  bool joy1Btn;
  int16_t joy2X;
  int16_t joy2Y;
  bool joy2Btn;
} JoystickData;

JoystickData joystick;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if(len != sizeof(joystick)) return; // kiểm tra kích thước

  memcpy(&joystick, incomingData, sizeof(joystick));

  Serial.print("J1 X: "); Serial.print(joystick.joy1X);
  Serial.print(" Y: "); Serial.print(joystick.joy1Y);
  Serial.print(" Btn: "); Serial.print(joystick.joy1Btn);

  Serial.print(" | J2 X: "); Serial.print(joystick.joy2X);
  Serial.print(" Y: "); Serial.print(joystick.joy2Y);
  Serial.print(" Btn: "); Serial.println(joystick.joy2Btn);
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP NOW INIT FAIL");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW RX READY");
}

void loop() {
  // RX không cần loop
}