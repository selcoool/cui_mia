
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsServer.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

// ================= WEB =================
WebServer server(80);
WebSocketsServer ws(81);
Preferences prefs;

// ================= DEVICE =================
String deviceMAC;
String remoteMAC = "";

// ================= BMP =================
Adafruit_BMP280 bmp;

float altitude = 0;
float alt0 = 0;
float SEA_LEVEL = 1009.5;

float altRaw = 0;
float altFilt = 0;
unsigned long lastAltRead = 0;
bool altReady = false;


// ================= PIN =================
#define SDA_PIN 8
#define SCL_PIN 9
#define MPU_ADDR 0x68

// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3


int M1_PIN=4, M2_PIN=5, M3_PIN=6, M4_PIN=7;


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

PID pidR, pidP, pidY;

// PID pidR = {4.5, 0.01, 0.08, 0, 0};
// PID pidP = {4.5, 0.01, 0.08, 0, 0};
// PID pidY = {0.0, 0.0, 0.02, 0, 0};
// PID pidAlt = {1.2, 0.02, 0.8, 0, 0};



float altTarget = 0;

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

  for(int i=0;i<1200;i++){
    gx += read16(0x43);
    gy += read16(0x45);
    gz += read16(0x47);

    ax += read16(0x3B);
    ay += read16(0x3D);
    az += read16(0x3F);
    delay(2);
  }

  gx_o = gx/1200.0;
  gy_o = gy/1200.0;
  gz_o = gz/1200.0;

  float axf=ax/1200.0/16384.0;
  float ayf=ay/1200.0/16384.0;
  float azf=az/1200.0/16384.0;

  roll0  = atan2(ayf, azf) * 57.2958;
  pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

  gz_bias = gz_o / 131.0;

  // ===== ALT CALIB FIX =====
  alt0 = 0;
  altReady = false;

  for(int i=0;i<60;i++){
    alt0 += bmp.readAltitude(SEA_LEVEL);
    delay(20);
  }

  alt0 /= 60;
  altFilt = 0;
  altitude = 0;
  altReady = true;
}

