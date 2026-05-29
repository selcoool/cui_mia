

















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



// #define CHANNELS 8

// volatile uint32_t lastPacketTime = 0;

// typedef struct {
//   uint16_t ch[CHANNELS];
// } PPMData;

// PPMData receivedData;

// // ===== callback nhận =====
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&receivedData, incomingData, sizeof(receivedData));

//   for (int i = 0; i < CHANNELS; i++) {

//     receivedData.ch[i];
//   // Serial.print("CH");
//   // Serial.print(i + 1);
//   // Serial.print(":");
//   // Serial.print(receivedData.ch[i]);
//   // Serial.print(" ");
// }
// Serial.println();

//  lastPacketTime = millis();
// }


// float RateRoll, RatePitch, RateYaw;
// float AccX, AccY, AccZ;
// float AngleRoll, AnglePitch;
// // float LoopTimer;
// void gyro_signals(void) {
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1A);
//   Wire.write(0x05);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1C);
//   Wire.write(0x10);
//   Wire.endTransmission();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x3B);
//   Wire.endTransmission(); 
//   Wire.requestFrom(0x68,6);
//   int16_t AccXLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccYLSB = Wire.read() << 8 | Wire.read();
//   int16_t AccZLSB = Wire.read() << 8 | Wire.read();
//   Wire.beginTransmission(0x68);
//   Wire.write(0x1B); 
//   Wire.write(0x8);
//   Wire.endTransmission();                                                   
//   Wire.beginTransmission(0x68);
//   Wire.write(0x43);
//   Wire.endTransmission();
//   Wire.requestFrom(0x68,6);
//   int16_t GyroX=Wire.read()<<8 | Wire.read();
//   int16_t GyroY=Wire.read()<<8 | Wire.read();
//   int16_t GyroZ=Wire.read()<<8 | Wire.read();
//   RateRoll=(float)GyroX/65.5;
//   RatePitch=(float)GyroY/65.5;
//   RateYaw=(float)GyroZ/65.5;
//   AccX=(float)AccXLSB/4096;
//   AccY=(float)AccYLSB/4096;
//   AccZ=(float)AccZLSB/4096;
//   AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
//   AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);


//     //  Serial.print("Roll: ");
//     //  Serial.print(RateRoll); Serial.print(" ");
//     //  Serial.print("Pitch: ");
//     //  Serial.print(RatePitch); Serial.print(" ");
//     // Serial.print("Yaw: ");
//     //  Serial.print(RateYaw); Serial.print(" ");


//     //   Serial.print("AccX: ");
//     //  Serial.print(AccX); Serial.print(" ");
//     //  Serial.print("AccY: ");
//     //  Serial.print(AccY); Serial.print(" ");
//     //  Serial.print("AccZ: ");
//     //  Serial.print(AccZ); Serial.print(" ");
  

//     //     Serial.print("AngleRoll: ");
//     //  Serial.print(AngleRoll); Serial.print(" ");
//     //  Serial.print("AnglePitch: ");
//     //  Serial.print(AnglePitch); Serial.print(" ");
//     //  Serial.println(" ");
// }


// void failsafe() {

//   if (millis() - lastPacketTime > 500) {

//     receivedData.ch[0] = 1000;
//     receivedData.ch[1] = 1500;
//     receivedData.ch[2] = 1500;
//     receivedData.ch[3] = 1500;
//   }
// }

// // =====================================================
// // ================= PID ================================
// // =====================================================

// float ErrorRoll, ErrorPitch, ErrorYaw;

// float InputRoll, InputPitch, InputYaw;
// float Throttle;

// float PR = 0.0;
// float IR = 0.0;
// float DR = 0.0;

// float PP = 0.0;
// float IP = 0.0;
// float DP = 0.0;

// float PY = 0.0;
// float IY = 0.0;
// float DY = 0.0;

// float PrevErrR = 0;
// float PrevErrP = 0;
// float PrevErrY = 0;

// float PrevIntR = 0;
// float PrevIntP = 0;
// float PrevIntY = 0;

// float PIDOut[3];

// void pidEquation(
//   float error,
//   float P,
//   float I,
//   float D,
//   float &prevErr,
//   float &prevInt
// ) {

//   float Pterm = P * error;

//   float Iterm =
//     prevInt +
//     I * (error + prevErr) * 0.004 * 0.5;

//   Iterm = constrain(Iterm, -400, 400);

//   float Dterm =
//     D * (error - prevErr) / 0.004;

//   float output = Pterm + Iterm + Dterm;

//   output = constrain(output, -400, 400);

//   prevErr = error;
//   prevInt = Iterm;

//   PIDOut[0] = output;
//   PIDOut[1] = error;
//   PIDOut[2] = Iterm;
// }


// void motorMix() {

//   float DesiredRoll =
//     0.10 * (receivedData.ch[3] - 1500);

//   float DesiredPitch =
//     0.10 * (receivedData.ch[2] - 1500);

//   float DesiredYaw =
//     0.15 * (receivedData.ch[1] - 1500);

//   ErrorRoll =
//     DesiredRoll - RateRoll;

