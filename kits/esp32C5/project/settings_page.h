const char SETTINGS_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Info</title>
<style>body{font-family:Arial;text-align:center;}</style>
</head>
<body>
<h2>Thông tin ESP32</h2>
<p>ESP32 đang phát WiFi AP: <strong>%AP_NAME%</strong></p>
<p>Trạng thái LED: <strong>%LED%</strong></p>
<br>
<a href="/">⬅️ Quay lại trang điều khiển</a>
</body>
</html>
)rawliteral";