// ================= ALT FIXED =================
void updateAltitude(){

  if(!altReady) return;
  if(millis() - lastAltRead < 40) return; // 25Hz

  float raw = bmp.readAltitude(SEA_LEVEL) - alt0;

  // spike filter mạnh hơn
  if(fabs(raw - altRaw) > 2.5){
    raw = altRaw;
  }

  altRaw = raw;

  // low-pass filter (ổn định hơn bản cũ)
  altFilt = altFilt * 0.9 + raw * 0.1;

  altitude = altFilt;

  lastAltRead = millis();
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



// ================= LOAD PIN =================
void loadPin(){
  prefs.begin("pin",true);
  M1_PIN=prefs.getInt("m1",4);
  M2_PIN=prefs.getInt("m2",5);
  M3_PIN=prefs.getInt("m3",6);
  M4_PIN=prefs.getInt("m4",7);
  prefs.end();
}

// ================= SAVE PIN =================
void savePin(){
  prefs.begin("pin",false);
  prefs.putInt("m1",M1_PIN);
  prefs.putInt("m2",M2_PIN);
  prefs.putInt("m3",M3_PIN);
  prefs.putInt("m4",M4_PIN);
  prefs.end();
}


// ================= SAVE PID =================
void savePID(){
  prefs.begin("pid", false);

  prefs.putFloat("r_kp", pidR.kp);
  prefs.putFloat("r_ki", pidR.ki);
  prefs.putFloat("r_kd", pidR.kd);

  prefs.putFloat("p_kp", pidP.kp);
  prefs.putFloat("p_ki", pidP.ki);
  prefs.putFloat("p_kd", pidP.kd);

  prefs.putFloat("y_kp", pidY.kp);
  prefs.putFloat("y_ki", pidY.ki);
  prefs.putFloat("y_kd", pidY.kd);

  prefs.end();

  Serial.println("PID SAVED ✔");
}

// ================= LOAD PID =================
void loadPID(){
  prefs.begin("pid", true);

  pidR.kp=prefs.getFloat("r_kp",3);
  pidR.ki=prefs.getFloat("r_ki",0);
  pidR.kd=prefs.getFloat("r_kd",1.2);

  pidP.kp=prefs.getFloat("p_kp",3);
  pidP.ki=prefs.getFloat("p_ki",0);
  pidP.kd=prefs.getFloat("p_kd",1.2);

  pidY.kp=prefs.getFloat("y_kp",2);
  pidY.ki=prefs.getFloat("y_ki",0);
  pidY.kd=prefs.getFloat("y_kd",0);

  prefs.end();
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

// .row{display:flex;justify-content:center;gap:10px;flex-wrap:wrap}

.bar{height:10px;background:#333;border-radius:5px;overflow:hidden;margin:4px 0;}
.fill{height:100%;background:#00ffcc;transition:width .15s;}
.motor .fill{background:#ff3b3b;}

#warn{
  text-align:center;
  font-weight:bold;
  font-size:20px;
  margin:10px;
}

// .warn{color:red;text-align:center;display:none;font-weight:bold;}
</style>
</head>

<body>

<h2>🚁 FC DRONE CONNECTED</h2>
<div id="warn" ></div>

<div class="grid">

<div class="card">
<h3>ATTITUDE</h3>
Roll: <span id="r"></span><br>
Pitch: <span id="p"></span><br>
Yaw: <span id="y"></span><br>
<div class="box">ALT: <span id="alt" class="val">0</span> (m)<br></div>
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

<button onclick="send()">SAVE PID</button>

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

<button onclick="savePin()">APPLY</button>

</div>

</div>

<script>
const r = document.getElementById("r");
const p = document.getElementById("p");
const y = document.getElementById("y");
const alt = document.getElementById("alt");
const mac = document.getElementById("mac");
const rmac = document.getElementById("rmac");
const warn = document.getElementById("warn");


const r_kp = document.getElementById("r_kp");
const r_ki = document.getElementById("r_ki");
const r_kd = document.getElementById("r_kd");

const p_kp = document.getElementById("p_kp");
const p_ki = document.getElementById("p_ki");
const p_kd = document.getElementById("p_kd");


const y_kp = document.getElementById("y_kp");
const y_ki = document.getElementById("y_ki");
const y_kd = document.getElementById("y_kd");


const pm1 = document.getElementById("pm1");
const pm2 = document.getElementById("pm2");
const pm3 = document.getElementById("pm3");
const pm4 = document.getElementById("pm4");


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

try {
    let d = JSON.parse(e.data);

    console.log(d);

    // ===== ATTITUDE =====
    r.innerText = Number(d.r || 0).toFixed(2);
    p.innerText = Number(d.p || 0).toFixed(2);
    y.innerText = Number(d.y || 0).toFixed(2);
     alt.innerText=Number(d.alt || 0).toFixed(2);
    

    mac.innerText = d.mac || "-";
    rmac.innerText = d.rmac || "-";



     if(!window.init){
    r_kp.value=d.r_kp;
    r_ki.value=d.r_ki;
    r_kd.value=d.r_kd;

    p_kp.value=d.p_kp;
    p_ki.value=d.p_ki;
    p_kd.value=d.p_kd;

    y_kp.value=d.y_kp;
    y_ki.value=d.y_ki;
    y_kd.value=d.y_kd;


    pm1.value=d.pm1;
    pm2.value=d.pm2;
    pm3.value=d.pm3;
    pm4.value=d.pm4;


    window.init=1;
  }

    // ===== FAILSAFE =====

          if(d.fail){
        warn.innerText = "⚠ FAILSAFE";
        warn.style.color = "red";
      }
      else if(d.arm){
        warn.innerText = "ARMED";
        warn.style.color = "lime";
      }
      else{
        warn.innerText = "UNARMED";
        warn.style.color = "orange";
      }
    //  warn.innerText=d.fail ? "UNARMED" : "ARMED";
    // // warn.style.display = d.fail ? "block" : "none";

    // ===== TX =====
    bar("ch0", smooth(0,(d.ch0-1000)/10));
    bar("ch1", smooth(1,(d.ch1-1000)/10));
    bar("ch2", smooth(2,(d.ch2-1000)/10));
    bar("ch3", smooth(3,(d.ch3-1000)/10));
    

    // ===== MOTORS =====
    bar("m1", d.m1*100/255);
    bar("m2", d.m2*100/255);
    bar("m3", d.m3*100/255);
    bar("m4", d.m4*100/255);

  } catch (err) {
    console.log("WS error:", err);
  }
};


// ===== SEND PID =====
function send(){
  ws.send(
    "PID|" +
    r_kp.value + "," + r_ki.value + "," + r_kd.value + "|" +
    p_kp.value + "," + p_ki.value + "," + p_kd.value + "|" +
    y_kp.value + "," + y_ki.value + "," + y_kd.value
  );
}

// ===== SEND PIN =====
function savePin(){
  ws.send("PIN|"+pm1.value+","+pm2.value+","+pm3.value+","+pm4.value);
}


</script>

</body>
</html>
)rawliteral";

// ================= WEB =================
// void handleRoot(){
//   server.send_P(200,"text/html",HTML);
// }

// ================= WS =================
void onWs(uint8_t num,WStype_t type,uint8_t *payload,size_t len){

  if(type==WStype_TEXT){

    String msg=(char*)payload;

    if(msg.startsWith("PID|")){

      String d=msg.substring(4);

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

      savePID();   // 🔥 IMPORTANT FIX
    }

     if(msg.startsWith("PIN|")){
      String d=msg.substring(4);

      int a=d.indexOf(',');
      int b=d.indexOf(',',a+1);
      int c=d.indexOf(',',b+1);

      M1_PIN=d.substring(0,a).toInt();
      M2_PIN=d.substring(a+1,b).toInt();
      M3_PIN=d.substring(b+1,c).toInt();
      M4_PIN=d.substring(c+1).toInt();

      savePin();
      motorInit();   // 🔥 rebind PWM
      return;
    }

  }
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


 // BMP
//    bmp.begin(0x76);

if(!bmp.begin(0x76))
{ Serial.println("BMP FAIL");
 } 
else { 
Serial.println("BMP OK"); 
bmp.setSampling( 
Adafruit_BMP280::MODE_NORMAL, 
Adafruit_BMP280::SAMPLING_X2,
 Adafruit_BMP280::SAMPLING_X16, 
Adafruit_BMP280::FILTER_X16,
 Adafruit_BMP280::STANDBY_MS_63 ); 
}


  loadPin();

  motorInit();
  loadPID();
  calibrate();

  

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("FC-DRONE","12345678");



  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  // server.on("/",handleRoot);

   server.on("/",[](void){
    server.send_P(200,"text/html",HTML);
  });
  server.begin();

  ws.begin();
  ws.onEvent(onWs);

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

  updateAltitude();

float altHold = 0;




  // ================= RC =================
  float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

  float rollSet  = (rx.ch[3] - 1500) / 10.0;
  float pitchSet = (rx.ch[2] - 1500) / 10.0;
  float yawSet   = (rx.ch[1] - 1500) / 20.0;

  float m1=0,m2=0,m3=0,m4=0;

  if(arm){

    


//   // setpoint từ RC (CH5 ví dụ - giữ cao)
//   if(rx.ch[5] > 1500){
//     altTarget = altitude; // giữ độ cao hiện tại
//   }

//   float altOut = compute(pidAlt, altTarget, altitude, dt);

//   // giới hạn lực
//   altOut = constrain(altOut, -30, 30);

//   // cộng vào throttle
//   throttle += altOut;




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

  }

  setMotor(0,m1);
  setMotor(1,m2);
  setMotor(2,m3);
  setMotor(3,m4);

 String json="{";
json+="\"r\":"+String(roll)+",";
json+="\"p\":"+String(pitch)+",";
json+="\"y\":"+String(yaw)+",";
json+="\"alt\":"+String(altitude,2)+",";  // ✅ FIX ở đây
json+="\"mac\":\""+deviceMAC+"\",";
json+="\"rmac\":\""+remoteMAC+"\",";
json+="\"fail\":"+String(rxFail)+",";
json+="\"arm\":"+String(arm)+",";

json+="\"r_kp\":"+String(pidR.kp)+",";
json+="\"r_ki\":"+String(pidR.ki)+",";
json+="\"r_kd\":"+String(pidR.kd)+",";

json+="\"p_kp\":"+String(pidP.kp)+",";
json+="\"p_ki\":"+String(pidP.ki)+",";
json+="\"p_kd\":"+String(pidP.kd)+",";

json+="\"y_kp\":"+String(pidY.kp)+",";
json+="\"y_ki\":"+String(pidY.ki)+",";
json+="\"y_kd\":"+String(pidY.kd)+",";

json+="\"ch0\":"+String(rx.ch[0])+",";
json+="\"ch1\":"+String(rx.ch[1])+",";
json+="\"ch2\":"+String(rx.ch[2])+",";
json+="\"ch3\":"+String(rx.ch[3])+",";
json+="\"m1\":"+String(m1)+",";
json+="\"m2\":"+String(m2)+",";
json+="\"m3\":"+String(m3)+",";
json+="\"m4\":"+String(m4)+",";

json+="\"pm1\":"+String(M1_PIN)+",";
json+="\"pm2\":"+String(M2_PIN)+",";
json+="\"pm3\":"+String(M3_PIN)+",";
json+="\"pm4\":"+String(M4_PIN);
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


// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <WebServer.h>
// #include <Preferences.h>
// #include <WebSocketsServer.h>
// #include <Wire.h>

// // ================= WEB =================
// WebServer server(80);
// WebSocketsServer ws(81);
// Preferences prefs;

// // ================= DEVICE =================
// String deviceMAC;
// String remoteMAC = "";

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3


// int trimM1 = 0;
// int trimM2 = 0;
// int trimM3 = 5;
// int trimM4 = 5;

// // ================= RX =================
// typedef struct {
//   uint16_t ch[8];
// } PPMData;

// PPMData rx;

// // ================= FAILSAFE =================
// unsigned long lastRX = 0;
// bool rxFail = true;
// bool prevFail = true;

// // ================= ARM =================
// bool lastArm = false;

// // ================= STATE =================
// float roll = 0, pitch = 0, yaw = 0;
// float roll0 = 0, pitch0 = 0;

// // ================= GYRO =================
// float gx_o = 0, gy_o = 0, gz_o = 0;
// float gz_bias = 0;
// float gz_f = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {4.5, 0.01, 0.08, 0, 0};
// PID pidP = {4.5, 0.01, 0.08, 0, 0};
// PID pidY = {0.0, 0.0, 0.02, 0, 0};

// // ================= TIME =================
// unsigned long lastT = 0;

// // ================= READ MPU =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read()<<8) | Wire.read();
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
//   ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
//   ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
//   ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
// }

// String getMac(){
//   uint8_t mac[6];
//   WiFi.macAddress(mac);

//   char buf[18];
//   sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
//           mac[0], mac[1], mac[2],
//           mac[3], mac[4], mac[5]);

//   return String(buf);
// }

// void setMotor(int ch, float val){
//   ledcWrite(ch, constrain((int)val, 0, 255));
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac, const uint8_t *data, int len){

//   remoteMAC = "";
//   for(int i=0;i<6;i++){
//     char buf[3];
//     sprintf(buf,"%02X", mac[i]);
//     remoteMAC += buf;
//     if(i<5) remoteMAC += ":";
//   }
//   // Serial.print("ESP-NOW from: ");
//   // Serial.println(remoteMAC);
//   memcpy(&rx, data, sizeof(rx));
//   lastRX = millis();
// }

// // ================= CALIB =================
// void calibrate(){
//   long gx=0, gy=0, gz=0;
//   long ax=0, ay=0, az=0;

//   Serial.println("CALIB... KEEP STILL");

//   for(int i=0;i<1500;i++){
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);

//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     delay(2);
//   }

//   gx_o = gx / 1500.0;
//   gy_o = gy / 1500.0;
//   gz_o = gz / 1500.0;

//   float axf = ax / 1500.0 / 16384.0;
//   float ayf = ay / 1500.0 / 16384.0;
//   float azf = az / 1500.0 / 16384.0;

//   roll0  = atan2(ayf, azf) * 57.2958;
//   pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

//   gz_bias = gz_o / 131.0;

//   Serial.println("CAL DONE");
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -30, 30);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }


// // ================= HTML =================
// const char HTML[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
// <meta charset="UTF-8">
// <meta name="viewport" content="width=device-width, initial-scale=1">
// <title>FC PRO FIXED</title>

// <style>
// body{margin:0;background:#0b0f1a;color:#fff;font-family:Arial;}
// h2{text-align:center;color:#00ffcc;}

// .grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;padding:10px;}

// .card{background:#1c1f2a;padding:10px;border-radius:10px;}

// .bar{height:10px;background:#333;border-radius:5px;overflow:hidden;margin:4px 0;}
// .fill{height:100%;background:#00ffcc;transition:width .15s;}
// .motor .fill{background:#ff3b3b;}

// .warn{color:red;text-align:center;display:none;font-weight:bold;}
// </style>
// </head>

// <body>

// <h2>🚁 FC DRONE CONNECTED</h2>
// <div id="warn" class="warn">⚠ FAILSAFE</div>

// <div class="grid">

// <div class="card">
// <h3>ATTITUDE</h3>
// Roll: <span id="r"></span><br>
// Pitch: <span id="p"></span><br>
// Yaw: <span id="y"></span><br>
// <h3>DEVICE</h3>
// Mac: <span id="mac"></span><br>
// Remote Mac: <span id="rmac"></span> 
// </div>

