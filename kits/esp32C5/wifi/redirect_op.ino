#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;

const byte DNS_PORT = 53;
IPAddress apIP(192,168,4,1);
DNSServer dnsServer;
WebServer server(80);

char ssid[32] = "ESP32_WIFI";     // SSID mặc định
char password[64] = "12345678";   // Mật khẩu mặc định

// ================== HTML Form ==================
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
</body>
</html>
)rawliteral";

// ================== Trang chính ==================
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

// ================== Đổi SSID + Pass ==================
void handleSetWiFi() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");

    if (newPass.length() >= 8 && newPass.length() <= 63) {
      newSSID.toCharArray(ssid, sizeof(ssid));
      newPass.toCharArray(password, sizeof(password));

      // Lưu vào NVS
      prefs.begin("wifi", false);
      prefs.putString("ssid", newSSID);
      prefs.putString("pass", newPass);
      prefs.end();

      server.send(200, "text/html",
        "<h2>✅ Đổi WiFi thành công!</h2>"
        "<p>SSID mới: " + newSSID + "</p>"
        "<p>Mật khẩu mới: " + newPass + "</p>"
        "<p>ESP32 sẽ khởi động lại...</p>"
        "<script>setTimeout(()=>{location.href='http://192.168.4.1';},5000);</script>"
      );

      Serial.println("==== WiFi mới đã lưu ====");
      Serial.print("SSID: "); Serial.println(ssid);
      Serial.print("PASS: "); Serial.println(password);

      delay(2000);
      ESP.restart(); // reset để áp dụng
    } else {
      server.send(400, "text/html", "❌ Mật khẩu phải từ 8 đến 63 ký tự!");
    }
  } else {
    server.send(400, "text/plain", "Thiếu SSID hoặc mật khẩu!");
  }
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);

  // Đọc SSID/PASS từ NVS
  prefs.begin("wifi", true);
  String savedSSID = prefs.getString("ssid", "");
  String savedPASS = prefs.getString("pass", "");
  prefs.end();

  if (savedSSID.length() > 0 && savedPASS.length() >= 8) {
    savedSSID.toCharArray(ssid, sizeof(ssid));
    savedPASS.toCharArray(password, sizeof(password));
  }

  Serial.println("===== WiFi đang dùng =====");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("PASS: "); Serial.println(password);

  // Bật WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  bool ok = WiFi.softAP(ssid, password);

  if (ok) {
    Serial.print("✅ AP đã khởi động! Truy cập: http://");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("❌ Lỗi bật AP! Quay về mặc định.");
    WiFi.softAP("ESP32_WIFI", "12345678");
  }

  // DNS: Mọi domain đều trỏ về ESP32
  dnsServer.start(DNS_PORT, "*", apIP);

  // WebServer
  server.on("/", handleRoot);
  server.on("/setwifi", HTTP_POST, handleSetWiFi);
  server.onNotFound([]() {
    server.send(200, "text/html", MAIN_page);
  });
  server.begin();
}

// ================== Loop ==================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
