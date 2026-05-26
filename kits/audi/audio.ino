// [env:esp32dev]
// platform = espressif32@6.7.0
// board = esp32dev
// framework = arduino

// monitor_speed = 115200

// board_build.filesystem = spiffs


#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#define I2S_DOUT 22
#define I2S_BCLK 14
#define I2S_LRC  25

#define SAMPLE_RATE 44100

struct Note {
    float freq;
    int duration;
};

// Melody
Note melody[] = {
    {262, 300}, // C4
    {294, 300}, // D4
    {330, 300}, // E4
    {349, 300}, // F4
    {392, 300}, // G4
    {440, 300}, // A4
    {494, 300}, // B4
    {523, 600}, // C5

    {494, 300},
    {440, 300},
    {392, 300},
    {349, 300},
    {330, 300},
    {294, 300},
    {262, 600},
};

void setupI2S() {

    i2s_config_t i2s_config = {
        .mode =
            (i2s_mode_t)(
                I2S_MODE_MASTER |
                I2S_MODE_TX
            ),

        .sample_rate = SAMPLE_RATE,

        .bits_per_sample =
            I2S_BITS_PER_SAMPLE_16BIT,

        .channel_format =
            I2S_CHANNEL_FMT_RIGHT_LEFT,

        .communication_format =
            I2S_COMM_FORMAT_I2S,

        .intr_alloc_flags = 0,

        .dma_buf_count = 8,

        .dma_buf_len = 64,

        .use_apll = false,

        .tx_desc_auto_clear = true,

        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(
        I2S_NUM_0,
        &i2s_config,
        0,
        NULL
    );

    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void playTone(float freq, int durationMs) {

    int samples = SAMPLE_RATE * durationMs / 1000;

    for (int i = 0; i < samples; i++) {

        float sine =
            sin(
                2.0 * PI *
                freq *
                i /
                SAMPLE_RATE
            );

        int16_t sample =
            (int16_t)(sine * 12000);

        int16_t stereo[2] = {
            sample,
            sample
        };

        size_t written;

        i2s_write(
            I2S_NUM_0,
            stereo,
            sizeof(stereo),
            &written,
            portMAX_DELAY
        );
    }
}

void setup() {

    Serial.begin(115200);

    setupI2S();

    Serial.println("Play melody");
}

void loop() {

    int count =
        sizeof(melody) / sizeof(Note);

    for (int i = 0; i < count; i++) {

        playTone(
            melody[i].freq,
            melody[i].duration
        );

        delay(30);
    }

    delay(1000);
}