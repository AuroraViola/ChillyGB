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

    // Audio settings
    int volume;
    bool ch_on[4];

    // Video settings
    palette palettes[100];
    int palettes_size;
    int selected_palette;
    bool integer_scaling;
    bool frame_blending;
    bool pixel_grid;
}settings;

extern settings set;

void load_settings();
void save_settings();

#endif //CHILLYGB_SETTINGS_H
