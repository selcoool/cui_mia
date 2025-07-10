#include <WiFi.h>
#include <WiFiServer.h>

const char* ssid = "My_AP";
const char* password = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);  // Cổng HTTP

// Giao diện HTML lưu trong Flash (PROGMEM)
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <title>ESP32-CAM Control</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }
    h1 { color: #333; }
    .controls button {
      width: 60px; height: 60px; margin: 10px;
      border-radius: 50%; font-size: 18px; background: #007BFF; color: white; border: none;
    }
    .controls button:active { background: #0056b3; }
  </style>
</head>
<body>
  <h1>🚗 Điều Khiển Robot</h1>
  <div class="controls">
    <button onclick="send('up')">⬆️</button><br>
    <button onclick="send('left')">⬅️</button>
    <button onclick="send('stop')">⏹</button>
    <button onclick="send('right')">➡️</button><br>
    <button onclick="send('down')">⬇️</button>
  </div>
  <script>
    function send(cmd) {
      fetch('/' + cmd)
        .then(res => console.log("Sent: " + cmd))
        .catch(err => console.log(err));
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect();
  delay(500);

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

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (request.endsWith("\r\n\r\n")) break;
      }
    }

    // Log nội dung yêu cầu
    Serial.println(request);

    // Trích URL từ request
    if (request.indexOf("GET /up") >= 0) {
      Serial.println("⬆️ Di chuyển lên");
    } else if (request.indexOf("GET /down") >= 0) {
      Serial.println("⬇️ Di chuyển xuống");
    } else if (request.indexOf("GET /left") >= 0) {
      Serial.println("⬅️ Di chuyển trái");
    } else if (request.indexOf("GET /right") >= 0) {
      Serial.println("➡️ Di chuyển phải");
    } else if (request.indexOf("GET /stop") >= 0) {
      Serial.println("⏹ Dừng lại");
    }

    // Gửi phản hồi HTML
    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
    client.print(htmlPage);
    delay(10);
    client.stop();
    Serial.println("🚪 Client ngắt kết nối");
  }
}
