#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "../includes/cpu.h"
#include "../includes/apu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include "../includes/input.h"
#include "../includes/opcodes.h"

const uint16_t clock_tac_shift2[] = {0x200, 0x8, 0x20, 0x80};

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
    c->memory[NR11] &= 0x3f;
    c->memory[NR12] = 0x00;
    c->memory[NR13] = 0x00;
    c->memory[NR14] = 0xb8;

    c->memory[NR21] &= 0x3f;
    c->memory[NR22] = 0x00;
    c->memory[NR23] = 0x00;
    c->memory[NR24] = 0xb8;

    c->memory[NR30] = 0x7f;
    c->memory[NR32] = 0x00;
    c->memory[NR33] = 0x00;
    c->memory[NR34] = 0xb8;

    c->memory[NR42] = 0x00;
    c->memory[NR43] = 0x00;
    c->memory[NR44] = 0xbf;

    c->memory[NR50] = 0x00;
    c->memory[NR51] = 0x00;
    c->memory[NR52] = 0x70;
}

void set_mem(cpu *c, uint16_t addr, uint8_t value) {
    uint8_t next_module;
    switch (addr) {
        case 0x0000 ... 0x7fff:
            switch (c->cart.type) {
                // MBC 1
                case 0x1 ... 0x3:
                    switch (addr) {
                        case 0x0000 ... 0x1fff:
                            if (c->cart.type != 0x1) {
                                if ((value & 0xf) == 0xa)
                                    c->cart.ram_enable = true;
                                else
                                    c->cart.ram_enable = false;
                            }
                            break;
                        case 0x2000 ... 0x3fff:
                            if ((value & 31) == 0) {
                                value = 1;
                            }
                            c->cart.bank_select = ((c->cart.bank_select & 96) | (value & 31)) % c->cart.banks;
                            break;
                        case 0x4000 ... 0x5fff:
                            value &= 3;
                            if (c->cart.mbc1mode) {
                                if (c->cart.banks_ram > 1) {
                                    c->cart.bank_select_ram = value;
                                }
                            }
                            else {
                                if (c->cart.banks_ram > 1) {
                                    c->cart.bank_select_ram = 0;
                                }
                            }

                            if (c->cart.banks > 32) {
                                c->cart.bank_select = ((c->cart.bank_select & 31) | (value << 5)) % c->cart.banks;
                            }

                            break;
                        case 0x6000 ... 0x7fff:
                            c->cart.mbc1mode = value & 1;
                            break;
                    }
                    break;

                // MBC 2
                case 0x5 ... 0x6:
                    if (addr < 0x4000) {
                        if ((addr & 0x100) != 0) {
                            if ((value & 15) == 0) {
                                value = 1;
                            }
                            c->cart.bank_select = (value & 15) % c->cart.banks;
                        } else {
                            if ((value & 0xf) == 0x0a)
                                c->cart.ram_enable = true;
                            else
                                c->cart.ram_enable = false;
                        }
                        break;
                    }
                    break;

                // MBC 3
                case 0x11 ... 0x13:
                    switch (addr) {
                        case 0x0000 ... 0x1fff:
                            if (c->cart.type != 0x11) {
                                if (value == 0x0a)
                                    c->cart.ram_enable = true;
                                else if (value == 0)
                                    c->cart.ram_enable = false;
                            }
                            break;
                        case 0x2000 ... 0x3fff:
                            if ((value & (c->cart.banks - 1)) == 0) {
                                value = 1;
                            }
                            c->cart.bank_select = value % c->cart.banks;
                            break;
                        case 0x4000 ... 0x5fff:
                            if (c->cart.type != 0x11) {
                                if (c->cart.banks_ram > 1) {
                                    if (value >= 0 && value <= 3) {
                                        c->cart.bank_select_ram = value;
                                    }
                                }
                            }
                            break;
                        case 0x6000 ... 0x7fff:
                            break;
                    }
                    break;

                // MBC 3 RTC
                case 0x0f ... 0x10:
                    switch (addr) {
                        case 0x0000 ... 0x1fff:
                            if (value == 0x0a)
                                c->cart.ram_enable = true;
                            else if (value == 0)
                                c->cart.ram_enable = false;
                            break;
                        case 0x2000 ... 0x3fff:
                            if ((value & (c->cart.banks - 1)) == 0) {
                                value = 1;
                            }
                            c->cart.bank_select = value % c->cart.banks;
                            break;
                        case 0x4000 ... 0x5fff:
                            if (c->cart.banks_ram > 1) {
                                if (value >= 0 && value <= 3) {
                                    c->cart.bank_select_ram = value;
                                }
                                else if (value >= 0x08 && value <= 0x0c) {
                                    c->cart.bank_select_ram = value;
                                }
                            }
                            else {
                                if (value >= 0x08 && value <= 0x0c) {
                                    c->cart.bank_select_ram = value;
                                }
                            }
                            break;
                        case 0x6000 ... 0x7fff:
                            if (value == 0) {
                                c->cart.rtc.about_to_latch = false;
                            }
                            else if (value == 1) {
                                if (!c->cart.rtc.about_to_latch) {
                                    c->cart.rtc.seconds = c->cart.rtc.time % 60;
                                    c->cart.rtc.minutes = (c->cart.rtc.time / 60) % 60;
                                    c->cart.rtc.hours = (c->cart.rtc.time / 3600) % 24;
                                    c->cart.rtc.days = (c->cart.rtc.time / 86400);

                                    if (c->cart.rtc.days > 511) {
                                        c->cart.rtc.day_carry = true;
                                        c->cart.rtc.days &= 511;
                                        c->cart.rtc.time += 512 * 3600 * 24;
                                    }
                                }
                                c->cart.rtc.about_to_latch = true;
                            }

                            break;
                    }
                    break;

                // MBC 5
                case 0x19 ... 0x1e:
                    switch (addr) {
                        case 0x0000 ... 0x1fff:
                            if (c->cart.type != 0x19 && c->cart.type != 0x1c) {
                                if (value == 0x0a)
                                    c->cart.ram_enable = true;
                                else if (value == 0)
                                    c->cart.ram_enable = false;
                            }
                            break;
                        case 0x2000 ... 0x2fff:
                            c->cart.bank_select = value | (c->cart.bank_select & 0x100);
                            c->cart.bank_select %= c->cart.banks;
                            break;
                        case 0x3000 ... 0x3fff:
                            if ((value & 1) == 0) {
                                c->cart.bank_select &= 255;
                            }
                            else {
                                c->cart.bank_select |= 256;
                            }
                            c->cart.bank_select %= c->cart.banks;
                            break;
                        case 0x4000 ... 0x5fff:
                            if (c->cart.type != 0x19 && c->cart.type != 0x1c) {
                                if (c->cart.banks_ram > 1) {
                                    if ((value >= 0 && value < 16) && value <= c->cart.banks_ram) {
                                        c->cart.bank_select_ram = value;
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
            break;

        case 0xa000 ... 0xbfff: // Ram
            if (c->cart.ram_enable) {
                if (c->cart.type == 1 || c->cart.type == 2 || c->cart.type == 3) {
                    if (c->cart.mbc1mode)
                        c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
                    else
                        c->cart.ram[0][addr - 0xa000] = value;
                }
                // MBC 2
                else if (c->cart.type == 5 || c->cart.type == 6) {
                    c->cart.ram[c->cart.bank_select_ram][(addr - 0xa000) & 0x1ff] = value;
                }
                // RTC
                else if ((c->cart.type == 0x0f || c->cart.type == 0x10) && c->cart.bank_select_ram >= 8) {
                    uint8_t seconds = c->cart.rtc.time % 60;
                    uint8_t minutes = (c->cart.rtc.time / 60) % 60;
                    uint8_t hours = (c->cart.rtc.time / 3600) % 24;
                    uint16_t days = c->cart.rtc.time / 86400;
                    switch (c->cart.bank_select_ram) {
                        case 0x08:
                            //if (value < 60)
                                seconds = value;
                            break;
                        case 0x09:
                            //if (value < 60)
                                minutes = value;
                            break;
                        case 0x0a:
                            //if (value < 24)
                                hours = value;
                            break;
                        case 0x0b:
                            days = (days & 0xff00) | value;
                            break;
                        case 0x0c:
                            days = (days & 0xff) | ((value & 1) << 8);
                            c->cart.rtc.is_halted = (value >> 6) & 1;
                            c->cart.rtc.day_carry = (value >> 7) & 1;
                            break;
                        default:
                            break;
                    }
                    c->cart.rtc.time = seconds + (minutes * 60) + (hours * 3600) + (days * 86400);
                }
                else if (c->cart.type != 0x0f) {
                    c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
                }
            }
            break;

        case 0xe000 ... 0xfdff: // Echo ram
            c->memory[addr - 0x2000] = value;
            break;

        case JOYP:
            if (((value >> 4) & 1) == 0)
                joypad1.dpad_on = true;
            else
                joypad1.dpad_on = false;
            if (((value >> 5) & 1) == 0)
                joypad1.btn_on = true;
            else
                joypad1.btn_on = false;

        case LYC:
            if (video.is_on) {
                if (value == get_mem(c, LY)) {
                    video.ly_eq_lyc = true;
                }
                else {
                    video.ly_eq_lyc = false;
                }
            }
            c->memory[addr] = value;
            break;

        case STAT: // STAT Interrupt
            c->memory[IF] |= 2;
            video.lyc_select = (value >> 6) & 1;
            video.mode_select = (value >> 3) & 7;
            break;

        case DMA: // OAM DMA transfer
            c->memory[addr] = value;
            dma_transfer(c);
            video.dma_transfer = 640;
            video.need_sprites_reload = true;
            break;

        case DIV: // divider register
            timer1.reset_timer = true;
            break;

        case TIMA:
            timer1.delay = false;
            timer1.tima = value;
            break;
        case TMA:
            timer1.tma = value;
            break;
        case TAC: // divider register
            next_module = value & 3;
            if ((value & 4) != 0) {
                if (timer1.is_tac_on && ((timer1.t_states & clock_tac_shift2[timer1.module]) != 0 && (timer1.t_states & clock_tac_shift2[next_module]) == 0)) {
                    timer1.tima++;
                    if (timer1.tima == 0) {
                        timer1.delay = true;
                    }
                }
                timer1.is_tac_on = true;
            }
            else {
                if (timer1.is_tac_on && (timer1.t_states & clock_tac_shift2[timer1.module]) != 0) {
                    timer1.tima++;
                    if (timer1.tima == 0) {
                        timer1.delay = true;
                    }
                }
                timer1.is_tac_on = false;
            }
            timer1.module = next_module;
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

            bool prev_is_on = video.is_on;
            video.is_on = (value >> 7) & 1;
            // PPU turned OFF
            if (!video.is_on && prev_is_on) {
                video.mode = 0;
                video.scan_line = 0;
                timer1.scanline_timer = 456;
                timer1.lcdoff_timer += 69768;
                load_display(c);
            }
            // PPU turned ON
            else if (video.is_on && !prev_is_on) {
                if (c->memory[LYC] == get_mem(c, LY)) {
                    video.ly_eq_lyc = true;
                    if (video.lyc_select)
                        c->memory[IF] |= 2;
                }
                else
                    video.ly_eq_lyc = false;
            }

            video.need_bg_wn_reload = true;
            video.tiles_write = true;
            video.tilemap_write = true;
            video.need_sprites_reload = true;
            break;

        case BGP:
            video.bgp[0] = value & 3;
            video.bgp[1] = (value >> 2) & 3;
            video.bgp[2] = (value >> 4) & 3;
            video.bgp[3] = (value >> 6) & 3;
            break;

        case OBP0:
            video.obp[0][0] = value & 3;
            video.obp[0][1] = (value >> 2) & 3;
            video.obp[0][2] = (value >> 4) & 3;
            video.obp[0][3] = (value >> 6) & 3;
            break;

        case OBP1:
            video.obp[1][0] = value & 3;
            video.obp[1][1] = (value >> 2) & 3;
            video.obp[1][2] = (value >> 4) & 3;
            video.obp[1][3] = (value >> 6) & 3;
            break;

        case NR11:
            audio.ch1.lenght = value & 0x3f;
            if (audio.is_on)
                c->memory[addr] = value;
            else
                c->memory[addr] = value & 0x3f;
            break;
        case NR10: case NR12: case NR13:
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

        case NR21:
            audio.ch2.lenght = value & 0x3f;
            if (audio.is_on)
                c->memory[addr] = value;
            else
                c->memory[addr] = value & 0x3f;
            break;
        case NR22: case NR23:
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

        case NR31:
            audio.ch3.lenght = value;
            c->memory[addr] = value;
            break;

        case NR30: case NR32: case NR33:
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

        case NR41:
            audio.ch4.lenght = value & 0x3f;
            c->memory[addr] = value;
            break;

        case NR42: case NR43:
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

        case 0xff50:
            c->bootrom.is_enabled = false;
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
            if (c->bootrom.is_enabled && addr < 0x100)
                return c->bootrom.data[addr];
            if (c->cart.type == 1 || c->cart.type == 2 || c->cart.type == 3) {
                if (c->cart.mbc1mode)
                    return c->cart.data[c->cart.bank_select & 96][addr];
                return c->cart.data[0][addr];
            }
            return c->cart.data[0][addr];

        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr-0x4000];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable) {
                // MBC 1 Bit mode
                if (c->cart.type == 1 || c->cart.type == 2 || c->cart.type == 3) {
                    if (c->cart.mbc1mode)
                        return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
                    return c->cart.ram[0][addr - 0xa000];
                }
                // MBC 2 built-in RAM
                if (c->cart.type == 5 || c->cart.type == 6) {
                    return (c->cart.ram[0][(addr - 0xa000) & 0x01ff] | 0xf0);
                }
                // RTC
                else if ((c->cart.type == 0x0f || c->cart.type == 0x10) && c->cart.bank_select_ram >= 8) {
                    switch (c->cart.bank_select_ram) {
                        case 0x08:
                            return c->cart.rtc.seconds;
                        case 0x09:
                            return c->cart.rtc.minutes;
                        case 0x0a:
                            return c->cart.rtc.hours;
                        case 0x0b:
                            return c->cart.rtc.days;
                        case 0x0c:
                            return (c->cart.rtc.day_carry << 7) | (c->cart.rtc.is_halted << 6) | ((c->cart.rtc.days & 256) >> 8);
                    }
                }
                else if (c->cart.type != 0x0f){
                    return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
                }
            }
            return 255;
        case 0xfea0 ... 0xfeff:
            return 255;
        case 0xe000 ... 0xfdff:
            return c->memory[addr - 0x2000];

        case JOYP:
            if (joypad1.btn_on && joypad1.dpad_on) {
                uint8_t enc = 0;
                for (int i = 0; i < 4; i++) {
                    enc |= (joypad1.btn[i]) << i;
                }
                return (joypad1.dpad_on << 4) | (joypad1.dpad_on << 5) | enc | 0xc0;
            }
            else if (joypad1.btn_on) {
                uint8_t enc = 0;
                for (int i = 0; i < 4; i++) {
                    enc |= (joypad1.btn[i]) << i;
                }
                return (joypad1.dpad_on << 4) | (joypad1.dpad_on << 5) | enc | 0xc0;
            }
            else if (joypad1.dpad_on) {
                uint8_t enc = 0;
                for (int i = 0; i < 4; i++) {
                    enc |= (joypad1.dpad[i]) << i;
                }
                return (joypad1.dpad_on << 4) | (joypad1.dpad_on << 5) | enc | 0xc0;
            }
            else {
                return (joypad1.dpad_on << 4) | (joypad1.dpad_on << 5) | 0xcf;
            }
        case SC:
            return c->memory[addr] | 0x7e;
        case DIV:
            return (timer1.t_states >> 8) & 255;
        case TIMA:
            return timer1.tima;
        case TMA:
            return timer1.tma;
        case TAC:
            return (timer1.is_tac_on << 2) | timer1.module | 0xf8;
        case IF:
            return c->memory[addr] | 0xe0;
        case STAT:
            if (video.is_on)
                return video.mode | (video.ly_eq_lyc << 2) | (video.mode_select << 3) | (video.lyc_select << 6) | 0x80;
            else
                return (video.ly_eq_lyc << 2) | (video.mode_select << 3) | (video.lyc_select << 6) | 0x80;

        case LY:
            if (video.scan_line == 153) {
                return 0;
            }
            if (timer1.scanline_timer == 4) {
                return video.scan_line + 1;
            }
            return video.scan_line;
        case LCDC:
            return video.bg_enable | (video.obj_enable << 1) | (video.obj_size << 2) | (video.bg_tilemap << 3) | (video.bg_tiles << 4) | (video.window_enable << 5) | (video.window_tilemap << 6) | (video.is_on << 7);

        case BGP:
            return video.bgp[0] | (video.bgp[1] << 2) | (video.bgp[2] << 4) | (video.bgp[3] << 6);

        case OBP0:
            return video.obp[0][0] | (video.obp[0][1] << 2) | (video.obp[0][2] << 4) | (video.obp[0][3] << 6);

        case OBP1:
            return video.obp[1][0] | (video.obp[1][1] << 2) | (video.obp[1][2] << 4) | (video.obp[1][3] << 6);

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
            return (c->memory[addr] & 0xf0) | (audio.ch1.is_active | (audio.ch2.is_active << 1) | (audio.ch3.is_active << 2) | (audio.ch4.is_active << 3) | 0x70);

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
        add_ticks(c, 4);
        set_mem(c, c->r.reg16[HL], p->imm8);
        return 8;
    }
    else {
        *r8(c, p->operand_stk_r8) = p->imm8;
        return 8;
    }
}

uint8_t ld_imm16_a(cpu *c, parameters *p) {
    c->pc += 3;
    add_ticks(c, 8);
    set_mem(c, p->imm16, c->r.reg8[A]);
    return 8;
}

uint8_t ld_a_imm16(cpu *c, parameters *p) {
    c->pc += 3;
    add_ticks(c, 8);
    c->r.reg8[A] = get_mem(c, p->imm16);
    return 8;
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

uint8_t inc_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_stk_r8 == 6) {
        uint8_t mem = get_mem(c, c->r.reg16[HL]);
        add_ticks(c, 4);
        set_mem(c, c->r.reg16[HL], (mem + 1));
        uint8_t mem_next = get_mem(c, c->r.reg16[HL]);
        add_ticks(c, 4);

        if ((((mem & 0xf) + 1) & 0xf) < (mem & 0xf))
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;

        if (mem_next == 0)
            c->r.reg8[F] |= flagZ;
        else
            c->r.reg8[F] &= ~flagZ;
    }
    else {
        if ((((*r8(c, p->operand_stk_r8) & 0xf) + 1) & 0xf) < (*r8(c, p->operand_stk_r8) & 0xf))
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;

        *r8(c, p->operand_stk_r8) += 1;

        if (*r8(c, p->operand_stk_r8) == 0)
            c->r.reg8[F] |= flagZ;
        else
            c->r.reg8[F] &= ~flagZ;
    }
    c->r.reg8[F] &= ~flagN;
    return 4;
}

uint8_t dec_r8(cpu *c, parameters *p) {
    c->pc += 1;
    if (p->operand_stk_r8 == 6) {
        uint8_t mem = get_mem(c, c->r.reg16[HL]);
        add_ticks(c, 4);
        set_mem(c, c->r.reg16[HL], (mem - 1));
        uint8_t mem_next = get_mem(c, c->r.reg16[HL]);
        add_ticks(c, 4);

        if ((((mem & 0xf) - 1) & 0xf) > (mem & 0xf))
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;

        if (mem_next == 0)
            c->r.reg8[F] |= flagZ;
        else
            c->r.reg8[F] &= ~flagZ;
    }
    else {
        if ((((*r8(c, p->operand_stk_r8) & 0xf) - 1) & 0xf) > (*r8(c, p->operand_stk_r8) & 0xf))
            c->r.reg8[F] |= flagH;
        else
            c->r.reg8[F] &= ~flagH;

        *r8(c, p->operand_stk_r8) -= 1;

        if (*r8(c, p->operand_stk_r8) == 0)
            c->r.reg8[F] |= flagZ;
        else
            c->r.reg8[F] &= ~flagZ;
    }

    c->r.reg8[F] |= flagN;

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
        return 12;
    }
    return 8;
}

uint8_t di(cpu *c, parameters *p) {
    c->pc += 1;
    c->ime = false;
    c->ime_to_be_setted = 0;
    return 4;
}

uint8_t ldh_imm8_a(cpu *c, parameters *p) {
    c->pc += 2;
    add_ticks(c, 4);
    set_mem(c, (0xff00 | p->imm8), c->r.reg8[A]);
    return 8;
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
    add_ticks(c, 4);
    c->r.reg8[A] = get_mem(c, (0xff00 | p->imm8));
    return 8;
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
    uint8_t iszero;
    uint8_t b3 = (p->imm8 & 0b00111000) >> 3;
    switch(p->imm8) {
        // RLC
        case 0x00 ... 0x07:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                if ((mem & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem << 1);

                if ((c->r.reg8[F] & flagC) != 0)
                    set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 1));

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;

                if ((c->r.reg8[F] & flagC) != 0)
                    *r8(c, p->operand_r8) |= 1;
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // RRC
        case 0x08 ... 0x0f:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                if ((mem & 1) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem >> 1);

                if ((c->r.reg8[F] & flagC) != 0)
                    set_mem(c, c->r.reg16[HL], (get_mem(c, c->r.reg16[HL]) | 128));
                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;

                if ((c->r.reg8[F] & flagC) != 0)
                    *r8(c, p->operand_r8) |= 128;

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // RL
        case 0x10 ... 0x17:
            prevC = c->r.reg8[F] & flagC;
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem << 1);
                if (prevC != 0)
                    set_mem(c, c->r.reg16[HL], get_mem(c, c->r.reg16[HL]) | 1);

                if ((mem & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) != 0)
                    c->r.reg8[F] |= flagC;
                else
                    c->r.reg8[F] &= ~flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;
                if (prevC != 0)
                    *r8(c, p->operand_r8) |= 1;
                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // RR
        case 0x18 ... 0x1f:
            prevC = (c->r.reg8[F] >> 4) & 1;;
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem >> 1);
                set_mem(c, c->r.reg16[HL], get_mem(c, c->r.reg16[HL]) | (prevC << 7));

                if ((mem & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
                *r8(c, p->operand_r8) |= (prevC << 7);

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // SLA
        case 0x20 ... 0x27:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem << 1);

                if ((mem & 128) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 128) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) << 1;

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // SRA
        case 0x28 ... 0x2f:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);

                if ((mem & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem >> 1);
                uint8_t new_val = get_mem(c, c->r.reg16[HL]);
                new_val |= (new_val & 64) << 1;
                set_mem(c, c->r.reg16[HL], new_val);

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;
                *r8(c, p->operand_r8) |= (*r8(c, p->operand_r8) & 64) << 1;

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }

            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;

            return 8;
        // SWAP
        case 0x30 ... 0x37:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t low = (get_mem(c, c->r.reg16[HL]) & 15) << 4;
                uint8_t high = get_mem(c, c->r.reg16[HL]) >> 4;
                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], (high | low));

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                uint8_t low = *r8(c, p->operand_r8) & 15;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 4;
                *r8(c, p->operand_r8) += low << 4;

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            c->r.reg8[F] &= ~flagC;

            return 8;
        // SRL
        case 0x38 ... 0x3f:
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] &= ~flagH;
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);
                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], mem >> 1);

                if ((mem & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;

                if (get_mem(c, c->r.reg16[HL]) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            else {
                if ((*r8(c, p->operand_r8) & 1) == 0)
                    c->r.reg8[F] &= ~flagC;
                else
                    c->r.reg8[F] |= flagC;
                *r8(c, p->operand_r8) = *r8(c, p->operand_r8) >> 1;

                if (*r8(c, p->operand_r8) == 0)
                    c->r.reg8[F] |= flagZ;
                else
                    c->r.reg8[F] &= ~flagZ;
            }
            return 8;
        // BIT
        case 0x40 ... 0x7f:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                iszero = (get_mem(c, c->r.reg16[HL]) >> b3) & 1;
            }
            else
                iszero = (*r8(c, p->operand_r8) >> b3) & 1;
            if (iszero == 0)
                c->r.reg8[F] |= flagZ;
            else
                c->r.reg8[F] &= ~flagZ;
            c->r.reg8[F] &= ~flagN;
            c->r.reg8[F] |= flagH;
            return 8;
        // RES
        case 0x80 ... 0xbf:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);
                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], (mem & ~(1 << b3)));
            }
            else
                *r8(c, p->operand_r8) &= ~(1 << b3);
            return 8;
        // SET
        case 0xc0 ... 0xff:
            if (p->operand_r8 == 6) {
                add_ticks(c, 4);
                uint8_t mem = get_mem(c, c->r.reg16[HL]);
                add_ticks(c, 4);
                set_mem(c, c->r.reg16[HL], (mem | 1 << b3));
            }
            else
                *r8(c, p->operand_r8) |= (1 << b3);
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
    if (c->ime_to_be_setted == 0) {
        c->ime_to_be_setted = 1;
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
    uint16_t temp;
    if (p->operand_r8 == 6)
        temp = c->r.reg8[A] - get_mem(c, c->r.reg16[HL]) - carry;
    else
        temp = c->r.reg8[A] - *r8(c, p->operand_r8) - carry;
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
    c->is_halted = true;
    return 4;
}

uint8_t stop(cpu *c, parameters *p) {
    c->pc += 2;
    return 4;
}
