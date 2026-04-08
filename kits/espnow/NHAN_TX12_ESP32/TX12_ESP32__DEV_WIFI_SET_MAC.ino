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

  if(pulse > 3000){ // sync pulse
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

uint8_t receiverMAC[6] = {0x18,0x8B,0x0E,0x92,0x66,0x34}; // default MAC

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "📡 SEND OK" : "❌ SEND FAIL");
}

void updatePeer(){
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_del_peer(receiverMAC); // xóa nếu đã tồn tại
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

<h2>⚙ ESP32 PPM Config</h2>

<form action="/set">
Receiver MAC:<br>
<input type="text" name="mac" id="mac"><br><br>
<input type="submit" value="Save">
</form>

<script>
// Load MAC từ ESP32 khi mở trang
fetch('/getMAC')
.then(res => res.text())
.then(mac => { document.getElementById('mac').value = mac; });
</script>

</body>
</html>
)rawliteral";

// ===== HTTP Handlers =====
void handleRoot(){
  server.send_P(200,"text/html", HTML_PAGE);
}

void handleGetMAC(){
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          receiverMAC[0],receiverMAC[1],receiverMAC[2],
          receiverMAC[3],receiverMAC[4],receiverMAC[5]);
  server.send(200,"text/plain", macStr);
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

void handleSet(){
  if(server.hasArg("mac")){
    String macStr = server.arg("mac");
    uint8_t newMAC[6];
    if(parseMAC(macStr,newMAC)){
      memcpy(receiverMAC,newMAC,6);

      prefs.begin("config", false);
      prefs.putBytes("mac", receiverMAC, 6);
      prefs.end();

      updatePeer();

      server.sendHeader("Location","/");
      server.send(303);
    } else {
      server.send(400,"text/plain","Invalid MAC");
    }
  }
}

// ===== SETUP =====
void setup(){
  Serial.begin(115200);

  // ===== PPM =====
  pinMode(PPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PPM_PIN), ppmISR, RISING);

  // ===== WiFi =====
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_Config","12345678");
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // ===== Load MAC =====
  prefs.begin("config", true);
  if(prefs.isKey("mac")){
    prefs.getBytes("mac", receiverMAC, 6);
    Serial.println("📥 Loaded MAC from flash");
  } else {
    Serial.println("⚙ Using default MAC");
  }
  prefs.end();

  // ===== ESP-NOW =====
  WiFi.disconnect();
  if(esp_now_init() != ESP_OK){
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(onSent);
  updatePeer();

  // ===== Web =====
  server.on("/", handleRoot);
  server.on("/getMAC", handleGetMAC);
  server.on("/set", handleSet);
  server.begin();
}

// ===== LOOP =====
void loop(){
  server.handleClient();

  static uint32_t lastSend = 0;
  if(millis()-lastSend>20){ // gửi 50Hz
    lastSend = millis();
    if(channelIndex<CHANNELS) return;

    PPMData data;
    noInterrupts();
    for(int i=0;i<CHANNELS;i++) data.ch[i]=channels[i];
    interrupts();

    esp_now_send(receiverMAC,(uint8_t*)&data,sizeof(data));

    // debug
    Serial.print("📤 Channels: ");
    for(int i=0;i<CHANNELS;i++){
      Serial.print(data.ch[i]); Serial.print(" ");
    }
    Serial.println();
  }
}
