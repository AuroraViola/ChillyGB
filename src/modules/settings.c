#include "../includes/settings.h"
#include "../includes/apu.h"
#include <stdio.h>
#include <malloc.h>
#include "../../cJSON/cJSON.h"

settings set = {};

void load_default_settings() {
    set.volume = 100;
    set.palette = 0;
    set.bootrom_enabled = true;
    set.integer_scaling = false;
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
    cJSON *palette = cJSON_GetObjectItem(settings, "palette");
    if (cJSON_IsNumber(palette))
        set.palette = palette->valueint;
    else
        set.palette = 0;
    cJSON *bootrom = cJSON_GetObjectItem(settings, "bootrom");
    if (cJSON_IsBool(bootrom))
        set.bootrom_enabled = bootrom->valueint;
    else
        set.bootrom_enabled = true;
    cJSON *int_scale = cJSON_GetObjectItem(settings, "int_scale");
    if (cJSON_IsBool(int_scale))
        set.integer_scaling = int_scale->valueint;
    else
        set.integer_scaling = false;

    cJSON_Delete(settings);
    fclose(file);
}

void save_settings() {
    cJSON *settings = cJSON_CreateObject();
    cJSON_AddNumberToObject(settings, "volume", set.volume);
    cJSON_AddNumberToObject(settings, "palette", set.palette);
    cJSON_AddBoolToObject(settings, "bootrom", set.bootrom_enabled);
    cJSON_AddBoolToObject(settings, "int_scale", set.integer_scaling);

    #if defined(PLATFORM_WEB)
    FILE *file = fopen("/saves/settings.json", "w");
    #else
    FILE *file = fopen("settings.json", "w");
    #endif
    fputs(cJSON_Print(settings), file);
    cJSON_Delete(settings);
    fclose(file);
}