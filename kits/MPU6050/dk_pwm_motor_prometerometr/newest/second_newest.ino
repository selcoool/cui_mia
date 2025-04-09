#include <Wire.h>
#include <MPU6050.h>
#include <SPI.h>
#include <LoRa.h>

#define SS 5
#define RST 4
#define DIO0 26
#define LED 2

// Chân PWM động cơ
int motorPin1 = 14;
int motorPin2 = 12;
int motorPin3 = 15;
int motorPin4 = 13;

// PID cho Pitch và Roll
float Kp_pitch = 15.0, Ki_pitch = 0.0, Kd_pitch = 5.0;
float Kp_roll = 15.0, Ki_roll = 0.0, Kd_roll = 5.0;

float previousErrorPitch = 0, integralPitch = 0;
float previousErrorRoll = 0, integralRoll = 0;

// Biến MPU6050
int16_t ax, ay, az, gx, gy, gz;
float pitch, roll;

// Biến động cơ
int motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4;

// Khởi tạo MPU6050
MPU6050 mpu;

void setup() {
    Serial.begin(115200);
     pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);  // Đảm bảo LED tắt ban đầu
    
    // Khởi động I2C với chân SDA = 21, SCL = 22
    Wire.begin(21, 22);
    
    Serial.println("🔍 Đang kiểm tra MPU6050...");
    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("❌ MPU6050 connection failed! Kiểm tra dây kết nối hoặc địa chỉ I2C.");
        while (1);
    }
    Serial.println("✅ MPU6050 đã kết nối thành công!");

    // Cấu hình PWM động cơ
    for (int i = 0; i < 4; i++) {
        ledcSetup(i, 1000, 8);
    }

    ledcAttachPin(motorPin1, 0);
    ledcAttachPin(motorPin2, 1);
    ledcAttachPin(motorPin3, 2);
    ledcAttachPin(motorPin4, 3);

    // Khởi tạo LoRa
    LoRa.setPins(SS, RST, DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("❌ LoRa init failed!");
        while (1);
    }
    Serial.println("✅ LoRa receiver ready");
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
        int key1 = getValue(received, "key1").toInt();
        int key2 = getValue(received, "key2").toInt();
        String key3 = getValue(received, "key3");

        int joy1X = getValue(received, "joy1X").toInt();
        int joy1Y = getValue(received, "joy1Y").toInt();
        bool joy1B = getValue(received, "joy1B") == "1";  

        int joy2X = getValue(received, "joy2X").toInt();
        int joy2Y = getValue(received, "joy2Y").toInt();
        bool joy2B = getValue(received, "joy2B") == "1";  




        // Đọc giá trị Joystick
  int pitchInput = map(joy1X, 0, 4095, -30, 30);
  int rollInput = map(joy1Y, 0, 4095, -30, 30);
  int yawInput = map(joy2X, 0, 4095, -20, 20);
  int throttle = map(joy2Y, 0, 4095, 50, 255);


 


  // Đọc dữ liệu từ MPU6050
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Tính toán góc Pitch và Roll
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  roll = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  // PID cho Pitch
  float errorPitch = -pitch;
  integralPitch += errorPitch;
  float derivativePitch = errorPitch - previousErrorPitch;
  float pidOutputPitch = Kp_pitch * errorPitch + Ki_pitch * integralPitch + Kd_pitch * derivativePitch;
  previousErrorPitch = errorPitch;

  // PID cho Roll
  float errorRoll = -roll;
  integralRoll += errorRoll;
  float derivativeRoll = errorRoll - previousErrorRoll;
  float pidOutputRoll = Kp_roll * errorRoll + Ki_roll * integralRoll + Kd_roll * derivativeRoll;
  previousErrorRoll = errorRoll;

  // Tính toán tốc độ động cơ
  motorSpeed1 = constrain(throttle + pitchInput + rollInput - yawInput + pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed2 = constrain(throttle - pitchInput + rollInput + yawInput - pidOutputPitch + pidOutputRoll, 0, 255);
  motorSpeed3 = constrain(throttle + pitchInput - rollInput + yawInput + pidOutputPitch - pidOutputRoll, 0, 255);
  motorSpeed4 = constrain(throttle - pitchInput - rollInput - yawInput - pidOutputPitch - pidOutputRoll, 0, 255);


 



        // Hiển thị dữ liệu
        Serial.println("🔹 Dữ liệu nhận được:");
        Serial.printf("➡ Key1: %d, Key2: %d, Key3: %s\n", key1, key2, key3.c_str());
        
        Serial.printf("🎮 Joystick 1 - X: %d, Y: %d, Button: %s\n", joy1X, joy1Y, joy1B ? "Pressed" : "Released");
        Serial.printf("🎮 Joystick 2 - X: %d, Y: %d, Button: %s\n", joy2X, joy2Y, joy2B ? "Pressed" : "Released");
       Serial.printf("Speed_Motor_1: %d, Speed_Motor_2: %d, Speed_Motor_3: %d, Speed_Motor_4: %d\n", motorSpeed1, motorSpeed2, motorSpeed3, motorSpeed4);
     Serial.printf("Joystick | Pitch: %d, Roll: %d, Yaw: %d, Throttle: %d\n",
              pitchInput, rollInput, yawInput, throttle);

        if(joy1X==0){
            Serial.println("Phai");
        }else if (joy1X ==4095) {
           
            Serial.println("Trai");
        }else if (joy1Y ==0) {          
            Serial.println("Tien");
        }else if (joy1Y ==4095) {          
            Serial.println("Lui");
        }else{
             // Cập nhật PWM động cơ
          ledcWrite(0, motorSpeed1);
          ledcWrite(1, motorSpeed2);
          ledcWrite(2, motorSpeed3);
          ledcWrite(3, motorSpeed4);
        }



        // Điều khiển LED
        if (key3 == "ON") {
            digitalWrite(LED, HIGH);
            Serial.println("💡 LED bật!");
        } else if (key3 == "OFF") {
            digitalWrite(LED, LOW);
            Serial.println("💡 LED tắt!");
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
