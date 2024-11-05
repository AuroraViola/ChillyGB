#include "../includes/settings.h"
#include "../includes/apu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

settings set = {};
settings set_prev = {};
char color_names[4][10] = {"color1", "color2", "color3", "color4"};
char keys_names[8][10] = {"Right:", "Left:", "Up:", "Down:", "A:", "B:", "Select:", "Start:"};

void convert_key(char key_name[15], int key) {
    switch (key) {
        case 32:
            strcpy(key_name, "Space");
            break;
        case 256:
            strcpy(key_name, "Esc");
            break;
        case 257:
            strcpy(key_name, "Enter");
            break;
        case 258:
            strcpy(key_name, "Tab");
            break;
        case 259:
            strcpy(key_name, "Backspace");
            break;
        case 260:
            strcpy(key_name, "Ins");
            break;
        case 261:
            strcpy(key_name, "Del");
            break;
        case 262:
            strcpy(key_name, "Right Arrow");
            break;
        case 263:
            strcpy(key_name, "Left Arrow");
            break;
        case 264:
            strcpy(key_name, "Down Arrow");
            break;
        case 265:
            strcpy(key_name, "Up Arrow");
            break;
        case 266:
            strcpy(key_name, "Page Up");
            break;
        case 267:
            strcpy(key_name, "Page down");
            break;
        case 268:
            strcpy(key_name, "Home");
            break;
        case 269:
            strcpy(key_name, "End");
            break;
        case 280:
            strcpy(key_name, "Caps Lock");
            break;
        case 281:
            strcpy(key_name, "Scroll lock");
            break;
        case 282:
            strcpy(key_name, "Num lock");
            break;
        case 283:
            strcpy(key_name, "Print screen");
            break;
        case 284:
            strcpy(key_name, "Pause");
            break;
        case 290:
            strcpy(key_name, "F1");
            break;
        case 291:
            strcpy(key_name, "F2");
            break;
        case 292:
            strcpy(key_name, "F3");
            break;
        case 293:
            strcpy(key_name, "F4");
            break;
        case 294:
            strcpy(key_name, "F5");
            break;
        case 295:
            strcpy(key_name, "F6");
            break;
        case 296:
            strcpy(key_name, "F7");
            break;
        case 297:
            strcpy(key_name, "F8");
            break;
        case 298:
            strcpy(key_name, "F9");
            break;
        case 299:
            strcpy(key_name, "F10");
            break;
        case 300:
            strcpy(key_name, "F11");
            break;
        case 301:
            strcpy(key_name, "F12");
            break;
        case 340:
            strcpy(key_name, "Shift left");
            break;
        case 341:
            strcpy(key_name, "Ctrl left");
            break;
        case 342:
            strcpy(key_name, "Alt left");
            break;
        case 343:
            #ifdef _WIN32
            strcpy(key_name, "Win left");
            #else
            strcpy(key_name, "Super left");
            #endif
            break;
        case 344:
            strcpy(key_name, "Shift right");
            break;
        case 345:
            strcpy(key_name, "Ctrl right");
            break;
        case 346:
            strcpy(key_name, "Alt right");
            break;
        case 347:
            #ifdef _WIN32
            strcpy(key_name, "Win right");
            #else
            strcpy(key_name, "Super right");
            #endif
            break;
        case 348:
            strcpy(key_name, "menu");
            break;

        default:
            key_name[0] = key;
            key_name[1] = '\0';
            break;
    }
}

void rgb_to_str(Color color, char string[10]) {
    sprintf(string, "#%02X%02X%02X", color.r, color.g, color.b);
}

Color str_to_rgb(char string[10]) {
    int r, g, b;
    sscanf(string, "#%02X%02X%02X", &r, &g, &b);
    return (Color) {r, g, b, 255};
}

