void setup() {
  Serial.begin(115200);             // Debug qua Serial Monitor
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // TX=17, RX=16
  Serial.println("ESP32 UART2 Test Started");
}

void loop() {
  // Gửi dữ liệu từ Serial Monitor sang RTL8720DN
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n'); // đọc đến newline
    cmd.trim(); // loại bỏ khoảng trắng
    if (cmd.length() > 0) {
      Serial2.println(cmd);  // gửi nguyên chuỗi và thêm newline
    }
  }

  // Nhận phản hồi từ RTL8720DN
  if (Serial2.available()) {
    String response = Serial2.readStringUntil('\n'); 
    response.trim();
    if (response.length() > 0) {
      Serial.print("Response from RTL8720DN: ");
      Serial.println(response);
    }
  }
}
