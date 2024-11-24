#include "../includes/ui.h"
#include "../includes/apu.h"
#include "../includes/open_dialog.h"
#include "../includes/cpu.h"
#include "../includes/cheats.h"
#include "../includes/cartridge.h"
#include "../includes/savestates.h"
#include "../includes/debug.h"
#include "../includes/input.h"
#include "../includes/camera.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#if PLATFORM_NX
#define RES_DIR "romfs:/"
#else
#define RES_DIR "res/"
#endif

#define MIN(a, b) ((a)<(b)? (a) : (b))

Ui ui;

#if defined(PLATFORM_WEB)
bool in_game() {
    return ui.game_started;
}
#endif

void initialize_ui() {
    // Initialize Raylib and Nuklear
    #ifndef PLATFORM_WEB
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    #endif
    InitWindow(160*4, 144*4, "ChillyGB");
    SetWindowIcon(LoadImage("res/icons/ChillyGB-128.png"));
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    ui.ctx = InitNuklearEx(LoadFontEx(RES_DIR "fonts/UbuntuMono.ttf", 20, 0, 250), 20);
    ui.ctx->style.checkbox.border = 2;
    memcpy(&ui.style.tab_button, &ui.ctx->style.button, sizeof(struct nk_style_button));
    memcpy(&ui.style.tab_button_active, &ui.ctx->style.button, sizeof(struct nk_style_button));
    ui.style.tab_button.border = 0;
    ui.style.tab_button.padding = nk_vec2(0,0);
    ui.style.tab_button.normal.data.color = nk_rgba(0,0,0,0);

    ui.style.tab_button_active.border = 0;
    ui.style.tab_button_active.padding = nk_vec2(0,0);
    ui.style.tab_button_active.normal.data.color.a = 191;

    memcpy(&ui.style.button_dpad, &ui.ctx->style.button, sizeof(struct nk_style_button));
    ui.style.button_dpad.border = 0;
    ui.style.button_dpad.active.data.color = nk_rgb(63,63,63);
    ui.style.button_dpad.hover.data.color = nk_rgb(63,63,63);
    ui.style.button_dpad.normal.data.color = nk_rgb(63,63,63);
    ui.style.button_dpad.rounding = 8;
    ui.style.button_dpad.text_normal = nk_rgb(96, 96, 96);
    ui.style.button_dpad.text_active = nk_rgb(96, 96, 96);
    ui.style.button_dpad.text_background = nk_rgb(96, 96, 96);
    ui.style.button_dpad.text_hover = nk_rgb(96, 96, 96);

    memcpy(&ui.style.button_dpad_pressed, &ui.ctx->style.button, sizeof(struct nk_style_button));
    ui.style.button_dpad_pressed.border = 0;
    ui.style.button_dpad_pressed.active.data.color = nk_rgb(191,63,63);
    ui.style.button_dpad_pressed.hover.data.color = nk_rgb(191,63,63);
    ui.style.button_dpad_pressed.normal.data.color = nk_rgb(191,63,63);
    ui.style.button_dpad_pressed.rounding = 8;

    memcpy(&ui.style.button_buttons, &ui.ctx->style.button, sizeof(struct nk_style_button));
    ui.style.button_buttons.border = 0;
    ui.style.button_buttons.padding = nk_vec2(-40, -40);
    ui.style.button_buttons.active.data.color = nk_rgb(63,63,63);
    ui.style.button_buttons.hover.data.color = nk_rgb(63,63,63);
    ui.style.button_buttons.normal.data.color = nk_rgb(63,63,63);
    ui.style.button_buttons.rounding = 40;
    ui.style.button_buttons.text_normal = nk_rgb(96, 96, 96);
    ui.style.button_buttons.text_active = nk_rgb(96, 96, 96);
    ui.style.button_buttons.text_background = nk_rgb(96, 96, 96);
    ui.style.button_buttons.text_hover = nk_rgb(96, 96, 96);

    memcpy(&ui.style.button_buttons_pressed, &ui.ctx->style.button, sizeof(struct nk_style_button));
    ui.style.button_buttons_pressed.border = 0;
    ui.style.button_buttons_pressed.padding = nk_vec2(-40, -40);
    ui.style.button_buttons_pressed.active.data.color = nk_rgb(191,63,63);
    ui.style.button_buttons_pressed.hover.data.color = nk_rgb(191,63,63);
    ui.style.button_buttons_pressed.normal.data.color = nk_rgb(191,63,63);
    ui.style.button_buttons_pressed.rounding = 40;

    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            ui.display.pixels[i][j] = (Color) {255, 255, 255, 255};
    ui.display.image = (Image){
            .data = ui.display.pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    ui.display.texture = LoadTextureFromImage(ui.display.image);
    ui.about.logo_image = LoadImage(RES_DIR "icons/ChillyGB-256.png");
    ui.about.logo_texture = LoadTextureFromImage(ui.about.logo_image);
    strcpy(ui.about.version, "v0.3.1");

    ui.shaders[SHADER_DEFAULT] = LoadShader(0, 0);
    ui.shaders[SHADER_PIXEL_GRID] = LoadShader(0, "res/shaders/pixel_grid.fs");
    ui.shaders[SHADER_COLOR_CORRECTION] = LoadShader(0, "res/shaders/color-correction.fs");

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();
}

void deinitialize_ui(cpu *c) {
    if (c->cart.mbc == POCKET_CAMERA)
        stop_camera();

    save_game(&c->cart, ui.rom_name);
    UnloadTexture(ui.display.texture);
    UnloadTexture(ui.about.logo_texture);
    UnloadNuklear(ui.ctx);
    CloseWindow();
}

void load_cartridge(cpu *c, char *path) {
    strcpy(ui.rom_name, path);
    if(!load_game(&c->cart, ui.rom_name)){
        fprintf(stderr, "Failed to open game: %s\n", ui.rom_name);
        return;
    }
    load_cheats(ui.rom_name);
    if (load_bootrom(&c->bootrom) && set.bootrom_enabled)
        initialize_cpu_memory(c, &set);
    else
        initialize_cpu_memory_no_bootrom(c, &set);
    ui.emulator_mode = GAME;
    ui.game_started = true;
    ResumeAudioStream(audio.ch1.stream);
    ResumeAudioStream(audio.ch2.stream);
    ResumeAudioStream(audio.ch3.stream);
    ResumeAudioStream(audio.ch4.stream);
}

void DrawNavBar(cpu *c) {
    if (nk_begin(ui.ctx, "Overview", nk_rect(0, 0, GetScreenWidth(), 35), 0)) {
        nk_menubar_begin(ui.ctx);
        nk_layout_row_begin(ui.ctx, NK_STATIC, 25, 5);
        nk_layout_row_push(ui.ctx, 60);
        if (nk_menu_begin_label(ui.ctx, "File", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ui.ctx, 25, 1);
            if (nk_menu_item_label(ui.ctx, "Load ROM", NK_TEXT_LEFT)) {
                char *path = do_open_rom_dialog(false);
                if (path != NULL) {
                    load_cartridge(c, path);
                }
            }
            if (nk_menu_item_label(ui.ctx, "Save SRAM", NK_TEXT_LEFT)) {
                if (ui.game_started) {
                    save_game(&c->cart, ui.rom_name);
                }
            }
            if (nk_menu_item_label(ui.ctx, "Load state", NK_TEXT_LEFT)) {
                if (ui.game_started)
                    load_state(c, ui.rom_name);
            }
            if (nk_menu_item_label(ui.ctx, "Save state", NK_TEXT_LEFT)) {
                if (ui.game_started)
                    save_state(c, ui.rom_name);
            }
#ifndef PLATFORM_WEB
            if (nk_menu_item_label(ui.ctx, "Exit", NK_TEXT_LEFT)) {
                ui.exited = true;
            }
#endif
            nk_menu_end(ui.ctx);
        }

        nk_layout_row_push(ui.ctx, 120);
        if (nk_menu_begin_label(ui.ctx, "Emulation", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ui.ctx, 25, 1);
            if (nk_menu_item_label(ui.ctx, "Reset", NK_TEXT_LEFT)) {
                if (ui.game_started) {
                    if (load_bootrom(&c->bootrom) && set.bootrom_enabled)
                        initialize_cpu_memory(c, &set);
                    else
                        initialize_cpu_memory_no_bootrom(c, &set);
                    ui.emulator_mode = GAME;
                    ResumeAudioStream(audio.ch1.stream);
                    ResumeAudioStream(audio.ch2.stream);
                    ResumeAudioStream(audio.ch3.stream);
                    ResumeAudioStream(audio.ch4.stream);
                }
            }
            if (nk_menu_item_label(ui.ctx, "Stop", NK_TEXT_LEFT)) {
                if (ui.game_started) {
                    ui.game_started = false;
                    c->is_color = false;
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            video.display[i][j] = 0;
                        }
                    }
                }
            }
            nk_menu_end(ui.ctx);
        }
        nk_layout_row_push(ui.ctx, 70);
        if (nk_menu_begin_label(ui.ctx, "Tools", NK_TEXT_LEFT, nk_vec2(230, 200))) {
            nk_layout_row_dynamic(ui.ctx, 25, 1);
            if (nk_menu_item_label(ui.ctx, "Debugger", NK_TEXT_LEFT)) {
                if (ui.game_started) {
                    ui.emulator_mode = DEBUG;
                }
            }
            if (nk_menu_item_label(ui.ctx, "Cheats", NK_TEXT_LEFT)) {
                ui.cheats.show = true;
            }
            if (nk_menu_item_label(ui.ctx, "Settings", NK_TEXT_LEFT)) {
                if (ui.settings.show == false) {
                    memcpy(&set_prev, &set, sizeof(settings));
                }
                ui.settings.show = true;
            }
            nk_menu_end(ui.ctx);
        }
        nk_layout_row_push(ui.ctx, 60);
        if (nk_menu_begin_label(ui.ctx, "Help", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            nk_layout_row_dynamic(ui.ctx, 25, 1);
            if (nk_menu_item_label(ui.ctx, "Github", NK_TEXT_LEFT)) {
                #ifdef _WIN32
                system("start https://github.com/AuroraViola/ChillyGB");
                #elif __APPLE__
                system("open https://github.com/AuroraViola/ChillyGB");
                #elif __linux__
                system("xdg-open https://github.com/AuroraViola/ChillyGB");
                #elif defined(PLATFORM_WEB)
                EM_ASM(
                    window.open("https://github.com/AuroraViola/ChillyGB")?.focus();
                );
                #else
                    printf("Failed to open GitHub link: Unsupported OS\n");
                #endif
            }
            if (nk_menu_item_label(ui.ctx, "About", NK_TEXT_LEFT)) {
                ui.about.show = true;
            }
            nk_menu_end(ui.ctx);
        }
        nk_menubar_end(ui.ctx);
    }
    nk_end(ui.ctx);
}

