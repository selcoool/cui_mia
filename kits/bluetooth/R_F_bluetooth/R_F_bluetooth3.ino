#include <Arduino.h>
#include "BluetoothA2DPSink.h"

#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DIN  22

BluetoothA2DPSink a2dp_sink;

// Hiển thị metadata (tên bài hát, artist...)
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("🔸 Metadata: %s\n", text);
}


// Callback khi kết nối hoặc ngắt kết nối Bluetooth
// Callback khi kết nối hoặc ngắt kết nối Bluetooth
void bt_connected_callback(bool connected, const char* name) {
  if (connected) {
    Serial.println("✅ Bluetooth connected!");
    Serial.print("🔗 Device name: ");
    Serial.println(name);  // In ra tên thiết bị đang kết nối
  } else {
    Serial.println("❌ Bluetooth disconnected.");
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println("🔊 ESP32 Bluetooth Speaker with MAX98357A");

  // Cấu hình I2S cho MAX98357A
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 16,
    .dma_buf_len = 128,
    .use_apll = true, // Giảm rè nhờ PLL chuẩn
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  a2dp_sink.set_i2s_config(i2s_config);
  a2dp_sink.set_pin_config(pin_config);

  // Khi kết nối Bluetooth
  a2dp_sink.set_connected([](bool connected, const char* name) {
    if (connected) {
      Serial.printf("✅ Connected to: %s\n", name);
    } else {
      Serial.printf("❌ Disconnected from: %s\n", name);
    }
  });

  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // Điều chỉnh âm lượng tránh clipping
  a2dp_sink.set_volume(0);  // Có thể thử 40 ~ 80 tùy loa

  // Bắt đầu Bluetooth
  a2dp_sink.start("ESP32-Speaker");
  Serial.println("📡 Waiting for Bluetooth connection...");
}

void loop() {
  // Không cần làm gì
}
