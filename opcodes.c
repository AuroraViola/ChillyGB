#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cpu.h"

uint8_t* r8(cpu *c, uint8_t r) {
    switch (r) {
        case 0:
            return &c->r.reg8[B];
        case 1:
            return &c->r.reg8[C];
        case 2:
            return &c->r.reg8[D];
        case 3:
            return &c->r.reg8[E];
        case 4:
            return &c->r.reg8[H];
        case 5:
            return &c->r.reg8[L];
        case 6:
            return &c->memory[c->r.reg16[HL]];
        case 7:
            return &c->r.reg8[A];
    }
}

void set_mem(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x7fff: // Rom
            break;
        case 0xe000 ... 0xfdff: // Echo ram
            c->memory[addr - 0x2000] = value;
            break;
        case 0xff46: // OAM DMA transfer
            c->memory[addr] = value;
            c->dma_transfer = true;
            break;
        case 0xff04: // divider register
            c->memory[addr] = 0;
            break;

        case 0x8000 ... 0x97ff: // Tiles
            c->memory[addr] = value;
            c->tiles_write = true;
            c->need_bg_wn_reload = true;
            break;
        case 0x9800 ... 0x9fff: // Tile map
            c->memory[addr] = value;
            c->tilemap_write = true;
            c->need_bg_wn_reload = true;
            break;

        case 0xff40: // LCDC
            c->memory[addr] = value;
            c->tiles_write = true;
            c->tilemap_write = true;
            c->need_bg_wn_reload = true;
            break;

        default:
            c->memory[addr] = value;
            break;
    }
}

uint8_t get_mem(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0xfea0 ... 0xfeff:
            return 255;
        case 0xe000 ... 0xfdff:
            return c->memory[addr - 0x2000];
        default:
            return c->memory[addr];
    }
}

uint16_t* r16(cpu *c, uint8_t r) {
    if (r == 3) {
        return &c->sp;
    }
    else {
        return &c->r.reg16[r];
    }
}

uint16_t* r16mem(cpu *c, uint8_t r) {
    if (r == 3) {
        return &c->r.reg16[HL];
    }
    else {
        return &c->r.reg16[r];
    }
}

uint16_t* r16stk(cpu *c, uint8_t r) {
    return &c->r.reg16[r];
}

uint8_t jp(cpu *c, parameters *p) {
    c->pc = p->imm16;
    return 16;
}

uint8_t ld_r8_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    if (p->operand_stk_r8 == 6) {
        set_mem(c, c->r.reg16[HL], p->imm8);
        return 12;
    }
    else {
        *r8(c, p->operand_stk_r8) = p->imm8;
        return 8;
    }
}

uint8_t ld_imm16_a(cpu *c, parameters *p) {
    c->pc += 3;
    set_mem(c, p->imm16, c->r.reg8[A]);
    return 16;
}

uint8_t ld_a_imm16(cpu *c, parameters *p) {
    c->pc += 3;
    c->r.reg8[A] = get_mem(c, p->imm16);
    return 16;
}

uint8_t cp_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t temp = c->r.reg8[A] - p->imm8;
    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] |= flagN;

    if (temp > c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) > (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;
    return 8;
}

uint8_t jp_cond(cpu *c, parameters *p) {
    c->pc += 3;
    bool flag = false;
    if ((c->r.reg8[F] & flagC) != 0 && (p->condition == condC))
        flag = true;
    else if (!(c->r.reg8[F] & flagC) && (p->condition == condNC))
        flag = true;
    else if ((c->r.reg8[F] & flagZ) && (p->condition == condZ))
        flag = true;
    else if (!(c->r.reg8[F] & flagZ) && (p->condition == condNZ))
        flag = true;
    if (flag) {
        c->pc = p->imm16;
        return 16;
    }
    return 12;
}

uint8_t jp_hl(cpu *c, parameters *p) {
    c->pc = c->r.reg16[HL];
    return 4;
}

uint8_t ld_r16_imm16(cpu *c, parameters *p) {
    c->pc += 3;
    *r16(c, p->operand_r16) = p->imm16;
    return 12;
}

