

// Luu y cam usb de doi thong tin

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Preferences.h>

// ===== PPM =====
#define PPM_PIN 34
#define CHANNELS 8

volatile uint16_t channels[CHANNELS];
volatile uint8_t channelIndex = 0;
volatile uint32_t lastRise = 0;

void IRAM_ATTR ppmISR() {
  uint32_t now = micros();
  uint32_t pulse = now - lastRise;
  lastRise = now;

  if(pulse > 3000){ 
    channelIndex = 0;
  } else if(channelIndex < CHANNELS){
    channels[channelIndex] = pulse;
    channelIndex++;
  }
}

// ===== ESP-NOW =====
typedef struct {
  uint16_t ch[CHANNELS];
} PPMData;

uint8_t receiverMAC[6] = {0x18,0x8B,0x0E,0x92,0x66,0x34};

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "📡 SEND OK" : "❌ SEND FAIL");
}

void updatePeer(){
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_del_peer(receiverMAC); 
  if(esp_now_add_peer(&peerInfo) == ESP_OK){
    Serial.println("✅ Peer updated");
  } else {
    Serial.println("❌ Peer update failed");
  }
}

// ===== WebServer =====
WebServer server(80);
Preferences prefs;

// ===== HTML =====
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 PPM Config</title>
<style>
body { font-family: Arial; text-align:center; }
input { width: 60%; margin-bottom: 10px; }
</style>
</head>
<body>

<h2>⚙ ESP32 Config</h2>

<form action="/set">
Receiver MAC:<br>
<input type="text" name="mac" id="mac"><br><br>
WiFi SSID:<br>
<input type="text" name="ssid" id="ssid"><br><br>
WiFi Password:<br>
<input type="text" name="password" id="password"><br><br>
<input type="submit" value="Save">
</form>

<script>
// Load current values
fetch('/getConfig')
.then(res => res.json())
.then(cfg => {
  document.getElementById('mac').value = cfg.mac;
  document.getElementById('ssid').value = cfg.ssid;
  document.getElementById('password').value = cfg.password;
});
</script>

</body>
</html>
)rawliteral";

// ===== HTTP Handlers =====
void handleRoot(){
  server.send_P(200,"text/html", HTML_PAGE);
}

String macToStr(uint8_t *mac){
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
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

// Trả về JSON {mac:ss, ssid:ss, password:ss}
void handleGetConfig(){
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid","ESP32_Config");
  String password = prefs.getString("password","12345678");
  prefs.end();

  String json = "{\"mac\":\""+macToStr(receiverMAC)+"\",";
  json += "\"ssid\":\""+ssid+"\",";
  json += "\"password\":\""+password+"\"}";
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
      updatePeer();

      prefs.begin("config", false);
      prefs.putBytes("mac", receiverMAC, 6);
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.end();

      // Cập nhật WiFi AP
      WiFi.softAP(ssid.c_str(), password.c_str());

      server.sendHeader("Location","/");
      server.send(303);
      return;
    }
  }
  server.send(400,"text/plain","Invalid input");
}

// ===== SETUP =====
void setup(){
  Serial.begin(115200);

  // PPM
  pinMode(PPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmISR, RISING);

  // WiFi
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid","ESP32_Config");
  String password = prefs.getString("password","12345678");
  if(prefs.isKey("mac")){
    prefs.getBytes("mac", receiverMAC, 6);
  }
  prefs.end();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // ESP-NOW
  WiFi.disconnect();
  if(esp_now_init() != ESP_OK){
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(onSent);
  updatePeer();

  // Web
  server.on("/", handleRoot);
  server.on("/getConfig", handleGetConfig);
  server.on("/set", handleSet);
  server.begin();
}

// ===== LOOP =====
void loop(){
  server.handleClient();

  static uint32_t lastSend = 0;
  if(millis()-lastSend>20){
    lastSend = millis();
    if(channelIndex<CHANNELS) return;

    PPMData data;
    noInterrupts();
    for(int i=0;i<CHANNELS;i++) data.ch[i]=channels[i];
    interrupts();

    esp_now_send(receiverMAC,(uint8_t*)&data,sizeof(data));

    Serial.print("📤 Channels: ");
    for(int i=0;i<CHANNELS;i++){
      Serial.print(data.ch[i]); Serial.print(" ");
    }
    Serial.println();
  }
}