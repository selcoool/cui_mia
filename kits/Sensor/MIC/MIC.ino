#include <Arduino.h>
#include "driver/i2s.h"

// ================= MIC =================
#define I2S_MIC I2S_NUM_0
#define MIC_BCLK 14
#define MIC_WS   15
#define MIC_SD   32

#define BUF_LEN 256

int32_t micBuffer[BUF_LEN];

void setupMic() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin = {
    .bck_io_num = MIC_BCLK,
    .ws_io_num = MIC_WS,
    .data_out_num = -1,
    .data_in_num = MIC_SD
  };

  i2s_driver_install(I2S_MIC, &cfg, 0, NULL);
  i2s_set_pin(I2S_MIC, &pin);
}

void setup() {
  Serial.begin(115200);
  setupMic();
  Serial.println("MIC TEST START");
}

void loop() {
  size_t bytesRead;

  i2s_read(I2S_MIC, micBuffer, BUF_LEN * 4, &bytesRead, portMAX_DELAY);

  int samples = bytesRead / 4;

  long avg = 0;
  int peak = 0;

  for (int i = 0; i < samples; i++) {

    int32_t raw = micBuffer[i];

    // debug raw mic
    avg += abs(raw);
    if (abs(raw) > peak) peak = abs(raw);
  }

  Serial.print("AVG:");
  Serial.print(avg / samples);
  Serial.print(" | PEAK:");
  Serial.println(peak);

  delay(100);
}