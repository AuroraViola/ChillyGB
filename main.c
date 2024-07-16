#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "timer.h"
#include "opcodes.h"
#include "debug.h"
#include "input.h"
#include "cartridge.h"
#include <stdio.h>
#include <string.h>

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "raylib-nuklear/include/raylib-nuklear.h"

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

int main(void) {
    uint8_t volume = 255;
    int current_palette = 0;

    // Initialize CPU, memory and timer
    cpu c = {};

    // Initialize Raylib and Nuklear
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    struct nk_context *ctx = InitNuklearEx(LoadFontEx("../res/fonts/UbuntuMono.ttf", 20, 0, 250), 20);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = Palettes[current_palette][4];
    Image display_image = {
            .data = pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    Texture2D display = LoadTextureFromImage(display_image);

    char rom_name[256];

    uint8_t emulator_mode = MENU;

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    float scale;

    char instructions[30][50];
    debugtexts texts;

    bool game_started = false;
    int ff_speed = 1;

    audio.volume = volume;

    while(!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if ((int) droppedFiles.count == 1) {
                save_game(&c.cart, rom_name);
                strcpy(rom_name, droppedFiles.paths[0]);
                load_game(&c.cart, rom_name);
                initialize_cpu_memory(&c);
                emulator_mode = GAME;
                game_started = true;
            }
            UnloadDroppedFiles(droppedFiles);
        }
        scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
        switch (emulator_mode) {
            case MENU:
                UpdateNuklear(ctx);
                if (nk_begin(ctx, "Settings", nk_rect(24, 24, 400, 400),
                            NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                    nk_layout_row_dynamic(ctx, 30, 2);
                    nk_label(ctx, "Sound Volume", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    audio.volume = nk_slide_int(ctx, 0, audio.volume, 255, 1);
                    nk_label(ctx, "Palette", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    struct nk_vec2 size = {200, 100};
                    nk_combobox(ctx, palettes, 6, &current_palette, 20, size);
                    if (game_started) {
                        nk_layout_row_dynamic(ctx, 60, 1);
                        if (nk_button_label(ctx, "Reset game")) {
                            initialize_cpu_memory(&c);
                            emulator_mode = GAME;
                            ResumeAudioStream(audio.ch1.stream);
                            ResumeAudioStream(audio.ch2.stream);
                            ResumeAudioStream(audio.ch3.stream);
                            ResumeAudioStream(audio.ch4.stream);
                        }
                    }
                }
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
                        DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, Palettes[current_palette][3]);
                    }
                    else {
                        float fontsize = 10 * scale;
                        int center = MeasureText("EMULATION PAUSED", fontsize);
                        DrawText("EMULATION PAUSED", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, Palettes[current_palette][3]);
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
                                switch (video.display[i][j]) {
                                    case 0:
                                        pixels[i][j] = Palettes[current_palette][0];
                                        break;
                                    case 1:
                                        pixels[i][j] = Palettes[current_palette][1];
                                        break;
                                    case 2:
                                        pixels[i][j] = Palettes[current_palette][2];
                                        break;
                                    case 3:
                                        pixels[i][j] = Palettes[current_palette][3];
                                        break;
                                }
                            }
                        }
                    }
                    else {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = Palettes[current_palette][4];
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
                    uint16_t fps = GetFPS();
                    char str[80];
                    sprintf(str, "ChillyGB - %d FPS - %.1fx", fps, (float)(fps)/60);
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
                    initialize_cpu_memory(&c);
                }

                if (IsKeyPressed(KEY_F3)) {
                    emulator_mode = GAME;
                    ResumeAudioStream(audio.ch1.stream);
                    ResumeAudioStream(audio.ch2.stream);
                    ResumeAudioStream(audio.ch3.stream);
                    ResumeAudioStream(audio.ch4.stream);
                }

                if (video.is_on) {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            switch (video.display[i][j]) {
                                case 0:
                                    pixels[i][j] = Palettes[current_palette][0];
                                    break;
                                case 1:
                                    pixels[i][j] = Palettes[current_palette][1];
                                    break;
                                case 2:
                                    pixels[i][j] = Palettes[current_palette][2];
                                    break;
                                case 3:
                                    pixels[i][j] = Palettes[current_palette][3];
                                    break;
                            }
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
                            pixels[i][j] = Palettes[current_palette][4];
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
    UnloadTexture(display);
    UnloadNuklear(ctx);
    CloseWindow();

    return 0;
}
