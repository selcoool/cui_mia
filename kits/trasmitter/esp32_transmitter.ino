#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsServer.h>



// ===== WebServer + WebSocket =====
WebServer server(80);
WebSocketsServer webSocket(81);
Preferences prefs;


// ===== Helpers =====
String macToStr(uint8_t *mac){
  char buf[18];
  sprintf(buf,"%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  return String(buf);
}

bool parseMAC(String macStr, uint8_t *mac){
  int values[6];
  if(sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
            &values[0],&values[1],&values[2],
            &values[3],&values[4],&values[5])==6){
    for(int i=0;i<6;i++) mac[i] = (uint8_t) values[i];
    return true;
  }
  return false;
}

// ======================
// JOYSTICK PINS
// ======================
#define JOY1_X 32
#define JOY1_Y 33
#define JOY1_B 27

#define JOY2_X 35
#define JOY2_Y 34
#define JOY2_B 25


// MAC của ESP32-C3 RX
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

uint8_t receiverMAC[] = {0x14,0x63,0x93,0xC5,0xCA,0x54};
 
// ======================
// DATA
// ======================
typedef struct
{
    uint16_t ch[8];
} Data;

Data tx;

// ======================

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // Serial.print("Send: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ======================



const int DEADZONE = 50;

// Giá trị tâm ban đầu
int centerJ1X = 2048;
int centerJ1Y = 2048;
int centerJ2X = 2048;
int centerJ2Y = 2048;

// Đọc joystick và chuyển sang RC
uint16_t joystickToRC(int adc, int center)
{
    // Deadzone
    if (abs(adc - center) <= DEADZONE)
        return 1500;

    if (adc < center)
    {
        return map(adc, 0, center, 1000, 1500);
    }
    else
    {
        return map(adc, center, 4095, 1500, 2000);
    }
}



// ===== HTML =====
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 PPM WebSocket</title>
<style>
body { font-family: Arial; text-align:center; }
input { width: 60%; margin-bottom: 10px; }
li { list-style:none; }
</style>
</head>
<body>

<h2>⚙ ESP32 PPM Config & WS</h2>

<form action="/set">
Receiver MAC:<br>
<input type="text" name="mac" id="mac"><br><br>
WiFi SSID:<br>
<input type="text" name="ssid" id="ssid"><br><br>
WiFi Password:<br>
<input type="text" name="password" id="password"><br><br>
<input type="submit" value="Save">
</form>

<p id="chipMAC"></p>
<h3>Channels (Realtime via WS):</h3>
<ul id="channelList"></ul>

<script>
function loadConfig(){
  fetch('/getConfig').then(res=>res.json()).then(cfg=>{
    document.getElementById('mac').value = cfg.mac;
    document.getElementById('ssid').value = cfg.ssid;
    document.getElementById('password').value = cfg.password;
    document.getElementById('chipMAC').innerText = 'Chip MAC: ' + cfg.chipMAC;
  });
}

loadConfig();

var ws = new WebSocket("ws://" + location.hostname + ":81/");
ws.onmessage = function(evt){
  let ch = JSON.parse(evt.data);
  let ul = document.getElementById('channelList');
  ul.innerHTML = '';
  for(let i=0;i<ch.length;i++){
    let li = document.createElement('li');
    li.innerText = 'CH'+(i+1)+': '+ch[i];
    ul.appendChild(li);
  }
};
</script>

</body>
</html>
)rawliteral";


// ===== HTTP Handlers =====
void handleRoot(){ server.send_P(200,"text/html", HTML_PAGE); }

void handleGetConfig(){
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid","ESP32_Config");
  String password = prefs.getString("password","12345678");
  prefs.end();

  uint8_t chipMAC[6];
  WiFi.macAddress(chipMAC);

  String json = "{\"mac\":\""+macToStr(receiverMAC)+"\",";
  json += "\"ssid\":\""+ssid+"\",";
  json += "\"password\":\""+password+"\",";
  json += "\"chipMAC\":\""+macToStr(chipMAC)+"\"}";
  server.send(200,"application/json", json);
}