void draw_input_editor(const char *text, KeyboardKey *key_variable, int option_number) {
    char input_value[15];
    nk_label(ui.ctx, text, NK_TEXT_ALIGN_RIGHT|NK_TEXT_ALIGN_MIDDLE);
    if (ui.settings.is_selected_input && option_number == ui.settings.selected_input) {
        sprintf(input_value, "...");
        int key_pressed = GetKeyPressed();
        if (key_pressed != 0) {
            if (key_pressed != KEY_ESCAPE)
                *key_variable = key_pressed;
            ui.settings.is_selected_input = false;
        }
    }
    else {
        char key_name[15];
        convert_key(key_name, *key_variable);
        sprintf(input_value, "%s", key_name);
    }
    if (nk_button_label(ui.ctx, input_value)) {
        ui.settings.selected_input = option_number;
        ui.settings.is_selected_input = true;
    }
}

void draw_settings_window() {
    if(nk_begin_titled(ui.ctx, "ctx-settings","Settings", nk_rect(20, 60, 550, 505),
                       NK_WINDOW_MOVABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ui.ctx, 30, 4);
        if (nk_button_label_styled(ui.ctx, (ui.settings.view != SET_EMU) ? &ui.style.tab_button : &ui.style.tab_button_active, "Emulation"))
            ui.settings.view = SET_EMU;
        if (nk_button_label_styled(ui.ctx, (ui.settings.view != SET_AUDIO) ? &ui.style.tab_button : &ui.style.tab_button_active, "Audio"))
            ui.settings.view = SET_AUDIO;
        if (nk_button_label_styled(ui.ctx, (ui.settings.view != SET_VIDEO) ? &ui.style.tab_button : &ui.style.tab_button_active, "Video"))
            ui.settings.view = SET_VIDEO;
        if (nk_button_label_styled(ui.ctx, (ui.settings.view != SET_INPUT) ? &ui.style.tab_button : &ui.style.tab_button_active, "Input"))
            ui.settings.view = SET_INPUT;

        struct nk_vec2 size = {280, 200};
        nk_layout_row_dynamic(ui.ctx, 370, 1);
        if (nk_group_begin(ui.ctx, "settings_view", 0)) {
            switch (ui.settings.view) {
                case SET_EMU:
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_label(ui.ctx, "Emulated Model:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_combobox_string(ui.ctx, "Game Boy (DMG)\0Game Boy Color (CGB)", &set.selected_gameboy, 2, 20, size);
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_label(ui.ctx, "DMG bootrom path:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    nk_edit_string_zero_terminated(ui.ctx, NK_EDIT_FIELD, set.bootrom_path_dmg, 255, nk_filter_default);
                    if (nk_button_label(ui.ctx, "...")) {
                        char *path = do_open_rom_dialog(true);
                        if (path != NULL) {
                            strcpy(set.bootrom_path_dmg, path);
                        }
                    }
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_label(ui.ctx, "CGB bootrom path:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    nk_edit_string_zero_terminated(ui.ctx, NK_EDIT_FIELD, set.bootrom_path_cgb, 255, nk_filter_default);
                    if (nk_button_label(ui.ctx, "...")) {
                        char *path = do_open_rom_dialog(true);
                        if (path != NULL) {
                            strcpy(set.bootrom_path_cgb, path);
                        }
                    }
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_checkbox_label(ui.ctx, "Enable Bootrom", &set.bootrom_enabled);
                    nk_checkbox_label(ui.ctx, "Sync RTC to gameboy clock (accurate)", &set.accurate_rtc);
                    break;
                case SET_AUDIO:
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_label(ui.ctx, "Sound Volume", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    set.volume = nk_slide_int(ui.ctx, 0, set.volume, 100, 1);
                    char str[6];
                    sprintf(str, "%i%%", set.volume);
                    nk_label(ui.ctx, str, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_checkbox_label(ui.ctx, "Channel 1", &set.ch_on[0]);
                    nk_checkbox_label(ui.ctx, "Channel 2", &set.ch_on[1]);
                    nk_checkbox_label(ui.ctx, "Channel 3", &set.ch_on[2]);
                    nk_checkbox_label(ui.ctx, "Channel 4", &set.ch_on[3]);
                    break;
                case SET_VIDEO:
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_label(ui.ctx, "DMG Palette:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);

                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    int comboxes_len = 0;
                    for (int i = 0; i < set.palettes_size; i++) {
                        stpcpy(ui.settings.comboxes + comboxes_len, set.palettes[i].name);
                        comboxes_len += strlen(set.palettes[i].name)+1;
                    }
                    nk_combobox_string(ui.ctx, ui.settings.comboxes, &set.selected_palette, set.palettes_size, 20, size);
                    if (nk_button_label(ui.ctx, "Add Palette")) {
                        if (set.palettes_size < 100) {
                            strcpy(set.palettes[set.palettes_size].name, "Custom Palette");
                            memcpy(set.palettes[set.palettes_size].colors, set.palettes[set.selected_palette].colors, 4*sizeof(Color));
                            set.selected_palette = set.palettes_size;
                            set.palettes_size++;
                        }
                    }
                    nk_spacer(ui.ctx);
                    if (set.selected_palette > 4) {
                        if (nk_button_label(ui.ctx, "Remove Palette")) {
                            for (int i = set.selected_palette; i < set.palettes_size; i++) {
                                strcpy(set.palettes[i].name, set.palettes[i+1].name);
                                memcpy(set.palettes[i].colors, set.palettes[i+1].colors, 4*sizeof(Color));
                            }
                            set.palettes_size--;
                            if (set.selected_palette == set.palettes_size)
                                set.selected_palette--;
                        }
                    }

                    nk_layout_row_dynamic(ui.ctx, 60, 4);
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
                        if (nk_button_color(ui.ctx, colors[i])) {
                            ui.settings.palette_color_selected = i;
                        }
                    }

                    if (set.selected_palette > 4) {
                        char rgb_value[6];
                        // Red slider
                        nk_layout_row_begin(ui.ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ui.ctx, 20);
                        nk_label(ui.ctx, "R:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ui.ctx, 445);
                        set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].r = nk_slide_int(ui.ctx, 0, set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].r, 255, 1);
                        nk_layout_row_push(ui.ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].r);
                        nk_label(ui.ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ui.ctx);

                        // Green slider
                        nk_layout_row_begin(ui.ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ui.ctx, 20);
                        nk_label(ui.ctx, "G:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ui.ctx, 445);
                        set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].g = nk_slide_int(ui.ctx, 0, set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].g, 255, 1);
                        nk_layout_row_push(ui.ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].g);
                        nk_label(ui.ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ui.ctx);

                        // Blue slider
                        nk_layout_row_begin(ui.ctx, NK_STATIC, 30, 3);
                        nk_layout_row_push(ui.ctx, 20);
                        nk_label(ui.ctx, "B:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_push(ui.ctx, 445);
                        set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].b = nk_slide_int(ui.ctx, 0, set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].b, 255, 1);
                        nk_layout_row_push(ui.ctx, 30);
                        sprintf(rgb_value, "%i", set.palettes[set.selected_palette].colors[ui.settings.palette_color_selected].b);
                        nk_label(ui.ctx, rgb_value, NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                        nk_layout_row_end(ui.ctx);
                    }

                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    nk_label(ui.ctx, "Video effect:", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_combobox_string(ui.ctx, "None\0Pixel grid", &set.selected_effect, 2, 20, size);
                    nk_layout_row_dynamic(ui.ctx, 30, 1);
                    nk_checkbox_label(ui.ctx, "Integer Scaling", &set.integer_scaling);
                    //nk_checkbox_label(ui.ui.ctx, "Frame Blending", &set.frame_blending);
                    nk_checkbox_label(ui.ctx, "GBC Color Correction", &set.color_correction);
                    break;
                case SET_INPUT:
                    nk_layout_row_dynamic(ui.ctx, 30, 3);
                    if (nk_button_label_styled(ui.ctx, (ui.settings.input_view != INPUT_GAMEPAD) ? &ui.style.tab_button : &ui.style.tab_button_active, "Gamepad"))
                        ui.settings.input_view = INPUT_GAMEPAD;
                    if (nk_button_label_styled(ui.ctx, (ui.settings.input_view != INPUT_ADVANCED) ? &ui.style.tab_button : &ui.style.tab_button_active, "Advanced"))
                        ui.settings.input_view = INPUT_ADVANCED;
                    if (nk_button_label_styled(ui.ctx, (ui.settings.input_view != INPUT_MOTION) ? &ui.style.tab_button : &ui.style.tab_button_active, "Motions"))
                        ui.settings.is_selected_input = INPUT_MOTION;
                    nk_layout_row_dynamic(ui.ctx, 150, 1);
                    if (nk_group_begin(ui.ctx, "key_management", 0)) {
                        switch (ui.settings.input_view) {
                            case INPUT_GAMEPAD:
                                nk_layout_row_dynamic(ui.ctx, 30, 4);
                                for (int i = 0; i < 8; i++) {
                                    draw_input_editor(keys_names[ui.settings.key_order[i]], &set.keyboard_keys[ui.settings.key_order[i]], i);
                                }
                                break;
                            case INPUT_ADVANCED:
                                nk_layout_row_dynamic(ui.ctx, 30, 4);
                                draw_input_editor("Save state:", &set.save_state_key, 11);
                                draw_input_editor("Fast forward:", &set.fast_forward_key, 10);
                                //draw_input_editor("Rewind:", &set.rewind_key, 12);
                                draw_input_editor("Load state:", &set.load_state_key, 13);
                                draw_input_editor("Debugger:", &set.debugger_key, 14);
                                break;
                            case INPUT_MOTION:
                                nk_layout_row_dynamic(ui.ctx, 30, 1);
                                nk_label(ui.ctx, "Motion control style", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                                nk_combobox_string(ui.ctx, "Gamepad axis\0Mouse pointer", &set.motion_style, 2, 20, size);

                                break;
                        }
                        nk_group_end(ui.ctx);
                    }
                    nk_layout_row_begin(ui.ctx, NK_STATIC, 150, 2);
                    nk_layout_row_push(ui.ctx, 120);
                    nk_spacer(ui.ctx);
                    nk_layout_row_push(ui.ctx, 300);
                    if (nk_group_begin(ui.ctx, "gamepad_view", 0)) {
                        nk_layout_space_begin(ui.ctx, NK_STATIC, 130, 10);
                        nk_layout_space_push(ui.ctx, nk_rect(15, 45, 60, 30));
                        nk_button_symbol_styled(ui.ctx, &ui.style.button_dpad, NK_SYMBOL_NONE);
                        nk_layout_space_push(ui.ctx, nk_rect(30, 30, 30, 60));
                        nk_button_symbol_styled(ui.ctx, &ui.style.button_dpad, NK_SYMBOL_NONE);
                        nk_layout_space_push(ui.ctx, nk_rect(0, 45, 30, 30));
                        nk_button_symbol_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[DPAD_LEFT])) ? &ui.style.button_dpad_pressed : &ui.style.button_dpad,NK_SYMBOL_TRIANGLE_LEFT);
                        nk_layout_space_push(ui.ctx, nk_rect(60, 45, 30, 30));
                        nk_button_symbol_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[DPAD_RIGHT])) ? &ui.style.button_dpad_pressed : &ui.style.button_dpad, NK_SYMBOL_TRIANGLE_RIGHT);
                        nk_layout_space_push(ui.ctx, nk_rect(30, 15, 30, 30));
                        nk_button_symbol_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[DPAD_UP])) ? &ui.style.button_dpad_pressed: &ui.style.button_dpad, NK_SYMBOL_TRIANGLE_UP);
                        nk_layout_space_push(ui.ctx, nk_rect(30, 75, 30, 30));
                        nk_button_symbol_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[DPAD_DOWN])) ? &ui.style.button_dpad_pressed: &ui.style.button_dpad, NK_SYMBOL_TRIANGLE_DOWN);
                        nk_layout_space_push(ui.ctx, nk_rect(105, 120, 30, 10));
                        nk_button_label_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[BUTTON_SELECT+4])) ? &ui.style.button_buttons_pressed: &ui.style.button_buttons, "");
                        nk_layout_space_push(ui.ctx, nk_rect(150, 120, 30, 10));
                        nk_button_label_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[BUTTON_START+4])) ? &ui.style.button_buttons_pressed: &ui.style.button_buttons, "");
                        nk_layout_space_push(ui.ctx, nk_rect(180, 50, 40, 40));
                        nk_button_label_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[BUTTON_B+4])) ? &ui.style.button_buttons_pressed: &ui.style.button_buttons, "B");
                        nk_layout_space_push(ui.ctx, nk_rect(240, 30, 40, 40));
                        nk_button_label_styled(ui.ctx, (IsKeyDown(set.keyboard_keys[BUTTON_A+4])) ? &ui.style.button_buttons_pressed: &ui.style.button_buttons, "A");
                        nk_layout_space_end(ui.ctx);
                        nk_group_end(ui.ctx);
                    }
                    nk_layout_row_end(ui.ctx);
                    break;
            }
            nk_group_end(ui.ctx);
        }
        nk_layout_row_dynamic(ui.ctx, 40, 4);
        nk_spacer(ui.ctx);
        nk_spacer(ui.ctx);
        if (nk_button_label(ui.ctx, "Cancel")) {
            ui.settings.show = false;
            ui.settings.is_selected_input = false;
            memcpy(&set, &set_prev, sizeof(settings));
        }
        if (nk_button_label(ui.ctx, "Apply")) {
            ui.settings.show = false;
            ui.settings.is_selected_input = false;
            save_settings();
            memcpy(&set_prev, &set, sizeof(settings));
        }
    }
    nk_end(ui.ctx);
}