// <div class="card">
// <h3>TX</h3>
// CH0 <div class="bar"><div id="ch0" class="fill"></div></div>
// CH1 <div class="bar"><div id="ch1" class="fill"></div></div>
// CH2 <div class="bar"><div id="ch2" class="fill"></div></div>
// CH3 <div class="bar"><div id="ch3" class="fill"></div></div>
// </div>


// <div class="card">
// <h2>PID CONTROL</h2>

// ROLL<br>
// KP <input id="r_kp"><br>
// KI <input id="r_ki"><br>
// KD <input id="r_kd"><br><br>

// PITCH<br>
// KP <input id="p_kp"><br>
// KI <input id="p_ki"><br>
// KD <input id="p_kd"><br><br>

// YAW<br>
// KP <input id="y_kp"><br>
// KI <input id="y_ki"><br>
// KD <input id="y_kd"><br><br>

// <button onclick="#">SAVE PID</button>

// </div>

// <div class="card">
// <h3>MOTORS</h3>
// M1 <div class="bar motor"><div id="m1" class="fill"></div></div>
// M2 <div class="bar motor"><div id="m2" class="fill"></div></div>
// M3 <div class="bar motor"><div id="m3" class="fill"></div></div>
// M4 <div class="bar motor"><div id="m4" class="fill"></div></div>

// <h3>MOTOR PINS</h3>
// M1 GPIO <input id="pm1"><br>
// M2 GPIO <input id="pm2"><br>
// M3 GPIO <input id="pm3"><br>
// M4 GPIO <input id="pm4"><br>

// <button onclick="#">APPLY</button>

// </div>

// </div>

// <script>

// let ws=new WebSocket("ws://"+location.hostname+":81/");

// function clamp(v,min,max){
//   return Math.max(min,Math.min(max,v));
// }

// // smooth bar (FIX JITTER)
// let sm=[0,0,0,0,0,0,0,0];

// function smooth(i,v){
//   sm[i]=sm[i]*0.7+v*0.3;
//   return sm[i];
// }

// // FIX SCALE 1000–2000 → 0–100%
// function bar(id,val){
//   val = clamp(val,0,100);
//   document.getElementById(id).style.width = val + "%";
// }

// ws.onmessage=(e)=>{
//   let d=JSON.parse(e.data);

//   r.innerText=d.r.toFixed(2);
//   p.innerText=d.p.toFixed(2);
//   y.innerText=d.y.toFixed(2);
//   mac.innerText = d.mac;
//   rmac.innerText = d.rmac;

//   warn.style.display = d.fail ? "block":"none";

//   // ===== FIX BAR SCALE =====
//   bar("ch0", smooth(0,(d.ch0-1000)/10));
//   bar("ch1", smooth(1,(d.ch1-1000)/10));
//   bar("ch2", smooth(2,(d.ch2-1000)/10));
//   bar("ch3", smooth(3,(d.ch3-1000)/10));

//   // motors
//   bar("m1", d.m1*100/255);
//   bar("m2", d.m2*100/255);
//   bar("m3", d.m3*100/255);
//   bar("m4", d.m4*100/255);
// };

// </script>

// </body>
// </html>
// )rawliteral";

// // ================= WEB =================
// void handleRoot(){
//   server.send_P(200,"text/html",HTML);
// }


// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);
//   deviceMAC = getMac();

//   Wire.setPins(SDA_PIN, SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0);
//   Wire.endTransmission();

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_AP_STA);
//   WiFi.softAP("FC-DRONE","12345678");

//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);

//   server.on("/",handleRoot);
//   server.begin();

//   ws.begin();

//   lastRX = millis();
// }

// // ================= LOOP =================
// void loop(){
//   server.handleClient();
//   ws.loop();
//   float dt = (micros() - lastT) * 1e-6;
//   lastT = micros();
//   if(dt <= 0 || dt > 0.05) dt = 0.01;

//   // ================= FAILSAFE =================
//   rxFail = (millis() - lastRX > 400);

//   if(rxFail){
//     setMotor(0,0);
//     setMotor(1,0);
//     setMotor(2,0);
//     setMotor(3,0);

//     roll = pitch = yaw = 0;
//     gz_bias = 0;

//     pidR.i = pidP.i = pidY.i = 0;

//     prevFail = true;
//     return;
//   }

//   if(prevFail && !rxFail){
//     yaw = 0;
//     gz_bias = 0;
//     pidR.i = pidP.i = pidY.i = 0;
//     prevFail = false;
//   }

//   // ================= MPU =================
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   float ax = read16(0x3B) / 16384.0;
//   float ay = read16(0x3D) / 16384.0;
//   float az = read16(0x3F) / 16384.0;

//   float accRoll  = atan2(ay, az) * 57.2958 - roll0;
//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

//   // ================= FILTER =================
//   gz_f = gz_f * 0.9 + gz * 0.1;

//   // ================= ARM =================
//   bool arm = rx.ch[4] > 1600;

//   // 🔥 ARM → CALIB
//   if(arm && !lastArm){
//     Serial.println("ARM -> CALIB");

//     calibrate();

//     roll = pitch = yaw = 0;
//     gz_bias = 0;
//     gz_f = 0;

//     pidR.i = pidP.i = pidY.i = 0;
//     pidR.last = pidP.last = pidY.last = 0;

//     delay(500);
//   }

//   lastArm = arm;

//   // ================= AUTO BIAS =================
//   if(arm && fabs(gz) < 1.0){
//     gz_bias = gz_bias * 0.999 + gz * 0.001;
//   }

//   float gz_corr = gz_f - gz_bias;
//   if(fabs(gz_corr) < 0.02) gz_corr = 0;

//   // ================= ANGLE =================
//   roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
//   pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;
//   yaw += gz_corr * dt;

//   if(yaw > 180) yaw -= 360;
//   if(yaw < -180) yaw += 360;

//   // ================= RC =================
//   float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

//   float rollSet  = (rx.ch[3] - 1500) / 10.0;
//   float pitchSet = (rx.ch[2] - 1500) / 10.0;
//   float yawSet   = (rx.ch[1] - 1500) / 20.0;

//   float m1=0,m2=0,m3=0,m4=0;

//   if(arm){
//     float rOut = compute(pidR, rollSet, roll, dt);
//     float pOut = compute(pidP, pitchSet, pitch, dt);
//     float yOut = compute(pidY, yawSet, yaw, dt);

//     rOut = constrain(rOut,-40,40);
//     pOut = constrain(pOut,-40,40);
//     yOut = constrain(yOut,-25,25);

//     m1 = throttle + pOut + rOut - yOut;
//     m2 = throttle + pOut - rOut + yOut;
//     m3 = throttle - pOut - rOut - yOut;
//     m4 = throttle - pOut + rOut + yOut;

//     // m1 = throttle + pOut + rOut ;
//     // m2 = throttle + pOut - rOut ;
//     // m3 = throttle - pOut - rOut;
//     // m4 = throttle - pOut + rOut ;

//     // m1 = throttle + pOut  + trimM1;
//     // m2 = throttle + pOut  + trimM2;
//     // m3 = throttle - pOut  + trimM3;
//     // m4 = throttle - pOut  + trimM4;


// //     m1 = throttle + pOut + rOut - yOut + trimM1;
// // m2 = throttle + pOut - rOut + yOut + trimM2;
// // m3 = throttle - pOut - rOut - yOut + trimM3;
// // m4 = throttle - pOut + rOut + yOut + trimM4;


 
//   }

//   setMotor(0,m1);
//   setMotor(1,m2);
//   setMotor(2,m3);
//   setMotor(3,m4);

//     String json="{";
//   json+="\"r\":"+String(roll)+",";
//   json+="\"p\":"+String(pitch)+",";
//   json+="\"y\":"+String(yaw)+",";
//   json+="\"mac\":\""+deviceMAC+"\",";
//   json+="\"rmac\":\""+remoteMAC+"\",";
//   json+="\"fail\":"+String(rxFail)+",";
//   json+="\"ch0\":"+String(rx.ch[0])+",";
//   json+="\"ch1\":"+String(rx.ch[1])+",";
//   json+="\"ch2\":"+String(rx.ch[2])+",";
//   json+="\"ch3\":"+String(rx.ch[3])+",";
//   json+="\"m1\":"+String(m1)+",";
//   json+="\"m2\":"+String(m2)+",";
//   json+="\"m3\":"+String(m3)+",";
//   json+="\"m4\":"+String(m4);
//   json+="}";

//   ws.broadcastTXT(json);

//   // ================= DEBUG =================
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.print(yaw);
//   Serial.print(" ARM:"); Serial.print(arm);

//   Serial.print(" deviceMAC:"); Serial.print(deviceMAC);

//   //  Serial.print(" pppppppppp:"); Serial.print( rx.ch[5]);
//   Serial.print(" FAIL:"); Serial.print(rxFail);

//   Serial.print(" M1:"); Serial.print(m1);
//   Serial.print(" M2:"); Serial.print(m2);
//   Serial.print(" M3:"); Serial.print(m3);
//   Serial.print(" M4:"); Serial.println(m4);
// }



