#include <string.h>
#include <stdio.h>
#include "../includes/memory.h"
#include "../includes/apu.h"
#include "../includes/cartridge.h"
#include "../includes/settings.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include "../includes/serial.h"
#include "../includes/input.h"
#include "../includes/opcodes.h"
#include "../includes/camera.h"
#include "../includes/cheats.h"

const uint16_t clock_tac_shift2[] = {0x200, 0x8, 0x20, 0x80};

uint8_t read_no_mbc(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[1][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            return 0xff;
    }
}

void write_mbc1(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x1fff:
            if (c->cart.has_ram) {
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
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable && c->cart.has_ram) {
                if (c->cart.mbc1mode)
                    c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
                else
                    c->cart.ram[0][addr - 0xa000] = value;
            }
            break;
    }
}

uint8_t read_mbc1(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            if (c->cart.mbc1mode)
                return c->cart.data[c->cart.bank_select & 96][addr & 0x3fff];
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable && c->cart.has_ram) {
                if (c->cart.mbc1mode)
                    return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
                return c->cart.ram[0][addr - 0xa000];
            }
            return 0xff;
    }
}

void write_mbc2(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
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
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable)
                c->cart.ram[c->cart.bank_select_ram][(addr - 0xa000) & 0x1ff] = value;
            break;
        default:
            break;
    }
}

uint8_t read_mbc2(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable) {
                return (c->cart.ram[0][(addr - 0xa000) & 0x01ff] | 0xf0);
            }
            return 0xff;
    }
}

void write_mbc3(cpu *c, uint16_t addr, uint8_t value) {
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
            if (c->cart.has_rtc) {
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
            }
            break;
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable) {
                if (c->cart.bank_select_ram >= 8 && c->cart.has_rtc) {
                    uint8_t seconds = c->cart.rtc.time % 60;
                    uint8_t minutes = (c->cart.rtc.time / 60) % 60;
                    uint8_t hours = (c->cart.rtc.time / 3600) % 24;
                    uint16_t days = c->cart.rtc.time / 86400;
                    switch (c->cart.bank_select_ram) {
                        case 0x08:
                            seconds = value;
                            break;
                        case 0x09:
                            minutes = value;
                            break;
                        case 0x0a:
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
                else if (c->cart.has_ram) {
                    c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
                }
            }
            break;
    }
}

uint8_t read_mbc3(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable) {
                if (c->cart.bank_select_ram >= 8 && c->cart.has_rtc) {
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
                else if (c->cart.has_ram)
                    return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
            }

            return 0xff;
    }
}

void write_mbc5(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x1fff:
            if (c->cart.has_ram) {
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
            if (c->cart.has_ram) {
                if ((value >= 0 && value < 16) && value <= c->cart.banks_ram) {
                    c->cart.bank_select_ram = value;
                }
            }
            break;
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable && c->cart.has_ram) {
                c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
            }
            break;
        default:
            break;
    }
}

uint8_t read_mbc5(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            if (c->cart.ram_enable && c->cart.has_ram) {
                return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
            }
            return 0xff;
    }
}

