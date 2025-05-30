#include <SPI.h>
#include <RF24.h>

// Chân kết nối
#define CE_PIN 14
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);  // CE, CSN
const byte address[6] = "00001";  // Địa chỉ pipe

void setup() {
  Serial.begin(115200);
  delay(1000); // Đợi Serial sẵn sàng

  if (!radio.begin()) {
    Serial.println("❌ NRF24 không phát hiện được!");
    while (1); // Dừng nếu không tìm thấy module
  }

  Serial.println("✅ NRF24 đã khởi động thành công!");
  radio.openWritingPipe(address);      // Mở pipe để gửi
  radio.setPALevel(RF24_PA_LOW);       // Mức công suất
  radio.setDataRate(RF24_1MBPS);       // Tốc độ truyền
  radio.stopListening();               // Chế độ gửi

  // In thông tin chi tiết của module
  radio.printDetails();
  Serial.println("🚀 Sẵn sàng gửi dữ liệu...");
}

void loop() {
  const char text[] = "ESP32 -> Nano!";
  bool success = radio.write(&text, sizeof(text));

  Serial.println(success ? "✅ Gửi thành công!" : "❌ Gửi thất bại!");
  delay(1000);
}
