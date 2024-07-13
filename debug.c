#include <stdio.h>
#include "debug.h"
#include "cpu.h"
#include "opcodes.h"
#include "ppu.h"

const uint8_t rst_vec_1[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
const char r8_1[][5] = {"B", "c", "d", "e", "h", "l", "[hl]", "a"};
const char r16_1[][3] = {"bc", "de", "hl", "sp"};
const char r16stk_1[][3] = {"bc", "de", "hl", "af"};
const char r16mem_1[][4] = {"bc", "de", "hl+", "hl-"};
const char cond_1[][4] = {"nz", "z", "nc", "c"};

void generate_texts(cpu *c, debugtexts *texts) {
    sprintf(texts->AFtext, "AF: %04X", c->r.reg16[AF]);
    sprintf(texts->BCtext, "BC: %04X", c->r.reg16[BC]);
    sprintf(texts->DEtext, "DE: %04X", c->r.reg16[DE]);
    sprintf(texts->HLtext, "HL: %04X", c->r.reg16[HL]);
    sprintf(texts->SPtext, "SP: %04X", c->sp);
    sprintf(texts->PCtext, "PC: %04X", c->pc);
    if (c->ime)
        sprintf(texts->IMEtext, "IME: ON");
    else
        sprintf(texts->IMEtext, "IME: OFF");

    sprintf(texts->LYtext, "LY: %i", get_mem(c, LY));
    sprintf(texts->LYCtext, "LYC: %i", get_mem(c, LYC));
    sprintf(texts->PPUMode, "PPU Mode: %i", video.mode);
    sprintf(texts->LCDCtext, "LCDC: %08b", get_mem(c, LCDC));
    sprintf(texts->STATtext, "STAT: %08b", get_mem(c, STAT));

    sprintf(texts->IEtext, "IE: %08b", get_mem(c, IE));
    sprintf(texts->IFtext, "IF: %08b", get_mem(c, IF));

    sprintf(texts->BANKtext, "Bank: %i", c->cart.bank_select);
    sprintf(texts->RAMBANKtext, "Ram Bank: %i", c->cart.bank_select_ram);
    sprintf(texts->RAMENtext, "Ram En: %i", c->cart.ram_enable);
}

int decode_instruction(cpu *c, uint16_t v_pc, char instruction[50]) {
    parameters p = {};

    uint8_t opcode = get_mem(c, v_pc);
    p.imm8 = get_mem(c, (v_pc+1));
    p.imm16 = ((uint16_t)get_mem(c, (v_pc+2)) << 8) | get_mem(c, (v_pc+1));

    p.condition = (opcode & 0b00011000) >> 3;

    if (opcode != 0xcb)
        p.operand_r8 = opcode & 0b00000111;
    else
        p.operand_r8 = p.imm8 & 0b00000111;

    p.operand_r16 = (opcode & 0b00110000) >> 4;
    p.operand_stk_r8 = (opcode & 0b00111000) >> 3;

    p.tgt3 = rst_vec_1[(opcode & 0b00111000) >> 3];
    switch(opcode) {
        case 0xc3:
            sprintf(instruction, "%04X: JP $%04X", v_pc, p.imm16);
            return 3;
        case 0x06: case 0x16: case 0x26: case 0x36: case 0x0e: case 0x1e: case 0x2e: case 0x3e:
            sprintf(instruction, "%04X: LD %s $%02X", v_pc, r8_1[p.operand_r8], p.imm8);
            return 2;
        case 0x04: case 0x14: case 0x24: case 0x34: case 0x0c: case 0x1c: case 0x2c: case 0x3c:
            sprintf(instruction, "%04X: INC %s", v_pc, r8_1[p.operand_stk_r8]);
            return 1;
        case 0x05: case 0x15: case 0x25: case 0x35: case 0x0d: case 0x1d: case 0x2d: case 0x3d:
            sprintf(instruction, "%04X: DEC %s", v_pc, r8_1[p.operand_stk_r8]);
            return 1;
        case 0xea:
            sprintf(instruction, "%04X: LDH [$%04X], a", v_pc, p.imm16);
            return 3;
        case 0xfa:
            sprintf(instruction, "%04X: LDH a, [$%04X]", v_pc, p.imm16);
            return 3;
        case 0xfe:
            sprintf(instruction, "%04X: CP a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xda: case 0xc2: case 0xd2: case 0xca:
            sprintf(instruction, "%04X: JP %s, $%04X", v_pc, cond_1[p.condition], p.imm16);
            return 3;
        case 0x01: case 0x11: case 0x21: case 0x31:
            sprintf(instruction, "%04X: LD %s, $%04X", v_pc, r16_1[p.operand_r16], p.imm16);
            return 3;
        case 0xe9:
            sprintf(instruction, "%04X: JP hl", v_pc);
            return 1;
        case 0x0a: case 0x1a: case 0x2a: case 0x3a:
            sprintf(instruction, "%04X: LD a, [%s]", v_pc, r16mem_1[p.operand_r16]);
            return 1;
        case 0x02: case 0x12: case 0x22: case 0x32:
            sprintf(instruction, "%04X: LD [%s], a", v_pc, r16mem_1[p.operand_r16]);
            return 1;
        case 0x03: case 0x13: case 0x23: case 0x33:
            sprintf(instruction, "%04X: INC %s", v_pc, r16_1[p.operand_r16]);
            return 1;
        case 0x0b: case 0x1b: case 0x2b: case 0x3b:
            sprintf(instruction, "%04X: DEC %s", v_pc, r16_1[p.operand_r16]);
            return 1;
        case 0x40 ... 0x75: case 0x77 ... 0x7f:
            sprintf(instruction, "%04X: LD %s, %s", v_pc, r8_1[p.operand_stk_r8], r8_1[p.operand_r8]);
            return 1;
        case 0xb0 ... 0xb7:
            sprintf(instruction, "%04X: OR a, %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0x00:
            sprintf(instruction, "%04X: NOP", v_pc);
            return 1;
        case 0x08:
            sprintf(instruction, "%04X: LD [$%04X], SP", v_pc, p.imm16);
            return 3;
        case 0x20: case 0x30: case 0x28: case 0x38:
            sprintf(instruction, "%04X: JR %s $%04X", v_pc, cond_1[p.condition], ((v_pc + (int8_t)(p.imm8)) + 2));
            return 2;
        case 0x18:
            sprintf(instruction, "%04X: JR $%04X", v_pc, ((v_pc + (int8_t)(p.imm8)) + 2));
            return 2;
        case 0xf3:
            sprintf(instruction, "%04X: DI", v_pc);
            return 1;
        case 0xe0:
            sprintf(instruction, "%04X: LDH [0xFF00 + $%02X], a", v_pc, p.imm8);
            return 2;
        case 0xf0:
            sprintf(instruction, "%04X: LDH a, [0xFF00 + $%02X]", v_pc, p.imm8);
            return 2;
        case 0xcd:
            sprintf(instruction, "%04X: CALL $%04X", v_pc, p.imm16);
            return 3;
        case 0xc9:
            sprintf(instruction, "%04X: RET", v_pc);
            return 1;
        case 0xc1: case 0xd1: case 0xe1: case 0xf1:
            sprintf(instruction, "%04X: POP %s", v_pc, r16stk_1[p.operand_r16]);
            return 1;
        case 0xc5: case 0xd5: case 0xe5: case 0xf5:
            sprintf(instruction, "%04X: PUSH %s", v_pc, r16stk_1[p.operand_r16]);
            return 1;
        case 0xa0 ... 0xa7:
            sprintf(instruction, "%04X: AND %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0xa8 ... 0xaf:
            sprintf(instruction, "%04X: XOR %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0xb8 ... 0xbf:
            sprintf(instruction, "%04X: CP %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0xe6:
            sprintf(instruction, "%04X: AND a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xee:
            sprintf(instruction, "%04X: XOR a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xc4: case 0xd4: case 0xcc: case 0xdc:
            sprintf(instruction, "%04X: CALL %s, $%04X", v_pc, cond_1[p.condition], p.imm16);
            return 3;
        case 0xc6:
            sprintf(instruction, "%04X: ADD a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xd6:
            sprintf(instruction, "%04X: SUB a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xcb:
            sprintf(instruction, "%04X: PREFIX", v_pc);
            return 3;
        case 0x1f:
            sprintf(instruction, "%04X: RRA", v_pc);
            return 1;
        case 0xce:
            sprintf(instruction, "%04X: ADC a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xc0: case 0xd0: case 0xc8: case 0xd8:
            sprintf(instruction, "%04X: RET %s", v_pc, cond_1[p.condition]);
            return 1;
        case 0x09: case 0x19: case 0x29: case 0x39:
            sprintf(instruction, "%04X: ADD hl, %s", v_pc, r16_1[p.operand_r16]);
            return 1;
        case 0x27:
            sprintf(instruction, "%04X: DAA", v_pc);
            return 1;
        case 0xe2:
            sprintf(instruction, "%04X: LD [0xFF00 + c], a", v_pc);
            return 1;
        case 0xf2:
            sprintf(instruction, "%04X: LD a, [0xFF00 + c]", v_pc);
            return 1;
        case 0xd9:
            sprintf(instruction, "%04X: RETI", v_pc);
            return 1;
        case 0x2f:
            sprintf(instruction, "%04X: CPL", v_pc);
            return 1;
        case 0x3f:
            sprintf(instruction, "%04X: CCF", v_pc);
            return 1;
        case 0x37:
            sprintf(instruction, "%04X: SCF", v_pc);
            return 1;
        case 0xf9:
            sprintf(instruction, "%04X: LD SP, HL", v_pc);
            return 1;
        case 0xfb:
            sprintf(instruction, "%04X: EI", v_pc);
            return 1;
        case 0xc7: case 0xd7: case 0xe7: case 0xf7: case 0xcf: case 0xdf: case 0xef: case 0xff:
            sprintf(instruction, "%04X: RST %02x", v_pc, p.tgt3);
            return 1;
        case 0xde:
            sprintf(instruction, "%04X: SBC a, $%02X", v_pc, p.imm8);
            return 2;
        case 0xf6:
            sprintf(instruction, "%04X: OR a, $%02X", v_pc, p.imm8);
            return 2;
        case 0x80 ... 0x87:
            sprintf(instruction, "%04X: ADD %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0x88 ... 0x8f:
            sprintf(instruction, "%04X: ADC %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0x90 ... 0x97:
            sprintf(instruction, "%04X: SUB %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0x98 ... 0x9f:
            sprintf(instruction, "%04X: SBC %s", v_pc, r8_1[p.operand_r8]);
            return 1;
        case 0x17:
            sprintf(instruction, "%04X: RLA", v_pc);
            return 1;
        case 0x0f:
            sprintf(instruction, "%04X: RRCA", v_pc);
            return 1;
        case 0x07:
            sprintf(instruction, "%04X: RLCA", v_pc);
            return 1;
        case 0xf8:
            sprintf(instruction, "%04X: LD HL, SP + ", v_pc);
            return 2;
        case 0xe8:
            sprintf(instruction, "%04X: ADD SP,", v_pc);
            return 2;
        case 0x76:
            sprintf(instruction, "%04X: HALT", v_pc);
            return 1;
        case 0x10:
            sprintf(instruction, "%04X: STOP", v_pc);
            return 1;
        default:
            sprintf(instruction, "%04X: Not an instruction", v_pc);
            return 1;
    }
}


void decode_instructions(cpu *c, char instruction[30][50]) {
    uint16_t virtual_pc = c->pc;
    for (int i = 0; i < 30; i++) {
        virtual_pc += decode_instruction(c, virtual_pc, instruction[i]);
    }
}
