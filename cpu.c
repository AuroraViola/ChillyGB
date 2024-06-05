#include "cpu.h"
#include "opcodes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const uint8_t rst_vec[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
const uint16_t clock_select[] = {1024, 16, 64, 256};

void add_ticks(cpu *c, tick *t, uint16_t ticks){
    t->t_states += ticks;
    t->scan_line_tick += ticks;
    t->frame_tick += ticks;

    if (t->frame_tick >= 69905) {
        t->is_frame = true;
        t->frame_tick -= 69905;
    }

    if (t->scan_line_tick >= 456) {
        t->is_scanline++;
        t->scan_line_tick -= 456;
        c->memory[0xff44] += 1;
        if (((c->memory[0xff4a] < c->memory[0xff44]) && (c->memory[0xff40] & 32) != 0) && ((c->memory[0xff4b] - 7) < 160)) {
            c->window_internal_line += 1;
        }

        if (c->memory[0xff44] == c->memory[0xff45]) {
            c->memory[0xff41] |= 2;
            if ((c->memory[0xff41] & 64) != 0)
                c->memory[0xff0f] |= 2;
        }
        if (c->memory[0xff44] == 144) {
            c->memory[0xff0f] |= 1;
        } else if (c->memory[0xff44] >= 153) {
            t->scan_line_tick -= 456;
            c->memory[0xff44] = 0;
            c->window_internal_line = 0;
        }
    }

    if ((c->memory[0xff07] & 4) != 0) {
        t->tima_counter += ticks;
        if (t->tima_counter > clock_select[c->memory[0xff07] & 3]) {
            t->tima_counter = 0;
            c->memory[0xff05] += 1;
            if (c->memory[0xff05] == 0) {
                c->memory[0xff05] = c->memory[0xff06];
                c->memory[0xff0f] |= 4;
            }

        }
    }

    t->divider_register += ticks;
    if (t->divider_register >= 16384) {
        t->divider_register -= 16384;
        c->memory[0xff04] += 1;
    }
}

void run_interrupt(cpu *c, tick *t) {
    if ((c->memory[0xffff] & c->memory[0xff0f]) != 0) {
        if (((c->memory[0xffff] & 1) == 1) && ((c->memory[0xff0f] & 1) == 1)) {
            c->memory[0xff0f] &= 0b11111110;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t)(c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t)(c->pc);
            c->pc = 0x40;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[0xffff] & 2) == 2) && ((c->memory[0xff0f] & 2) == 2)) {
            c->memory[0xff0f] &= 0b11111101;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x48;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[0xffff] & 4) == 4) && ((c->memory[0xff0f] & 4) == 4)) {
            c->memory[0xff0f] &= 0b11111011;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x50;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[0xffff] & 8) == 8) && ((c->memory[0xff0f] & 8) == 8)) {
            c->memory[0xff0f] &= 0b11110111;
            c->ime = false;
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc >> 8);
            c->sp--;
            c->memory[c->sp] = (uint8_t) (c->pc);
            c->pc = 0x58;
            add_ticks(c, t, 20);
        }
        else if (((c->memory[0xffff] & 16) == 16) && ((c->memory[0xff0f] & 16) == 16)) {
            c->memory[0xff0f] &= 0b11101111;
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
    uint16_t to_transfer = (uint16_t)(c->memory[0xff46]) << 8;
    for (int i = 0; i < 160; i++) {
        c->memory[0xfe00 + i] = c->memory[to_transfer + i];
    }
    //add_ticks(c, t, 640);
}

uint8_t execute_instruction(cpu *c) {
    parameters p = {};
    uint8_t opcode = c->memory[c->pc];
    p.imm8 = c->memory[c->pc + 1];
    p.imm16 = ((uint16_t)c->memory[c->pc + 2] << 8) | c->memory[c->pc + 1];

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
    if (c->dma_transfer) {
        c->dma_transfer = false;
        dma_transfer(c, t);
    }
}
