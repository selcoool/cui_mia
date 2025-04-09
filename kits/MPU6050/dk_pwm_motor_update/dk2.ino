#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <MPU6050.h>

#define SS 5
#define RST 4
#define DIO0 26
#define LED 2  

// Định nghĩa chân PWM cho động cơ
int motorPin1 = 14; // Chân PWM cho động cơ 1
int motorPin2 = 12; // Chân PWM cho động cơ 2
int motorPin3 = 15; // Chân PWM cho động cơ 3
int motorPin4 = 13; // Chân PWM cho động cơ 4



// Khởi tạo đối tượng MPU6050
MPU6050 mpu;

// Các tham số PID cho pitch và roll (đã giảm để tránh dao động)
float Kp_pitch = 5.0;  // Hệ số P cho pitch
float Ki_pitch = 0.0;   // Hệ số I cho pitch
float Kd_pitch = 2.0;   // Hệ số D cho pitch
float Kp_roll = 5.0;   // Hệ số P cho roll
float Ki_roll = 0.0;    // Hệ số I cho roll
float Kd_roll = 2.0;    // Hệ số D cho roll

float previousErrorPitch = 0;
float integralPitch = 0;
float previousErrorRoll = 0;
float integralRoll = 0;

// Các biến cho cảm biến MPU6050
int16_t ax, ay, az;  // Gia tốc (thay kiểu dữ liệu thành int16_t)
int16_t gx, gy, gz;  // Tốc độ quay (thay kiểu dữ liệu thành int16_t)
float pitch, roll;   // Góc nghiêng (vẫn giữ kiểu dữ liệu float)
int td =0;

void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa init failed!");
        while (1);
    }
    Serial.println("✅ LoRa receiver ready");

    // Khởi tạo I2C và MPU6050
    Wire.begin();
    mpu.initialize();

    // Kiểm tra kết nối MPU6050
    if (!mpu.testConnection()) {
        Serial.println("MPU6050 connection failed");
        while (1);  // Dừng nếu không kết nối được
    }

    // Cấu hình PWM cho các động cơ
    ledcSetup(0, 1000, 8);  
    ledcSetup(1, 1000, 8);  
    ledcSetup(2, 1000, 8);  
    ledcSetup(3, 1000, 8);  

    ledcAttachPin(motorPin1, 0);
    ledcAttachPin(motorPin2, 1);
    ledcAttachPin(motorPin3, 2);
    ledcAttachPin(motorPin4, 3);

    // Cập nhật giá trị PWM ban đầu ổn định
    ledcWrite(0, 128);
    ledcWrite(1, 128);
    ledcWrite(2, 128);
    ledcWrite(3, 128);
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String received = "";
        while (LoRa.available()) {
            received += (char)LoRa.read();
        }
        Serial.println("📩 Nhận được: " + received);

        // Phân tích dữ liệu
        int joy1X = getValue(received, "joy1X").toInt();
        int joy1Y = getValue(received, "joy1Y").toInt();
        bool joy1B = getValue(received, "joy1B") == "1";  

        int joy2X = getValue(received, "joy2X").toInt();
        int joy2Y = getValue(received, "joy2Y").toInt();
        bool joy2B = getValue(received, "joy2B") == "1";  

        // Hiển thị dữ liệu
        Serial.println("🔹 Dữ liệu nhận được:");
        Serial.printf("🎮 Joystick 1 - X: %d, Y: %d, Button: %s\n", joy1X, joy1Y, joy1B ? "Pressed" : "Released");
        Serial.printf("🎮 Joystick 2 - X: %d, Y: %d, Button: %s\n", joy2X, joy2Y, joy2B ? "Pressed" : "Released");

        

                  if (joy1X == 4095 || joy1X == 0 || joy1Y == 0 || joy1Y == 4095 || joy2X == 4095 || joy2X == 0 || joy2Y == 4095 || joy2Y == 0) {
                // Tăng giá trị của một biến nếu joystick bị đẩy hết giới hạn
                static int someVariable = 0;  // Biến này sẽ thay đổi trong suốt quá trình chạy

                // Khi joystick 1 đạt giá trị cực đại (4095)
                if (joy1X == 4095) {
                    // Tăng giá trị, nhưng không vượt quá 255
                    if (someVariable < 255) {
                        someVariable += 10;  // Tăng thêm 10
                        if (someVariable > 255) {
                            someVariable = 255;  // Giới hạn giá trị không vượt quá 255
                        }
                    }
                    
                    // In ra giá trị mới của biến
                    Serial.println("Cập nhật biến: " + String(someVariable));
                }
                // Khi joystick 1 ở vị trí cực tiểu (0)
                else if (joy1X == 0) {
                    // Giảm giá trị, nhưng không dưới 0
                    if (someVariable > 0) {
                        someVariable -= 10;  // Giảm đi 10
                        if (someVariable < 0) {
                            someVariable = 0;  // Giới hạn giá trị không nhỏ hơn 0
                        }
                    }

                    // In ra giá trị mới của biến
                    Serial.println("Cập nhật biến: " + String(someVariable));
                }
            }











    }
}

// 🔎 Hàm tách giá trị từ chuỗi key:value
String getValue(String data, String key) {
    int startIndex = data.indexOf(key + ":");
    if (startIndex == -1) return "";  
    startIndex += key.length() + 1;
    int endIndex = data.indexOf(" ", startIndex);
    if (endIndex == -1) endIndex = data.length();
    return data.substring(startIndex, endIndex);
}