//   ErrorPitch =
//     DesiredPitch - RatePitch;

//   ErrorYaw =
//     DesiredYaw - RateYaw;

//   // ROLL
//   pidEquation(
//     ErrorRoll,
//     PR,
//     IR,
//     DR,
//     PrevErrR,
//     PrevIntR
//   );

//   InputRoll = PIDOut[0];

//   // PITCH
//   pidEquation(
//     ErrorPitch,
//     PP,
//     IP,
//     DP,
//     PrevErrP,
//     PrevIntP
//   );

//   InputPitch = PIDOut[0];

//   // YAW
//   pidEquation(
//     ErrorYaw,
//     PY,
//     IY,
//     DY,
//     PrevErrY,
//     PrevIntY
//   );

//   InputYaw = PIDOut[0];

//   Throttle = receivedData.ch[0];

//   // X QUADCOPTER MIX
//   int m1 =
//     Throttle
//     - InputRoll
//     - InputPitch
//     - InputYaw;

//   int m2 =
//     Throttle
//     + InputRoll
//     - InputPitch
//     + InputYaw;

//   int m3 =
//     Throttle
//     + InputRoll
//     + InputPitch
//     - InputYaw;

//   int m4 =
//     Throttle
//     - InputRoll
//     + InputPitch
//     + InputYaw;

//   // m1 = constrain(m1, 1000, 2000);
//   // m2 = constrain(m2, 1000, 2000);
//   // m3 = constrain(m3, 1000, 2000);
//   // m4 = constrain(m4, 1000, 2000);

//   // constrain trước
// m1 = constrain(m1, 1000, 2000);
// m2 = constrain(m2, 1000, 2000);
// m3 = constrain(m3, 1000, 2000);
// m4 = constrain(m4, 1000, 2000);

// // map sang PWM 0-255
// m1 = map(m1, 1000, 2000, 0, 255);
// m2 = map(m2, 1000, 2000, 0, 255);
// m3 = map(m3, 1000, 2000, 0, 255);
// m4 = map(m4, 1000, 2000, 0, 255);

// // constrain lại lần cuối
// m1 = constrain(m1, 0, 255);
// m2 = constrain(m2, 0, 255);
// m3 = constrain(m3, 0, 255);
// m4 = constrain(m4, 0, 255);



//   // writeMotor(M1_CH, m1);
//   // writeMotor(M2_CH, m2);
//   // writeMotor(M3_CH, m3);
//   // writeMotor(M4_CH, m4);


   
//  String json="{";

// json+="\"x\":"+String(RateRoll)+",";
// json+="\"y\":"+String(RatePitch)+",";
// json+="\"z\":"+String(RateYaw)+",";

// json+="\"ch0\":"+String(receivedData.ch[0])+",";
// json+="\"ch1\":"+String(receivedData.ch[1])+",";
// json+="\"ch2\":"+String(receivedData.ch[2])+",";
// json+="\"ch3\":"+String(receivedData.ch[3])+",";
// json+="\"m1\":"+String(m1)+",";
// json+="\"m2\":"+String(m2)+",";
// json+="\"m3\":"+String(m3)+",";
// json+="\"m4\":"+String(m4);


// json+="}";
//   ws.broadcastTXT(json);


//   // DEBUG
//   static uint32_t lastPrint = 0;

//   if (millis() - lastPrint > 100) {

//     lastPrint = millis();

//     // Serial.println("---------------");

//     //   Serial.print(" THR: ");
//     // Serial.print(receivedData.ch[0]);

//     // Serial.print(" YAW: ");
//     // Serial.print(receivedData.ch[1]);

//     // Serial.print(" ROLL: ");
//     // Serial.print(receivedData.ch[3]);

//     // Serial.print(" PITCH: ");
//     // Serial.println(receivedData.ch[2]);

  

//     // Serial.print("GYRO ROLL: ");
//     // Serial.print(RateRoll);

//     // Serial.print(" PITCH: ");
//     // Serial.print(RatePitch);

//     // Serial.print(" YAW: ");
//     // Serial.println(RateYaw);

//     Serial.print("M1: ");
//     Serial.print(m1);

//     Serial.print(" M2: ");
//     Serial.print(m2);

//     Serial.print(" M3: ");
//     Serial.print(m3);

//     Serial.print(" M4: ");
//     Serial.println(m4);
//   }
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
// .row {
//   display: flex;
//   gap: 20px;
//   flex-wrap: wrap;
//   justify-content: space-evenly;
// }

// .bar{height:10px;background:#333;border-radius:5px;overflow:hidden;margin:4px 0;}
// .fill{height:100%;background:#00ffcc;transition:width .15s;}
// .motor .fill{background:#ff3b3b;}

// #warn{
//   text-align:center;
//   font-weight:bold;
//   font-size:20px;
//   margin:10px;
// }

// // .warn{color:red;text-align:center;display:none;font-weight:bold;}
// </style>
// </head>

// <body>

// <h2>🚁 FC DRONE CONNECTED</h2>
// <div id="warn" ></div>

// <div class="grid">

// <div class="card row">

