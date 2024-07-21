#include "raylib.h"
#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/settings.h"
#include "includes/apu.h"
#include "includes/timer.h"
#include "includes/debug.h"
#include "includes/input.h"
#include "includes/cartridge.h"
#include <stdio.h>
#include <string.h>

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "../raylib-nuklear/include/raylib-nuklear.h"

#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef enum EmuModes{
    MENU = 0,
    GAME,
    DEBUG,
}EmuModes;

static const char *palettes[] = {"ChillyGB", "SamePalette", "GrayScale", "Super Game Boy", "Realistic", "bgb"};

Color Palettes[6][5] = {
        {
            (Color) {185, 237, 186, 255},
            (Color) {118, 196, 123, 255},
            (Color) {49, 106, 64, 255},
            (Color) {10, 38, 16, 255},
            (Color) {200, 237, 186, 255}
        },
        {
            (Color) {198, 222, 140, 255},
            (Color) {132, 165, 99, 255},
            (Color) {57, 97, 57, 255},
            (Color) {8, 24, 16, 255},
            (Color) {198, 222, 140, 255}
        },
        {
            (Color) {255, 255, 255, 255},
            (Color) {176, 176, 176, 255},
            (Color) {104, 104, 104, 255},
            (Color) {0, 0, 0, 255},
            (Color) {255, 255, 255, 255}
        },
        {
            (Color) {255, 239, 206, 255},
            (Color) {222, 148, 74, 255},
            (Color) {173, 41, 33, 255},
            (Color) {49, 24, 82, 255},
            (Color) {255, 239, 206, 255}
        },
        {
            (Color) {117, 152, 51, 255},
            (Color) {88, 143, 81, 255},
            (Color) {59, 117, 96, 255},
            (Color) {46, 97, 90, 255},
            (Color) {148, 138, 4, 255}
        },
        {
            (Color) {224, 248, 208, 255},
            (Color) {136, 192, 112, 255},
            (Color) {52, 104, 86, 255},
            (Color) {8, 24, 32, 255},
            (Color) {224, 248, 208, 255}
        }
};

cpu c = {};
settings set = {};
bool exited = false;
bool game_started = false;
bool show_settings = false;
bool show_about = false;
char rom_name[256];
uint8_t emulator_mode = MENU;

