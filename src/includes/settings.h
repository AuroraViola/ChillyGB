#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_SETTINGS_H
#define CHILLYGB_SETTINGS_H

typedef struct {
    int palette;
    int volume;
    bool custom_boot_logo;
}settings;

void load_settings(settings *s);
void save_settings(settings *s);

#endif //CHILLYGB_SETTINGS_H