//   <div>
//     <h3>ATTITUDE</h3>
//     Roll: <span id="r"></span><br>
//     Pitch: <span id="p"></span><br>
//     Yaw: <span id="y"></span><br>
//     <div class="box">ALT: <span id="alt" class="val">0</span> (m)<br></div>
//   </div>

//   <div>
//     <h3>DEVICE</h3>
//     Mac: <span id="mac"></span><br>
//     Remote Mac: <span id="rmac"></span><br>
//   </div>

//   <div>
//     <h3>I2C CONFIG</h3>
//     SDA GPIO <input id="sda"><br>
//     SCL GPIO <input id="scl"><br>
//     MPU ADDR <input id="addr"><br>
//     <button onclick="saveI2C()">SAVE I2C</button> <button onclick="reset_memmory()">Reset Memmory</button>
//   </div>

// </div>


// <div class="card">
// <div>
// <h3>TX</h3>
// CH0 <div class="bar"><div id="ch0" class="fill"></div></div>
// CH1 <div class="bar"><div id="ch1" class="fill"></div></div>
// CH2 <div class="bar"><div id="ch2" class="fill"></div></div>
// CH3 <div class="bar"><div id="ch3" class="fill"></div></div>

// </div>

// <div class="row">

//    <div class="card">
//     <h3>PULSE AND PWM</h3>
//     PULSE MIN <input id="pulse_min"><br>
//     PULSE MAX <input id="pulse_max"><br>
//     PWM MIN <input id="pwm_min"><br>
//     PWM MAX <input id="pwm_max"><br>
//       <button onclick="savePusle_Pwm()">APPLY</button>
//   </div>

// </div>


// </div>


// <div class="card">
// <h2>PID CONTROL</h2>

// ROLL<br>
// KP <input id="r_kp" type="number" step="0.001"><br>
// KI <input id="r_ki" type="number" step="0.001"><br>
// KD <input id="r_kd" type="number" step="0.001"><br><br>

// PITCH<br>
// KP <input id="p_kp" type="number" step="0.001"><br>
// KI <input id="p_ki" type="number" step="0.001"><br>
// KD <input id="p_kd" type="number" step="0.001"><br><br>

// YAW<br>
// KP <input id="y_kp" type="number" step="0.001"><br>
// KI <input id="y_ki" type="number" step="0.001"><br>
// KD <input id="y_kd" type="number" step="0.001"><br><br>

// <button onclick="send()">SAVE PID</button>

// </div>

// <div class="card">
// <div>
// <h3>MOTORS</h3>
// M1 <div class="bar motor"><div id="m1" class="fill"></div></div>
// M2 <div class="bar motor"><div id="m2" class="fill"></div></div>
// M3 <div class="bar motor"><div id="m3" class="fill"></div></div>
// M4 <div class="bar motor"><div id="m4" class="fill"></div></div>
// </div>

// <div>


// <div class="row">

//   <div class="card">
//     <h3>PINS</h3>
//     M1 GPIO <input id="pm1"><br>
//     M2 GPIO <input id="pm2"><br>
//     M3 GPIO <input id="pm3"><br>
//     M4 GPIO <input id="pm4"><br>
//     <button onclick="savePin()">APPLY</button>
//   </div>

//   <div class="card">
//     <h3>TRIM</h3>
//     TRIM1 <input id="trim1"><br>
//      TRIM2 <input id="trim2"><br>
//      TRIM3 <input id="trim3"><br>
//      TRIM4 <input id="trim4"><br>
//       <button onclick="saveTrim()">APPLY</button>
//   </div>


//     <div class="card">
//     <h3>MOTOR POSITION</h3>
//     POSITION M1 <input id="position_m1"><br>
//    POSITION M2 <input id="position_m2"><br>
//     POSITION M3 <input id="position_m3"><br>
//    POSITION M4 <input id="position_m4"><br>
//     <button onclick="saveMotorPosition()">APPLY</button>
//    </div>



// </div>


// <div>



// </div>

// </div>

// <script>
// const sda = document.getElementById("sda");
// const scl = document.getElementById("scl");
// const addr = document.getElementById("addr");


// const x = document.getElementById("x");
// const y = document.getElementById("y");
// const z = document.getElementById("z");
// const alt = document.getElementById("alt");
// const mac = document.getElementById("mac");
// const rmac = document.getElementById("rmac");
// const warn = document.getElementById("warn");


// const r_kp = document.getElementById("r_kp");
// const r_ki = document.getElementById("r_ki");
// const r_kd = document.getElementById("r_kd");

// const p_kp = document.getElementById("p_kp");
// const p_ki = document.getElementById("p_ki");
// const p_kd = document.getElementById("p_kd");


// const y_kp = document.getElementById("y_kp");
// const y_ki = document.getElementById("y_ki");
// const y_kd = document.getElementById("y_kd");





// const pm1 = document.getElementById("pm1");
// const pm2 = document.getElementById("pm2");
// const pm3 = document.getElementById("pm3");
// const pm4 = document.getElementById("pm4");


// const position_m1 = document.getElementById("position_m1");
// const position_m2 = document.getElementById("position_m2");
// const position_m3 = document.getElementById("position_m3");
// const position_m4 = document.getElementById("position_m4");


