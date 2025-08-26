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
<a href="/settings">➡️ Trang thông tin</a>
</body>
</html>
)rawliteral";
