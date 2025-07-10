#include <SPI.h>
#include <RF24.h>

// Chân kết nối NRF24 với Arduino Nano
#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);  // Khởi tạo đối tượng radio với chân CE và CSN
const byte address[6] = "00001";  // Địa chỉ pipe

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Kiểm tra nếu module NRF24L01 khởi tạo thành công
  if (!radio.begin()) {
    Serial.println("❌ Không tìm thấy NRF24!");
    while (1);  // Dừng chương trình nếu không tìm thấy module
  }

  // Nếu thành công, thông báo và in thông tin chi tiết của module
  Serial.println("✅ NRF24L01 đã khởi tạo thành công!");
  
  // Mở pipe để nhận dữ liệu từ ESP32
  radio.openReadingPipe(0, address);   
  radio.setPALevel(RF24_PA_LOW);       // Công suất thấp
  radio.setDataRate(RF24_1MBPS);       // Tốc độ truyền giống ESP32
  radio.startListening();              // Bắt đầu lắng nghe

  // In thông tin chi tiết của NRF24L01
  radio.printDetails();  // In chi tiết cấu hình module
  Serial.println("🚀 NRF24L01 đang chờ dữ liệu...");
}

void loop() {
  // Kiểm tra xem có dữ liệu đến không
  if (radio.available()) {
    char text[32] = {0};  // Bộ đệm để lưu dữ liệu nhận
    radio.read(&text, sizeof(text));
    Serial.print("📩 Dữ liệu nhận được: ");
    Serial.println(text);
  }
}
