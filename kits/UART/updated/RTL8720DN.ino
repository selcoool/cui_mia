void setup() {
  Serial.begin(115200);  // Debug
  Serial1.begin(115200); // UART giao tiếp với ESP32
  Serial.println("RTL8720DN UART Test Started");
}

void loop() {
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n'); // đọc nguyên chuỗi
    cmd.trim(); // loại bỏ khoảng trắng đầu/cuối

    if (cmd.length() == 0) return; // bỏ qua chuỗi rỗng

    Serial.print("Received: ");
    Serial.println(cmd); // log nguyên chuỗi

    // Kiểm tra ký tự đặc biệt
    if (cmd == "h") {
      String result = greeting();  // chạy chức năng
      Serial1.println(result);     // gửi kết quả về ESP32
    }
  }
}

// Hàm greeting trả về "hello"
String greeting() {
  return "hello";
}
