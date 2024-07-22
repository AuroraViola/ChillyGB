#include "../includes/settings.h"
#include "../includes/apu.h"
#include <stdio.h>
#include <string.h>

settings set = {};

void load_settings() {
    #if defined(PLATFORM_WEB)
    FILE *f = fopen("/saves/settings.conf", "r");
    #else
    FILE *f = fopen("settings.conf", "r");
    #endif
    if (f != NULL) {
        fscanf(f, "volume: %i\n", &set.volume);
        audio.volume = set.volume;
        fscanf(f, "palette: %i\n", &set.palette);
        fscanf(f, "custom_logo: %i\n", &set.custom_boot_logo);
        fclose(f);
    }
    else {
        set.volume = 100;
        audio.volume = set.volume;
        set.palette = 0;
        set.custom_boot_logo = true;
    }
}

void save_settings() {
    #if defined(PLATFORM_WEB)
    FILE *file = fopen("/saves/settings.conf", "w");
    #else
    FILE *file = fopen("settings.conf", "w");
    #endif
    fprintf(file, "volume: %i\n", audio.volume);
    fprintf(file, "palette: %i\n", set.palette);
    fprintf(file, "custom_logo: %i\n", set.custom_boot_logo);
    fclose(file);
}