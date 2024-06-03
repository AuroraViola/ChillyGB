#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_PPU_H
#define CHILLYGB_PPU_H

typedef struct {
    uint8_t display[144][160];
    uint8_t background[256][256];
    uint8_t window[256][256];
    uint8_t sprites[40][4];
}ppu;

void load_display(cpu *c, ppu *p);

#endif //CHILLYGB_PPU_H
