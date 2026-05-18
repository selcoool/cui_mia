#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

// ================= STORAGE =================
Preferences prefs;

// ================= WIFI =================
String ssid = "ESP32_RC";
String pass = "12345678";

// ================= JOYSTICK PINS =================
uint8_t J1_X=0;
uint8_t J1_Y=1;
uint8_t J2_X=3;
uint8_t J2_Y=4;

// ================= CALIB =================
int j1x_min=100;
int j1x_center=2048;
int j1x_max=4000;

int j1y_min=100;
int j1y_center=2048;
int j1y_max=4000;

int j2x_min=100;
int j2x_center=2048;
int j2x_max=4000;

int j2y_min=100;
int j2y_center=2048;
int j2y_max=4000;

// ================= FILTER =================
float l1=1500;
float l2=1500;
float l3=1500;
float l4=1500;

// ================= ESP NOW =================
uint8_t receiverMAC[6]={0x18,0x8B,0x0E,0x92,0x66,0x34};

typedef struct{
  uint16_t ch[4];
} Data;

Data data;

// ================= SERVER =================
WebServer server(80);
WebSocketsServer ws(81);

// ================= HELPERS =================
String macToString(uint8_t mac[6]){
  char buf[18];

  sprintf(buf,
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]);

  return String(buf);
}

bool parseMAC(String s){

  int v[6];

  if(sscanf(s.c_str(),
            "%x:%x:%x:%x:%x:%x",
            &v[0],
            &v[1],
            &v[2],
            &v[3],
            &v[4],
            &v[5])==6){

    for(int i=0;i<6;i++){
      receiverMAC[i]=(uint8_t)v[i];
    }

    return true;
  }

  return false;
}

// ================= SAVE =================
void saveConfig(){

  prefs.begin("rc", false);

  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);

  prefs.putBytes("mac", receiverMAC, 6);

  prefs.putUChar("j1x", J1_X);
  prefs.putUChar("j1y", J1_Y);
  prefs.putUChar("j2x", J2_X);
  prefs.putUChar("j2y", J2_Y);

  prefs.putInt("j1x_min", j1x_min);
  prefs.putInt("j1x_ct", j1x_center);
  prefs.putInt("j1x_max", j1x_max);

  prefs.putInt("j1y_min", j1y_min);
  prefs.putInt("j1y_ct", j1y_center);
  prefs.putInt("j1y_max", j1y_max);

  prefs.putInt("j2x_min", j2x_min);
  prefs.putInt("j2x_ct", j2x_center);
  prefs.putInt("j2x_max", j2x_max);

  prefs.putInt("j2y_min", j2y_min);
  prefs.putInt("j2y_ct", j2y_center);
  prefs.putInt("j2y_max", j2y_max);

  prefs.end();
}

// ================= LOAD =================
void loadConfig(){

  prefs.begin("rc", true);

  ssid = prefs.getString("ssid", ssid);
  pass = prefs.getString("pass", pass);

  prefs.getBytes("mac", receiverMAC, 6);

  J1_X = prefs.getUChar("j1x", J1_X);
  J1_Y = prefs.getUChar("j1y", J1_Y);
  J2_X = prefs.getUChar("j2x", J2_X);
  J2_Y = prefs.getUChar("j2y", J2_Y);

  j1x_min = prefs.getInt("j1x_min", j1x_min);
  j1x_center = prefs.getInt("j1x_ct", j1x_center);
  j1x_max = prefs.getInt("j1x_max", j1x_max);

  j1y_min = prefs.getInt("j1y_min", j1y_min);
  j1y_center = prefs.getInt("j1y_ct", j1y_center);
  j1y_max = prefs.getInt("j1y_max", j1y_max);

  j2x_min = prefs.getInt("j2x_min", j2x_min);
  j2x_center = prefs.getInt("j2x_ct", j2x_center);
  j2x_max = prefs.getInt("j2x_max", j2x_max);

  j2y_min = prefs.getInt("j2y_min", j2y_min);
  j2y_center = prefs.getInt("j2y_ct", j2y_center);
  j2y_max = prefs.getInt("j2y_max", j2y_max);

  prefs.end();
}

