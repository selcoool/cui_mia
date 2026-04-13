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

PID pidR, pidP, pidY;

// ================= PWM =================
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

  gz_bias=gz_o/131.0;
}

// ================= PID =================
float compute(PID &p,float set,float in,float dt){
  float e=set-in;

  p.i += e*dt;
  p.i = constrain(p.i,-25,25);

  float d=(e-p.last)/dt;
  p.last=e;

  return p.kp*e + p.ki*p.i + p.kd*d;
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

// ================= ESP-NOW =================
void onRecv(const uint8_t *mac,const uint8_t *data,int len){
  memcpy(&rx,data,sizeof(rx));
  lastRX=millis();
}

// ================= WEB =================
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>FC PID</title>
</head>
<body style="background:#111;color:white;font-family:Arial">

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

<script>
let ws=new WebSocket("ws://"+location.hostname+":81/");

ws.onmessage=(e)=>{
  let d=JSON.parse(e.data);

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

    window.init=1;
  }
};

function send(){
  ws.send(
    "PID|"+
    r_kp.value+","+r_ki.value+","+r_kd.value+"|"+
    p_kp.value+","+p_ki.value+","+p_kd.value+"|"+
    y_kp.value+","+y_ki.value+","+y_kd.value
  );
}
</script>

</body>
</html>
)rawliteral";

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
  }
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  Wire.begin();
  Wire.setClock(400000);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  motorInit();
  mpuCalib();

  loadPID();   // 🔥 LOAD PID

  WiFi.softAP("FC-DRONE","12345678");

  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  server.on("/",[](void){
    server.send_P(200,"text/html",HTML);
  });
  server.begin();

  ws.begin();
  ws.onEvent(onWs);
}

// ================= LOOP =================
void loop(){
  server.handleClient();
  ws.loop();

  float gx=(read16(0x43)-gx_o)/131.0;
  float gy=(read16(0x45)-gy_o)/131.0;
  float gz=(read16(0x47)-gz_o)/131.0;

  roll += gx*0.01;
  pitch += gy*0.01;
  yaw += gz*0.01;

  String json="{";
  json+="\"r_kp\":"+String(pidR.kp)+",";
  json+="\"r_ki\":"+String(pidR.ki)+",";
  json+="\"r_kd\":"+String(pidR.kd)+",";

  json+="\"p_kp\":"+String(pidP.kp)+",";
  json+="\"p_ki\":"+String(pidP.ki)+",";
  json+="\"p_kd\":"+String(pidP.kd)+",";

  json+="\"y_kp\":"+String(pidY.kp)+",";
  json+="\"y_ki\":"+String(pidY.ki)+",";
  json+="\"y_kd\":"+String(pidY.kd);

  json+="}";

  ws.broadcastTXT(json);
}