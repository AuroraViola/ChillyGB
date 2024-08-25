#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"

#ifndef CHILLYGB_SAVESTATES_H
#define CHILLYGB_SAVESTATES_H

typedef struct {
    uint8_t scanline;
    uint8_t mode;
    uint8_t bgp[4];
    uint8_t obp[2][4];

    bool is_on;
    bool bg_enable;
    bool window_enable;
    bool obj_enable;

    bool bg_tiles;
    bool bg_tilemap;
    bool window_tilemap;
    bool obj_size;

    bool lyc_select;
    uint8_t mode_select;
    bool ly_eq_lyc;
}savestate_ppu;

typedef struct {
    uint16_t bank_select;
    uint8_t bank_select_ram;

    uint8_t ram[16][0x2000];
    bool ram_enable;
    bool mbc1mode;

    rtc_clock rtc;
}savestate_cart;

typedef struct {
    int version;
    registers r;
    uint16_t sp;
    uint16_t pc;
    bool ime;
    uint8_t ime_to_be_setted;
    bool is_halted;
    bool first_halt;
    uint8_t memory[0x10000];
    uint8_t tilemaps[2][1024];
    uint8_t apu_div;

    savestate_cart cart;

    savestate_ppu ppu;

    timer timer_save;
}savestate;

void save_state(cpu *c, char rom_name[256]);
void load_state(cpu *c, char rom_name[256]);

#endif //CHILLYGB_SAVESTATES_H