uint8_t ld_a_r16mem(cpu *c, parameters *p) {
    c->pc += 1;
    c->r.reg8[A] = get_mem(c, *r16mem(c, p->dest_source_r16mem));
    if (p->dest_source_r16mem == 2) {
        c->r.reg16[HL]++;
    }
    else if (p->dest_source_r16mem == 3) {
        c->r.reg16[HL]--;
    }
    return 8;
}

uint8_t ld_r16mem_a(cpu *c, parameters *p) {
    c->pc += 1;
    set_mem(c, *r16mem(c, p->dest_source_r16mem), c->r.reg8[A]);
    if (p->dest_source_r16mem == 2) {
        c->r.reg16[HL]++;
    }
    else if (p->dest_source_r16mem == 3) {
        c->r.reg16[HL]--;
    }
    return 8;
}

uint8_t inc_r16(cpu *c, parameters *p) {
    c->pc += 1;
    *r16(c, p->operand_r16) += 1;
    return 8;
}

uint8_t dec_r16(cpu *c, parameters *p) {
    c->pc += 1;
    *r16(c, p->operand_r16) -= 1;
    return 8;
}

uint8_t ld_r8_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_stk_r8 == 6) {
        set_mem(c, c->r.reg16[HL], *r8(c, p->operand_r8));
        return 8;
    }
    else if (p->operand_r8 == 6) {
        *r8(c, p->operand_stk_r8) = get_mem(c, c->r.reg16[HL]);
        return 8;
    }
    else {
        *r8(c, p->operand_stk_r8) = *r8(c, p->operand_r8);
        return 4;
    }
}

uint8_t or_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_r8 == 6) {
        c->r.reg8[A] |= get_mem(c, c->r.reg16[HL]);
    }
    else {
        c->r.reg8[A] |= *r8(c, p->operand_r8);
    }
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagC;
    c->r.reg8[F] &= ~flagH;
    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t nop(cpu *c, parameters *p) {
    c->pc += 1;
    return 4;
}

uint8_t ld_imm16_sp(cpu *c, parameters *p) {
    c->pc += 3;
    set_mem(c, p->imm16, (uint8_t)(c->sp & 255));
    set_mem(c, (p->imm16+1), (uint8_t)(c->sp >> 8));
    return 20;
}

uint8_t inc_r8(cpu *c, parameters *p) { // TODO
    c->pc += 1;
    if ((((*r8(c, p->operand_stk_r8) & 0xf) + 1) & 0xf) < (*r8(c, p->operand_stk_r8) & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    if (p->operand_stk_r8 == 6)
        set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) + 1));
    else
        *r8(c, p->operand_stk_r8) += 1;
    c->r.reg8[F] &= ~flagN;

    if (*r8(c, p->operand_stk_r8) == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    if (p->operand_stk_r8 == 6)
        return 12;
    return 4;
}

uint8_t dec_r8(cpu *c, parameters *p) { //TODO
    c->pc += 1;
    if ((((*r8(c, p->operand_stk_r8) & 0xf) - 1) & 0xf) > (*r8(c, p->operand_stk_r8) & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;
    if (p->operand_stk_r8 == 6)
        set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) - 1));
    else
        *r8(c, p->operand_stk_r8) -= 1;

    c->r.reg8[F] |= flagN;

    if (*r8(c, p->operand_stk_r8) == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    if (p->operand_stk_r8 == 6)
        return 12;
    return 4;
}

uint8_t jr_cond(cpu *c, parameters *p) {
    c->pc += 2;
    bool flag = false;
    if ((c->r.reg8[F] & flagC) && (p->condition == condC))
        flag = true;
    else if (!(c->r.reg8[F] & flagC) && (p->condition == condNC))
        flag = true;
    else if ((c->r.reg8[F] & flagZ) && (p->condition == condZ))
        flag = true;
    else if (!(c->r.reg8[F] & flagZ) && (p->condition == condNZ))
        flag = true;
    if (flag) {
        c->pc += (int8_t)p->imm8;
        return 16;
    }
    return 12;
}

uint8_t di(cpu *c, parameters *p) {
    c->pc += 1;
    c->ime = false;
    return 4;
}

uint8_t ldh_imm8_a(cpu *c, parameters *p) {
    c->pc += 2;
    set_mem(c, (0xff00 | p->imm8), c->r.reg8[A]);
    return 12;
}

