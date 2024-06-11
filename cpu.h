#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_CPU_H
#define CHILLYGB_CPU_H

typedef union {
    uint16_t reg16[4];
    uint8_t reg8[8];
}registers;

typedef struct {
    uint8_t data[256][0x4000];
    uint8_t type;
    uint16_t banks;
    uint16_t bank_select;
}cartridge;

typedef struct {
    registers r;
    uint16_t sp;
    uint16_t pc;
    bool ime;
    uint8_t ime_to_be_setted;

    uint8_t window_internal_line;
    bool dma_transfer;
    bool tilemap_write;
    bool tiles_write;
    bool need_bg_wn_reload;
    bool need_sprites_reload;
    bool reset_sprite_display;


    cartridge cart;
    uint8_t memory[0x10000];

    uint8_t apu_div;
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

typedef struct {
    uint64_t t_states;
    int32_t scan_line_tick;
    int32_t window_line_tick;
    uint32_t divider_register;
    uint32_t tima_counter;

    uint32_t div_apu_tick;

    uint8_t is_scanline;
    bool is_frame;
}tick;

enum memregs {
    // Joypad
    P1 = 0xff00,

    // Serial
    SB = 0xff01,
    SC = 0xff02,

    // Timer
    DIV = 0xff04,
    TIMA = 0xff05,
    TMA = 0xff06,
    TAC = 0xff07,

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

void execute(cpu *c, tick *t);

#endif //CHILLYGB_CPU_H
