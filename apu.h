#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "cpu.h"

#ifndef CHILLYGB_APU_H
#define CHILLYGB_APU_H

typedef struct {
    AudioStream stream;
    bool is_triggered;
    bool is_active;
    uint8_t lenght;
    uint8_t wave_duty;

    uint8_t volume;
    uint8_t sweep_pace;
    uint8_t env_dir;

    uint16_t period_value;
    float idx;
}pulse_channel;

typedef struct {
    AudioStream stream;
    bool is_triggered;
    bool is_active;
    uint8_t lenght;

    uint8_t volume;

    uint8_t wave_pattern[32];
    uint16_t period_value;
    float idx;
}wave_channel;

typedef struct {
    AudioStream stream;
    bool is_triggered;
    bool is_active;
    uint8_t lenght;

    uint8_t volume;
    uint8_t sweep_pace;
    uint8_t env_dir;

    uint16_t lfsr;
    uint32_t period_value;
    uint8_t current_bit;
    bool bit_7_mode;

    float idx;
}noise_channel;

typedef struct {
    pulse_channel ch1;
    pulse_channel ch2;
    wave_channel ch3;
    noise_channel ch4;
    float pan[4];
    bool is_on;
}channels;

extern channels audio;

void AudioInputCallback_CH1(void *buffer, unsigned int frames);
void AudioInputCallback_CH2(void *buffer, unsigned int frames);
void AudioInputCallback_CH3(void *buffer, unsigned int frames);
void AudioInputCallback_CH4(void *buffer, unsigned int frames);

void Update_Audio(cpu *c);
void tick_lfsr();

#endif //CHILLYGB_APU_H
