#define LED_PIN 2
const unsigned long blinkInterval = 300;     // Tốc độ nhấp nháy
const int voltageThreshold = 6000;           // Ngưỡng điện áp: 3000 mV = 3.0V

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  long vcc = readVcc();  // Đọc điện áp nguồn Vcc nội bộ
  Serial.print("🔋 Vcc đo được: ");
  Serial.print(vcc);
  Serial.println(" mV");

  if (vcc <= voltageThreshold) {
    // Pin yếu → nhấp nháy LED
    if ((millis() / blinkInterval) % 2 == 0) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  } else {
    // Pin mạnh → LED sáng liên tục
    digitalWrite(LED_PIN, HIGH);
  }

  delay(200);  // Đọc mỗi 200ms
}

// Hàm đo Vcc bằng phần mềm (không cần A0)
long readVcc() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // Đọc Vref = 1.1V
  delay(2);
  ADCSRA |= _BV(ADSC);  // Bắt đầu đo ADC
  while (bit_is_set(ADCSRA, ADSC));  // Đợi đo xong

  long result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result;  // Công thức tính Vcc (đơn vị mV)

  return result;
}