// const trim1 = document.getElementById("trim1");
// const trim2 = document.getElementById("trim2");
// const trim3 = document.getElementById("trim3");
// const trim4 = document.getElementById("trim4");

// const pulse_min = document.getElementById("pulse_min");
// const pulse_max = document.getElementById("pulse_max");
// const pwm_min = document.getElementById("pwm_min");
// const pwm_max = document.getElementById("pwm_max");


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

// try {
//     let d = JSON.parse(e.data);

//     console.log(d);

//     // ===== ATTITUDE =====
//     x.innerText = Number(d.x || 0).toFixed(2);
//     y.innerText = Number(d.y || 0).toFixed(2);
//     z.innerText = Number(d.z || 0).toFixed(2);
//      alt.innerText=Number(d.alt || 0).toFixed(2);
    

//     mac.innerText = d.mac || "-";
//     rmac.innerText = d.rmac || "-";



//      if(!window.init){


//     sda.value =d.sda;
//     scl.value = d.scl;
//     addr.value = d.addr;



//     r_kp.value=Number(d.r_kp || 0.00).toFixed(2);
//     r_ki.value=Number(d.r_ki || 0.00).toFixed(2);
//     r_kd.value=Number(d.r_kd || 0.00).toFixed(2);

//     p_kp.value=Number(d.p_kp || 0.00).toFixed(2);
//     p_ki.value=Number(d.p_ki || 0.00).toFixed(2);
//     p_kd.value=Number(d.p_kd|| 0.00).toFixed(2);

    
//     y_kp.value=Number(d.y_kp || 0.00).toFixed(2);
//     y_ki.value=Number(d.y_ki || 0.00).toFixed(2);
//     y_kd.value= Number(d.y_kd || 0.00).toFixed(2);


//     pm1.value=d.pm1;
//     pm2.value=d.pm2;
//     pm3.value=d.pm3;
//     pm4.value=d.pm4;

//     position_m1.value=d.position_m1;
//     position_m2.value=d.position_m2;
//     position_m3.value=d.position_m3;
//     position_m4.value=d.position_m4;

//     trim1.value =d.trim1;
//     trim2.value = d.trim2;
//     trim3.value =d.trim3;
//     trim4.value = d.trim4;

//     pulse_min.value =d.pulse_min;
//     pulse_max.value = d.pulse_max;
//     pwm_min.value =d.pwm_min;
//     pwm_max.value = d.pwm_max;


//     window.init=1;
//   }

//     // ===== FAILSAFE =====

//           if(d.fail){
//         warn.innerText = "⚠ FAILSAFE";
//         warn.style.color = "red";
//       }
//       else if(d.arm){
//         warn.innerText = "ARMED";
//         warn.style.color = "lime";
//       }
//       else{
//         warn.innerText = "UNARMED";
//         warn.style.color = "orange";
//       }
//     //  warn.innerText=d.fail ? "UNARMED" : "ARMED";
//     // // warn.style.display = d.fail ? "block" : "none";

//     // ===== TX =====
//     bar("ch0", smooth(0,(d.ch0-1000)/10));
//     bar("ch1", smooth(1,(d.ch1-1000)/10));
//     bar("ch2", smooth(2,(d.ch2-1000)/10));
//     bar("ch3", smooth(3,(d.ch3-1000)/10));
    

//     // ===== MOTORS =====
//     bar("m1", d.m1*100/255);
//     bar("m2", d.m2*100/255);
//     bar("m3", d.m3*100/255);
//     bar("m4", d.m4*100/255);

//   } catch (err) {
//     console.log("WS error:", err);
//   }
// };

// // ===== SEND I2C =====
// function saveI2C(){
//   ws.send("I2C|" + sda.value + "," + scl.value + "," + addr.value);
// }

// // ===== SEND PID =====
// function send(){
//   ws.send(
//     "PID|" +
//     r_kp.value + "," + r_ki.value + "," + r_kd.value + "|" +
//     p_kp.value + "," + p_ki.value + "," + p_kd.value + "|" +
//     y_kp.value + "," + y_ki.value + "," + y_kd.value
//   );
// }

// // ===== SEND PULSE AND PWM =====
// function savePusle_Pwm(){
//   ws.send("PULSE_PWM|"+pulse_min.value+","+pulse_max.value+","+pwm_min.value+","+pwm_max.value);
// }


// // ===== SEND MOTOR POSOTION =====
// function saveMotorPosition(){
//   ws.send("MOTOR_POSITION|"+position_m1.value+","+position_m2.value+","+position_m3.value+","+position_m4.value);
// }


// // ===== SEND PIN =====
// function saveTrim(){
//   ws.send("TRIM|"+trim1.value+","+trim2.value+","+trim3.value+","+trim4.value);
// }


// // ===== SEND PIN =====
// function savePin(){
//   ws.send("PIN|"+pm1.value+","+pm2.value+","+pm3.value+","+pm4.value);
// }


// function reset_memmory(){
//    ws.send("MEMMORY");
// }

// </script>

// </body>
// </html>
// )rawliteral";


