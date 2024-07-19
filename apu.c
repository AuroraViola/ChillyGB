#include "apu.h"
#include "cpu.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

channels audio;

uint8_t outputlevels[] = {4, 0, 1, 2};

void load_audio_streams() {
    audio.ch1.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch1.stream, AudioInputCallback_CH1);
    PlayAudioStream(audio.ch1.stream);

    audio.ch2.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch2.stream, AudioInputCallback_CH2);
    PlayAudioStream(audio.ch2.stream);

    audio.ch3.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch3.stream, AudioInputCallback_CH3);
    PlayAudioStream(audio.ch3.stream);

    audio.ch4.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch4.stream, AudioInputCallback_CH4);
    PlayAudioStream(audio.ch4.stream);
}
void tick_lfsr() {
    uint16_t bit0 = audio.ch4.lfsr & 1;
    uint16_t bit1 = (audio.ch4.lfsr >> 1) & 1;
    if (bit0 == bit1) {
        audio.ch4.lfsr |= 0x8000;
        if (audio.ch4.bit_7_mode)
            audio.ch4.lfsr |= 0x0080;
    }
    else {
        if (audio.ch4.bit_7_mode)
            audio.ch4.lfsr &= 0xff7f;
    }
    audio.ch4.current_bit = audio.ch4.lfsr & 1;
    audio.ch4.lfsr >>= 1;
}

uint8_t get_volume(cpu *c, uint16_t NRx2) {
    return c->memory[NRx2] >> 4;
}

uint8_t get_lenght(cpu *c, uint16_t NRx1) {
    return c->memory[NRx1] & 0x3f;
}

uint16_t get_periodvalue(cpu *c, uint16_t NRx3, uint16_t NRx4) {
    return (uint16_t) ((c->memory[NRx4] & 7) << 8) | c->memory[NRx3];
}

uint8_t get_wave_value(float idx) {
    idx *= 32;
    return audio.ch3.wave_pattern[(uint8_t)(idx)] >> audio.ch3.volume;
}

uint16_t get_wave_duty(cpu *c, uint16_t NRx1) {
    uint8_t duty = c->memory[NRx1] >> 6;
    switch (duty) {
        case 0:
            duty = 1;
            break;
        case 1:
            duty = 2;
            break;
        case 2:
            duty = 4;
            break;
        case 3:
            duty = 6;
            break;
    }
    return duty;
}
uint8_t get_wave_duty_ch1(float idx) {
    idx *= 8;
    return ((uint8_t)(idx) < audio.ch1.wave_duty) ? 1 : 0;
}

uint8_t get_wave_duty_ch2(float idx) {
    idx *= 8;
    return ((uint8_t)(idx) < audio.ch2.wave_duty) ? 1 : 0;
}

void create_wave_pattern(cpu *c) {
    for (int i = 0; i < 16; i++) {
        audio.ch3.wave_pattern[i * 2] = c->memory[0xff30+i] >> 4;
    }
    for (int i = 0; i < 16; i++) {
        audio.ch3.wave_pattern[(i * 2) + 1] = c->memory[0xff30+i] & 0xf;
    }
}