void load_default_keys() {
    KeyboardKey keys[] = {KEY_D, KEY_A, KEY_W, KEY_S, KEY_L, KEY_K, KEY_BACKSPACE, KEY_ENTER};
    memcpy(set.keyboard_keys, keys, 8*sizeof(KeyboardKey));
    GamepadButton joystick_keys[] = {
            GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
            GAMEPAD_BUTTON_LEFT_FACE_LEFT,
            GAMEPAD_BUTTON_LEFT_FACE_UP,
            GAMEPAD_BUTTON_LEFT_FACE_DOWN,
            GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
            GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
            GAMEPAD_BUTTON_MIDDLE_LEFT,
            GAMEPAD_BUTTON_MIDDLE_RIGHT
    };
    memcpy(set.gamepad_keys, joystick_keys, 8*sizeof(GamepadButton));
    set.fast_forward_key = KEY_SPACE;
    set.save_state_key = KEY_F2;
    set.load_state_key = KEY_F1;
    set.debugger_key = KEY_F3;
    set.rewind_key = KEY_TAB;
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
    set.selected_gameboy = 1;
    set.selected_palette = 0;
    set.bootrom_enabled = false;
    set.integer_scaling = false;
    set.selected_effect = 0;
    set.frame_blending = false;
    set.accurate_rtc = true;
    set.color_correction = false;
    set.ch_on[0] = true;
    set.ch_on[1] = true;
    set.ch_on[2] = true;
    set.ch_on[3] = true;
    load_default_palettes();
    load_default_keys();
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
    cJSON *selected_gameboy = cJSON_GetObjectItem(settings, "selected_gameboy");
    if (cJSON_IsNumber(selected_gameboy))
        set.selected_gameboy = selected_gameboy->valueint;
    else
        set.selected_gameboy = 1;

    cJSON *bootrom_dmg = cJSON_GetObjectItem(settings, "bootrom_path_dmg");
    if (cJSON_IsString(bootrom_dmg))
        strcpy(set.bootrom_path_dmg, bootrom_dmg->valuestring);
    else
        strcpy(set.bootrom_path_dmg, "");

    cJSON *bootrom_cgb = cJSON_GetObjectItem(settings, "bootrom_path_cgb");
    if (cJSON_IsString(bootrom_cgb))
        strcpy(set.bootrom_path_cgb, bootrom_cgb->valuestring);
    else
        strcpy(set.bootrom_path_cgb, "");

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
    cJSON *accurate_rtc = cJSON_GetObjectItem(settings, "accurate_rtc");
    if (cJSON_IsBool(accurate_rtc))
        set.accurate_rtc = accurate_rtc->valueint;
    else
        set.accurate_rtc = true;
    cJSON *palette = cJSON_GetObjectItem(settings, "selected_palette");
    if (cJSON_IsNumber(palette))
        set.selected_palette = palette->valueint;
    else
        set.selected_palette = 0;

    cJSON *color_correction = cJSON_GetObjectItem(settings, "color_correction");
    if (cJSON_IsBool(color_correction))
        set.color_correction = color_correction->valueint;
    else
        set.color_correction = false;

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
    cJSON *selected_effect = cJSON_GetObjectItem(settings, "selected_effect");
    if (cJSON_IsNumber(selected_effect))
        set.selected_effect = selected_effect->valueint;
    else
        set.selected_effect = 0;
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
    int keys_size = cJSON_GetArraySize(cJSON_GetObjectItem(settings, "keyboard_keys"));
    int gamepad_size = cJSON_GetArraySize(cJSON_GetObjectItem(settings, "gamepad_keys"));

    if (keys_size == 8 && gamepad_size == 8) {
        cJSON *keys = cJSON_GetObjectItem(settings, "keyboard_keys");
        cJSON *gamepad = cJSON_GetObjectItem(settings, "gamepad_keys");

        for (int i = 0; i < 8; i++) {
            cJSON *item = cJSON_GetArrayItem(keys, i);
            cJSON *item2 = cJSON_GetArrayItem(gamepad, i);
            set.keyboard_keys[i] = item->valueint;
            set.gamepad_keys[i] = item2->valueint;
        }
    }
    else {
        load_default_keys();
    }
    cJSON *fast_forward_key = cJSON_GetObjectItem(settings, "fast_forward_key");
    if (cJSON_IsNumber(fast_forward_key))
        set.fast_forward_key = fast_forward_key->valueint;
    else
        set.fast_forward_key = KEY_SPACE;
    cJSON *rewind_key = cJSON_GetObjectItem(settings, "rewind_key");
    if (cJSON_IsNumber(rewind_key))
        set.rewind_key = rewind_key->valueint;
    else
        set.rewind_key = KEY_TAB;
    cJSON *load_state_key = cJSON_GetObjectItem(settings, "load_state_key");
    if (cJSON_IsNumber(load_state_key))
        set.load_state_key = load_state_key->valueint;
    else
        set.load_state_key = KEY_F1;
    cJSON *save_state_key = cJSON_GetObjectItem(settings, "save_state_key");
    if (cJSON_IsNumber(save_state_key))
        set.save_state_key = save_state_key->valueint;
    else
        set.save_state_key = KEY_F2;
    cJSON *debugger_key = cJSON_GetObjectItem(settings, "debugger_key");
    if (cJSON_IsNumber(debugger_key))
        set.debugger_key = debugger_key->valueint;
    else
        set.debugger_key = KEY_F3;

    cJSON *motion_style = cJSON_GetObjectItem(settings, "motion_style");
    if (cJSON_IsNumber(motion_style))
        set.motion_style = motion_style->valueint;
    else
        set.motion_style = 0;

    cJSON *dsu_ip = cJSON_GetObjectItem(settings, "dsu_ip");
    if (cJSON_IsString(dsu_ip))
        strcpy(set.dsu_ip, dsu_ip->valuestring);
    else
        strcpy(set.dsu_ip, "127.0.0.1");

    cJSON *dsu_port = cJSON_GetObjectItem(settings, "dsu_port");
    if (cJSON_IsNumber(dsu_port))
        set.dsu_port = dsu_port->valueint;
    else
        set.dsu_port = 26760;

    if (set.palettes_size <= set.selected_palette)
        set.selected_palette = 0;

    cJSON_Delete(settings);
    fclose(file);
}

