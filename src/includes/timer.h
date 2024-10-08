#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_TIMER_H
#define CHILLYGB_TIMER_H

typedef struct {
    bool is_tac_on;
    uint32_t timer_global;
    uint32_t t_states;
    uint8_t module;
    uint8_t tma;
    uint8_t tima;
    bool delay;
    bool reset_timer;

    int32_t scanline_timer;
    int32_t lcdoff_timer;

    int32_t rtc_timer;

    int16_t serial_timer;
}timer;

extern timer timer1;

#endif //CHILLYGB_TIMER_H
