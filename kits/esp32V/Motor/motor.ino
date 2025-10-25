
#include <Arduino.h>

// 4 chân PWM riêng, không xung đột với I2C
#define PWM1 18
#define PWM2 19
#define PWM3 23
#define PWM4 5

const int freq = 1000;       // tần số PWM 1 kHz
const int resolution = 16;   // 16-bit PWM

uint16_t pwmValue1 = 0;
uint16_t pwmValue2 = 0;
uint16_t pwmValue3 = 0;
uint16_t pwmValue4 = 0;

void setup() {
  Serial.begin(115200);

  // setup 4 kênh PWM riêng
  ledcSetup(0, freq, resolution);
  ledcSetup(1, freq, resolution);
  ledcSetup(2, freq, resolution);
  ledcSetup(3, freq, resolution);

  // attach từng chân riêng
  ledcAttachPin(PWM1, 0);
  ledcAttachPin(PWM2, 1);
  ledcAttachPin(PWM3, 2);
  ledcAttachPin(PWM4, 3);

  Serial.println("Enter PWM values (1000-2000) for 4 channels, format: ch1 ch2 ch3 ch4");
}

void loop() {
  // if (Serial.available() > 0) {
  //   int ch1 = Serial.parseInt();
  //   int ch2 = Serial.parseInt();
  //   int ch3 = Serial.parseInt();
  //   int ch4 = Serial.parseInt();

      int ch1 = 1200;
    int ch2 = 1200;
    int ch3 = 1200;
    int ch4 = 2000;

    // map từ 1000-2000 → 0-65535 (16-bit)
    pwmValue1 = map(ch1, 1000, 2000, 0, 65535);
    pwmValue2 = map(ch2, 1000, 2000, 0, 65535);
    pwmValue3 = map(ch3, 1000, 2000, 0, 65535);
    pwmValue4 = map(ch4, 1000, 2000, 0, 65535);

    // gán PWM riêng cho từng kênh
    ledcWrite(0, pwmValue1);
    ledcWrite(1, pwmValue2);
    ledcWrite(2, pwmValue3);
    ledcWrite(3, pwmValue4);

    Serial.print("PWM set: ");
Serial.print(pwmValue1); Serial.print(" ");
Serial.print(pwmValue2); Serial.print(" ");
Serial.print(pwmValue3); Serial.print(" ");
Serial.print(pwmValue4); 
Serial.print("\n"); // xuống dòng

  // }
}