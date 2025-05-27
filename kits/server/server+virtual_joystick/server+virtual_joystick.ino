#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32_WIFI";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Joystick Control</title>
  <style>
    body {
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background: #f0f0f0;
    }
    #joystick-zone {
      width: 200px;
      height: 200px;
      background: #ccc;
      border-radius: 50%;
      position: relative;
      touch-action: none;
    }
    #joystick-stick {
      width: 60px;
      height: 60px;
      background: #333;
      border-radius: 50%;
      position: absolute;
      left: 70px;
      top: 70px;
      touch-action: none;
    }
    #output {
      margin-top: 20px;
      font-size: 20px;
    }
  </style>
</head>
<body>
  <div id="joystick-zone">
    <div id="joystick-stick"></div>
  </div>
  <div id="output">X: 2000, Y: 2000</div>

  <script>
    const stick = document.getElementById('joystick-stick');
    const zone = document.getElementById('joystick-zone');
    const output = document.getElementById('output');
    let dragging = false;
    const center = 100; // Center of the joystick zone (200px / 2)
    const maxDistance = 70; // Maximum distance from the center to the border (radius of the joystick circle)
    const deadzone = 5; // Area where values don't change (when joystick is near the center)

    const webSocket = new WebSocket('ws://' + location.host.split(":")[0] + ':81');

    webSocket.onopen = () => console.log('WebSocket connected');
    webSocket.onmessage = (event) => console.log('Received: ' + event.data);

    zone.addEventListener('mousedown', startDrag);
    zone.addEventListener('touchstart', startDrag);
    document.addEventListener('mousemove', drag);
    document.addEventListener('touchmove', drag);
    document.addEventListener('mouseup', endDrag);
    document.addEventListener('touchend', endDrag);

    function getPosition(event) {
      const rect = zone.getBoundingClientRect();
      let x = (event.touches ? event.touches[0].clientX : event.clientX) - rect.left;
      let y = (event.touches ? event.touches[0].clientY : event.clientY) - rect.top;
      return { x, y };
    }

    function startDrag(e) {
      dragging = true;
      drag(e);
    }

    function drag(e) {
      if (!dragging) return;
      const pos = getPosition(e);
      let dx = pos.x - center;
      let dy = pos.y - center;
      let distance = Math.sqrt(dx * dx + dy * dy);

      // Clamp the joystick stick to the border of the joystick zone
      if (distance > maxDistance) {
        dx = (dx / distance) * maxDistance;
        dy = (dy / distance) * maxDistance;
      }

      // Update the position of the joystick stick
      stick.style.left = (center + dx - 30) + 'px';
      stick.style.top = (center + dy - 30) + 'px';

      // Calculate joystick values based on distance
      let xValue = 2000;
      let yValue = 2000;

      if (Math.abs(dx) <= deadzone && Math.abs(dy) <= deadzone) {
        // Joystick is near the center, set to neutral values
        xValue = 2000;
        yValue = 2000;
      } else {
        // Calculate the X and Y values based on distance and direction
        xValue = Math.round(2000 + (dx / maxDistance) * 2000);
        yValue = Math.round(2000 + (dy / maxDistance) * 2000);
      }

      output.innerText = `X: ${xValue}, Y: ${yValue}`;
      webSocket.send(`MOVE,${xValue},${yValue}`);
    }

    function endDrag() {
      dragging = false;
      stick.style.left = '70px';
      stick.style.top = '70px';
      output.innerText = 'X: 2000, Y: 2000';
      webSocket.send('MOVE,2000,2000'); // Send neutral values when joystick is released
    }
  </script>
</body>
</html>
  )rawliteral");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = (char*)payload;
    Serial.print("Command received: ");
    Serial.println(message);

    if (message.startsWith("MOVE,")) {
      int comma1 = message.indexOf(',');
      int comma2 = message.indexOf(',', comma1 + 1);
      int x = message.substring(comma1 + 1, comma2).toInt();
      int y = message.substring(comma2 + 1).toInt();
      Serial.printf("Joystick X: %d, Y: %d\n", x, y);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();
}