// // ================= WS =================
// void onWs(uint8_t num,WStype_t type,uint8_t *payload,size_t len){

//   if(type==WStype_TEXT){

//     String msg=(char*)payload;





    

//   }
// }

// void setup() {
//   Serial.begin(115200);

//   delay(1000);

//   // I2C
//   Wire.begin();
//   Wire.setClock(400000);

//   // WAKE MPU6050
//   Wire.beginTransmission(0x68);
//   Wire.write(0x6B);
//   Wire.write(0x00);
//   Wire.endTransmission();

//   delay(100);


//   // WiFi.mode(WIFI_STA);

//   WiFi.mode(WIFI_AP_STA);
//   WiFi.softAP("FC-DRONE","12345678");


//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP-NOW init failed");
//     return;
//   }

//   esp_now_register_recv_cb(OnDataRecv);

//    server.on("/",[](void){
//     server.send_P(200,"text/html",HTML);
//   });

//   //   server.on("/reset_saving",[](void){
//   //   server.send_P(200,"text/html",HTML1);
//   // });
//   server.begin();

//   ws.begin();
//   ws.onEvent(onWs);

// }

// void loop() {

//   server.handleClient();
//   ws.loop();

//   // Serial.print(receivedData.ch[0]);

//     gyro_signals();
//     failsafe();
//     motorMix();
//   // Serial.print("Acceleration X [g]= ");
//   // Serial.print(AccX);
//   // Serial.print(" Acceleration Y [g]= ");
//   // Serial.print(AccY);
//   // Serial.print(" Acceleration Z [g]= ");
//   // Serial.println(AccZ);
//   // delay(50);
// }































// // include <Arduino.h>
// // #include <WiFi.h>
// // #include <esp_now.h>
// // #include <Wire.h>

// // // =====================================================
// // // ================= ESP NOW RX =========================
// // // =====================================================

// // #define CHANNELS 8

// // typedef struct {
// //   uint16_t ch[CHANNELS];
// // } PPMData;

// // PPMData receivedData;

// // // =====================================================
// // // ================= MPU6050 ============================
// // // =====================================================

// // float RateRoll, RatePitch, RateYaw;
// // float AccX, AccY, AccZ;
// // float AngleRoll, AnglePitch;

// // // =====================================================
// // // ================= MOTOR PWM ==========================
// // // =====================================================

// // // ESC pins
// // #define M1_PIN 25
// // #define M2_PIN 26
// // #define M3_PIN 27
// // #define M4_PIN 14

// // // LEDC channels
// // #define M1_CH 0
// // #define M2_CH 1
// // #define M3_CH 2
// // #define M4_CH 3

// // // PWM config
// // #define PWM_FREQ 50
// // #define PWM_RESOLUTION 16

// // // 50Hz = 20ms
// // // 1000us -> ~3276
// // // 2000us -> ~6553

// // uint32_t usToDuty(uint16_t us) {
// //   return map(us, 1000, 2000, 3276, 6553);
// // }

// // void writeMotor(uint8_t channel, uint16_t us) {
// //   us = constrain(us, 1000, 2000);
// //   ledcWrite(channel, usToDuty(us));
// // }

// // // =====================================================
// // // ================= PID ================================
// // // =====================================================

// // float ErrorRoll, ErrorPitch, ErrorYaw;

// // float InputRoll, InputPitch, InputYaw;
// // float Throttle;

// // float PR = 0.6;
// // float IR = 3.5;
// // float DR = 0.03;

// // float PP = 0.6;
// // float IP = 3.5;
// // float DP = 0.03;

// // float PY = 2.0;
// // float IY = 12.0;
// // float DY = 0.0;

// // float PrevErrR = 0;
// // float PrevErrP = 0;
// // float PrevErrY = 0;

// // float PrevIntR = 0;
// // float PrevIntP = 0;
// // float PrevIntY = 0;

// // float PIDOut[3];

// // void pidEquation(
// //   float error,
// //   float P,
// //   float I,
// //   float D,
// //   float &prevErr,
// //   float &prevInt
// // ) {

// //   float Pterm = P * error;

// //   float Iterm =
// //     prevInt +
// //     I * (error + prevErr) * 0.004 * 0.5;

// //   Iterm = constrain(Iterm, -400, 400);

// //   float Dterm =
// //     D * (error - prevErr) / 0.004;

// //   float output = Pterm + Iterm + Dterm;

// //   output = constrain(output, -400, 400);

// //   prevErr = error;
// //   prevInt = Iterm;

// //   PIDOut[0] = output;
// //   PIDOut[1] = error;
// //   PIDOut[2] = Iterm;
// // }

// // // =====================================================
// // // ================= ESP NOW CALLBACK ===================
// // // =====================================================

// // volatile uint32_t lastPacketTime = 0;


// // // void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
// // //   memcpy(&receivedData, incomingData, sizeof(receivedData));

// // //   for (int i = 0; i < CHANNELS; i++) {

// // //     receivedData.ch[i];

// // //   }
// // // Serial.println();
// // // }


// // void OnDataRecv(
// //   const uint8_t *mac,
// //   const uint8_t *incomingData,
// //   int len
// // ) {

