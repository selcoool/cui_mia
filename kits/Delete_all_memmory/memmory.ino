#include <Arduino.h>
#include <nvs_flash.h>
#include <esp_system.h>

void setup() {
  Serial.begin(115200);

  Serial.println("Dang xoa NVS...");

  esp_err_t err = nvs_flash_erase();

  if (err == ESP_OK) {
    Serial.println("Xoa thanh cong!");
  } else {
    Serial.printf("Loi: %d\n", err);
  }

  delay(2000);

  esp_restart();
}

void loop() {
}