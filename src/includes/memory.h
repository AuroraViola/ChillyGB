#include "cpu.h"

#ifndef CHILLYGB_MEMORY_H
#define CHILLYGB_MEMORY_H

uint8_t get_mem(cpu *c, uint16_t addr);
void set_mem(cpu *c, uint16_t addr, uint8_t value);

enum mem_regs {
    // Joypad
    JOYP = 0xff00,

    // Serial
    SB = 0xff01,
    SC = 0xff02,

    // Timer
    DIV = 0xff04,
    TIMA = 0xff05,
    TMA = 0xff06,
    TAC = 0xff07,

    IF = 0xff0f,

    // Audio
    NR10 = 0xff10,
    NR11 = 0xff11,
    NR12 = 0xff12,
    NR13 = 0xff13,
    NR14 = 0xff14,

    NR21 = 0xff16,
    NR22 = 0xff17,
    NR23 = 0xff18,
    NR24 = 0xff19,

    NR30 = 0xff1a,
    NR31 = 0xff1b,
    NR32 = 0xff1c,
    NR33 = 0xff1d,
    NR34 = 0xff1e,

    NR41 = 0xff20,
    NR42 = 0xff21,
    NR43 = 0xff22,
    NR44 = 0xff23,

    NR50 = 0xff24,
    NR51 = 0xff25,
    NR52 = 0xff26,

    LCDC = 0xff40,
    STAT = 0xff41,
    SCY = 0xff42,
    SCX = 0xff43,
    LY = 0xff44,
    LYC = 0xff45,
    DMA = 0xff46,
    BGP = 0xff47,
    OBP0 = 0xff48,
    OBP1 = 0xff49,
    WY = 0xff4a,
    WX = 0xff4b,

    KEY0 = 0xff4c,
    KEY1 = 0xff4d,

    VBK = 0xff4f,

    HDMA1 = 0xff51,
    HDMA2 = 0xff52,
    HDMA3 = 0xff53,
    HDMA4 = 0xff54,
    HDMA5 = 0xff55,

    BCPS = 0xff68,
    BCPD = 0xff69,
    OCPS = 0xff6a,
    OCPD = 0xff6b,
    OPRI = 0xff6c,

    SVBK = 0xff70,


    IE = 0xffff,
};

#endif //CHILLYGB_MEMORY_H
