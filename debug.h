#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_DEBUG_H
#define CHILLYGB_DEBUG_H

typedef struct {
    char AFtext[20];
    char BCtext[20];
    char DEtext[20];
    char HLtext[20];
    char SPtext[20];
    char PCtext[20];
    char IMEtext[20];

    char PPUMode[20];
    char LYtext[20];
    char LYCtext[20];
    char LCDCtext[20];
    char STATtext[20];

    char IEtext[20];
    char IFtext[20];

    char BANKtext[20];
    char RAMBANKtext[20];
    char RAMENtext[20];
}debugtexts;

void generate_texts(cpu *c, debugtexts *texts);
void decode_instructions(cpu *c, char instruction[20][50]);

#endif //CHILLYGB_DEBUG_H
