#include "../includes/cpu.h"
#include "../includes/cartridge.h"
#include "../includes/savestates.h"
#include "../includes/timer.h"
#include "../includes/ppu.h"
#include "../includes/memory.h"
#include <string.h>
#include <stdio.h>

const int savestate_version = 8;

void get_save_state_name(char rom_name[256], char save_state_name[256]) {
    strcpy(save_state_name, rom_name);

    #if defined(PLATFORM_WEB)
    char * skip = save_state_name + 14;
    char * save = "/saves/";
    memcpy(skip, save, 7);
    strcpy(save_state_name, skip);
    #endif

    char *ext = strrchr (save_state_name, '.');
    if (ext != NULL)
        *ext = '\0';
    char new_ext[] = ".s1";
    strcat(save_state_name, new_ext);
}

void load_registers(cpu *c, FILE *save) {
    uint8_t registers[128];
    for (int i = 0; i < 128; i++) {
        fread(&registers[i], 1, 1, save);
    }

    set_mem(c, JOYP, registers[JOYP-0xff00]);
    set_mem(c, SB, registers[SB-0xff00]);
    set_mem(c, SC, registers[SC-0xff00]);

    timer1.t_states = registers[DIV-0xff00] << 8;
    set_mem(c, TIMA, registers[TIMA-0xff00]);
    set_mem(c, TMA, registers[TMA-0xff00]);
    set_mem(c, TAC, registers[TAC-0xff00]);
    set_mem(c, IF, registers[IF-0xff00]);

    set_mem(c, NR52, registers[NR52-0xff00]);
    set_mem(c, NR51, registers[NR51-0xff00]);
    set_mem(c, NR50, registers[NR50-0xff00]);
    set_mem(c, NR10, registers[NR10-0xff00]);
    set_mem(c, NR11, registers[NR11-0xff00]);
    set_mem(c, NR12, registers[NR12-0xff00]);
    set_mem(c, NR13, registers[NR13-0xff00]);
    set_mem(c, NR14, registers[NR14-0xff00] & 0x7f);
    set_mem(c, NR21, registers[NR21-0xff00]);
    set_mem(c, NR22, registers[NR22-0xff00]);
    set_mem(c, NR23, registers[NR23-0xff00]);
    set_mem(c, NR24, registers[NR24-0xff00] & 0x7f);
    set_mem(c, NR30, registers[NR30-0xff00]);
    set_mem(c, NR31, registers[NR31-0xff00]);
    set_mem(c, NR32, registers[NR32-0xff00]);
    set_mem(c, NR33, registers[NR33-0xff00]);
    set_mem(c, NR34, registers[NR34-0xff00] & 0x7f);
    set_mem(c, NR41, registers[NR41-0xff00]);
    set_mem(c, NR42, registers[NR42-0xff00]);
    set_mem(c, NR43, registers[NR43-0xff00]);
    set_mem(c, NR44, registers[NR44-0xff00] & 0x7f);

    for (int i = 0; i < 0x10; i++) {
        c->memory[0xff30+i] = registers[0x30+i];
    }

    set_mem(c, LCDC, registers[LCDC-0xff00]);
    video.lyc_select = (registers[STAT-0xff00] >> 6) & 1;
    video.mode_select = (registers[STAT-0xff00] >> 3) & 7;
    set_mem(c, SCY, registers[SCY-0xff00]);
    set_mem(c, SCX, registers[SCX-0xff00]);
    video.scan_line = registers[LY-0xff00];
    set_mem(c, LYC, registers[LYC-0xff00]);
    set_mem(c, BGP, registers[BGP-0xff00]);
    set_mem(c, OBP0, registers[OBP0-0xff00]);
    set_mem(c, OBP1, registers[OBP1-0xff00]);
    set_mem(c, WY, registers[WY-0xff00]);
    set_mem(c, WX, registers[WX-0xff00]);
    c->memory[DMA] = registers[DMA-0xff00];

    if (c->is_color) {
        c->armed = registers[KEY1 - 0xff00] & 1;
        c->double_speed = registers[KEY1 - 0xff00] >> 7;
        set_mem(c, VBK, registers[VBK - 0xff00]);
        set_mem(c, HDMA1, registers[HDMA1 - 0xff00]);
        set_mem(c, HDMA2, registers[HDMA2 - 0xff00]);
        set_mem(c, HDMA3, registers[HDMA3 - 0xff00]);
        set_mem(c, HDMA4, registers[HDMA4 - 0xff00]);
        c->hdma.status = 0;
        c->hdma.lenght = 0;
        c->hdma.finished = true;
        c->gdma_halt = false;
        set_mem(c, BCPS, registers[BCPS - 0xff00]);
        set_mem(c, OCPS, registers[OCPS - 0xff00]);
        video.opri = (registers[KEY0 - 0xff00] == 0x04);
        set_mem(c, KEY0, registers[KEY0 - 0xff00]);
        set_mem(c, SVBK, registers[SVBK - 0xff00]);
    }
    c->bootrom.is_enabled = (registers[0x50] == 0);
    if (registers[0x50] == 0) {
        c->bootrom.is_enabled = true;
    }
    else {
        set_mem(c, 0xff50, 1);
    }
}

