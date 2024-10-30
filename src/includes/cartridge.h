#include "cpu.h"

#ifndef CHILLYGB_CARTRIDGE_H
#define CHILLYGB_CARTRIDGE_H

bool load_game(cartridge *cart, char rom_name[256]);
void save_game(cartridge *cart, char rom_name[256]);

enum mbcs {
    NO_MBC = 0,
    MBC1,
    MBC2,
    MBC3,
    MBC5,
    MBC6,
    MBC7,
    POCKET_CAMERA,
    MMM01,
    TAMA5,
    HuC3,
    HuC1,
};

#endif //CHILLYGB_CARTRIDGE_H
