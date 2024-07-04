#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cpu.h"
#include "apu.h"
#include "ppu.h"

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

void reset_apu_regs(cpu *c) {
    audio.is_on = false;
    audio.ch1.is_active = false;
    audio.ch2.is_active = false;
    audio.ch3.is_active = false;
    audio.ch4.is_active = false;
    c->memory[NR10] = 0x80;
    c->memory[NR11] = 0x00;
    c->memory[NR12] = 0x00;
    c->memory[NR13] = 0x00;
    c->memory[NR14] = 0xb8;

    c->memory[NR21] = 0x00;
    c->memory[NR22] = 0x00;
    c->memory[NR23] = 0x00;
    c->memory[NR24] = 0xb8;

    c->memory[NR30] = 0x7f;
    c->memory[NR31] = 0x00;
    c->memory[NR32] = 0x00;
    c->memory[NR33] = 0x00;
    c->memory[NR34] = 0xb8;

    c->memory[NR41] = 0xc0;
    c->memory[NR42] = 0x00;
    c->memory[NR43] = 0x00;
    c->memory[NR44] = 0xbf;

    c->memory[NR50] = 0x00;
    c->memory[NR51] = 0x00;
    c->memory[NR52] = 0x70;
}

void set_mem(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x1fff: // Rom
            switch (c->cart.type) {
                // MBC 1
                case 2 ... 3:
                    if ((value & 0xf) == 0xa)
                        c->cart.ram_enable = true;
                    else
                        c->cart.ram_enable = false;
                    break;
                // MBC 3
                case 0x12 ... 0x13: case 0x10:
                    if (value == 0x0a)
                        c->cart.ram_enable = true;
                    else if (value == 0)
                        c->cart.ram_enable = false;
                    break;
                // MBC 5
                case 0x1a ... 0x1b: case 0x1d ... 0x1e:
                    if (value == 0x0a)
                        c->cart.ram_enable = true;
                    else if (value == 0)
                        c->cart.ram_enable = false;
                    break;
                default:
                    break;
            }
            break;
        case 0x2000 ... 0x3fff: // Rom
            switch (c->cart.type) {
                // MBC 1
                case 1 ... 3:
                    if ((value & 31) == 0) {
                        value = 1;
                    }
                    c->cart.bank_select = (value & 31) % c->cart.banks;
                    break;
                // MBC 3
                case 0x0f ... 0x13:
                    if ((value & 127) == 0) {
                        value = 1;
                    }
                    c->cart.bank_select = value & 127;
                    break;
                // MBC 5
                case 0x1a ... 0x1e:
                    if (addr <= 0x2fff)
                        c->cart.bank_select = value;
                    break;
                default:
                    break;
            }
            break;
        case 0x4000 ... 0x5fff: // Rom
            switch (c->cart.type) {
                // MBC 1
                case 2 ... 3:
                    if (c->cart.banks > 1) {
                        if (value >= 0 && value <= 3) {
                            c->cart.bank_select_ram = value;
                        }
                    }
                    break;
                // MBC 3
                case 0x12 ... 0x13:
                    if (c->cart.banks > 1) {
                        if (value >= 0 && value <= 3) {
                            c->cart.bank_select_ram = value;
                        }
                    }
                    break;
                // MBC3 Timer
                case 0x0f ... 0x10:
                    if (c->cart.banks > 1) {
                        if (value >= 0 && value <= 3) {
                            c->cart.bank_select_ram = value;
                        }
                    }
                    if (value >= 8 && value <= 12) {
                        c->cart.bank_select_ram = value;
                    }
                    break;
                // MBC 5
                case 0x1a ... 0x1b: case 0x1d ... 0x1e:
                    if (c->cart.banks > 1) {
                        if (value >= 0 && value <= 16) {
                            c->cart.bank_select_ram = value;
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        case 0x6000 ... 0x7fff: // Rom
            break;

        case 0xa000 ... 0xbfff: // Ram
            if (c->cart.ram_enable) {
                if ((c->cart.type == 0x0f || c->cart.type == 0x10) && (c->cart.bank_select_ram >= 8)) {
                    // TODO
                }
                else {
                    c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
                }
            }
            break;

        case 0xe000 ... 0xfdff: // Echo ram
            c->memory[addr - 0x2000] = value;
            break;

        case STAT: // STAT Interrupt
            if (((value >> 6) & 1) != video.lyc_select)
                c->memory[IF] |= 2;
            if (((value >> 5) & 1) != video.mode2_select)
                c->memory[IF] |= 2;
            if (((value >> 4) & 1) != video.mode1_select)
                c->memory[IF] |= 2;
            if (((value >> 3) & 1) != video.mode0_select)
                c->memory[IF] |= 2;
            video.lyc_select = (value >> 6) & 1;
            video.mode2_select = (value >> 5) & 1;
            video.mode1_select = (value >> 4) & 1;
            video.mode0_select = (value >> 3) & 1;
            break;

        case DMA: // OAM DMA transfer
            c->memory[addr] = value;
            video.dma_transfer = true;
            break;

        case DIV: // divider register
            c->memory[addr] = 0;
            break;

        case 0x8000 ... 0x97ff: // Tiles
            c->memory[addr] = value;
            video.tiles_write = true;
            video.need_bg_wn_reload = true;
            break;
        case 0x9800 ... 0x9fff: // Tile map
            c->memory[addr] = value;
            video.tilemap_write = true;
            video.need_bg_wn_reload = true;
            break;

        case 0xfe00 ... 0xfe9f: // OAM
            c->memory[addr] = value;
            video.need_sprites_reload = true;
            break;

        case LCDC:
            video.bg_enable = value & 1;
            video.obj_enable = (value >> 1) & 1;
            video.obj_size = (value >> 2) & 1;
            video.bg_tilemap = (value >> 3) & 1;
            video.bg_tiles = (value >> 4) & 1;
            if (video.bg_enable)
                video.window_enable = (value >> 5) & 1;
            else
                video.window_enable = false;
            video.window_tilemap = (value >> 6) & 1;
            video.is_on = (value >> 7) & 1;

            video.need_bg_wn_reload = true;
            video.tiles_write = true;
            video.tilemap_write = true;
            video.need_sprites_reload = true;
            break;

        case NR10: case NR11: case NR12: case NR13:
            if (audio.is_on)
                c->memory[addr] = value;
            break;
        case NR14:
            if (audio.is_on) {
                if ((value & 0x80) != 0) {
                    audio.ch1.is_triggered = true;
                    audio.ch1.is_active = true;
                }
                c->memory[addr] = value;
            }
            break;

        case NR21: case NR22: case NR23:
            if (audio.is_on)
                c->memory[addr] = value;
            break;
        case NR24:
            if (audio.is_on) {
                if ((value & 0x80) != 0) {
                    audio.ch2.is_triggered = true;
                    audio.ch2.is_active = true;
                }
                c->memory[addr] = value;
            }
            break;

        case NR30: case NR31: case NR32: case NR33:
            if (audio.is_on)
                c->memory[addr] = value;
            break;
        case NR34:
            if (audio.is_on) {
                if ((value & 0x80) != 0) {
                    audio.ch3.is_triggered = true;
                    audio.ch3.is_active = true;
                }
                c->memory[addr] = value;
            }
            break;

        case NR41: case NR42: case NR43:
            if (audio.is_on)
                c->memory[addr] = value;
            break;
        case NR44:
            if (audio.is_on) {
                if ((value & 0x80) != 0) {
                    audio.ch4.is_triggered = true;
                    audio.ch4.is_active = true;
                }
                c->memory[addr] = value;
            }
            break;

        case NR50:
            if (audio.is_on) {
                c->memory[addr] = value;
            }
            break;

        case NR51:
            if (audio.is_on) {
                uint8_t ch[4];
                for (uint8_t i = 0; i < 4; i++) {
                    ch[i] = (value >> i) & 17;
                    switch (ch[i]) {
                        case 17:
                            audio.pan[i] = 0.5f;
                            break;
                        case 16:
                            audio.pan[i] = 1.0f;
                            break;
                        case 1:
                            audio.pan[i] = 0.0f;
                            break;
                        case 0:
                            audio.pan[i] = 0.6f;
                            break;
                        default:
                            break;
                    }
                }
                c->memory[addr] = value;
            }
            break;
        case NR52:
            if ((value & 0x80) == 0)
                reset_apu_regs(c);
            else
                audio.is_on = true;
            c->memory[addr] = value;
            break;


        case 0xff30 ... 0xff3f: // Wave Ram
            if (!audio.ch3.is_active)
                c->memory[addr] = value;
            break;

        default:
            c->memory[addr] = value;
            break;
    }
}

uint8_t get_mem(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr-0x4000];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable) {
                if ((c->cart.type == 0x0f || c->cart.type == 0x10) && (c->cart.bank_select_ram >= 8)) {
                    // TODO
                }
                else {
                    return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
                }
            }
            return 0;
        case 0xfea0 ... 0xfeff:
            return 0;
        case 0xe000 ... 0xfdff:
            return c->memory[addr - 0x2000];

        case JOYP:
            return c->memory[addr] | 0xc0;
        case SC:
            return c->memory[addr] | 0x7e;
        case TAC:
            return c->memory[addr] | 0xf8;
        case IF:
            return c->memory[addr] | 0xe0;
        case STAT:
            return video.mode | ((video.scan_line == c->memory[LYC]) << 2) | (video.mode0_select << 3) | (video.mode1_select << 4) | (video.mode2_select << 5) | (video.lyc_select << 6) | 0x80;

        case LY:
            return video.scan_line;
        case LCDC:
            return video.bg_enable | (video.obj_enable << 1) | (video.obj_size << 2) | (video.bg_tilemap << 3) | (video.bg_tiles << 4) | (video.window_enable << 5) | (video.window_tilemap << 6) | (video.is_on << 7);

        case NR10:
            return c->memory[addr] | 0x80;
        case NR11:
            return c->memory[addr] | 0x3f;
        case NR12:
            return c->memory[addr];
        case NR13:
            return 255;
        case NR14:
            return c->memory[addr] | 0xbf;

        case NR21:
            return c->memory[addr] | 0x3f;
        case NR22:
            return c->memory[addr];
        case NR23:
            return 255;
        case NR24:
            return c->memory[addr] | 0xbf;

        case NR30:
            return c->memory[addr] | 0x7f;
        case NR31:
            return 255;
        case NR32:
            return c->memory[addr] | 0x9f;
        case NR33:
            return 255;
        case NR34:
            return c->memory[addr] | 0xbf;

        case NR41:
            return 255;
        case NR42:
            return c->memory[addr];
        case NR43:
            return c->memory[addr];
        case NR44:
            return c->memory[addr] | 0xbf;

        case NR50:
            return c->memory[addr];
        case NR51:
            return c->memory[addr];
        case NR52:
            uint8_t ch_on = audio.ch1.is_active | (audio.ch2.is_active << 1) | (audio.ch3.is_active << 2) | (audio.ch4.is_active << 3) | 0x70;
            return (c->memory[addr] & 0xf0) | ch_on;

        case 0xff30 ... 0xff3f:
            if (audio.ch3.is_active)
                return 0xff;
            return c->memory[addr];

        case 0xff03: case 0xff08 ... 0xff0e: case 0xff15: case 0xff1f: case 0xff27 ... 0xff2f: case 0xff4c ... 0xff7f:
            return 0xff;


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
    c->ime_to_be_setted = 0;
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

uint8_t ldh_a_imm8(cpu *c, parameters *p) {
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

uint8_t prefix(cpu *c, parameters *p) {
    c->pc += 2;
    uint8_t prevC;
    uint8_t b3 = (p->imm8 & 0b00111000) >> 3;
    switch(p->imm8) {
        // RLC
        case 0x00 ... 0x07:
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) << 1));

                if ((c->r.reg8[F] & flagC) != 0)
                    set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 1));
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;

                if ((c->r.reg8[F] & flagC) != 0)
                    *r8(c, p->operand_r8) |= 1;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RRC
        case 0x08 ... 0x0f:
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 1) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) >> 1));

                if ((c->r.reg8[F] & flagC) != 0)
                    set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 128));
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;

                if ((c->r.reg8[F] & flagC) != 0)
                    *r8(c, p->operand_r8) |= 128;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RL
        case 0x10 ... 0x17:
            prevC = c->r.reg8[F] & flagC;
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) << 1));
                if (prevC != 0)
                    set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 1));
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;
                if (prevC != 0)
                    *r8(c, p->operand_r8) |= 1;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // RR
        case 0x18 ... 0x1f:
            prevC = (c->r.reg8[F] >> 4) & 1;;
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) >> 1));
                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | (prevC << 7)));
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
                *r8(c, p->operand_r8) |= (prevC << 7);
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SLA
        case 0x20 ... 0x27:
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 128) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) << 1));
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SRA
        case 0x28 ... 0x2f:
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) >> 1));
                uint8_t new_val = get_mem(c, c->r.reg16[HL]);
                new_val |= (new_val & 64) << 1;
                set_mem(c, c->r.reg16[HL], new_val);
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
                *r8(c, p->operand_r8) |= (*r8(c, p->operand_r8) & 64) << 1;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SWAP
        case 0x30 ... 0x37:
            if (p->operand_r8 == 6) {
                uint8_t low = get_mem(c, c->r.reg16[HL]) & 15;
                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) >> 4));
                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) + (low << 4)));
            }
            else {
                uint8_t low = *r8(c, p->operand_r8) & 15;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 4;
                *r8(c, p->operand_r8) += low << 4;
            }
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            c->r.reg8[F] &= ~flagC;
            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // SRL
        case 0x38 ... 0x3f:
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if (p->operand_r8 == 6) {
                if ((get_mem(c, c->r.reg16[HL]) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) >> 1));
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
            }

            if (p->operand_r8 == 6) {
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            if (p->operand_r8 == 6)
                return 16;
            return 8;
        // BIT
        case 0x40 ... 0x7f:
            uint8_t iszero;
            if (p->operand_r8 == 6)
                iszero = (get_mem(c, c->r.reg16[HL]) >> b3) & 1;
            else
                iszero = (*r8(c, p->operand_r8) >> b3) & 1;
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
    if (c->ime == 0) {
        c->ime_to_be_setted += 1;
    }
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
    c->is_halted = true;
    return 4;
}

uint8_t stop(cpu *c, parameters *p) {
    c->pc += 2;
    return 4;
}
