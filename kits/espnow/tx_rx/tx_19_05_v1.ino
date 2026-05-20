#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ===== RX MAC =====
uint8_t rxMAC[] = {0x14,0x63,0x93,0x6E,0x77,0xB4};

typedef struct {
  uint16_t ch[4];
} Data;

Data data;

// ===== center calibration =====
int center[4] = {2000, 2000, 2000, 2000};
int deadzone = 80;

// ===== MAP JOYSTICK =====
uint16_t mapCenter(int v, int c){
  if(abs(v - c) < deadzone) return 1500;

  if(v < c){
    return map(v, 0, c, 1000, 1500);
  } else {
    return map(v, c, 4095, 1500, 2000);
  }
}

float smooth(float v, float &last){
  last = last * 0.7 + v * 0.3;
  return last;
}

float f[4];

void addPeer(){
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, rxMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  addPeer();

  // AUTO CENTER CALIB
  Serial.println("CALIB CENTER...");
  long sum[4]={0};

  for(int i=0;i<200;i++){
    sum[0]+=analogRead(0);
    sum[1]+=analogRead(1);
    sum[2]+=analogRead(3);
    sum[3]+=analogRead(4);
    delay(5);
  }

  for(int i=0;i<4;i++){
    center[i]=sum[i]/200;
  }

  Serial.println("CENTER DONE");
}

void loop(){

  int raw[4];
  raw[0]=analogRead(0);
  raw[1]=analogRead(1);
  raw[2]=analogRead(3);
  raw[3]=analogRead(4);

  for(int i=0;i<4;i++){
    data.ch[i] = smooth(mapCenter(raw[i], center[i]), f[i]);
  }

  esp_now_send(rxMAC, (uint8_t*)&data, sizeof(data));

  Serial.print("TX: ");
  Serial.print(data.ch[0]); Serial.print(" ");
  Serial.print(data.ch[1]); Serial.print(" ");
  Serial.print(data.ch[2]); Serial.print(" ");
  Serial.println(data.ch[3]);

  delay(20);
}