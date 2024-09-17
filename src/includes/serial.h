#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_SERIAL_H
#define CHILLYGB_SERIAL_H

typedef struct {
    uint8_t value;
    bool is_master;
    bool is_transfering;
    bool clock_speed;
    uint8_t shifted_bits;
}serial;

extern serial serial1;

void transfer_bit();

#endif //CHILLYGB_SERIAL_H
