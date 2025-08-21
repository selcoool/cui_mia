void setup() {
  Serial.begin(115200);   // Debug
  Serial1.begin(115200);  // UART giao tiếp với ESP32

  Serial.println("RTL8720DN UART Test Started");
}

void loop() {
  // Gửi dữ liệu từ RTL8720DN sang ESP32
  Serial1.println("Hello from RTL8720DN");
  delay(1000);

  // Đọc dữ liệu từ ESP32
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    Serial.print("Received from ESP32: ");
    Serial.println(data);
  }
}