void write_mbc7(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x1fff:
            if (value == 0x0a)
                c->cart.ram_enable = true;
            else if (value == 0)
                c->cart.ram_enable = false;
            break;
        case 0x2000 ... 0x3fff:
            c->cart.bank_select = value & 0x7f;
            break;
        case 0x4000 ... 0x5fff:
            if (value == 0x40)
                c->cart.ram_enable2 = true;
            else
                c->cart.ram_enable2 = false;
            break;
        case 0xa000 ... 0xafff:
            if (c->cart.ram_enable && c->cart.ram_enable2) {
                addr &= 0xf0f0;
                switch (addr) {
                    case 0xa000:
                        if (value == 0x55) {
                            c->cart.accel.has_latched = false;
                            c->cart.accel.x_latched = 0x8000;
                            c->cart.accel.y_latched = 0x8000;
                        }
                        break;
                    case 0xa010:
                        if (!c->cart.accel.has_latched && value == 0xaa) {
                            c->cart.accel.has_latched = true;
                            c->cart.accel.x_latched = 0x81d0 + get_x_accel();
                            c->cart.accel.y_latched = 0x81d0 + get_y_accel();
                        }
                        break;
                    case 0xa080:
                        c->cart.eeprom.CS = value >> 7;
                        c->cart.eeprom.DI = (value >> 1) & 1;
                        if (c->cart.eeprom.CS) {
                            if (!c->cart.eeprom.CLK && (value & 0x40)) {
                                c->cart.eeprom.DO = c->cart.eeprom.read_bits >> 15;
                                c->cart.eeprom.read_bits <<= 1;
                                c->cart.eeprom.read_bits |= 1;
                                if (c->cart.eeprom.argument_bits_left == 0) {
                                    c->cart.eeprom.command <<= 1;
                                    c->cart.eeprom.command |= c->cart.eeprom.DI;
                                    if (c->cart.eeprom.command & 0x400) {
                                        switch ((c->cart.eeprom.command >> 6) & 0xf) {
                                            case 0x8:
                                            case 0x9:
                                            case 0xa:
                                            case 0xb:
                                                c->cart.eeprom.read_bits = ((uint16_t *)c->cart.ram)[c->cart.eeprom.command & 0x7f];
                                                c->cart.eeprom.command = 0;
                                                break;
                                            case 0x3:
                                                c->cart.eeprom.write_enable = true;
                                                c->cart.eeprom.command = 0;
                                                break;
                                            case 0x0:
                                                c->cart.eeprom.write_enable = false;
                                                c->cart.eeprom.command = 0;
                                                break;
                                            case 0x4:
                                            case 0x5:
                                            case 0x6:
                                            case 0x7:
                                                if (c->cart.eeprom.write_enable) {
                                                    ((uint16_t *)c->cart.ram)[c->cart.eeprom.command & 0x7f] = 0;
                                                }
                                                c->cart.eeprom.argument_bits_left = 16;
                                                break;
                                            case 0xc:
                                            case 0xd:
                                            case 0xe:
                                            case 0xf:
                                                c->cart.eeprom.command = 0;
                                                break;
                                            case 0x2:
                                                if (c->cart.eeprom.write_enable) {
                                                    memset(c->cart.ram, 0xff, 256);
                                                    ((uint16_t *)c->cart.ram)[c->cart.eeprom.command & 0x7f] = 0xffff;
                                                    c->cart.eeprom.read_bits = 0xff;
                                                }
                                                c->cart.eeprom.command = 0;
                                                break;
                                            case 0x1:
                                                if (c->cart.eeprom.write_enable) {
                                                    memset(c->cart.ram, 0, 256);
                                                }
                                                c->cart.eeprom.argument_bits_left = 16;
                                                break;
                                        }
                                    }
                                }
                                else {
                                    c->cart.eeprom.argument_bits_left--;
                                    c->cart.eeprom.DO = true;
                                    if (c->cart.eeprom.DI) {
                                        uint16_t bit = (1 << c->cart.eeprom.argument_bits_left);
                                        if (c->cart.eeprom.command & 0x100) {
                                            ((uint16_t *)c->cart.ram)[c->cart.eeprom.command & 0x7f] |= bit;
                                        }
                                        else {
                                            for (uint8_t i = 0; i < 0x7f; i++) {
                                                ((uint16_t *)c->cart.ram)[i] |= bit;
                                            }
                                        }
                                    }
                                    if (c->cart.eeprom.argument_bits_left == 0) {
                                        c->cart.eeprom.command = 0;
                                        c->cart.eeprom.read_bits = (c->cart.eeprom.command & 0x100) ? 0xff : 0x3fff;
                                    }
                                }
                            }
                        }
                        c->cart.eeprom.CLK = (value >> 6) & 1;
                        break;
                    default:
                        break;
                }
            }
            break;
        default:
            break;
    }
}

