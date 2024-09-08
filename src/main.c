#define NK_INCLUDE_STANDARD_BOOL
#include "raylib.h"
#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/settings.h"
#include "includes/apu.h"
#include "includes/timer.h"
#include "includes/debug.h"
#include "includes/input.h"
#include "includes/cartridge.h"
#include "includes/open_dialog.h"
#include "includes/savestates.h"
#include "includes/opcodes.h"
#include "../raylib-nuklear/include/raylib-nuklear.h"
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
char *stpcpy (char *__restrict __dest, const char *__restrict __src);
#endif

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#if PLATFORM_NX
    #define RES_DIR "romfs:/"
#else 
    #define RES_DIR "res/"
#endif


#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef enum EmuModes{
    MENU = 0,
    GAME,
    DEBUG,
}EmuModes;

typedef enum SettingsView {
    SET_EMU = 0,
    SET_VIDEO,
    SET_AUDIO,
    SET_INPUT,
}SettingsView;

typedef enum InputSettingsView {
    INPUT_GAMEPAD = 0,
    INPUT_ADVANCED,
}InputSettingsView;

typedef enum ShadersList {
    SHADER_DEFAULT = 0,
    SHADER_PIXEL_GRID,
}ShadersList;

cpu c = {};
bool exited = false;
bool game_started = false;
bool show_settings = false;
bool show_about = false;
char rom_name[256];
uint8_t emulator_mode = MENU;
uint8_t settings_view = SET_EMU;
uint8_t input_settings_view = INPUT_GAMEPAD;
uint8_t palette_color_selected = 0;
bool is_selected_input = false;
uint8_t selected_input = 0;
Color pixels_screen[144][160];
Color pixels[144][160] = { 0 };
Image display_image;
Image logo_image;
Texture2D logo;
Texture2D display;
char instructions[30][50];
float scale;
int scale_integer;
float speed_mult = 1;
int ff_speed = 1;
struct nk_context *ctx;
char comboxes[2500] = "";
char version[20] = "v0.1.1";
int key_order[8] = {2, 5, 3, 4, 1, 6, 0, 7};
struct nk_style_button tab_button_style = {};
struct nk_style_button tab_button_active_style = {};
struct nk_style_button button_dpad_style = {};
struct nk_style_button button_dpad_pressed_style = {};
struct nk_style_button button_buttons_style = {};
struct nk_style_button button_buttons_pressed_style = {};
Shader shaders[2];

void load_cartridge(char *path) {
    strcpy(rom_name, path);
    if(!load_game(&c.cart, rom_name)){
        fprintf(stderr, "Failed to open game: %s\n", rom_name);
        return;
    }
    if (load_bootrom(&c.bootrom) && set.bootrom_enabled)
        initialize_cpu_memory(&c, &set);
    else
        initialize_cpu_memory_no_bootrom(&c, &set);
    emulator_mode = GAME;
    game_started = true;
    ResumeAudioStream(audio.ch1.stream);
    ResumeAudioStream(audio.ch2.stream);
    ResumeAudioStream(audio.ch3.stream);
    ResumeAudioStream(audio.ch4.stream);
}

void DrawNavBar() {
    if (nk_begin(ctx, "Overview", nk_rect(0, 0, GetScreenWidth(), 35), 0)) {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Load ROM", NK_TEXT_LEFT)) {
                char *path = do_open_rom_dialog();
                if (path != NULL) {
                    load_cartridge(path);
                }
            }
            if (nk_menu_item_label(ctx, "Save SRAM", NK_TEXT_LEFT)) {
                if (game_started) {
                    save_game(&c.cart, rom_name);
                }
            }
            if (nk_menu_item_label(ctx, "Load state", NK_TEXT_LEFT)) {
                if (game_started)
                    load_state(&c, rom_name);
            }
            if (nk_menu_item_label(ctx, "Save state", NK_TEXT_LEFT)) {
                if (game_started)
                    save_state(&c, rom_name);
            }
            #ifndef PLATFORM_WEB
            if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
                exited = true;
            }
            #endif
            nk_menu_end(ctx);
        }

        nk_layout_row_push(ctx, 120);
        if (nk_menu_begin_label(ctx, "Emulation", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Reset", NK_TEXT_LEFT)) {
                if (game_started) {
                    if (load_bootrom(&c.bootrom) && set.bootrom_enabled)
                        initialize_cpu_memory(&c, &set);
                    else
                        initialize_cpu_memory_no_bootrom(&c, &set);
                    emulator_mode = GAME;
                    ResumeAudioStream(audio.ch1.stream);
                    ResumeAudioStream(audio.ch2.stream);
                    ResumeAudioStream(audio.ch3.stream);
                    ResumeAudioStream(audio.ch4.stream);
                }
            }
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 70);
        if (nk_menu_begin_label(ctx, "Tools", NK_TEXT_LEFT, nk_vec2(230, 200))) {
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Debugger", NK_TEXT_LEFT)) {
                if (game_started) {
                    emulator_mode = DEBUG;
                }
            }
            if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT)) {
                if (show_settings == false) {
                    memcpy(&set_prev, &set, sizeof(settings));
                }
                show_settings = true;
            }
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "Help", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Github", NK_TEXT_LEFT)) {
                #ifdef _WIN32
                    system("start https://github.com/AuroraViola/ChillyGB");
                #elif __APPLE__
                    system("open https://github.com/AuroraViola/ChillyGB");
                #elif __linux__
                    system("xdg-open https://github.com/AuroraViola/ChillyGB");
                #elif defined(PLATFORM_WEB)
                EM_ASM(
                    window.open("https://github.com/AuroraViola/ChillyGB").focus();
                );
                #else
                    printf("Failed to open GitHub link: Unsupported OS\n");
                #endif
            }
            if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
                show_about = true;
            }
            nk_menu_end(ctx);
        }
        nk_menubar_end(ctx);
    }
    nk_end(ctx);
}