// #include <Arduino.h>
// #include <Wire.h>
// #include <math.h>

// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// // ================= STATE =================
// float roll = 0, pitch = 0, yaw = 0;
// float roll0 = 0, pitch0 = 0;

// // ================= GYRO =================
// float gx_o = 0, gy_o = 0, gz_o = 0;
// float gz_bias = 0;
// float gz_f = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {7.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.02, 0, 0};

// // ================= TIME =================
// unsigned long lastT = 0;

// // ================= READ =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read() << 8) | Wire.read();
// }

// // ================= CALIB =================
// void calibrate(){
//   long gx=0, gy=0, gz=0;
//   long ax=0, ay=0, az=0;

//   Serial.println("CALIB... KEEP STILL");

//   for(int i=0;i<1500;i++){
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);

//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     delay(2);
//   }

//   gx_o = gx / 1500.0;
//   gy_o = gy / 1500.0;
//   gz_o = gz / 1500.0;

//   // lấy góc ban đầu làm ZERO
//   float axf = ax / 1500.0 / 16384.0;
//   float ayf = ay / 1500.0 / 16384.0;
//   float azf = az / 1500.0 / 16384.0;

//   roll0  = atan2(ayf, azf) * 57.2958;
//   pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

//   Serial.println("CALIB DONE");
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -30, 30);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN, SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0);
//   Wire.endTransmission();

//   calibrate();
// }

// // ================= LOOP =================
// void loop(){

//   unsigned long now = micros();
//   float dt = (now - lastT) * 1e-6;
//   lastT = now;
//   if(dt <= 0 || dt > 0.05) dt = 0.01;

//   // ===== RAW =====
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   float ax = read16(0x3B) / 16384.0;
//   float ay = read16(0x3D) / 16384.0;
//   float az = read16(0x3F) / 16384.0;

//   // ===== ACC ANGLE =====
//   float accRoll  = atan2(ay, az) * 57.2958 - roll0;
//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

//   // ===== GYRO FILTER =====
//   gz_f = gz_f * 0.9 + gz * 0.1;

//   // ===== BIAS AUTO =====
//   gz_bias = gz_bias * 0.999 + gz_f * 0.001;

//   float gz_corr = gz_f - gz_bias;

//   if(fabs(gz_corr) < 0.02) gz_corr = 0;

//   // ===== ANGLES =====
//   roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
//   pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;

//   yaw += gz_corr * dt;

//   if(yaw > 180) yaw -= 360;
//   if(yaw < -180) yaw += 360;

//   // ===== SETPOINT ZERO =====
//   float rollSet = 0;
//   float pitchSet = 0;
//   float yawSet = 0;

//   // ===== PID =====
//   float rOut = compute(pidR, rollSet, roll, dt);
//   float pOut = compute(pidP, pitchSet, pitch, dt);
//   float yOut = compute(pidY, yawSet, yaw, dt);

//   rOut = constrain(rOut, -40, 40);
//   pOut = constrain(pOut, -40, 40);
//   yOut = constrain(yOut, -25, 25);

//   // ===== DEBUG =====
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.println(yaw);

//   delay(5);
// }


// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <esp_now.h>

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3

// // ================= RC =================
// typedef struct {
//   uint16_t ch[8];
// } PPMData;

// PPMData rx;

// // ================= FAILSAFE =================
// unsigned long lastRX = 0;
// bool rxFail = true;
// bool prevFail = true;

// // ================= STATE =================
// float roll = 0, pitch = 0, yaw = 0;

// // ================= GYRO =================
// float gx_o=0, gy_o=0, gz_o=0;
// float gz_bias = 0;
// float gz_f = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {3.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.03, 0, 0};

// // ================= MPU =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read()<<8) | Wire.read();
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
//   ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
//   ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
//   ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
// }

// void setMotor(int ch, float val){
//   ledcWrite(ch, constrain((int)val, 0, 255));
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac, const uint8_t *data, int len){
//   memcpy(&rx, data, sizeof(rx));
//   lastRX = millis();
// }

// // ================= CALIB =================
// void calibrate(){
//   long gx=0, gy=0, gz=0;

//   Serial.println("CALIB... KEEP STILL");

//   for(int i=0;i<1500;i++){
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);
//     delay(2);
//   }

//   gx_o = gx/1500.0;
//   gy_o = gy/1500.0;
//   gz_o = gz/1500.0;

//   gz_bias = gz_o / 131.0;

//   Serial.println("CAL DONE");
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -25, 25);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// unsigned long lastT = 0;

// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN,SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0x00);
//   Wire.endTransmission();

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);

//   lastRX = millis();
// }

// // ================= LOOP =================
// void loop(){

//   float dt = (micros() - lastT) / 1000000.0;
//   lastT = micros();
//   if(dt <= 0 || dt > 0.05) dt = 0.01;

//   // ================= FAILSAFE =================
//   rxFail = (millis() - lastRX > 400);

//   if(rxFail){
//     setMotor(0,0);
//     setMotor(1,0);
//     setMotor(2,0);
//     setMotor(3,0);

//     roll = pitch = yaw = 0;
//     gz_bias = 0;

//     pidR.i = pidP.i = pidY.i = 0;

//     prevFail = true;
//     return;
//   }

//   // ================= MPU =================
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   float ax = read16(0x3B) / 16384.0;
//   float ay = read16(0x3D) / 16384.0;
//   float az = read16(0x3F) / 16384.0;

//   // ================= ACC ANGLE =================
//   float accRoll  = atan2(ay, az) * 57.3;
//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.3;

//   // ================= COMPLEMENTARY FILTER =================
//   roll  = 0.98*(roll + gx*dt) + 0.02*accRoll;
//   pitch = 0.98*(pitch + gy*dt) + 0.02*accPitch;

//   // ================= YAW FIX =================
//   gz_f = gz_f*0.9 + gz*0.1;

//   bool arm = rx.ch[4] > 1500;
//   float throttle = rx.ch[0];

//   // 🔥 chỉ học bias khi đứng yên + throttle thấp
//   if(arm && throttle < 1100 && fabs(gz_f) < 0.5){
//     gz_bias = gz_bias*0.995 + gz_f*0.005;
//   }

//   float gz_corr = gz_f - gz_bias;

//   // deadzone
//   if(fabs(gz_corr) < 0.02) gz_corr = 0;

//   yaw += gz_corr * dt;

//   if(yaw > 180) yaw -= 360;
//   if(yaw < -180) yaw += 360;

//   // ================= ARM EVENT =================
//   static bool lastArm = false;
//   if(arm && !lastArm){
//     yaw = 0;
//     gz_bias = 0;
//     pidR.i = pidP.i = pidY.i = 0;
//   }
//   lastArm = arm;

//   // ================= SETPOINT =================
//   float rollSet  = (rx.ch[3] - 1500) / 10.0;
//   float pitchSet = (rx.ch[2] - 1500) / 10.0;
//   float yawSet   = (rx.ch[1] - 1500) / 20.0;

//   float throttleOut = map(rx.ch[0], 1000, 2000, 90, 180);

//   float m1=0,m2=0,m3=0,m4=0;

//   if(arm){

//     float rOut = compute(pidR, rollSet, roll, dt);
//     float pOut = compute(pidP, pitchSet, pitch, dt);
//     float yOut = compute(pidY, yawSet, yaw, dt);

//     rOut = constrain(rOut,-35,35);
//     pOut = constrain(pOut,-35,35);
//     yOut = constrain(yOut,-25,25);

//     m1 = throttleOut + pOut + rOut - yOut;
//     m2 = throttleOut + pOut - rOut + yOut;
//     m3 = throttleOut - pOut - rOut - yOut;
//     m4 = throttleOut - pOut + rOut + yOut;
//   }

//   setMotor(0,m1);
//   setMotor(1,m2);
//   setMotor(2,m3);
//   setMotor(3,m4);

//   // ================= DEBUG =================
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.print(yaw);
//   Serial.print(" B:"); Serial.print(gz_bias);
//   Serial.print(" ARM:"); Serial.println(arm);
// }














// #include <Arduino.h>
// #include <Wire.h>
// #include <math.h>

// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// // ================= STATE =================
// float roll = 0, pitch = 0, yaw = 0;
// float roll0 = 0, pitch0 = 0;

// // ================= GYRO =================
// float gx_o = 0, gy_o = 0, gz_o = 0;
// float gz_bias = 0;
// float gz_f = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {7.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.02, 0, 0};

// // ================= TIME =================
// unsigned long lastT = 0;

// // ================= READ =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read() << 8) | Wire.read();
// }

// // ================= CALIB =================
// void calibrate(){
//   long gx=0, gy=0, gz=0;
//   long ax=0, ay=0, az=0;

//   Serial.println("CALIB... KEEP STILL");

//   for(int i=0;i<1500;i++){
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);

//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     delay(2);
//   }

//   gx_o = gx / 1500.0;
//   gy_o = gy / 1500.0;
//   gz_o = gz / 1500.0;

