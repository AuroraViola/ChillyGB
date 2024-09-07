#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

#ifndef CHILLYGB_SETTINGS_H
#define CHILLYGB_SETTINGS_H

typedef struct {
    char name[25];
    Color colors[4];
}palette;

typedef struct {
    // Emulator settings
    bool bootrom_enabled;
    bool accurate_rtc;

    // Audio settings
    int volume;
    bool ch_on[4];

    // Video settings
    palette palettes[100];
    int palettes_size;
    int selected_palette;
    int selected_gameboy;
    bool integer_scaling;
    bool frame_blending;
    int selected_effect;

    // Input settings
    KeyboardKey keyboard_keys[8];
    GamepadButton gamepad_keys[8];
    KeyboardKey fast_forward_key;
    KeyboardKey save_state_key;
    KeyboardKey load_state_key;
    KeyboardKey debugger_key;
    KeyboardKey rewind_key;
}settings;

extern settings set;
extern settings set_prev;
extern char keys_names[8][10];
extern char converted_keys[][10];

void convert_key(char key_name[15], int key);
void load_settings();
void save_settings();

#endif //CHILLYGB_SETTINGS_H