// //   if (len != sizeof(PPMData)) {
// //     Serial.println("INVALID PACKET");
// //     return;
// //   }

// //   memcpy(&receivedData, incomingData, sizeof(receivedData));

// //   lastPacketTime = millis();
// // }

// // // =====================================================
// // // ================= MPU6050 READ =======================
// // // =====================================================

// // void gyro_signals() {

// //   // ACCEL CONFIG
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x1C);
// //   Wire.write(0x10);
// //   Wire.endTransmission();

// //   // READ ACC
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x3B);
// //   Wire.endTransmission();

// //   Wire.requestFrom((uint8_t)0x68, (uint8_t)6, true);

// //   int16_t AccXLSB =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t AccYLSB =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t AccZLSB =
// //     Wire.read() << 8 | Wire.read();

// //   // GYRO CONFIG
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x1B);
// //   Wire.write(0x08);
// //   Wire.endTransmission();

// //   // READ GYRO
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x43);
// //   Wire.endTransmission();

// //   Wire.requestFrom((uint8_t)0x68, (uint8_t)6, true);

// //   int16_t GyroX =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t GyroY =
// //     Wire.read() << 8 | Wire.read();

// //   int16_t GyroZ =
// //     Wire.read() << 8 | Wire.read();

// //   // SCALE
// //   RateRoll  = (float)GyroX / 65.5;
// //   RatePitch = (float)GyroY / 65.5;
// //   RateYaw   = (float)GyroZ / 65.5;

// //   AccX = (float)AccXLSB / 4096;
// //   AccY = (float)AccYLSB / 4096;
// //   AccZ = (float)AccZLSB / 4096;

// //   AngleRoll =
// //     atan(
// //       AccY /
// //       sqrt(AccX * AccX + AccZ * AccZ)
// //     ) * 57.2958;

// //   AnglePitch =
// //     -atan(
// //       AccX /
// //       sqrt(AccY * AccY + AccZ * AccZ)
// //     ) * 57.2958;
// // }

// // // =====================================================
// // // ================= FAILSAFE ===========================
// // // =====================================================

// // // void failsafe() {

// // //   if (millis() - lastPacketTime > 500) {

// // //     receivedData[0] = 1500;
// // //     receivedData[1] = 1500;
// // //     receivedData[2] = 1000;
// // //     receivedData[3] = 1500;
// // //   }
// // // }

// // // =====================================================
// // // ================= MOTOR MIX ==========================
// // // =====================================================

// // void motorMix() {

// //   float DesiredRoll =
// //     0.10 * (receivedData.ch[0] - 1500);

// //   float DesiredPitch =
// //     0.10 * (receivedData.ch[1] - 1500);

// //   float DesiredYaw =
// //     0.15 * (receivedData.ch[3] - 1500);

// //   ErrorRoll =
// //     DesiredRoll - RateRoll;

// //   ErrorPitch =
// //     DesiredPitch - RatePitch;

// //   ErrorYaw =
// //     DesiredYaw - RateYaw;

// //   // ROLL
// //   pidEquation(
// //     ErrorRoll,
// //     PR,
// //     IR,
// //     DR,
// //     PrevErrR,
// //     PrevIntR
// //   );

// //   InputRoll = PIDOut[0];

// //   // PITCH
// //   pidEquation(
// //     ErrorPitch,
// //     PP,
// //     IP,
// //     DP,
// //     PrevErrP,
// //     PrevIntP
// //   );

// //   InputPitch = PIDOut[0];

// //   // YAW
// //   pidEquation(
// //     ErrorYaw,
// //     PY,
// //     IY,
// //     DY,
// //     PrevErrY,
// //     PrevIntY
// //   );

// //   InputYaw = PIDOut[0];

// //   Throttle = receivedData.ch[2];

// //   // X QUADCOPTER MIX
// //   int m1 =
// //     Throttle
// //     - InputRoll
// //     - InputPitch
// //     - InputYaw;

// //   int m2 =
// //     Throttle
// //     + InputRoll
// //     - InputPitch
// //     + InputYaw;

// //   int m3 =
// //     Throttle
// //     + InputRoll
// //     + InputPitch
// //     - InputYaw;

// //   int m4 =
// //     Throttle
// //     - InputRoll
// //     + InputPitch
// //     + InputYaw;

// //   m1 = constrain(m1, 1000, 2000);
// //   m2 = constrain(m2, 1000, 2000);
// //   m3 = constrain(m3, 1000, 2000);
// //   m4 = constrain(m4, 1000, 2000);

// //   // writeMotor(M1_CH, m1);
// //   // writeMotor(M2_CH, m2);
// //   // writeMotor(M3_CH, m3);
// //   // writeMotor(M4_CH, m4);

// //   // DEBUG
// //   static uint32_t lastPrint = 0;

// //   if (millis() - lastPrint > 100) {

// //     lastPrint = millis();

// //     Serial.println("---------------");

// //     Serial.print("CH1: ");
// //     Serial.print(receivedData.ch[0]);

// //     Serial.print(" CH2: ");
// //     Serial.print(receivedData.ch[1]);