//   // lấy góc ban đầu làm ZERO
//   float axf = ax / 1500.0 / 16384.0;
//   float ayf = ay / 1500.0 / 16384.0;
//   float azf = az / 1500.0 / 16384.0;

//   roll0  = atan2(ayf, azf) * 57.2958;
//   pitch0 = atan2(-axf, sqrt(ayf*ayf + azf*azf)) * 57.2958;

//   Serial.println("CALIB DONE");
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -30, 30);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN, SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0);
//   Wire.endTransmission();

//   calibrate();
// }

// // ================= LOOP =================
// void loop(){

//   unsigned long now = micros();
//   float dt = (now - lastT) * 1e-6;
//   lastT = now;
//   if(dt <= 0 || dt > 0.05) dt = 0.01;

//   // ===== RAW =====
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   float ax = read16(0x3B) / 16384.0;
//   float ay = read16(0x3D) / 16384.0;
//   float az = read16(0x3F) / 16384.0;

//   // ===== ACC ANGLE =====
//   float accRoll  = atan2(ay, az) * 57.2958 - roll0;
//   float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 57.2958 - pitch0;

//   // ===== GYRO FILTER =====
//   gz_f = gz_f * 0.9 + gz * 0.1;

//   // ===== BIAS AUTO =====
//   gz_bias = gz_bias * 0.999 + gz_f * 0.001;

//   float gz_corr = gz_f - gz_bias;

//   if(fabs(gz_corr) < 0.02) gz_corr = 0;

//   // ===== ANGLES =====
//   roll  = 0.98 * (roll  + gx * dt) + 0.02 * accRoll;
//   pitch = 0.98 * (pitch + gy * dt) + 0.02 * accPitch;

//   yaw += gz_corr * dt;

//   if(yaw > 180) yaw -= 360;
//   if(yaw < -180) yaw += 360;

//   // ===== SETPOINT ZERO =====
//   float rollSet = 0;
//   float pitchSet = 0;
//   float yawSet = 0;

//   // ===== PID =====
//   float rOut = compute(pidR, rollSet, roll, dt);
//   float pOut = compute(pidP, pitchSet, pitch, dt);
//   float yOut = compute(pidY, yawSet, yaw, dt);

//   rOut = constrain(rOut, -40, 40);
//   pOut = constrain(pOut, -40, 40);
//   yOut = constrain(yOut, -25, 25);

//   // ===== DEBUG =====
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.println(yaw);

//   delay(5);
// }



// #include <Arduino.h>
// #include <Wire.h>

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// // ================= STATE =================
// float roll = 0, pitch = 0, yaw = 0;

// // ================= GYRO =================
// float gx_o = 0, gy_o = 0, gz_o = 0;
// float gz_bias = 0;
// float gz_f = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {3.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.0, 0, 0};

// // ================= TIME =================
// unsigned long lastT = 0;

// // ================= MPU READ =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read() << 8) | Wire.read();
// }

// // ================= CALIB =================
// void calibrate(){
//     long ax=0,ay=0,az=0,gx=0,gy=0,gz=0;

//   Serial.println("CALIBRATE KEEP STILL...");

//   for(int i=0;i<1500;i++){


//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);
//     delay(2);
//   }

//   gx_o = gx / 1500.0;
//   gy_o = gy / 1500.0;
//   gz_o = gz / 1500.0;

//   Serial.println("CALIB DONE");
// }

// // ================= FILTER =================
// float gyroFilter(float raw){
//   gz_f = gz_f * 0.85 + raw * 0.15;
//   return gz_f;
// }

// // ================= STABLE CHECK =================
// bool isStable(float gx, float gy, float gz){
//   return (fabs(gx) < 0.5 &&
//           fabs(gy) < 0.5 &&
//           fabs(gz) < 0.5);
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -25, 25);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN, SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0x00);
//   Wire.endTransmission();

//   calibrate();
// }

// // ================= LOOP =================
// void loop(){

//   // ===== DT =====
//   unsigned long now = micros();
//   float dt = (now - lastT) * 1e-6;
//   lastT = now;

//   if (dt <= 0 || dt > 0.05) dt = 0.01;

//   // ===== MPU =====
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   // ===== FILTER =====
//   gz = gyroFilter(gz);

//   // ===== AUTO BIAS =====
//   if (isStable(gx, gy, gz)) {
//     gz_bias = gz_bias * 0.995 + gz * 0.005;
//   }

//   // ===== CORRECT GYRO =====
//   float gz_corr = gz - gz_bias;

//   // deadzone chống drift
//   if (fabs(gz_corr) < 0.03) gz_corr = 0;

//   // ===== YAW =====
//   yaw += gz_corr * dt;

//   if (yaw > 180) yaw -= 360;
//   if (yaw < -180) yaw += 360;

//   // ===== ROLL / PITCH (simple integration) =====
//   roll  = 0.98 * roll  + gx * dt;
//   pitch = 0.98 * pitch + gy * dt;

//   // ===== SETPOINT = 0 (NO RX) =====
//   float rollSet = 0;
//   float pitchSet = 0;
//   float yawSet = 0;

//   // ===== PID =====
//   float rOut = compute(pidR, rollSet, roll, dt);
//   float pOut = compute(pidP, pitchSet, pitch, dt);
//   float yOut = compute(pidY, yawSet, yaw, dt);

//   // clamp output
//   rOut = constrain(rOut, -35, 35);
//   pOut = constrain(pOut, -35, 35);
//   yOut = constrain(yOut, -25, 25);

//   // ===== DEBUG =====
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.print(yaw);
//   Serial.print(" BIAS:"); Serial.println(gz_bias);

//   delay(5);
// }

// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Adafruit_BMP280.h>

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3

// // ================= RC =================
// typedef struct {
//   uint16_t ch[8];
// } PPMData;

// PPMData rx;

// // ================= FAILSAFE =================
// unsigned long lastRX = 0;
// bool rxFail = true;
// bool prevFail = true;

// // ================= STATE =================
// float roll = 0, pitch = 0;
// float yaw = 0;

// // ================= GYRO =================
// float gx_o=0, gy_o=0, gz_o=0;
// float gz_bias = 0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {3.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.0, 0, 0};

// // ================= MPU =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read()<<8) | Wire.read();
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
//   ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
//   ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
//   ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
// }

// void setMotor(int ch, float val){
//   ledcWrite(ch, constrain((int)val, 0, 255));
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac, const uint8_t *data, int len){
//   memcpy(&rx, data, sizeof(rx));
//   lastRX = millis();
// }

// // ================= CALIB =================
// void calibrate(){
//   long gx=0, gy=0, gz=0;

//   Serial.println("GYRO CALIB...");

//   for(int i=0;i<1500;i++){
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);
//     delay(2);
//   }

//   gx_o = gx/1500.0;
//   gy_o = gy/1500.0;
//   gz_o = gz/1500.0;

//   gz_bias = gz_o / 131.0;

//   Serial.println("CAL DONE");
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -25, 25);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// unsigned long lastT = 0;

// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN,SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0x00);
//   Wire.endTransmission();

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);

//   lastRX = millis();
// }

// // ================= LOOP =================
// void loop(){

//   float dt = (micros() - lastT) / 1000000.0;
//   lastT = micros();

//   // ================= FAILSAFE =================
//   rxFail = (millis() - lastRX > 400);

//   if(rxFail){

//     setMotor(0,0);
//     setMotor(1,0);
//     setMotor(2,0);
//     setMotor(3,0);

//     roll = 0;
//     pitch = 0;
//     yaw = 0;

//     gz_bias = 0; // 🔥 IMPORTANT RESET

//     pidR.i = pidP.i = pidY.i = 0;

//     prevFail = true;
//     return;
//   }

//   // ================= RECONNECT =================
//   if(prevFail && !rxFail){

//     yaw = 0;
//     gz_bias = 0;   // 🔥 RESET BIAS

//     pidR.i = pidP.i = pidY.i = 0;

//     prevFail = false;
//   }

//   // ================= MPU =================
//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   // ================= AUTO BIAS LEARN =================
//   if (!rxFail) {
//     gz_bias = gz_bias * 0.999 + gz * 0.001;
//   }

//   // ================= YAW FIX CORE =================
//   float gz_corr = gz - gz_bias;

//   if (fabs(gz_corr) < 0.03) gz_corr = 0;

//   yaw += gz_corr * dt;

//   // ================= ANGLE =================
//   roll  = 0.98 * roll + gx * dt;
//   pitch = 0.98 * pitch + gy * dt;

//   // ================= RC =================
//   bool arm = rx.ch[4] > 1500;

//   float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

//   float rollSet  = (rx.ch[3] - 1500) / 10.0;
//   float pitchSet = (rx.ch[2] - 1500) / 10.0;
//   float yawSet   = (rx.ch[1] - 1500) / 20.0;

//   float m1=0,m2=0,m3=0,m4=0;

//   if(arm){

