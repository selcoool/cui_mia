#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ================= RX MAC =================
uint8_t rxMAC[6] = {0x14,0x63,0x93,0x6E,0x77,0xB4};

// ================= DATA =================
typedef struct {
  uint16_t ch[4];
} Data;

Data data;

// ================= JOYSTICK =================
#define J1X 0
#define J1Y 1
#define J2X 3
#define J2Y 4

float f1=1500,f2=1500,f3=1500,f4=1500;

// ================= MAP =================
int smooth(int v, float &last){
  v = map(v,0,4095,1000,2000);
  last = last*0.6 + v*0.4;
  return last;
}

// ================= ESP-NOW =================
void addPeer(){
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, rxMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

  esp_now_add_peer(&peer);
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  analogReadResolution(12);
  WiFi.mode(WIFI_STA);

  if(esp_now_init()!=ESP_OK){
    Serial.println("ESP-NOW FAIL");
    return;
  }

  addPeer();

  Serial.println("TX READY");
}

// ================= LOOP =================
void loop(){

  data.ch[0] = smooth(analogRead(J1X), f1);
  data.ch[1] = smooth(analogRead(J1Y), f2);
  data.ch[2] = smooth(analogRead(J2X), f3);
  data.ch[3] = smooth(analogRead(J2Y), f4);

  esp_now_send(rxMAC, (uint8_t*)&data, sizeof(data));

  Serial.print("TX -> ");
  Serial.print(data.ch[0]); Serial.print(" ");
  Serial.print(data.ch[1]); Serial.print(" ");
  Serial.print(data.ch[2]); Serial.print(" ");
  Serial.println(data.ch[3]);

  delay(20);
}