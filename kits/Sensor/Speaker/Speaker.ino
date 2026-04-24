#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>

#define I2S_PORT I2S_NUM_0

#define BCLK 25
#define LRC  26
#define DIN  27

#define SAMPLE_RATE 16000
#define BUF_LEN 256

int16_t buffer[BUF_LEN];

float phase = 0;

// ================= SOUND MODE =================
int soundMode = 3;

// melody notes
float notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
int noteIndex = 0;
int noteTimer = 0;

// 🔊 volume control
float volume = 1.2;

void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,

    // ✅ FIX WARNING HERE
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,

    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = BCLK,
    .ws_io_num = LRC,
    .data_out_num = DIN,
    .data_in_num = -1
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
}

void loop() {
  size_t bytesWritten;

  float freq = 440;

  // ===== melody =====
  if (soundMode == 3) {
    freq = notes[noteIndex];

    noteTimer++;
    if (noteTimer > 50) {
      noteTimer = 0;
      noteIndex = (noteIndex + 1) % 8;
    }
  }

  for (int i = 0; i < BUF_LEN; i++) {

    float sample = 0;

    if (soundMode == 0) {
      sample = sin(phase);

    } else if (soundMode == 1) {
      sample = (sin(phase) > 0) ? 1 : -1;

    } else if (soundMode == 2) {
      sample = (float)random(-1000, 1000) / 1000.0;

    } else if (soundMode == 3) {
      sample = sin(phase);
    }

    // 🔊 tăng âm + chống rè
    float s = sample * volume;
    s = tanh(s);

    buffer[i] = s * 30000;

    phase += 2 * PI * freq / SAMPLE_RATE;
    if (phase > 2 * PI) phase -= 2 * PI;
  }

  i2s_write(I2S_PORT, buffer, BUF_LEN * 2, &bytesWritten, portMAX_DELAY);
}
