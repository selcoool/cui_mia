
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


String deviceMAC;

// ================= PIN MOTOR (DYNAMIC) =================
int pinM1=4, pinM2=5, pinM3=6, pinM4=7;

// ================= RC =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= FAILSAFE =================
unsigned long lastRX=0;
bool failSafe=true;

// ================= MOTOR =================
float m1=0,m2=0,m3=0,m4=0;

// ================= MPU =================
#define MPU_ADDR 0x68
float roll=0,pitch=0,yaw=0;
float gx_o=0,gy_o=0,gz_o=0;
float gz_bias=0;

// ================= PID =================
struct PID{
  float kp,ki,kd;
  float i,last;
};

PID pidR,pidP,pidY;

// ================= MOTOR PWM =================
void motorInit(){
  ledcSetup(0,400,8); ledcAttachPin(pinM1,0);
  ledcSetup(1,400,8); ledcAttachPin(pinM2,1);
  ledcSetup(2,400,8); ledcAttachPin(pinM3,2);
  ledcSetup(3,400,8); ledcAttachPin(pinM4,3);
}

void setMotor(int ch,float v){
  ledcWrite(ch, constrain((int)v,0,255));
}

// ================= MPU =================
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

  for(int i=0;i<800;i++){
    gx+=read16(0x43);
    gy+=read16(0x45);
    gz+=read16(0x47);
    delay(2);
  }

  gx_o=gx/800.0;
  gy_o=gy/800.0;
  gz_o=gz/800.0;

  gz_bias=gz_o/131.0;
}

// ================= PID =================
float compute(PID &p,float set,float in,float dt){
  float e=set-in;

  p.i+=e*dt;
  p.i=constrain(p.i,-25,25);

  float d=(e-p.last)/dt;
  p.last=e;

  return p.kp*e + p.ki*p.i + p.kd*d;
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac,const uint8_t *data,int len){
  memcpy(&rx,data,sizeof(rx));
  lastRX=millis();
}

// ================= LOAD PIN =================
void loadPin(){
  prefs.begin("pin",true);
  pinM1=prefs.getInt("m1",4);
  pinM2=prefs.getInt("m2",5);
  pinM3=prefs.getInt("m3",6);
  pinM4=prefs.getInt("m4",7);
  prefs.end();
}

// ================= SAVE PIN =================
void savePin(){
  prefs.begin("pin",false);
  prefs.putInt("m1",pinM1);
  prefs.putInt("m2",pinM2);
  prefs.putInt("m3",pinM3);
  prefs.putInt("m4",pinM4);
  prefs.end();
}


String getMac(){
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[18];

  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          (uint8_t)(chipid >> 40),
          (uint8_t)(chipid >> 32),
          (uint8_t)(chipid >> 24),
          (uint8_t)(chipid >> 16),
          (uint8_t)(chipid >> 8),
          (uint8_t)(chipid));

  return String(macStr);
}

// ================= HTML =================
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FC PRO</title>

