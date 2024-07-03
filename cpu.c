#include "cpu.h"
#include "apu.h"
#include "opcodes.h"
#include "raylib.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

const uint8_t rst_vec[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
const uint16_t clock_select[] = {1024, 16, 64, 256};

uint8_t stretch_number(uint8_t num) {
    uint8_t t = (num | (num << 2)) & 0b00110011;
    return ((t + (t | 0b1010101)) ^ 0b1010101) & 0b11111111;
}

void initialize_cpu_memory(cpu *c) {
    srand(time(NULL));
    c->r.reg8[A] = 0x01;
    c->r.reg8[B] = 0x00;
    c->r.reg8[C] = 0x13;
    c->r.reg8[D] = 0x00;
    c->r.reg8[E] = 0xd8;
    c->r.reg8[F] = 0xb0;
    c->r.reg8[H] = 0x01;
    c->pc = 0x100;
    c->sp = 0xfffe;
    c->ime = false;
    c->ime_to_be_setted = 0;
    c->need_bg_wn_reload = true;
    c->tilemap_write = true;
    c->tiles_write = true;
    c->need_sprites_reload = true;

    c->memory[JOYP] = 0xcf;
    c->memory[SB] = 0x00;
    c->memory[SC] = 0x7e;
    c->memory[DIV] = 0xac;
    c->memory[TIMA] = 0x00;
    c->memory[TMA] = 0x00;
    c->memory[TAC] = 0xf8;
    c->memory[IF] = 0xe1;

    audio.is_on = true;
    audio.ch1.is_active = true;
    audio.ch2.is_active = false;
    audio.ch3.is_active = false;
    audio.ch4.is_active = false;
    audio.ch4.period_value = 8;
    c->memory[NR10] = 0x80;
    c->memory[NR11] = 0xbf;
    c->memory[NR12] = 0xf3;
    c->memory[NR13] = 0xff;
    c->memory[NR14] = 0xbf;

    c->memory[NR21] = 0x3f;
    c->memory[NR22] = 0x00;
    c->memory[NR23] = 0xff;
    c->memory[NR24] = 0xbf;

    c->memory[NR30] = 0x7f;
    c->memory[NR31] = 0xff;
    c->memory[NR32] = 0x9f;
    c->memory[NR33] = 0xff;
    c->memory[NR34] = 0xbf;

    c->memory[NR41] = 0xff;
    c->memory[NR42] = 0x00;
    c->memory[NR43] = 0x00;
    c->memory[NR44] = 0xbf;

    c->memory[NR50] = 0x77;
    c->memory[NR51] = 0xf3;
    c->memory[NR52] = 0xf1;

    c->memory[LCDC] = 0x91;
    c->memory[STAT] = 0x89;
    c->memory[SCY] = 0x00;
    c->memory[SCX] = 0x00;
    c->memory[LY] = 0x91;
    c->memory[LYC] = 0x00;
    c->memory[DMA] = 0xff;
    c->memory[BGP] = 0xfc;
    c->memory[OBP0] = 0xff;
    c->memory[OBP1] = 0xff;
    c->memory[WY] = 0x00;
    c->memory[WX] = 0x00;
    c->memory[IE] = 0x00;

    // Initialize WRAM
    for (uint16_t i = 0xc000; i <= 0xdfff; i++) {
        c->memory[i] = rand();
    }

    // Initialize HRAM
    for (uint16_t i = 0xff80; i <= 0xfffe; i++) {
        c->memory[i] = rand();
    }

    // Initialize VRAM
    for (uint16_t i = 0x8000; i <= 0x9fff; i++) {
        c->memory[i] = 0;
    }

    // Initialize Background tiles
    for (uint16_t i = 0; i < 12; i++) {
        c->memory[0x9904 + i] = i+1;
        c->memory[0x9924 + i] = i+13;
    }
    c->memory[0x9910] = 0x19;

    // Initialize tiles data
    uint8_t logo_tiles_initial[24][2];
    uint8_t logo_tiles[24][4];

    for (uint16_t i = 0; i < 24; i++) {
        logo_tiles_initial[i][0] = c->cart.data[0][0x104 + (i*2)];
        logo_tiles_initial[i][1] = c->cart.data[0][0x104 + (i*2) + 1];
    }

    for (uint16_t i = 0; i < 24; i++) {
        logo_tiles[i][0] = stretch_number(logo_tiles_initial[i][0] >> 4);
        logo_tiles[i][1] = stretch_number(logo_tiles_initial[i][0] & 0xf);
        logo_tiles[i][2] = stretch_number(logo_tiles_initial[i][1] >> 4);
        logo_tiles[i][3] = stretch_number(logo_tiles_initial[i][1] & 0xf);
    }

    for (uint16_t i = 0; i < 24; i++) {
        for (uint16_t j = 0; j < 4; j ++) {
            c->memory[0x8010 + (i << 4) + (j*4)] = logo_tiles[i][j];
            c->memory[0x8010 + (i << 4) + (j*4)+2] = logo_tiles[i][j];
        }
    }

    uint8_t r_tile[] = {0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c};
    for (uint16_t i = 0; i < 8; i++) {
        c->memory[0x8190 + (i * 2)] = r_tile[i];
    }
}

void add_ticks(cpu *c, tick *t, uint16_t ticks){
    t->t_states += ticks;
    t->scan_line_tick += ticks;

    // Mode 2
    if (t->scan_line_tick < 80) {
        if ((c->memory[LY] < 144) && ((c->memory[STAT] & 3) != 2)) {
            c->memory[STAT] = (c->memory[STAT] & 252) | 2;
            if ((c->memory[STAT] & 32) != 0 && c->mode2_select == false) {
                c->mode2_select = true;
                c->memory[IF] |= 2;
            }
        }
    }
    // Mode 3
    if (t->scan_line_tick < 250 && t->scan_line_tick > 80) {
        if ((c->memory[LY] < 144) && ((c->memory[STAT] & 3) != 0))
            c->memory[STAT] = (c->memory[STAT] & 252) | 3;
    }
    // Mode 0
    if (t->scan_line_tick >= 250) {
        if ((c->memory[LY] < 144) && ((c->memory[STAT] & 3) != 0)) {
            c->memory[STAT] = (c->memory[STAT] & 252);
            if ((c->memory[STAT] & 8) != 0 && c->mode0_select == false) {
                c->mode0_select = true;
                c->memory[IF] |= 2;
            }
        }
        t->is_scanline++;
    }

    if (t->scan_line_tick >= 456) {
        t->scan_line_tick -= 456;
        c->memory[LY] += 1;
        c->lyc_select = false;
        c->mode0_select = false;
        c->mode2_select = false;
        if (((c->memory[WY] < c->memory[LY]) && (c->memory[LCDC] & 32) != 0) && ((c->memory[WX] - 7) < 160)) {
            c->window_internal_line += 1;
        }

        if (c->memory[LY] == 144) {
            c->memory[IF] |= 1;
            t->is_frame = true;
            c->memory[STAT] = (c->memory[STAT] & 252) | 1;
        } else if (c->memory[LY] >= 153) {
            t->scan_line_tick -= 456;
            c->memory[LY] = 0;
            c->window_internal_line = 0;
        }
    }

    if (c->memory[LY] == c->memory[LYC]) {
        c->memory[STAT] |= 4;
        if ((c->memory[STAT] & 64) != 0 && c->lyc_select == false) {
            c->lyc_select = true;
            c->memory[IF] |= 2;
        }
    }
    else {
        c->memory[STAT] &= 251;
    }


    if ((c->memory[TAC] & 4) != 0) {
        t->tima_counter += ticks;
        if (t->tima_counter > clock_select[c->memory[TAC] & 3]) {
            t->tima_counter = 0;
            c->memory[TIMA] += 1;
            if (c->memory[TIMA] == 0) {
                c->memory[TIMA] = c->memory[TMA];
                c->memory[IF] |= 4;
            }

        }
    }

    t->divider_register += ticks;
    if (t->divider_register >= 256) {
        t->divider_register -= 256;
        if ((c->memory[DIV] & 16) > ((c->memory[DIV] + 1) & 16)) {
            c->apu_div++;
            if (c->apu_div % 2 == 0) {
                c->sound_lenght = true;
            }
            if (c->apu_div % 4 == 0) {
                c->freq_sweep = true;
                c->freq_sweep_pace++;
            }
            if (c->apu_div % 8 == 0) {
                c->envelope_sweep = true;
                c->envelope_sweep_pace++;
            }
        }
        c->memory[DIV] += 1;
    }
}

void run_interrupt(cpu *c, tick *t) {
    if ((c->memory[IE] & c->memory[IF]) != 0) {
        if (((c->memory[IE] & 1) == 1) && ((c->memory[IF] & 1) == 1)) {
            c->memory[IF] &= 0b11111110;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t)(c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t)(c->pc);
            c->pc = 0x40;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[IE] & 2) == 2) && ((c->memory[IF] & 2) == 2)) {
            c->memory[IF] &= 0b11111101;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x48;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[IE] & 4) == 4) && ((c->memory[IF] & 4) == 4)) {
            c->memory[IF] &= 0b11111011;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x50;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[IE] & 8) == 8) && ((c->memory[IF] & 8) == 8)) {
            c->memory[IF] &= 0b11110111;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x58;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[IE] & 16) == 16) && ((c->memory[IE] & 16) == 16)) {
            c->memory[IF] &= 0b11101111;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x60;
            add_ticks(c, t, 20);
        }
    }
}

