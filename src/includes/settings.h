#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_SETTINGS_H
#define CHILLYGB_SETTINGS_H

typedef struct {
    int palette;
    int volume;
    bool custom_boot_logo;
}settings;

extern settings set;

void load_settings();
void save_settings();

#endif //CHILLYGB_SETTINGS_H