void draw_about_window() {
    if(nk_begin_titled(ui.ctx, "ctx-about","About", nk_rect((GetScreenWidth()/2-200), (GetScreenHeight()/2-250), 400, 520),
                       NK_WINDOW_CLOSABLE)) {
        nk_layout_row_begin(ui.ctx, NK_STATIC, 256, 2);
        nk_layout_row_push(ui.ctx, (370-150)/2);
        nk_spacer(ui.ctx);

        struct nk_image chillygb_logo = TextureToNuklear(ui.about.logo_texture);
        nk_layout_row_push(ui.ctx, 150);
        nk_image(ui.ctx, chillygb_logo);
        nk_layout_row_end(ui.ctx);

        nk_layout_row_dynamic(ui.ctx, 20, 1);
        nk_spacer(ui.ctx);
        nk_label(ui.ctx, "ChillyGB", NK_TEXT_CENTERED);
        nk_label(ui.ctx, ui.about.version, NK_TEXT_CENTERED);
        nk_spacer(ui.ctx);
        nk_label(ui.ctx, "By AuroraViola", NK_TEXT_CENTERED);
        nk_spacer(ui.ctx);
        nk_label(ui.ctx, "ChillyGB is licensed under the",  NK_TEXT_CENTERED);
        nk_label(ui.ctx, "GNU General Public License v3.0.",  NK_TEXT_CENTERED);
    }
    nk_end(ui.ctx);
}