// //     Serial.print(" THR: ");
// //     Serial.print(receivedData.ch[2]);

// //     Serial.print(" YAW: ");
// //     Serial.println(receivedData.ch[3]);

// //     Serial.print("GYRO ROLL: ");
// //     Serial.print(RateRoll);

// //     Serial.print(" PITCH: ");
// //     Serial.print(RatePitch);

// //     Serial.print(" YAW: ");
// //     Serial.println(RateYaw);

// //     Serial.print("M1: ");
// //     Serial.print(m1);

// //     Serial.print(" M2: ");
// //     Serial.print(m2);

// //     Serial.print(" M3: ");
// //     Serial.print(m3);

// //     Serial.print(" M4: ");
// //     Serial.println(m4);
// //   }
// // }

// // // =====================================================
// // // ================= SETUP ==============================
// // // =====================================================

// // void setup() {

// //   Serial.begin(115200);

// //   delay(1000);

// //   // I2C
// //   Wire.begin();
// //   Wire.setClock(400000);

// //   // WAKE MPU6050
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x6B);
// //   Wire.write(0x00);
// //   Wire.endTransmission();

// //   delay(100);

// //   // WIFI
// //   WiFi.mode(WIFI_STA);

// //   Serial.print("RX MAC: ");
// //   Serial.println(WiFi.macAddress());

// //   // ESP NOW
// //   if (esp_now_init() != ESP_OK) {

// //     Serial.println("ESP NOW INIT FAILED");

// //     while (1);
// //   }

// //   esp_now_register_recv_cb(OnDataRecv);

// //   // PWM SETUP
// //   ledcSetup(M1_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M2_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M3_CH, PWM_FREQ, PWM_RESOLUTION);
// //   ledcSetup(M4_CH, PWM_FREQ, PWM_RESOLUTION);

// //   ledcAttachPin(M1_PIN, M1_CH);
// //   ledcAttachPin(M2_PIN, M2_CH);
// //   ledcAttachPin(M3_PIN, M3_CH);
// //   ledcAttachPin(M4_PIN, M4_CH);

// //   // ARM ESC
// //   writeMotor(M1_CH, 1000);
// //   writeMotor(M2_CH, 1000);
// //   writeMotor(M3_CH, 1000);
// //   writeMotor(M4_CH, 1000);

// //   Serial.println("DRONE RX READY");
// // }

// // // =====================================================
// // // ================= LOOP ===============================
// // // =====================================================

// // void loop() {

// //   gyro_signals();

// //   // failsafe();

// //   motorMix();

// //   delay(4);
// // }











// // #include <Arduino.h>
// // #include <WiFi.h>
// // #include <esp_now.h>
// // #include <Wire.h>

// // // ================= RECEIVER =================
// // #define MAX_CHANNELS 16

// // typedef struct {
// //   uint16_t ch[MAX_CHANNELS];
// // } PPMData;

// // PPMData receiverData;
// // float ReceiverValue[8] = {1500,1500,1000,1500,0,0,0,0};

// // uint32_t lastPacket = 0;
// // uint32_t lastPrint = 0;
// // bool systemReady = false;

// // // ================= ESP-NOW =================
// // void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

// //   // FIX: chống data lỗi gây crash / rác
// //   if (len != sizeof(PPMData)) return;

// //   memcpy(&receiverData, incomingData, sizeof(receiverData));
// //   lastPacket = millis();

// //   for (int i = 0; i < 8; i++) {
// //     ReceiverValue[i] = receiverData.ch[i];
// //   }
// // }

// // // // ================= GYRO (RAW MPU6050) =================
// // float RateRoll = 0, RatePitch = 0, RateYaw = 0;

// // void gyro_signals() {
// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x3B);
// //   Wire.endTransmission();
// //   Wire.requestFrom(0x68, 6);

// //   int16_t Ax = Wire.read() << 8 | Wire.read();
// //   int16_t Ay = Wire.read() << 8 | Wire.read();
// //   int16_t Az = Wire.read() << 8 | Wire.read();

// //   Wire.beginTransmission(0x68);
// //   Wire.write(0x43);
// //   Wire.endTransmission();
// //   Wire.requestFrom(0x68, 6);

// //   int16_t Gx = Wire.read() << 8 | Wire.read();
// //   int16_t Gy = Wire.read() << 8 | Wire.read();
// //   int16_t Gz = Wire.read() << 8 | Wire.read();

// //   RateRoll  = Gx / 65.5;
// //   RatePitch = Gy / 65.5;
// //   RateYaw   = Gz / 65.5;
// // }

// // // ================= PID =================
// // float ErrorRoll, ErrorPitch, ErrorYaw;
// // float InputRoll, InputPitch, InputYaw, InputThrottle;

// // float PR = 0.6, IR = 3.5, DR = 0.03;
// // float PP = 0.6, IP = 3.5, DP = 0.03;
// // float PY = 2.0, IY = 12,   DY = 0;

// // float PrevErrR=0, PrevErrP=0, PrevErrY=0;
// // float PrevIntR=0, PrevIntP=0, PrevIntY=0;

// // float PID[3];