uint8_t ldh_c_a(cpu *c, parameters *p) {
    c->pc += 1;
    set_mem(c, (0xff00 | c->r.reg8[C]), c->r.reg8[A]);
    return 8;
}

uint8_t ldh_a_c(cpu *c, parameters *p) {
    c->pc += 1;
    c->r.reg8[A] = get_mem(c, (0xff00 | c->r.reg8[C]));
    return 8;
}

uint8_t call(cpu *c, parameters *p) {
    c->pc += 3;
    c->sp--;
    set_mem(c, c->sp, (uint8_t)(c->pc >> 8));
    c->sp--;
    set_mem(c, c->sp, (uint8_t)(c->pc));
    c->pc = p->imm16;
    return 24;
}

uint8_t jr(cpu *c, parameters *p) {
    c->pc += 2;
    c->pc += (int8_t)p->imm8;
    return 12;
}

uint8_t ret(cpu *c, parameters *p) {
    c->pc = get_mem(c, c->sp);
    c->sp++;
    c->pc |= get_mem(c, c->sp) << 8;
    c->sp++;
    return 16;
}

uint8_t reti(cpu *c, parameters *p) {
    c->pc = get_mem(c, c->sp);
    c->sp++;
    c->pc |= get_mem(c, c->sp) << 8;
    c->sp++;
    c->ime = true;
    return 16;
}

uint8_t push(cpu *c, parameters *p) {
    c->pc += 1;
    c->sp -= 1;
    set_mem(c, c->sp, (*r16stk(c, p->operand_stk_r16) >> 8));
    c->sp -= 1;
    set_mem(c, c->sp, (*r16stk(c, p->operand_stk_r16) & 255));
    return 16;
}

uint8_t pop(cpu *c, parameters *p) {
    c->pc += 1;
    *r16stk(c, p->operand_stk_r16) = get_mem(c, c->sp);
    c->sp += 1;
    *r16stk(c, p->operand_stk_r16) |= get_mem(c, (c->sp)) << 8;
    c->sp += 1;
    c->r.reg8[F] &= 0xf0;
    return 12;
}

uint8_t ldh_a_imm8(cpu *c, parameters *p) { //TODO
    c->pc += 2;
    c->r.reg8[A] = get_mem(c, (0xff00 | p->imm8));
    return 12;
}

uint8_t and_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_r8 == 6)
        c->r.reg8[A] &= get_mem(c, c->r.reg16[HL]);
    else
        c->r.reg8[A] &= *r8(c, p->operand_r8);

    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] |= flagH;
    c->r.reg8[F] &= ~flagC;
    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t xor_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_r8 == 6)
        c->r.reg8[A] ^= get_mem(c, c->r.reg16[HL]);
    else
        c->r.reg8[A] ^= *r8(c, p->operand_r8);

    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagC;
    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t and_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    c->r.reg8[A] &= p->imm8;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] |= flagH;
    c->r.reg8[F] &= ~flagC;
    return 8;
}

uint8_t xor_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    c->r.reg8[A] ^= p->imm8;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagC;
    return 8;
}

uint8_t or_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    c->r.reg8[A] |= p->imm8;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagC;
    return 8;
}

uint8_t call_cond(cpu *c, parameters *p) {
    c->pc += 3;
    bool flag = false;
    if ((c->r.reg8[F] & flagC) != 0 && (p->condition == condC))
        flag = true;
    else if (!(c->r.reg8[F] & flagC) && (p->condition == condNC))
        flag = true;
    else if ((c->r.reg8[F] & flagZ) && (p->condition == condZ))
        flag = true;
    else if (!(c->r.reg8[F] & flagZ) && (p->condition == condNZ))
        flag = true;
    if (flag) {
        c->sp--;
        set_mem(c, c->sp, (uint8_t)(c->pc >> 8));
        c->sp--;
        set_mem(c, c->sp, (uint8_t)(c->pc));
        c->pc = p->imm16;
        return 24;
    }
    return 12;
}

