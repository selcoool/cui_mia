#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ====== Cấu hình Wi-Fi AP ======
const char* AP_SSID     = "ESP32_WIFI";
const char* AP_PASSWORD = "12345678"; // ít nhất 8 ký tự

// ====== Pin LED (onboard thường là GPIO 2) ======
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

WebServer server(80);
WebSocketsServer webSocket(81);

bool ledState = false;
unsigned long lastTick = 0;

// --------- Trang HTML đơn giản có WebSocket ----------
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="vi">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>ESP32 WebSocket Demo</title>
<style>
  body { font-family: ui-sans-serif, system-ui, Arial; margin: 0; padding: 24px; background:#0b1220; color:#e6edf3; }
  .card { max-width: 680px; margin: 0 auto; background: #0f172a; border: 1px solid #1f2937; border-radius: 16px; padding: 20px; box-shadow: 0 10px 30px rgba(0,0,0,.3); }
  h1 { margin-top: 0; font-size: 22px; }
  button { font-size: 16px; border:0; padding: 10px 16px; border-radius: 10px; background:#2563eb; color:white; cursor:pointer; }
  button:disabled { opacity:.5; cursor:not-allowed; }
  .row { display:flex; gap:12px; align-items:center; flex-wrap:wrap; }
  .badge { display:inline-block; padding:6px 10px; border-radius:999px; background:#111827; border:1px solid #374151; }
  .mono { font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }
  .ok { color:#22c55e; } .err { color:#ef4444; }
  .muted { color:#9ca3af; }
  .box { background:#0b1220; border:1px solid #1f2937; border-radius: 12px; padding:12px; }
</style>
</head>
<body>
  <div class="card">
    <h1>ESP32 ⟡ WebServer + WebSocket</h1>

    <div class="row">
      <span>WebSocket:</span>
      <span id="wsStatus" class="badge err">DISCONNECTED</span>
      <span class="muted">|</span>
      <span>LED:</span>
      <span id="ledBadge" class="badge">OFF</span>
      <button id="btnToggle" disabled>Toggle LED</button>
    </div>

    <div style="height:12px"></div>

    <div class="box mono" id="log" style="min-height:140px; white-space:pre-wrap;"></div>

    <div style="height:12px"></div>

    <div class="row">
      <button id="btnPing" disabled>Ping</button>
      <button id="btnGet" disabled>Get Status</button>
      <span class="muted">Uptime: <span id="uptime">0</span> s</span>
    </div>
  </div>

<script>
  const logEl = document.getElementById('log');
  const wsStatus = document.getElementById('wsStatus');
  const btnToggle = document.getElementById('btnToggle');
  const btnPing = document.getElementById('btnPing');
  const btnGet = document.getElementById('btnGet');
  const ledBadge = document.getElementById('ledBadge');
  const uptimeEl = document.getElementById('uptime');

  let ws;
  function connectWS() {
    const url = "ws://" + window.location.hostname + ":81/";
    ws = new WebSocket(url);

    ws.onopen = () => {
      wsStatus.textContent = "CONNECTED";
      wsStatus.classList.remove('err'); wsStatus.classList.add('ok');
      btnToggle.disabled = false; btnPing.disabled = false; btnGet.disabled = false;
      ws.send("hello");
      addLog("WS connected: " + url);
    };

    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data);
        if (msg.type === "status") {
          ledBadge.textContent = msg.led ? "ON" : "OFF";
        }
        if (msg.type === "uptime") {
          uptimeEl.textContent = msg.seconds;
        }
        if (msg.type === "pong") {
          addLog("PONG: " + (msg.t || ""));
        }
        if (msg.type === "info") {
          addLog("INFO: " + msg.msg);
        }
      } catch(e) {
        addLog("RX: " + ev.data);
      }
    };

    ws.onclose = () => {
      wsStatus.textContent = "DISCONNECTED";
      wsStatus.classList.remove('ok'); wsStatus.classList.add('err');
      btnToggle.disabled = true; btnPing.disabled = true; btnGet.disabled = true;
      addLog("WS disconnected. Reconnecting in 2s…");
      setTimeout(connectWS, 2000);
    };

    ws.onerror = () => {
      addLog("WS error");
    };
  }

  function addLog(s) {
    const time = new Date().toLocaleTimeString();
    logEl.textContent = `[${time}] ${s}\n` + logEl.textContent;
  }

  btnToggle.onclick = () => ws.send("toggle");
  btnPing.onclick   = () => ws.send(JSON.stringify({type:"ping", t: Date.now()}));
  btnGet.onclick    = () => ws.send("get_status");

  connectWS();
</script>
</body>
</html>
)HTML";

// ---------- WebSocket events ----------
void handleWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] Client %u connected: %s\n", num, ip.toString().c_str());
      // Gửi trạng thái ban đầu
      String s = String("{\"type\":\"status\",\"led\":") + (ledState ? "true" : "false") + "}";
      webSocket.sendTXT(num, s);
      break;
    }
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Client %u disconnected\n", num);
      break;

    case WStype_TEXT: {
      String msg = String((char*)payload, length);
      Serial.printf("[WS] RX: %s\n", msg.c_str());

      if (msg == "toggle") {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        String s = String("{\"type\":\"status\",\"led\":") + (ledState ? "true" : "false") + "}";
        webSocket.broadcastTXT(s);
      } else if (msg == "get_status") {
        String s = String("{\"type\":\"status\",\"led\":") + (ledState ? "true" : "false") + "}";
        webSocket.sendTXT(num, s);
      } else {
        // Thử parse JSON cho ping
        if (msg.indexOf("\"type\":\"ping\"") != -1) {
          webSocket.sendTXT(num, String("{\"type\":\"pong\",\"t\":") + millis() + "}");
        } else {
          webSocket.sendTXT(num, "{\"type\":\"info\",\"msg\":\"Unknown command\"}");
        }
      }
      break;
    }

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Bật AP
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (!ok) {
    Serial.println("[WiFi] softAP failed!");
  }
  // (Tùy chọn) đặt IP AP cố định: 192.168.4.1 là mặc định
  // WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

  Serial.print("[WiFi] AP SSID: "); Serial.println(AP_SSID);
  Serial.print("[WiFi] AP IP  : "); Serial.println(WiFi.softAPIP());

  // HTTP server
  server.on("/", []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/api/led", HTTP_GET, [](){
    String s = String("{\"led\":") + (ledState ? "true":"false") + "}";
    server.send(200, "application/json", s);
  });
  server.begin();
  Serial.println("[HTTP] Server started on :80");

  // WebSocket server
  webSocket.begin();
  webSocket.onEvent(handleWsEvent);
  Serial.println("[WS] Server started on :81");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // Gửi uptime mỗi 1 giây qua WS
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    String up = String("{\"type\":\"uptime\",\"seconds\":") + (millis()/1000) + "}";
    webSocket.broadcastTXT(up);
  }
}
