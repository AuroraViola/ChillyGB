#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_CPU_H
#define CHILLYGB_CPU_H

typedef union {
    uint16_t reg16[4];
    uint8_t reg8[8];
}registers;

typedef struct {
    registers r;
    uint16_t sp;
    uint16_t pc;
    bool ime;
    bool ime_to_be_setted;

    uint8_t window_internal_line;
    bool dma_transfer;
    bool tilemap_write;
    bool tiles_write;
    bool need_bg_wn_reload;

    uint8_t memory[0x10000];
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
    uint32_t frame_tick;

    uint8_t is_scanline;
    bool is_frame;
}tick;

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