void load_wram(cpu *c, FILE *save, int64_t core_start) {
    fseek(save, core_start+0x98, SEEK_SET);
    int32_t size, addr;
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&c->wram[i >> 12][i & 0xfff], 1, 1, save);
    }
}

void load_vram(cpu *c, FILE *save, int64_t core_start) {
    fseek(save, core_start+0xa0, SEEK_SET);
    int32_t size, addr;
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&video.vram[i >> 13][i & 0x1fff], 1, 1, save);
    }
}

void load_hram(cpu *c, FILE *save, int64_t core_start) {
    fseek(save, core_start+0xb8, SEEK_SET);
    int32_t size, addr;
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&c->memory[0xff80+i], 1, 1, save);
    }
}

void load_xram(cpu *c, FILE *save, int64_t core_start) {
    fseek(save, core_start+0xa8, SEEK_SET);
    int32_t size, addr;
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&c->cart.ram[i >> 13][i & 0x1fff], 1, 1, save);
    }
}

void load_oam(cpu *c, FILE *save, int64_t core_start) {
    int32_t size, addr;
    fseek(save, core_start+0xb0, SEEK_SET);
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&c->memory[0xfe00 + i], 1, 1, save);
    }
}

void load_cram(cpu *c, FILE *save, int64_t core_start) {
    int32_t size, addr;
    fseek(save, core_start+0xc0, SEEK_SET);
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&video.bgp[i], 1, 1, save);
    }
    fseek(save, core_start+0xc8, SEEK_SET);
    fread(&size, 4, 1, save);
    fread(&addr, 4, 1, save);
    fseek(save, addr, SEEK_SET);
    for (int i = 0; i < size; i++) {
        fread(&video.obp[i], 1, 1, save);
    }
}

void save_registers(cpu *c, FILE *save) {
    uint8_t registers[128];
    registers[JOYP-0xff00] = get_mem(c, JOYP);
    registers[SB-0xff00] = get_mem(c, SB);
    registers[SC-0xff00] = get_mem(c, SC);
    registers[DIV-0xff00] = get_mem(c, DIV);
    registers[TIMA-0xff00] = get_mem(c, TIMA);
    registers[TMA-0xff00] = get_mem(c, TMA);
    registers[TAC-0xff00] = get_mem(c, TAC);
    registers[IF-0xff00] = get_mem(c, IF);

    registers[NR52-0xff00] = get_mem(c, NR52);
    registers[NR51-0xff00] = get_mem(c, NR51);
    registers[NR50-0xff00] = get_mem(c, NR50);
    for (int i = 0; i < 0x10; i++) {
        registers[0x30+i] = c->memory[0xff30+i];
    }

    registers[LCDC-0xff00] = get_mem(c, LCDC);
    registers[STAT-0xff00] = get_mem(c, STAT);
    registers[SCY-0xff00] = get_mem(c, SCY);
    registers[SCX-0xff00] = get_mem(c, SCX);
    registers[LY-0xff00] = video.scan_line;
    registers[LYC-0xff00] = get_mem(c, LYC);
    registers[DMA-0xff00] = get_mem(c, DMA);
    registers[BGP-0xff00] = get_mem(c, BGP);
    registers[OBP0-0xff00] = get_mem(c, OBP0);
    registers[OBP1-0xff00] = get_mem(c, OBP1);
    registers[WY-0xff00] = get_mem(c, WY);
    registers[WX-0xff00] = get_mem(c, WX);
    registers[0x50] = c->bootrom.is_enabled ? 0 : 1;
    if (c->is_color) {
        registers[KEY0-0xff00] = get_mem(c, KEY0);
        registers[KEY1-0xff00] = get_mem(c, KEY1);
        registers[VBK-0xff00] = get_mem(c, VBK);
        registers[HDMA1-0xff00] = c->hdma.source >> 8;
        registers[HDMA2-0xff00] = c->hdma.source & 255;
        registers[HDMA3-0xff00] = c->hdma.destination >> 8;
        registers[HDMA4-0xff00] = c->hdma.destination & 255;
        registers[HDMA5-0xff00] = 0;
        registers[BCPS-0xff00] = get_mem(c, BCPS);
        registers[OCPS-0xff00] = get_mem(c, OCPS);
        registers[SVBK-0xff00] = get_mem(c, SVBK);
    }
    for (int i = 0; i < 128; i++) {
        fwrite(&registers[i], 1, 1, save);
    }
}

