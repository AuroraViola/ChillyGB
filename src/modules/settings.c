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
        char string;
        fscanf(f, "%s %i\n", &string, &set.volume);
        audio.volume = set.volume;
        fscanf(f, "%s %i\n", &string, &set.palette);
        fscanf(f, "%s %i\n", &string, &set.custom_boot_logo);
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
    fprintf(file, "%s: %i\n", "volume", audio.volume);
    fprintf(file, "%s: %i\n", "palette", set.palette);
    fprintf(file, "%s: %i\n", "custom_logo", set.custom_boot_logo);
    fclose(file);
}