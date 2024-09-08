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
    /*
    memcpy(&savestate1.timer_save, &timer1, sizeof(timer));

    memcpy(&savestate1.cart.ram, &c->cart.ram, (sizeof(uint8_t) * 16 * 0x2000));

    memcpy(&savestate1.savestate_ppu, &video, sizeof(ppu));

    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "wb");
    fwrite(&savestate1, sizeof(savestate), 1, save);
    fclose(save);
     */
    char save_state_name[256];
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "wb");

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
    fwrite(&c->cart.ram, sizeof(uint8_t)*0x2000*c->cart.banks_ram, 1, save);
    fwrite(&timer1, sizeof(timer), 1, save);
    fwrite(&video, sizeof(ppu), 1, save);

    fclose(save);
}

void load_state(cpu *c, char rom_name[256]) {
    savestate savestate1;
    char save_state_name[256];
    int save_version;
    get_save_state_name(rom_name, save_state_name);
    FILE *save = fopen(save_state_name, "rb");
    if (save != NULL) {
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
        fread(&c->cart.ram, sizeof(uint8_t)*0x2000*c->cart.banks_ram, 1, save);
        fread(&timer1, sizeof(timer), 1, save);
        fread(&video, sizeof(ppu), 1, save);
        /*
        fread(&savestate1, sizeof(savestate), 1, save);
        fclose(save);

        FILE *version = fopen(save_state_name, "rb");
        fread(&save_version, sizeof(int), 1, version);
        fclose(version);

        if (save_version == 5) {
            c->is_halted = savestate1.is_halted;
            c->sp = savestate1.sp;
            c->pc = savestate1.pc;
            c->ime = savestate1.ime;
            c->ime_to_be_setted = savestate1.ime_to_be_setted;
            c->is_halted = savestate1.is_halted;
            c->first_halt = savestate1.first_halt;
            c->apu_div = savestate1.apu_div;
            c->wram_bank = savestate1.wram_bank;
            c->double_speed = savestate1.double_speed;
            c->gdma_halt = savestate1.gdma_halt;
            c->armed = savestate1.armed;
            memcpy(&c->r, &savestate1.r, sizeof(registers));
            memcpy(&c->memory, &savestate1.memory, (0x10000 * sizeof(uint8_t)));
            memcpy(&timer1, &savestate1.timer_save, sizeof(timer));
            memcpy(&c->wram, &savestate1.wram,(0x8000*sizeof(uint8_t)));
            memcpy(&c->hdma, &savestate1.hdma, sizeof(dma));

            c->cart.bank_select = savestate1.cart.bank_select;
            c->cart.bank_select_ram = savestate1.cart.bank_select_ram;
            c->cart.ram_enable = savestate1.cart.ram_enable;
            c->cart.mbc1mode = savestate1.cart.mbc1mode;
            memcpy(&c->cart.ram, &savestate1.cart.ram, (sizeof(uint8_t) * 16 * 0x2000));
            memcpy(&c->cart.rtc, &savestate1.cart.rtc, sizeof(rtc_clock));

            memcpy(&video, &savestate1.savestate_ppu, sizeof(ppu));

        }
        */
    }
}