uint32_t save_wram(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    uint32_t size = c->is_color ? 0x8000 : 0x2000;
    for (int i = 0; i < size; i++) {
        fwrite(&c->wram[i >> 12][i & 0xfff] , 1, 1, save);
    }
    return addr;
}

uint32_t save_vram(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    uint32_t size = c->is_color ? 0x4000 : 0x2000;
    for (int i = 0; i < size; i++) {
        fwrite(&video.vram[i >> 13][i & 0x1fff] , 1, 1, save);
    }
    return addr;
}

uint32_t save_xram(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    uint32_t sizes[] = {0, 0, 0x2000, 0x8000, 0x20000,0x10000};
    uint32_t size = sizes[c->cart.banks_ram];
    if (c->cart.mbc == MBC2) {
        size = 0x200;
    }
    else if (c->cart.mbc == MBC7) {
        size = 0x100;
    }
    for (int i = 0; i < size; i++) {
        fwrite(&c->cart.ram[i >> 13][i & 0x1fff] , 1, 1, save);
    }
    return addr;
}

uint32_t save_oam(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    for (int i = 0; i < 0xa0; i++) {
        fwrite(&c->memory[0xfe00 + i] , 1, 1, save);
    }
    return addr;
}

uint32_t save_hram(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    for (int i = 0; i < 0x7f; i++) {
        fwrite(&c->memory[0xff80 + i] , 1, 1, save);
    }
    return addr;
}

uint32_t save_cram(cpu *c, FILE *save) {
    uint32_t addr = ftell(save);
    if (c->is_color) {
        for (int i = 0; i < 0x40; i++) {
            fwrite(&video.bgp[i] , 1, 1, save);
        }
        for (int i = 0; i < 0x40; i++) {
            fwrite(&video.obp[i] , 1, 1, save);
        }
        return addr;
    }
    return 0;
}

void save_mbc_data(cpu *c, FILE *save) {
    uint32_t block_size;
    uint16_t cart_addr;
    uint8_t value;
    switch (c->cart.mbc) {
        case NO_MBC:
            break;
        case MBC1:
            fwrite("MBC ", 4, 1, save);
            block_size = 9;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x2000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select & 31;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x6000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.mbc1mode;
            fwrite(&value, 1, 1, save);
            break;
        case MBC2:
            fwrite("MBC ", 4, 1, save);
            block_size = 6;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x0100;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select;
            fwrite(&value, 1, 1, save);
            break;
        case MBC3:
            fwrite("MBC ", 4, 1, save);
            block_size = 9;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x2000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x4000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select_ram;
            fwrite(&value, 1, 1, save);
            break;
        case MBC5:
            fwrite("MBC ", 4, 1, save);
            block_size = 12;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x2000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select & 0xff;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x3000;
            fwrite(&cart_addr, 2, 1, save);
            value = ((c->cart.bank_select & 0xff) == 0) ? 0 : 1;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x4000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select_ram;
            fwrite(&value, 1, 1, save);
            break;
        case MBC7:
            fwrite("MBC ", 4, 1, save);
            block_size = 12;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x2000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x4000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable2 ? 0x40 : 0;
            fwrite(&value, 1, 1, save);
            break;
        case POCKET_CAMERA:
            fwrite("MBC ", 4, 1, save);
            block_size = 9;
            fwrite(&block_size, 4, 1, save);

            cart_addr = 0x0000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.ram_enable ? 0xa : 0;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x2000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select;
            fwrite(&value, 1, 1, save);

            cart_addr = 0x4000;
            fwrite(&cart_addr, 2, 1, save);
            value = c->cart.bank_select_ram;
            fwrite(&value, 1, 1, save);
            break;
    }
}

