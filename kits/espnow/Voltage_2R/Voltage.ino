#include <Arduino.h>

#define LED_PIN 8
#define BATTERY_PIN 1

float readBatteryVoltage() {
  int sum = 0;

  // Lấy trung bình ADC cho ổn định
  for (int i = 0; i < 10; i++) {
    sum += analogRead(BATTERY_PIN);
    delay(5);
  }

  float adcValue = sum / 10.0;

  // Chuyển sang điện áp
  float v_adc = adcValue * 3.3 / 4095.0;

  // Nhân 2 vì chia áp 100k/100k
  float v_bat = v_adc * 2;

  return v_bat;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  float voltage = readBatteryVoltage();

  Serial.print("Battery Voltage: ");
  Serial.println(voltage);

  if (voltage <= 3.5) {
    // Pin yếu -> LED chớp (LED active LOW)
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
  } else {
    // Pin bình thường -> LED tắt
    digitalWrite(LED_PIN, HIGH);
    delay(500);
  }
}