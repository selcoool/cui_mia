// ============================================================
// ESP32-C3 DRONE RX FULL FIX
// FULL FEATURES + FULL UI + REALTIME VALUES
// ONLY LIB:
// Arduino.h
// WiFi.h
// esp_now.h
// WebServer.h
// WebSocketsServer.h
// Preferences.h
// Wire.h
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <Wire.h>

// ============================================================
// WIFI
// ============================================================

WebServer server(80);
WebSocketsServer ws(81);
Preferences prefs;

// ============================================================
// MPU6050
// ============================================================

#define MPU_ADDR 0x68

int SDA_PIN = 8;
int SCL_PIN = 9;

// ============================================================
// MOTOR
// ============================================================

#define M1_PIN 3
#define M2_PIN 4
#define M3_PIN 5
#define M4_PIN 6

int PWM_MIN = 80;
int PWM_MAX = 255;

// ============================================================
// MOTOR POSITION
// ============================================================

int POSITION_M1 = 0;
int POSITION_M2 = 1;
int POSITION_M3 = 2;
int POSITION_M4 = 3;

// ============================================================
// MOTOR TRIM
// ============================================================

int TrimM1 = 0;
int TrimM2 = 0;
int TrimM3 = 0;
int TrimM4 = 0;

// ============================================================
// SENSOR
// ============================================================

float roll = 0;
float pitch = 0;
float yaw = 0;

float gxOffset = 0;
float gyOffset = 0;
float gzOffset = 0;

float rollOffset = 0;
float pitchOffset = 0;

// ============================================================
// RX DATA
// ============================================================

typedef struct {
  uint16_t ch[4];
} RXData;

RXData rx;

// ============================================================
// MAC
// ============================================================

String rxMAC = "";
String txMAC = "";

// ============================================================
// FAILSAFE
// ============================================================

unsigned long lastRX = 0;
bool rxFail = true;
bool prevFail = true;

// ============================================================
// PID
// ============================================================

struct PID{

  float kp;
  float ki;
  float kd;

  float integral;
  float lastError;
};

PID pidRoll;
PID pidPitch;
PID pidYaw;

// ============================================================
// TIMER
// ============================================================

unsigned long lastMicros = 0;

// ============================================================
// HTML
// ============================================================

const char HTML[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">

<title>ESP32 DRONE RX</title>

<style>

body{
  margin:0;
  background:#0f172a;
  color:white;
  font-family:Arial;
  padding:10px;
}

.card{
  background:#1e293b;
  border-radius:14px;
  padding:12px;
  margin-bottom:10px;
}

.title{
  font-size:18px;
  font-weight:bold;
  margin-bottom:10px;
}

.bar{
  width:100%;
  height:14px;
  background:#334155;
  border-radius:8px;
  overflow:hidden;
  margin-bottom:8px;
}

.fill{
  height:100%;
  width:0%;
  background:#00e5ff;
}

.motor{
  background:#ff5252;
}

.val{
  color:#00e5ff;
  font-weight:bold;
}

input{
  width:100%;
  padding:8px;
  border:none;
  border-radius:8px;
  background:#111827;
  color:white;
  margin-top:4px;
  margin-bottom:8px;
  box-sizing:border-box;
}

button{
  width:100%;
  border:none;
  border-radius:10px;
  padding:10px;
  background:#00c853;
  color:white;
  font-weight:bold;
  margin-top:8px;
}

#status{
  text-align:center;
  font-size:24px;
  font-weight:bold;
  margin-bottom:10px;
}

.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
}

</style>

</head>

<body>

<div id="status">
FAILSAFE
</div>

<div class="card">

<div class="title">
MAC
</div>

<div>
RX:
<span class="val" id="rxmac">
-
</span>
</div>

<div>
TX:
<span class="val" id="txmac">
-
</span>
</div>

</div>

<div class="card">

<div class="title">
SENSOR
</div>

<div>
ROLL:
<span class="val" id="roll">
0
</span>
</div>

<div>
PITCH:
<span class="val" id="pitch">
0
</span>
</div>

<div>
YAW:
<span class="val" id="yaw">
0
</span>
</div>

</div>

<div class="card">

<div class="title">
CHANNEL
</div>

ROLL
<div class="bar">
<div class="fill" id="c1"></div>
</div>

PITCH
<div class="bar">
<div class="fill" id="c2"></div>
</div>

