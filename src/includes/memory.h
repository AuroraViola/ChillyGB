#include "cpu.h"

#ifndef CHILLYGB_MEMORY_H
#define CHILLYGB_MEMORY_H

uint8_t get_mem(cpu *c, uint16_t addr);
void set_mem(cpu *c, uint16_t addr, uint8_t value);

#endif //CHILLYGB_MEMORY_H
