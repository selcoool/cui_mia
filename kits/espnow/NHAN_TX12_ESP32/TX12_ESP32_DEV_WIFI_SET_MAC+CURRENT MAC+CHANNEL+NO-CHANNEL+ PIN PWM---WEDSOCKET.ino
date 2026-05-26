 #include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsServer.h>

// ================= CONFIG =================
#define MAX_CHANNELS 16

uint8_t ppmPin = 34;
uint8_t channelCount = 8;

// ===== PPM =====
volatile uint16_t channels[MAX_CHANNELS];
volatile uint8_t channelIndex = 0;
volatile uint32_t lastRise = 0;

void IRAM_ATTR ppmISR() {
  uint32_t now = micros();
  uint32_t pulse = now - lastRise;
  lastRise = now;

  if (pulse > 3000) {
    channelIndex = 0;
  } else if (channelIndex < channelCount) {
    channels[channelIndex] = pulse;
    channelIndex++;
  }
}

// ===== ESP-NOW =====
typedef struct {
  uint16_t ch[MAX_CHANNELS];
} PPMData;

uint8_t receiverMAC[6] = {0x18, 0x8B, 0x0E, 0x92, 0x66, 0x34};

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "📡 SEND OK" : "❌ SEND FAIL");
}

void updatePeer() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_del_peer(receiverMAC);
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("✅ Peer updated");
  } else {
    Serial.println("❌ Peer update failed");
  }
}

// ===== WebServer + WebSocket =====
WebServer server(80);
WebSocketsServer webSocket(81);
Preferences prefs;

// ===== Helpers =====
String macToStr(const uint8_t *mac) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]);
  return String(buf);
}

bool parseMAC(String macStr, uint8_t *mac) {
  int values[6];
  if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) mac[i] = (uint8_t)values[i];
    return true;
  }
  return false;
}

// ===== HTML =====
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 PPM Config</title>
<style>
body { font-family: Arial; text-align:center; }
input { width: 60%; margin-bottom: 10px; padding: 8px; }
li { list-style:none; }
button { padding: 10px 20px; }
</style>
</head>
<body>

<h2>⚙ ESP32 PPM Config</h2>

<form action="/set">
Receiver MAC:<br>
<input type="text" name="mac" id="mac"><br>

WiFi SSID:<br>
<input type="text" name="ssid" id="ssid"><br>

WiFi Password:<br>
<input type="text" name="password" id="password"><br>

PPM Pin:<br>
<input type="number" name="ppmpin" id="ppmpin"><br>

Channels (1–16):<br>
<input type="number" name="channels" id="channels" min="1" max="16"><br>

<input type="submit" value="Save">
</form>

<p id="chipMAC"></p>

<h3>Channels (Realtime via WebSocket)</h3>
<ul id="channelList"></ul>

<script>
function loadConfig(){
  fetch('/getConfig')
    .then(res => res.json())
    .then(cfg => {
      document.getElementById('mac').value = cfg.mac;
      document.getElementById('ssid').value = cfg.ssid;
      document.getElementById('password').value = cfg.password;
      document.getElementById('ppmpin').value = cfg.ppmpin;
      document.getElementById('channels').value = cfg.channels;
      document.getElementById('chipMAC').innerText =
        'Chip MAC: ' + cfg.chipMAC;
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
    li.innerText = 'CH'+(i+1)+': ' + ch[i];
    ul.appendChild(li);
  }
};
</script>

</body>
</html>
)rawliteral";

// ===== HTTP Handlers =====
void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handleGetConfig() {
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid", "ESP32_Config");
  String password = prefs.getString("password", "12345678");
  uint8_t pin = prefs.getUChar("ppmpin", 34);
  uint8_t ch = prefs.getUChar("channels", 8);
  prefs.end();

  uint8_t chipMAC[6];
  WiFi.macAddress(chipMAC);

  String json = "{";
  json += "\"mac\":\"" + macToStr(receiverMAC) + "\",";
  json += "\"ssid\":\"" + ssid + "\",";
  json += "\"password\":\"" + password + "\",";
  json += "\"ppmpin\":" + String(pin) + ",";
  json += "\"channels\":" + String(ch) + ",";
  json += "\"chipMAC\":\"" + macToStr(chipMAC) + "\"}";
  
  server.send(200, "application/json", json);
}

void handleSet() {
  if (server.hasArg("mac") &&
      server.hasArg("ssid") &&
      server.hasArg("password") &&
      server.hasArg("ppmpin") &&
      server.hasArg("channels")) {

    String macStr = server.arg("mac");
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    uint8_t newPin = server.arg("ppmpin").toInt();
    uint8_t newChannels = server.arg("channels").toInt();

    if (newChannels < 1 || newChannels > MAX_CHANNELS) {
      server.send(400, "text/plain", "Invalid channel count");
      return;
    }

    uint8_t newMAC[6];
    if (!parseMAC(macStr, newMAC)) {
      server.send(400, "text/plain", "Invalid MAC");
      return;
    }

    memcpy(receiverMAC, newMAC, 6);
    updatePeer();

    // Update PPM pin
    detachInterrupt(digitalPinToInterrupt(ppmPin));
    ppmPin = newPin;
    channelCount = newChannels;

    pinMode(ppmPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(ppmPin), ppmISR, RISING);

    // Save configuration
    prefs.begin("config", false);
    prefs.putBytes("mac", receiverMAC, 6);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putUChar("ppmpin", ppmPin);
    prefs.putUChar("channels", channelCount);
    prefs.end();

    WiFi.softAP(ssid.c_str(), password.c_str());

    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }

  server.send(400, "text/plain", "Invalid input");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  // Load configuration
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid", "ESP32_Config");
  String password = prefs.getString("password", "12345678");
  ppmPin = prefs.getUChar("ppmpin", 34);
  channelCount = prefs.getUChar("channels", 8);
  if (prefs.isKey("mac")) {
    prefs.getBytes("mac", receiverMAC, 6);
  }
  prefs.end();

  pinMode(ppmPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(ppmPin), ppmISR, RISING);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), password.c_str());
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // ESP-NOW
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSent);
  updatePeer();

  // WebServer
  server.on("/", handleRoot);
  server.on("/getConfig", handleGetConfig);
  server.on("/set", handleSet);
  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
      Serial.printf("WS msg from %u: %s\n", num, payload);
    }
  });

  Serial.println("✅ System Ready");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
  webSocket.loop();

  static uint32_t lastSend = 0;
  if (millis() - lastSend > 20) {
    lastSend = millis();
    if (channelIndex < channelCount) return;

    PPMData data;

    noInterrupts();
    for (int i = 0; i < channelCount; i++) {
      data.ch[i] = channels[i];
    }
    interrupts();

    esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

    // Send via WebSocket
    String chJson = "[";
    for (int i = 0; i < channelCount; i++) {
      chJson += String(data.ch[i]);
      if (i < channelCount - 1) chJson += ",";
    }
    chJson += "]";
    webSocket.broadcastTXT(chJson);

    // Serial Debug
    Serial.print("📤 Channels: ");
    for (int i = 0; i < channelCount; i++) {
      Serial.print("CH");
      Serial.print(i + 1);
      Serial.print(":");
      Serial.print(data.ch[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
}