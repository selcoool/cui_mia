#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <ESP32Servo.h>

// WiFi AP setup
const char* ssid = "ESP32_WIFI";
const char* password = "12345678";
WebServer server(80);
WebSocketsServer webSocket(81);

// MPU6050
MPU6050 mpu(Wire);
float pitch = 0, roll = 0, yaw = 0;

// Motors (ESCs)
Servo motor1, motor2, motor3, motor4;
const int PWM_MIN = 1000;
const int PWM_MAX = 2000;
int throttle = 1200;

// Control
String lastCommand = "";

// Timing
unsigned long lastTime = 0;
const int loopDelay = 50;

// Web UI with 3D model
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Drone 3D</title>
  <script src="https://cdn.jsdelivr.net/npm/three@0.150.1/build/three.min.js"></script>
  <style>
    body { margin: 0; overflow: hidden; font-family: sans-serif; }
    #controls {
      position: absolute;
      top: 10px; left: 10px;
      background: rgba(255,255,255,0.9);
      padding: 10px;
      border-radius: 8px;
    }
    button { width: 60px; margin: 2px; height: 30px; }
  </style>
</head>
<body>
<div id="controls">
  <div>
    <button onclick="sendCmd('up')">Up</button>
    <button onclick="sendCmd('down')">Down</button>
  </div>
  <div>
    <button onclick="sendCmd('left')">Left</button>
    <button onclick="sendCmd('right')">Right</button>
  </div>
  <div>
    <button onclick="sendCmd('yawL')">Yaw ⟲</button>
    <button onclick="sendCmd('yawR')">Yaw ⟳</button>
  </div>
  <div>
    <button onclick="sendCmd('stop')">STOP</button>
  </div>
</div>

<script>
  let socket = new WebSocket("ws://" + location.hostname + ":81/");
  let scene, camera, renderer, drone;

  socket.onopen = () => console.log("WebSocket connected");
  socket.onmessage = event => {
    try {
      const obj = JSON.parse(event.data);
      drone.rotation.x = obj.pitch * Math.PI / 180;
      drone.rotation.z = obj.roll  * Math.PI / 180;
      drone.rotation.y = obj.yaw   * Math.PI / 180;
    } catch (e) {
      console.error("Invalid data", e);
    }
  };

  function sendCmd(cmd) {
    socket.send(cmd);
    console.log("Sent:", cmd);
  }

  function init3D() {
    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(0, 2, 5);
    camera.lookAt(0, 0, 0);

    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    // Drone group
    drone = new THREE.Group();

    // Body
    const bodyGeometry = new THREE.BoxGeometry(1.5, 0.3, 1.5);
    const bodyMaterial = new THREE.MeshStandardMaterial({ color: 0x00aaff });
    const body = new THREE.Mesh(bodyGeometry, bodyMaterial);
    drone.add(body);

    // Arms
    const armGeometry = new THREE.CylinderGeometry(0.05, 0.05, 2.2, 8);
    const armMaterial = new THREE.MeshStandardMaterial({ color: 0x333333 });

    const arm1 = new THREE.Mesh(armGeometry, armMaterial);
    arm1.rotation.z = Math.PI / 4;
    drone.add(arm1);

    const arm2 = new THREE.Mesh(armGeometry, armMaterial);
    arm2.rotation.z = -Math.PI / 4;
    drone.add(arm2);

    // Propellers
    const propellerGeometry = new THREE.CylinderGeometry(0.1, 0.1, 0.02, 16);
    const propellerMaterial = new THREE.MeshStandardMaterial({ color: 0x000000 });
    const offset = 1.1;

    const positions = [
      [ offset, 0.2,  offset], // front right
      [-offset, 0.2,  offset], // front left
      [ offset, 0.2, -offset], // back right
      [-offset, 0.2, -offset]  // back left
    ];

    positions.forEach(pos => {
      const propeller = new THREE.Mesh(propellerGeometry, propellerMaterial);
      propeller.position.set(pos[0], pos[1], pos[2]);
      propeller.rotation.x = Math.PI / 2;
      drone.add(propeller);
    });

    scene.add(drone);

    const light = new THREE.PointLight(0xffffff, 1);
    light.position.set(5, 5, 5);
    scene.add(light);

    function animate() {
      requestAnimationFrame(animate);
      renderer.render(scene, camera);
    }
    animate();
  }

  window.onload = init3D;
</script>
</body>
</html>
)rawliteral";

// Xử lý root
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL

  byte status = mpu.begin();
  if (status != 0) {
    Serial.print("MPU6050 init failed! Code: ");
    Serial.println(status);
    while (1) delay(1000);
  }
  mpu.calcOffsets(true, true);
  Serial.println("MPU6050 ready");

  // ESC pin setup
  motor1.attach(13);
  motor2.attach(12);
  motor3.attach(25);
  motor4.attach(26);

  motor1.writeMicroseconds(PWM_MIN);
  motor2.writeMicroseconds(PWM_MIN);
  motor3.writeMicroseconds(PWM_MIN);
  motor4.writeMicroseconds(PWM_MIN);
  delay(2000);

  // WiFi
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.begin();

  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      lastCommand = String((char*)payload);
      Serial.println("Command: " + lastCommand);
    }
  });

  lastTime = millis();
}

void loop() {
  server.handleClient();
  webSocket.loop();

  if (millis() - lastTime < loopDelay) return;
  float dt = (millis() - lastTime) / 1000.0;
  lastTime = millis();

  mpu.update();
  pitch = mpu.getAngleX();
  roll = mpu.getAngleY();
  yaw = mpu.getAngleZ();

  // Gửi JSON tới client
  String json = "{\"pitch\":" + String(pitch, 2)
              + ",\"roll\":" + String(roll, 2)
              + ",\"yaw\":" + String(yaw, 2) + "}";
  webSocket.broadcastTXT(json);

  // In Serial
  Serial.printf("P: %.2f R: %.2f Y: %.2f\n", pitch, roll, yaw);

  // Xử lý lệnh điều khiển
  if (lastCommand != "") {
    if (lastCommand == "up") throttle = min(throttle + 10, PWM_MAX);
    else if (lastCommand == "down") throttle = max(throttle - 10, PWM_MIN);
    else if (lastCommand == "left") {
      motor1.writeMicroseconds(throttle - 20);
      motor2.writeMicroseconds(throttle + 20);
      motor3.writeMicroseconds(throttle + 20);
      motor4.writeMicroseconds(throttle - 20);
      delay(200);
    } else if (lastCommand == "right") {
      motor1.writeMicroseconds(throttle + 20);
      motor2.writeMicroseconds(throttle - 20);
      motor3.writeMicroseconds(throttle - 20);
      motor4.writeMicroseconds(throttle + 20);
      delay(200);
    } else if (lastCommand == "yawL") {
      motor1.writeMicroseconds(throttle + 30);
      motor2.writeMicroseconds(throttle - 30);
      motor3.writeMicroseconds(throttle + 30);
      motor4.writeMicroseconds(throttle - 30);
      delay(200);
    } else if (lastCommand == "yawR") {
      motor1.writeMicroseconds(throttle - 30);
      motor2.writeMicroseconds(throttle + 30);
      motor3.writeMicroseconds(throttle - 30);
      motor4.writeMicroseconds(throttle + 30);
      delay(200);
    } else if (lastCommand == "stop") {
      throttle = PWM_MIN;
    }

    // Gửi tốc độ mới
    motor1.writeMicroseconds(throttle);
    motor2.writeMicroseconds(throttle);
    motor3.writeMicroseconds(throttle);
    motor4.writeMicroseconds(throttle);

    lastCommand = "";
  }
}
