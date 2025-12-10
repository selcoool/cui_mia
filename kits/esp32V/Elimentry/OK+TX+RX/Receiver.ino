#include <esp_now.h>
#include <WiFi.h>

// MAC của Transmitter
uint8_t txMac[] = {0x14,0x2B,0x2F,0xEC,0x17,0xB8};

// Struct giống transmitter
typedef struct __attribute__((packed)) {
  int16_t joyX;
  int16_t joyY;
  int16_t joyZ;
  int16_t throttle;
  uint8_t button1;
  uint8_t button2;
  uint32_t timestamp;
} ControlPacket;

ControlPacket rxData;

const int LED_PIN = 2;
unsigned long ledOffTime = 0;

// ====== CALLBACK NHẬN DỮ LIỆU ======
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(ControlPacket)) {
    memcpy(&rxData, incomingData, len);

    Serial.print("X="); Serial.print(rxData.joyX);
    Serial.print(" Y="); Serial.print(rxData.joyY);
    Serial.print(" Z="); Serial.print(rxData.joyZ);
    Serial.print(" T="); Serial.print(rxData.throttle);
    Serial.print(" B1="); Serial.print(rxData.button1);
    Serial.print(" B2="); Serial.println(rxData.button2);

    digitalWrite(LED_PIN, HIGH);
    ledOffTime = millis() + 30;
  }
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, txMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Add peer failed");
    return;
  }

  Serial.println("Drone receiver ready ✅");
}

// ====== LOOP ======
void loop() {
  if (ledOffTime && millis() >= ledOffTime) {
    digitalWrite(LED_PIN, LOW);
    ledOffTime = 0;
  }
}