<style>
body{margin:0;background:#0b0f1a;color:#fff;font-family:Arial;}
h2{text-align:center;color:#00ffcc;}

.grid{
  display:grid;
  grid-template-columns:repeat(2,1fr);
  gap:10px;
  padding:10px;
}

.card{
  background:#1c1f2a;
  padding:10px;
  border-radius:10px;
}

.bar{
  height:10px;
  background:#333;
  border-radius:5px;
  overflow:hidden;
  margin:4px 0;
}

.fill{height:100%;background:#00ffcc;}
.motor .fill{background:#ff3b3b;}

.warn{color:red;text-align:center;display:none;font-weight:bold;}
input{width:60px;}
button{margin-top:5px;}
</style>
</head>

<body>

<h2>🚁 FC BETAFLIGHT PRO</h2>
<div id="warn" class="warn">⚠ FAILSAFE</div>

<div class="grid">

<!-- ATTITUDE -->
<div class="card">
<h3>ATTITUDE</h3>
Roll <span id="r"></span><br>
Pitch <span id="p"></span><br>
Yaw <span id="y"></span>
</div>

<div class="card">
<h3>DEVICE INFO</h3>
MAC: <span id="mac"></span>
</div>

<!-- TX -->
<div class="card">
<h3>TX</h3>
CH0 <div class="bar"><div id="ch0" class="fill"></div></div>
CH1 <div class="bar"><div id="ch1" class="fill"></div></div>
CH2 <div class="bar"><div id="ch2" class="fill"></div></div>
CH3 <div class="bar"><div id="ch3" class="fill"></div></div>
</div>

<!-- MOTORS -->
<div class="card">
<h3>MOTORS</h3>
M1 <div class="bar motor"><div id="m1" class="fill"></div></div>
M2 <div class="bar motor"><div id="m2" class="fill"></div></div>
M3 <div class="bar motor"><div id="m3" class="fill"></div></div>
M4 <div class="bar motor"><div id="m4" class="fill"></div></div>
</div>

<!-- PIN CONFIG -->
<div class="card">
<h3>MOTOR PINS</h3>
M1 GPIO <input id="pm1"><br>
M2 GPIO <input id="pm2"><br>
M3 GPIO <input id="pm3"><br>
M4 GPIO <input id="pm4"><br>

<button onclick="savePin()">APPLY</button>
</div>

</div>

<script>
let ws=new WebSocket("ws://"+location.hostname+":81/");

function bar(id,val){
  document.getElementById(id).style.width=val+"%";
}

ws.onmessage=(e)=>{
  let d=JSON.parse(e.data);

  r.innerText=d.r.toFixed(2);
  p.innerText=d.p.toFixed(2);
  y.innerText=d.y.toFixed(2);
  mac.innerText = d.mac;

  warn.style.display=d.fail?"block":"none";

  bar("ch0",(d.ch0-1000)/10);
  bar("ch1",(d.ch1-1000)/10);
  bar("ch2",(d.ch2-1000)/10);
  bar("ch3",(d.ch3-1000)/10);

  bar("m1",d.m1);
  bar("m2",d.m2);
  bar("m3",d.m3);
  bar("m4",d.m4);

  if(!window.l){
    pm1.value=d.pm1;
    pm2.value=d.pm2;
    pm3.value=d.pm3;
    pm4.value=d.pm4;
    window.l=1;
  }
};

function savePin(){
  ws.send("PIN|"+pm1.value+","+pm2.value+","+pm3.value+","+pm4.value);
}
</script>

</body>
</html>
)rawliteral";

// ================= WEB =================
void handleRoot(){
  server.send_P(200,"text/html",HTML);
}

// ================= WS =================
void onWs(uint8_t num,WStype_t type,uint8_t *payload,size_t len){

  if(type==WStype_TEXT){

    String msg=(char*)payload;

    if(msg.startsWith("PIN|")){
      String d=msg.substring(4);

      int a=d.indexOf(',');
      int b=d.indexOf(',',a+1);
      int c=d.indexOf(',',b+1);

      pinM1=d.substring(0,a).toInt();
      pinM2=d.substring(a+1,b).toInt();
      pinM3=d.substring(b+1,c).toInt();
      pinM4=d.substring(c+1).toInt();

      savePin();
      motorInit();   // 🔥 rebind PWM
      return;
    }
  }
}

// ================= SETUP =================
unsigned long lastT=0;

void setup(){
  Serial.begin(115200);
  deviceMAC = getMac();

  Wire.begin();
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  loadPin();
  motorInit();
  mpuCalib();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("FC-DRONE","12345678");

  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  server.on("/",handleRoot);
  server.begin();

  

  ws.begin();
  ws.onEvent(onWs);

  lastRX=millis();
}

// ================= LOOP =================
void loop(){
  server.handleClient();
  ws.loop();

  float dt=(micros()-lastT)/1000000.0;
  lastT=micros();

  failSafe=(millis()-lastRX>500);

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

  gz_bias=gz_bias*0.999+gz*0.001;
  float gz2=gz-gz_bias;

  if(fabs(gz2)<0.03) gz2=0;

  yaw+=gz2*dt;
  roll=0.98*roll+gx*dt;
  pitch=0.98*pitch+gy*dt;

  bool arm=rx.ch[4]>1500;
  float throttle=map(rx.ch[0],1000,2000,90,180);

  float rs=(rx.ch[3]-1500)/10.0;
  float ps=(rx.ch[2]-1500)/10.0;
  float ys=(rx.ch[1]-1500)/20.0;

  if(arm){
    m1=throttle+ps+rs-ys;
    m2=throttle+ps-rs+ys;
    m3=throttle-ps-rs-ys;
    m4=throttle-ps+rs+ys;
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
  json+="\"m4\":"+String(m4)+",";

  json += "\"mac\":\"" + deviceMAC + "\",";

  json+="\"pm1\":"+String(pinM1)+",";
  json+="\"pm2\":"+String(pinM2)+",";
  json+="\"pm3\":"+String(pinM3)+",";
  json+="\"pm4\":"+String(pinM4);

  json+="}";

  ws.broadcastTXT(json);
}