void save_settings() {
    cJSON *settings = cJSON_CreateObject();
    cJSON_AddBoolToObject(settings, "bootrom", set.bootrom_enabled);
    cJSON_AddBoolToObject(settings, "accurate_rtc", set.accurate_rtc);
    cJSON_AddNumberToObject(settings, "volume", set.volume);
    cJSON_AddNumberToObject(settings, "selected_gameboy", set.selected_gameboy);
    cJSON_AddBoolToObject(settings, "ch1", set.ch_on[0]);
    cJSON_AddBoolToObject(settings, "ch2", set.ch_on[1]);
    cJSON_AddBoolToObject(settings, "ch3", set.ch_on[2]);
    cJSON_AddBoolToObject(settings, "ch4", set.ch_on[3]);
    cJSON_AddNumberToObject(settings, "selected_palette", set.selected_palette);
    cJSON_AddBoolToObject(settings, "frame_blending", set.frame_blending);
    cJSON_AddNumberToObject(settings, "selected_effect", set.selected_effect);
    cJSON_AddBoolToObject(settings, "int_scale", set.integer_scaling);
    cJSON_AddBoolToObject(settings, "color_correction", set.color_correction);

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

    cJSON *keys_array = cJSON_CreateArray();
    for (int i = 0; i < 8; i++) {
        cJSON_AddItemToArray(keys_array, cJSON_CreateNumber(set.keyboard_keys[i]));
    }
    cJSON_AddItemToObject(settings, "keyboard_keys", keys_array);

    cJSON *gamepad_array = cJSON_CreateArray();
    for (int i = 0; i < 8; i++) {
        cJSON_AddItemToArray(gamepad_array, cJSON_CreateNumber(set.gamepad_keys[i]));
    }
    cJSON_AddItemToObject(settings, "gamepad_keys", gamepad_array);

    cJSON_AddNumberToObject(settings, "fast_forward_key", set.fast_forward_key);
    cJSON_AddNumberToObject(settings, "save_state_key", set.save_state_key);
    cJSON_AddNumberToObject(settings, "load_state_key", set.load_state_key);
    cJSON_AddNumberToObject(settings, "debugger_key", set.debugger_key);
    cJSON_AddNumberToObject(settings, "rewind_key", set.rewind_key);
    cJSON_AddStringToObject(settings, "bootrom_path_cgb", set.bootrom_path_cgb);
    cJSON_AddStringToObject(settings, "bootrom_path_dmg", set.bootrom_path_dmg);
    cJSON_AddNumberToObject(settings, "motion_style", set.motion_style);
    cJSON_AddStringToObject(settings, "dsu_ip", set.dsu_ip);
    cJSON_AddNumberToObject(settings, "dsu_port", set.dsu_port);

    #if defined(PLATFORM_WEB)
    FILE *file = fopen("/saves/settings.json", "w");
    #else
    FILE *file = fopen("settings.json", "w");
    #endif
    fputs(cJSON_Print(settings), file);
    cJSON_Delete(settings);
    fclose(file);
}