uint8_t read_mbc7(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xafff:
            if (c->cart.ram_enable && c->cart.ram_enable2) {
                addr &= 0xf0f0;
                switch (addr) {
                    case 0xa020:
                        return c->cart.accel.x_latched & 0xff;
                    case 0xa030:
                        return c->cart.accel.x_latched >> 8;
                    case 0xa040:
                        return c->cart.accel.y_latched & 0xff;
                    case 0xa050:
                        return c->cart.accel.y_latched >> 8;
                    case 0xa080:
                        return c->cart.eeprom.DO | (c->cart.eeprom.DI << 1) | (c->cart.eeprom.CLK << 6) | (c->cart.eeprom.CS << 7);
                    default:
                        return 0xff;
                }
            }
        case 0xb000 ... 0xbfff:
            return 0xff;
    }
}

void write_pocket_camera(cpu *c, uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... 0x1fff:
            if (value == 0x0a)
                c->cart.ram_enable = true;
            else if (value == 0)
                c->cart.ram_enable = false;
            break;
        case 0x2000 ... 0x3fff:
            if ((value & 127) == 0) {
                value = 1;
            }
            c->cart.bank_select = value & 127;
            break;
        case 0x4000 ... 0x5fff:
            c->cart.bank_select_ram = value;
            break;
        case 0x6000 ... 0x7fff:
            if (c->cart.bank_select_ram < 16 && c->cart.ram_enable) {
                c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
            }
            else if (c->cart.bank_select_ram == 16) {
                uint8_t reg = addr & 0x7f;
                if (reg == 0) {
                    value &= 7;
                    if ((value & 1) && !(gbcamera.reg[0] & 1)) {
                        take_picture(c);
                    }
                    if (!(value & 1) && (gbcamera.reg[0] & 1)) {
                        value |= 1;
                    }
                    gbcamera.reg[0] = value;
                }
                else {
                    if (reg <= 0x36) {
                        gbcamera.reg[reg] = value;
                    }
                }
            }
            break;
        case 0xa000 ... 0xbfff:
            if (c->cart.bank_select_ram < 16 && c->cart.ram_enable) {
                c->cart.ram[c->cart.bank_select_ram][addr - 0xa000] = value;
            }
            else if (c->cart.bank_select_ram == 16) {
                uint8_t reg = addr & 0x7f;
                if (reg == 0) {
                    value &= 7;
                    if ((value & 1) && !(gbcamera.reg[0] & 1)) {
                        take_picture(c);
                    }
                    if (!(value & 1) && (gbcamera.reg[0] & 1)) {
                        value |= 1;
                    }
                    gbcamera.reg[0] = value;
                }
                else {
                    if (reg <= 0x36) {
                        gbcamera.reg[reg] = value;
                    }
                }
            }
            break;
    }
}

uint8_t read_pocket_camera(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3fff:
            return c->cart.data[0][addr & 0x3fff];
        case 0x4000 ... 0x7fff:
            return c->cart.data[c->cart.bank_select][addr & 0x3fff];
        case 0xa000 ... 0xbfff:
            if (c->cart.bank_select_ram < 16)
                return c->cart.ram[c->cart.bank_select_ram][addr - 0xa000];
            else {
                uint8_t reg = (addr & 0x7f);
                if (reg == 0) {
                    return gbcamera.reg[0];
                }
                return 0;
            }
    }
}

void write_cart(cpu *c, uint16_t addr, uint8_t value) {
    switch (c->cart.mbc) {
        case MBC1:
            write_mbc1(c, addr, value);
            break;

        case MBC2:
            write_mbc2(c, addr, value);
            break;

        case MBC3:
            write_mbc3(c, addr, value);
            break;

        case MBC5:
            write_mbc5(c, addr, value);
            break;

        case MBC7:
            write_mbc7(c, addr, value);
            break;

        case POCKET_CAMERA:
            write_pocket_camera(c, addr, value);
            break;

        default:
            break;
    }
}

