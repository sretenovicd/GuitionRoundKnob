#include "audio_buzzer.h"
#include "../config.h"
#include "driver/i2s_std.h"
#include <math.h>

static i2s_chan_handle_t tx_chan = NULL;
static unsigned long alert_end_time = 0;
static bool alert_active = false;
static float phase = 0.0f;

#define SAMPLE_RATE 16000
#define CHUNK_SAMPLES 256

void audio_buzzer_init() {
    pinMode(PIN_DAC_MUTE, OUTPUT);
    digitalWrite(PIN_DAC_MUTE, LOW); // Mute initially

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, &tx_chan, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] i2s_new_channel failed: %d\n", err);
        return;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)PIN_DAC_BCK,
            .ws = (gpio_num_t)PIN_DAC_WS,
            .dout = (gpio_num_t)PIN_DAC_DO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(tx_chan, &std_cfg);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] i2s_channel_init_std_mode failed: %d\n", err);
        return;
    }

    i2s_channel_enable(tx_chan);
    Serial.println("[AUDIO] PCM5100A I2S DAC initialized successfully");
}

void audio_buzzer_trigger_alert(uint32_t duration_ms) {
    if (!tx_chan) return;
    alert_end_time = millis() + duration_ms;
    alert_active = true;
    digitalWrite(PIN_DAC_MUTE, HIGH); // Unmute DAC
}

void audio_buzzer_update() {
    if (!alert_active || !tx_chan) return;

    unsigned long now = millis();
    if (now >= alert_end_time) {
        alert_active = false;
        digitalWrite(PIN_DAC_MUTE, LOW); // Mute DAC to save power and eliminate hiss
        return;
    }

    // Pleasant rhythmic chime: 200ms tone + 150ms pause
    // Alternates between 880Hz (A5) and 1320Hz (E6)
    unsigned long pattern_pos = (now % 700);
    bool sound_on = (pattern_pos < 200) || (pattern_pos >= 350 && pattern_pos < 550);
    float freq = (pattern_pos < 200) ? 880.0f : 1320.0f;

    int16_t buffer[CHUNK_SAMPLES * 2]; // Stereo L + R
    float phase_inc = (2.0f * M_PI * freq) / (float)SAMPLE_RATE;

    if (sound_on) {
        for (int i = 0; i < CHUNK_SAMPLES; i++) {
            phase += phase_inc;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            int16_t sample = (int16_t)(sinf(phase) * 8000.0f); // Pleasant audible volume
            buffer[i * 2] = sample;     // Left
            buffer[i * 2 + 1] = sample; // Right
        }
    } else {
        memset(buffer, 0, sizeof(buffer));
    }

    size_t bytes_written = 0;
    i2s_channel_write(tx_chan, buffer, sizeof(buffer), &bytes_written, 10);
}
