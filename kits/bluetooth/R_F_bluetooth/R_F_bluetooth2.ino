#include <Arduino.h>
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

#define I2S_DOUT 22
#define I2S_BCLK 26
#define I2S_LRC  25

void setup() {
  Serial.begin(115200);
  Serial.println("🔊 ESP32 Bluetooth Speaker with MAX98357A");

  i2s_pin_config_t my_pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  a2dp_sink.set_pin_config(my_pin_config);
  a2dp_sink.set_avrc_metadata_callback([](uint8_t id, const uint8_t *text) {
    Serial.printf("🎶 Metadata: %s\n", text);
  });

  a2dp_sink.set_volume(90);  // volume: 0 - 100
  a2dp_sink.start("ESP32-LOA");
  Serial.println("📡 Waiting for Bluetooth connection...");
}

void loop() {
  // nothing here
}
