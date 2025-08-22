#include <Arduino.h>

// ============================
// Hàm greeting trả về "hello"
// ============================
String greeting() {
  return "hello";
}

// ============================
// Hàm lấy giá trị theo key
// ============================
String getValue(String data, String key) {
  int startIndex = data.indexOf(key + ":");
  if (startIndex == -1) return "";  
  startIndex += key.length() + 1;
  int endIndex = data.indexOf(" ", startIndex);
  if (endIndex == -1) endIndex = data.length();
  return data.substring(startIndex, endIndex);
}

// ============================
// Setup
// ============================
void setup() {
  Serial.begin(115200);   // Debug
  Serial1.begin(115200);  // UART giao tiếp với ESP32
  Serial.println("RTL8720DN UART Test Started");
}

// ============================
// Loop
// ============================
void loop() {
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n'); // đọc nguyên chuỗi tới newline
    cmd.trim(); // loại bỏ khoảng trắng đầu/cuối

    if (cmd.length() == 0) return; // bỏ qua chuỗi rỗng

    Serial.print("Received: ");
    Serial.println(cmd); // log nguyên chuỗi

    // Nếu nhận "h" thì trả về "hello"
    if (cmd == "h") {
      String result = greeting();
      Serial1.println(result);     // gửi kết quả về ESP32
    } 
    // Nếu nhận chuỗi có dạng key:value
    else if (cmd.indexOf(":") != -1) {
      String v1 = getValue(cmd, "k1");
      String v2 = getValue(cmd, "k2");
      String v3 = getValue(cmd, "y2");

      Serial.println("Parsed values:");
      Serial.println("k1 = " + v1);
      Serial.println("k2 = " + v2);
      Serial.println("y2 = " + v3);

      // gửi kết quả trả về ESP32
      Serial1.println("OK:" + v1 + "," + v2 + "," + v3);
    }
  }
}
