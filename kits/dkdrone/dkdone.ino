#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Tạo đối tượng cảm biến ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();

void setup() {
  // Khởi động Serial để in thông tin
  Serial.begin(115200);  // Đảm bảo sử dụng baud rate là 115200
  while (!Serial);  // Đợi cho Serial chuẩn bị sẵn sàng (cho ESP32)

  // Kiểm tra xem cảm biến có kết nối thành công không
  if (!accel.begin()) {
    Serial.println("Không thể tìm thấy cảm biến ADXL345");
    while (1);  // Dừng chương trình nếu không tìm thấy cảm biến
  }

  // Đặt phạm vi đo gia tốc (ví dụ: 2g, 4g, 8g, 16g)
  accel.setRange(ADXL345_RANGE_16_G);  // Đặt phạm vi gia tốc là 16g
  Serial.println("Cảm biến ADXL345 đã sẵn sàng.");
}

void loop() {
  // Đọc dữ liệu cảm biến
  sensors_event_t event;
  accel.getEvent(&event);

  // In dữ liệu gia tốc ra Serial Plotter
  Serial.print("X:");
  Serial.print(event.acceleration.x);
  Serial.print("\t");

  Serial.print("Y:");
  Serial.print(event.acceleration.y);
  Serial.print("\t");

  Serial.print("Z:");
  Serial.println(event.acceleration.z);

  // Thêm độ trễ để làm cho dữ liệu dễ theo dõi
  delay(100);  // Giảm độ trễ để Serial Plotter có thể cập nhật nhanh hơn
}