uint8_t prefix(cpu *c, parameters *p) { // TODO
    c->pc += 2;
    uint8_t prevC;
    uint8_t b3 = (p->imm8 & 0b00111000) >> 3;
    switch(p->imm8) {
        // RLC
        case 0x00 ... 0x07:
            if ((*r8(c, p->operand_r8) & 128) != 0)
                c->r.reg8[F] |= flagC;
            else
                c->r.reg8[F] &= ~flagC;

            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;

            if ((c->r.reg8[F] & flagC) != 0)
                *r8(c, p->operand_r8) |= 1;

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RRC
        case 0x08 ... 0x0f:
            if ((*r8(c, p->operand_r8) & 1) != 0)
                c->r.reg8[F] |= flagC;
            else
                c->r.reg8[F] &= ~flagC;

            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;

            if ((c->r.reg8[F] & flagC) != 0)
                *r8(c, p->operand_r8) |= 128;

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RL
        case 0x10 ... 0x17:
            prevC = c->r.reg8[F] & flagC;

            if ((*r8(c, p->operand_r8) & 128) != 0)
                c->r.reg8[F] |= flagC;
            else
                c->r.reg8[F] &= ~flagC;

            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;
            if (prevC != 0)
                *r8(c, p->operand_r8) |= 1;

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RR
        case 0x18 ... 0x1f:
            prevC = (c->r.reg8[F] >> 4) & 1;;
            if ((*r8(c, p->operand_r8) & 1) == 0)
                c->r.reg8[F] &= ~flagC;
            else
                c->r.reg8[F] |= flagC;
            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
            *r8(c, p->operand_r8) |= (prevC << 7);

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SLA
        case 0x20 ... 0x27:
            if ((*r8(c, p->operand_r8) & 128) == 0)
                c->r.reg8[F] &= ~flagC;
            else
                c->r.reg8[F] |= flagC;

            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SRA
        case 0x28 ... 0x2f:
            if ((*r8(c, p->operand_r8) & 1) == 0)
                c->r.reg8[F] &= ~flagC;
            else
                c->r.reg8[F] |= flagC;

            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
            *r8(c, p->operand_r8) |= (*r8(c, p->operand_r8) & 64) << 1;

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SWAP
        case 0x30 ... 0x37:
            uint8_t low = *r8(c, p->operand_r8) & 15;
            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 4;
            *r8(c, p->operand_r8) += low << 4;
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            c->r.reg8[F] &= ~flagC;
            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SRL
        case 0x38 ... 0x3f:
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if ((*r8(c, p->operand_r8) & 1) == 0)
                c->r.reg8[F] &= ~flagC;
            else
                c->r.reg8[F] |= flagC;
            *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;

            if (*r8(c, p->operand_r8) == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // BIT
        case 0x40 ... 0x7f:
            uint8_t iszero = (*r8(c, p->operand_r8) >> b3) & 1;
            if (iszero == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] |= flagH;
            if (p->operand_r8 == 6)
                return 12;
            return 8;
        // RES
        case 0x80 ... 0xbf:
            if (p->operand_r8 == 6)
                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) & ~(1 << b3)));
            else
                *r8(c, p->operand_r8) &= ~(1 << b3);
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SET
        case 0xc0 ... 0xff:
            if (p->operand_r8 == 6)
                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 1 << b3));
            else
                *r8(c, p->operand_r8) |= (1 << b3);
            if (p->operand_r8 == 6)
                return 16;
            return 8;
    }
}

uint8_t rra(cpu *c, parameters *p) {
    c->pc += 1;

    uint8_t prevC = (c->r.reg8[F] >> 4) & 1;
    if ((c->r.reg8[A] & 1) == 0)
        c->r.reg8[F] &= ~flagC;
    else
        c->r.reg8[F] |= flagC;
    c->r.reg8[A] = c->r.reg8[A] >> 1;
    c->r.reg8[A] |= (prevC << 7);

    c->r.reg8[F] &= ~flagZ;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    return 4;
}

uint8_t add_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t temp = c->r.reg8[A] + p->imm8;
    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] &= ~flagN;

    if (temp < c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) < (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = temp;

    return 8;
}

uint8_t adc_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t carry = (c->r.reg8[F] >> 4) & 1;
    uint16_t temp = c->r.reg8[A] + p->imm8 + carry;

    c->r.reg8[F] &= ~flagN;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if (((c->r.reg8[A] & 15) + (p->imm8 & 15) + carry) > 15)
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = c->r.reg8[A] + p->imm8 + carry;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    return 8;
}

uint8_t sub_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t temp = c->r.reg8[A] - p->imm8;
    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] |= flagN;

    if (temp > c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) > (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = temp;
    return 8;
}

