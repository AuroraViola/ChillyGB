#include "../includes/cpu.h"
#include "../includes/savestates.h"
#include "../includes/timer.h"
#include "../includes/ppu.h"
#include <string.h>
#include <stdio.h>

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
    char new_ext[] = ".ss1";
    strcat(save_state_name, new_ext);
}

void save_state(cpu *c, char rom_name[256]) {
    savestate savestate1;
    savestate1.is_halted = c->is_halted;
    savestate1.sp = c->sp;
    savestate1.pc = c->pc;
    savestate1.ime = c->ime;
    savestate1.ime_to_be_setted = c->ime_to_be_setted;
    savestate1.is_halted = c->is_halted;
    savestate1.first_halt = c->first_halt;
    savestate1.apu_div = c->apu_div;
    memcpy(&savestate1.r, &c->r, sizeof(registers));
    memcpy(&savestate1.memory, &c->memory, (0x10000*sizeof(uint8_t)));
    memcpy(&savestate1.timer_save, &timer1, sizeof(timer));

    savestate1.cart.bank_select = c->cart.bank_select;
    savestate1.cart.bank_select_ram = c->cart.bank_select_ram;
    savestate1.cart.ram_enable = c->cart.ram_enable;
    savestate1.cart.mbc1mode = c->cart.mbc1mode;
    memcpy(&savestate1.cart.rtc, &c->cart.rtc, sizeof(rtc_clock));
    memcpy(&savestate1.cart.ram, &c->cart.ram, (sizeof(uint8_t) * 16 * 0x2000));

    savestate1.ppu.mode = video.mode;
    savestate1.ppu.scanline = video.scan_line;
    savestate1.ppu.is_on = video.is_on;
    savestate1.ppu.bg_enable = video.bg_enable;
    savestate1.ppu.window_enable = video.window_enable;
    savestate1.ppu.obj_enable = video.obj_enable;
    savestate1.ppu.bg_tiles = video.bg_tiles;
    savestate1.ppu.bg_tilemap = video.bg_tilemap;
    savestate1.ppu.window_tilemap = video.window_tilemap;
    savestate1.ppu.obj_size = video.obj_size;
    savestate1.ppu.lyc_select = video.lyc_select;
    savestate1.ppu.mode_select = video.mode_select;
    savestate1.ppu.ly_eq_lyc = video.ly_eq_lyc;
    memcpy(&savestate1.ppu.bgp, &video.bgp, (sizeof(uint8_t) * 4));
    memcpy(&savestate1.ppu.obp, &video.obp, (sizeof(uint8_t) * 8));

    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "wb");
    fwrite(&savestate1, sizeof(savestate), 1, save);
    fclose(save);
}

void load_state(cpu *c, char rom_name[256]) {
    savestate savestate1;
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "rb");
    if (save != NULL) {
        fread(&savestate1, sizeof(savestate), 1, save);
        fclose(save);

        c->is_halted = savestate1.is_halted;
        c->sp = savestate1.sp;
        c->pc = savestate1.pc;
        c->ime = savestate1.ime;
        c->ime_to_be_setted = savestate1.ime_to_be_setted;
        c->is_halted = savestate1.is_halted;
        c->first_halt = savestate1.first_halt;
        c->apu_div = savestate1.apu_div;
        memcpy(&c->r, &savestate1.r, sizeof(registers));
        memcpy(&c->memory, &savestate1.memory, (0x10000 * sizeof(uint8_t)));
        memcpy(&timer1, &savestate1.timer_save, sizeof(timer));

        c->cart.bank_select = savestate1.cart.bank_select;
        c->cart.bank_select_ram = savestate1.cart.bank_select_ram;
        c->cart.ram_enable = savestate1.cart.ram_enable;
        c->cart.mbc1mode = savestate1.cart.mbc1mode;
        memcpy(&c->cart.ram, &savestate1.cart.ram, (sizeof(uint8_t) * 16 * 0x2000));
        memcpy(&c->cart.rtc, &savestate1.cart.rtc, sizeof(rtc_clock));

        video.mode = savestate1.ppu.mode;
        video.scan_line = savestate1.ppu.scanline;
        video.is_on = savestate1.ppu.is_on;
        video.bg_enable = savestate1.ppu.bg_enable;
        video.window_enable = savestate1.ppu.window_enable;
        video.obj_enable = savestate1.ppu.obj_enable;
        video.bg_tiles = savestate1.ppu.bg_tiles;
        video.bg_tilemap = savestate1.ppu.bg_tilemap;
        video.window_tilemap = savestate1.ppu.window_tilemap;
        video.obj_size = savestate1.ppu.obj_size;
        video.lyc_select = savestate1.ppu.lyc_select;
        video.mode_select = savestate1.ppu.mode_select;
        video.ly_eq_lyc = savestate1.ppu.ly_eq_lyc;

        video.need_sprites_reload = true;
        video.need_bg_wn_reload = true;
        video.tiles_write = true;
        video.tilemap_write = true;
        video.tilemap_write = true;
        c->bootrom.is_enabled = false;
        memcpy(&video.bgp, &savestate1.ppu.bgp, (sizeof(uint8_t) * 4));
        memcpy(&video.obp, &savestate1.ppu.obp, (sizeof(uint8_t) * 8));
    }
}
