#include <stdbool.h>

#ifndef CHILLYGB_INPUT_H
#define CHILLYGB_INPUT_H

typedef struct {
    uint8_t dpad[4];
    uint8_t buttons[4];
}joypad;

enum dpad {
    DPAD_RIGHT = 0,
    DPAD_LEFT = 1,
    DPAD_UP = 2,
    DPAD_DOWN = 3
};

enum buttons {
    BUTTON_A = 0,
    BUTTON_B = 1,
    BUTTON_SELECT = 2,
    BUTTON_START = 3
};

uint8_t get_joypad(cpu *c, joypad *j);

#endif //CHILLYGB_INPUT_H