void draw_cheats_window() {
    if(nk_begin_titled(ui.ctx, "ctx-cheats","Cheats", nk_rect(20, 60, 550, 505),
                       NK_WINDOW_MOVABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ui.ctx, 30, 2);
        if (nk_button_label_styled(ui.ctx, (ui.cheats.view != CHEAT_GAMEGENIE) ? &ui.style.tab_button : &ui.style.tab_button_active, "GameGenie"))
            ui.cheats.view = CHEAT_GAMEGENIE;
        if (nk_button_label_styled(ui.ctx, (ui.cheats.view != CHEAT_GAMESHARK) ? &ui.style.tab_button : &ui.style.tab_button_active, "GameShark"))
            ui.cheats.view = CHEAT_GAMESHARK;

        nk_layout_row_dynamic(ui.ctx, 370, 1);
        if (nk_group_begin(ui.ctx, "cheats_view", 0)) {
            switch (ui.cheats.view) {
                case CHEAT_GAMEGENIE:
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    for (int i = 0; i < cheats.gameGenie_count; i++) {
                        nk_checkbox_label(ui.ctx, TextFormat("%X: %X => %X", cheats.gameGenie[i].address, cheats.gameGenie[i].old_data, cheats.gameGenie[i].new_data), &cheats.gameGenie[i].enabled);
                        if (nk_button_label(ui.ctx, "Delete cheat")) {
                            remove_gamegenie_cheat(i);
                        }
                    }
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    nk_edit_string_zero_terminated(ui.ctx, NK_EDIT_FIELD, cheats.gamegenie_code, 10, nk_filter_default);
                    if (nk_button_label(ui.ctx, "Add cheat")) {
                        if (ui.game_started)
                            add_gamegenie_cheat();
                    }
                    break;
                case CHEAT_GAMESHARK:
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    for (int i = 0; i < cheats.gameShark_count; i++) {
                        nk_checkbox_label(ui.ctx, TextFormat("%X-%X = %X", cheats.gameShark[i].sram_bank, cheats.gameShark[i].address, cheats.gameShark[i].new_data), &cheats.gameShark[i].enabled);
                        if (nk_button_label(ui.ctx, "Delete cheat")) {
                            remove_gameshark_cheat(i);
                        }
                    }
                    nk_layout_row_dynamic(ui.ctx, 30, 2);
                    nk_edit_string_zero_terminated(ui.ctx, NK_EDIT_FIELD, cheats.gameshark_code, 9, nk_filter_default);
                    if (nk_button_label(ui.ctx, "Add cheat")) {
                        if (ui.game_started)
                            add_gameshark_cheat();
                    }
                    break;
            }
            nk_group_end(ui.ctx);
        }
        nk_layout_row_dynamic(ui.ctx, 40, 4);
        nk_spacer(ui.ctx);
        nk_spacer(ui.ctx);
        if (nk_button_label(ui.ctx, "Cancel")) {
            ui.cheats.show = false;
        }
        if (nk_button_label(ui.ctx, "Save")) {
            ui.cheats.show = false;
            if (ui.game_started)
                save_cheats(ui.rom_name);
        }
    }
    nk_end(ui.ctx);
}

