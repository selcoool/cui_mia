const char PRODUCT_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Product</title>
<style>
body { font-family: Arial; text-align: center; }
table { margin:auto; border-collapse: collapse; width: 80%; }
th, td { border: 1px solid #333; padding: 8px; }
th { background-color: #ddd; }
</style>
</head>
<body>
<h2>Danh sách sản phẩm</h2>
<table>
  <tr><th>ID</th><th>Tên sản phẩm</th><th>Giá</th></tr>
  <tr><td>1</td><td>Đèn LED</td><td>50.000₫</td></tr>
  <tr><td>2</td><td>Motor DC</td><td>120.000₫</td></tr>
  <tr><td>3</td><td>ESP32 DevKit</td><td>250.000₫</td></tr>
</table>
<br>
<a href="/">⬅️ Quay lại trang điều khiển</a>
</body>
</html>
)rawliteral";