void save_mbc7_data(cpu *c, FILE *save) {
    fwrite("MBC7", 4, 1, save);
    uint32_t block_size = 0xa;
    fwrite(&block_size, 4, 1, save);
    uint8_t value = ((~c->cart.accel.has_latched) & 1) |
            (c->cart.eeprom.DO << 1) |
            (c->cart.eeprom.DI << 2) |
            (c->cart.eeprom.CLK << 3) |
            (c->cart.eeprom.CS << 4) |
            (c->cart.eeprom.write_enable << 5);
    fwrite(&value, 1, 1, save);
    fwrite(&c->cart.eeprom.argument_bits_left, 1, 1, save);
    fwrite(&c->cart.eeprom.command, 2, 1, save);
    fwrite(&c->cart.eeprom.read_bits, 2, 1, save);
    fwrite(&c->cart.accel.x_latched, 2, 1, save);
    fwrite(&c->cart.accel.y_latched, 2, 1, save);
}

void save_rtc_data(cpu *c, FILE *save) {
    fwrite("RTC ", 4, 1, save);
    uint32_t block_size = 0x30;
    fwrite(&block_size, 4, 1, save);
    uint32_t s = c->cart.rtc.time % 60;
    uint32_t m = (c->cart.rtc.time / 60) % 60;
    uint32_t h = (c->cart.rtc.time / 3600) % 24;
    uint32_t dl = (c->cart.rtc.time / 86400) & 0xff;
    uint32_t dh = ((c->cart.rtc.time / 86400) >> 8) & 1;
    uint32_t dl_latch = c->cart.rtc.days & 0xff;
    uint32_t dh_latch = (c->cart.rtc.days >> 8) & 1;
    uint64_t time_saved = (uint64_t)(time(NULL));
    fwrite(&s, 4, 1, save);
    fwrite(&m, 4, 1, save);
    fwrite(&h, 4, 1, save);
    fwrite(&dl, 4, 1, save);
    fwrite(&dh, 4, 1, save);
    fwrite(&c->cart.rtc.seconds, 4, 1, save);
    fwrite(&c->cart.rtc.minutes, 4, 1, save);
    fwrite(&c->cart.rtc.hours, 4, 1, save);
    fwrite(&dl_latch, 4, 1, save);
    fwrite(&dh_latch, 4, 1, save);
    fwrite(&time_saved, 8, 1, save);
}

