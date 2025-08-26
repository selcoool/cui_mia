#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

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

// Quét Wi-Fi
void scanWiFiList() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  wifiList.clear();
  for (int i = 0; i < n; i++) {
    WiFiInfo info;
    info.ssid = WiFi.SSID(i);
    info.rssi = WiFi.RSSI(i);
    info.channel = WiFi.channel(i);
    info.encryption = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted";
    wifiList.push_back(info);
  }
}

// Chuyển danh sách Wi-Fi thành JSON
String wifiListJSON() {
  String json = "[";
  for (size_t i = 0; i < wifiList.size(); i++) {
    json += "{\"index\":" + String(i) + ",\"ssid\":\"" + wifiList[i].ssid + "\",\"rssi\":" + String(wifiList[i].rssi) + "}";
    if (i < wifiList.size() -1) json += ",";
  }
  json += "]";
  return json;
}

// Trang HTML
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>ESP32 Wi-Fi Scanner</title>
<script>
var socket;
function init() {
  socket = new WebSocket("ws://" + window.location.hostname + ":81/");
  socket.onopen = function() { scanWiFi(); };
  socket.onmessage = function(event) {
    try {
      if(event.data.startsWith("scanning")) {
        document.getElementById("wifiList").innerHTML = "Đang quét...";
        document.getElementById("detail").style.display="none";
        document.getElementById("wifiList").style.display="block";
      } else {
        var data = JSON.parse(event.data);
        if(data.ssid){ // Nếu là chi tiết
          document.getElementById("wifiList").style.display="none";
          document.getElementById("detail").style.display="block";
          document.getElementById("ssid").innerText = "SSID: " + data.ssid;
          document.getElementById("rssi").innerText = "RSSI: " + data.rssi + " dBm";
          document.getElementById("channel").innerText = "Channel: " + data.channel;
          document.getElementById("encryption").innerText = "Encryption: " + data.encryption;
        } else { // Danh sách
          var list = "<ul>";
          for(var i=0;i<data.length;i++){
            list += "<li><a href='#' onclick='showDetail(" + data[i].index + ")'>" + data[i].ssid + " (RSSI: " + data[i].rssi + ")</a></li>";
          }
          list += "</ul>";
          document.getElementById("wifiList").innerHTML = list;
          document.getElementById("detail").style.display="none";
          document.getElementById("wifiList").style.display="block";
        }
      }
    } catch(e){ console.log("Error:",e); }
  };
}

function scanWiFi() {
  socket.send('scan');
  document.getElementById("wifiList").innerHTML = "Đang quét...";
}

function showDetail(index){
  socket.send("detail," + index);
}

function showBack(){
  document.getElementById("detail").style.display="none";
  document.getElementById("wifiList").style.display="block";
}
</script>
</head>
<body onload="init()">
<h1>ESP32 Wi-Fi Scanner</h1>
<div id="wifiList">Đang quét...</div>
<div id="detail" style="display:none;">
  <h2>Chi tiết Wi-Fi</h2>
  <p id="ssid"></p>
  <p id="rssi"></p>
  <p id="channel"></p>
  <p id="encryption"></p>
  <button onclick="showBack()">⬅ Quay lại</button>
</div>
<button onclick="scanWiFi()">Quét lại</button>
</body>
</html>
)rawliteral";

// HTTP
void handleRoot() { server.send(200,"text/html",MAIN_page); }

// WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type != WStype_TEXT) return;
  String msg = (char*)payload;
  if(msg=="scan"){
    webSocket.sendTXT(num,"scanning");
    Serial.println("Scanning Wi-Fi...");
    scanWiFiList();
    String wifiJSON = wifiListJSON();  // <- tạo biến tạm để fix lỗi sendTXT
    webSocket.sendTXT(num, wifiJSON);
    Serial.println(wifiJSON);
  }
  else if(msg.startsWith("detail,")){
    int idx = msg.substring(7).toInt();
    if(idx>=0 && idx < wifiList.size()){
      String detail = "{\"ssid\":\"" + wifiList[idx].ssid + "\",\"rssi\":" + String(wifiList[idx].rssi) +
                      ",\"channel\":" + String(wifiList[idx].channel) + ",\"encryption\":\"" + wifiList[idx].encryption + "\"}";
      webSocket.sendTXT(num, detail);
      Serial.println("Detail sent: " + detail);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid,password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Server + WebSocket started");
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
