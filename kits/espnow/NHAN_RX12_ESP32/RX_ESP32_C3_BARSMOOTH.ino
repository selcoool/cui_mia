#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsServer.h>
#include <Wire.h>

// ================= WEB =================
WebServer server(80);
WebSocketsServer ws(81);
Preferences prefs;

// ================= DEVICE =================
String deviceMAC;

// ================= MOTOR =================
int pinM1=4, pinM2=5, pinM3=6, pinM4=7;

// ================= RC =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= FAILSAFE =================
unsigned long lastRX=0;
bool failSafe=true;

// ================= MOTOR OUT =================
float m1=0,m2=0,m3=0,m4=0;

// ================= MPU =================
#define MPU_ADDR 0x68
float roll=0,pitch=0,yaw=0;
float gx_o=0,gy_o=0,gz_o=0;
float gz_bias=0;

// ================= PWM =================
void motorInit(){
  ledcSetup(0,400,8); ledcAttachPin(pinM1,0);
  ledcSetup(1,400,8); ledcAttachPin(pinM2,1);
  ledcSetup(2,400,8); ledcAttachPin(pinM3,2);
  ledcSetup(3,400,8); ledcAttachPin(pinM4,3);
}

inline void setMotor(int ch,float v){
  v = constrain(v,0,255);
  ledcWrite(ch,(int)v);
}

// ================= MPU READ =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR,2);
  return (Wire.read()<<8)|Wire.read();
}

// ================= CALIB =================
void mpuCalib(){
  long gx=0,gy=0,gz=0;

  for(int i=0;i<500;i++){
    gx+=read16(0x43);
    gy+=read16(0x45);
    gz+=read16(0x47);
    delay(2);
  }

  gx_o=gx/500.0;
  gy_o=gy/500.0;
  gz_o=gz/500.0;
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac,const uint8_t *data,int len){
  memcpy(&rx,data,sizeof(rx));
  lastRX = millis();
}

// ================= HTML =================
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FC PRO FIXED</title>

<style>
body{margin:0;background:#0b0f1a;color:#fff;font-family:Arial;}
h2{text-align:center;color:#00ffcc;}

.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;padding:10px;}

.card{background:#1c1f2a;padding:10px;border-radius:10px;}

.bar{height:10px;background:#333;border-radius:5px;overflow:hidden;margin:4px 0;}
.fill{height:100%;background:#00ffcc;transition:width .15s;}
.motor .fill{background:#ff3b3b;}

.warn{color:red;text-align:center;display:none;font-weight:bold;}
</style>
</head>

<body>

<h2>🚁 FC DRONE FIXED</h2>
<div id="warn" class="warn">⚠ FAILSAFE</div>

<div class="grid">

<div class="card">
<h3>ATTITUDE</h3>
Roll <span id="r"></span><br>
Pitch <span id="p"></span><br>
Yaw <span id="y"></span>
</div>

<div class="card">
<h3>TX</h3>
CH0 <div class="bar"><div id="ch0" class="fill"></div></div>
CH1 <div class="bar"><div id="ch1" class="fill"></div></div>
CH2 <div class="bar"><div id="ch2" class="fill"></div></div>
CH3 <div class="bar"><div id="ch3" class="fill"></div></div>
</div>

<div class="card">
<h3>MOTORS</h3>
M1 <div class="bar motor"><div id="m1" class="fill"></div></div>
M2 <div class="bar motor"><div id="m2" class="fill"></div></div>
M3 <div class="bar motor"><div id="m3" class="fill"></div></div>
M4 <div class="bar motor"><div id="m4" class="fill"></div></div>
</div>

</div>

<script>

let ws=new WebSocket("ws://"+location.hostname+":81/");

function clamp(v,min,max){
  return Math.max(min,Math.min(max,v));
}

// smooth bar (FIX JITTER)
let sm=[0,0,0,0,0,0,0,0];

function smooth(i,v){
  sm[i]=sm[i]*0.7+v*0.3;
  return sm[i];
}

// FIX SCALE 1000–2000 → 0–100%
function bar(id,val){
  val = clamp(val,0,100);
  document.getElementById(id).style.width = val + "%";
}

ws.onmessage=(e)=>{
  let d=JSON.parse(e.data);

  r.innerText=d.r.toFixed(2);
  p.innerText=d.p.toFixed(2);
  y.innerText=d.y.toFixed(2);

  warn.style.display = d.fail ? "block":"none";

  // ===== FIX BAR SCALE =====
  bar("ch0", smooth(0,(d.ch0-1000)/10));
  bar("ch1", smooth(1,(d.ch1-1000)/10));
  bar("ch2", smooth(2,(d.ch2-1000)/10));
  bar("ch3", smooth(3,(d.ch3-1000)/10));

  // motors
  bar("m1", d.m1*100/255);
  bar("m2", d.m2*100/255);
  bar("m3", d.m3*100/255);
  bar("m4", d.m4*100/255);
};

</script>

</body>
</html>
)rawliteral";

// ================= WEB =================
void handleRoot(){
  server.send_P(200,"text/html",HTML);
}

// ================= SETUP =================
unsigned long lastT=0;

void setup(){
  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  motorInit();
  mpuCalib();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("FC-DRONE","12345678");

  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  server.on("/",handleRoot);
  server.begin();

  ws.begin();

  lastRX = millis();
}

// ================= LOOP =================
void loop(){
  server.handleClient();
  ws.loop();

  unsigned long nowMicros = micros();
  float dt = (nowMicros - lastT) / 1000000.0;
  lastT = nowMicros;

  if(dt <= 0 || dt > 0.1) dt = 0.01;

  failSafe = (millis() - lastRX > 500);

  if(failSafe){
    setMotor(0,0);
    setMotor(1,0);
    setMotor(2,0);
    setMotor(3,0);
    return;
  }

  float gx=(read16(0x43)-gx_o)/131.0;
  float gy=(read16(0x45)-gy_o)/131.0;
  float gz=(read16(0x47)-gz_o)/131.0;

  gz_bias = gz_bias*0.995 + gz*0.005;
  float gz2 = gz - gz_bias;

  yaw += gz2 * dt;
  roll += gx * dt;
  pitch += gy * dt;

  bool arm = rx.ch[4] > 1500;

  float throttle = (rx.ch[0]-1000)*255.0/1000.0;

  float rs=(rx.ch[3]-1500)/10.0;
  float ps=(rx.ch[2]-1500)/10.0;
  float ys=(rx.ch[1]-1500)/20.0;

  if(arm){
    m1=throttle+ps+rs-ys;
    m2=throttle+ps-rs+ys;
    m3=throttle-ps-rs-ys;
    m4=throttle-ps+rs+ys;
  }else{
    m1=m2=m3=m4=0;
  }

  setMotor(0,m1);
  setMotor(1,m2);
  setMotor(2,m3);
  setMotor(3,m4);

  String json="{";
  json+="\"r\":"+String(roll)+",";
  json+="\"p\":"+String(pitch)+",";
  json+="\"y\":"+String(yaw)+",";
  json+="\"fail\":"+String(failSafe)+",";
  json+="\"ch0\":"+String(rx.ch[0])+",";
  json+="\"ch1\":"+String(rx.ch[1])+",";
  json+="\"ch2\":"+String(rx.ch[2])+",";
  json+="\"ch3\":"+String(rx.ch[3])+",";
  json+="\"m1\":"+String(m1)+",";
  json+="\"m2\":"+String(m2)+",";
  json+="\"m3\":"+String(m3)+",";
  json+="\"m4\":"+String(m4);
  json+="}";

  ws.broadcastTXT(json);
}