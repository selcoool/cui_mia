#define JOY1_X A0
#define JOY1_Y A1
#define JOY1_B 2

#define JOY2_X A2
#define JOY2_Y A3
#define JOY2_B 3

void setup() {
  Serial.begin(115200);  // Khởi tạo giao tiếp Serial với baud rate 115200
    pinMode(JOY1_B, INPUT_PULLUP);
    pinMode(JOY2_B, INPUT_PULLUP);
}

void loop() {
  // Đọc giá trị từ trục X và trục Y của joystick
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);
  int joy1B = !digitalRead(JOY1_B); // Nút bấm nhấn = 1, thả = 0

  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);
  int joy2B = !digitalRead(JOY2_B);

  // Hiển thị giá trị của joystick lên Serial Monitor
  Serial.print("joy1X: "); Serial.print(joy1X);
  Serial.print(", joy1Y: "); Serial.print(joy1Y);
  Serial.print(", joy1B: "); 
  Serial.println(joy1B == LOW ? "Pressed" : "Released");  // In trạng thái nút nhấn


    // Hiển thị giá trị của joystick lên Serial Monitor
  Serial.print("joy2X: "); Serial.print(joy2X);
  Serial.print(", joy2Y: "); Serial.print(joy2Y);
  Serial.print(", joy1B: "); 
  Serial.println(joy2B == LOW ? "Pressed" : "Released");  // In trạng thái nút nhấn

  delay(500);  // Đợi 500ms trước khi lấy giá trị tiếp theo
}
