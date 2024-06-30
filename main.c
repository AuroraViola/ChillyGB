#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

char *strreplace(char *s, const char *s1, const char *s2) {
    char *p = strstr(s, s1);
    if (p != NULL) {
        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);
        if (len1 != len2)
            memmove(p + len2, p + len1, strlen(p + len1) + 1);
        memcpy(p, s2, len2);
    }
    return s;
}

int main(void) {
    // Initialize CPU, memory and timer
    cpu c = {};
    tick t = {.tima_counter = 0, .divider_register = 0, .scan_line_tick = 0, .t_states = 0};

    // Initialize Joypad
    joypad j1 = {.buttons = { 0 }, .dpad = { 0 }};

    // Initialize PPU and Raylib
    ppu p = {.display = { 0 }, .background = { 0 }, .window = { 0 }, .sprite_display = { 0 }};
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    RenderTexture2D display = LoadRenderTexture(160, 144);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};

    // Load ROM to Memory
    //char rom_name[80] = "../Roms/Private/20y.gb";
    //char rom_name[80] = "../Roms/Private/CounterTest.gb";
    //char rom_name[80] = "../Roms/Private/bgbtest.gb";
    //char rom_name[80] = "../Roms/Private/DrMario.gb";
    //char rom_name[80] = "../Roms/Private/PokemonBlue.gb";
    //char rom_name[80] = "../Roms/Private/KirbyDreamLand.gb";
    //char rom_name[80] = "../Roms/Private/MarioLand.gb";
    //char rom_name[80] = "../Roms/Private/PokemonGiallo.gb";
    //char rom_name[80] = "../Roms/Private/PokemonBlue.gb";
    //char rom_name[80] = "../Roms/Private/PokemonGold.gbc";
    //char rom_name[80] = "../Roms/Private/Tetris.gb";
    //char rom_name[80] = "../Roms/Private/Zelda.gb";
    //char rom_name[80] = "../Roms/Private/DrMario.gb";
    //char rom_name[80] = "../Roms/dmg_sound/01-registers.gb";
    //char rom_name[80] = "../Roms/dmg_sound/02-len ctr.gb";
    //char rom_name[80] = "../Roms/dmg_sound/09-wave read while on.gb";
    //char rom_name[80] = "../Roms/dmg_sound/10-wave trigger while on.gb";
    //char rom_name[80] = "../Roms/dmg_sound/11-regs after power.gb";
    //char rom_name[80] = "../Roms/dmg_sound/12-wave write while on.gb";
    char rom_name[80] = "../Roms/Private/bully.gb";
    //char rom_name[80] = "../Roms/mooneye-acceptance/boot_hwio-dmgABCmgb.gb";
    char save_name[80];
    strncpy(save_name, rom_name, 50);
    strreplace(save_name, ".gb", ".sv");

    FILE *cartridge = fopen(rom_name, "r");

    fread(&c.cart.data[0], 0x4000, 1, cartridge);

    c.cart.type = c.cart.data[0][0x0147];
    c.cart.banks = (2 << c.cart.data[0][0x0148]);

    c.cart.banks_ram = 0;
    if (c.cart.data[0][0x0149] == 2) {
        c.cart.banks_ram = 1;
    }
    else if (c.cart.data[0][0x0149] == 3) {
        c.cart.banks_ram = 4;
    }
    else if (c.cart.data[0][0x0149] == 4) {
        c.cart.banks_ram = 16;
    }
    else if (c.cart.data[0][0x0149] == 5) {
        c.cart.banks_ram = 8;
    }
    c.cart.bank_select_ram = 0;
    c.cart.ram_enable = false;

    for (int i = 1; i < c.cart.banks; i++)
        fread(&c.cart.data[i], 0x4000, 1, cartridge);
    c.cart.bank_select = 1;
    c.cart.bank_select_ram = 0;
    fclose(cartridge);

    // Initialize APU
    InitAudioDevice();

    audio.ch1.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch1.stream, AudioInputCallback_CH1);
    PlayAudioStream(audio.ch1.stream);

    audio.ch2.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch2.stream, AudioInputCallback_CH2);
    PlayAudioStream(audio.ch2.stream);

    audio.ch3.stream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(audio.ch3.stream, AudioInputCallback_CH3);
    PlayAudioStream(audio.ch3.stream);

    if (c.cart.type == 3 || c.cart.type == 0x13 || c.cart.type == 0x1b) {
        FILE *save = fopen(save_name, "r");
        if (save != NULL) {
            if (c.cart.banks_ram == 1)
                fread(&c.cart.ram, 0x2000, 1, save);
            else if (c.cart.banks_ram == 4)
                fread(&c.cart.ram, 0x8000, 1, save);
            else if (c.cart.banks_ram == 8)
                fread(&c.cart.ram, 0x10000, 1, save);
            else if (c.cart.banks_ram == 16)
                fread(&c.cart.ram, 0x20000, 1, save);
            fclose(save);
        }
    }

    initialize_cpu_memory(&c);

    while(!WindowShouldClose()) {
        execute(&c, &t);
        c.memory[JOYP] = get_joypad(&c, &j1);
        Update_Audio(&c);

        if (t.is_scanline > 0) {
            if (c.memory[LY] <= 144) {
                load_display(&c, &p);
                t.is_scanline = 0;
                int y = c.memory[LY] - 1;
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
            }
            uint8_t y1 = c.memory[LY] - 8;
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
                /*
                DrawText(TextFormat("CH3 on: %i", audio.ch3.is_active), 0, 0, 20, RED);
                DrawText(TextFormat("CH3 DAC: %i", (uint8_t)((c.memory[NR30] & 0x80) != 0)), 120, 0, 20, RED);
                DrawText(TextFormat("CH3 period: %x", audio.ch3.period_value), 260, 0, 20, RED);
                DrawText(TextFormat("CH3 volume: %i", audio.ch3.volume), 0, 32, 20, RED);
                DrawText(TextFormat("CH3 lenght enable: %i", (uint8_t)((c.memory[NR34] & 64) != 0)), 0, 48, 20, RED);
                DrawText(TextFormat("CH3 lenght: %i", audio.ch3.lenght), 0, 64, 20, RED);
                 */
            EndDrawing();
            uint16_t fps = GetFPS();
            char str[22];
            sprintf(str, "ChillyGB - %d FPS", fps);
            SetWindowTitle(str);
        }
    }

    if (c.cart.type == 3 || c.cart.type == 0x13 || c.cart.type == 0x1b) {
        FILE *save = fopen(save_name, "w");
        if (c.cart.banks_ram == 1)
            fwrite(&c.cart.ram, 0x2000, 1, save);
        else if (c.cart.banks_ram == 4)
            fwrite(&c.cart.ram, 0x8000, 1, save);
        else if (c.cart.banks_ram == 8)
            fwrite(&c.cart.ram, 0x10000, 1, save);
        else if (c.cart.banks_ram == 16)
            fwrite(&c.cart.ram, 0x20000, 1, save);
        fclose(save);
    }

    UnloadRenderTexture(display);
    CloseWindow();

    return 0;
}
