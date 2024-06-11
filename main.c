#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "input.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

float frequency_CH2 = 100.0f;
uint8_t volume_CH2 = 15;

float frequency_CH1 = 100.0f;
uint8_t volume_CH1 = 15;

float duty_cicle[] = {0.125f, 0.25f, 0.5f, 0.75f};

float sineIdx1 = 0;
float sineIdx2 = 0;
float duty_CH1 = 0.5f;
float duty_CH2 = 0.5f;

void AudioInputCallback_CH1(void *buffer, unsigned int frames) {
    float incr = frequency_CH1 / 44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        int8_t sinemap;
        if (sineIdx1 >= duty_CH1)
            sinemap = 1;
        else
            sinemap = 0;
        d[i] = (short)(((float)(volume_CH1) * 250) * (PI * sinemap));
        sineIdx1 += incr;
        if (sineIdx1 > 1.0f) sineIdx1 -= 1.0f;
    }
}

void AudioInputCallback_CH2(void *buffer, unsigned int frames) {
    float incr = frequency_CH2 / 44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        int8_t sinemap;
        if (sineIdx2 >= duty_CH2)
            sinemap = 1;
        else
            sinemap = 0;
        d[i] = (short)(((float)(volume_CH2) * 250) * (PI * sinemap));
        sineIdx2 += incr;
        if (sineIdx2 > 1.0f) sineIdx2 -= 1.0f;
    }
}

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
    InitWindow(160*4, 144*3, "ChillyGB");
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    RenderTexture2D display = LoadRenderTexture(160, 144);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};

    // Load ROM to Memory
    //char rom_name[50] = "../Roms/Private/PokemonBlue.gb";
    //char rom_name[50] = "../Roms/Private/Tetris.gb";
    char rom_name[50] = "../Roms/Private/Zelda.gb";
    char save_name[50];
    strncpy(save_name, rom_name, 50);
    strreplace(save_name, ".gb", ".sv");

    printf("%s\n%s\n", rom_name, save_name);

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
    c.cart.bank_select_ram = 0;

    for (int i = 1; i < c.cart.banks; i++)
        fread(&c.cart.data[i], 0x4000, 1, cartridge);
    c.cart.bank_select = 1;
    c.cart.bank_select_ram = 0;
    fclose(cartridge);

    if (c.cart.type == 3 || c.cart.type == 0x13) {
        FILE *save = fopen(save_name, "r");
        if (save != NULL) {
            if (c.cart.banks_ram == 1)
                fread(&c.cart.ram, 0x2000, 1, save);
            else if (c.cart.banks_ram == 4)
                fread(&c.cart.ram, 0x8000, 1, save);
            fclose(save);
        }
    }

    //Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(256);
    // Channel 1
    AudioStream CH1 = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(CH1, AudioInputCallback_CH1);
    PlayAudioStream(CH1);
    // Channel 2
    AudioStream CH2 = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(CH2, AudioInputCallback_CH2);
    PlayAudioStream(CH2);


    int ticks = 0;
    while(!WindowShouldClose()) {
        execute(&c, &t);
        c.memory[JOYP] = get_joypad(&c, &j1);

        uint32_t periodvalue_CH1 = (uint16_t) ((c.memory[NR14] & 7) << 8) | c.memory[NR13];
        if ((c.memory[NR12] & 0xf8) != 0) {
            frequency_CH1 = 131072 / (2048 - periodvalue_CH1);
            volume_CH1 = (c.memory[NR12] >> 4);
        }
        else {
            frequency_CH1 = 0;
            volume_CH1 = 0;
        }

        uint32_t periodvalue_CH2 = (uint16_t) ((c.memory[NR24] & 7) << 8) | c.memory[NR23];
        if ((c.memory[NR22] & 0xf8) != 0) {
            frequency_CH2 = 131072 / (2048 - periodvalue_CH2);
            volume_CH2 = (c.memory[NR22] >> 4);
        }
        else {
            frequency_CH2 = 0;
            volume_CH2 = 0;
        }

        if (t.is_scanline > 0) {
            if (c.memory[0xff44] <= 144) {
                load_display(&c, &p);
                t.is_scanline = 0;
                int y = c.memory[0xff44] - 1;
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
            uint8_t y1 = c.memory[0xff44] - 8;
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

    if (c.cart.type == 3 || c.cart.type == 0x13) {
        FILE *save = fopen(save_name, "w");
        if (c.cart.banks_ram == 1)
            fwrite(&c.cart.ram, 0x2000, 1, save);
        if (c.cart.banks_ram == 4)
            fwrite(&c.cart.ram, 0x8000, 1, save);
        fclose(save);
    }
    CloseWindow();

    return 0;
}
