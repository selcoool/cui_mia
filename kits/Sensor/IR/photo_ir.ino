
#include <Arduino.h>
#define SENSOR 34

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR, INPUT);
}

void loop() {
  int val = analogRead(SENSOR);
  Serial.println(val);
  delay(100);
}
