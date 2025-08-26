#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>

const char* ssid = "ESP32_WIFI";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

struct WiFiInfo {
  String ssid;
  int rssi;
  int channel;
  String encryption;
};

std::vector<WiFiInfo> wifiList;

// --- SPIFFS ---
bool initFS() {
  if(!SPIFFS.begin(true)){
    Serial.println("❌ Failed to mount SPIFFS");
    return false;
  }
  Serial.println("SPIFFS mounted");
  return true;
}

// --- Scan Wi-Fi ---
void scanWiFiList() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  wifiList.clear();
  for(int i=0;i<n;i++){
    WiFiInfo info;
    info.ssid = WiFi.SSID(i);
    info.rssi = WiFi.RSSI(i);
    info.channel = WiFi.channel(i);
    info.encryption = WiFi.encryptionType(i)==WIFI_AUTH_OPEN?"Open":"Encrypted";
    wifiList.push_back(info);
  }
}

// --- Lưu file JSON ---
void saveWiFiListFile() {
  File f = SPIFFS.open("/wifi_list.json","w");
  if(!f){
    Serial.println("❌ Failed to open file for writing");
    return;
  }
  String json = "[";
  for(size_t i=0;i<wifiList.size();i++){
    json += "{\"ssid\":\""+wifiList[i].ssid+"\",\"rssi\":"+String(wifiList[i].rssi)+
            ",\"channel\":"+String(wifiList[i].channel)+",\"encryption\":\""+wifiList[i].encryption+"\"}";
    if(i<wifiList.size()-1) json += ",";
  }
  json += "]";
  f.print(json);
  f.close();
  Serial.println("✅ Wi-Fi list saved to /wifi_list.json");
}

// --- Đọc file JSON ---
String readWiFiListFile() {
  if(!SPIFFS.exists("/wifi_list.json")) return "[]";
  File f = SPIFFS.open("/wifi_list.json","r");
  if(!f) return "[]";
  String data = f.readString();
  f.close();
  return data;
}

// --- HTTP ---
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>ESP32 Wi-Fi Scanner</title>
<script>
var socket;
function init() {
  socket = new WebSocket("ws://"+window.location.hostname+":81/");
  socket.onopen = function(){ scanWiFi(); };
  socket.onmessage = function(event){
    document.getElementById("wifiList").innerHTML = event.data;
  };
}

function scanWiFi(){
  socket.send("scan");
}

function loadFile(){
  socket.send("loadfile");
}
</script>
</head>
<body onload="init()">
<h1>ESP32 Wi-Fi Scanner + File</h1>
<div id="wifiList">Đang quét...</div>
<button onclick="scanWiFi()">Quét Wi-Fi</button>
<button onclick="loadFile()">Xem danh sách đã lưu</button>
</body>
</html>
)rawliteral";

void handleRoot(){ server.send(200,"text/html",MAIN_page); }

// --- WebSocket ---
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length){
  if(type != WStype_TEXT) return;
  String msg = (char*)payload;

  if(msg=="scan"){
    Serial.println("Scanning Wi-Fi...");
    scanWiFiList();
    saveWiFiListFile(); // Lưu luôn vào file
    String json = "[";
    for(size_t i=0;i<wifiList.size();i++){
      json += wifiList[i].ssid + " (RSSI: " + String(wifiList[i].rssi) + " dBm)<br>";
    }
    webSocket.sendTXT(num,json);
  }
  else if(msg=="loadfile"){
    String data = readWiFiListFile();
    webSocket.sendTXT(num,data);
    Serial.println("Sent saved Wi-Fi list");
  }
}

void setup() {
  Serial.begin(115200);
  initFS();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid,password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  server.on("/",handleRoot);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Server + WebSocket started");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