// ================= ESP NOW =================
void addPeer(){

  esp_now_peer_info_t peer={};

  memcpy(peer.peer_addr, receiverMAC, 6);

  peer.channel=0;
  peer.encrypt=false;

  esp_now_del_peer(receiverMAC);
  esp_now_add_peer(&peer);
}

// ================= JOYSTICK =================
int mapJoy(int v,int mn,int ct,int mx){

  v=constrain(v,mn,mx);

  if(v<ct){
    return map(v,mn,ct,1000,1500);
  }

  return map(v,ct,mx,1500,2000);
}

int stable(int raw,int mn,int ct,int mx,float &last){

  int v=mapJoy(raw,mn,ct,mx);

  if(abs(v-1500)<8){
    v=1500;
  }

  v=(last*0.5)+(v*0.5);

  if(abs(v-last)<2){
    v=last;
  }

  last=v;

  return v;
}

// ================= API =================
void handleGet(){

  String j="{";

  j+="\"esp_mac\":\""+WiFi.macAddress()+"\",";
  j+="\"receiver_mac\":\""+macToString(receiverMAC)+"\",";

  j+="\"ssid\":\""+ssid+"\",";
  j+="\"pass\":\""+pass+"\",";

  j+="\"j1x_pin\":"+String(J1_X)+",";
  j+="\"j1y_pin\":"+String(J1_Y)+",";
  j+="\"j2x_pin\":"+String(J2_X)+",";
  j+="\"j2y_pin\":"+String(J2_Y)+",";

  j+="\"j1x_min\":"+String(j1x_min)+",";
  j+="\"j1x_center\":"+String(j1x_center)+",";
  j+="\"j1x_max\":"+String(j1x_max)+",";

  j+="\"j1y_min\":"+String(j1y_min)+",";
  j+="\"j1y_center\":"+String(j1y_center)+",";
  j+="\"j1y_max\":"+String(j1y_max)+",";

  j+="\"j2x_min\":"+String(j2x_min)+",";
  j+="\"j2x_center\":"+String(j2x_center)+",";
  j+="\"j2x_max\":"+String(j2x_max)+",";

  j+="\"j2y_min\":"+String(j2y_min)+",";
  j+="\"j2y_center\":"+String(j2y_center)+",";
  j+="\"j2y_max\":"+String(j2y_max);

  j+="}";

  server.send(200,"application/json",j);
}

// ================= WIFI =================
void handleSetWifi(){

  ssid=server.arg("ssid");
  pass=server.arg("pass");

  WiFi.softAP(ssid.c_str(), pass.c_str());

  saveConfig();

  server.send(200,"text/plain","OK");
}

// ================= MAC =================
void handleSetMac(){

  if(parseMAC(server.arg("mac"))){

    addPeer();

    saveConfig();

    server.send(200,"text/plain","OK");
  }
  else{
    server.send(400,"text/plain","BAD MAC");
  }
}

// ================= PINS =================
void handleSetPins(){

  J1_X=server.arg("j1x_pin").toInt();
  J1_Y=server.arg("j1y_pin").toInt();

  J2_X=server.arg("j2x_pin").toInt();
  J2_Y=server.arg("j2y_pin").toInt();

  saveConfig();

  server.send(200,"text/plain","OK");
}

// ================= CALIB =================
void handleSetCalib(){

  j1x_min=server.arg("j1x_min").toInt();
  j1x_center=server.arg("j1x_center").toInt();
  j1x_max=server.arg("j1x_max").toInt();

  j1y_min=server.arg("j1y_min").toInt();
  j1y_center=server.arg("j1y_center").toInt();
  j1y_max=server.arg("j1y_max").toInt();

  j2x_min=server.arg("j2x_min").toInt();
  j2x_center=server.arg("j2x_center").toInt();
  j2x_max=server.arg("j2x_max").toInt();

  j2y_min=server.arg("j2y_min").toInt();
  j2y_center=server.arg("j2y_center").toInt();
  j2y_max=server.arg("j2y_max").toInt();

  saveConfig();

  server.send(200,"text/plain","OK");
}