void handleSet(){
  if(server.hasArg("mac") && server.hasArg("ssid") && server.hasArg("password")){
    String macStr = server.arg("mac");
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    uint8_t newMAC[6];
    if(parseMAC(macStr,newMAC)){
      memcpy(receiverMAC,newMAC,6);
      // updatePeer();

      prefs.begin("config", false);
      prefs.putBytes("mac", receiverMAC, 6);
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.end();

      WiFi.softAP(ssid.c_str(), password.c_str());
      server.sendHeader("Location","/");
      server.send(303);
      return;
    }
  }
  server.send(400,"text/plain","Invalid input");
}


void setup()
{
    Serial.begin(115200);


    // Load saved config
    prefs.begin("config", true);
    String ssid = prefs.getString("ssid","ESP32_Config");
    String password = prefs.getString("password","12345678");
    if(prefs.isKey("mac")) prefs.getBytes("mac", receiverMAC, 6);
    prefs.end();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

     // WebServer
    server.on("/", handleRoot);
    server.on("/getConfig", handleGetConfig);
    server.on("/set", handleSet);
    server.begin();

    // WebSocket
    webSocket.begin();
    webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length){
      if(type==WStype_TEXT) Serial.printf("WS msg from %u: %s\n", num, payload);
    });

    pinMode(JOY1_B, INPUT_PULLUP);
    pinMode(JOY2_B, INPUT_PULLUP);

    // Calibrate joystick
    Serial.println("Calibrating joystick...");
    delay(2000);

    centerJ1X = analogRead(JOY1_X);
    centerJ1Y = analogRead(JOY1_Y);
    centerJ2X = analogRead(JOY2_X);
    centerJ2Y = analogRead(JOY2_Y);

    Serial.println("CENTER");
    Serial.print("J1X = "); Serial.println(centerJ1X);
    Serial.print("J1Y = "); Serial.println(centerJ1Y);
    Serial.print("J2X = "); Serial.println(centerJ2X);
    Serial.print("J2Y = "); Serial.println(centerJ2Y);

    // ESP-NOW
    // WiFi.mode(WIFI_STA);
    // WiFi.disconnect();

    // WiFi.mode(WIFI_AP_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP NOW INIT FAIL");
        while (1);
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("ADD PEER FAIL");
        while (1);
    }

    Serial.println("TX READY");


    
}

// ======================

void loop()
{


   server.handleClient();
  webSocket.loop();


//    tx.ch[0] = joystickToRC(analogRead(JOY1_Y), centerJ1Y);

// tx.ch[1] = 3000 - joystickToRC(analogRead(JOY2_Y), centerJ2Y);

// // Pitch đảo chiều
// tx.ch[2] = joystickToRC(analogRead(JOY1_X), centerJ1X);

// tx.ch[3] = joystickToRC(analogRead(JOY2_X), centerJ2X);



   tx.ch[0] = joystickToRC(analogRead(JOY2_X), centerJ2X);

tx.ch[1] = joystickToRC(analogRead(JOY1_X), centerJ1X);

// Pitch đảo chiều
tx.ch[2] = joystickToRC(analogRead(JOY2_Y), centerJ2Y);

tx.ch[3] = joystickToRC(analogRead(JOY1_Y), centerJ1Y);

tx.ch[4] = 1500;
tx.ch[5] = 1500;
tx.ch[6] = 1500;
tx.ch[7] = 1500;

  Serial.printf(
    "CH0:%4d CH1:%4d CH2:%4d CH3:%4d CH4:%4d CH5:%4d CH6:%4d CH7:%4d\n",
    tx.ch[0],
    tx.ch[1],
    tx.ch[2],
    tx.ch[3],
    tx.ch[4],
    tx.ch[5],
    tx.ch[6],
    tx.ch[7]
);

esp_now_send(receiverMAC, (uint8_t *)&tx, sizeof(tx));


 // Broadcast PPM to WS clients
    String chJson = "[";
    for(int i=0;i<8;i++){
      chJson += String(tx.ch[i]);
      if(i<8-1) chJson += ",";
    }
    chJson += "]";
    webSocket.broadcastTXT(chJson);
// Serial debug


    Serial.print("📤 Channels: ");
    for(int i=0;i<8;i++){ Serial.print("CH"); Serial.print(i+1); Serial.print(":"); Serial.print(tx.ch[i]); Serial.print(" "); }
    Serial.println();

delay(20);
}






// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>

// // ======================
// // JOYSTICK PINS
// // ======================
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27

// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// // MAC của ESP32-C3 RX
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// // ======================
// // DATA
// // ======================
// typedef struct
// {
//     uint16_t ch[8];
// } Data;

// Data tx;

// // ======================

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//     Serial.print("Send: ");
//     Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
// }

// // ======================

// void setup()
// {
//     Serial.begin(115200);

//     pinMode(JOY1_B, INPUT_PULLUP);
//     pinMode(JOY2_B, INPUT_PULLUP);

//     WiFi.mode(WIFI_STA);
//     WiFi.disconnect();

//     if (esp_now_init() != ESP_OK)
//     {
//         Serial.println("ESP NOW INIT FAIL");
//         while (1);
//     }

//     esp_now_register_send_cb(OnDataSent);

//     esp_now_peer_info_t peerInfo = {};
//     memcpy(peerInfo.peer_addr, receiverMAC, 6);
//     peerInfo.channel = 0;
//     peerInfo.encrypt = false;

//     if (esp_now_add_peer(&peerInfo) != ESP_OK)
//     {
//         Serial.println("ADD PEER FAIL");
//         while (1);
//     }

//     Serial.println("TX READY");
// }

// // ======================

// void loop()
// {
//     tx.ch[0] = constrain(map(analogRead(JOY1_Y), 0, 4095, 1000, 2000), 1000, 2000);
//     tx.ch[1] = constrain(map(analogRead(JOY1_X), 0, 4095, 1000, 2000), 1000, 2000);
//     tx.ch[2] = constrain(map(analogRead(JOY2_Y), 4095, 0, 1000, 2000), 1000, 2000);
//     tx.ch[3] = constrain(map(analogRead(JOY2_X), 0, 4095, 1000, 2000), 1000, 2000);

//     tx.ch[4] = 1500;
//     tx.ch[5] = digitalRead(JOY1_B) == LOW ? 1000 : 2000;
//     tx.ch[6] = 1500;
//     tx.ch[7] = 1500;

//     // In dữ liệu trước khi gửi
//     Serial.printf(
//         "CH0:%4d CH1:%4d CH2:%4d CH3:%4d CH4:%4d CH5:%4d CH6:%4d CH7:%4d\n",
//         tx.ch[0],
//         tx.ch[1],
//         tx.ch[2],
//         tx.ch[3],
//         tx.ch[4],
//         tx.ch[5],
//         tx.ch[6],
//         tx.ch[7]);

//     // Gửi ESP-NOW
//     esp_now_send(receiverMAC, (uint8_t *)&tx, sizeof(tx));

//     delay(20);
// }


// void loop()
// {
//     // Đọc joystick
//     tx.ch[0] = map(analogRead(JOY1_Y), 0, 4095, 1000, 2000); // Throttle
//     tx.ch[1] = map(analogRead(JOY1_X), 0, 4095, 1000, 2000); // Yaw
//     tx.ch[2] = map(analogRead(JOY2_Y), 4095, 0, 1000, 2000); // Pitch
//     tx.ch[3] = map(analogRead(JOY2_X), 0, 4095, 1000, 2000); // Roll


//     tx.ch[4] = 0;
//     tx.ch[5] = 0;
//     tx.ch[6] = 0;
//     tx.ch[7] = 0;

//     // tx.ch[4] = 1500;
//     // tx.ch[5] = digitalRead(JOY1_B) == LOW ? 1000 : 2000;
//     // tx.ch[6] = 1500;
//     // tx.ch[7] = 1500;

