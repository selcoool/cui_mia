#include <driver/i2s.h>

#define I2S_SAMPLE_RATE   44100
#define I2S_BCLK          26
#define I2S_LRCLK         25
#define I2S_DOUT          22

void setup() {
  Serial.begin(115200);

  // I2S configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  // I2S pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRCLK,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Install and start I2S
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("I2S sine wave test starting...");
}

void loop() {
  static int sample_index = 0;
  const float frequency = 440.0;
  const float amplitude = 30000.0;
  const int sample_rate = I2S_SAMPLE_RATE;

  float theta = 2.0 * PI * frequency * sample_index / sample_rate;
  int16_t sample = (int16_t)(amplitude * sin(theta));

  size_t bytes_written;
  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytes_written, portMAX_DELAY); // for stereo

  sample_index++;
}
