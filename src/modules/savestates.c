#include "../includes/cpu.h"
#include "../includes/savestates.h"
#include "../includes/timer.h"
#include "../includes/ppu.h"
#include <string.h>
#include <stdio.h>

const int savestate_version = 6;

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
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "wb");

    fwrite(&savestate_version, sizeof(int), 1, save);
    fwrite(&c->r, sizeof(registers), 1, save);
    fwrite(&c->pc, sizeof(uint16_t), 1, save);
    fwrite(&c->sp, sizeof(uint16_t), 1, save);
    fwrite(&c->ime, sizeof(bool), 1, save);
    fwrite(&c->ime_to_be_setted, sizeof(uint8_t), 1, save);
    fwrite(&c->is_halted, sizeof(bool), 1, save);
    fwrite(&c->is_color, sizeof(bool), 1, save);
    fwrite(&c->gdma_halt, sizeof(bool), 1, save);
    fwrite(&c->double_speed, sizeof(bool), 1, save);
    fwrite(&c->hdma, sizeof(dma), 1, save);
    fwrite(&c->armed, sizeof(bool), 1, save);
    fwrite(&c->apu_div, sizeof(uint8_t), 1, save);
    fwrite(&c->wram, (c->is_color ? 0x8000 : 0x2000)*sizeof(uint8_t), 1, save);
    fwrite(&c->wram_bank, sizeof(uint8_t), 1, save);
    fwrite(&c->memory[0xfe00], 0x200*sizeof(uint8_t), 1, save);

    fwrite(&c->cart.rtc, sizeof(rtc_clock), 1, save);
    fwrite(&c->cart.bank_select, sizeof(uint16_t), 1, save);
    fwrite(&c->cart.mbc1mode, sizeof(bool), 1, save);
    fwrite(&c->cart.ram_enable, sizeof(bool), 1, save);
    fwrite(&c->cart.bank_select_ram, sizeof(uint8_t), 1, save);

    if (c->cart.type == 5 || c->cart.type == 6) {
        fwrite(&c->cart.ram, sizeof(uint8_t) * 0x200, 1, save);
    }
    else {
        fwrite(&c->cart.ram, sizeof(uint8_t)*0x2000*c->cart.banks_ram, 1, save);
    }
    fwrite(&timer1, sizeof(timer), 1, save);
    fwrite(&video, sizeof(ppu), 1, save);

    fclose(save);
}

void load_state(cpu *c, char rom_name[256]) {
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "rb");
    if (save != NULL) {
        int version;
        fread(&version, sizeof(int), 1, save);
        if (version == savestate_version) {
            fread(&c->r, sizeof(registers), 1, save);
            fread(&c->pc, sizeof(uint16_t), 1, save);
            fread(&c->sp, sizeof(uint16_t), 1, save);
            fread(&c->ime, sizeof(bool), 1, save);
            fread(&c->ime_to_be_setted, sizeof(uint8_t), 1, save);
            fread(&c->is_halted, sizeof(bool), 1, save);
            fread(&c->is_color, sizeof(bool), 1, save);
            fread(&c->gdma_halt, sizeof(bool), 1, save);
            fread(&c->double_speed, sizeof(bool), 1, save);
            fread(&c->hdma, sizeof(dma), 1, save);
            fread(&c->armed, sizeof(bool), 1, save);
            fread(&c->apu_div, sizeof(uint8_t), 1, save);
            fread(&c->wram, (c->is_color ? 0x8000 : 0x2000)*sizeof(uint8_t), 1, save);
            fread(&c->wram_bank, sizeof(uint8_t), 1, save);
            fread(&c->memory[0xfe00], 0x200*sizeof(uint8_t), 1, save);

            fread(&c->cart.rtc, sizeof(rtc_clock), 1, save);
            fread(&c->cart.bank_select, sizeof(uint16_t), 1, save);
            fread(&c->cart.mbc1mode, sizeof(bool), 1, save);
            fread(&c->cart.ram_enable, sizeof(bool), 1, save);
            fread(&c->cart.bank_select_ram, sizeof(uint8_t), 1, save);
            if (c->cart.type == 5 || c->cart.type == 6) {
                fread(&c->cart.ram, sizeof(uint8_t) * 0x200, 1, save);
            }
            else {
                fread(&c->cart.ram, sizeof(uint8_t) * 0x2000 * c->cart.banks_ram, 1, save);
            }
            fread(&timer1, sizeof(timer), 1, save);
            fread(&video, sizeof(ppu), 1, save);
        }
        fclose(save);
    }
}
