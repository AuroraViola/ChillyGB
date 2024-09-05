#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "settings.h"

#ifndef CHILLYGB_CPU_H
#define CHILLYGB_CPU_H

typedef union {
    uint16_t reg16[4];
    uint8_t reg8[8];
}registers;

typedef struct {
    uint32_t time;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint16_t days;

    bool day_carry;
    bool is_halted;
    bool about_to_latch;
}rtc_clock;

typedef struct {
    uint8_t data[512][0x4000];

    uint8_t type;
    uint16_t banks;
    uint16_t bank_select;

    uint8_t ram[16][0x2000];
    uint8_t banks_ram;
    uint8_t bank_select_ram;
    bool ram_enable;
    bool mbc1mode;

    rtc_clock rtc;
}cartridge;

typedef struct {
    uint8_t data[0x100];
    bool is_enabled;
}rom;

typedef struct {
    registers r;
    uint16_t sp;
    uint16_t pc;
    bool ime;
    uint8_t ime_to_be_setted;
    bool is_halted;
    bool first_halt;

    cartridge cart;
    uint8_t memory[0x10000];

    uint8_t apu_div;
    bool envelope_sweep;
    uint8_t envelope_sweep_pace;
    bool freq_sweep;
    uint8_t freq_sweep_pace;
    bool sound_lenght;

    rom bootrom;
}cpu;

typedef struct {
    uint8_t imm8;
    uint16_t imm16;

    uint8_t condition;

    uint8_t dest_source_r16mem;

    uint8_t operand_r8;
    uint8_t operand_r16;
    uint8_t operand_stk_r8;
    uint8_t operand_stk_r16;

    uint8_t tgt3;
}parameters;

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

    BCPS = 0xff68,
    BCPD = 0xff69,
    OCPS = 0xff6a,
    OCPD = 0xff6b,

    VBK = 0xff4f,

    IE = 0xffff,
};

enum reg8 {
    B = 1,
    C = 0,
    D = 3,
    E = 2,
    H = 5,
    L = 4,
    A = 7,
    F = 6,
};

enum reg16 {
    BC = 0,
    DE = 1,
    HL = 2,
    AF = 3,
};

enum flag {
    flagZ = 128,
    flagN = 64,
    flagH = 32,
    flagC = 16,
};

enum cond {
    condC = 3,
    condNC = 2,
    condZ = 1,
    condNZ = 0,
};

extern float speed_mult;

void initialize_cpu_memory_no_bootrom(cpu *c, settings *s);
void initialize_cpu_memory(cpu *c, settings *s);
void execute(cpu *c);
void add_ticks(cpu *c, uint16_t ticks);
void dma_transfer(cpu *c);
bool load_bootrom(rom *bootrom);

#endif //CHILLYGB_CPU_H
