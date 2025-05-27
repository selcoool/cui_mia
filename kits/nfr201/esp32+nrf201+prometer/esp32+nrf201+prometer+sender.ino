#include <SPI.h>
#include <RF24.h>

// Chân kết nối
#define CE_PIN 14
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);  // CE, CSN
const byte address[6] = "00001";  // Địa chỉ pipe

// Chân joystick
#define JOY2_X 35
#define JOY2_Y 34
#define JOY2_B 25

#define JOY1_X 33
#define JOY1_Y 32
#define JOY1_B 27

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
  // Cập nhật giá trị joystick mỗi lần lặp
  int joy1X = analogRead(JOY1_X);
  int joy1Y = analogRead(JOY1_Y);


  int joy2X = analogRead(JOY2_X);
  int joy2Y = analogRead(JOY2_Y);


  // Tạo chuỗi để gửi
  String message = "q:" + String(joy1X) +
                   " w:" + String(joy1Y) +
                   " e:" + String(joy2X) +
                   " f:" + String(joy2Y);

  char text[100];
  message.toCharArray(text, sizeof(text));

  bool success = radio.write(&text, sizeof(text));

  Serial.println(success ? "✅ Gửi thành công!" : "❌ Gửi thất bại!");
  Serial.print("📤 Đã gửi: ");
  Serial.println(text);

  delay(1000);
}