void draw_input_editor(const char *text, KeyboardKey *key_variable, int option_number) {
    char input_value[15];
    nk_label(ctx, text, NK_TEXT_ALIGN_RIGHT|NK_TEXT_ALIGN_MIDDLE);
    if (is_selected_input && option_number == selected_input) {
        sprintf(input_value, "...");
        int key_pressed = GetKeyPressed();
        if (key_pressed != 0) {
            if (key_pressed != KEY_ESCAPE)
                *key_variable = key_pressed;
            is_selected_input = false;
        }
    }
    else {
        char key_name[15];
        convert_key(key_name, *key_variable);
        sprintf(input_value, "%s", key_name);
    }
    if (nk_button_label(ctx, input_value)) {
        selected_input = option_number;
        is_selected_input = true;
    }
}

void draw_settings_window() {
    if(nk_begin_titled(ctx, "ctx-settings","Settings", nk_rect(20, 60, 550, 505),
                NK_WINDOW_MOVABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 30, 4);
        if (nk_button_label_styled(ctx, (settings_view != SET_EMU) ? &tab_button_style : &tab_button_active_style, "Emulation"))
            settings_view = SET_EMU;
        if (nk_button_label_styled(ctx, (settings_view != SET_AUDIO) ? &tab_button_style : &tab_button_active_style, "Audio"))
            settings_view = SET_AUDIO;
        if (nk_button_label_styled(ctx, (settings_view != SET_VIDEO) ? &tab_button_style : &tab_button_active_style, "Video"))
            settings_view = SET_VIDEO;
        if (nk_button_label_styled(ctx, (settings_view != SET_INPUT) ? &tab_button_style : &tab_button_active_style, "Input"))
            settings_view = SET_INPUT;

        struct nk_vec2 size = {280, 200};
        nk_layout_row_dynamic(ctx, 370, 1);
        if (nk_group_begin(ctx, "settings_view", 0)) {
            switch (settings_view) {
                case SET_EMU:
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label(ctx, "Emulated Model:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_combobox_string(ctx, "Game Boy (DMG)\0Game Boy Color (CGB)", &set.selected_gameboy, 2, 20, size);
                    nk_checkbox_label(ctx, "Sync RTC to gameboy clock (accurate)", &set.accurate_rtc);
                    nk_checkbox_label(ctx, "Boot Rom", &set.bootrom_enabled);
                    break;
                case SET_AUDIO:
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label(ctx, "Sound Volume", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ctx, 30, 2);
                    set.volume = nk_slide_int(ctx, 0, set.volume, 100, 1);
                    char str[6];
                    sprintf(str, "%i%%", set.volume);
                    nk_label(ctx, str, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_checkbox_label(ctx, "Channel 1", &set.ch_on[0]);
                    nk_checkbox_label(ctx, "Channel 2", &set.ch_on[1]);
                    nk_checkbox_label(ctx, "Channel 3", &set.ch_on[2]);
                    nk_checkbox_label(ctx, "Channel 4", &set.ch_on[3]);
                    break;
                case SET_VIDEO:
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_label(ctx, "DMG Palette:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);

                    nk_layout_row_dynamic(ctx, 30, 2);
                    int comboxes_len = 0;
                    for (int i = 0; i < set.palettes_size; i++) {
                        stpcpy(comboxes + comboxes_len, set.palettes[i].name);
                        comboxes_len += strlen(set.palettes[i].name)+1;
                    }
                    nk_combobox_string(ctx, comboxes, &set.selected_palette, set.palettes_size, 20, size);
                    if (nk_button_label(ctx, "Add Palette")) {
                        if (set.palettes_size < 100) {
                            strcpy(set.palettes[set.palettes_size].name, "Custom Palette");
                            memcpy(set.palettes[set.palettes_size].colors, set.palettes[set.selected_palette].colors, 4*sizeof(Color));
                            set.selected_palette = set.palettes_size;
                            set.palettes_size++;
                        }
                    }
                    nk_spacer(ctx);
                    if (set.selected_palette > 4) {
                        if (nk_button_label(ctx, "Remove Palette")) {
                            for (int i = set.selected_palette; i < set.palettes_size; i++) {
                                strcpy(set.palettes[i].name, set.palettes[i+1].name);
                                memcpy(set.palettes[i].colors, set.palettes[i+1].colors, 4*sizeof(Color));
                            }
                            set.palettes_size--;
                            if (set.selected_palette == set.palettes_size)
                                set.selected_palette--;
                        }
                    }

                    nk_layout_row_dynamic(ctx, 60, 4);
                    struct nk_color colors[4] = {
                        {
                            set.palettes[set.selected_palette].colors[0].r,
                            set.palettes[set.selected_palette].colors[0].g,
                            set.palettes[set.selected_palette].colors[0].b,
                            255
                        },
                        {
                            set.palettes[set.selected_palette].colors[1].r,
                            set.palettes[set.selected_palette].colors[1].g,
                            set.palettes[set.selected_palette].colors[1].b,
                            255
                        },
                        {
                            set.palettes[set.selected_palette].colors[2].r,
                            set.palettes[set.selected_palette].colors[2].g,
                            set.palettes[set.selected_palette].colors[2].b,
                            255
                        },
                        {
                            set.palettes[set.selected_palette].colors[3].r,
                            set.palettes[set.selected_palette].colors[3].g,
                            set.palettes[set.selected_palette].colors[3].b,
                            255
                        },
                    };
                    for (int i = 0; i < 4; i++) {
                        if (nk_button_color(ctx, colors[i])) {
                            palette_color_selected = i;
                        }
                    }

                    if (set.selected_palette > 4) {
                        char rgb_value[6];
                        // Red slider
                        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ctx, 20);
                        nk_label(ctx, "R:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ctx, 445);
                        set.palettes[set.selected_palette].colors[palette_color_selected].r = nk_slide_int(ctx, 0, set.palettes[set.selected_palette].colors[palette_color_selected].r, 255, 1);
                        nk_layout_row_push(ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[palette_color_selected].r);
                        nk_label(ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ctx);

                        // Green slider
                        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ctx, 20);
                        nk_label(ctx, "G:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ctx, 445);
                        set.palettes[set.selected_palette].colors[palette_color_selected].g = nk_slide_int(ctx, 0, set.palettes[set.selected_palette].colors[palette_color_selected].g, 255, 1);
                        nk_layout_row_push(ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[palette_color_selected].g);
                        nk_label(ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ctx);

                        // Blue slider
                        nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ctx, 20);
                        nk_label(ctx, "B:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ctx, 445);
                        set.palettes[set.selected_palette].colors[palette_color_selected].b = nk_slide_int(ctx, 0, set.palettes[set.selected_palette].colors[palette_color_selected].b, 255, 1);
                        nk_layout_row_push(ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[palette_color_selected].b);
                        nk_label(ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ctx);
                    }

                    nk_layout_row_dynamic(ctx, 30, 2);
                    nk_label(ctx, "Video effect:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_combobox_string(ctx, "None\0Pixel grid", &set.selected_effect, 2, 20, size);
                    nk_layout_row_dynamic(ctx, 30, 1);
                    nk_checkbox_label(ctx, "Integer Scaling", &set.integer_scaling);
                    //nk_checkbox_label(ctx, "Frame Blending", &set.frame_blending);
                    break;
                case SET_INPUT:
                    nk_layout_row_dynamic(ctx, 30, 2);
                    if (nk_button_label_styled(ctx, (input_settings_view != INPUT_GAMEPAD) ? &tab_button_style : &tab_button_active_style, "Gamepad"))
                        input_settings_view = INPUT_GAMEPAD;
                    if (nk_button_label_styled(ctx, (input_settings_view != INPUT_ADVANCED) ? &tab_button_style : &tab_button_active_style, "Advanced"))
                        input_settings_view = INPUT_ADVANCED;
                    nk_layout_row_dynamic(ctx, 150, 1);
                    if (nk_group_begin(ctx, "key_management", 0)) {
                        nk_layout_row_dynamic(ctx, 30, 4);
                        switch (input_settings_view) {
                            case INPUT_GAMEPAD:
                                for (int i = 0; i < 8; i++) {
                                    draw_input_editor(keys_names[key_order[i]], &set.keyboard_keys[key_order[i]], i);
                                }
                                break;
                            case INPUT_ADVANCED:
                                draw_input_editor("Save state:", &set.save_state_key, 11);
                                draw_input_editor("Fast forward:", &set.fast_forward_key, 10);
                                //draw_input_editor("Rewind:", &set.rewind_key, 12);
                                draw_input_editor("Load state:", &set.load_state_key, 13);
                                draw_input_editor("Debugger:", &set.debugger_key, 14);
                                break;
                        }
                        nk_group_end(ctx);
                    }
                    nk_layout_row_begin(ctx, NK_STATIC, 150, 2);
                    nk_layout_row_push(ctx, 120);
                    nk_spacer(ctx);
                    nk_layout_row_push(ctx, 300);
                    if (nk_group_begin(ctx, "gamepad_view", 0)) {
                        nk_layout_space_begin(ctx, NK_STATIC, 130, 10);
                        nk_layout_space_push(ctx, nk_rect(15, 45, 60, 30));
                        nk_button_symbol_styled(ctx, &button_dpad_style, NK_SYMBOL_NONE);
                        nk_layout_space_push(ctx, nk_rect(30, 30, 30, 60));
                        nk_button_symbol_styled(ctx, &button_dpad_style, NK_SYMBOL_NONE);
                        nk_layout_space_push(ctx, nk_rect(0, 45, 30, 30));
                        nk_button_symbol_styled(ctx, (IsKeyDown(set.keyboard_keys[DPAD_LEFT])) ? &button_dpad_pressed_style : &button_dpad_style,NK_SYMBOL_TRIANGLE_LEFT);
                        nk_layout_space_push(ctx, nk_rect(60, 45, 30, 30));
                        nk_button_symbol_styled(ctx, (IsKeyDown(set.keyboard_keys[DPAD_RIGHT])) ? &button_dpad_pressed_style : &button_dpad_style, NK_SYMBOL_TRIANGLE_RIGHT);
                        nk_layout_space_push(ctx, nk_rect(30, 15, 30, 30));
                        nk_button_symbol_styled(ctx, (IsKeyDown(set.keyboard_keys[DPAD_UP])) ? &button_dpad_pressed_style : &button_dpad_style, NK_SYMBOL_TRIANGLE_UP);
                        nk_layout_space_push(ctx, nk_rect(30, 75, 30, 30));
                        nk_button_symbol_styled(ctx, (IsKeyDown(set.keyboard_keys[DPAD_DOWN])) ? &button_dpad_pressed_style : &button_dpad_style, NK_SYMBOL_TRIANGLE_DOWN);
                        nk_layout_space_push(ctx, nk_rect(105, 120, 30, 10));
                        nk_button_label_styled(ctx, (IsKeyDown(set.keyboard_keys[BUTTON_SELECT+4])) ? &button_buttons_pressed_style : &button_buttons_style, "");
                        nk_layout_space_push(ctx, nk_rect(150, 120, 30, 10));
                        nk_button_label_styled(ctx, (IsKeyDown(set.keyboard_keys[BUTTON_START+4])) ? &button_buttons_pressed_style : &button_buttons_style, "");
                        nk_layout_space_push(ctx, nk_rect(180, 50, 40, 40));
                        nk_button_label_styled(ctx, (IsKeyDown(set.keyboard_keys[BUTTON_B+4])) ? &button_buttons_pressed_style : &button_buttons_style, "B");
                        nk_layout_space_push(ctx, nk_rect(240, 30, 40, 40));
                        nk_button_label_styled(ctx, (IsKeyDown(set.keyboard_keys[BUTTON_A+4])) ? &button_buttons_pressed_style : &button_buttons_style, "A");
                        nk_layout_space_end(ctx);
                        nk_group_end(ctx);
                    }
                    nk_layout_row_end(ctx);
                    break;
            }
            nk_group_end(ctx);
        }
        nk_layout_row_dynamic(ctx, 40, 4);
        nk_spacer(ctx);
        nk_spacer(ctx);
        if (nk_button_label(ctx, "Cancel")) {
            show_settings = false;
            is_selected_input = false;
            memcpy(&set, &set_prev, sizeof(settings));
        }
        if (nk_button_label(ctx, "Apply")) {
            show_settings = false;
            is_selected_input = false;
            save_settings();
            memcpy(&set_prev, &set, sizeof(settings));
        }
    }
    nk_end(ctx);
}

void draw_about_window() {
    if(nk_begin_titled(ctx, "ctx-about","About", nk_rect((GetScreenWidth()/2-200), (GetScreenHeight()/2-250), 400, 520),
                                              NK_WINDOW_CLOSABLE)) {
        nk_layout_row_begin(ctx, NK_STATIC, 256, 2);
        nk_layout_row_push(ctx, (370-150)/2);
        nk_spacer(ctx);

        struct nk_image chillygb_logo = TextureToNuklear(logo);
        nk_layout_row_push(ctx, 150);
        nk_image(ctx, chillygb_logo);
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_spacer(ctx);
        nk_label(ctx, "ChillyGB", NK_TEXT_CENTERED);
        nk_label(ctx, version, NK_TEXT_CENTERED);
        nk_spacer(ctx);
        nk_label(ctx, "By AuroraViola", NK_TEXT_CENTERED);
        nk_spacer(ctx);
        nk_label(ctx, "ChillyGB is licensed under the",  NK_TEXT_CENTERED);
        nk_label(ctx, "GNU General Public License v3.0.",  NK_TEXT_CENTERED);
    }
    nk_end(ctx);
}

void pause_game() {
    if (emulator_mode != GAME) return;
    emulator_mode = MENU;
    PauseAudioStream(audio.ch1.stream);
    PauseAudioStream(audio.ch2.stream);
    PauseAudioStream(audio.ch3.stream);
    PauseAudioStream(audio.ch4.stream);
}

void draw_display() {
    if (set.integer_scaling) {
        DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, display.width, display.height},
                       (Rectangle) {(GetScreenWidth() - (160 * scale_integer)) * 0.5f,
                                    (GetScreenHeight() - (144 * scale_integer)) * 0.5f,
                                    160 * scale_integer, 144 * scale_integer}, (Vector2) {0, 0}, 0.0f,
                       WHITE);
    }
    else {
        DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                       (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                    (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                    (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
    }
}

void update_frame() {
    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();
        if ((int) droppedFiles.count == 1) {
            if (game_started)
                save_game(&c.cart, rom_name);
            load_cartridge(droppedFiles.paths[0]);
        }
        UnloadDroppedFiles(droppedFiles);
    }
    scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
    scale_integer = MIN((int) GetScreenWidth() / 160, (int) GetScreenHeight() / 144);
    switch (emulator_mode) {
        case MENU:
            UpdateNuklear(ctx);
            DrawNavBar();
            #ifdef CUSTOM_OPEN_DIALOG
            DrawFileManager(ctx);
            #endif
            if (show_settings) {
                draw_settings_window();
            }
            if (nk_window_is_hidden(ctx, "ctx-settings")) {
                show_settings = false;
            }

            if (show_about) {
                draw_about_window();
            }
            if (nk_window_is_hidden(ctx, "ctx-about"))
                show_about = false;
            
            if (IsKeyPressed(KEY_ESCAPE) && game_started) {
                emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            BeginDrawing();
                ClearBackground(BLACK);
                BeginShaderMode(shaders[set.selected_effect]);
                    draw_display();
                    if (!game_started) {
                        float fontsize = (set.integer_scaling) ? (7 * scale_integer) : (7 * scale);
                        int center = MeasureText("Drop a Game Boy ROM to start playing", fontsize);
                        DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, set.palettes[set.selected_palette].colors[3]);
                    }
                EndShaderMode();
                DrawNuklear(ctx);
            DrawText(debug_text, 50, 50, 25, WHITE);

            EndDrawing();
            if (game_started)
                SetWindowTitle("ChillyGB - Paused");
            break;
        case GAME:
            timer1.timer_global = 0;
            while (timer1.timer_global < ((c.double_speed ? 139810 : 69905) * joypad1.fast_forward)) {
                execute(&c);
                Update_Audio(&c);
            }
            if (video.draw_screen) {
                if (IsKeyPressed(KEY_ESCAPE) ) {
                    save_game(&c.cart, rom_name);
                    pause_game();
                }
                if (IsKeyPressed(set.debugger_key)) {
                    emulator_mode = DEBUG;
                    PauseAudioStream(audio.ch1.stream);
                    PauseAudioStream(audio.ch2.stream);
                    PauseAudioStream(audio.ch3.stream);
                    PauseAudioStream(audio.ch4.stream);
                }

                if (IsKeyPressed(set.save_state_key)) {
                    save_state(&c, rom_name);
                }
                if (IsKeyPressed(set.load_state_key)) {
                    load_state(&c, rom_name);
                }
                video.draw_screen = false;
                if (video.is_on) {
                    if (c.is_color) {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                if (video.display[i][j] < 32) {
                                    uint8_t px_addr = video.display[i][j] << 1;
                                    uint16_t rgb555_color = (video.bgp[px_addr | 1] << 8) | (video.bgp[px_addr]);
                                    pixels[i][j].r = (rgb555_color & 0x001f) << 3;
                                    pixels[i][j].g = (rgb555_color & 0x03e0) >> 2;
                                    pixels[i][j].b = (rgb555_color & 0x7c00) >> 7;
                                    pixels[i][j].a = 255;
                                } else {
                                    uint8_t px_addr = (video.display[i][j] << 1) & 0x3f;
                                    uint16_t rgb555_color = (video.obp[px_addr | 1] << 8) | (video.obp[px_addr]);
                                    pixels[i][j].r = (rgb555_color & 0x001f) << 3;
                                    pixels[i][j].g = (rgb555_color & 0x03e0) >> 2;
                                    pixels[i][j].b = (rgb555_color & 0x7c00) >> 7;
                                    pixels[i][j].a = 255;
                                }
                            }
                        }
                    }
                    else {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                            }
                        }
                    }
                }
                else {
                    if (c.is_color) {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = (Color) {255, 255, 255, 255};
                            }
                        }
                    }
                    else {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                            }
                        }
                    }
                }
                UpdateTexture(display, pixels);
                BeginDrawing();
                    ClearBackground(BLACK);
                    BeginShaderMode(shaders[set.selected_effect]);
                        draw_display();
                    EndShaderMode();
                EndDrawing();
                char str[80];
                speed_mult = ((float)(GetFPS())/60) * joypad1.fast_forward;
                sprintf(str, "ChillyGB - %d FPS - %.1fx", GetFPS(), speed_mult);
                SetWindowTitle(str);
            }
            break;

        case DEBUG:
            UpdateNuklear(ctx);

            decode_instructions(&c, instructions);

            if (nk_begin(ctx, "Registers", nk_rect(24, 24, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, TextFormat("AF: %04X", c.r.reg16[AF]), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("BC: %04X", c.r.reg16[BC]), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("DE: %04X", c.r.reg16[DE]), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("HL: %04X", c.r.reg16[HL]), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("SP: %04X", c.sp), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("PC: %04X", c.pc), NK_TEXT_CENTERED);
                nk_label(ctx, TextFormat("IME: %s", (c.ime) ? "ON" : "OFF"), NK_TEXT_CENTERED);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "STAT", nk_rect(186, 250, 160, 250),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_checkbox_label(ctx, "LY=LYC int", &video.lyc_select);
                nk_checkbox_flags_label(ctx, "Mode 2 int", (unsigned int *)(&video.mode_select), 4);
                nk_checkbox_flags_label(ctx, "Mode 1 int", (unsigned int *)(&video.mode_select), 2);
                nk_checkbox_flags_label(ctx, "Mode 0 int", (unsigned int *)(&video.mode_select), 1);
                nk_checkbox_label(ctx, "LY=LYC", &video.ly_eq_lyc);
                nk_label(ctx, TextFormat("Mode: %i", video.mode), NK_TEXT_ALIGN_LEFT);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "LCD", nk_rect(186, 24, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, TextFormat("LY: %02X", get_mem(&c, LY)), NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, TextFormat("LYC: %02X", get_mem(&c, LYC)), NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, TextFormat("SCX: %02X", video.scx), NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, TextFormat("SCY: %02X", video.scy), NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, TextFormat("WX: %02X", video.wx), NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, TextFormat("WY: %02X", video.wy), NK_TEXT_ALIGN_CENTERED);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "LCDC", nk_rect(24, 250, 160, 250),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_checkbox_label(ctx, "LCD", &video.is_on);
                nk_checkbox_label(ctx, "Win Map", &video.window_tilemap);
                nk_checkbox_label(ctx, "Win En", &video.window_enable);
                nk_checkbox_label(ctx, "Tiles", &video.bg_tiles);
                nk_checkbox_label(ctx, "Bg Map", &video.bg_tilemap);
                nk_checkbox_label(ctx, "Obj size", &video.obj_size);
                nk_checkbox_label(ctx, "Obj En", &video.obj_enable);
                nk_checkbox_label(ctx, "Bg En", &video.bg_enable);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Instructions", nk_rect(1000, 24, 424, 418),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                for (int i = 0; i < 30; i++) {
                    nk_label(ctx, instructions[i], NK_TEXT_LEFT);
                }
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Memory Viewer", nk_rect(1000, 444, 650, 300),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_begin(ctx, NK_STATIC, 20, 17);
                for (int i = 0; i < 4096; i++) {
                    nk_layout_row_push(ctx, 65);
                    nk_label(ctx, TextFormat("%03X0:", i), NK_TEXT_LEFT);
                    for (int j = 0; j < 16; j++) {
                        nk_layout_row_push(ctx, 30);
                        nk_label(ctx, TextFormat("%02X", get_mem(&c, (i << 4) | j)), NK_TEXT_LEFT);
                    }
                }
                nk_layout_row_end(ctx);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Stack Viewer", nk_rect(1426, 24, 224, 418),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                for (int i = 1; (c.sp + i -1) < 0xffff; i+=2) {
                    nk_label(ctx, TextFormat("%04X: %02X%02X", (c.sp + i - 1), c.memory[c.sp+i], c.memory[c.sp+i-1]), NK_TEXT_ALIGN_LEFT);
                }
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Interrupts", nk_rect(24, 502, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_static(ctx, 20, 25, 2);
                nk_label(ctx, "IF", NK_TEXT_ALIGN_LEFT);
                nk_label(ctx, "IE", NK_TEXT_ALIGN_LEFT);

                nk_layout_row_begin(ctx, NK_STATIC, 20, 3);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IF]), 16);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IE]), 16);
                nk_layout_row_push(ctx, 80);
                nk_label(ctx, "Joypad", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IF]), 8);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IE]), 8);
                nk_layout_row_push(ctx, 80);
                nk_label(ctx, "Serial", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IF]), 4);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IE]), 4);
                nk_layout_row_push(ctx, 80);
                nk_label(ctx, "Timer", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IF]), 2);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IE]), 2);
                nk_layout_row_push(ctx, 80);
                nk_label(ctx, "STAT", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IF]), 1);
                nk_layout_row_push(ctx, 25);
                nk_checkbox_flags_label(ctx, "", (unsigned int *)(&c.memory[IE]), 1);
                nk_layout_row_push(ctx, 80);
                nk_label(ctx, "VBlank", NK_TEXT_ALIGN_LEFT);
            }
            nk_end(ctx);

            if (IsKeyPressed(KEY_N)) {
                execute(&c);
                Update_Audio(&c);
            }

            if (IsKeyDown(KEY_C)) {
                for (int i = 0; i < ff_speed; i++) {
                    execute(&c);
                    Update_Audio(&c);
                }
            }

            if (IsKeyPressed(KEY_R)) {
                initialize_cpu_memory_no_bootrom(&c, &set);
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            if (video.is_on) {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                        if (i == video.scan_line) {
                            pixels[i][j].a = 127;
                            if (j == video.current_pixel)
                                pixels[i][j].a += 16;
                        }
                        if (i == c.memory[LYC])
                            pixels[i][j].r += 20;
                    }
                }
            }
            else {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                    }
                }
            }
            UpdateTexture(display, pixels);
            struct nk_image display_debug = TextureToNuklear(display);
            if (nk_begin(ctx, "Display", nk_rect(348, 24, 650, 600),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, nk_window_get_height(ctx)-56, 1);
                nk_image_color(ctx, display_debug, (struct nk_color){255,255,255,255});
            }
            nk_end(ctx);
            BeginDrawing();
            ClearBackground(BLACK);
            DrawNuklear(ctx);
            EndDrawing();
            SetWindowTitle("ChillyGB - Debug");
            break;
    }
}

