#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_DEBUG_H
#define CHILLYGB_DEBUG_H

extern char *debug_text;

void decode_instructions(cpu *c, char instruction[20][50]);
Image take_debug_screenshot(Color pixels[144][160]);
void export_screenshot(Image screenshot, char rom_name[256]);
void test_rom(cpu *c, int n_ticks);

#endif //CHILLYGB_DEBUG_H
