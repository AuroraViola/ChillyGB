#include "cpu.h"
#include "ppu.h"
#include <stdint.h>
#include <stdio.h>

void decode_tile(cpu *c, uint16_t addr, uint8_t tile[8][8]) {
    uint8_t encoded_tile[16];
    for (int i = 0; i < 16; i++) {
        encoded_tile[i] = c->memory[addr + i];
    }

    for (int y = 0; y < 8; y++) {
        uint8_t byte1 = encoded_tile[y * 2];
        uint8_t byte2 = encoded_tile[y * 2 + 1];
        for (int x = 0; x < 8; x++) {
            uint8_t bit1 = (byte1 >> (7-x)) & 1;
            uint8_t bit2 = (byte2 >> (7-x)) & 1;
            tile[y][x] = (bit2 << 1) | bit1;
        }
    }
}

void flipY_tile(uint8_t tile[8][8]) {
    uint8_t tilecopy[8][8];
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tilecopy[i][j] = tile[i][j];
        }
    }

    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tile[i][j] = tilecopy[7-i][j];
        }
    }
}

void flipX_tile(uint8_t tile[8][8]) {
    uint8_t tilecopy[8][8];
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tilecopy[i][j] = tile[i][j];
        }
    }

    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tile[i][j] = tilecopy[i][7-j];
        }
    }
}

void load_sprites(cpu *c, ppu *p) {
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 4; j++) {
            p->sprites[i][j] = c->memory[0xfe00 + (4 * i) + j];
        }
    }
}

void load_tiles(cpu *c, ppu *p) {
    if ((c->memory[0xff40] & 16) != 0) {
        for (int i = 0; i < 256; i++) {
            decode_tile(c, (0x8000 + (i << 4)), p->tiles[i]);
        }
    }
    else {
        for (int i = 0; i < 128; i++) {
            decode_tile(c, (0x9000 | (i << 4)), p->tiles[i]);
        }
        for (int i = 128; i < 256; i++) {
            decode_tile(c, (0x8800 | (i << 4)), p->tiles[i]);
        }
    }
}

void load_tilemap(cpu *c, ppu *p) {
    for (int i = 0; i < 1024 ; i++) {
        p->tilemap[0][i] = c->memory[i + 0x9800];
    }
    for (int i = 0; i < 1024 ; i++) {
        p->tilemap[1][i] = c->memory[i + 0x9c00];
    }
}

void load_background(cpu *c, ppu *p) {
    uint8_t tilemap_select = 1;
    if ((c->memory[0xff40] & 8) == 0)
        tilemap_select = 0;
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                        p->background[i*8 + y][j*8 + x] = p->tiles[p->tilemap[tilemap_select][32*i+j]][y][x];
                }
            }
        }
    }
}

void load_window(cpu *c, ppu *p) {
    uint8_t tilemap_select = 1;
    if ((c->memory[0xff40] & 64) == 0)
        tilemap_select = 0;
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    p->window[i*8 + y][j*8 + x] = p->tiles[p->tilemap[tilemap_select][32*i+j]][y][x];
                }
            }
        }
    }
}

void load_display(cpu *c, ppu *p) {
    uint8_t bg_pal = c->memory[0xff47];
    uint8_t bg_palette[] = { (bg_pal & 3), ((bg_pal & 12) >> 2), ((bg_pal & 48) >> 4), (bg_pal >> 6) };
    uint8_t obp0 = c->memory[0xff48];
    uint8_t obp1 = c->memory[0xff49];
    uint8_t s_palette[4];

    if (c->tiles_write) {
        c->tiles_write = false;
        load_tiles(c, p);
    }
    if (c->tilemap_write) {
        c->tilemap_write = false;
        load_tilemap(c, p);
    }

    if (c->need_bg_wn_reload) {
        load_background(c, p);
        load_window(c, p);
    }

    if ((c->memory[0xff40] & 128) != 0) {
        // Background
        if ((c->memory[0xff40] & 1) != 0) {
            uint8_t scx = c->memory[0xff43];
            uint8_t scy = c->memory[0xff42];
            if (c->memory[0xff44] < 144) {
                int y = c->memory[0xff44];
                for (uint8_t x = 0; x < 160; x++) {
                    p->display[y][x] = bg_palette[p->background[(uint8_t) (y + scy)][(uint8_t) (x + scx)]];
                }
            }
        }
        else {
            if (c->memory[0xff44] < 144) {
                int y = c->memory[0xff44];
                for (uint8_t x = 0; x < 160; x++) {
                    p->display[y][x] = 0;
                }
            }
        }
        // Window
        if (((c->memory[0xff40] & 1) != 0) && ((c->memory[0xff40] & 32) != 0)) {
            uint8_t wy = c->memory[0xff4a];
            uint8_t wx = c->memory[0xff4b];
            if (c->memory[0xff44] < 144) {
                int y = c->memory[0xff44];
                for (uint8_t x = 0; x < 167; x++) {
                    if ((y >= wy) && ((x + wx - 7) < 160) && ((x + wx - 7) >= 0)) {
                        p->display[y][x + wx - 7] = p->window[c->window_internal_line][x];
                    }
                }
            }
        }
        // Objects
        if ((c->memory[0xff40] & 2) != 0) {
            load_sprites(c, p);
            for (int i = 0; i < 40; i++) {
                if (((p->sprites[i][0] < 160) && (p->sprites[i][0] > 15) && (p->sprites[i][1] < 160) && (p->sprites[i][1] > 7))) {
                    uint8_t sprite_tile[8][8];
                    decode_tile(c, (0x8000 | (p->sprites[i][2] << 4)), sprite_tile);

                    if (((p->sprites[i][3] >> 4) & 1) != 0) {
                        s_palette[0] = obp1 & 3;
                        s_palette[1] = (obp1 >> 2) & 3;
                        s_palette[2] = (obp1 >> 4) & 3;
                        s_palette[3] = (obp1 >> 6) & 3;
                    }
                    else {
                        s_palette[0] = obp0 & 3;
                        s_palette[1] = (obp0 >> 2) & 3;
                        s_palette[2] = (obp0 >> 4) & 3;
                        s_palette[3] = (obp0 >> 6) & 3;
                    }

                    // X flip
                    if (((p->sprites[i][3] >> 5) & 1) != 0)
                        flipX_tile(sprite_tile);

                    // Y flip
                    if (((p->sprites[i][3] >> 6) & 1) != 0)
                        flipY_tile(sprite_tile);

                    for (int16_t y = 0; y < 8; y++) {
                        for (int16_t x = 0; x < 8; x++) {
                            if (sprite_tile[y][x] != 0) {
                                if ((((p->sprites[i][3] >> 7) & 1) == 0) || (p->display[p->sprites[i][0] + y - 16][p->sprites[i][1] + x - 8] == bg_palette[0]))
                                    p->display[p->sprites[i][0] + y - 16][p->sprites[i][1] + x - 8] = s_palette[sprite_tile[y][x]];
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        for (uint8_t y = 0; y < 144; y++) {
            for (uint8_t x = 0; x < 160; x++) {
                p->display[y][x] = 0;
            }
        }
    }
}
