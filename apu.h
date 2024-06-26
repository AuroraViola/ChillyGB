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

    uint16_t period_value;
    float idx;
    uint8_t volume;
}audio;

typedef struct {
    uint8_t volume;
    float pan[4];
}audio_master;

extern audio ch[4];
extern audio_master master;

void AudioInputCallback_CH1(void *buffer, unsigned int frames);
void AudioInputCallback_CH2(void *buffer, unsigned int frames);
void AudioInputCallback_CH3(void *buffer, unsigned int frames);
void AudioInputCallback_CH4(void *buffer, unsigned int frames);

void Update_Audio(cpu *c);

#endif //CHILLYGB_APU_H
