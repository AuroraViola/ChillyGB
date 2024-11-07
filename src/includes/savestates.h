#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "ppu.h"
#include "timer.h"

#ifndef CHILLYGB_SAVESTATES_H
#define CHILLYGB_SAVESTATES_H

enum savestate_load_errors{
    NO_ERROR = 0,
    INVALID_MODEL,
    UNKNOWN_BLOCK_BEFORE_CORE,
    INVALID_MBC_BLOCK_SIZE,
    NOT_BESS_FORMAT,
};

void save_state(cpu *c, char rom_name[256]);
int load_state(cpu *c, char rom_name[256]);


#endif //CHILLYGB_SAVESTATES_H