//     float rOut = compute(pidR, rollSet, roll, dt);
//     float pOut = compute(pidP, pitchSet, pitch, dt);
//     float yOut = compute(pidY, yawSet, yaw, dt);

//     rOut = constrain(rOut,-35,35);
//     pOut = constrain(pOut,-35,35);
//     yOut = constrain(yOut,-25,25);

//     m1 = throttle + pOut + rOut - yOut;
//     m2 = throttle + pOut - rOut + yOut;
//     m3 = throttle - pOut - rOut - yOut;
//     m4 = throttle - pOut + rOut + yOut;
//   }

//   setMotor(0,m1);
//   setMotor(1,m2);
//   setMotor(2,m3);
//   setMotor(3,m4);

//   // ================= DEBUG =================
//   Serial.print("R:"); Serial.print(roll);
//   Serial.print(" P:"); Serial.print(pitch);
//   Serial.print(" Y:"); Serial.print(yaw);
//   Serial.print(" BIAS:"); Serial.print(gz_bias);
//   Serial.print(" ARM:"); Serial.print(arm);
//   Serial.print(" FAIL:"); Serial.println(rxFail);
// }


// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Adafruit_BMP280.h>

// // ================= PIN =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// #define M1_PIN 4
// #define M2_PIN 5
// #define M3_PIN 6
// #define M4_PIN 3

// // ================= BMP =================
// Adafruit_BMP280 bmp;
// float seaLevel = 1013.25;

// // ================= RC =================
// typedef struct {
//   uint16_t ch[8];
// } PPMData;

// PPMData rx;

// // ================= STATE =================
// float roll=0, pitch=0, yaw=0;
// float altF=0;

// // ================= OFFSET =================
// float ax_o=0, ay_o=0, az_o=0;
// float gx_o=0, gy_o=0, gz_o=0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {3.0, 0.01, 0.08, 0, 0};
// PID pidP = {3.0, 0.01, 0.08, 0, 0};
// PID pidY = {2.0, 0.0, 0.0, 0, 0};

// // ================= MPU READ =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   return (Wire.read()<<8) | Wire.read();
// }

// void writeReg(uint8_t reg, uint8_t val){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.write(val);
//   Wire.endTransmission();
// }

// // ================= PID =================
// float compute(PID &p, float set, float in, float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i, -25, 25);

//   float d = (e - p.last) / dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= CALIBRATION FIXED =================
// void calibrate(){
//   long ax=0,ay=0,az=0,gx=0,gy=0,gz=0;

//   Serial.println("KEEP DRONE FLAT...");

//   for(int i=0;i<1500;i++){
//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);

//     delay(2);
//   }

//   ax_o = ax/1000.0;
//   ay_o = ay/1000.0;
//   az_o = az/1000.0;   // 🔥 gravity fix

//   gx_o = gx/1000.0;
//   gy_o = gy/1000.0;
//   gz_o = gz/1000.0;

//   Serial.println("CAL DONE");
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac, const uint8_t *data, int len){
//   memcpy(&rx, data, sizeof(rx));
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 400, 8); ledcAttachPin(M1_PIN,0);
//   ledcSetup(1, 400, 8); ledcAttachPin(M2_PIN,1);
//   ledcSetup(2, 400, 8); ledcAttachPin(M3_PIN,2);
//   ledcSetup(3, 400, 8); ledcAttachPin(M4_PIN,3);
// }

// void setMotor(int ch, float val){
//   ledcWrite(ch, constrain((int)val, 0, 255));
// }

// // ================= SETUP =================
// unsigned long lastT=0;

// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN,SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   writeReg(0x6B,0x00);

//   bmp.begin(0x76);

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);
// }

// // ================= LOOP =================
// void loop(){

//   float dt = (micros() - lastT) / 1000000.0;
//   lastT = micros();

//   // ================= MPU =================
//   float ax = (read16(0x3B) - ax_o) / 16384.0;
//   float ay = (read16(0x3D) - ay_o) / 16384.0;
//   float az = (read16(0x3F) - az_o) / 16384.0;

//   float gx = (read16(0x43) - gx_o) / 131.0;
//   float gy = (read16(0x45) - gy_o) / 131.0;
//   float gz = (read16(0x47) - gz_o) / 131.0;

//   // ================= ANGLE =================
//   float rollAcc  = atan2(ay, az) * 57.3;
//   float pitchAcc = atan2(-ax, sqrt(ay*ay + az*az)) * 57.3;

//   // 🔥 STRONGER FILTER (VERY IMPORTANT)
//   roll  = 0.985 * (roll + gx * dt) + 0.015 * rollAcc;
//   pitch = 0.985 * (pitch + gy * dt) + 0.015 * pitchAcc;

//   // yaw drift only (no mag)
//   yaw += gz * dt;
//   if(fabs(gz) < 0.02) yaw *= 0.999;

//   // ================= ALT =================
//   altF = 0.9 * altF + 0.1 * bmp.readAltitude(seaLevel);

//   // ================= RC =================
//   bool arm = rx.ch[4] > 1500;

//   float throttle = map(rx.ch[0], 1000, 2000, 90, 180);

//   float rollSet  = (rx.ch[3] - 1500) / 10.0;
//   float pitchSet = (rx.ch[2] - 1500) / 10.0;
//   float yawSet   = (rx.ch[1] - 1500) / 20.0;

//   float m1=0,m2=0,m3=0,m4=0;

//   // if(arm){

//   //   // 🔥 deadband chống rung
//   //   if(abs(roll) < 0.8) roll = 0;
//   //   if(abs(pitch) < 0.8) pitch = 0;

//   //   float rOut = compute(pidR, rollSet, roll, dt);
//   //   float pOut = compute(pidP, pitchSet, pitch, dt);
//   //   float yOut = compute(pidY, yawSet, yaw, dt);

//   //   rOut = constrain(rOut, -35, 35);
//   //   pOut = constrain(pOut, -35, 35);
//   //   yOut = constrain(yOut, -25, 25);

//   //   // ================= MIXER X =================
//   //   m1 = throttle + pOut + rOut - yOut;
//   //   m2 = throttle + pOut - rOut + yOut;
//   //   m3 = throttle - pOut - rOut - yOut;
//   //   m4 = throttle - pOut + rOut + yOut;
//   // }

//   // setMotor(0,m1);
//   // setMotor(1,m2);
//   // setMotor(2,m3);
//   // setMotor(3,m4);

//   // // ================= DEBUG =================
//   // Serial.print("R:"); Serial.print(roll);
//   // Serial.print(" P:"); Serial.print(pitch);
//   // Serial.print(" Y:"); Serial.print(yaw);
//   // Serial.print(" ALT:"); Serial.print(altF);
//   // Serial.print(" ARM:"); Serial.print(arm);

//   // Serial.print(" M1:"); Serial.print(m1);
//   // Serial.print(" M2:"); Serial.print(m2);
//   // Serial.print(" M3:"); Serial.print(m3);
//   // Serial.print(" M4:"); Serial.println(m4);

//   delay(5);
// }






// #include <Wire.h>
//  #include <Arduino.h>

// #define MPU_ADDR 0x68

// // offset gyro
// float gx_o = 0, gy_o = 0, gz_o = 0;

// // angle
// float roll = 0, pitch = 0, yaw = 0;

// unsigned long lastTime = 0;

// // ===== READ 16 BIT SIGNED =====
// int16_t read16(uint8_t reg) {
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);

//   int16_t value = Wire.read() << 8 | Wire.read();
//   return value;
// }

// // ===== WRITE REG =====
// void writeReg(uint8_t reg, uint8_t val) {
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.write(val);
//   Wire.endTransmission();
// }

// // ===== CALIBRATE GYRO =====
// void calibrateGyro() {
//   long gx = 0, gy = 0, gz = 0;

//   for (int i = 0; i < 1000; i++) {
//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);
//     delay(2);
//   }

//   gx_o = gx / 1000.0;
//   gy_o = gy / 1000.0;
//   gz_o = gz / 1000.0;
// }

// void setup() {
//   Serial.begin(115200);
//   Wire.begin();

//   // wake up MPU6050
//   writeReg(0x6B, 0);

//   delay(100);

//   Serial.println("Calibrating gyro...");
//   calibrateGyro();
//   Serial.println("Done!");
  
//   lastTime = millis();
// }

// void loop() {
//   unsigned long now = millis();
//   float dt = (now - lastTime) / 1000.0;
//   lastTime = now;

//   // ===== RAW DATA =====
//   int16_t ax_raw = read16(0x3B);
//   int16_t ay_raw = read16(0x3D);
//   int16_t az_raw = read16(0x3F);

//   int16_t gx_raw = read16(0x43);
//   int16_t gy_raw = read16(0x45);
//   int16_t gz_raw = read16(0x47);

//   // ===== ACCEL (g) =====
//   float ax = ax_raw / 16384.0;
//   float ay = ay_raw / 16384.0;
//   float az = az_raw / 16384.0;