THROTTLE
<div class="bar">
<div class="fill" id="c3"></div>
</div>

YAW
<div class="bar">
<div class="fill" id="c4"></div>
</div>

</div>

<div class="card">

<div class="title">
MOTORS
</div>

M1
<div class="bar">
<div class="fill motor" id="m1"></div>
</div>

M2
<div class="bar">
<div class="fill motor" id="m2"></div>
</div>

M3
<div class="bar">
<div class="fill motor" id="m3"></div>
</div>

M4
<div class="bar">
<div class="fill motor" id="m4"></div>
</div>

</div>

<div class="card">

<div class="title">
PWM MIN/MAX
</div>

PWM MIN
<input id="pmin">

PWM MAX
<input id="pmax">

<button onclick="savePWM()">
SAVE PWM
</button>

</div>

<div class="card">

<div class="title">
PID
</div>

ROLL KP
<input id="rkp">

ROLL KI
<input id="rki">

ROLL KD
<input id="rkd">

PITCH KP
<input id="pkp">

PITCH KI
<input id="pki">

PITCH KD
<input id="pkd">

YAW KP
<input id="ykp">

YAW KI
<input id="yki">

YAW KD
<input id="ykd">

<button onclick="savePID()">
SAVE PID
</button>

</div>

<div class="card">

<div class="title">
CALIBRATE
</div>

<button onclick="calib()">
CALIB SENSOR
</button>

</div>

<script>

function el(id){
  return document.getElementById(id);
}

function setBar(id,v){

  v=Math.max(0,Math.min(100,v));

  el(id).style.width=v+"%";
}

let ws;

function connectWS(){

  ws=new WebSocket(
    "ws://"+location.hostname+":81/"
  );

  ws.onopen=function(){

    console.log("WS CONNECT");
  };

  ws.onclose=function(){

    console.log("WS CLOSE");

    setTimeout(connectWS,1000);
  };

  ws.onerror=function(e){

    console.log(e);
  };

  ws.onmessage=function(e){

    let d=JSON.parse(e.data);

    // =========================
    // STATUS
    // =========================

    if(d.fail){

      el("status").innerText="FAILSAFE";
      el("status").style.color="red";

    }else{

      el("status").innerText="CONNECTED";
      el("status").style.color="lime";
    }

    // =========================
    // MAC
    // =========================

    el("rxmac").innerText=d.rxmac || "-";
    el("txmac").innerText=d.txmac || "-";

    // =========================
    // SENSOR
    // =========================

    el("roll").innerText=d.roll;
    el("pitch").innerText=d.pitch;
    el("yaw").innerText=d.yaw;

    // =========================
    // CHANNEL
    // =========================

    setBar("c1",(d.ch1-1000)/10);
    setBar("c2",(d.ch2-1000)/10);
    setBar("c3",(d.ch3-1000)/10);
    setBar("c4",(d.ch4-1000)/10);

    // =========================
    // MOTOR
    // =========================

    setBar("m1",d.m1/2.55);
    setBar("m2",d.m2/2.55);
    setBar("m3",d.m3/2.55);
    setBar("m4",d.m4/2.55);

    // =========================
    // INIT
    // =========================

    if(!window.init){

      el("pmin").value=d.pwmMin;
      el("pmax").value=d.pwmMax;

      el("rkp").value=d.rkp;
      el("rki").value=d.rki;
      el("rkd").value=d.rkd;

      el("pkp").value=d.pkp;
      el("pki").value=d.pki;
      el("pkd").value=d.pkd;

      el("ykp").value=d.ykp;
      el("yki").value=d.yki;
      el("ykd").value=d.ykd;

      window.init=true;
    }
  };
}

function savePWM(){

  ws.send(
    "PWM|"+
    el("pmin").value+"|"+
    el("pmax").value
  );
}

function savePID(){

  ws.send(
    "PID|"+

    el("rkp").value+","+
    el("rki").value+","+
    el("rkd").value+"|"+

    el("pkp").value+","+
    el("pki").value+","+
    el("pkd").value+"|"+

    el("ykp").value+","+
    el("yki").value+","+
    el("ykd").value
  );
}

function calib(){

  ws.send("CALIB");
}

connectWS();

</script>

</body>
</html>

)rawliteral";

// ============================================================
// READ MPU
// ============================================================

int16_t read16(uint8_t reg){

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR,2);

  return (Wire.read()<<8) | Wire.read();
}