void draw_alert_window() {
    if(nk_begin_titled(ui.ctx, "ctx-alert","Warning", nk_rect((GetScreenWidth()/2-150), (GetScreenHeight()/2-70), 300, 140),
                       NK_WINDOW_MOVABLE|NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ui.ctx, 40, 1);
        nk_label(ui.ctx, "Illegal opcode", NK_TEXT_ALIGN_CENTERED|NK_TEXT_ALIGN_MIDDLE);
        nk_layout_row_dynamic(ui.ctx, 40, 4);
        nk_spacer(ui.ctx);
        nk_spacer(ui.ctx);
        nk_spacer(ui.ctx);
        if (nk_button_label(ui.ctx, "ok")) {
            ui.alert.show = false;
        }
    }
    nk_end(ui.ctx);
}

void pause_game() {
    if (ui.emulator_mode != GAME) return;
    ui.emulator_mode = MENU;
    PauseAudioStream(audio.ch1.stream);
    PauseAudioStream(audio.ch2.stream);
    PauseAudioStream(audio.ch3.stream);
    PauseAudioStream(audio.ch4.stream);
}

void draw_display(cpu *c) {
    BeginShaderMode(ui.shaders[set.selected_effect]);
    if (c->is_color && set.color_correction) {
        BeginShaderMode(ui.shaders[SHADER_COLOR_CORRECTION]);
    }
    if (set.integer_scaling) {
        DrawTexturePro(ui.display.texture, (Rectangle) {0.0f, 0.0f, ui.display.texture.width, ui.display.texture.height},
                       (Rectangle) {(GetScreenWidth() - (160 * ui.scale_integer)) / 2,
                                    (GetScreenHeight() - (144 * ui.scale_integer)) / 2,
                                    160 * ui.scale_integer, 144 * ui.scale_integer}, (Vector2) {0, 0}, 0.0f,
                       WHITE);
    }
    else {
        DrawTexturePro(ui.display.texture, (Rectangle) {0.0f, 0.0f, (float) ui.display.texture.width, (float) ui.display.texture.height},
                       (Rectangle) {(GetScreenWidth() - ((float) 160 * ui.scale)) / 2,
                                    (GetScreenHeight() - ((float) 144 * ui.scale)) / 2,
                                    (float) 160 * ui.scale, (float) 144 * ui.scale}, (Vector2) {0, 0}, 0.0f, WHITE);
    }
    if (c->is_color && set.color_correction) {
        EndShaderMode();
    }
    EndShaderMode();
    DrawText(debug_text, 0, 40, 20, RED);
}

