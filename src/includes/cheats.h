#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_CHEATS_H
#define CHILLYGB_CHEATS_H

typedef struct {
    bool enabled;
    uint8_t new_data;
    uint16_t address;
    uint8_t old_data;
}GameGenie;

typedef struct {
    bool enabled;
    uint8_t sram_bank;
    uint8_t new_data;
    uint16_t address;
}GameShark;

typedef struct {
    uint8_t gameGenie_count;
    uint8_t gameShark_count;
    GameShark gameShark[50];
    GameGenie gameGenie[50];
    char gamegenie_code[11];
    char gameshark_code[10];
}Cheats;

extern Cheats cheats;
void load_cheats(char rom_name[256]);
void save_cheats(char rom_name[256]);

void add_gamegenie_cheat();
void remove_gamegenie_cheat(uint8_t cheat_i);

void add_gameshark_cheat();
void remove_gameshark_cheat(uint8_t cheat_i);

#endif //CHILLYGB_CHEATS_H