void Update_CH1(cpu *c) {
    audio.ch1.period_value = get_periodvalue(c, NR13, NR14);
    if (audio.ch1.is_triggered) {
        audio.ch1.is_triggered = false;
        audio.ch1.wave_duty = get_wave_duty(c, NR11);
        audio.ch1.volume = get_volume(c, NR12);
        audio.ch1.sweep_pace = (c->memory[NR12] & 7);
        audio.ch1.env_dir = (c->memory[NR12] & 8);
    }
    if ((c->memory[NR14] & 64) != 0 && c->sound_lenght) {
        if (audio.ch1.lenght < 63)
            audio.ch1.lenght += 1;
        else
            audio.ch1.is_active = false;
    }
    if (audio.ch1.sweep_pace != 0 && c->envelope_sweep && audio.ch1.is_active) {
        if ((c->envelope_sweep_pace % audio.ch1.sweep_pace) == 0) {
            if (audio.ch1.env_dir != 0) {
                if (audio.ch1.volume < 15)
                    audio.ch1.volume += 1;
            } else {
                if (audio.ch1.volume > 0)
                    audio.ch1.volume -= 1;
            }
        }
    }
    if ((c->memory[NR10] & 112) != 0 && c->freq_sweep) {
        if ((c->freq_sweep_pace % ((c->memory[NR10] & 112) >> 4)) == 0) {
            if ((c->memory[NR10] & 8) != 0) {
                audio.ch1.period_value -= (audio.ch1.period_value / (1 << (c->memory[NR10] & 7)));
                c->memory[NR13] = audio.ch1.period_value & 0xff;
                c->memory[NR14] = (c->memory[NR14] & 0xf8) | (audio.ch1.period_value >> 8);
            } else {
                audio.ch1.period_value += (audio.ch1.period_value / (1 << (c->memory[NR10] & 7)));
                if (audio.ch1.period_value >= 2048)
                    audio.ch1.is_active = false;
                c->memory[NR13] = audio.ch1.period_value & 0xff;
                c->memory[NR14] = (c->memory[NR14] & 0xf8) | (audio.ch1.period_value >> 8);
            }
        }
    }
}

void Update_CH2(cpu *c) {
    audio.ch2.period_value = get_periodvalue(c, NR23, NR24);
    if (audio.ch2.is_triggered) {
        audio.ch2.is_triggered = false;
        audio.ch2.wave_duty = get_wave_duty(c, NR21);
        audio.ch2.volume = get_volume(c, NR22);
        audio.ch2.sweep_pace = (c->memory[NR22] & 7);
        audio.ch2.env_dir = (c->memory[NR22] & 8);
    }
    if ((c->memory[NR24] & 64) != 0 && c->sound_lenght) {
        if (audio.ch2.lenght < 63)
            audio.ch2.lenght += 1;
        else
            audio.ch2.is_active = false;
    }
    if (audio.ch2.sweep_pace != 0 && c->envelope_sweep && audio.ch2.is_active) {
        if ((c->envelope_sweep_pace % audio.ch2.sweep_pace) == 0) {
            if (audio.ch2.env_dir != 0) {
                if (audio.ch2.volume < 15)
                    audio.ch2.volume += 1;
            } else {
                if (audio.ch2.volume > 0)
                    audio.ch2.volume -= 1;
            }
        }
    }
}

void Update_CH3(cpu *c) {
    audio.ch3.period_value = get_periodvalue(c, NR33, NR34);
    audio.ch3.volume = outputlevels[(c->memory[NR32] & 96) >> 5];
    if (audio.ch3.is_triggered) {
        audio.ch3.is_triggered = false;
        create_wave_pattern(c);
    }
    if ((c->memory[NR34] & 64) != 0 && c->sound_lenght) {
        if (audio.ch3.lenght < 255)
            audio.ch3.lenght += 1;
        else {
            audio.ch3.is_active = false;
        }
    }
}

void Update_CH4(cpu *c) {
    uint8_t clk_div = c->memory[NR43] & 7;
    uint8_t clk_shift = c->memory[NR43] >> 4;
    if (clk_div != 0)
        audio.ch4.period_value = clk_div * (1 << clk_shift);
    else
        audio.ch4.period_value = (1 << clk_shift) / 2;

    if ((c->memory[NR43] & 8) != 0)
        audio.ch4.bit_7_mode = true;
    else
        audio.ch4.bit_7_mode = false;

    if (audio.ch4.is_triggered) {
        audio.ch4.is_triggered = false;
        audio.ch4.volume = get_volume(c, NR42);
        audio.ch4.sweep_pace = (c->memory[NR42] & 7);
        audio.ch4.env_dir = (c->memory[NR42] & 8);
        audio.ch4.lfsr = 0;
    }
    if ((c->memory[NR44] & 64) != 0 && c->sound_lenght) {
        if (audio.ch4.lenght < 63)
            audio.ch4.lenght += 1;
        else {
            audio.ch4.is_active = false;
        }
    }

    if (audio.ch4.sweep_pace != 0 && c->envelope_sweep && audio.ch4.is_active) {
        if ((c->envelope_sweep_pace % audio.ch4.sweep_pace) == 0) {
            if (audio.ch4.env_dir != 0) {
                if (audio.ch4.volume < 15)
                    audio.ch4.volume += 1;
            } else {
                if (audio.ch4.volume > 0)
                    audio.ch4.volume -= 1;
            }
        }
    }
}