void DrawNavBar(struct nk_context *ctx) {
    if (nk_begin(ctx, "Overview", nk_rect(0, 0, GetScreenWidth(), 35), 0)) {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Load ROM", NK_TEXT_LEFT)) {
            }
            if (nk_menu_item_label(ctx, "Save SRAM", NK_TEXT_LEFT)) {
                if (game_started) {
                    save_game(&c.cart, rom_name);
                }
            }
            if (nk_menu_item_label(ctx, "Load state", NK_TEXT_LEFT)) {
            }
            if (nk_menu_item_label(ctx, "Save state", NK_TEXT_LEFT)) {
            }
            if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
                exited = true;
            }
            nk_menu_end(ctx);
        }

        nk_layout_row_push(ctx, 120);
        if (nk_menu_begin_label(ctx, "Emulation", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Reset", NK_TEXT_LEFT)) {
                if (game_started) {
                    initialize_cpu_memory(&c, &set);
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
        if (nk_menu_begin_label(ctx, "Tools", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Debugger", NK_TEXT_LEFT)) {
                if (game_started) {
                    emulator_mode = DEBUG;
                }
            }
            if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT)) {
                show_settings = true;
            }
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "Help", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Github", NK_TEXT_LEFT)) {
                #ifdef _WIN32
                    system("start https://github.com/AuroraViola/ChillyGB");
                #elif __APPLE__
                    system("open https://github.com/AuroraViola/ChillyGB");
                #elif __linux__
                    system("xdg-open https://github.com/AuroraViola/ChillyGB");
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

int main(void) {
    load_settings(&set);

    // Initialize Raylib and Nuklear
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    struct nk_context *ctx = InitNuklearEx(LoadFontEx("res/fonts/UbuntuMono.ttf", 20, 0, 250), 20);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = Palettes[set.palette][4];
    Image display_image = {
            .data = pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    Texture2D display = LoadTextureFromImage(display_image);
    Image logo_image = LoadImage("res/icons/ChillyGB-256.png");
    Texture2D logo = LoadTextureFromImage(logo_image);
    float scale;

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    // Initialize Debug variables
    char instructions[30][50];
    debugtexts texts;
    int ff_speed = 1;

    while(!WindowShouldClose() && !exited) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if ((int) droppedFiles.count == 1) {
                save_game(&c.cart, rom_name);
                strcpy(rom_name, droppedFiles.paths[0]);
                load_game(&c.cart, rom_name);
                initialize_cpu_memory(&c, &set);
                emulator_mode = GAME;
                game_started = true;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }
            UnloadDroppedFiles(droppedFiles);
        }
        scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
        switch (emulator_mode) {
            case MENU:
                UpdateNuklear(ctx);
                DrawNavBar(ctx);

                if (show_settings && nk_begin_titled(ctx, "ctx-settings","Settings", nk_rect(24, 64, 400, 200),
                                                     NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_CLOSABLE)) {
                    nk_layout_row_dynamic(ctx, 30, 2);
                    nk_label(ctx, "Sound Volume", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    audio.volume = nk_slide_int(ctx, 0, audio.volume, 100, 1);
                    nk_label(ctx, "Palette", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    struct nk_vec2 size = {200, 100};
                    nk_combobox(ctx, palettes, 6, &set.palette, 20, size);
                    nk_label(ctx, "Original logo", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_checkbox_label(ctx, "", &set.custom_boot_logo);
                }
                if (nk_window_is_hidden(ctx, "ctx-settings"))
                    show_settings = false;

                if (show_about && nk_begin_titled(ctx, "ctx-about","About", nk_rect((GetScreenWidth()/2-200), (GetScreenHeight()/2-250), 400, 500),
                                                  NK_WINDOW_CLOSABLE)) {
                    nk_layout_row_begin(ctx, NK_STATIC, 256, 3);
                    /* padding */
                    nk_layout_row_push(ctx, (370-150)/2);
                    nk_label(ctx, "", NK_TEXT_LEFT);

                    /* header */
                    struct nk_image chillygb_logo = TextureToNuklear(logo);
                    nk_layout_row_push(ctx, 150);
                    nk_image(ctx, chillygb_logo);

                    /* padding */
                    nk_layout_row_push(ctx, (370-150)/2);
                    nk_label(ctx, "", NK_TEXT_LEFT);
                    nk_layout_row_end(ctx);
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, "",  NK_TEXT_CENTERED);
                    nk_label(ctx, "ChillyGB", NK_TEXT_CENTERED);
                    nk_label(ctx, "By AuroraViola", NK_TEXT_CENTERED);
                    nk_label(ctx, "",  NK_TEXT_CENTERED);
                    nk_label(ctx, "ChillyGB is licensed under the",  NK_TEXT_CENTERED);
                    nk_label(ctx, "GNU General Public License v3.0.",  NK_TEXT_CENTERED);
                }
                if (nk_window_is_hidden(ctx, "ctx-about"))
                    show_about = false;

                nk_end(ctx);
                if (IsKeyPressed(KEY_ESCAPE) && game_started) {
                    emulator_mode = GAME;
                    ResumeAudioStream(audio.ch1.stream);
                    ResumeAudioStream(audio.ch2.stream);
                    ResumeAudioStream(audio.ch3.stream);
                    ResumeAudioStream(audio.ch4.stream);
                }
                BeginDrawing();
                    ClearBackground(BLACK);
                    DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                                   (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                                (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                                (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
                    if (!game_started) {
                        float fontsize = 7 * scale;
                        int center = MeasureText("Drop a Game Boy ROM to start playing", fontsize);
                        DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, Palettes[set.palette][3]);
                    }
                    DrawNuklear(ctx);
                EndDrawing();
                if (game_started)
                    SetWindowTitle("ChillyGB - Paused");
                break;
            case GAME:
                execute(&c);
                Update_Audio(&c);

                if (video.draw_screen == true) {
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        emulator_mode = MENU;
                        PauseAudioStream(audio.ch1.stream);
                        PauseAudioStream(audio.ch2.stream);
                        PauseAudioStream(audio.ch3.stream);
                        PauseAudioStream(audio.ch4.stream);
                    }
                    if (IsKeyPressed(KEY_F3)) {
                        emulator_mode = DEBUG;
                        PauseAudioStream(audio.ch1.stream);
                        PauseAudioStream(audio.ch2.stream);
                        PauseAudioStream(audio.ch3.stream);
                        PauseAudioStream(audio.ch4.stream);
                    }
                    video.draw_screen = false;
                    if (video.is_on) {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = Palettes[set.palette][video.display[i][j]];
                            }
                        }
                    }
                    else {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = Palettes[set.palette][4];
                            }
                        }
                    }
                    UpdateTexture(display, pixels);
                    BeginDrawing();
                        ClearBackground(BLACK);
                        DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                                       (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                                    (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                                    (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
                    EndDrawing();
                    char str[80];
                    sprintf(str, "ChillyGB - %d FPS - %.1fx", GetFPS(), (float)(GetFPS())/60);
                    SetWindowTitle(str);
                }
                break;

            case DEBUG:
                UpdateNuklear(ctx);

                generate_texts(&c, &texts);
                decode_instructions(&c, instructions);

                if (nk_begin(ctx, "Registers", nk_rect(24, 24, 190, 224),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, texts.AFtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.BCtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.DEtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.HLtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.SPtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.PCtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.IMEtext, NK_TEXT_CENTERED);
                }
                nk_end(ctx);

                if (nk_begin(ctx, "PPU Info", nk_rect(24, 272, 190, 170),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, texts.LYtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.LYCtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.PPUMode, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.LCDCtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.STATtext, NK_TEXT_CENTERED);
                }
                nk_end(ctx);

                if (nk_begin(ctx, "Interrupts", nk_rect(24, 466, 190, 128),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, texts.IEtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.IFtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.IMEtext, NK_TEXT_CENTERED);
                }
                nk_end(ctx);

                if (nk_begin(ctx, "Fast Forward", nk_rect(24, 618, 190, 80),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 2);
                    nk_label(ctx, "Speed", NK_TEXT_CENTERED);
                    ff_speed = nk_slide_int(ctx, 1, ff_speed, 1000, 1);
                }
                nk_end(ctx);

                if (nk_begin(ctx, "Timer Info", nk_rect(238, 466, 210, 200),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, texts.DIV, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.TIMA, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.TMA, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.TIMER_ON, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.MODULE, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.TSTATEStext, NK_TEXT_CENTERED);
                }
                nk_end(ctx);


                if (nk_begin(ctx, "Cart Info", nk_rect(472, 466, 190, 128),
                             NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, texts.BANKtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.RAMBANKtext, NK_TEXT_CENTERED);
                    nk_label(ctx, texts.RAMENtext, NK_TEXT_CENTERED);
                }
                nk_end(ctx);


                if (nk_begin(ctx, "Memory Instructions", nk_rect(238, 24, 424, 418),
                            NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    for (int i = 0; i < 30; i++) {
                        nk_label(ctx, instructions[i], NK_TEXT_LEFT);
                    }
                }
                nk_end(ctx);

                if (nk_begin(ctx, "Memory Viewer", nk_rect(686, 648, 650, 300),
                             NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    for (int i = 0; i < 4096; i++) {
                        nk_label(ctx, texts.memory[i], NK_TEXT_LEFT);
                    }
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
                    initialize_cpu_memory(&c, &set);
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
                            pixels[i][j] = Palettes[set.palette][video.display[i][j]];
                            if (i == video.scan_line)
                                pixels[i][j].a = 127;
                            if (i == c.memory[LYC])
                                pixels[i][j].r += 20;
                        }
                    }
                }
                else {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            pixels[i][j] = Palettes[set.palette][4];
                        }
                    }
                }
                UpdateTexture(display, pixels);
                struct nk_image display_debug = TextureToNuklear(display);
                if (nk_begin(ctx, "Display", nk_rect(686, 24, 650, 600),
                             NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, nk_window_get_height(ctx)-56, 1);
                    nk_image_color(ctx, display_debug, nk_white);
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

    save_game(&c.cart, rom_name);
    save_settings(&set);
    UnloadTexture(display);
    UnloadTexture(logo);
    UnloadNuklear(ctx);
    CloseWindow();

    return 0;
}
