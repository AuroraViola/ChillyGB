#include "../includes/settings.h"
#include "../includes/apu.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../../cJSON/cJSON.h"

settings set = {};
char color_names[4][10] = {"color1", "color2", "color3", "color4"};

void rgb_to_str(Color color, char string[10]) {
    sprintf(string, "#%02X%02X%02X", color.r, color.g, color.b);
}

Color str_to_rgb(char string[10]) {
    int r, g, b;
    sscanf(string, "#%02X%02X%02X", &r, &g, &b);
    return (Color) {r, g, b, 255};
}

void load_default_palettes() {
    set.palettes_size = 9;
    strcpy(set.palettes[0].name, "Chilly Original");
    set.palettes[0].colors[0] = (Color) {185, 237, 186, 255};
    set.palettes[0].colors[1] = (Color) {118, 196, 123, 255};
    set.palettes[0].colors[2] = (Color) {49, 106, 64, 255};
    set.palettes[0].colors[3] = (Color) {10, 38, 16, 255};
    strcpy(set.palettes[1].name, "Chilly Pocket");
    set.palettes[1].colors[0] = (Color) {229, 227, 186, 255};
    set.palettes[1].colors[1] = (Color) {184, 178, 123, 255};
    set.palettes[1].colors[2] = (Color) {106, 102, 64, 255};
    set.palettes[1].colors[3] = (Color) {40, 38, 16, 255};
    strcpy(set.palettes[2].name, "Chilly Light");
    set.palettes[2].colors[0] = (Color) {185, 237, 242, 255};
    set.palettes[2].colors[1] = (Color) {118, 196, 210, 255};
    set.palettes[2].colors[2] = (Color) {49, 106, 129, 255};
    set.palettes[2].colors[3] = (Color) {10, 38, 51, 255};
    strcpy(set.palettes[3].name, "Super Chilly");
    set.palettes[3].colors[0] = (Color) {247, 234, 206, 255};
    set.palettes[3].colors[1] = (Color) {232, 134, 92, 255};
    set.palettes[3].colors[2] = (Color) {206, 59, 17, 255};
    set.palettes[3].colors[3] = (Color) {77, 10, 65, 255};
    strcpy(set.palettes[4].name, "Grayscale");
    set.palettes[4].colors[0] = (Color) {255, 255, 255, 255};
    set.palettes[4].colors[1] = (Color) {176, 176, 176, 255};
    set.palettes[4].colors[2] = (Color) {104, 104, 104, 255};
    set.palettes[4].colors[3] = (Color) {0, 0, 0, 255};
    strcpy(set.palettes[5].name, "Lime (SameBoy)");
    set.palettes[5].colors[0] = (Color) {198, 222, 140, 255};
    set.palettes[5].colors[1] = (Color) {132, 165, 99, 255};
    set.palettes[5].colors[2] = (Color) {57, 97, 57, 255};
    set.palettes[5].colors[3] = (Color) {8, 24, 16, 255};
    strcpy(set.palettes[6].name, "Olive (SameBoy)");
    set.palettes[6].colors[0] = (Color) {194, 206, 147, 255};
    set.palettes[6].colors[1] = (Color) {129, 141, 102, 255};
    set.palettes[6].colors[2] = (Color) {58, 76, 58, 255};
    set.palettes[6].colors[3] = (Color) {7, 16, 14, 255};
    strcpy(set.palettes[7].name, "Teal (SameBoy)");
    set.palettes[7].colors[0] = (Color) {127, 226, 195, 255};
    set.palettes[7].colors[1] = (Color) {86, 180, 149, 255};
    set.palettes[7].colors[2] = (Color) {53, 120, 98, 255};
    set.palettes[7].colors[3] = (Color) {10, 28, 21, 255};
    strcpy(set.palettes[8].name, "bgb");
    set.palettes[8].colors[0] = (Color) {224, 248, 208, 255};
    set.palettes[8].colors[1] = (Color) {136, 192, 112, 255};
    set.palettes[8].colors[2] = (Color) {52, 104, 86, 255};
    set.palettes[8].colors[3] = (Color) {8, 24, 32, 255};
}

void load_default_settings() {
    set.volume = 100;
    set.selected_palette = 0;
    set.bootrom_enabled = false;
    set.integer_scaling = false;
    set.pixel_grid = false;
    set.frame_blending = false;
    set.ch_on[0] = true;
    set.ch_on[1] = true;
    set.ch_on[2] = true;
    set.ch_on[3] = true;
    load_default_palettes();
}

