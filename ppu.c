#include "cpu.h"
#include "ppu.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ppu video;

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

void decode_tile_16(cpu *c, uint16_t addr, uint8_t tile_16[16][8]) {
    uint8_t encoded_tile[32];
    addr &= 0xffe0;
    for (int i = 0; i < 32; i++) {
        encoded_tile[i] = c->memory[addr + i];
    }

    for (int y = 0; y < 16; y++) {
        uint8_t byte1 = encoded_tile[y * 2];
        uint8_t byte2 = encoded_tile[y * 2 + 1];
        for (int x = 0; x < 8; x++) {
            uint8_t bit1 = (byte1 >> (7-x)) & 1;
            uint8_t bit2 = (byte2 >> (7-x)) & 1;
            tile_16[y][x] = (bit2 << 1) | bit1;
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

void flipY_tile_16(uint8_t tile[16][8]) {
    uint8_t tilecopy[16][8];
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tilecopy[i][j] = tile[i][j];
        }
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tile[i][j] = tilecopy[15-i][j];
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

void flipX_tile_16(uint8_t tile[16][8]) {
    uint8_t tilecopy[16][8];
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tilecopy[i][j] = tile[i][j];
        }
    }

    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            tile[i][j] = tilecopy[i][7-j];
        }
    }
}

void load_sprites(cpu *c) {
    for (int i = 0; i < 40; i++) {
        video.sprites[i].y = c->memory[0xfe00 + (i*4)];
        video.sprites[i].x = c->memory[0xfe00 + (i*4) + 1];
        uint16_t tile_index = 0x8000 | ((uint16_t)(c->memory[0xfe00 + (i*4) + 2]) << 4);
        decode_tile(c, tile_index, video.sprites[i].tile);
        decode_tile_16(c, tile_index, video.sprites[i].tile_16);
        video.sprites[i].priority = (c->memory[0xfe00 + (i*4) + 3] >> 7) & 1;

        if (((c->memory[0xfe00 + (i*4) + 3] >> 5) & 1) != 0) {
            flipX_tile(video.sprites[i].tile);
            flipX_tile_16(video.sprites[i].tile_16);
        }

        if (((c->memory[0xfe00 + (i*4) + 3] >> 6) & 1) != 0) {
            flipY_tile(video.sprites[i].tile);
            flipY_tile_16(video.sprites[i].tile_16);
        }

        video.sprites[i].palette = (c->memory[0xfe00 + (i*4) + 3] >> 4) & 1;
    }
}

void load_tiles(cpu *c) {
    if (video.bg_tiles) {
        for (int i = 0; i < 256; i++) {
            decode_tile(c, (0x8000 + (i << 4)), video.tiles[i]);
        }
    }
    else {
        for (int i = 0; i < 128; i++) {
            decode_tile(c, (0x9000 | (i << 4)), video.tiles[i]);
        }
        for (int i = 128; i < 256; i++) {
            decode_tile(c, (0x8800 | (i << 4)), video.tiles[i]);
        }
    }
}

void load_tilemap(cpu *c) {
    for (int i = 0; i < 1024 ; i++) {
        video.tilemap[0][i] = c->memory[i + 0x9800];
    }
    for (int i = 0; i < 1024 ; i++) {
        video.tilemap[1][i] = c->memory[i + 0x9c00];
    }
}

void load_background() {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    video.background[i*8 + y][j*8 + x] = video.tiles[video.tilemap[video.bg_tilemap][32*i+j]][y][x];
                }
            }
        }
    }
}

void load_window() {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    video.window[i*8 + y][j*8 + x] = video.tiles[video.tilemap[video.window_tilemap][32*i+j]][y][x];
                }
            }
        }
    }
}

void load_sprite_displays() {
    memset(video.sprite_display, 0, 2*176*176*sizeof(sprite_px));
    for (int i = 0; i < 40; i++) {
        if (video.sprites[i].x < 176 && video.sprites[i].y < 176) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    if (video.sprites[i].y + y < 176 && video.sprites[i].x + x < 176 && video.sprite_display[0][video.sprites[i].y + y][video.sprites[i].x + x].color == 0) {
                        video.sprite_display[0][video.sprites[i].y + y][video.sprites[i].x + x].color = video.sprites[i].tile[y][x];
                        video.sprite_display[0][video.sprites[i].y + y][video.sprites[i].x + x].palette = video.sprites[i].palette;
                        video.sprite_display[0][video.sprites[i].y + y][video.sprites[i].x + x].priority = video.sprites[i].priority;
                    }
                }
                if (video.sprites[i].y + y < 176)
                    video.sprite_display[0][video.sprites[i].y + y][video.sprites[i].x].initial_obj_px = true;
            }
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 8; x++) {
                    if (video.sprites[i].y + y < 176 && video.sprites[i].x + x < 176 && video.sprite_display[1][video.sprites[i].y + y][video.sprites[i].x + x].color == 0) {
                        video.sprite_display[1][video.sprites[i].y + y][video.sprites[i].x +x].color = video.sprites[i].tile_16[y][x];
                        video.sprite_display[1][video.sprites[i].y + y][video.sprites[i].x + x].palette = video.sprites[i].palette;
                        video.sprite_display[1][video.sprites[i].y + y][video.sprites[i].x + x].priority = video.sprites[i].priority;
                    }
                }
                if (video.sprites[i].y + y < 176)
                    video.sprite_display[1][video.sprites[i].y + y][video.sprites[i].x].initial_obj_px = true;
            }
        }
    }
}