uint8_t sbc_a_imm8(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t carry = (c->r.reg8[F] >> 4) & 1;
    uint16_t temp = c->r.reg8[A] - p->imm8 - carry;
    c->r.reg8[F] |= flagN;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((uint16_t)((c->r.reg8[A] & 15) - (p->imm8 & 15) - carry) > 255)
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = c->r.reg8[A] - p->imm8 - carry;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    return 8;
}

uint8_t ret_cond(cpu *c, parameters *p) {
    c->pc += 1;
    bool flag = false;
    if ((c->r.reg8[F] & flagC) != 0 && (p->condition == condC))
        flag = true;
    else if (!(c->r.reg8[F] & flagC) && (p->condition == condNC))
        flag = true;
    else if ((c->r.reg8[F] & flagZ) && (p->condition == condZ))
        flag = true;
    else if (!(c->r.reg8[F] & flagZ) && (p->condition == condNZ))
        flag = true;
    if (flag) {
        c->pc = get_mem(c, c->sp);
        c->sp++;
        c->pc |= get_mem(c, c->sp) << 8;
        c->sp++;
        return 20;
    }
    return 8;
}

uint8_t add_hl_r16(cpu *c, parameters *p) {
    c->pc += 1;
    uint16_t temp = c->r.reg16[HL] + *r16(c, p->operand_r16);

    c->r.reg8[F] &= ~flagN;

    if (temp < c->r.reg16[HL])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xfff) < (c->r.reg16[HL] & 0xfff))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg16[HL] = temp;

    return 8;
}

uint8_t cp_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t temp;
    if (p->operand_r8 == 6)
        temp = c->r.reg8[A] - get_mem(c, c->r.reg16[HL]);
    else
        temp = c->r.reg8[A] - *r8(c, p->operand_r8);
    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] |= flagN;

    if (temp > c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) > (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;
    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t daa(cpu *c, parameters *p) {
    c->pc += 1;

    uint8_t temp = c->r.reg8[A];
    uint8_t corr = 0;

    if ((c->r.reg8[F] & flagH) != 0)
        corr |= 0x06;
    if ((c->r.reg8[F] & flagC) != 0)
        corr |= 0x60;

    if ((c->r.reg8[F] & flagN) == 0) {
        if ((temp & 0x0f) > 0x09)
            corr |= 0x06;
        if (temp > 0x99)
            corr |= 0x60;
        temp += corr;
    }
    else {
        temp -= corr;
    }

    if ((corr & 0x60) != 0)
        c->r.reg8[F] |= flagC;

    c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = temp;

    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;
    return 4;
}

uint8_t cpl(cpu *c, parameters *p) {
    c->pc += 1;
    c->r.reg8[F] |= flagH;
    c->r.reg8[F] |= flagN;

    c->r.reg8[A] = ~(c->r.reg8[A]);
    return 4;
}

uint8_t ccf(cpu *c, parameters *p) {
    c->pc += 1;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagN;

    if ((c->r.reg8[F] & flagC) == 0)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    return 4;
}

uint8_t scf(cpu *c, parameters *p) {
    c->pc += 1;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] |= flagC;
    return 4;
}

uint8_t ld_sp_hl(cpu *c, parameters *p) {
    c->pc += 1;
    c->sp = c->r.reg16[HL];
    return 8;
}

uint8_t ei(cpu *c, parameters *p) {
    c->pc += 1;
    c->ime = true;
    return 4;
}

uint8_t rst(cpu *c, parameters *p) {
    c->pc += 1;
    c->sp--;
    set_mem(c, c->sp, (uint8_t)(c->pc >> 8));
    c->sp--;
    set_mem(c, c->sp, (uint8_t)(c->pc));
    c->pc = p->tgt3;
    return 16;
}