// // void pid(float error, float P, float I, float D,
// //          float &prevErr, float &prevInt) {

// //   float Pterm = P * error;
// //   float Iterm = prevInt + I * (error + prevErr) * 0.004 / 2;
// //   float Dterm = D * (error - prevErr) / 0.004;

// //   Iterm = constrain(Iterm, -400, 400);

// //   float out = Pterm + Iterm + Dterm;
// //   out = constrain(out, -400, 400);

// //   prevErr = error;
// //   prevInt = Iterm;

// //   PID[0] = out;
// //   PID[1] = error;
// //   PID[2] = Iterm;
// // }

// // // ================= FAILSAFE =================
// // void failsafe() {
// //   if (millis() - lastPacket > 200) {
// //     ReceiverValue[0] = 1500;
// //     ReceiverValue[1] = 1500;
// //     ReceiverValue[2] = 1000;
// //     ReceiverValue[3] = 1500;
// //   }
// // }

// // // ================= MOTOR =================
// // void motor_write(float m1, float m2, float m3, float m4) {
// //   analogWrite(1, constrain(m1, 1000, 2000));
// //   analogWrite(2, constrain(m2, 1000, 2000));
// //   analogWrite(3, constrain(m3, 1000, 2000));
// //   analogWrite(4, constrain(m4, 1000, 2000));
// // }

// // // ================= ESP-NOW INIT =================
// // void espnow_init() {

// //   WiFi.mode(WIFI_STA);
// //   WiFi.disconnect(true, true);
// //   delay(200);

// //   if (esp_now_init() != ESP_OK) {
// //     Serial.println("ESP-NOW FAIL");
// //     return;
// //   }

// //   esp_now_register_recv_cb(onDataRecv);
// // }

// // // ================= SETUP =================
// // void setup() {

// //   Serial.begin(115200);
// //   delay(2000);
// //   Serial.println("\nBOOTING DRONE...");

// //   Wire.begin();
// //   Wire.setClock(400000);

// //   espnow_init();

// //   pinMode(5, OUTPUT);
// //   digitalWrite(5, HIGH);

// //   systemReady = true;

// //   Serial.println("DRONE READY ESP-NOW");
// // }

// // // ================= LOOP =================
// // void loop() {

// //   if (!systemReady) return;

// //   gyro_signals();
// //   failsafe();

// //   // ===== CONTROL =====
// //   float DesiredRoll  = 0.1 * (ReceiverValue[0] - 1500);
// //   float DesiredPitch = 0.1 * (ReceiverValue[1] - 1500);
// //   float DesiredYaw   = 0.15 * (ReceiverValue[3] - 1500);

// //   ErrorRoll  = DesiredRoll - RateRoll;
// //   ErrorPitch = DesiredPitch - RatePitch;
// //   ErrorYaw   = DesiredYaw - RateYaw;

// //   pid(ErrorRoll, PR, IR, DR, PrevErrR, PrevIntR);
// //   InputRoll = PID[0];

// //   pid(ErrorPitch, PP, IP, DP, PrevErrP, PrevIntP);
// //   InputPitch = PID[0];

// //   pid(ErrorYaw, PY, IY, DY, PrevErrY, PrevIntY);
// //   InputYaw = PID[0];

// //   InputThrottle = ReceiverValue[2];

// //   float m1 = InputThrottle - InputRoll - InputPitch - InputYaw;
// //   float m2 = InputThrottle + InputRoll - InputPitch + InputYaw;
// //   float m3 = InputThrottle + InputRoll + InputPitch - InputYaw;
// //   float m4 = InputThrottle - InputRoll + InputPitch + InputYaw;

// //   motor_write(m1, m2, m3, m4);

// //   // ================= CLEAN DEBUG =================
// //   if (millis() - lastPrint > 100) {
// //     lastPrint = millis();

// //     Serial.println("----------------------");
// //     Serial.println("----- DRONE DATA -----");

// //     Serial.print("RX: ");
// //     Serial.print(ReceiverValue[0]); Serial.print(" ");
// //     Serial.print(ReceiverValue[1]); Serial.print(" ");
// //     Serial.print(ReceiverValue[2]); Serial.print(" ");
// //     Serial.println(ReceiverValue[3]);

// //     Serial.print("GYRO: ");
// //     Serial.print(RateRoll); Serial.print(" ");
// //     Serial.print(RatePitch); Serial.print(" ");
// //     Serial.println(RateYaw);

// //     Serial.print("ERR: ");
// //     Serial.print(ErrorRoll); Serial.print(" ");
// //     Serial.print(ErrorPitch); Serial.print(" ");
// //     Serial.println(ErrorYaw);

// //     Serial.print("PID: ");
// //     Serial.print(PID[0]); Serial.print(" ");
// //     Serial.print(PID[1]); Serial.print(" ");
// //     Serial.println(PID[2]);

// //     Serial.print("MOTOR: ");
// //     Serial.print(m1); Serial.print(" ");
// //     Serial.print(m2); Serial.print(" ");
// //     Serial.print(m3); Serial.print(" ");
// //     Serial.println(m4);
// //   }

// //   delay(4);
// // }