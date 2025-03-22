#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Tạo đối tượng cảm biến ADXL345
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();

// Định nghĩa chân PWM cho ESC
#define ESC_PIN 19  // Chân PWM để điều khiển ESC (ví dụ: pin 19 trên ESP32)

// Thiết lập kênh PWM và cấu hình của ESP32
const int pwmFreq = 5000;   // Tần số PWM là 5kHz
const int pwmResolution = 8; // Độ phân giải 8 bit (0-255)
const int pwmLedChannel = 0; // Kênh PWM mà chúng ta sẽ sử dụng

// Tốc độ PWM bình thường (có thể điều chỉnh)
const int pwm_normal_speed = 128;  // Giá trị PWM cho tốc độ bình thường

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

  // Cài đặt PWM cho ESP32
  ledcSetup(pwmLedChannel, pwmFreq, pwmResolution);  // Kênh 0, tần số 5kHz, độ phân giải 8 bit
  ledcAttachPin(ESC_PIN, pwmLedChannel);  // Kết nối chân GPIO với kênh PWM
}

void loop() {
  // Đọc dữ liệu cảm biến
  sensors_event_t event;
  accel.getEvent(&event);

  // Lấy giá trị gia tốc từ các trục X, Y, Z
  float x_accel = event.acceleration.x;
  float y_accel = event.acceleration.y;
  float z_accel = event.acceleration.z;

  // Tính toán giá trị PWM dựa trên gia tốc X (hoặc Y, Z)
  int pwm_value = map(x_accel, -16, 16, 0, 255); // Ánh xạ giá trị gia tốc X vào PWM
  pwm_value = constrain(pwm_value, 0, 255); // Giới hạn PWM từ 0 đến 255

  // In gia tốc và giá trị PWM ra Serial Monitor để hiển thị trên Serial Plotter
  Serial.print("X: ");
  Serial.print(x_accel);
  Serial.print("\tY: ");
  Serial.print(y_accel);
  Serial.print("\tZ: ");
  Serial.print(z_accel);
  Serial.print("\tPWM: ");
  Serial.println(pwm_value);  // In giá trị PWM lên Serial Plotter

  // Điều khiển PWM motor (ESC) theo gia tốc (có thể bỏ qua nếu không điều khiển motor)
  ledcWrite(pwmLedChannel, pwm_value);

  // Thêm độ trễ để làm cho dữ liệu dễ theo dõi
  delay(100);  // Giảm độ trễ để Serial Plotter có thể cập nhật nhanh hơn
}
