
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
String remoteMAC = "";

// ================= PIN =================
#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

#define M1_PIN 4
#define M2_PIN 5
#define M3_PIN 6
#define M4_PIN 3


int trimM1 = 0;
int trimM2 = 0;
int trimM3 = 5;
int trimM4 = 5;

// ================= RX =================
typedef struct {
  uint16_t ch[8];
} PPMData;

PPMData rx;

// ================= FAILSAFE =================
unsigned long lastRX = 0;
bool rxFail = true;
bool prevFail = true;

// ================= ARM =================
bool lastArm = false;

// ================= STATE =================
float roll = 0, pitch = 0, yaw = 0;
float roll0 = 0, pitch0 = 0;

// ================= GYRO =================
float gx_o = 0, gy_o = 0, gz_o = 0;
float gz_bias = 0;
float gz_f = 0;

// ================= PID =================
struct PID {
  float kp, ki, kd;
  float i, last;
};

PID pidR = {4.5, 0.01, 0.08, 0, 0};
PID pidP = {4.5, 0.01, 0.08, 0, 0};
PID pidY = {0.0, 0.0, 0.02, 0, 0};

// ================= TIME =================
unsigned long lastT = 0;

// ================= READ MPU =================
int16_t read16(uint8_t reg){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read()<<8) | Wire.read();
}

// ================= MOTOR =================
void motorInit(){
  ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
  ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
  ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
  ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
}

String getMac(){
  uint8_t mac[6];
  WiFi.macAddress(mac);

  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);

  return String(buf);
}

void setMotor(int ch, float val){
  ledcWrite(ch, constrain((int)val, 0, 255));
}

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac, const uint8_t *data, int len){

  remoteMAC = "";
  for(int i=0;i<6;i++){
    char buf[3];
    sprintf(buf,"%02X", mac[i]);
    remoteMAC += buf;
    if(i<5) remoteMAC += ":";
  }
  // Serial.print("ESP-NOW from: ");
  // Serial.println(remoteMAC);
  memcpy(&rx, data, sizeof(rx));
  lastRX = millis();
}

// ================= CALIB =================
void calibrate(){
  long gx=0, gy=0, gz=0;
  long ax=0, ay=0, az=0;

  Serial.println("CALIB... KEEP STILL");

  for(int i=0;i<1500;i++){
    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);

    delay(2);
  }

  gx_o = gx / 1500.0;
  gy_o = gy / 1500.0;
  gz_o = gz / 1500.0;

  float axf = ax / 1500.0 / 16384.0;
  float ayf = ay / 1500.0 / 16384.0;
  float azf = az / 1500.0 / 16384.0;

  roll0  = atan2(ayf, azf) * 57.2958;
  pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

  gz_bias = gz_o / 131.0;

  Serial.println("CAL DONE");
}

