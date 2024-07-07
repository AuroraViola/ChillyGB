#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
#include <stdio.h>
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

    // Initialize Raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
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


    // Load Cartridge to Memory
    //char rom_name[256] = "../Roms/HelloWorld.gb";
    //char rom_name[256] = "../Roms/Private/dmg-acid2.gb";
    //char rom_name[256] = "../Roms/Private/DrMario.gb";
    //char rom_name[256] = "../Roms/Private/PokemonGiallo.gb";
    //char rom_name[256] = "../Roms/Private/PokemonBlue.gb";
    //char rom_name[256] = "../Roms/Private/MarioLand.gb";
    //char rom_name[256] = "../Roms/Private/Zelda.gb";
    //char rom_name[256] = "../Roms/Private/gb240p.gb";
    //char rom_name[256] = "../Roms/Private/Spot.gb";
    //char rom_name[256] = "../Roms/Private/bad_apple.gb";
    //char rom_name[256] = "../Roms/Private/20y.gb";
    //char rom_name[256] = "../Roms/Private/bgbtest.gb";
    //char rom_name[256] = "../Roms/Private/KirbyDreamLand.gb";
    //char rom_name[256] = "../Roms/Private/Tetris.gb";
    //char rom_name[256] = "../Roms/Private/instr_timing.gb";
    //char rom_name[256] = "../Roms/Private/interrupt_time.gb";
    char rom_name[256] = "../Roms/mooneye-acceptance/boot_hwio-dmgABCmgb.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/div_write.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/rapid_toggle.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim00.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim00_div_trigger.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim01.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim01_div_trigger.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim10.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim10_div_trigger.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim11.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tim11_div_trigger.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tima_reload.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tima_write_reloading.gb";
    //char rom_name[256] = "../Roms/mooneye-acceptance/timer/tma_write_reloading.gb";
    //char rom_name[256] = "../Roms/Private/L2.GB";
    //char rom_name[256] = "../Roms/Private/halt_bug.gb";
    char save_name[256];
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


    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    // Initialize Value
    initialize_cpu_memory(&c);

    while(!WindowShouldClose()) {
        execute(&c);
        Update_Audio(&c);

        if (video.draw_screen == true) {
            video.draw_screen = false;
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
            UpdateTexture(display, pixels);
            // Draw
            float scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
            BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                               (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                            (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                            (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
            EndDrawing();
            uint16_t fps = GetFPS();
            char str[40];
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

    CloseWindow();

    return 0;
}
