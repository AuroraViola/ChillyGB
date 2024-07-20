#include <stdbool.h>

#ifndef CHILLYGB_INPUT_H
#define CHILLYGB_INPUT_H

typedef struct {
    KeyboardKey keys_dpad[4];
    KeyboardKey keys_btn[4];
    bool dpad[4];
    bool btn[4];
    bool dpad_on;
    bool btn_on;
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

extern joypad joypad1;
bool update_keys();

#endif //CHILLYGB_INPUT_H
