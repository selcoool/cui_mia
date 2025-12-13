#include <esp_now.h>
#include <WiFi.h>

// ====== MAC ADDRESS DRONE ======
uint8_t droneMac[] = {0x90,0xE5,0xB1,0x99,0xA2,0xBE};

// ====== STRUCT DATA ======
typedef struct __attribute__((packed)) {
  int16_t joyX;       // Roll -2048..+2047
  int16_t joyY;       // Pitch
  int16_t joyZ;       // Yaw
  int16_t throttle;   // Throttle
  uint8_t button1;    // ARM
  uint8_t button2;    // DISARM
  uint32_t timestamp;
} ControlPacket;

ControlPacket packet;

// ====== PINS ======
const int JOY_X = 35;
const int JOY_Y = 34;
const int JOY_Z = 33;
const int JOY_T = 32;
const int BTN1 = 25;
const int BTN2 = 26;
const int LED_PIN = 2;

// ====== SEND INTERVAL ======
unsigned long lastSend = 0;
const int SEND_INTERVAL = 50; // 20Hz
unsigned long ledOffTime = 0;

// ====== JOYSTICK CENTER ======
int joyX_center = 2048;
int joyY_center = 2048;
int joyZ_center = 2048;
int joyT_center = 2048;

// ====== CALLBACK GỬI DỮ LIỆU ======
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  digitalWrite(LED_PIN, HIGH);
  ledOffTime = millis() + 30;
}

// ====== CALIBRATE JOYSTICK ======
int readCenter(int pin) {
  long sum = 0;
  for(int i=0;i<50;i++){
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 50;
}

void calibrateJoystick() {
  Serial.println("Calibrating joystick, keep it centered...");
  joyX_center = readCenter(JOY_X);
  joyY_center = readCenter(JOY_Y);
  joyZ_center = readCenter(JOY_Z);
  joyT_center = readCenter(JOY_T);
  Serial.println("Calibration done:");
  Serial.print("X_center="); Serial.println(joyX_center);
  Serial.print("Y_center="); Serial.println(joyY_center);
  Serial.print("Z_center="); Serial.println(joyZ_center);
  Serial.print("T_center="); Serial.println(joyT_center);
}

// ====== MAP WITH DEADZONE ======
int16_t mapAxisWithDeadzone(int raw, int center, int deadzone = 10) {
  int delta = raw - center;

  if (abs(delta) <= deadzone) return 0;

  if(delta > 0)
    return map(delta - deadzone, 0, 4095 - center - deadzone, 0, 2048);
  else
    return map(delta + deadzone, -center + deadzone, 0, -2048, 0);
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  calibrateJoystick();

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    while(1);
  }
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, droneMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Add peer failed");
    while(1);
  }

  Serial.println("Transmitter ready ✅");
}

// ====== LOOP ======
void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {

    int rawX = analogRead(JOY_X);
    int rawY = analogRead(JOY_Y);
    int rawZ = analogRead(JOY_Z);
    int rawT = analogRead(JOY_T);

    // Map X/Y/Z with deadzone
    packet.joyX = mapAxisWithDeadzone(rawX, joyX_center);
    packet.joyY = mapAxisWithDeadzone(rawY, joyY_center);
    packet.joyZ = mapAxisWithDeadzone(rawZ, joyZ_center);

    // Map Throttle separately: center=0, min=-2048, max=+2047
    packet.throttle = mapAxisWithDeadzone(rawT, joyT_center);

    // Buttons
    packet.button1 = !digitalRead(BTN1);
    packet.button2 = !digitalRead(BTN2);

    packet.timestamp = millis();

    // Serial print
    Serial.print("X="); Serial.print(packet.joyX);
    Serial.print("\tY="); Serial.print(packet.joyY);
    Serial.print("\tZ="); Serial.print(packet.joyZ);
    Serial.print("\tT="); Serial.print(packet.throttle);
    Serial.print("\tB1="); Serial.print(packet.button1);
    Serial.print("\tB2="); Serial.println(packet.button2);

    // Send ESP-NOW
    esp_now_send(droneMac, (uint8_t*)&packet, sizeof(packet));

    lastSend = millis();
  }

  // LED off
  if (ledOffTime && millis() >= ledOffTime) {
    digitalWrite(LED_PIN, LOW);
    ledOffTime = 0;
  }
}