uint8_t read_cart(cpu *c, uint16_t addr) {
    switch (c->cart.mbc) {
        case NO_MBC:
            return read_no_mbc(c, addr);

        case MBC1:
            return read_mbc1(c, addr);

        case MBC2:
            return read_mbc2(c, addr);

        case MBC3:
            return read_mbc3(c, addr);

        case MBC5:
            return read_mbc5(c, addr);

        case MBC7:
            return read_mbc7(c, addr);

        case POCKET_CAMERA:
            return read_pocket_camera(c, addr);

        default:
            return read_no_mbc(c, addr);
    }
}

void set_mem(cpu *c, uint16_t addr, uint8_t value) {
    uint8_t next_module;
    switch (addr) {
        case 0x0000 ... 0x7fff:
        case 0xa000 ... 0xbfff:
            write_cart(c, addr, value);
            break;

        case 0xc000 ... 0xcfff: // WRAM0
            c->wram[0][addr - 0xc000] = value;
            break;

        case 0xd000 ... 0xdfff: // WRAM1
            if (c->cgb_mode)
                c->wram[(c->wram_bank == 0) ? 1 : c->wram_bank][addr - 0xd000] = value;
            else
                c->wram[1][addr - 0xd000] = value;
            break;

        case 0xe000 ... 0xefff: // EchoRAM0
            c->wram[0][addr - 0xe000] = value;
            break;

        case 0xf000 ... 0xfdff: // EchoRAM1
            if (c->cgb_mode)
                c->wram[(c->wram_bank == 0) ? 1 : c->wram_bank][addr - 0xd000] = value;
            else
                c->wram[1][addr - 0xf000] = value;
            break;

        case 0x8000 ... 0x9fff: // VRAM
            if (c->cgb_mode)
                video.vram[video.vram_bank][addr-0x8000] = value;
            else
                video.vram[0][addr-0x8000] = value;
            break;

        case 0xfe00 ... 0xfe9f:
            c->memory[addr] = value;
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
            break;

        case SB:
            serial1.value = value;
            break;
        case SC:
            if (c->cgb_mode) {
                serial1.clock_speed = (value >> 1) & 1;
            }
            serial1.is_master = value & 1;
            serial1.is_transfering = (value >> 7) & 1;
            break;

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

        case LCDC:
            video.bg_enable = value & 1;
            video.obj_enable = (value >> 1) & 1;
            video.obj_size = (value >> 2) & 1;
            video.bg_tilemap = (value >> 3) & 1;
            video.bg_tiles = (value >> 4) & 1;
            video.window_enable = (value >> 5) & 1;
            video.window_tilemap = (value >> 6) & 1;

            bool prev_is_on = video.is_on;
            video.is_on = (value >> 7) & 1;
            // PPU turned OFF
            if (!video.is_on && prev_is_on) {
                video.mode = 0;
                video.scan_line = 0;
                timer1.scanline_timer = 456;
                timer1.lcdoff_timer += 70224;
                load_line();
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

            break;

        case SCX:
            video.scx = value;
            break;
        case SCY:
            video.scy = value;
            break;
        case WX:
            video.wx = value;
            break;
        case WY:
            video.wy = value;
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
            if (c->is_color) {
                if (c->memory[KEY0] == 0x04) {
                    c->cgb_mode = false;
                }
                else {
                    c->cgb_mode = true;
                }
            }
            break;

        case 0xff30 ... 0xff3f: // Wave Ram
            if (!audio.ch3.is_active)
                c->memory[addr] = value;
            break;

        case VBK:
            if (c->cgb_mode)
                video.vram_bank = value & 1;
            break;

        case BGP:
            video.bgp_dmg[0] = value & 3;
            video.bgp_dmg[1] = (value >> 2) & 3;
            video.bgp_dmg[2] = (value >> 4) & 3;
            video.bgp_dmg[3] = (value >> 6) & 3;
            break;

        case OBP0:
            video.obp_dmg[0][0] = value & 3;
            video.obp_dmg[0][1] = (value >> 2) & 3;
            video.obp_dmg[0][2] = (value >> 4) & 3;
            video.obp_dmg[0][3] = (value >> 6) & 3;
            break;

        case OBP1:
            video.obp_dmg[1][0] = value & 3;
            video.obp_dmg[1][1] = (value >> 2) & 3;
            video.obp_dmg[1][2] = (value >> 4) & 3;
            video.obp_dmg[1][3] = (value >> 6) & 3;
            break;

        case BCPS:
            if (c->cgb_mode) {
                video.bcps_inc = value >> 7;
                video.bgp_addr = value & 0x3f;
            }
            break;
        case BCPD:
            if (c->cgb_mode) {
                video.bgp[video.bgp_addr] = value;
                if (video.bcps_inc) {
                    video.bgp_addr = (video.bgp_addr + 1) & 0x3f;
                }
            }
            break;

        case OCPS:
            if (c->cgb_mode) {
                video.ocps_inc = value >> 7;
                video.obp_addr = value & 0x3f;
            }
            break;
        case OCPD:
            if (c->cgb_mode) {
                video.obp[video.obp_addr] = value;
                if (video.ocps_inc) {
                    video.obp_addr = (video.obp_addr + 1) & 0x3f;
                }
            }
            break;
        case SVBK:
            if (c->cgb_mode)
                c->wram_bank = value & 7;
            break;

        case HDMA1:
            if (c->cgb_mode)
                c->hdma.source = (c->hdma.source & 0xff) | (value << 8);
            break;
        case HDMA2:
            if (c->cgb_mode)
                c->hdma.source = (c->hdma.source & 0xff00) | value;
            break;

        case HDMA3:
            if (c->cgb_mode)
                c->hdma.destination = (c->hdma.destination & 0xff) | (value << 8);
            break;
        case HDMA4:
            if (c->cgb_mode)
                c->hdma.destination = (c->hdma.destination & 0xff00) | value;
            break;

        case HDMA5:
            if (c->cgb_mode) {
                c->hdma.finished = false;
                c->hdma.mode = value >> 7;
                c->hdma.lenght = ((value & 0x7f) + 1) << 4;
                c->hdma.status = 0;
                if (c->hdma.mode == false) {
                    c->is_halted = true;
                    c->gdma_halt = true;
                }
            }
            break;

        case KEY0:
            if (c->is_color) {
                c->memory[KEY0] = value;
            }
            break;

        case KEY1:
            if (c->is_color)
                c->armed = value & 1;
            break;

        case OPRI:
            if (c->bootrom.is_enabled && c->is_color) {
                video.opri = value & 1;
            }
            break;

        default:
            c->memory[addr] = value;
            break;
    }
}

uint8_t get_mem(cpu *c, uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x00ff:
            if (c->bootrom.is_enabled) {
                return c->bootrom.data[addr];
            }
        case 0x0200 ... 0x08ff:
            if (c->bootrom.is_enabled && c->is_color) {
                return c->bootrom.data[addr];
            }
        case 0x0100 ... 0x01ff:
        case 0x0900 ... 0x7fff:
            if (cheats.gameGenie_count > 0) {
                uint8_t cart_data = read_cart(c, addr);
                for (int i = 0; i < cheats.gameGenie_count; i++) {
                    if ((cheats.gameGenie[i].enabled) && (cheats.gameGenie[i].address == addr) && (cheats.gameGenie[i].old_data == cart_data)) {
                        return cheats.gameGenie[i].new_data;
                    }
                }
            }
        case 0xa000 ... 0xbfff:
            return read_cart(c, addr);
        case 0xfea0 ... 0xfeff:
            return 255;

        case 0xc000 ... 0xcfff: // WRAM0
            return c->wram[0][addr - 0xc000];

        case 0xd000 ... 0xdfff: // WRAM1
            if (c->cgb_mode)
                return c->wram[(c->wram_bank == 0) ? 1 : c->wram_bank][addr - 0xd000];
            return c->wram[1][addr - 0xd000];

        case 0xe000 ... 0xefff: // EchoRAM0
            return c->wram[0][addr - 0xe000];

        case 0xf000 ... 0xfdff: // EchoRAM1
            if (c->cgb_mode)
                return c->wram[(c->wram_bank == 0) ? 1 : c->wram_bank][addr - 0xf000];
            return c->wram[1][addr - 0xf000];


        case 0x8000 ... 0x9fff: // VRAM
            if (c->cgb_mode)
                return video.vram[video.vram_bank][addr-0x8000];
            return video.vram[0][addr-0x8000];

        case 0xfe00 ... 0xfe9f:
            return c->memory[addr];

        case SB:
            return serial1.value;
        case SC:
            if (c->cgb_mode)
                return (serial1.is_transfering << 7) | (serial1.clock_speed << 1) | (serial1.is_master) | 0x7c;
            else
                return (serial1.is_transfering << 7) | (serial1.is_master) | 0x7e;

        case SCX:
            return video.scx;
        case SCY:
            return video.scy;
        case WX:
            return video.wx;
        case WY:
            return video.wy;

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
            if (timer1.scanline_timer == 0) {
                return video.scan_line + 1;
            }
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
            return (c->memory[addr] & 0xf0) | (audio.ch1.is_active | (audio.ch2.is_active << 1) | (audio.ch3.is_active << 2) | (audio.ch4.is_active << 3) | 0x70);

        case 0xff30 ... 0xff3f:
            if (audio.ch3.is_active)
                return 0xff;
            return c->memory[addr];

        case KEY0:
            if (c->is_color)
                return c->memory[KEY0];
            return 0xff;

        case KEY1:
            if (c->is_color)
                return (c->double_speed << 7) | 0x7e | c->armed;
            return 0xff;

        case VBK:
            if (c->cgb_mode)
                return video.vram_bank | 0xfe;
            return 0xff;

        case HDMA1: case HDMA2: case HDMA3: case HDMA4:
            return 0xff;

        case HDMA5:
            if (c->cgb_mode) {
                if (c->hdma.finished)
                    return 0xff;
                return 0x00;
            }
            return 0xff;

        case BGP:
            return video.bgp_dmg[0] | (video.bgp_dmg[1] << 2) | (video.bgp_dmg[2] << 4) | (video.bgp_dmg[3] << 6);
        case OBP0:
            return video.obp_dmg[0][0] | (video.obp_dmg[0][1] << 2) | (video.obp_dmg[0][2] << 4) | (video.obp_dmg[0][3] << 6);
        case OBP1:
            return video.obp_dmg[1][0] | (video.obp_dmg[1][1] << 2) | (video.obp_dmg[1][2] << 4) | (video.obp_dmg[1][3] << 6);

        case BCPS:
            if (c->cgb_mode)
                return (video.bcps_inc << 7) | video.bgp_addr;
            return 0xff;
        case BCPD:
            if (c->cgb_mode)
                return video.bgp[video.bgp_addr];
            return 0xff;
        case OCPS:
            if (c->cgb_mode)
                return (video.ocps_inc << 7) | video.obp_addr;
            return 0xff;
        case OCPD:
            if (c->cgb_mode)
                return video.obp[video.obp_addr];
            return 0xff;

        case OPRI:
            if (c->is_color)
                return 0xfe | video.opri;
            return 0xff;

        case SVBK:
            if (c->cgb_mode)
                return c->wram_bank | 0xf8;
            return 0xff;

        case 0xff03:
        case 0xff08 ... 0xff0e:
        case 0xff15:
        case 0xff1f:
        case 0xff27 ... 0xff2f:
        case 0xff4e:
        case 0xff50:
        case 0xff56:
        case 0xff57 ... 0xff67:
        case 0xff71 ... 0xff7f:
            return 0xff;

        default:
            return c->memory[addr];
    }
}