//   // ===== GYRO (deg/s) =====
//   float gx = (gx_raw - gx_o) / 131.0;
//   float gy = (gy_raw - gy_o) / 131.0;
//   float gz = (gz_raw - gz_o) / 131.0;

//   // ===== ACCEL ANGLE =====
//   float roll_acc  = atan2(ay, az) * 57.2958;
//   float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az)) * 57.2958;

//   // ===== COMPLEMENTARY FILTER =====
//   roll  = 0.98 * (roll + gx * dt) + 0.02 * roll_acc;
//   pitch = 0.98 * (pitch + gy * dt) + 0.02 * pitch_acc;
//   yaw   += gz * dt; // yaw không có accel reference

//   // ===== OUTPUT =====
//   Serial.print("Roll: ");
//   Serial.print(roll);
//   Serial.print("  Pitch: ");
//   Serial.print(pitch);
//   Serial.print("  Yaw: ");
//   Serial.println(yaw);

//   delay(10);
// }

// #include <Arduino.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <Adafruit_BMP280.h>

// // ================= I2C =================
// #define SDA_PIN 8
// #define SCL_PIN 9
// #define MPU_ADDR 0x68

// // ================= MOTOR =================
// #define M1 4
// #define M2 5
// #define M3 6
// #define M4 3

// // ================= BMP =================
// Adafruit_BMP280 bmp;
// float seaLevel = 1013.25;

// // ================= RC =================
// typedef struct {
//   uint16_t ch[8];
// } RC;

// RC rx;

// // ================= STATE =================
// float roll=0, pitch=0, yaw=0;
// float altF=0;

// // ================= OFFSETS =================
// float gx_o=0, gy_o=0, gz_o=0;
// float ax_o=0, ay_o=0, az_o=0;

// // ================= TIME =================
// unsigned long lastT=0;

// // ================= PID =================
// struct PID {
//   float kp, ki, kd;
//   float i, last;
// };

// PID pidR = {3.0,0.0,0.05,0,0};
// PID pidP = {3.0,0.0,0.05,0,0};
// PID pidY = {2.0,0.0,0.0,0,0};

// // ================= I2C SAFE =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR, 2);
//   if(Wire.available()<2) return 0;
//   return (Wire.read()<<8)|Wire.read();
// }

// void writeReg(uint8_t reg,uint8_t val){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.write(val);
//   Wire.endTransmission();
// }

// // ================= MOTOR =================
// void motorInit(){
//   ledcSetup(0, 200, 10); ledcAttachPin(M1,0);
//   ledcSetup(1, 200, 10); ledcAttachPin(M2,1);
//   ledcSetup(2, 200, 10); ledcAttachPin(M3,2);
//   ledcSetup(3, 200, 10); ledcAttachPin(M4,3);
// }

// void setMotor(int ch,int v){
//   ledcWrite(ch, constrain(v,0,255));
// }

// // ================= ESP-NOW =================
// void onRecv(const uint8_t *mac,const uint8_t *data,int len){
//   memcpy(&rx,data,sizeof(rx));
// }

// // ================= CALIB =================
// void calibrate(){

//   long gx=0,gy=0,gz=0;
//   long ax=0,ay=0,az=0;

//   Serial.println("CALIBRATION... DO NOT MOVE");
//   delay(2000);

//   for(int i=0;i<1000;i++){

//     ax += read16(0x3B);
//     ay += read16(0x3D);
//     az += read16(0x3F);

//     gx += read16(0x43);
//     gy += read16(0x45);
//     gz += read16(0x47);

//     delay(2);
//   }

//   gx_o = gx / 1000.0;
//   gy_o = gy / 1000.0;
//   gz_o = gz / 1000.0;

//   ax_o = ax / 1000.0;
//   ay_o = ay / 1000.0;
//   az_o = (az / 1000.0) - 16384.0;

//   roll = 0;
//   pitch = 0;
//   yaw = 0;

//   Serial.println("CALIB DONE");
// }

// // ================= PID =================
// float compute(PID &p,float set,float in,float dt){
//   float e = set - in;

//   p.i += e * dt;
//   p.i = constrain(p.i,-30,30);

//   float d = (e - p.last)/dt;
//   p.last = e;

//   return p.kp*e + p.ki*p.i + p.kd*d;
// }

// // ================= SETUP =================
// void setup(){
//   Serial.begin(115200);

//   Wire.setPins(SDA_PIN,SCL_PIN);
//   Wire.begin();
//   Wire.setClock(400000);

//   writeReg(0x6B,0x00);

//   bmp.begin(0x76);

//   motorInit();
//   calibrate();

//   WiFi.mode(WIFI_STA);
//   esp_now_init();
//   esp_now_register_recv_cb(onRecv);

//   lastT = micros();
// }

// // ================= LOOP =================
// void loop(){

//   float dt = (micros()-lastT)/1000000.0;
//   lastT = micros();

//   // ================= RAW =================
//   float ax_raw = read16(0x3B);
//   float ay_raw = read16(0x3D);
//   float az_raw = read16(0x3F);

//   float gx_raw = read16(0x43);
//   float gy_raw = read16(0x45);
//   float gz_raw = read16(0x47);

//   float ax = ax_raw - ax_o;
//   float ay = ay_raw - ay_o;
//   float az = az_raw - az_o;

//   float gx = (gx_raw - gx_o)/131.0;
//   float gy = (gy_raw - gy_o)/131.0;
//   float gz = (gz_raw - gz_o)/131.0;

//   // ================= ANGLE =================
//   float rollAcc  = atan2(ay,az)*57.3;
//   float pitchAcc = atan2(-ax,sqrt(ay*ay+az*az))*57.3;

//   roll  = 0.98*(roll + gx*dt) + 0.02*rollAcc;
//   pitch = 0.98*(pitch + gy*dt) + 0.02*pitchAcc;
//   yaw   += gz*dt;

//   // ================= ALT =================
//   float alt = bmp.readAltitude(seaLevel);
//   altF = 0.9*altF + 0.1*alt;

//   // ================= RC =================
//   float throttle = map(rx.ch[0],1000,2000,90,180);
//   float rSet = (rx.ch[3]-1500)/10.0;
//   float pSet = (rx.ch[2]-1500)/10.0;
//   float ySet = (rx.ch[1]-1500)/20.0;

//   // ================= PID =================
//   float rOut = compute(pidR,rSet,roll,dt);
//   float pOut = compute(pidP,pSet,pitch,dt);
//   float yOut = compute(pidY,ySet,yaw,dt);

//   rOut = constrain(rOut,-40,40);
//   pOut = constrain(pOut,-40,40);
//   yOut = constrain(yOut,-25,25);

//   // ================= MIX =================
//   float m1 = throttle + pOut + rOut - yOut;
//   float m2 = throttle + pOut - rOut + yOut;
//   float m3 = throttle - pOut - rOut - yOut;
//   float m4 = throttle - pOut + rOut + yOut;

//   setMotor(0,m1);
//   setMotor(1,m2);
//   setMotor(2,m3);
//   setMotor(3,m4);

//   // ================= DEBUG =================
//   Serial.print("R:");Serial.print(roll);
//   Serial.print(" P:");Serial.print(pitch);
//   Serial.print(" Y:");Serial.print(yaw);
//   Serial.print(" ALT:");Serial.println(altF);
// }










// #include <Arduino.h>
// #include <WiFi.h>
// #include <WebServer.h>
// #include <WebSocketsServer.h>
// #include <Preferences.h>
// #include <Wire.h>
// #include <Adafruit_BMP280.h>

// // ================= SERVER =================
// WebServer server(80);
// WebSocketsServer ws(81);
// Preferences prefs;

// // ================= WIFI =================
// const char* ssid = "FC-DRONE";
// const char* pass = "12345678";

// // ================= MPU =================
// #define MPU_ADDR 0x68

// float roll=0,pitch=0,yaw=0;
// float gx_o=0,gy_o=0,gz_o=0;

// unsigned long lastMicros=0;
// float dt=0;

// // ================= ALT =================
// Adafruit_BMP280 bmp;
// float altitude=0;
// float baseAlt=0;

// // ================= PID =================
// struct PID{float kp,ki,kd;};
// PID pidR={3,0,1.2};
// PID pidP={3,0,1.2};
// PID pidY={2,0,0};

// // ================= HTML =================
// const char html[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
// <meta charset="utf-8">
// <title>FC DRONE</title>

// <style>
// body{background:#0b0b0b;color:#00ff88;text-align:center;font-family:Arial}
// .row{display:flex;justify-content:center;gap:10px;flex-wrap:wrap}
// .box{border:1px solid #00ff88;padding:10px;margin:5px;width:220px;border-radius:10px}
// .title{color:#888;font-size:13px}
// .value{font-size:24px;font-weight:bold}
// input{width:60px;text-align:center}
// button{padding:10px;margin:5px;background:#111;color:#00ff88;border:1px solid #00ff88}
// </style>

// </head>

// <body>

// <h2>🚁 FC DRONE DASHBOARD</h2>

