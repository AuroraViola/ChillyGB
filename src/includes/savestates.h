#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"

#ifndef CHILLYGB_SAVESTATES_H
#define CHILLYGB_SAVESTATES_H

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
    bool gdma_halt;
    bool first_halt;
    uint8_t memory[0x10000];
    uint8_t apu_div;

    uint8_t wram[8][0x1000];
    uint8_t wram_bank;

    bool double_speed;
    bool armed;

    dma hdma;

    savestate_cart cart;

    ppu savestate_ppu;

    timer timer_save;
}savestate;

void save_state(cpu *c, char rom_name[256]);
void load_state(cpu *c, char rom_name[256]);

#endif //CHILLYGB_SAVESTATES_H
