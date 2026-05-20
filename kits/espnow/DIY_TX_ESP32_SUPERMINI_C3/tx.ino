#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t rxMAC[] = {0x14,0x63,0x93,0x6E,0x77,0xB4};

typedef struct {
  uint16_t ch[4];
} Data;

Data data;

int center[4];

#define CH1 0
#define CH2 1
#define CH3 3
#define CH4 4

uint16_t mapStick(int v, int c){
  if(abs(v - c) < 80) return 1500;

  if(v < c)
    return map(v, 0, c, 1000, 1500);
  else
    return map(v, c, 4095, 1500, 2000);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  esp_now_init();

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, rxMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("CALIB...");

  long s[4] = {0};

  for(int i=0;i<200;i++){
    s[0]+=analogRead(CH1);
    s[1]+=analogRead(CH2);
    s[2]+=analogRead(CH3);
    s[3]+=analogRead(CH4);
    delay(5);
  }

  for(int i=0;i<4;i++) center[i] = s[i]/200;

  Serial.println("READY");
}

void loop() {
  int r[4];

  r[0] = analogRead(CH1);
  r[1] = analogRead(CH2);
  r[2] = analogRead(CH3);
  r[3] = analogRead(CH4);

  for(int i=0;i<4;i++){
    data.ch[i] = mapStick(r[i], center[i]);
  }

  esp_now_send(rxMAC, (uint8_t*)&data, sizeof(data));

  Serial.printf("TX %d %d %d %d\n",
    data.ch[0], data.ch[1], data.ch[2], data.ch[3]);

  delay(20);
}