void dma_transfer(cpu *c, tick *t) {
    uint16_t to_transfer = (uint16_t)(c->memory[DMA]) << 8;
    for (int i = 0; i < 160; i++) {
        c->memory[0xfe00 + i] = c->memory[to_transfer + i];
    }
    //add_ticks(c, t, 640);
}

uint8_t execute_instruction(cpu *c) {
    parameters p = {};

    uint8_t opcode = get_mem(c, c->pc);
    p.imm8 = get_mem(c, (c->pc+1));
    p.imm16 = ((uint16_t)get_mem(c, (c->pc+2)) << 8) | get_mem(c, (c->pc+1));

    p.condition = (opcode & 0b00011000) >> 3;

    p.dest_source_r16mem = (opcode & 0b00110000) >> 4;

    if (opcode != 0xcb)
        p.operand_r8 = opcode & 0b00000111;
    else
        p.operand_r8 = p.imm8 & 0b00000111;

    p.operand_r16 = (opcode & 0b00110000) >> 4;
    p.operand_stk_r8 = (opcode & 0b00111000) >> 3;
    p.operand_stk_r16 = (opcode & 0b00110000) >> 4;

    p.tgt3 = rst_vec[(opcode & 0b00111000) >> 3];
    switch(opcode) {
        case 0xc3:
            return jp(c, &p);
        case 0x06: case 0x16: case 0x26: case 0x36: case 0x0e: case 0x1e: case 0x2e: case 0x3e:
            return ld_r8_imm8(c, &p);
        case 0x04: case 0x14: case 0x24: case 0x34: case 0x0c: case 0x1c: case 0x2c: case 0x3c:
            return inc_r8(c, &p);
        case 0x05: case 0x15: case 0x25: case 0x35: case 0x0d: case 0x1d: case 0x2d: case 0x3d:
            return dec_r8(c, &p);
        case 0xea:
            return ld_imm16_a(c, &p);
        case 0xfa:
            return ld_a_imm16(c, &p);
        case 0xfe:
            return cp_a_imm8(c, &p);
        case 0xda: case 0xc2: case 0xd2: case 0xca:
            return jp_cond(c, &p);
        case 0x01: case 0x11: case 0x21: case 0x31:
            return ld_r16_imm16(c, &p);
        case 0xe9:
            return jp_hl(c, &p);
        case 0x0a: case 0x1a: case 0x2a: case 0x3a:
            return ld_a_r16mem(c, &p);
        case 0x02: case 0x12: case 0x22: case 0x32:
            return ld_r16mem_a(c, &p);
        case 0x03: case 0x13: case 0x23: case 0x33:
            return inc_r16(c, &p);
        case 0x0b: case 0x1b: case 0x2b: case 0x3b:
            return dec_r16(c, &p);
        case 0x40 ... 0x75: case 0x77 ... 0x7f:
            return ld_r8_r8(c, &p);
        case 0xb0 ... 0xb7:
            return or_a_r8(c, &p);
        case 0x00:
            return nop(c, &p);
        case 0x08:
            return ld_imm16_sp(c, &p);
        case 0x20: case 0x30: case 0x28: case 0x38:
            return jr_cond(c, &p);
        case 0x18:
            return jr(c, &p);
        case 0xf3:
            return di(c, &p);
        case 0xe0:
            return ldh_imm8_a(c, &p);
        case 0xf0:
            return ldh_a_imm8(c, &p);
        case 0xcd:
            return call(c, &p);
        case 0xc9:
            return ret(c, &p);
        case 0xc1: case 0xd1: case 0xe1: case 0xf1:
            return pop(c, &p);
        case 0xc5: case 0xd5: case 0xe5: case 0xf5:
            return push(c, &p);
        case 0xa0 ... 0xa7:
            return and_a_r8(c, &p);
        case 0xa8 ... 0xaf:
            return xor_a_r8(c, &p);
        case 0xb8 ... 0xbf:
            return cp_a_r8(c, &p);
        case 0xe6:
            return and_a_imm8(c, &p);
        case 0xee:
            return xor_a_imm8(c, &p);
        case 0xc4: case 0xd4: case 0xcc: case 0xdc:
            return call_cond(c, &p);
        case 0xc6:
            return add_a_imm8(c, &p);
        case 0xd6:
            return sub_a_imm8(c, &p);
        case 0xcb:
            return prefix(c, &p);
        case 0x1f:
            return rra(c, &p);
        case 0xce:
            return adc_a_imm8(c, &p);
        case 0xc0: case 0xd0: case 0xc8: case 0xd8:
            return ret_cond(c, &p);
        case 0x09: case 0x19: case 0x29: case 0x39:
            return add_hl_r16(c, &p);
        case 0x27:
            return daa(c, &p);
        case 0xe2:
            return ldh_c_a(c, &p);
        case 0xf2:
            return ldh_a_c(c, &p);
        case 0xd9:
            return reti(c, &p);
        case 0x2f:
            return cpl(c, &p);
        case 0x3f:
            return ccf(c, &p);
        case 0x37:
            return scf(c, &p);
        case 0xf9:
            return ld_sp_hl(c, &p);
        case 0xfb:
            return ei(c, &p);
        case 0xc7: case 0xd7: case 0xe7: case 0xf7: case 0xcf: case 0xdf: case 0xef: case 0xff:
            return rst(c, &p);
        case 0xde:
            return sbc_a_imm8(c, &p);
        case 0xf6:
            return or_a_imm8(c, &p);
        case 0x80 ... 0x87:
            return add_a_r8(c, &p);
        case 0x88 ... 0x8f:
            return adc_a_r8(c, &p);
        case 0x90 ... 0x97:
            return sub_a_r8(c, &p);
        case 0x98 ... 0x9f:
            return sbc_a_r8(c, &p);
        case 0x17:
            return rla(c, &p);
        case 0x0f:
            return rrca(c, &p);
        case 0x07:
            return rlca(c, &p);
        case 0xf8:
            return ld_hl_sp_imm8(c, &p);
        case 0xe8:
            return add_sp_imm8(c, &p);
        case 0x76:
            return halt(c, &p);
        case 0x10:
            return stop(c, &p);
    }
    printf("%x opcode non implementato", opcode);
    exit(1);
}

void execute(cpu *c, tick *t) {
    if (c->ime == true)
        run_interrupt(c, t);
    uint8_t ticks = execute_instruction(c);
    add_ticks(c, t, ticks);
    if (c->ime_to_be_setted == 1) {
        c->ime_to_be_setted = 2;
    }
    else if (c->ime_to_be_setted == 2) {
        c->ime_to_be_setted = 0;
        c->ime = true;
    }
    if (c->dma_transfer) {
        c->dma_transfer = false;
        c->need_sprites_reload = true;
        dma_transfer(c, t);
    }
}
