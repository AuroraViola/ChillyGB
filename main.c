#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
#include "cartridge.h"
#include <stdio.h>
#include <string.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef enum EmuModes{
    MENU = 0,
    GAME,
}EmuModes;

int main(void) {
    // Initialize CPU, memory and timer
    cpu c = {};

    // Initialize Raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};
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

    bool game_started = false;
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
                        DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, (Color) {10, 38, 16, 255});
                    }
                    else {
                        float fontsize = 10 * scale;
                        int center = MeasureText("EMULATION PAUSED", fontsize);
                        DrawText("EMULATION PAUSED", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, (Color) {10, 38, 16, 255});
                    }
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
                    video.draw_screen = false;
                    if (video.is_on) {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                switch (video.display[i][j]) {
                                    case 0:
                                        pixels[i][j] = (Color) {185, 237, 186, 255};
                                        break;
                                    case 1:
                                        pixels[i][j] = (Color) {118, 196, 123, 255};
                                        break;
                                    case 2:
                                        pixels[i][j] = (Color) {49, 106, 64, 255};
                                        break;
                                    case 3:
                                        pixels[i][j] = (Color) {10, 38, 16, 255};
                                        break;
                                }
                            }
                        }
                    }
                    else {
                        for (int i = 0; i < 144; i++) {
                            for (int j = 0; j < 160; j++) {
                                pixels[i][j] = (Color) {200, 237, 186, 255};
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
        }

    }

    save_game(&c.cart, rom_name);
    UnloadTexture(display);
    CloseWindow();

    return 0;
}