void load_settings() {
    #if defined(PLATFORM_WEB)
    FILE *file = fopen("/saves/settings.json", "r");
    #else
    FILE *file = fopen("settings.json", "r");
    #endif
    if (file == NULL) {
        printf("Failed to open settings.json\nLoading default settings\n");
        load_default_settings();
        return;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';

    cJSON *settings = cJSON_Parse(buffer);
    if (settings == NULL) {
        printf("Failed to parse settings.json\nLoading default settings\n");
        load_default_settings();
        cJSON_Delete(settings);
        fclose(file);
        return;
    }

    cJSON *volume = cJSON_GetObjectItem(settings, "volume");
    if (cJSON_IsNumber(volume))
        set.volume = volume->valueint;
    else
        set.volume = 100;
    cJSON *ch1 = cJSON_GetObjectItem(settings, "ch1");
    if (cJSON_IsBool(ch1))
        set.ch_on[0] = ch1->valueint;
    else
        set.ch_on[0] = true;
    cJSON *ch2 = cJSON_GetObjectItem(settings, "ch2");
    if (cJSON_IsBool(ch2))
        set.ch_on[1] = ch2->valueint;
    else
        set.ch_on[1] = true;
    cJSON *ch3 = cJSON_GetObjectItem(settings, "ch3");
    if (cJSON_IsBool(ch3))
        set.ch_on[2] = ch3->valueint;
    else
        set.ch_on[2] = true;
    cJSON *ch4 = cJSON_GetObjectItem(settings, "ch4");
    if (cJSON_IsBool(ch4))
        set.ch_on[3] = ch4->valueint;
    else
        set.ch_on[3] = true;
    cJSON *palette = cJSON_GetObjectItem(settings, "selected_palette");
    if (cJSON_IsNumber(palette))
        set.selected_palette = palette->valueint;
    else
        set.selected_palette = 0;
    cJSON *bootrom = cJSON_GetObjectItem(settings, "bootrom");
    if (cJSON_IsBool(bootrom))
        set.bootrom_enabled = bootrom->valueint;
    else
        set.bootrom_enabled = true;

    cJSON *frame_blending = cJSON_GetObjectItem(settings, "frame_blending");
    if (cJSON_IsBool(frame_blending))
        set.frame_blending = frame_blending->valueint;
    else
        set.frame_blending = false;
    cJSON *pixel_grid = cJSON_GetObjectItem(settings, "pixel_grid");
    if (cJSON_IsBool(pixel_grid))
        set.pixel_grid = pixel_grid->valueint;
    else
        set.pixel_grid = false;
    cJSON *int_scale = cJSON_GetObjectItem(settings, "int_scale");
    if (cJSON_IsBool(int_scale))
        set.integer_scaling = int_scale->valueint;
    else
        set.integer_scaling = false;

    set.palettes_size = cJSON_GetArraySize(cJSON_GetObjectItem(settings, "palettes"));
    if (set.palettes_size != 0) {
        cJSON *palettes = cJSON_GetObjectItem(settings, "palettes");

        for (int i = 0; i < set.palettes_size; i++) {
            cJSON *item = cJSON_GetArrayItem(palettes, i);
            strcpy(set.palettes[i].name, cJSON_GetObjectItem(item, "name")->valuestring);
            for (int j = 0; j < 4; j++) {
                set.palettes[i].colors[j] = str_to_rgb(cJSON_GetObjectItem(item, color_names[j])->valuestring);
            }
        }
    }
    else {
        load_default_palettes();
    }

    if (set.palettes_size <= set.selected_palette)
        set.selected_palette = 0;

    cJSON_Delete(settings);
    fclose(file);
}

void save_settings() {
    cJSON *settings = cJSON_CreateObject();
    cJSON_AddBoolToObject(settings, "bootrom", set.bootrom_enabled);
    cJSON_AddNumberToObject(settings, "volume", set.volume);
    cJSON_AddBoolToObject(settings, "ch1", set.ch_on[0]);
    cJSON_AddBoolToObject(settings, "ch2", set.ch_on[1]);
    cJSON_AddBoolToObject(settings, "ch3", set.ch_on[2]);
    cJSON_AddBoolToObject(settings, "ch4", set.ch_on[3]);
    cJSON_AddNumberToObject(settings, "selected_palette", set.selected_palette);
    cJSON_AddBoolToObject(settings, "frame_blending", set.frame_blending);
    cJSON_AddBoolToObject(settings, "pixel_grid", set.pixel_grid);
    cJSON_AddBoolToObject(settings, "int_scale", set.integer_scaling);

    cJSON *palettes_array = cJSON_CreateArray();

    for (int i = 0; i < set.palettes_size; i++) {
        cJSON *palette = cJSON_CreateObject();
        cJSON_AddStringToObject(palette, "name", set.palettes[i].name);
        char str[10];

        for (int j = 0; j < 4; j++) {
            rgb_to_str(set.palettes[i].colors[j], str);
            cJSON_AddStringToObject(palette, color_names[j], str);
        }
        cJSON_AddItemToArray(palettes_array, palette);
    }

    cJSON_AddItemToObject(settings, "palettes", palettes_array);

    #if defined(PLATFORM_WEB)
    FILE *file = fopen("/saves/settings.json", "w");
    #else
    FILE *file = fopen("settings.json", "w");
    #endif
    fputs(cJSON_Print(settings), file);
    cJSON_Delete(settings);
    fclose(file);
}