#include <SPI.h>
#include <RF24.h>

// Chân kết nối NRF24 với Arduino Nano
#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";  // Phải giống TX

struct Data {
  int16_t throttle;
  int16_t roll;
  int16_t pitch;
  int16_t yaw;
};


Data receivedData;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!radio.begin()) {
    Serial.println("❌ Không tìm thấy NRF24!");
    while (1);
  }

  Serial.println("✅ NRF24L01 đã khởi tạo!");

  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.startListening();  // RX lắng nghe

  radio.printDetails();
  Serial.println("🚀 NRF24L01 đang chờ dữ liệu...");
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));



    // In debug
   Serial.print("Throttle: "); Serial.print(receivedData.throttle); // đúng
Serial.print(" | Roll: "); Serial.print(receivedData.roll);
Serial.print(" | Pitch: "); Serial.print(receivedData.pitch);
Serial.print(" | Yaw: "); Serial.println(receivedData.yaw);

  
  }
}
