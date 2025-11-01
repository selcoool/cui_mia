#include <SPI.h>
#include <RF24.h>

// Chân kết nối NRF24 với ESP32
#define CE_PIN 14
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";  // Địa chỉ truyền

// Struct dữ liệu
struct Data {
  int16_t throttle;
  int16_t roll;
  int16_t pitch;
  int16_t yaw;
};

Data controlData;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!radio.begin()) {
    Serial.println("❌ Không tìm thấy NRF24!");
    while (1);
  }

  Serial.println("✅ NRF24L01 đã khởi tạo!");

  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.stopListening();  // ESP32 TX
}

void loop() {
  // Đọc analog từ joystick hoặc input
  controlData.throttle = map(analogRead(32), 4095, 0, 1000, 2000);
  controlData.roll     = map(analogRead(35), 4095, 0, 1000, 2000);
  controlData.pitch    = map(analogRead(34), 4095, 0, 1000, 2000);
  controlData.yaw      = map(analogRead(33), 4095, 0 , 1000, 2000);

  // Gửi dữ liệu
  radio.write(&controlData, sizeof(controlData));

  // Debug
  Serial.print("Throttle: "); Serial.print(controlData.throttle);
  Serial.print(" | Roll: "); Serial.print(controlData.roll);
  Serial.print(" | Pitch: "); Serial.print(controlData.pitch);
  Serial.print(" | Yaw: "); Serial.println(controlData.yaw);

  delay(50); // 20 lần/giây
}
