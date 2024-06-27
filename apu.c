#include "apu.h"
#include "cpu.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>

audio ch[4];
audio_master master;

uint8_t get_volume(cpu *c, uint16_t NRx2) {
    return c->memory[NRx2] >> 4;
}

uint8_t get_lenght(cpu *c, uint16_t NRx1) {
    return c->memory[NRx1] & 0x3f;
}

uint16_t get_periodvalue(cpu *c, uint16_t NRx3, uint16_t NRx4) {
    return (uint16_t) ((c->memory[NRx4] & 7) << 8) | c->memory[NRx3];
}

void Update_CH1(cpu *c) {
    if (ch[0].is_triggered) {
        ch[0].is_triggered = false;
        ch[0].lenght = get_lenght(c, NR11);
        ch[0].period_value = get_periodvalue(c, NR13, NR14);
        ch[0].volume = get_volume(c, NR12);
    }
    if ((c->memory[NR14] & 64) != 0 && c->sound_lenght) {
        if (ch[0].lenght < 64)
            ch[0].lenght += 1;
        if (ch[0].lenght >= 64)
            ch[0].volume = 0;
    }
    if ((c->memory[NR12] & 7) != 0 && c->envelope_sweep) {
        if ((c->envelope_sweep_pace % (c->memory[NR12] & 7)) == 0) {
            if ((c->memory[NR12] & 8) != 0) {
                if (ch[0].volume < 15)
                    ch[0].volume += 1;
            } else {
                if (ch[0].volume > 0)
                    ch[0].volume -= 1;
            }
        }
    }
    if ((c->memory[NR10] & 112) != 0 && c->freq_sweep) {
        if ((c->freq_sweep_pace % ((c->memory[NR10] & 112) >> 4)) == 0) {
            if ((c->memory[NR10] & 8) != 0) {
                ch[0].period_value -= (ch[0].period_value / (1 << (c->memory[NR10] & 7)));
            } else {
                ch[0].period_value += (ch[0].period_value / (1 << (c->memory[NR10] & 7)));
            }
        }
    }
}

void Update_CH2(cpu *c) {
    if (ch[1].is_triggered) {
        ch[1].is_triggered = false;
        ch[1].lenght = get_lenght(c, NR21);
        ch[1].period_value = get_periodvalue(c, NR23, NR24);
        ch[1].volume = get_volume(c, NR22);
    }
    if ((c->memory[NR24] & 64) != 0 && c->sound_lenght) {
        if (ch[1].lenght < 64)
            ch[1].lenght += 1;
        if (ch[1].lenght >= 64)
            ch[1].volume = 0;
    }
    if ((c->memory[NR22] & 7) != 0 && c->envelope_sweep) {
        if ((c->envelope_sweep_pace % (c->memory[NR22] & 7)) == 0) {
            if ((c->memory[NR22] & 8) != 0) {
                if (ch[1].volume < 15)
                    ch[1].volume += 1;
            } else {
                if (ch[1].volume > 0)
                    ch[1].volume -= 1;
            }
        }
    }
}

void Update_Audio(cpu *c) {
    if ((c->memory[NR12] & 0xf8) != 0)
        Update_CH1(c);
    else
        ch[0].volume = 0;
    if ((c->memory[NR22] & 0xf8) != 0)
        Update_CH2(c);
    else
        ch[1].volume = 0;
    c->sound_lenght = false;
    c->envelope_sweep = false;
    c->freq_sweep = false;
}

void AudioInputCallback_CH1(void *buffer, unsigned int frames) {
    int frequency = 131072 / (2048 - ch[0].period_value);
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        int8_t sinemap;
        if (ch[0].idx >= 0.5f)
            sinemap = 1;
        else
            sinemap = 0;
        d[i] = (short)(((float)(ch[0].volume) * 256) * (PI * sinemap));
        ch[0].idx += incr;
        if (ch[0].idx > 1.0f)
            ch[0].idx -= 1.0f;
    }
}

void AudioInputCallback_CH2(void *buffer, unsigned int frames) {
    int frequency = 131072 / (2048 - ch[1].period_value);
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        int8_t sinemap;
        if (ch[1].idx >= 0.5f)
            sinemap = 1;
        else
            sinemap = 0;
        d[i] = (short)(((float)(ch[1].volume) * 256) * (PI * sinemap));
        ch[1].idx += incr;
        if (ch[1].idx > 1.0f)
            ch[1].idx -= 1.0f;
    }
}