// ============================================================
// MOTOR INIT
// ============================================================

void motorInit(){

  ledcSetup(0,400,8);
  ledcAttachPin(M1_PIN,0);

  ledcSetup(1,400,8);
  ledcAttachPin(M2_PIN,1);

  ledcSetup(2,400,8);
  ledcAttachPin(M3_PIN,2);

  ledcSetup(3,400,8);
  ledcAttachPin(M4_PIN,3);
}

// ============================================================
// MOTOR
// ============================================================

void setMotor(int ch,int pwm){

  pwm=constrain(pwm,0,255);

  ledcWrite(ch,pwm);
}

// ============================================================
// PID
// ============================================================

float computePID(
  PID &pid,
  float setpoint,
  float input,
  float dt
){

  float error=setpoint-input;

  pid.integral += error*dt;

  pid.integral=
    constrain(
      pid.integral,
      -50,
      50
    );

  float derivative=
    (error-pid.lastError)/dt;

  pid.lastError=error;

  return
    pid.kp*error +
    pid.ki*pid.integral +
    pid.kd*derivative;
}

// ============================================================
// CALIBRATE
// ============================================================

void calibrateSensor(){

  Serial.println("CALIB...");

  long gx=0;
  long gy=0;
  long gz=0;

  long ax=0;
  long ay=0;
  long az=0;

  for(int i=0;i<1000;i++){

    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);

    delay(2);
  }

  gxOffset=gx/1000.0;
  gyOffset=gy/1000.0;
  gzOffset=gz/1000.0;

  float axf=ax/1000.0/16384.0;
  float ayf=ay/1000.0/16384.0;
  float azf=az/1000.0/16384.0;

  rollOffset=
    atan2(ayf,azf)*57.2958;

  pitchOffset=
    atan2(
      -axf,
      sqrt(ayf*ayf+azf*azf)
    )*57.2958;

  roll=0;
  pitch=0;
  yaw=0;

  Serial.println("CALIB DONE");
}

// ============================================================
// ESP NOW RX
// ============================================================

void onRecv(
  const uint8_t *mac,
  const uint8_t *incomingData,
  int len
){

  memcpy(&rx,incomingData,sizeof(rx));

  lastRX=millis();

  txMAC="";

  for(int i=0;i<6;i++){

    char buf[4];

    sprintf(buf,"%02X",mac[i]);

    txMAC += buf;

    if(i<5){
      txMAC += ":";
    }
  }

  Serial.print("ROLL:");
  Serial.print(rx.ch[0]);

  Serial.print(" PITCH:");
  Serial.print(rx.ch[1]);

  Serial.print(" THROTTLE:");
  Serial.print(rx.ch[2]);

  Serial.print(" YAW:");
  Serial.println(rx.ch[3]);
}

// ============================================================
// WS EVENT
// ============================================================

void onWs(
  uint8_t num,
  WStype_t type,
  uint8_t *payload,
  size_t len
){

  if(type!=WStype_TEXT){
    return;
  }

  String msg=(char*)payload;

  // ==========================================================
  // CALIB
  // ==========================================================

  if(msg=="CALIB"){

    calibrateSensor();

    return;
  }

  // ==========================================================
  // PWM
  // ==========================================================

  if(msg.startsWith("PWM|")){

    int p1=msg.indexOf('|');
    int p2=msg.indexOf('|',p1+1);

    PWM_MIN=
      msg.substring(p1+1,p2).toInt();

    PWM_MAX=
      msg.substring(p2+1).toInt();

    prefs.begin("cfg",false);

    prefs.putInt("pmin",PWM_MIN);
    prefs.putInt("pmax",PWM_MAX);

    prefs.end();

    return;
  }

  // ==========================================================
  // PID
  // ==========================================================

  if(msg.startsWith("PID|")){

    String d=msg.substring(4);

    int a=d.indexOf('|');
    int b=d.indexOf('|',a+1);

    String r=d.substring(0,a);
    String p=d.substring(a+1,b);
    String y=d.substring(b+1);

    pidRoll.kp=
      r.substring(0,r.indexOf(',')).toFloat();

    pidRoll.ki=
      r.substring(
        r.indexOf(',')+1,
        r.lastIndexOf(',')
      ).toFloat();

    pidRoll.kd=
      r.substring(
        r.lastIndexOf(',')+1
      ).toFloat();

    pidPitch.kp=
      p.substring(0,p.indexOf(',')).toFloat();

    pidPitch.ki=
      p.substring(
        p.indexOf(',')+1,
        p.lastIndexOf(',')
      ).toFloat();

    pidPitch.kd=
      p.substring(
        p.lastIndexOf(',')+1
      ).toFloat();

    pidYaw.kp=
      y.substring(0,y.indexOf(',')).toFloat();

    pidYaw.ki=
      y.substring(
        y.indexOf(',')+1,
        y.lastIndexOf(',')
      ).toFloat();

    pidYaw.kd=
      y.substring(
        y.lastIndexOf(',')+1
      ).toFloat();

    prefs.begin("cfg",false);

    prefs.putFloat("rkp",pidRoll.kp);
    prefs.putFloat("rki",pidRoll.ki);
    prefs.putFloat("rkd",pidRoll.kd);

    prefs.putFloat("pkp",pidPitch.kp);
    prefs.putFloat("pki",pidPitch.ki);
    prefs.putFloat("pkd",pidPitch.kd);

    prefs.putFloat("ykp",pidYaw.kp);
    prefs.putFloat("yki",pidYaw.ki);
    prefs.putFloat("ykd",pidYaw.kd);

    prefs.end();
  }
}

