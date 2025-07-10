#include <WiFi.h>
#include <WiFiServer.h>

const char* ssid = "My_AP";
const char* password = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);  // Cổng HTTP

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect();
  delay(1000);

  WiFi.config(local_ip, gateway, subnet);
  WiFi.apbegin((char*)ssid, (char*)password, (char*)"6");

  server.begin();

  Serial.print("✅ Wi-Fi đã phát: ");
  Serial.println(ssid);
  Serial.print("🔗 Truy cập: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("🧑 Client kết nối");
    String request = "";

    // Đọc yêu cầu từ client
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (request.endsWith("\r\n\r\n")) break;
      }
    }

    // Gửi phản hồi HTML
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n\r\n";
    response += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    response += "<title>Trang Điều Khiển</title>";
    response += "<style>body{font-family:sans-serif;text-align:center;background:#fdf6e3;}</style>";
    response += "</head><body>";
    response += "<h1>Xin chào từ RTL8720DN!</h1>";
    response += "<p>Đây là giao diện web đơn giản chạy trực tiếp từ thiết bị.</p>";
    response += "<form method='POST' action='/toggle'><button>Bật/Tắt LED</button></form>";
    response += "</body></html>";

    client.print(response);
    delay(10);
    client.stop();
    Serial.println("🚪 Client ngắt kết nối");
  }
}
