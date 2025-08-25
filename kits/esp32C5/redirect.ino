#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(192,168,4,1);
DNSServer dnsServer;
WebServer webServer(80);

const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";

// Trang web hiển thị khi user kết nối
const char* redirectPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Xin chào</title>
  </head>
  <body>
    <h1>Xin chào!</h1>
    <p>Bạn đang được chuyển hướng tới trang demo.</p>
    <a href="https://www.example.com">Bấm vào đây để tiếp tục</a>
  </body>
</html>
)rawliteral";


void setup() {
  Serial.begin(115200);

  // Tạo WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(100);
  Serial.println("AP Started!");
  Serial.println(WiFi.softAPIP());

  // DNS: Mọi domain đều trỏ về ESP32
  dnsServer.start(DNS_PORT, "*", apIP);

  // Web server: trả về trang chào
  webServer.onNotFound([]() {
    webServer.send(200, "text/html", redirectPage);
  });
  webServer.begin();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
}