// ============================================================
// SETUP
// ============================================================

void setup(){

  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(
    "ESP32-DRONE",
    "12345678"
  );

  rxMAC=WiFi.macAddress();

  Wire.setPins(
    SDA_PIN,
    SCL_PIN
  );

  Wire.begin();

  // MPU WAKEUP

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  // ==========================================================
  // LOAD PREFS
  // ==========================================================

  prefs.begin("cfg",true);

  PWM_MIN=
    prefs.getInt("pmin",80);

  PWM_MAX=
    prefs.getInt("pmax",255);

  pidRoll.kp=
    prefs.getFloat("rkp",3.0);

  pidRoll.ki=
    prefs.getFloat("rki",0.0);

  pidRoll.kd=
    prefs.getFloat("rkd",1.0);

  pidPitch.kp=
    prefs.getFloat("pkp",3.0);

  pidPitch.ki=
    prefs.getFloat("pki",0.0);

  pidPitch.kd=
    prefs.getFloat("pkd",1.0);

  pidYaw.kp=
    prefs.getFloat("ykp",2.0);

  pidYaw.ki=
    prefs.getFloat("yki",0.0);

  pidYaw.kd=
    prefs.getFloat("ykd",0.0);

  prefs.end();

  // ==========================================================
  // MOTOR
  // ==========================================================

  motorInit();

  // ==========================================================
  // CALIB
  // ==========================================================

  calibrateSensor();

  // ==========================================================
  // ESP NOW
  // ==========================================================

  if(esp_now_init()!=ESP_OK){

    Serial.println("ESP NOW ERROR");

    return;
  }

  esp_now_register_recv_cb(onRecv);

  // ==========================================================
  // WEB
  // ==========================================================

  server.on("/",[](){

    server.send_P(
      200,
      "text/html",
      HTML
    );
  });

  server.begin();

  ws.begin();

  ws.onEvent(onWs);

  Serial.println("READY");
}

// ============================================================
// LOOP
// ============================================================