void update_display_texture(cpu *c) {
    if (video.is_on) {
        if (c->is_color) {
            for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j++) {
                    uint16_t rgb555_color = video.display[i][j];
                    ui.display.pixels[i][j].r = (rgb555_color & 0x001f) << 3;
                    ui.display.pixels[i][j].g = (rgb555_color & 0x03e0) >> 2;
                    ui.display.pixels[i][j].b = (rgb555_color & 0x7c00) >> 7;
                    ui.display.pixels[i][j].a = 255;
                }
            }
        }
        else {
            for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j++) {
                    ui.display.pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                }
            }
        }
    }
    else {
        if (c->is_color) {
            for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j++) {
                    ui.display.pixels[i][j] = (Color) {255, 255, 255, 255};
                }
            }
        }
        else {
            for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j++) {
                    ui.display.pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                }
            }
        }
    }
}

void update_frame(cpu *c) {
    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();
        if ((int) droppedFiles.count == 1) {
            if (ui.game_started)
                save_game(&c->cart, ui.rom_name);
            load_cartridge(c, droppedFiles.paths[0]);
        }
        UnloadDroppedFiles(droppedFiles);
    }
    ui.scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
    ui.scale_integer = MIN((int) GetScreenWidth() / 160, (int) GetScreenHeight() / 144);
    switch (ui.emulator_mode) {
        case MENU:
            UpdateNuklear(ui.ctx);
            DrawNavBar(c);
            #ifdef CUSTOM_OPEN_DIALOG
            DrawFileManager(ui.ctx);
            #endif
            if (ui.settings.show) {
                draw_settings_window();
            }
            if (nk_window_is_hidden(ui.ctx, "ctx-settings")) {
                ui.settings.show = false;
            }

            if (ui.cheats.show) {
                draw_cheats_window();
            }
            if (nk_window_is_hidden(ui.ctx, "ctx-cheats"))
                ui.cheats.show = false;

            if (ui.alert.show) {
                draw_alert_window();
            }
            if (nk_window_is_hidden(ui.ctx, "ctx-alert"))
                ui.alert.show = false;

            if (ui.about.show) {
                draw_about_window();
            }
            if (nk_window_is_hidden(ui.ctx, "ctx-about"))
                ui.about.show = false;

            if ((IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) && ui.game_started ) {
                ui.emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            update_display_texture(c);
            UpdateTexture(ui.display.texture, ui.display.pixels);
            BeginDrawing();
            ClearBackground(BLACK);
            draw_display(c);
            if (!ui.game_started) {
                float fontsize = (set.integer_scaling) ? (7 * ui.scale_integer) : (7 * ui.scale);
                int center = MeasureText("Drop a Game Boy ROM to start playing", fontsize);
                DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, set.palettes[set.selected_palette].colors[3]);
            }
            DrawNuklear(ui.ctx);

            EndDrawing();
            if (ui.game_started)
                SetWindowTitle("ChillyGB - Paused");
            break;
        case GAME:
            timer1.timer_global = 0;
            while (timer1.timer_global < ((c->double_speed ? 139810 : 69905) * joypad1.fast_forward)) {
                if (!execute(c)) {
                    pause_game();
                    ui.alert.show = true;
                    break;
                }
                Update_Audio(c);
            }
            if (video.draw_screen) {
                if (IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
                    save_game(&c->cart, ui.rom_name);
                    pause_game();
                }
                if (IsKeyPressed(set.debugger_key)) {
                    ui.emulator_mode = DEBUG;
                    PauseAudioStream(audio.ch1.stream);
                    PauseAudioStream(audio.ch2.stream);
                    PauseAudioStream(audio.ch3.stream);
                    PauseAudioStream(audio.ch4.stream);
                }

                if (IsKeyPressed(set.save_state_key)) {
                    save_state(c, ui.rom_name);
                }
                if (IsKeyPressed(set.load_state_key)) {
                    load_state(c, ui.rom_name);
                }
                video.draw_screen = false;
                update_display_texture(c);
                UpdateTexture(ui.display.texture, ui.display.pixels);
                BeginDrawing();
                ClearBackground(BLACK);
                draw_display(c);
                EndDrawing();
                char str[80];
                speed_mult = ((float)(GetFPS())/60) * joypad1.fast_forward;
                sprintf(str, "ChillyGB - %d FPS - %.1fx", GetFPS(), speed_mult);
                SetWindowTitle(str);
            }
            break;

        case DEBUG:
            UpdateNuklear(ui.ctx);

            decode_instructions(c, ui.debugger.instructions);

            if (nk_begin(ui.ctx, "Registers", nk_rect(24, 24, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                nk_label(ui.ctx, TextFormat("AF: %04X", c->r.reg16[AF]), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("BC: %04X", c->r.reg16[BC]), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("DE: %04X", c->r.reg16[DE]), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("HL: %04X", c->r.reg16[HL]), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("SP: %04X", c->sp), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("PC: %04X", c->pc), NK_TEXT_CENTERED);
                nk_label(ui.ctx, TextFormat("IME: %s", (c->ime) ? "ON" : "OFF"), NK_TEXT_CENTERED);
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "STAT", nk_rect(186, 250, 160, 250),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                nk_checkbox_label(ui.ctx, "LY=LYC int", &video.lyc_select);
                nk_checkbox_flags_label(ui.ctx, "Mode 2 int", (unsigned int *)(&video.mode_select), 4);
                nk_checkbox_flags_label(ui.ctx, "Mode 1 int", (unsigned int *)(&video.mode_select), 2);
                nk_checkbox_flags_label(ui.ctx, "Mode 0 int", (unsigned int *)(&video.mode_select), 1);
                nk_checkbox_label(ui.ctx, "LY=LYC", &video.ly_eq_lyc);
                nk_label(ui.ctx, TextFormat("Mode: %i", video.mode), NK_TEXT_ALIGN_LEFT);
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "LCD", nk_rect(186, 24, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                nk_label(ui.ctx, TextFormat("LY: %02X", get_mem(c, LY)), NK_TEXT_ALIGN_CENTERED);
                nk_label(ui.ctx, TextFormat("LYC: %02X", get_mem(c, LYC)), NK_TEXT_ALIGN_CENTERED);
                nk_label(ui.ctx, TextFormat("SCX: %02X", video.scx), NK_TEXT_ALIGN_CENTERED);
                nk_label(ui.ctx, TextFormat("SCY: %02X", video.scy), NK_TEXT_ALIGN_CENTERED);
                nk_label(ui.ctx, TextFormat("WX: %02X", video.wx), NK_TEXT_ALIGN_CENTERED);
                nk_label(ui.ctx, TextFormat("WY: %02X", video.wy), NK_TEXT_ALIGN_CENTERED);
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "LCDC", nk_rect(24, 250, 160, 250),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                nk_checkbox_label(ui.ctx, "LCD", &video.is_on);
                nk_checkbox_label(ui.ctx, "Win Map", &video.window_tilemap);
                nk_checkbox_label(ui.ctx, "Win En", &video.window_enable);
                nk_checkbox_label(ui.ctx, "Tiles", &video.bg_tiles);
                nk_checkbox_label(ui.ctx, "Bg Map", &video.bg_tilemap);
                nk_checkbox_label(ui.ctx, "Obj Size", &video.obj_size);
                nk_checkbox_label(ui.ctx, "Obj En", &video.obj_enable);
                nk_checkbox_label(ui.ctx, "Bg En", &video.bg_enable);
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "Instructions", nk_rect(1000, 24, 424, 418),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                for (int i = 0; i < 30; i++) {
                    nk_label(ui.ctx, ui.debugger.instructions[i], NK_TEXT_LEFT);
                }
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "Memory Viewer", nk_rect(1000, 444, 650, 300),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_begin(ui.ctx, NK_STATIC, 20, 17);
                for (int i = 0; i < 4096; i++) {
                    nk_layout_row_push(ui.ctx, 65);
                    nk_label(ui.ctx, TextFormat("%03X0:", i), NK_TEXT_LEFT);
                    for (int j = 0; j < 16; j++) {
                        nk_layout_row_push(ui.ctx, 30);
                        nk_label(ui.ctx, TextFormat("%02X", get_mem(c, (i << 4) | j)), NK_TEXT_LEFT);
                    }
                }
                nk_layout_row_end(ui.ctx);
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "Stack Viewer", nk_rect(1426, 24, 224, 418),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, 20, 1);
                for (int i = 1; (c->sp + i -1) < 0xffff; i+=2) {
                    nk_label(ui.ctx, TextFormat("%04X: %02X%02X", (c->sp + i - 1), c->memory[c->sp+i], c->memory[c->sp+i-1]), NK_TEXT_ALIGN_LEFT);
                }
            }
            nk_end(ui.ctx);

            if (nk_begin(ui.ctx, "Interrupts", nk_rect(24, 502, 160, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_static(ui.ctx, 20, 25, 2);
                nk_label(ui.ctx, "IF", NK_TEXT_ALIGN_LEFT);
                nk_label(ui.ctx, "IE", NK_TEXT_ALIGN_LEFT);

                nk_layout_row_begin(ui.ctx, NK_STATIC, 20, 3);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IF]), 16);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IE]), 16);
                nk_layout_row_push(ui.ctx, 80);
                nk_label(ui.ctx, "Joypad", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IF]), 8);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IE]), 8);
                nk_layout_row_push(ui.ctx, 80);
                nk_label(ui.ctx, "Serial", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IF]), 4);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IE]), 4);
                nk_layout_row_push(ui.ctx, 80);
                nk_label(ui.ctx, "Timer", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IF]), 2);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IE]), 2);
                nk_layout_row_push(ui.ctx, 80);
                nk_label(ui.ctx, "STAT", NK_TEXT_ALIGN_LEFT);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IF]), 1);
                nk_layout_row_push(ui.ctx, 25);
                nk_checkbox_flags_label(ui.ctx, "", (unsigned int *)(&c->memory[IE]), 1);
                nk_layout_row_push(ui.ctx, 80);
                nk_label(ui.ctx, "VBlank", NK_TEXT_ALIGN_LEFT);
            }
            nk_end(ui.ctx);

            if (IsKeyPressed(KEY_N)) {
                execute(c);
                Update_Audio(c);
            }

            if (IsKeyDown(KEY_C)) {
                execute(c);
                Update_Audio(c);
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                ui.emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            if (c->is_color) {
                if (video.is_on) {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            uint16_t rgb555_color = video.display[i][j];
                            ui.display.pixels[i][j].r = (rgb555_color & 0x001f) << 3;
                            ui.display.pixels[i][j].g = (rgb555_color & 0x03e0) >> 2;
                            ui.display.pixels[i][j].b = (rgb555_color & 0x7c00) >> 7;
                            ui.display.pixels[i][j].a = 255;
                            if (i == video.scan_line) {
                                ui.display.pixels[i][j].a = 127;
                                if (j == video.current_pixel)
                                    ui.display.pixels[i][j].a += 16;
                            }
                            if (i == c->memory[LYC])
                                ui.display.pixels[i][j].r += 20;
                        }
                    }
                } else {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            ui.display.pixels[i][j] = (Color){255, 255, 255, 255};
                        }
                    }
                }
            }
            else {
                if (video.is_on) {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            ui.display.pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                            if (i == video.scan_line) {
                                ui.display.pixels[i][j].a = 127;
                                if (j == video.current_pixel)
                                    ui.display.pixels[i][j].a += 16;
                            }
                            if (i == c->memory[LYC])
                                ui.display.pixels[i][j].r += 20;

                        }
                    }
                } else {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            ui.display.pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                        }
                    }
                }
            }
            UpdateTexture(ui.display.texture, ui.display.pixels);
            struct nk_image display_debug = TextureToNuklear(ui.display.texture);
            if (nk_begin(ui.ctx, "Display", nk_rect(348, 24, 650, 600),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ui.ctx, nk_window_get_height(ui.ctx)-56, 1);
                nk_image_color(ui.ctx, display_debug, (struct nk_color){255,255,255,255});
            }
            nk_end(ui.ctx);
            BeginDrawing();
            ClearBackground(BLACK);
            DrawNuklear(ui.ctx);
            EndDrawing();
            SetWindowTitle("ChillyGB - Debug");
            break;
    }
}
