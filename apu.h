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

    uint16_t period_value;
    float idx;
    uint8_t volume;
}pulse_channel;

typedef struct {
    AudioStream stream;
    bool is_triggered;
    bool is_active;
    uint8_t lenght;

    uint8_t wave_pattern[32];
    uint16_t period_value;
    float idx;
    uint8_t volume;
}wave_channel;

typedef struct {
    pulse_channel ch1;
    pulse_channel ch2;
    wave_channel ch3;
    float pan[4];
}channels;

extern channels audio;

void AudioInputCallback_CH1(void *buffer, unsigned int frames);
void AudioInputCallback_CH2(void *buffer, unsigned int frames);
void AudioInputCallback_CH3(void *buffer, unsigned int frames);
void AudioInputCallback_CH4(void *buffer, unsigned int frames);

void Update_Audio(cpu *c);

#endif //CHILLYGB_APU_H
