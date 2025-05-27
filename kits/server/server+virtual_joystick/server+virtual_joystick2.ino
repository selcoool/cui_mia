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
    .joystick-direction {
      display: flex;
      flex-direction: row;
      justify-content: center;
      align-items: center;
     
     margin: 20px;
      background: #f0f0f0;
    }
    .joystick-zone {
      width: 200px;
      height: 200px;
      background: #ccc;
      border-radius: 50%;
      position: relative;
      margin: 20px;
      touch-action: none;
    }
    .joystick-stick {
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
 <div class="joystick-direction">
  <div id="joystick1" class="joystick-zone">
    <div id="joystick-stick1" class="joystick-stick"></div>
  </div>
  <div id="joystick2" class="joystick-zone">
    <div id="joystick-stick2" class="joystick-stick"></div>
  </div>
  </div>
  <div id="output">Joystick 1 - X: 2000, Y: 2000 | Joystick 2 - X: 2000, Y: 2000</div>

  <script>
    const joystick1Stick = document.getElementById('joystick-stick1');
    const joystick2Stick = document.getElementById('joystick-stick2');
    const joystick1Zone = document.getElementById('joystick1');
    const joystick2Zone = document.getElementById('joystick2');
    const output = document.getElementById('output');
    const center = 100;
    const maxDistance = 70;
    const deadzone = 5;

    const webSocket = new WebSocket('ws://' + location.host.split(":")[0] + ':81');

    webSocket.onopen = () => console.log('WebSocket connected');
    webSocket.onmessage = (event) => console.log('Received: ' + event.data);

    joystick1Zone.addEventListener('mousedown', (e) => startDrag(e, 1));
    joystick2Zone.addEventListener('mousedown', (e) => startDrag(e, 2));
    joystick1Zone.addEventListener('touchstart', (e) => startDrag(e, 1));
    joystick2Zone.addEventListener('touchstart', (e) => startDrag(e, 2));

    document.addEventListener('mousemove', drag);
    document.addEventListener('touchmove', drag);
    document.addEventListener('mouseup', endDrag);
    document.addEventListener('touchend', endDrag);

    let draggingJoystick = null;

    function getPosition(event, joystickNum) {
      const zone = joystickNum === 1 ? joystick1Zone : joystick2Zone;
      const rect = zone.getBoundingClientRect();
      let x = (event.touches ? event.touches[0].clientX : event.clientX) - rect.left;
      let y = (event.touches ? event.touches[0].clientY : event.clientY) - rect.top;
      return { x, y };
    }

    function startDrag(e, joystickNum) {
      if (draggingJoystick !== null) return; // Nếu đang kéo joystick khác thì không bắt đầu kéo joystick này nữa
      draggingJoystick = joystickNum;
      drag(e);
    }

    function drag(e) {
      if (!draggingJoystick) return;

      const joystickNum = draggingJoystick;
      const stick = joystickNum === 1 ? joystick1Stick : joystick2Stick;
      const zone = joystickNum === 1 ? joystick1Zone : joystick2Zone;

      const pos = getPosition(e, joystickNum);
      let dx = pos.x - center;
      let dy = pos.y - center;
      let distance = Math.sqrt(dx * dx + dy * dy);

      if (distance > maxDistance) {
        dx = (dx / distance) * maxDistance;
        dy = (dy / distance) * maxDistance;
      }

      stick.style.left = (center + dx - 30) + 'px';
      stick.style.top = (center + dy - 30) + 'px';

      let xValue = 2000;
      let yValue = 2000;

      if (Math.abs(dx) <= deadzone && Math.abs(dy) <= deadzone) {
        xValue = 2000;
        yValue = 2000;
      } else {
        xValue = Math.round(2000 + (dx / maxDistance) * 2000);
        yValue = Math.round(2000 + (dy / maxDistance) * 2000);
      }

      if (joystickNum === 1) {
        output.innerText = `Joystick 1 - X: ${xValue}, Y: ${yValue} | Joystick 2 - X: 2000, Y: 2000`;
        webSocket.send(`MOVE1,${xValue},${yValue}`);
      } else {
        output.innerText = `Joystick 1 - X: 2000, Y: 2000 | Joystick 2 - X: ${xValue}, Y: ${yValue}`;
        webSocket.send(`MOVE2,${xValue},${yValue}`);
      }
    }

    function endDrag() {
      if (draggingJoystick === 1) {
        joystick1Stick.style.left = '70px';
        joystick1Stick.style.top = '70px';
        webSocket.send('MOVE1,2000,2000');
      } else if (draggingJoystick === 2) {
        joystick2Stick.style.left = '70px';
        joystick2Stick.style.top = '70px';
        webSocket.send('MOVE2,2000,2000');
      }

      draggingJoystick = null;
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

    if (message.startsWith("MOVE1,")) {
      int comma1 = message.indexOf(',');
      int comma2 = message.indexOf(',', comma1 + 1);
      int x = message.substring(comma1 + 1, comma2).toInt();
      int y = message.substring(comma2 + 1).toInt();
      Serial.printf("Joystick 1 - X: %d, Y: %d\n", x, y);
    } 
    else if (message.startsWith("MOVE2,")) {
      int comma1 = message.indexOf(',');
      int comma2 = message.indexOf(',', comma1 + 1);
      int x = message.substring(comma1 + 1, comma2).toInt();
      int y = message.substring(comma2 + 1).toInt();
      Serial.printf("Joystick 2 - X: %d, Y: %d\n", x, y);
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
