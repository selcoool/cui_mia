#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN

const byte address[6] = "00001";  // Địa chỉ 6 byte để nhận tín hiệu

void setup() {
  Serial.begin(115200);  // Khởi tạo giao tiếp Serial

  // Kiểm tra trạng thái khởi tạo của nRF24L01
  if (!radio.begin()) {
    Serial.println("Khởi tạo nRF24L01 thất bại!");
    while (1);  // Dừng lại nếu khởi tạo không thành công
  }

  // Nếu khởi tạo thành công
  Serial.println("Khởi tạo nRF24L01 thành công!");

  radio.openReadingPipe(1, address);  // Đặt địa chỉ 6 byte để nhận dữ liệu
  radio.setPALevel(RF24_PA_HIGH);  // Cài đặt công suất nhận
  radio.startListening();  // Thiết lập chế độ nhận
}

void loop() {
  if (radio.available()) {
    char text[32] = "";  // Mảng lưu trữ dữ liệu nhận được
    radio.read(&text, sizeof(text));  // Đọc dữ liệu từ I2C

    Serial.print("Đã nhận: ");
    Serial.println(text);  // In dữ liệu nhận được lên Serial Monitor
  }
}