void save_state(cpu *c, char rom_name[256]) {
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "wb");
    if (save != NULL) {
        uint8_t value_8;
        uint16_t value_16;
        uint32_t value_32;

        uint32_t wram_start = save_wram(c, save);
        uint32_t vram_start = save_vram(c, save);
        uint32_t xram_start = save_xram(c, save);
        uint32_t oam_start = save_oam(c, save);
        uint32_t hram_start = save_hram(c, save);
        uint32_t cram_start = save_cram(c, save);

        uint32_t bess_start = ftell(save);
        fwrite("NAME", 4, 1, save);
        value_32 = 15;
        fwrite(&value_32, 4, 1, save);
        fwrite("ChillyGB v0.3.0", 15, 1, save);
        fwrite("INFO", 4, 1, save);
        value_32 = 0x12;
        fwrite(&value_32, 4, 1, save);
        fwrite(&c->cart.data[0][0x134], 16, 1, save);
        fwrite(&c->cart.data[0][0x14e], 2, 1, save);

        value_32 = 0xd0;
        fwrite("CORE", 4, 1, save);
        fwrite(&value_32, 4, 1, save);
        value_16 = 1;
        fwrite(&value_16, 2, 1, save);
        fwrite(&value_16, 2, 1, save);
        if (c->is_color) {
            fwrite("C   ", 4, 1, save);
        }
        else {
            fwrite("G   ", 4, 1, save);
        }
        fwrite(&c->pc, 2, 1, save);
        fwrite(&c->r.reg16[AF], 2, 1, save);
        fwrite(&c->r.reg16[BC], 2, 1, save);
        fwrite(&c->r.reg16[DE], 2, 1, save);
        fwrite(&c->r.reg16[HL], 2, 1, save);
        fwrite(&c->sp, 2, 1, save);
        fwrite(&c->ime, 1, 1, save);
        fwrite(&c->memory[IE], 1, 1, save);
        fwrite(&c->is_halted, 1, 1, save);
        value_8 = 0;
        fwrite(&value_8, 1, 1, save);
        save_registers(c, save);

        value_32 = c->cgb_mode ? 0x8000 : 0x2000;
        fwrite(&value_32, 4, 1, save);
        fwrite(&wram_start, 4, 1, save);

        value_32 = c->cgb_mode ? 0x4000 : 0x2000;
        fwrite(&value_32, 4, 1, save);
        fwrite(&vram_start, 4, 1, save);

        uint32_t sizes[] = {0, 0, 0x2000, 0x8000, 0x20000,0x10000};
        value_32 = sizes[c->cart.banks_ram];
        fwrite(&value_32, 4, 1, save);
        fwrite(&xram_start, 4, 1, save);

        value_32 = 0xa0;
        fwrite(&value_32, 4, 1, save);
        fwrite(&oam_start, 4, 1, save);

        value_32 = 0x7f;
        fwrite(&value_32, 4, 1, save);
        fwrite(&hram_start, 4, 1, save);

        value_32 = c->is_color ? 0x40 : 0;
        fwrite(&value_32, 4, 1, save);
        fwrite(&cram_start, 4, 1, save);
        cram_start += 64;
        fwrite(&value_32, 4, 1, save);
        fwrite(&cram_start, 4, 1, save);

        save_mbc_data(c, save);
        if (c->cart.mbc == MBC7) {
            save_mbc7_data(c, save);
        }
        if ((c->cart.mbc == MBC3) && c->cart.has_rtc) {
            save_rtc_data(c, save);
        }

        fwrite("END ", 4, 1, save);
        value_32 = 0;
        fwrite(&value_32, 4, 1, save);

        fwrite(&bess_start, 4, 1, save);
        fwrite("BESS", 4, 1, save);
        fclose(save);
    }
}

