#include <Arduino.h>
#include "esp_system.h"

const char* chipName(){
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  switch(chip_info.model){
    case CHIP_ESP32: return "ESP32";
    case CHIP_ESP32S2: return "ESP32-S2";
    case CHIP_ESP32S3: return "ESP32-S3";
    case CHIP_ESP32C3: return "ESP32-C3";
    default: return "UNKNOWN";
  }
}

void setup(){
  Serial.begin(115200);
  delay(2000);

  Serial.println("BOOT OK");
  Serial.println(chipName());
}

void loop(){
  Serial.println("RUNNING");
  delay(1000);
}
