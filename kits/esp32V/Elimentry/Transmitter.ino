#include <esp_now.h>
#include <WiFi.h>

uint8_t droneMac[] = {0x1C, 0x69, 0x20, 0x96, 0x3A, 0xF4}; // MAC Drone

const int LED_PIN = 2;
const int LED_BLINK_DURATION = 50;
unsigned long ledTurnOffTime = 0;

char buffer[128];
char telemetryBuffer[128];

unsigned long lastPingTime = 0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS){
    Serial.println("C:Data sent ✅");
    digitalWrite(LED_PIN, HIGH);
    ledTurnOffTime = millis() + LED_BLINK_DURATION;
  } else {
    Serial.println("C:Data send ❌");
  }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if(len > 0){
    memcpy(telemetryBuffer, incomingData, len);
    telemetryBuffer[len] = '\0';
    Serial.print("T:"); Serial.println(telemetryBuffer);

    if(strcmp(telemetryBuffer, "PONG") == 0){
      Serial.println("Drone online ✅");
      digitalWrite(LED_PIN, HIGH);
      ledTurnOffTime = millis() + LED_BLINK_DURATION;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
    Serial.println("ESP-NOW init failed ❌");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, droneMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Add peer failed ❌");
    return;
  }

  Serial.println("Ground Station ready ✅");
}

void loop() {
  // Gửi lệnh từ Serial
  if(Serial.available()){
    int len = Serial.readBytes(buffer, sizeof(buffer)-1);
    buffer[len] = '\0';
    esp_now_send(droneMac, (uint8_t*)buffer, len);
  }

  // Gửi ping mỗi 1s để kiểm tra kết nối
  if(millis() - lastPingTime > 1000){
    char ping[] = "PING";
    esp_now_send(droneMac, (uint8_t*)ping, strlen(ping));
    lastPingTime = millis();
  }

  if(ledTurnOffTime > 0 && millis() >= ledTurnOffTime){
    digitalWrite(LED_PIN, LOW);
    ledTurnOffTime = 0;
  }
}
