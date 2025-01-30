// Định nghĩa chân kết nối
const int potentiometerPin = 34;  // Chân ADC kết nối với potentiometer
const int ledPin = 2;            // Chân GPIO kết nối với LED

void setup() {
  // Khởi tạo chân LED là OUTPUT
  pinMode(ledPin, OUTPUT);
  // Khởi tạo cổng serial để kiểm tra giá trị đọc được từ potentiometer
  Serial.begin(115200);
}

void loop() {
  // Đọc giá trị từ potentiometer (giá trị từ 0 đến 4095)
  int potValue = analogRead(potentiometerPin);
  
  // In giá trị đọc được từ potentiometer ra Serial Monitor (nếu cần kiểm tra)
  Serial.println("potValue");
  Serial.println(potValue);

  // Chuyển giá trị từ potentiometer thành giá trị PWM (từ 0 đến 255)
  int brightness = map(potValue, 0, 4095, 0, 255);
 Serial.println("brightness");
  Serial.println(brightness);
  // Điều chỉnh độ sáng của LED bằng PWM
  analogWrite(ledPin, brightness);

  // Chờ một chút trước khi đọc lại giá trị
  delay(100);
}