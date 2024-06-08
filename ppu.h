#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_PPU_H
#define CHILLYGB_PPU_H

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t tile[8][8];
    uint8_t tile_16[16][8];

    bool priority;
    bool palette;
}sprite;

typedef struct {
    uint8_t display[144][160];
    uint8_t background[256][256];
    uint8_t window[256][256];
    uint8_t tiles[256][8][8];
    uint8_t tilemap[2][1024];
    sprite sprites[40];
    uint8_t sprite_display[144][160];
}ppu;

void load_display(cpu *c, ppu *p);

#endif //CHILLYGB_PPU_H