// ================= HTML =================
const char HTML[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">

<title>ESP32 RC</title>

<style>

body{
  margin:0;
  background:#0d1117;
  color:white;
  font-family:Arial;
  padding:10px;
}

.title{
  text-align:center;
  font-size:22px;
  color:#00e5ff;
  margin-bottom:10px;
}

.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:10px;
}

.card{
  background:#161b22;
  border-radius:14px;
  padding:12px;
}

.card h3{
  margin-top:0;
  color:#00ff95;
}

input{
  width:100%;
  padding:8px;
  margin:4px 0;
  border:none;
  border-radius:8px;
  background:#222b36;
  color:white;
  box-sizing:border-box;
}

button{
  width:100%;
  padding:10px;
  margin-top:8px;
  border:none;
  border-radius:8px;
  background:#00c853;
  color:white;
  font-weight:bold;
}

.live{
  display:grid;
  grid-template-columns:1fr 1fr 1fr 1fr;
  gap:8px;
}

.box{
  background:#222b36;
  padding:10px;
  border-radius:10px;
  text-align:center;
}

.val{
  font-size:20px;
  color:#00e5ff;
}

.full{
  grid-column:1 / span 2;
}

.label{
  font-size:12px;
  color:#aaa;
  margin-top:6px;
}

.mac{
  background:#222b36;
  padding:8px;
  border-radius:8px;
  word-break:break-all;
}

@media(max-width:700px){

  .grid{
    grid-template-columns:1fr;
  }

  .full{
    grid-column:auto;
  }

  .live{
    grid-template-columns:1fr 1fr;
  }
}

</style>

</head>

<body>

<div class="title">
ESP32 RC CONTROL
</div>

<div class="grid">

<div class="card">

<h3>WIFI</h3>

<input id="ssid" placeholder="SSID">
<input id="pass" placeholder="PASSWORD">

<button onclick="saveWifi()">
SAVE WIFI
</button>

</div>

<div class="card">

<h3>ESP NOW</h3>

<div class="label">ESP32 MAC</div>
<div class="mac" id="esp_mac"></div>

<div class="label">RECEIVER MAC</div>
<input id="receiver_mac">

<button onclick="saveMac()">
SAVE MAC
</button>

</div>



<div class="card">

<h3>JOY1 CALIB</h3>

<div class="label">JOY1 X</div>

<input id="j1x_min" placeholder="MIN">
<input id="j1x_center" placeholder="CENTER">
<input id="j1x_max" placeholder="MAX">

<div class="label">JOY1 Y</div>

<input id="j1y_min" placeholder="MIN">
<input id="j1y_center" placeholder="CENTER">
<input id="j1y_max" placeholder="MAX">


<div class="label">JOY2 X</div>

<input id="j2x_min" placeholder="MIN">
<input id="j2x_center" placeholder="CENTER">
<input id="j2x_max" placeholder="MAX">

<div class="label">JOY2 Y</div>

<input id="j2y_min" placeholder="MIN">
<input id="j2y_center" placeholder="CENTER">
<input id="j2y_max" placeholder="MAX">

<button onclick="saveCalib()">
SAVE CALIB
</button>

</div>





<div class="card">

<h3>JOYSTICK GPIO</h3>

<div class="label">JOY1 X GPIO</div>
<input id="j1x_pin">

<div class="label">JOY1 Y GPIO</div>
<input id="j1y_pin">

<div class="label">JOY2 X GPIO</div>
<input id="j2x_pin">

<div class="label">JOY2 Y GPIO</div>
<input id="j2y_pin">

<button onclick="savePins()">
SAVE GPIO
</button>



<div class="card">

<h3>LIVE DATA</h3>

<div class="live">