int main(int argc, char **argv) {
    int n_ticks = 0;
    char *test_image_path;

    int arg_count;
    const char *short_opt = "i:t:h";
    struct option long_opt[] = {
            {"image",          required_argument, NULL, 'i'},
            {"ticks",          required_argument, NULL, 't'},
            {"help",          no_argument, NULL, 'h'},
            {NULL,            0,                 NULL, 0  }
    };

    while((arg_count = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch(arg_count) {
            case -1:
            case 0:
                break;

            case 'i':
                test_image_path = optarg;
                break;

            case 't':
                n_ticks = atoi(optarg);
                break;

            case 'h':
                printf("Usage: ChillyGB [ROM] [OPTIONS]\n");
                printf("  -h, --help                      Print this help and exit\n");
                printf("  -i filename, --image filename   The file path to the image for comparing the final result\n");
                printf("  -t, --ticks, n_ticks            Number of t-cycles to do before exiting the test\n");
                printf("\n");
                return(0);

            case ':':
            case '?':
                fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
                return(-2);

            default:
                fprintf(stderr, "%s: invalid option -- %c\n", argv[0], arg_count);
                return(-2);
        };
    };

    if (optind < argc) {
        load_settings();
        if (n_ticks > 0) {
            set.selected_gameboy = 0;
            set.bootrom_enabled = false;
        }
        load_cartridge(argv[optind]);
    }

    if (n_ticks > 0) {
        SetTraceLogLevel(LOG_ERROR);
        test_rom(&c, n_ticks);
        Image screenshot = take_debug_screenshot(pixels_screen);
        Image expected = LoadImage(test_image_path);
        export_screenshot(screenshot, rom_name);
        for (int i = 0; i < 144; i++) {
            for (int j = 0; j < 160; j++) {
                if (GetImageColor(screenshot, j, i).r != GetImageColor(expected, j, i).r) {
                    printf("Test FAILED\n");
                    printf("Reason: Image mismatch\n");
                    return 1;
                }
            }
        }
        printf("Test PASSED");
        return 0;
    }
    // Initialize Raylib and Nuklear
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetWindowIcon(LoadImage("res/icons/ChillyGB-128.png"));
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    ctx = InitNuklearEx(LoadFontEx(RES_DIR "fonts/UbuntuMono.ttf", 20, 0, 250), 20);
    ctx->style.checkbox.border = 2;
    memcpy(&tab_button_style, &ctx->style.button, sizeof(struct nk_style_button));
    memcpy(&tab_button_active_style, &ctx->style.button, sizeof(struct nk_style_button));
    tab_button_style.border = 0;
    tab_button_style.padding = nk_vec2(0,0);
    tab_button_style.normal.data.color = nk_rgba(0,0,0,0);

    tab_button_active_style.border = 0;
    tab_button_active_style.padding = nk_vec2(0,0);
    tab_button_active_style.normal.data.color.a = 191;

    memcpy(&button_dpad_style, &ctx->style.button, sizeof(struct nk_style_button));
    button_dpad_style.border = 0;
    button_dpad_style.active.data.color = nk_rgb(63,63,63);
    button_dpad_style.hover.data.color = nk_rgb(63,63,63);
    button_dpad_style.normal.data.color = nk_rgb(63,63,63);
    button_dpad_style.rounding = 8;
    button_dpad_style.text_normal = nk_rgb(96, 96, 96);
    button_dpad_style.text_active = nk_rgb(96, 96, 96);
    button_dpad_style.text_background = nk_rgb(96, 96, 96);
    button_dpad_style.text_hover = nk_rgb(96, 96, 96);

    memcpy(&button_dpad_pressed_style, &ctx->style.button, sizeof(struct nk_style_button));
    button_dpad_pressed_style.border = 0;
    button_dpad_pressed_style.active.data.color = nk_rgb(191,63,63);
    button_dpad_pressed_style.hover.data.color = nk_rgb(191,63,63);
    button_dpad_pressed_style.normal.data.color = nk_rgb(191,63,63);
    button_dpad_pressed_style.rounding = 8;

    memcpy(&button_buttons_style, &ctx->style.button, sizeof(struct nk_style_button));
    button_buttons_style.border = 0;
    button_buttons_style.padding = nk_vec2(-40, -40);
    button_buttons_style.active.data.color = nk_rgb(63,63,63);
    button_buttons_style.hover.data.color = nk_rgb(63,63,63);
    button_buttons_style.normal.data.color = nk_rgb(63,63,63);
    button_buttons_style.rounding = 40;
    button_buttons_style.text_normal = nk_rgb(96, 96, 96);
    button_buttons_style.text_active = nk_rgb(96, 96, 96);
    button_buttons_style.text_background = nk_rgb(96, 96, 96);
    button_buttons_style.text_hover = nk_rgb(96, 96, 96);

    memcpy(&button_buttons_pressed_style, &ctx->style.button, sizeof(struct nk_style_button));
    button_buttons_pressed_style.border = 0;
    button_buttons_pressed_style.padding = nk_vec2(-40, -40);
    button_buttons_pressed_style.active.data.color = nk_rgb(191,63,63);
    button_buttons_pressed_style.hover.data.color = nk_rgb(191,63,63);
    button_buttons_pressed_style.normal.data.color = nk_rgb(191,63,63);
    button_buttons_pressed_style.rounding = 40;

    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color) {255, 255, 255, 255};
    display_image = (Image){
            .data = pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    display = LoadTextureFromImage(display_image);
    logo_image = LoadImage(RES_DIR "icons/ChillyGB-256.png");
    logo = LoadTextureFromImage(logo_image);

    shaders[SHADER_DEFAULT] = LoadShader(0, 0);
    shaders[SHADER_PIXEL_GRID] = LoadShader(0, "res/shaders/pixel_grid.fs");

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    joypad1.fast_forward = 1;

    #if defined(PLATFORM_WEB)
    EM_ASM(
        FS.mkdir('/file_picker_uploads');
        // Make a directory other than '/'
        FS.mkdir('/saves');
        // Then mount with IDBFS type
        FS.mount(IDBFS, {autoPersist:true}, '/saves');

        // Then sync
        FS.syncfs(true, function (err) {
            console.log(err);
            ccall("load_settings", "v");
        });
    );
    emscripten_set_main_loop(update_frame, 0, 1);
    #else
    load_settings();
    //memcpy(&set_prev, &set, sizeof(settings));
    while(!WindowShouldClose() && !exited) {
        update_frame();
    }
    #endif

    save_game(&c.cart, rom_name);
    UnloadTexture(display);
    UnloadTexture(logo);
    UnloadNuklear(ctx);
    CloseWindow();

    return 0;
}