// <!-- ATTITUDE -->
// <div class="row">

// <div class="box"><div class="title">ROLL</div><div class="value" id="r">0</div></div>
// <div class="box"><div class="title">PITCH</div><div class="value" id="p">0</div></div>
// <div class="box"><div class="title">YAW</div><div class="value" id="y">0</div></div>
// <div class="box"><div class="title">ALT</div><div class="value" id="a">0</div></div>

// </div>

// <!-- PID -->
// <h3>PID CONTROL</h3>


// <div class="row"> <input id="mc"> </div>

// <div class="box">
// ROLL PID<br>
// <input id="rk"><input id="ri"><input id="rd">
// </div>

// <div class="box">
// PITCH PID<br>
// <input id="pk"><input id="pi"><input id="pd">
// </div>

// <div class="box">
// YAW PID<br>
// <input id="yk"><input id="yi"><input id="yd">
// </div>

// </div>

// <button onclick="save()">SAVE</button>
// <button onclick="reset()">RESET</button>
// <button onclick="calib()">CALIB</button>

// <script>

// let ws=new WebSocket("ws://"+location.hostname+":81/");

// ws.onopen=()=>{
//   console.log("WS CONNECTED");
// };

// // ===== RECEIVE =====
// ws.onmessage=(e)=>{
// console.log(e);
//  let d=JSON.parse(e.data);

//  if(d.t=="r"){
//   r.innerText=d.r.toFixed(2);
//   p.innerText=d.p.toFixed(2);
//   y.innerText=d.y.toFixed(2);
//   a.innerText=d.alt.toFixed(2);
//  }

//  if(d.t=="c"){
//   rk.value=d.rk;
//   ri.value=d.ri;
//   rd.value=d.rd;

//   pk.value=d.pk;
//   pi.value=d.pi;
//   pd.value=d.pd;

//   yk.value=d.yk;
//   yi.value=d.yi;
//   yd.value=d.yd;
//  }
// };

// // ===== SEND =====
// function save(){
//  ws.send("SAVE|"+rk.value+","+ri.value+","+rd.value+"|"
//  +pk.value+","+pi.value+","+pd.value+"|"
//  +yk.value+","+yi.value+","+yd.value);
// }

// function reset(){
//  ws.send("RESET");
// }

// function calib(){
//  ws.send("CALIB");
// }

// </script>

// </body>
// </html>
// )rawliteral";



// const char htmlOK[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head><title>ESP32 OK</title></head>
// <body>
// <h1>ESP32 WEB SERVER RUNNING</h1>
// </body>
// </html>
// )rawliteral";


// // ================= MPU =================
// int16_t read16(uint8_t reg){
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(reg);
//   Wire.endTransmission(false);
//   Wire.requestFrom(MPU_ADDR,2);
//   return (Wire.read()<<8)|Wire.read();
// }

// // ================= INIT =================
// void setupMPU(){
//   Wire.begin();
//   Wire.beginTransmission(MPU_ADDR);
//   Wire.write(0x6B);
//   Wire.write(0);
//   Wire.endTransmission();
// }

// // ================= CALIB =================
// void calibrateGyro(){
//   long gx=0,gy=0,gz=0;

//   for(int i=0;i<500;i++){
//     gx+=read16(0x43);
//     gy+=read16(0x45);
//     gz+=read16(0x47);
//     delay(2);
//   }

//   gx_o=gx/500.0;
//   gy_o=gy/500.0;
//   gz_o=gz/500.0;
// }

// void calibrateAlt(){
//   float s=0;
//   for(int i=0;i<100;i++){
//     s+=bmp.readAltitude(1013.25);
//     delay(10);
//   }
//   baseAlt=s/100.0;
// }

// // ================= SEND =================
// void sendRealtime(){
//   String j="{\"t\":\"r\",";
//   j+="\"r\":"+String(roll)+",";
//   j+="\"p\":"+String(pitch)+",";
//   j+="\"y\":"+String(yaw)+",";
//   j+="\"alt\":"+String(altitude)+"}";
//   ws.broadcastTXT(j);
// }

// void sendConfig(){
//   String j="{\"t\":\"c\",";
//   j+="\"rk\":"+String(pidR.kp)+",\"ri\":"+String(pidR.ki)+",\"rd\":"+String(pidR.kd)+",";
//   j+="\"pk\":"+String(pidP.kp)+",\"pi\":"+String(pidP.ki)+",\"pd\":"+String(pidP.kd)+",";
//   j+="\"yk\":"+String(pidY.kp)+",\"yi\":"+String(pidY.ki)+",\"yd\":"+String(pidY.kd)+"}";
//   ws.broadcastTXT(j);
// }

// // ================= RESET =================
// void factoryReset(){
//   roll=pitch=yaw=0;

//   pidR={3,0,1.2};
//   pidP={3,0,1.2};
//   pidY={2,0,0};

//   calibrateAlt();

//   sendConfig();   // 🔥 FIX UI UPDATE
// }

// // ================= WS =================
// void onWs1(uint8_t num,WStype_t type,uint8_t *payload,size_t len){

//   if(type==WStype_CONNECTED){
//     sendConfig();      // 🔥 FIX: gửi PID ngay khi connect
//     sendRealtime();
//     return;
//   }

//   if(type!=WStype_TEXT) return;

//   String msg="";
//   for(int i=0;i<len;i++) msg+=(char)payload[i];

//   if(msg=="RESET") factoryReset();

//   if(msg=="CALIB"){
//     calibrateGyro();
//     calibrateAlt();
//   }

//   if(msg.startsWith("SAVE|")){
//     String d=msg.substring(5);

//     int a=d.indexOf('|');
//     int b=d.indexOf('|',a+1);


//     Serial.print("      ");
//     Serial.print("d");
//     Serial.print(d);
//     Serial.print("     ");
//        Serial.print("a");
//     Serial.print(a);
//     Serial.print("   ");
//          Serial.print("b");
//     Serial.print(b);


//     String r=d.substring(0,a);
//     String p=d.substring(a+1,b);
//     String y=d.substring(b+1);

//     pidR.kp=r.substring(0,r.indexOf(',')).toFloat();
//     pidR.ki=r.substring(r.indexOf(',')+1,r.lastIndexOf(',')).toFloat();
//     pidR.kd=r.substring(r.lastIndexOf(',')+1).toFloat();

//     pidP.kp=p.substring(0,p.indexOf(',')).toFloat();
//     pidP.ki=p.substring(p.indexOf(',')+1,p.lastIndexOf(',')).toFloat();
//     pidP.kd=p.substring(p.lastIndexOf(',')+1).toFloat();

//     pidY.kp=y.substring(0,y.indexOf(',')).toFloat();
//     pidY.ki=y.substring(y.indexOf(',')+1,y.lastIndexOf(',')).toFloat();
//     pidY.kd=y.substring(y.lastIndexOf(',')+1).toFloat();

//     sendConfig(); // 🔥 FIX UPDATE UI
//   }
// }

// // ================= UPDATE =================
// void updateSensors(){

//   unsigned long now=micros();
//   dt=(now-lastMicros)/1000000.0;
//   lastMicros=now;

//   if(dt<=0||dt>0.1) dt=0.01;

//   float gx=(read16(0x43)-gx_o)/131.0;
//   float gy=(read16(0x45)-gy_o)/131.0;
//   float gz=(read16(0x47)-gz_o)/131.0;

//   float ax=read16(0x3B)/16384.0;
//   float ay=read16(0x3D)/16384.0;
//   float az=read16(0x3F)/16384.0;

//   float ar=atan2(ay,az)*57.3;
//   float ap=atan2(-ax,sqrt(ay*ay+az*az))*57.3;

//   roll=0.98*(roll+gx*dt)+0.02*ar;
//   pitch=0.98*(pitch+gy*dt)+0.02*ap;
//   yaw+=gz*dt;

//   float raw=bmp.readAltitude(1013.25);
//   altitude=0.9*altitude+0.1*(raw-baseAlt);
// }

// // ================= SETUP =================
// void setup(){

//   Serial.begin(115200);

//   setupMPU();

//   if(!bmp.begin(0x76)){
//     Serial.println("BMP FAIL");
//   }

//   calibrateGyro();
//   calibrateAlt();

//   WiFi.softAP(ssid,pass);

//   server.on("/",[](){
//     server.send_P(200,"text/html",html);
//   });
//   server.on("/sss",[](){
//     server.send_P(200,"text/html",htmlOK);
//   });
  
// //  server.on("/test", []() {
// //   server.send(200, "text/plain", "sddsds");
// // });

//   server.begin();

//   ws.begin();
//   ws.onEvent(onWs1);

//   lastMicros=micros();
// }

// // ================= LOOP =================
// void loop(){

//   server.handleClient();
//   ws.loop();

//   updateSensors();

//   static unsigned long t=0;
//   if(millis()-t>20){
//     sendRealtime();
//     t=millis();
//   }

//   delay(5);
// }



