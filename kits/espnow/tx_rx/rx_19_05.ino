#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ================= MOTOR PINS =================
#define M1 3
#define M2 4
#define M3 5
#define M4 6

// ================= DATA =================
typedef struct {
  uint16_t ch[4];
} Data;

Data rx;

// ================= STATE =================
unsigned long lastRX = 0;
bool armed = false;

// ================= MOTOR =================
void motorInit(){

  ledcSetup(0,400,8);
  ledcSetup(1,400,8);
  ledcSetup(2,400,8);
  ledcSetup(3,400,8);

  ledcAttachPin(M1,0);
  ledcAttachPin(M2,1);
  ledcAttachPin(M3,2);
  ledcAttachPin(M4,3);
}

void motorWrite(int ch, int val){
  ledcWrite(ch, constrain(val,0,255));
}

// ================= ESP-NOW CALLBACK (FIX C3) =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len){

  memcpy(&rx, data, sizeof(rx));

  lastRX = millis();
  armed = true;

  Serial.print("RX MAC: ");
  for(int i=0;i<6;i++){
    Serial.print(mac[i], HEX);
    if(i<5) Serial.print(":");
  }

  Serial.println();

  Serial.print("CH: ");
  Serial.print(rx.ch[0]); Serial.print(" ");
  Serial.print(rx.ch[1]); Serial.print(" ");
  Serial.print(rx.ch[2]); Serial.print(" ");
  Serial.println(rx.ch[3]);
}

// ================= SETUP =================
void setup(){

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if(esp_now_init()!=ESP_OK){
    Serial.println("ESP-NOW FAIL");
    return;
  }

  esp_now_register_recv_cb(onRecv);

  motorInit();

  Serial.println("RX READY");
}

// ================= LOOP =================
void loop(){

  // ================= FAILSAFE =================
  if(millis() - lastRX > 300){
    armed = false;
  }

  int m1=0,m2=0,m3=0,m4=0;

  if(armed){

    int throttle = map(rx.ch[2],1000,2000,0,255);

    m1 = throttle;
    m2 = throttle;
    m3 = throttle;
    m4 = throttle;

    Serial.println("ARMED");
  }
  else{
    Serial.println("FAILSAFE");
  }

  motorWrite(0,m1);
  motorWrite(1,m2);
  motorWrite(2,m3);
  motorWrite(3,m4);

  Serial.print("MOTOR: ");
  Serial.print(m1); Serial.print(" ");
  Serial.print(m2); Serial.print(" ");
  Serial.print(m3); Serial.print(" ");
  Serial.println(m4);

  delay(50);
}