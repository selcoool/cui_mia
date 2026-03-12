// test


// ARDUINO pro mini

// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 7
// #define CSN_PIN 10

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   if (!radio.begin()) {
//     Serial.println("❌ NRF24 không phát hiện!");
//     while(1);
//   }

//   radio.openReadingPipe(0, address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.startListening();

//   Serial.println("✅ RX ready (Arduino Pro Mini)");
//   Serial.println("------------------------------------------------");
// }

// void loop() {
//   if (radio.available()) {
//     JoystickData data;
//     radio.read(&data, sizeof(data));

//     Serial.print("🎮 Joy1: X="); Serial.print(data.joy1X);
//     Serial.print(", Y="); Serial.print(data.joy1Y);
//     Serial.print(" | Btn="); Serial.print(data.joy1Btn);

//     Serial.print(" || Joy2: X="); Serial.print(data.joy2X);
//     Serial.print(", Y="); Serial.print(data.joy2Y);
//     Serial.print(" | Btn="); Serial.println(data.joy2Btn);

//     // RAW DATA
//     Serial.print("📦 RAW DATA: ");
//     Serial.print(data.joy1X); Serial.print(",");
//     Serial.print(data.joy1Y); Serial.print(",");
//     Serial.print(data.joy2X); Serial.print(",");
//     Serial.print(data.joy2Y); Serial.print(",");
//     Serial.print(data.joy1Btn); Serial.print(",");
//     Serial.println(data.joy2Btn);

//     Serial.println("------------------------------------------------");
//   }

//   delay(20); // 50Hz
// }




// ESP32

// #include <SPI.h>
// #include <RF24.h>

// #define CE_PIN 14
// #define CSN_PIN 5

// RF24 radio(CE_PIN, CSN_PIN);
// const byte address[6] = "00001";

// // Pin joystick
// #define JOY1_X 33
// #define JOY1_Y 32
// #define JOY1_B 27
// #define JOY2_X 35
// #define JOY2_Y 34
// #define JOY2_B 25

// #define DEADZONE 247  // vùng chết

// struct JoystickData {
//   int16_t joy1X;
//   int16_t joy1Y;
//   int16_t joy2X;
//   int16_t joy2Y;
//   uint8_t joy1Btn;
//   uint8_t joy2Btn;
// };

// JoystickData lastData;

// int applyDeadzone(int value) {
//   int center = 2048; // ADC 12bit ESP32 (0-4095)
//   if (abs(value - center) < DEADZONE) return 0;
//   return value - center; // giá trị lệch tâm
// }

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   pinMode(JOY1_B, INPUT_PULLUP);
//   pinMode(JOY2_B, INPUT_PULLUP);

//   radio.begin();
//   radio.openWritingPipe(address);
//   radio.setPALevel(RF24_PA_LOW);
//   radio.setDataRate(RF24_1MBPS);
//   radio.stopListening();

//   Serial.println("✅ TX Ready");
// }

// void loop() {
//   JoystickData data;

//   data.joy1X = applyDeadzone(analogRead(JOY1_X));
//   data.joy1Y = applyDeadzone(analogRead(JOY1_Y));
//   data.joy2X = applyDeadzone(analogRead(JOY2_X));
//   data.joy2Y = applyDeadzone(analogRead(JOY2_Y));
//   data.joy1Btn = !digitalRead(JOY1_B);
//   data.joy2Btn = !digitalRead(JOY2_B);

//   // Chỉ gửi khi có thay đổi
//   if (memcmp(&data, &lastData, sizeof(data)) != 0) {
//     bool ok = radio.write(&data, sizeof(data));
//     lastData = data;
//     Serial.print(ok ? "✅ Sent: " : "❌ Failed: ");
//     Serial.print("Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//     Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);
//   } else {
//     // Khi không thay đổi, vẫn hiển thị Serial
//     Serial.print("📡 No movement | Joy1(X,Y,B)="); Serial.print(data.joy1X); Serial.print(","); Serial.print(data.joy1Y); Serial.print(","); Serial.print(data.joy1Btn);
//     Serial.print(" || Joy2(X,Y,B)="); Serial.print(data.joy2X); Serial.print(","); Serial.print(data.joy2Y); Serial.print(","); Serial.println(data.joy2Btn);
//   }

//   delay(20); // 50Hz
// }
