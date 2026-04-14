#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// ================= SERVER =================
WebServer server(80);
WebSocketsServer ws(81);
Preferences prefs;

// ================= WIFI =================
const char* ssid = "FC-DRONE";
const char* pass = "12345678";

// ================= MPU =================
#define MPU_ADDR 0x68

float roll=0,pitch=0,yaw=0;
float gx_o=0,gy_o=0,gz_o=0;

unsigned long lastMicros=0;
float dt=0;

// ================= ALT =================
Adafruit_BMP280 bmp;
float altitude=0;
float baseAlt=0;

// ================= PID =================
struct PID{float kp,ki,kd;};
PID pidR={3,0,1.2};
PID pidP={3,0,1.2};
PID pidY={2,0,0};

// ================= HTML =================
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>FC DRONE</title>

<style>
body{background:#0b0b0b;color:#00ff88;text-align:center;font-family:Arial}
.row{display:flex;justify-content:center;gap:10px;flex-wrap:wrap}
.box{border:1px solid #00ff88;padding:10px;margin:5px;width:220px;border-radius:10px}
.title{color:#888;font-size:13px}
.value{font-size:24px;font-weight:bold}
input{width:60px;text-align:center}
button{padding:10px;margin:5px;background:#111;color:#00ff88;border:1px solid #00ff88}
</style>

</head>

<body>

<h2>🚁 FC DRONE DASHBOARD</h2>

<!-- ATTITUDE -->
<div class="row">

<div class="box"><div class="title">ROLL</div><div class="value" id="r">0</div></div>
<div class="box"><div class="title">PITCH</div><div class="value" id="p">0</div></div>
<div class="box"><div class="title">YAW</div><div class="value" id="y">0</div></div>
<div class="box"><div class="title">ALT</div><div class="value" id="a">0</div></div>

</div>

<!-- PID -->
<h3>PID CONTROL</h3>

<div class="row">

<div class="box">
ROLL PID<br>
<input id="rk"><input id="ri"><input id="rd">
</div>

<div class="box">
PITCH PID<br>
<input id="pk"><input id="pi"><input id="pd">
</div>

<div class="box">
YAW PID<br>
<input id="yk"><input id="yi"><input id="yd">
</div>

</div>

<button onclick="save()">SAVE</button>
<button onclick="reset()">RESET</button>
<button onclick="calib()">CALIB</button>

<script>

let ws=new WebSocket("ws://"+location.hostname+":81/");

ws.onopen=()=>{
  console.log("WS CONNECTED");
};

// ===== RECEIVE =====
ws.onmessage=(e)=>{
 let d=JSON.parse(e.data);

 if(d.t=="r"){
  r.innerText=d.r.toFixed(2);
  p.innerText=d.p.toFixed(2);
  y.innerText=d.y.toFixed(2);
  a.innerText=d.alt.toFixed(2);
 }

 if(d.t=="c"){
  rk.value=d.rk;
  ri.value=d.ri;
  rd.value=d.rd;

  pk.value=d.pk;
  pi.value=d.pi;
  pd.value=d.pd;

  yk.value=d.yk;
  yi.value=d.yi;
  yd.value=d.yd;
 }
};

// ===== SEND =====
function save(){
 ws.send("SAVE|"+rk.value+","+ri.value+","+rd.value+"|"
 +pk.value+","+pi.value+","+pd.value+"|"
 +yk.value+","+yi.value+","+yd.value);
}

function reset(){
 ws.send("RESET");
}

function calib(){
 ws.send("CALIB");
}

</script>

</body>
</html>
)rawliteral";

// ================= MPU =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR,2);
  return (Wire.read()<<8)|Wire.read();
}

// ================= INIT =================
void setupMPU(){
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();
}

// ================= CALIB =================
void calibrateGyro(){
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

void calibrateAlt(){
  float s=0;
  for(int i=0;i<100;i++){
    s+=bmp.readAltitude(1013.25);
    delay(10);
  }
  baseAlt=s/100.0;
}

// ================= SEND =================
void sendRealtime(){
  String j="{\"t\":\"r\",";
  j+="\"r\":"+String(roll)+",";
  j+="\"p\":"+String(pitch)+",";
  j+="\"y\":"+String(yaw)+",";
  j+="\"alt\":"+String(altitude)+"}";
  ws.broadcastTXT(j);
}

void sendConfig(){
  String j="{\"t\":\"c\",";
  j+="\"rk\":"+String(pidR.kp)+",\"ri\":"+String(pidR.ki)+",\"rd\":"+String(pidR.kd)+",";
  j+="\"pk\":"+String(pidP.kp)+",\"pi\":"+String(pidP.ki)+",\"pd\":"+String(pidP.kd)+",";
  j+="\"yk\":"+String(pidY.kp)+",\"yi\":"+String(pidY.ki)+",\"yd\":"+String(pidY.kd)+"}";
  ws.broadcastTXT(j);
}

// ================= RESET =================
void factoryReset(){
  roll=pitch=yaw=0;

  pidR={3,0,1.2};
  pidP={3,0,1.2};
  pidY={2,0,0};

  calibrateAlt();

  sendConfig();   // 🔥 FIX UI UPDATE
}

// ================= WS =================
void onWs(uint8_t num,WStype_t type,uint8_t *payload,size_t len){

  if(type==WStype_CONNECTED){
    sendConfig();      // 🔥 FIX: gửi PID ngay khi connect
    sendRealtime();
    return;
  }

  if(type!=WStype_TEXT) return;

  String msg="";
  for(int i=0;i<len;i++) msg+=(char)payload[i];

  if(msg=="RESET") factoryReset();

  if(msg=="CALIB"){
    calibrateGyro();
    calibrateAlt();
  }

  if(msg.startsWith("SAVE|")){
    String d=msg.substring(5);

    int a=d.indexOf('|');
    int b=d.indexOf('|',a+1);

    String r=d.substring(0,a);
    String p=d.substring(a+1,b);
    String y=d.substring(b+1);

    pidR.kp=r.substring(0,r.indexOf(',')).toFloat();
    pidR.ki=r.substring(r.indexOf(',')+1,r.lastIndexOf(',')).toFloat();
    pidR.kd=r.substring(r.lastIndexOf(',')+1).toFloat();

    pidP.kp=p.substring(0,p.indexOf(',')).toFloat();
    pidP.ki=p.substring(p.indexOf(',')+1,p.lastIndexOf(',')).toFloat();
    pidP.kd=p.substring(p.lastIndexOf(',')+1).toFloat();

    pidY.kp=y.substring(0,y.indexOf(',')).toFloat();
    pidY.ki=y.substring(y.indexOf(',')+1,y.lastIndexOf(',')).toFloat();
    pidY.kd=y.substring(y.lastIndexOf(',')+1).toFloat();

    sendConfig(); // 🔥 FIX UPDATE UI
  }
}

// ================= UPDATE =================
void updateSensors(){

  unsigned long now=micros();
  dt=(now-lastMicros)/1000000.0;
  lastMicros=now;

  if(dt<=0||dt>0.1) dt=0.01;

  float gx=(read16(0x43)-gx_o)/131.0;
  float gy=(read16(0x45)-gy_o)/131.0;
  float gz=(read16(0x47)-gz_o)/131.0;

  float ax=read16(0x3B)/16384.0;
  float ay=read16(0x3D)/16384.0;
  float az=read16(0x3F)/16384.0;

  float ar=atan2(ay,az)*57.3;
  float ap=atan2(-ax,sqrt(ay*ay+az*az))*57.3;

  roll=0.98*(roll+gx*dt)+0.02*ar;
  pitch=0.98*(pitch+gy*dt)+0.02*ap;
  yaw+=gz*dt;

  float raw=bmp.readAltitude(1013.25);
  altitude=0.9*altitude+0.1*(raw-baseAlt);
}

// ================= SETUP =================
void setup(){

  Serial.begin(115200);

  setupMPU();

  if(!bmp.begin(0x76)){
    Serial.println("BMP FAIL");
  }

  calibrateGyro();
  calibrateAlt();

  WiFi.softAP(ssid,pass);

  server.on("/",[](){
    server.send_P(200,"text/html",html);
  });

  server.begin();

  ws.begin();
  ws.onEvent(onWs);

  lastMicros=micros();
}

// ================= LOOP =================
void loop(){

  server.handleClient();
  ws.loop();

  updateSensors();

  static unsigned long t=0;
  if(millis()-t>20){
    sendRealtime();
    t=millis();
  }

  delay(5);
}
