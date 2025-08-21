void setup() {
  Serial.begin(115200);      // Debug qua USB
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX=16, TX=17

  Serial.println("ESP32 UART2 Test Started");
}

void loop() {
  // Gửi dữ liệu từ ESP32 sang RTL8720DN
  Serial2.println("Hello from ESP32");
  delay(1000);

  // Đọc dữ liệu từ RTL8720DN
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    Serial.print("Received from RTL8720DN: ");
    Serial.println(data);
  }
}
