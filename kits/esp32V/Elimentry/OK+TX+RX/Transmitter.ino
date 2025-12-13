#include <esp_now.h>
#include <WiFi.h>

// ====== MAC ADDRESS DRONE ======
uint8_t droneMac[] = {0x90,0xE5,0xB1,0x99,0xA2,0xBE};

// ====== STRUCT DATA ======
typedef struct __attribute__((packed)) {
  int16_t joyX;
  int16_t joyY;
  int16_t joyZ;
  int16_t throttle;
  uint8_t button1;
  uint8_t button2;
  uint32_t timestamp;
} ControlPacket;

ControlPacket packet;

// Joystick pins
const int JOY_X = 35;
const int JOY_Y = 34;
const int JOY_Z = 33;
const int JOY_T = 32;

const int BTN1 = 25;
const int BTN2 = 26;

unsigned long lastSend = 0;
const int SEND_INTERVAL = 50; // 20Hz

// LED indicator
const int LED_PIN = 2;
unsigned long ledOffTime = 0;

// ====== CALLBACK ======
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  digitalWrite(LED_PIN, HIGH);
  ledOffTime = millis() + 30;
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, droneMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Add peer failed");
    return;
  }

  Serial.println("Transmitter ready ✅");
}

// ====== LOOP ======
void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {

    packet.joyX     = analogRead(JOY_X);
    packet.joyY     = analogRead(JOY_Y);
    packet.joyZ     = analogRead(JOY_Z);
    packet.throttle = analogRead(JOY_T);

    packet.button1 = !digitalRead(BTN1);
    packet.button2 = !digitalRead(BTN2);

    packet.timestamp = millis();

    esp_now_send(droneMac, (uint8_t*)&packet, sizeof(packet));

    lastSend = millis();
  }

  if (ledOffTime && millis() >= ledOffTime) {
    digitalWrite(LED_PIN, LOW);
    ledOffTime = 0;
  }
}