int load_state(cpu *c, char rom_name[256]) {
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);

    FILE *save = fopen(save_state_name, "rb");
    if (save != NULL) {
        fseek(save, -8, SEEK_END);
        uint32_t first_block_addr;
        char bess_string[5];
        fread(&first_block_addr, 4, 1, save);
        fread(&bess_string, 4, 1, save);

        if (strcmp(bess_string, "BESS") != 0) {
            fclose(save);
            return NOT_BESS_FORMAT;
        }

        fseek(save, first_block_addr, SEEK_SET);

        char current_block[5] = "";
        uint32_t block_lenght;

        fread(&current_block, 4, 1, save);
        fread(&block_lenght, 4, 1, save);
        if (strcmp(current_block, "NAME") == 0) {
            fseek(save, block_lenght, SEEK_CUR);

            fread(&current_block, 4, 1, save);
            fread(&block_lenght, 4, 1, save);
            if (strcmp(current_block, "INFO") == 0) {
                fseek(save, block_lenght, SEEK_CUR);
                fread(&current_block, 4, 1, save);
                fread(&block_lenght, 4, 1, save);
            }
        }

        if (strcmp(current_block, "CORE") == 0) {
            int64_t core_start = ftell(save);
            fseek(save, 4, SEEK_CUR);
            char model_number[5];
            fread(&model_number, 4, 1, save);
            if (((model_number[0] == 'G') && !c->is_color) || ((model_number[0] == 'C') && c->is_color)) {
                fread(&c->pc, 2, 1, save);
                fread(&c->r.reg16[AF], 2, 1, save);
                fread(&c->r.reg16[BC], 2, 1, save);
                fread(&c->r.reg16[DE], 2, 1, save);
                fread(&c->r.reg16[HL], 2, 1, save);
                fread(&c->sp, 2, 1, save);
                fread(&c->ime, 1, 1, save);
                fread(&c->memory[IE], 1, 1, save);
                uint8_t exec_state;
                fread(&exec_state, 1, 1, save);
                if (exec_state == 0)
                    c->is_halted = false;
                else
                    c->is_halted = true;
                fseek(save, 1, SEEK_CUR);
                load_registers(c, save);
                load_wram(c, save, core_start);
                load_vram(c, save, core_start);
                load_xram(c, save, core_start);
                load_oam(c, save, core_start);
                load_hram(c, save, core_start);
                load_cram(c, save, core_start);

                // Read other blocks;
                fseek(save, core_start + 0xd0, SEEK_SET);
                do {
                    fread(&current_block, 4, 1, save);
                    fread(&block_lenght, 4, 1, save);
                    if (strcmp(current_block, "XOAM") == 0) {
                        fseek(save, block_lenght, SEEK_CUR);
                    }
                    else if (strcmp(current_block, "MBC ") == 0) {
                        if (block_lenght % 3 == 0) {
                            for (int i = 0; i < block_lenght/3; i++) {
                                uint16_t addr;
                                uint8_t value;
                                fread(&addr, 2, 1, save);
                                fread(&value, 1, 1, save);
                                set_mem(c, addr, value);
                            }
                        }
                        else {
                            fclose(save);
                            return INVALID_MBC_BLOCK_SIZE;
                        }
                    }
                    else if (strcmp(current_block, "RTC ") == 0) {
                        uint8_t seconds, minutes, hours, days_hi, days_lo;
                        uint16_t days;
                        uint8_t latched_days_hi, latched_days_lo;
                        uint64_t time_saved;
                        fread(&seconds, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&minutes, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&hours, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&days_lo, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&days_hi, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&c->cart.rtc.seconds, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&c->cart.rtc.minutes, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&c->cart.rtc.hours, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&latched_days_lo, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&latched_days_hi, 1, 1, save);
                        fseek(save, 3, SEEK_CUR);
                        fread(&time_saved, 8, 1, save);
                        days = days_lo & ((days_hi & 1) << 8);
                        c->cart.rtc.days = latched_days_lo & ((latched_days_hi & 1) << 8);
                        c->cart.rtc.is_halted = (latched_days_hi >> 6) & 1;
                        c->cart.rtc.day_carry = (latched_days_hi >> 7) & 1;
                        c->cart.rtc.time = seconds + (minutes * 60) + (hours * 3600) + (days * 86400);
                        if (!set.accurate_rtc) {
                            uint64_t current_time = (uint64_t)(time(NULL));
                            c->cart.rtc.time += current_time - time_saved;
                        }
                    }
                    else if (strcmp(current_block, "MBC7") == 0) {
                        uint8_t flags;
                        fread(&flags, 1, 1, save);
                        c->cart.accel.has_latched = (~flags) & 1;
                        c->cart.eeprom.DO = (flags >> 1) & 1;
                        c->cart.eeprom.DI = (flags >> 2) & 1;
                        c->cart.eeprom.CLK = (flags >> 3) & 1;
                        c->cart.eeprom.CS = (flags >> 4) & 1;
                        c->cart.eeprom.write_enable = (flags >> 5) & 1;
                        fread(&c->cart.eeprom.argument_bits_left, 1, 1, save);
                        fread(&c->cart.eeprom.command, 2, 1, save);
                        fread(&c->cart.eeprom.read_bits, 2, 1, save);
                        fread(&c->cart.accel.x_latched, 2, 1, save);
                        fread(&c->cart.accel.y_latched, 2, 1, save);
                    }
                    else if (strcmp(current_block, "END ") != 0) {
                        break;
                    }
                } while (strcmp(current_block, "END ") != 0);

                fclose(save);
                return NO_ERROR;
            }
            else {
                fclose(save);
                return INVALID_MODEL;
            }
        }
        else {
            fclose(save);
            return UNKNOWN_BLOCK_BEFORE_CORE;
        }
        fclose(save);
    }
    return NO_ERROR;
}