uint16_t get_mode3_duration(cpu *c) {
    uint16_t duration = c->memory[SCX] % 8;
    if (c->memory[WX] < 167 && video.window_enable && video.wy_trigger)
        duration += 6;
    if (video.obj_enable) {
        int y = video.scan_line;
        for (uint8_t x = 0; x < 176; x++) {
            if (video.sprite_display[video.obj_size][y][x].initial_obj_px) {
                if (x == 0)
                    duration += 11;
                else
                    duration += 6;
            }
        }
    }
    return duration;
}

void load_display(cpu *c) {
    uint8_t bg_pal = c->memory[BGP];
    uint8_t obp0 = c->memory[OBP0];
    uint8_t obp1 = c->memory[OBP1];
    uint8_t bg_palette[] = { (bg_pal & 3), ((bg_pal & 12) >> 2), ((bg_pal & 48) >> 4), (bg_pal >> 6) };
    uint8_t obp0_palette[] = { (obp0 & 3), ((obp0 & 12) >> 2), ((obp0 & 48) >> 4), (obp0 >> 6) };
    uint8_t obp1_palette[] = { (obp1 & 3), ((obp1 & 12) >> 2), ((obp1 & 48) >> 4), (obp1 >> 6) };

    if (video.tiles_write) {
        video.tiles_write = false;
        load_tiles(c);
    }
    if (video.tilemap_write) {
        video.tilemap_write = false;
        load_tilemap(c);
    }
    if (video.need_bg_wn_reload) {
        video.need_bg_wn_reload = false;
        load_background();
        load_window();
    }
    if (video.need_sprites_reload) {
        video.need_sprites_reload = false;
        load_sprites(c);
        load_sprite_displays();
    }

    if (video.is_on) {
        // Background
        if (video.bg_enable) {
            uint8_t scx = c->memory[SCX];
            uint8_t scy = c->memory[SCY];
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                video.display[y][x] = bg_palette[video.background[(uint8_t) (y + scy)][(uint8_t) (x + scx)]];
            }
        }
        else {
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                video.display[y][x] = 0;
            }
        }
        // Window
        if (video.window_enable && video.wy_trigger) {
            uint8_t wy = c->memory[WY];
            uint8_t wx = c->memory[WX];
            int y = video.scan_line;
            if (wx < 167) {
                for (uint8_t x = 0; x < 167; x++) {
                    if ((y >= wy) && ((x + wx - 7) < 160) && ((x + wx - 7) >= 0)) {
                        video.display[y][x + wx - 7] = bg_palette[video.window[video.window_internal_line][x]];
                    }
                }
                video.window_internal_line++;
            }
        }
        // Sprites
        if (video.obj_enable) {
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                if (video.sprite_display[video.obj_size][y+16][x+8].color != 0) {
                    if (video.sprite_display[video.obj_size][y+16][x+8].palette) {
                        if (!video.sprite_display[video.obj_size][y + 16][x + 8].priority) {
                            video.display[y][x] = obp1_palette[video.sprite_display[video.obj_size][y + 16][x + 8].color];
                        }
                        else {
                            if (video.display[y][x] == bg_palette[0]) {
                                video.display[y][x] = obp1_palette[video.sprite_display[video.obj_size][y + 16][x + 8].color];
                            }
                        }
                    }
                    else {
                        if (!video.sprite_display[video.obj_size][y + 16][x + 8].priority) {
                            video.display[y][x] = obp0_palette[video.sprite_display[video.obj_size][y + 16][x + 8].color];
                        }
                        else {
                            if (video.display[y][x] == bg_palette[0]) {
                                video.display[y][x] = obp0_palette[video.sprite_display[video.obj_size][y + 16][x + 8].color];
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
                video.display[y][x] = 0;
            }
        }
    }
}
