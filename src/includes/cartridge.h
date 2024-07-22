#include "cpu.h"

#ifndef CHILLYGB_CARTRIDGE_H
#define CHILLYGB_CARTRIDGE_H

void load_game(cartridge *cart, char rom_name[256]);
void save_game(cartridge *cart, char rom_name[256]);

#endif //CHILLYGB_CARTRIDGE_H