// ================= PID =================
float compute(PID &p, float set, float in, float dt){
  float e = set - in;

  p.i += e * dt;
  p.i = constrain(p.i, -30, 30);

  float d = (e - p.last) / dt;
  p.last = e;

  return p.kp*e + p.ki*p.i + p.kd*d;
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

<h2>🚁 FC DRONE CONNECTED</h2>
<div id="warn" class="warn">⚠ FAILSAFE</div>

<div class="grid">

<div class="card">
<h3>ATTITUDE</h3>
Roll: <span id="r"></span><br>
Pitch: <span id="p"></span><br>
Yaw: <span id="y"></span><br>
<h3>DEVICE</h3>
Mac: <span id="mac"></span><br>
Remote Mac: <span id="rmac"></span> 
</div>

<div class="card">
<h3>TX</h3>
CH0 <div class="bar"><div id="ch0" class="fill"></div></div>
CH1 <div class="bar"><div id="ch1" class="fill"></div></div>
CH2 <div class="bar"><div id="ch2" class="fill"></div></div>
CH3 <div class="bar"><div id="ch3" class="fill"></div></div>
</div>


<div class="card">
<h2>PID CONTROL</h2>

ROLL<br>
KP <input id="r_kp"><br>
KI <input id="r_ki"><br>
KD <input id="r_kd"><br><br>

PITCH<br>
KP <input id="p_kp"><br>
KI <input id="p_ki"><br>
KD <input id="p_kd"><br><br>

YAW<br>
KP <input id="y_kp"><br>
KI <input id="y_ki"><br>
KD <input id="y_kd"><br><br>

<button onclick="#">SAVE PID</button>

</div>

<div class="card">
<h3>MOTORS</h3>
M1 <div class="bar motor"><div id="m1" class="fill"></div></div>
M2 <div class="bar motor"><div id="m2" class="fill"></div></div>
M3 <div class="bar motor"><div id="m3" class="fill"></div></div>
M4 <div class="bar motor"><div id="m4" class="fill"></div></div>

<h3>MOTOR PINS</h3>
M1 GPIO <input id="pm1"><br>
M2 GPIO <input id="pm2"><br>
M3 GPIO <input id="pm3"><br>
M4 GPIO <input id="pm4"><br>

<button onclick="#">APPLY</button>

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
  mac.innerText = d.mac;
  rmac.innerText = d.rmac;

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
void setup(){
  Serial.begin(115200);
  deviceMAC = getMac();

  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  motorInit();
  calibrate();

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
  float dt = (micros() - lastT) * 1e-6;
  lastT = micros();
  if(dt <= 0 || dt > 0.05) dt = 0.01;

  // ================= FAILSAFE =================
  rxFail = (millis() - lastRX > 400);

  if(rxFail){
    setMotor(0,0);
    setMotor(1,0);
    setMotor(2,0);
    setMotor(3,0);

    roll = pitch = yaw = 0;
    gz_bias = 0;

    pidR.i = pidP.i = pidY.i = 0;

    prevFail = true;
    return;
  }

  if(prevFail && !rxFail){
    yaw = 0;
    gz_bias = 0;
    pidR.i = pidP.i = pidY.i = 0;
    prevFail = false;
  }

  // ================= MPU =================
  float gx = (read16(0x43) - gx_o) / 131.0;
  float gy = (read16(0x45) - gy_o) / 131.0;
  float gz = (read16(0x47) - gz_o) / 131.0;

  float ax = read16(0x3B) / 16384.0;
  float ay = read16(0x3D) / 16384.0;
  float az = read16(0x3F) / 16384.0;

  float accRoll  = atan2(ay, az) * 57.2958 - roll0;
  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

  // ================= FILTER =================
  gz_f = gz_f * 0.9 + gz * 0.1;

  // ================= ARM =================
  bool arm = rx.ch[4] > 1600;

  // 🔥 ARM → CALIB
  if(arm && !lastArm){
    Serial.println("ARM -> CALIB");

    calibrate();

    roll = pitch = yaw = 0;
    gz_bias = 0;
    gz_f = 0;

    pidR.i = pidP.i = pidY.i = 0;
    pidR.last = pidP.last = pidY.last = 0;

    delay(500);
  }

  lastArm = arm;

  // ================= AUTO BIAS =================
  if(arm && fabs(gz) < 1.0){
    gz_bias = gz_bias * 0.999 + gz * 0.001;
  }

  float gz_corr = gz_f - gz_bias;
  if(fabs(gz_corr) < 0.02) gz_corr = 0;

  // ================= ANGLE =================
  roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
  pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;
  yaw += gz_corr * dt;

  if(yaw > 180) yaw -= 360;
  if(yaw < -180) yaw += 360;

  // ================= RC =================
  float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

  float rollSet  = (rx.ch[3] - 1500) / 10.0;
  float pitchSet = (rx.ch[2] - 1500) / 10.0;
  float yawSet   = (rx.ch[1] - 1500) / 20.0;

  float m1=0,m2=0,m3=0,m4=0;

  if(arm){
    float rOut = compute(pidR, rollSet, roll, dt);
    float pOut = compute(pidP, pitchSet, pitch, dt);
    float yOut = compute(pidY, yawSet, yaw, dt);

    rOut = constrain(rOut,-40,40);
    pOut = constrain(pOut,-40,40);
    yOut = constrain(yOut,-25,25);

    m1 = throttle + pOut + rOut - yOut;
    m2 = throttle + pOut - rOut + yOut;
    m3 = throttle - pOut - rOut - yOut;
    m4 = throttle - pOut + rOut + yOut;

    // m1 = throttle + pOut + rOut ;
    // m2 = throttle + pOut - rOut ;
    // m3 = throttle - pOut - rOut;
    // m4 = throttle - pOut + rOut ;

    // m1 = throttle + pOut  + trimM1;
    // m2 = throttle + pOut  + trimM2;
    // m3 = throttle - pOut  + trimM3;
    // m4 = throttle - pOut  + trimM4;


//     m1 = throttle + pOut + rOut - yOut + trimM1;
// m2 = throttle + pOut - rOut + yOut + trimM2;
// m3 = throttle - pOut - rOut - yOut + trimM3;
// m4 = throttle - pOut + rOut + yOut + trimM4;


 
  }

  setMotor(0,m1);
  setMotor(1,m2);
  setMotor(2,m3);
  setMotor(3,m4);

    String json="{";
  json+="\"r\":"+String(roll)+",";
  json+="\"p\":"+String(pitch)+",";
  json+="\"y\":"+String(yaw)+",";
  json+="\"mac\":\""+deviceMAC+"\",";
  json+="\"rmac\":\""+remoteMAC+"\",";
  json+="\"fail\":"+String(rxFail)+",";
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

  // ================= DEBUG =================
  Serial.print("R:"); Serial.print(roll);
  Serial.print(" P:"); Serial.print(pitch);
  Serial.print(" Y:"); Serial.print(yaw);
  Serial.print(" ARM:"); Serial.print(arm);

  Serial.print(" deviceMAC:"); Serial.print(deviceMAC);

  //  Serial.print(" pppppppppp:"); Serial.print( rx.ch[5]);
  Serial.print(" FAIL:"); Serial.print(rxFail);

  Serial.print(" M1:"); Serial.print(m1);
  Serial.print(" M2:"); Serial.print(m2);
  Serial.print(" M3:"); Serial.print(m3);
  Serial.print(" M4:"); Serial.println(m4);
}
