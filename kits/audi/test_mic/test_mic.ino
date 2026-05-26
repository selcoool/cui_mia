#include <Arduino.h>
#include <driver/i2s.h>

// ================= INMP441 =================
#define MIC_WS    15
#define MIC_SCK   14
#define MIC_SD    33

// L/R của INMP441:
// GND  -> LEFT
// 3.3V -> RIGHT

#define SAMPLE_RATE 16000

void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println("INMP441 TEST");

    // ================= I2S CONFIG =================
    i2s_config_t config = {
        .mode = (i2s_mode_t)(
            I2S_MODE_MASTER |
            I2S_MODE_RX
        ),

        .sample_rate = SAMPLE_RATE,

        .bits_per_sample =
            I2S_BITS_PER_SAMPLE_32BIT,

        // Nếu L/R nối GND dùng LEFT
        .channel_format =
            I2S_CHANNEL_FMT_ONLY_RIGHT,

        .communication_format =
            I2S_COMM_FORMAT_STAND_I2S,

        .intr_alloc_flags = 0,

        .dma_buf_count = 8,

        .dma_buf_len = 64,

        .use_apll = false,

        .tx_desc_auto_clear = false,

        .fixed_mclk = 0
    };

    // ================= PIN CONFIG =================
    i2s_pin_config_t pins = {
        .bck_io_num = MIC_SCK,
        .ws_io_num = MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_SD
    };

    // ================= INSTALL =================
    esp_err_t err;

    err = i2s_driver_install(
        I2S_NUM_0,
        &config,
        0,
        NULL
    );

    if (err != ESP_OK) {

        Serial.println("I2S DRIVER FAIL");

        while (1);
    }

    err = i2s_set_pin(
        I2S_NUM_0,
        &pins
    );

    if (err != ESP_OK) {

        Serial.println("I2S PIN FAIL");

        while (1);
    }

    Serial.println("MIC START");
}

void loop() {

    int32_t samples[64];

    size_t bytesRead = 0;

    esp_err_t result = i2s_read(
        I2S_NUM_0,
        samples,
        sizeof(samples),
        &bytesRead,
        portMAX_DELAY
    );

    if (result != ESP_OK) {

        Serial.println("READ FAIL");

        return;
    }

    long avg = 0;

    for (int i = 0; i < 64; i++) {

        avg += abs(samples[i]);
    }

    avg /= 64;

    Serial.println(avg);

    delay(20);
}