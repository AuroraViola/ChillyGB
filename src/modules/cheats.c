#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../includes/cheats.h"

Cheats cheats;

void remove_gameshark_cheat(uint8_t cheat_i) {
    for (int i = cheat_i; i < cheats.gameShark_count-1; i++) {
        cheats.gameShark[i] = cheats.gameShark[i+1];
    }
    cheats.gameShark_count--;
}

void add_gameshark_cheat() {
    if (cheats.gameShark_count < 50) {
        uint32_t cheat_code = strtol(cheats.gameshark_code, NULL, 16);
        cheats.gameShark[cheats.gameShark_count].sram_bank = cheat_code >> 24;
        cheats.gameShark[cheats.gameShark_count].new_data = (cheat_code >> 16) & 0xff;
        cheats.gameShark[cheats.gameShark_count].address = ((cheat_code & 0xff00) >> 8) | ((cheat_code & 0xff) << 8);
        strcpy(cheats.gameshark_code, "");
        cheats.gameShark_count++;
    }
}

void remove_gamegenie_cheat(uint8_t cheat_i) {
    for (int i = cheat_i; i < cheats.gameGenie_count-1; i++) {
        cheats.gameGenie[i] = cheats.gameGenie[i+1];
    }
    cheats.gameGenie_count--;
}

void add_gamegenie_cheat() {
    if (cheats.gameGenie_count < 50) {
        uint64_t cheat_code = strtol(cheats.gamegenie_code, NULL, 16);
        cheats.gameGenie[cheats.gameGenie_count].new_data = cheat_code >> 28;
        cheats.gameGenie[cheats.gameGenie_count].address = ((cheat_code & 0xf000) | ((cheat_code >> 16) & 0xfff)) ^ 0xf000;
        cheats.gameGenie[cheats.gameGenie_count].old_data = ((cheat_code >> 4) & 0xf0) | (cheat_code & 0xf);
        uint8_t temp = (cheats.gameGenie[cheats.gameGenie_count].old_data & 0x7) << 6;
        cheats.gameGenie[cheats.gameGenie_count].old_data >>= 2;
        cheats.gameGenie[cheats.gameGenie_count].old_data |= temp;
        cheats.gameGenie[cheats.gameGenie_count].old_data ^= 0xba;
        strcpy(cheats.gamegenie_code, "");
        cheats.gameGenie_count++;
    }
}

void get_cheat_file(char rom_name[256], char cheats_name[256]) {
    strcpy(cheats_name, rom_name);

    #if defined(PLATFORM_WEB)
        char * skip = cheats_name + 14;
        char * save = "/saves/";
        memcpy(skip, save, 7);
        strcpy(cheats_name, skip);
    #endif

    char *ext = strrchr (cheats_name, '.');
    if (ext != NULL)
        *ext = '\0';
    char new_ext[] = ".cheats";
    strcat(cheats_name, new_ext);
}

void load_cheats(char rom_name[256]) {
    char cheats_name[256];
    get_cheat_file(rom_name, cheats_name);
    FILE *cheats_file = fopen(cheats_name, "rb");
    if (cheats_file != NULL) {
        fread(&cheats.gameGenie_count, 1, 1, cheats_file);
        for (int i = 0; i < cheats.gameGenie_count; i++) {
            fread(&cheats.gameGenie[i].new_data, 1, 1, cheats_file);
            fread(&cheats.gameGenie[i].address, 2, 1, cheats_file);
            fread(&cheats.gameGenie[i].old_data, 1, 1, cheats_file);
        }
        fread(&cheats.gameShark_count, 1, 1, cheats_file);
        for (int i = 0; i < cheats.gameShark_count; i++) {
            fread(&cheats.gameShark[i].sram_bank, 1, 1, cheats_file);
            fread(&cheats.gameShark[i].new_data, 1, 1, cheats_file);
            fread(&cheats.gameShark[i].address, 2, 1, cheats_file);
        }
        fclose(cheats_file);
    }
    else {
        cheats.gameShark_count = 0;
        cheats.gameGenie_count = 0;
    }
}

void save_cheats(char rom_name[256]) {
    char cheats_name[256];
    get_cheat_file(rom_name, cheats_name);
    FILE *cheats_file = fopen(cheats_name, "wb");
    if (cheats_file != NULL) {
        fwrite(&cheats.gameGenie_count, 1, 1, cheats_file);
        for (int i = 0; i < cheats.gameGenie_count; i++) {
            fwrite(&cheats.gameGenie[i].new_data, 1, 1, cheats_file);
            fwrite(&cheats.gameGenie[i].address, 2, 1, cheats_file);
            fwrite(&cheats.gameGenie[i].old_data, 1, 1, cheats_file);
        }
        fwrite(&cheats.gameShark_count, 1, 1, cheats_file);
        for (int i = 0; i < cheats.gameShark_count; i++) {
            fwrite(&cheats.gameShark[i].sram_bank, 1, 1, cheats_file);
            fwrite(&cheats.gameShark[i].new_data, 1, 1, cheats_file);
            fwrite(&cheats.gameShark[i].address, 2, 1, cheats_file);
        }
        fclose(cheats_file);
    }
}