//     // In dữ liệu sẽ gửi
//     Serial.print("TX -> ");
//     for (int i = 0; i < 8; i++)
//     {
//         Serial.print("CH");
//         Serial.print(i);
//         Serial.print(":");
//         Serial.print(tx.ch[i]);

//         if (i < 7)
//             Serial.print(" | ");
//     }
//     Serial.println();

//     // Gửi ESP-NOW
//     esp_now_send(receiverMAC, (uint8_t *)&tx, sizeof(tx));

//     delay(20);   // 50 Hz
// }

//  #include <Arduino.h>
// #include <esp_now.h>
// #include <WiFi.h>

// // Pins joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// // MAC RX (ESP32-C3)
// uint8_t receiverMAC[] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

// // Struct dữ liệu
// typedef struct {
//   int16_t joy1X;
//   int16_t joy1Y;
//   bool joy1Btn;
//   int16_t joy2X;
//   int16_t joy2Y;
//   bool joy2Btn;
// } JoystickData;

// JoystickData joystick;

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   Serial.print("Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   // Cấu hình pins
//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP NOW INIT FAIL");
//     return;
//   }

//   esp_now_register_send_cb(OnDataSent);

//   esp_now_peer_info_t peerInfo = {};
//   memcpy(peerInfo.peer_addr, receiverMAC, 6);
//   peerInfo.channel = 0;
//   peerInfo.encrypt = false;

//   if (esp_now_add_peer(&peerInfo) != ESP_OK) {
//     Serial.println("Add peer failed");
//     return;
//   }

//   Serial.println("TX READY");
// }

// void loop() {

//   // Đọc joystick 1
//   joystick.joy1X = analogRead(JOY1_X);
//   joystick.joy1Y = analogRead(JOY1_Y);
//   joystick.joy1Btn = digitalRead(JOY1_B) == LOW;

//   // Đọc joystick 2
//   joystick.joy2X = analogRead(JOY2_X);
//   joystick.joy2Y = analogRead(JOY2_Y);
//   joystick.joy2Btn = digitalRead(JOY2_B) == LOW;

//   // In dữ liệu gửi
//   Serial.print("J1 X: ");
//   Serial.print(joystick.joy1X);

//   Serial.print("  Y: ");
//   Serial.print(joystick.joy1Y);

//   Serial.print("  B: ");
//   Serial.print(joystick.joy1Btn);

//   Serial.print("   ||   ");

//   Serial.print("J2 X: ");
//   Serial.print(joystick.joy2X);

//   Serial.print("  Y: ");
//   Serial.print(joystick.joy2Y);

//   Serial.print("  B: ");
//   Serial.println(joystick.joy2Btn);

//   // Gửi dữ liệu
//   esp_now_send(receiverMAC, (uint8_t *)&joystick, sizeof(joystick));

//   delay(50);
// }


// #include <Arduino.h>

// // Joystick trái
// #define JOY1_X 32
// #define JOY1_Y 33

// // Joystick phải
// #define JOY2_X 34
// #define JOY2_Y 35

// // Nút nhấn
// #define SW1 25
// #define SW2 26

// void setup() {
//   Serial.begin(115200);

//   pinMode(SW1, INPUT_PULLUP);
//   pinMode(SW2, INPUT_PULLUP);
// }

// void loop() {

//   // Đọc joystick
//   int joy1X = analogRead(JOY1_X);
//   int joy1Y = analogRead(JOY1_Y);

//   int joy2X = analogRead(JOY2_X);
//   int joy2Y = analogRead(JOY2_Y);

//   // Đọc nút
//   bool sw1 = !digitalRead(SW1);
//   bool sw2 = !digitalRead(SW2);

//   Serial.print("JOY1 X:");
//   Serial.print(joy1X);

//   Serial.print("  Y:");
//   Serial.print(joy1Y);

//   Serial.print("   JOY2 X:");
//   Serial.print(joy2X);

//   Serial.print("  Y:");
//   Serial.print(joy2Y);

//   Serial.print("   SW1:");
//   Serial.print(sw1);

//   Serial.print("   SW2:");
//   Serial.println(sw2);

//   delay(50);
// }