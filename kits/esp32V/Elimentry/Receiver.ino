// #include <WiFi.h>

// void setup() {
//   Serial.begin(115200);
//   String mac = WiFi.macAddress();
//   Serial.println("WiFi MAC Address: " + mac);
// }

// void loop() {}




#include <esp_now.h>
#include <WiFi.h>

uint8_t groundMac[] = {0x14,0x2B,0x2F,0xEC,0x17,0xB8}; // MAC Ground

const int LED_PIN = 2;
const int LED_BLINK_DURATION = 50;
unsigned long ledTurnOffTime = 0;

char telemetryBuffer[128];

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if(len > 0){
    memcpy(telemetryBuffer, incomingData, len);
    telemetryBuffer[len] = '\0';
    Serial.print("Received: "); Serial.println(telemetryBuffer);

    // Nếu nhận ping, trả pong
    if(strcmp(telemetryBuffer, "PING") == 0){
      char pong[] = "PONG";
      esp_now_send(groundMac, (uint8_t*)pong, strlen(pong));
      Serial.println("Sent PONG ✅");
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
  memcpy(peerInfo.peer_addr, groundMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Add peer failed ❌");
    return;
  }

  Serial.println("Drone ready ✅");
}

void loop() {
  if(ledTurnOffTime > 0 && millis() >= ledTurnOffTime){
    digitalWrite(LED_PIN, LOW);
    ledTurnOffTime = 0;
  }
}

