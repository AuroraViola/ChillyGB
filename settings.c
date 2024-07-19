#include "settings.h"
#include "apu.h"
#include <stdio.h>
#include <string.h>

void load_settings(settings *s) {
    FILE *f = fopen("settings.conf", "r");
    if (f != NULL) {
        char junk;
        fscanf(f, "%s %i\n", &junk, &s->volume);
        audio.volume = s->volume;
        fscanf(f, "%s %i\n", &junk, &s->palette);
        fscanf(f, "%s %i\n", &junk, &s->custom_boot_logo);
        fclose(f);
    }
    else {
        s->volume = 100;
        audio.volume = s->volume;
        s->palette = 0;
        s->custom_boot_logo = true;
    }
}

void save_settings(settings *s) {
    FILE *file = fopen("settings.conf", "w");
    fprintf(file, "%s: %i\n", "volume", audio.volume);
    fprintf(file, "%s: %i\n", "palette", s->palette);
    fprintf(file, "%s: %i\n", "custom_logo", s->custom_boot_logo);
    fclose(file);
}