void Update_Audio(cpu *c) {
    if (audio.is_on) {
        if ((c->memory[NR12] & 0xf8) != 0)
            Update_CH1(c);
        else
            audio.ch1.is_active = false;
        SetAudioStreamPan(audio.ch1.stream, audio.pan[0]);
        if ((c->memory[NR22] & 0xf8) != 0)
            Update_CH2(c);
        else
            audio.ch2.is_active = false;
        SetAudioStreamPan(audio.ch2.stream, audio.pan[1]);
        if ((c->memory[NR30] & 128) != 0)
            Update_CH3(c);
        else
            audio.ch3.is_active = false;
        SetAudioStreamPan(audio.ch3.stream, audio.pan[2]);
        if ((c->memory[NR42] & 0xf8) != 0)
            Update_CH4(c);
        else
            audio.ch4.is_active = false;
        SetAudioStreamPan(audio.ch4.stream, audio.pan[3]);
        c->sound_lenght = false;
        c->envelope_sweep = false;
        c->freq_sweep = false;
    }
}

void AudioInputCallback_CH1(void *buffer, unsigned int frames) {
    int frequency = 131072 / (2048 - audio.ch1.period_value);
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        if (audio.ch1.is_active && audio.pan[0] != 0.6f)
            d[i] = (short)(audio.ch1.volume * audio.volume * get_wave_duty_ch1(audio.ch1.idx) * 3);
        else
            d[i] = 0;
        audio.ch1.idx += incr;
        while (audio.ch1.idx > 1.0f)
            audio.ch1.idx -= 1.0f;
    }
}

void AudioInputCallback_CH2(void *buffer, unsigned int frames) {
    int frequency = 131072 / (2048 - audio.ch2.period_value);
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        if (audio.ch2.is_active && audio.pan[1] != 0.6f)
            d[i] = (short)(audio.ch2.volume * audio.volume * get_wave_duty_ch2(audio.ch2.idx) * 3);
        else
            d[i] = 0;
        audio.ch2.idx += incr;
        while (audio.ch2.idx > 1.0f)
            audio.ch2.idx -= 1.0f;
    }
}

void AudioInputCallback_CH3(void *buffer, unsigned int frames) {
    int frequency = 65536 / (2048 - audio.ch3.period_value);
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        if (audio.ch3.is_active && audio.pan[2] != 0.6f)
            d[i] = (short)(audio.volume * get_wave_value(audio.ch3.idx) * 3);
        else
            d[i] = 0;
        audio.ch3.idx += incr;
        while (audio.ch3.idx > 1.0f)
            audio.ch3.idx -= 1.0f;
    }
}

void AudioInputCallback_CH4(void *buffer, unsigned int frames) {
    int frequency = 262144;
    if (audio.ch4.period_value != 0)
        frequency /= audio.ch4.period_value;
    float incr = frequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        if (audio.ch4.is_active && audio.pan[3] != 0.6f) {
            d[i] = (short)(audio.ch4.volume * audio.volume * audio.ch4.current_bit * 3);
        }
        else
            d[i] = 0;
        audio.ch4.idx += incr;
        while (audio.ch4.idx > 1.0f) {
            audio.ch4.idx -= 1.0f;
            tick_lfsr();
        }
    }
}
