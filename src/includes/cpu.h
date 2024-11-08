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
    bool has_latched;
    uint16_t x;
    uint16_t y;
    uint16_t x_latched;
    uint16_t y_latched;
}mbc7_accel;

typedef struct {
    bool DO;
    bool DI;
    bool CLK;
    bool CS;

    bool write_enable;
    uint16_t read_bits;
    uint16_t command;
    uint8_t argument_bits_left;
}mbc7_eeprom;

typedef struct {
    uint8_t data[512][0x4000];

    uint16_t banks;
    uint16_t bank_select;

    uint8_t mbc;
    bool has_ram;
    bool has_rtc;
    bool has_battery;
    bool has_rumble;

    uint8_t ram[16][0x2000];
    uint8_t banks_ram;
    uint8_t bank_select_ram;
    bool ram_enable;
    bool ram_enable2;
    bool mbc1mode;

    mbc7_accel accel;
    mbc7_eeprom eeprom;

    rtc_clock rtc;
}cartridge;

typedef struct {
    uint8_t data[0x900];
    bool is_enabled;
}rom;

typedef struct {
    uint16_t source;
    uint16_t destination;
    bool mode;
    uint16_t status;
    uint16_t lenght;
    bool finished;
}dma;

typedef struct {
    registers r;
    uint16_t sp;
    uint16_t pc;
    bool ime;
    uint8_t ime_to_be_setted;
    bool is_halted;
    bool gdma_halt;
    bool first_halt;

    bool double_speed;
    bool armed;

    bool is_color;
    bool cgb_mode;

    cartridge cart;
    uint8_t memory[0x10000];
    uint8_t wram[8][0x1000];
    uint8_t wram_bank;

    uint8_t apu_div;
    bool envelope_sweep;
    uint8_t envelope_sweep_pace;
    bool freq_sweep;
    uint8_t freq_sweep_pace;
    bool sound_lenght;

    rom bootrom;

    dma hdma;
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
bool execute(cpu *c);
void hdma_transfer(cpu *c);
void add_ticks(cpu *c, uint16_t ticks);
void dma_transfer(cpu *c);
bool load_bootrom(rom *bootrom);

#endif //CHILLYGB_CPU_H
