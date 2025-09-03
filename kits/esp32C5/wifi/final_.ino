#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

Preferences prefs;

#define LED_PIN 2

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(192,168,4,1);

char ssid[32] = "ESP32_WIFI";
char password[64] = "12345678";

int speed = 100;
bool ledState = false;

// ================== HTML Pages ==================
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Cấu hình WiFi ESP32</title>
</head>
<body>
  <h1>⚙️ Đổi SSID & Mật khẩu</h1>
  <form action="/setwifi" method="POST">
    <p>SSID mới: <input type="text" name="ssid" minlength="1" maxlength="31" required></p>
    <p>Mật khẩu mới: <input type="text" name="pass" minlength="8" maxlength="63" required></p>
    <input type="submit" value="Đổi">
  </form>
  <br>
  <a href="/control">➡️ Trang điều khiển</a>
</body>
</html>
)rawliteral";

const char CONTROL_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Điều Khiển</title>
<script>
var socket;
function init() {
  socket = new WebSocket("ws://" + window.location.hostname + ":81/");
  socket.onmessage = function(event) {
    if(event.data.startsWith("speed,")){
      document.getElementById("speedValue").innerText = event.data.split(",")[1];
    }
  };
}
function sendCommand(cmd){ socket.send(cmd); }
function sendSpeed(val){ socket.send("speed," + val); }
</script>
<style>
body { font-family: Arial; text-align: center; }
.btn { padding: 20px; font-size: 24px; margin: 5px; cursor: pointer; }
.grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; width: 300px; margin: auto; }
</style>
</head>
<body onload="init()">
<h2>ESP32 Điều Khiển 4 Hướng</h2>
<div class="grid">
  <div></div>
  <button class="btn" onmousedown="sendCommand('UP')" onmouseup="sendCommand('STOP')">⬆️</button>
  <div></div>
  <button class="btn" onmousedown="sendCommand('LEFT')" onmouseup="sendCommand('STOP')">⬅️</button>
  <button class="btn" onmousedown="sendCommand('STOP')">⏹</button>
  <button class="btn" onmousedown="sendCommand('RIGHT')" onmouseup="sendCommand('STOP')">➡️</button>
  <div></div>
  <button class="btn" onmousedown="sendCommand('DOWN')" onmouseup="sendCommand('STOP')">⬇️</button>
  <div></div>
</div>
<br>
<button class="btn" onclick="sendCommand('BOOST')">🚀 Tăng tốc</button>
<button class="btn" onclick="sendCommand('TOGGLE_LED')">💡 Bật/Tắt LED</button>
<p>Tốc độ: <span id="speedValue">100</span></p>
<input type="range" min="0" max="255" value="100" oninput="sendSpeed(this.value)">
<br><br>
<a href="/">⬅️ Quay về trang cài đặt</a>
</body>
</html>
)rawliteral";

// ================== Handlers ==================
void handleRoot() { server.send(200, "text/html", MAIN_page); }
void handleControl() { server.send(200, "text/html", CONTROL_page); }

void handleSetWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");
    if(newPass.length() >= 8 && newPass.length() <= 63) {
      newSSID.toCharArray(ssid, sizeof(ssid));
      newPass.toCharArray(password, sizeof(password));

      // Lưu NVS
      prefs.begin("wifi", false);
      prefs.putString("ssid", newSSID);
      prefs.putString("pass", newPass);
      prefs.end();

      server.send(200, "text/html",
        "<h2>✅ Đổi WiFi thành công!</h2>"
        "<p>SSID: " + newSSID + "</p>"
        "<p>Pass: " + newPass + "</p>"
        "<p>ESP32 sẽ khởi động lại...</p>"
        "<script>setTimeout(()=>{location.href='http://192.168.4.1';},2000)</script>"
      );

      Serial.println("==== WiFi mới đã lưu ====");
      Serial.print("SSID: "); Serial.println(ssid);
      Serial.print("PASS: "); Serial.println(password);

      delay(2000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "❌ Mật khẩu phải từ 8 đến 63 ký tự!");
    }
  } else {
    server.send(400, "text/plain", "Thiếu SSID hoặc mật khẩu!");
  }
}

// ================== WebSocket ==================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type != WStype_TEXT) return;
  String msg = (char*)payload;

  Serial.print("Lệnh nhận từ WebSocket: ");
  Serial.println(msg);

  if(msg.startsWith("speed,")){
    speed = msg.substring(6).toInt();
    webSocket.broadcastTXT("speed," + String(speed));
  } else if(msg == "BOOST"){
    speed += 50; if(speed>255) speed=255;
    webSocket.broadcastTXT("speed," + String(speed));
  } else if(msg == "TOGGLE_LED"){
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState?HIGH:LOW);
  } else if(msg=="UP" || msg=="DOWN" || msg=="LEFT" || msg=="RIGHT" || msg=="STOP"){
    // TODO: điều khiển motor
  }
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Đọc NVS
  prefs.begin("wifi", true);
  String savedSSID = prefs.getString("ssid", "");
  String savedPASS = prefs.getString("pass", "");
  prefs.end();

  if(savedSSID.length()>0 && savedPASS.length()>=8){
    savedSSID.toCharArray(ssid,sizeof(ssid));
    savedPASS.toCharArray(password,sizeof(password));
  }

  Serial.println("===== WiFi đang dùng =====");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("PASS: "); Serial.println(password);

  // Bật AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  bool ok = WiFi.softAP(ssid,password);
  if(ok){
    Serial.print("✅ AP đã khởi động! Truy cập: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("❌ Lỗi bật AP! Quay về mặc định.");
    WiFi.softAP("ESP32_WIFI","12345678");
  }

  // DNS Server
  dnsServer.start(DNS_PORT, "*", apIP);

  // WebServer
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/setwifi", HTTP_POST, handleSetWiFi);
  server.onNotFound([](){ server.send(200,"text/html",MAIN_page); });
  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// ================== Loop ==================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  webSocket.loop();
}
