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

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "style_dark.h"

#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef enum EmuModes{
    MENU = 0,
    GAME,
    DEBUG,
}EmuModes;

int main(void) {
    // Initialize CPU, memory and timer
    cpu c = {};

    // Initialize Raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetExitKey(KEY_NULL);
    GuiLoadStyleDark();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 32);
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

    int ff_speed = 50;

    char instructions[30][50];
    Vector2 MousePosition = { 0 };
    Vector2 panOffset = MousePosition;


    Vector2 DisplayOffset = {-664, 24};
    bool DisplayBoxActive = true;
    bool DisplayBoxDrag = false;

    Vector2 RegistersOffset = {24, 24};
    bool RegistersBoxActive = true;
    bool RegistersBoxDrag = false;

    Vector2 InstructionsOffset = {280, 24};
    bool InstructionsBoxActive = false;
    bool InstructionsBoxDrag = false;

    Vector2 PPUInfoOffset = {24, 300};
    bool PPUInfoBoxActive = false;
    bool PPUInfoBoxDrag = false;

    Vector2 IntrInfoOffset = {24, 364};
    bool IntrInfoBoxActive = false;
    bool IntrInfoBoxDrag = false;

    Vector2 CartInfoOffset = {24, 428};
    bool CartInfoBoxActive = false;
    bool CartInfoBoxDrag = false;

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

            case DEBUG:
                Vector2 TopRightAnchor = {GetScreenWidth(), 0};
                MousePosition = GetMousePosition();
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !DisplayBoxDrag && !RegistersBoxDrag && !InstructionsBoxDrag && !PPUInfoBoxDrag && !CartInfoBoxDrag && !IntrInfoBoxDrag) {
                    if (CheckCollisionPointRec(MousePosition, (Rectangle){ IntrInfoOffset.x, IntrInfoOffset.y, 250, 24 })) {
                        IntrInfoBoxDrag = true;
                    }
                    else if (CheckCollisionPointRec(MousePosition, (Rectangle){ CartInfoOffset.x, CartInfoOffset.y, 250, 24 })) {
                        CartInfoBoxDrag = true;
                    }
                    else if (CheckCollisionPointRec(MousePosition, (Rectangle){ PPUInfoOffset.x, PPUInfoOffset.y, 250, 24 })) {
                        PPUInfoBoxDrag = true;
                    }
                    else if (CheckCollisionPointRec(MousePosition, (Rectangle){ InstructionsOffset.x, InstructionsOffset.y, 350, 24 })) {
                        InstructionsBoxDrag = true;
                    }
                    else if (CheckCollisionPointRec(MousePosition, (Rectangle){ RegistersOffset.x, RegistersOffset.y, 176, 24 })) {
                        RegistersBoxDrag = true;
                    }
                    else if (CheckCollisionPointRec(MousePosition, (Rectangle){ TopRightAnchor.x-1 + DisplayOffset.x, DisplayOffset.y, 642, 24 })) {
                        DisplayBoxDrag = true;
                    }
                }

                if (DisplayBoxDrag) {
                    DisplayOffset.x += (MousePosition.x - panOffset.x);
                    DisplayOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        DisplayBoxDrag = false;
                    }
                }

                else if (RegistersBoxDrag) {
                    RegistersOffset.x += (MousePosition.x - panOffset.x);
                    RegistersOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        RegistersBoxDrag = false;
                    }
                }

                else if (InstructionsBoxDrag) {
                    InstructionsOffset.x += (MousePosition.x - panOffset.x);
                    InstructionsOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        InstructionsBoxDrag = false;
                    }
                }
                else if (PPUInfoBoxDrag) {
                    PPUInfoOffset.x += (MousePosition.x - panOffset.x);
                    PPUInfoOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        PPUInfoBoxDrag = false;
                    }
                }
                else if (CartInfoBoxDrag) {
                    CartInfoOffset.x += (MousePosition.x - panOffset.x);
                    CartInfoOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        CartInfoBoxDrag = false;
                    }
                }
                else if (IntrInfoBoxDrag) {
                    IntrInfoOffset.x += (MousePosition.x - panOffset.x);
                    IntrInfoOffset.y += (MousePosition.y - panOffset.y);

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                        IntrInfoBoxDrag = false;
                    }
                }
                panOffset = MousePosition;
                debugtexts texts;
                generate_texts(&c, &texts);
                decode_instructions(&c, instructions);

                if (IsKeyDown(KEY_F)) {
                    for (int i = 0; i < ff_speed; i++) {
                        execute(&c);
                        Update_Audio(&c);
                    }
                }

                if (IsKeyPressed(KEY_N) || IsKeyDown(KEY_C)) {
                    execute(&c);
                    Update_Audio(&c);
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
                            int alpha = 255;
                            int red = 0;
                            if (i == video.scan_line)
                                alpha = 127;
                            if (i == c.memory[LYC])
                                red = 20;
                            switch (video.display[i][j]) {
                                case 0:
                                    pixels[i][j] = (Color) {185 + red, 237, 186, alpha};
                                    break;
                                case 1:
                                    pixels[i][j] = (Color) {118 + red, 196, 123, alpha};
                                    break;
                                case 2:
                                    pixels[i][j] = (Color) {49 + red, 106, 64, alpha};
                                    break;
                                case 3:
                                    pixels[i][j] = (Color) {10 + red, 38, 16, alpha};
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
                    ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
                    if (DisplayBoxActive) {
                        DisplayBoxActive = !GuiWindowBox((Rectangle){ TopRightAnchor.x-1 + DisplayOffset.x, DisplayOffset.y, 642, 601 }, "Display");
                        DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                                       (Rectangle) {(TopRightAnchor.x + DisplayOffset.x),
                                                    DisplayOffset.y+24,
                                                    (float) 160 * 4, (float) 144 * 4},
                                       (Vector2) {0, 0}, 0.0f, WHITE);
                    }
                    else
                        DisplayBoxActive = GuiWindowBox((Rectangle){ TopRightAnchor.x-1 + DisplayOffset.x, DisplayOffset.y, 642, 0 }, "Display");

                    if (RegistersBoxActive) {
                        RegistersBoxActive = !GuiWindowBox((Rectangle){ RegistersOffset.x, RegistersOffset.y, 176, 258 }, "Registers");
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 32, 120, 24 }, texts.AFtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 64, 120, 24 }, texts.BCtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 96, 120, 24 }, texts.DEtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 128, 120, 24 }, texts.HLtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 160, 120, 24 }, texts.SPtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 192, 120, 24 }, texts.PCtext);
                        GuiLabel((Rectangle){ RegistersOffset.x + 8, RegistersOffset.y + 224, 120, 24 }, texts.IMEtext);
                    }
                    else
                        RegistersBoxActive = GuiWindowBox((Rectangle){ RegistersOffset.x, RegistersOffset.y, 176, 0 }, "Registers");

                    if (InstructionsBoxActive) {
                        InstructionsBoxActive = !GuiWindowBox((Rectangle){ InstructionsOffset.x, InstructionsOffset.y, 350, 670 }, "Instructions");
                        for (int i = 0; i < 20; i++) {
                            GuiLabel((Rectangle){ InstructionsOffset.x + 5, InstructionsOffset.y + (32 * (i+1)), 340, 24 }, instructions[i]);
                        }
                    }
                    else
                        InstructionsBoxActive = GuiWindowBox((Rectangle){ InstructionsOffset.x, InstructionsOffset.y, 350, 0 }, "Instructions");

                    if (PPUInfoBoxActive) {
                        PPUInfoBoxActive = !GuiWindowBox((Rectangle){ PPUInfoOffset.x, PPUInfoOffset.y, 250, 200 }, "PPU State");
                        GuiLabel((Rectangle){ PPUInfoOffset.x + 8, PPUInfoOffset.y + 32, 250, 24 }, texts.LYtext);
                        GuiLabel((Rectangle){ PPUInfoOffset.x + 8, PPUInfoOffset.y + 64, 250, 24 }, texts.LYCtext);
                        GuiLabel((Rectangle){ PPUInfoOffset.x + 8, PPUInfoOffset.y + 96, 250, 24 }, texts.PPUMode);
                        GuiLabel((Rectangle){ PPUInfoOffset.x + 8, PPUInfoOffset.y + 128, 250, 24 }, texts.LCDCtext);
                        GuiLabel((Rectangle){ PPUInfoOffset.x + 8, PPUInfoOffset.y + 160, 250, 24 }, texts.STATtext);
                    }
                    else
                        PPUInfoBoxActive = GuiWindowBox((Rectangle){ PPUInfoOffset.x, PPUInfoOffset.y, 250, 0 }, "PPU State");

                    if (CartInfoBoxActive) {
                        CartInfoBoxActive = !GuiWindowBox((Rectangle) {CartInfoOffset.x, CartInfoOffset.y, 250, 130}, "Cartridge state");
                        GuiLabel((Rectangle) {CartInfoOffset.x + 8, CartInfoOffset.y + 32, 250, 24}, texts.BANKtext);
                        GuiLabel((Rectangle) {CartInfoOffset.x + 8, CartInfoOffset.y + 64, 250, 24}, texts.RAMBANKtext);
                        GuiLabel((Rectangle) {CartInfoOffset.x + 8, CartInfoOffset.y + 96, 250, 24}, texts.RAMENtext);
                    }
                    else
                        CartInfoBoxActive = GuiWindowBox((Rectangle) {CartInfoOffset.x, CartInfoOffset.y, 250, 0}, "Cartridge state");

                    if (IntrInfoBoxActive) {
                        IntrInfoBoxActive = !GuiWindowBox((Rectangle) {IntrInfoOffset.x, IntrInfoOffset.y, 250, 130}, "Interrupts state");
                        GuiLabel((Rectangle) {IntrInfoOffset.x + 8, IntrInfoOffset.y + 32, 250, 24}, texts.IEtext);
                        GuiLabel((Rectangle) {IntrInfoOffset.x + 8, IntrInfoOffset.y + 64, 250, 24}, texts.IFtext);
                        GuiLabel((Rectangle) {IntrInfoOffset.x + 8, IntrInfoOffset.y + 96, 250, 24}, texts.IMEtext);
                    }
                    else
                        IntrInfoBoxActive = GuiWindowBox((Rectangle) {IntrInfoOffset.x, IntrInfoOffset.y, 250, 0}, "Interrupts state");

                EndDrawing();
                SetWindowTitle("ChillyGB - Debug");
                break;
        }

    }

    save_game(&c.cart, rom_name);
    UnloadTexture(display);
    CloseWindow();

    return 0;
}