uint8_t add_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t temp;
    if (p->operand_r8 == 6)
        temp = c->r.reg8[A] + get_mem(c, c->r.reg16[HL]);
    else
        temp = c->r.reg8[A] + *r8(c, p->operand_r8);
    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] &= ~flagN;

    if (temp < c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) < (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = temp;

    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t adc_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t carry = (c->r.reg8[F] >> 4) & 1;
    uint16_t temp;
    if (p->operand_r8 == 6)
        temp = c->r.reg8[A] + get_mem(c, c->r.reg16[HL]) + carry;
    else
        temp = c->r.reg8[A] + *r8(c, p->operand_r8) + carry;

    c->r.reg8[F] &= ~flagN;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if (p->operand_r8 == 6) {
        if (((c->r.reg8[A] & 15) + (get_mem(c, c->r.reg16[HL]) & 15) + carry) > 15)
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;
    }
    else {
        if (((c->r.reg8[A] & 15) + (*r8(c, p->operand_r8) & 15) + carry) > 15)
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;
    }

    c->r.reg8[A] = temp;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t sub_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t temp;
    if (p->operand_r8 == 6)
        temp = c->r.reg8[A] - get_mem(c, c->r.reg16[HL]);
    else
        temp = c->r.reg8[A] - *r8(c, p->operand_r8);

    if (temp == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    c->r.reg8[F] |= flagN;

    if (temp > c->r.reg8[A])
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if ((temp & 0xf) > (c->r.reg8[A] & 0xf))
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg8[A] = temp;
    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t sbc_a_r8(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t carry = (c->r.reg8[F] >> 4) & 1;
    uint16_t temp = c->r.reg8[A] - *r8(c, p->operand_r8) - carry;
    c->r.reg8[F] |= flagN;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if (p->operand_r8 == 6) {
        if ((uint16_t) ((c->r.reg8[A] & 15) - (get_mem(c, c->r.reg16[HL]) & 15) - carry) > 255)
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;
    }
    else {
        if ((uint16_t) ((c->r.reg8[A] & 15) - (*r8(c, p->operand_r8) & 15) - carry) > 255)
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;
    }

    c->r.reg8[A] = temp;
    if (c->r.reg8[A] == 0)
        c->r.reg8[F] |= flagZ;
    else
        c->r.reg8[F] &= ~flagZ;

    if (p->operand_r8 == 6)
        return 8;
    return 4;
}

uint8_t rla(cpu *c, parameters *p) {
    c->pc += 1;
    uint8_t prevC = c->r.reg8[F] & flagC;

    if ((c->r.reg8[A] & 128) != 0)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    c->r.reg8[A] = c->r.reg8[A] << 1;
    if (prevC != 0)
        c->r.reg8[A] |= 1;

    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagZ;

    return 4;
}

uint8_t rlca(cpu *c, parameters *p) {
    c->pc += 1;
    if ((c->r.reg8[A] & 128) != 0)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    c->r.reg8[A] = c->r.reg8[A] << 1;

    if ((c->r.reg8[F] & flagC) != 0)
        c->r.reg8[A] |= 1;

    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagZ;

    return 4;
}

uint8_t rrca(cpu *c, parameters *p) {
    c->pc += 1;
    if ((c->r.reg8[A] & 1) != 0)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    c->r.reg8[A] = c->r.reg8[A] >> 1;

    if ((c->r.reg8[F] & flagC) != 0)
        c->r.reg8[A] |= 128;

    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagH;
    c->r.reg8[F] &= ~flagZ;

    return 4;
}

uint8_t add_sp_imm8(cpu *c, parameters *p) {
    c->pc += 2;

    uint16_t temp = (c->sp & 0xff) + p->imm8;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if (((c->sp & 15) + (p->imm8 & 15)) > 15)
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->sp = c->sp + (int8_t)p->imm8;

    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagZ;

    return 16;
}

uint8_t ld_hl_sp_imm8(cpu *c, parameters *p) {
    c->pc += 2;

    uint16_t temp = (c->sp & 0xff) + p->imm8;

    if (temp > 255)
        c->r.reg8[F] |= flagC;
    else
        c->r.reg8[F] &= ~flagC;

    if (((c->sp & 15) + (p->imm8 & 15)) > 15)
        c->r.reg8[F] |= flagH;
    else
        c->r.reg8[F] &= ~flagH;

    c->r.reg16[HL] = c->sp + (int8_t)p->imm8;

    c->r.reg8[F] &= ~flagN;
    c->r.reg8[F] &= ~flagZ;

    return 12;
}

uint8_t halt(cpu *c, parameters *p) {
    c->pc += 1;
    return 4;
}

uint8_t stop(cpu *c, parameters *p) {
    c->pc += 2;
    return 4;
}
