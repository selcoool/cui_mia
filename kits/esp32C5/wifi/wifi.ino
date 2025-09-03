#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

char ssid[32] = "ESP32_WIFI";     // SSID mặc định
char password[32] = "12345678";   // Mật khẩu mặc định

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Giao diện HTML
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
    <p>Mật khẩu mới: <input type="text" name="pass" minlength="8" maxlength="31" required></p>
    <input type="submit" value="Đổi">
  </form>
</body>
</html>
)rawliteral";

// Trang chính
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

// Xử lý đổi SSID + Pass
void handleSetWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");

    if (newPass.length() >= 8 && newPass.length() <= 31) {
      newSSID.toCharArray(ssid, sizeof(ssid));
      newPass.toCharArray(password, sizeof(password));

      server.send(200, "text/html",
        "<h2>✅ Đổi WiFi thành công!</h2>"
        "<p>SSID mới: " + newSSID + "</p>"
        "<p>Mật khẩu mới: " + newPass + "</p>"
        "<p>ESP32 sẽ khởi động lại AP...</p>"
      );

      Serial.println("==== WiFi mới ====");
      Serial.print("SSID: "); Serial.println(ssid);
      Serial.print("PASS: "); Serial.println(password);

      // Ngắt AP cũ
      WiFi.softAPdisconnect(true);
      delay(1000);

      // Bật lại AP với SSID & PASS mới
      WiFi.softAP(ssid, password);

      Serial.println("AP đã khởi động lại!");
      Serial.print("IP: "); Serial.println(WiFi.softAPIP());
    } else {
      server.send(400, "text/html", "❌ Mật khẩu phải từ 8 đến 31 ký tự!");
    }
  } else {
    server.send(400, "text/plain", "Thiếu SSID hoặc mật khẩu!");
  }
}

void setup() {
  Serial.begin(115200);

  // Bật WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.print("AP SSID: ");
  Serial.println(ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // WebServer
  server.on("/", handleRoot);
  server.on("/setwifi", HTTP_POST, handleSetWiFi);
  server.begin();

  // WebSocket (chưa dùng, để sẵn nếu cần realtime)
  webSocket.begin();
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
