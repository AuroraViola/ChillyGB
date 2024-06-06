#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "input.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

int main(void) {
    // Initialize CPU and memory
    cpu c = {.pc = 0x100, .sp = 0xfffe};
    c.r.reg8[A] = 0x01;
    c.r.reg8[B] = 0x00;
    c.r.reg8[C] = 0x13;
    c.r.reg8[D] = 0x00;
    c.r.reg8[E] = 0xd8;
    c.r.reg8[F] = 0xb0;
    c.r.reg8[H] = 0x01;
    c.r.reg8[L] = 0x4d;
    c.memory[0xff44] = 0x90;
    c.memory[0xff04] = 0xab;

    // Initialize Timer
    tick t = {.tima_counter = 0, .divider_register = 0, .scan_line_tick = 0, .t_states = 0};

    // Initialize Joypad
    joypad j1 = {.buttons = { 0 }, .dpad = { 0 }};

    // Initialize PPU and Raylib
    ppu p = {.display = { 0 }, .background = { 0 }, .window = { 0 }, .sprite_display = { 0 }};
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*3, 144*3, "ChillyGB");
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    RenderTexture2D display = LoadRenderTexture(160, 144);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};

    // Load ROM to Memory
    //FILE *cartridge = fopen("../Roms/HelloWorld.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/winpos.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/Tetris.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/bgbtest.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/tellinglys.gb", "r");
    FILE *cartridge = fopen("../Roms/Private/dmg-acid2.gb", "r");
    //FILE *cartridge = fopen("../Roms/Private/bully.gb", "r");
    //assert(cartridge != NULL && "File not found");
    fread(&c.memory[0], 0x8000, 1, cartridge);

    int ticks = 0;
    while(!WindowShouldClose()) {
        //printf("A:%.2X F:%.2X B:%.2X C:%.2X D:%.2X E:%.2X H:%.2X L:%.2X SP:%.4X PC:%.4X PCMEM:%.2X,%.2X,%.2X,%.2X\n",
            //c.r.reg8[A], c.r.reg8[F], c.r.reg8[B], c.r.reg8[C], c.r.reg8[D], c.r.reg8[E], c.r.reg8[H], c.r.reg8[L],
            //c.sp, c.pc, c.memory[c.pc], c.memory[c.pc+1], c.memory[c.pc+2], c.memory[c.pc+3]);
        execute(&c, &t);
        c.memory[0xff00] = get_joypad(&c, &j1);

        if (t.is_scanline > 0 && c.memory[0xff44] <= 144) {
        //if (((ticks % 54) == 0) && c.memory[0xff44] <= 144) {  // temporary fix
            load_display(&c, &p);
            t.is_scanline = 0;
            int y = c.memory[0xff44]-1;
            for (int x = 0; x < 160; x++) {
                switch (p.display[y][x]) {
                    case 0:
                        pixels[y][x] = (Color) {185, 237, 186, 255};
                        break;
                    case 1:
                        pixels[y][x] = (Color) {118, 196, 123, 255};
                        break;
                    case 2:
                        pixels[y][x] = (Color) {49, 106, 64, 255};
                        break;
                    case 3:
                        pixels[y][x] = (Color) {10, 38, 16, 255};
                        break;
                }
            }
            uint8_t y1 = c.memory[0xff44] - 16;
            if (y1 < 144) {
                for (int x = 0; x < 160; x++) {
                    switch (p.sprite_display[y1][x]) {
                        case 1:
                            pixels[y1][x] = (Color) {185, 237, 186, 255};
                            break;
                        case 2:
                            pixels[y1][x] = (Color) {118, 196, 123, 255};
                            break;
                        case 3:
                            pixels[y1][x] = (Color) {49, 106, 64, 255};
                            break;
                        case 4:
                            pixels[y1][x] = (Color) {10, 38, 16, 255};
                            break;
                    }
                }
            }
        }

        if (t.is_frame && (c.memory[0xff44] < 144)) {
            t.is_frame = false;

            BeginTextureMode(display);
                ClearBackground(BLACK);
                for (int i = 0; i < 144; i++)
                    for (int j = 0; j < 160; j++)
                        DrawRectangle(j, -i+143, 1, 1, pixels[i][j]);
            EndTextureMode();
            // Draw
            float scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
            BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(display.texture, (Rectangle) {0.0f, 0.0f, (float) display.texture.width, (float) display.texture.height},
                               (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                            (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                            (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
            EndDrawing();
            uint16_t fps = GetFPS();
            char str[22];
            sprintf(str, "ChillyGB - %d FPS", fps);
            SetWindowTitle(str);
        }

        ticks += 1;
    }

    UnloadRenderTexture(display);

    CloseWindow();

    return 0;
}