void loop(){

  server.handleClient();

  ws.loop();

  // ==========================================================
  // DT
  // ==========================================================

  float dt=
    (micros()-lastMicros)*1e-6;

  lastMicros=micros();

  if(dt<=0 || dt>0.05){
    dt=0.01;
  }

  // ==========================================================
  // FAILSAFE
  // ==========================================================

  rxFail=
    millis()-lastRX > 100;

  // ==========================================================
  // AUTO CALIB
  // ==========================================================

  if(prevFail && !rxFail){

    Serial.println("TX CONNECT");

    calibrateSensor();
  }

  prevFail=rxFail;

  // ==========================================================
  // SENSOR
  // ==========================================================

  float gyroX=
    (read16(0x43)-gxOffset)/131.0;

  float gyroY=
    (read16(0x45)-gyOffset)/131.0;

  float gyroZ=
    (read16(0x47)-gzOffset)/131.0;

  float accX=
    read16(0x3B)/16384.0;

  float accY=
    read16(0x3D)/16384.0;

  float accZ=
    read16(0x3F)/16384.0;

  float accRoll=
    atan2(accY,accZ)
    *57.2958
    - rollOffset;

  float accPitch=
    atan2(
      -accX,
      sqrt(accY*accY+accZ*accZ)
    )*57.2958
    - pitchOffset;

  roll=
    0.98*(roll+gyroX*dt)
    +0.02*accRoll;

  pitch=
    0.98*(pitch+gyroY*dt)
    +0.02*accPitch;

  yaw += gyroZ*dt;

  // ==========================================================
  // CHANNEL
  // ==========================================================

  float rollSet=
    (rx.ch[0]-1500)/100.0;

  float pitchSet=
    (rx.ch[1]-1500)/100.0;

  float throttle=
    map(
      rx.ch[2],
      1000,
      2000,
      PWM_MIN,
      PWM_MAX
    );

  float yawSet=
    (rx.ch[3]-1500)/100.0;

  // ==========================================================
  // PID
  // ==========================================================

  float rOut=
    computePID(
      pidRoll,
      rollSet,
      roll,
      dt
    );

  float pOut=
    computePID(
      pidPitch,
      pitchSet,
      pitch,
      dt
    );

  float yOut=
    computePID(
      pidYaw,
      yawSet,
      yaw,
      dt
    );

  // ==========================================================
  // MIXER
  // ==========================================================

  int m1=
    throttle
    -pOut
    +rOut
    -yOut
    +TrimM1;

  int m2=
    throttle
    -pOut
    -rOut
    +yOut
    +TrimM2;

  int m3=
    throttle
    +pOut
    -rOut
    -yOut
    +TrimM3;

  int m4=
    throttle
    +pOut
    +rOut
    +yOut
    +TrimM4;

  m1=constrain(m1,0,255);
  m2=constrain(m2,0,255);
  m3=constrain(m3,0,255);
  m4=constrain(m4,0,255);

  // ==========================================================
  // FAILSAFE STOP
  // ==========================================================

  if(rxFail){

    m1=0;
    m2=0;
    m3=0;
    m4=0;
  }

  // ==========================================================
  // MOTOR
  // ==========================================================

  setMotor(POSITION_M1,m1);
  setMotor(POSITION_M2,m2);
  setMotor(POSITION_M3,m3);
  setMotor(POSITION_M4,m4);

  // ==========================================================
  // SERIAL
  // ==========================================================

  Serial.print("ROLL:");
  Serial.print(roll);

  Serial.print(" PITCH:");
  Serial.print(pitch);

  Serial.print(" YAW:");
  Serial.print(yaw);

  Serial.print(" TX:");
  Serial.print(txMAC);

  Serial.print(" RX:");
  Serial.println(rxMAC);

  // ==========================================================
  // JSON
  // ==========================================================

  String j="{";

  j+="\"fail\":"+String(rxFail)+",";

  j+="\"roll\":"+String(roll,1)+",";
  j+="\"pitch\":"+String(pitch,1)+",";
  j+="\"yaw\":"+String(yaw,1)+",";

  j+="\"rxmac\":\""+rxMAC+"\",";
  j+="\"txmac\":\""+txMAC+"\",";

  j+="\"ch1\":"+String(rx.ch[0])+",";
  j+="\"ch2\":"+String(rx.ch[1])+",";
  j+="\"ch3\":"+String(rx.ch[2])+",";
  j+="\"ch4\":"+String(rx.ch[3])+",";

  j+="\"m1\":"+String(m1)+",";
  j+="\"m2\":"+String(m2)+",";
  j+="\"m3\":"+String(m3)+",";
  j+="\"m4\":"+String(m4)+",";

  j+="\"pwmMin\":"+String(PWM_MIN)+",";
  j+="\"pwmMax\":"+String(PWM_MAX)+",";

  j+="\"rkp\":"+String(pidRoll.kp)+",";
  j+="\"rki\":"+String(pidRoll.ki)+",";
  j+="\"rkd\":"+String(pidRoll.kd)+",";

  j+="\"pkp\":"+String(pidPitch.kp)+",";
  j+="\"pki\":"+String(pidPitch.ki)+",";
  j+="\"pkd\":"+String(pidPitch.kd)+",";

  j+="\"ykp\":"+String(pidYaw.kp)+",";
  j+="\"yki\":"+String(pidYaw.ki)+",";
  j+="\"ykd\":"+String(pidYaw.kd);

  j+="}";

  ws.broadcastTXT(j);

  delay(5);
}