<div class="box">
CH1
<div class="val" id="ch1">0</div>
</div>

<div class="box">
CH2
<div class="val" id="ch2">0</div>
</div>

<div class="box">
CH3
<div class="val" id="ch3">0</div>
</div>

<div class="box">
CH4
<div class="val" id="ch4">0</div>
</div>

</div>

</div>



</div>




</div>

<script>

function $(id){
  return document.getElementById(id);
}

fetch("/get")
.then(r=>r.json())
.then(d=>{

  for(let k in d){

    let el=$(k);

    if(el){
      el.value=d[k];
    }
  }

  $("esp_mac").innerText=d.esp_mac;
});

function saveWifi(){

  fetch("/setwifi?ssid="+
  ssid.value+
  "&pass="+
  pass.value);
}

function saveMac(){

  fetch("/setmac?mac="+
  receiver_mac.value);
}

function savePins(){

  fetch("/setpins?"+
  "j1x_pin="+j1x_pin.value+
  "&j1y_pin="+j1y_pin.value+
  "&j2x_pin="+j2x_pin.value+
  "&j2y_pin="+j2y_pin.value);
}

function saveCalib(){

  let url="/setcalib?"+

  "j1x_min="+j1x_min.value+
  "&j1x_center="+j1x_center.value+
  "&j1x_max="+j1x_max.value+

  "&j1y_min="+j1y_min.value+
  "&j1y_center="+j1y_center.value+
  "&j1y_max="+j1y_max.value+

  "&j2x_min="+j2x_min.value+
  "&j2x_center="+j2x_center.value+
  "&j2x_max="+j2x_max.value+

  "&j2y_min="+j2y_min.value+
  "&j2y_center="+j2y_center.value+
  "&j2y_max="+j2y_max.value;

  fetch(url);
}

let ws=new WebSocket("ws://"+location.hostname+":81/");

ws.onmessage=function(e){

  let d=JSON.parse(e.data);

  ch1.innerText=d.ch1;
  ch2.innerText=d.ch2;
  ch3.innerText=d.ch3;
  ch4.innerText=d.ch4;
};

</script>

</body>
</html>

)rawliteral";

// ================= SETUP =================
void setup(){

  Serial.begin(115200);

  loadConfig();

  analogReadResolution(12);

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(ssid.c_str(), pass.c_str());

  esp_now_init();

  addPeer();

  server.on("/", [](){
    server.send_P(200,"text/html",HTML);
  });

  server.on("/get", handleGet);

  server.on("/setwifi", handleSetWifi);

  server.on("/setmac", handleSetMac);

  server.on("/setpins", handleSetPins);

  server.on("/setcalib", handleSetCalib);

  server.begin();

  ws.begin();

  Serial.println();
  Serial.println("ESP32 READY");

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("ESP MAC: ");
  Serial.println(WiFi.macAddress());
}

// ================= LOOP =================
void loop(){

  server.handleClient();

  ws.loop();

  data.ch[0]=stable(
    analogRead(J1_X),
    j1x_min,
    j1x_center,
    j1x_max,
    l1
  );

  data.ch[1]=stable(
    analogRead(J1_Y),
    j1y_min,
    j1y_center,
    j1y_max,
    l2
  );

  data.ch[2]=stable(
    analogRead(J2_X),
    j2x_min,
    j2x_center,
    j2x_max,
    l3
  );

  data.ch[3]=stable(
    analogRead(J2_Y),
    j2y_min,
    j2y_center,
    j2y_max,
    l4
  );

  String wsData="{";

  wsData+="\"ch1\":"+String(data.ch[0])+",";
  wsData+="\"ch2\":"+String(data.ch[1])+",";
  wsData+="\"ch3\":"+String(data.ch[2])+",";
  wsData+="\"ch4\":"+String(data.ch[3]);

  wsData+="}";

  ws.broadcastTXT(wsData);

  esp_now_send(
    receiverMAC,
    (uint8_t*)&data,
    sizeof(data)
  );

  delay(20);
}