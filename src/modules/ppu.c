#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include <stdint.h>
#include <string.h>

ppu video;

void decode_tile(cpu *c, uint16_t addr, uint8_t tile[8][8]) {
    uint8_t encoded_tile[16];
    memcpy(encoded_tile, &c->memory[addr], 16);

    for (int y = 0; y < 8; y++) {
        uint8_t byte1 = encoded_tile[y * 2];
        uint8_t byte2 = encoded_tile[y * 2 + 1];
        tile[y][0] = ((byte2 >> 7) & 1) << 1 | ((byte1 >> 7) & 1);
        tile[y][1] = ((byte2 >> 6) & 1) << 1 | ((byte1 >> 6) & 1);
        tile[y][2] = ((byte2 >> 5) & 1) << 1 | ((byte1 >> 5) & 1);
        tile[y][3] = ((byte2 >> 4) & 1) << 1 | ((byte1 >> 4) & 1);
        tile[y][4] = ((byte2 >> 3) & 1) << 1 | ((byte1 >> 3) & 1);
        tile[y][5] = ((byte2 >> 2) & 1) << 1 | ((byte1 >> 2) & 1);
        tile[y][6] = ((byte2 >> 1) & 1) << 1 | ((byte1 >> 1) & 1);
        tile[y][7] = (byte2 & 1) << 1 | (byte1 & 1);
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

void load_background() {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                memcpy(&video.background[i*8 + y][j*8], &video.tiles[video.tilemap[video.bg_tilemap][32*i+j]][y][0], 8);
            }
        }
    }
}

void load_window() {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int y = 0; y < 8; y++) {
                memcpy(&video.window[i*8 + y][j*8], &video.tiles[video.tilemap[video.window_tilemap][32*i+j]][y][0], 8);
            }
        }
    }
}

uint8_t get_sprite_row(int sprite_index) {
    return (video.scan_line + 16) - video.sprites[sprite_index].y;
}

void load_sprite_line() {
    memset(video.sprite_line, 0, 176*sizeof(sprite_px));
    for (int i = 0; i < video.buffer_size; i++) {
        uint8_t y = get_sprite_row(video.oam_buffer[i]);
        if (y >= 16) {
            continue;
        }
        sprite selected_sprite = video.sprites[video.oam_buffer[i]];
        if (!video.obj_size) {
            for (int x = 0; x < 8; x++) {
                if ((selected_sprite.x + x) < 176) {
                    if (selected_sprite.x < video.sprite_line[selected_sprite.x + x].x || video.sprite_line[selected_sprite.x + x].x == 0) {
                        if (selected_sprite.tile[y][x] != 0) {
                            video.sprite_line[selected_sprite.x + x].color = selected_sprite.tile[y][x];
                            video.sprite_line[selected_sprite.x + x].priority = selected_sprite.priority;
                            video.sprite_line[selected_sprite.x + x].palette = selected_sprite.palette;
                            video.sprite_line[selected_sprite.x + x].x = selected_sprite.x;
                        }
                    }
                }
            }
        }
        else {
            for (int x = 0; x < 8; x++) {
                if ((selected_sprite.x + x) < 176) {
                    if (selected_sprite.x < video.sprite_line[selected_sprite.x + x].x || video.sprite_line[selected_sprite.x + x].x == 0) {
                        if (selected_sprite.tile_16[y][x] != 0) {
                            video.sprite_line[selected_sprite.x + x].color = selected_sprite.tile_16[y][x];
                            video.sprite_line[selected_sprite.x + x].priority = selected_sprite.priority;
                            video.sprite_line[selected_sprite.x + x].palette = selected_sprite.palette;
                            video.sprite_line[selected_sprite.x + x].x = selected_sprite.x;
                        }
                    }
                }
            }
        }
    }
}

void oam_scan() {
    video.buffer_size = 0;
    uint8_t sprite_height = 8;
    if (video.obj_size)
        sprite_height = 16;
    for (int i = 0; (i < 40) && (video.buffer_size < 10); i++) {
        if ((video.scan_line + 16) >= video.sprites[i].y && (video.scan_line + 16) < video.sprites[i].y + sprite_height) {
            video.oam_buffer[video.buffer_size] = i;
            video.buffer_size++;
        }
    }
}

uint16_t get_mode3_duration(cpu *c) {
    uint16_t duration = c->memory[SCX] % 8;
    if (c->memory[WX] < 167 && video.window_enable && video.wy_trigger)
        duration += 6;
    return duration;
}

void push_pixels() {
    if (timer1.scanline_timer < 360) {
        video.current_pixel += 4;
        if (video.current_pixel >= 160) {
            video.current_pixel = 0;
            video.mode = 0;
        }
    }
}

void load_display(cpu *c) {
    if (video.tiles_write) {
        video.tiles_write = false;
        load_tiles(c);
    }
    if (video.need_bg_wn_reload) {
        video.need_bg_wn_reload = false;
        load_background();
        load_window();
    }
    if (video.need_sprites_reload) {
        video.need_sprites_reload = false;
        load_sprites(c);
    }
    load_sprite_line();

    if (video.is_on) {
        // Background
        if (video.bg_enable) {
            uint8_t scx = c->memory[SCX];
            uint8_t scy = c->memory[SCY];
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                video.display[y][x] = video.bgp[video.background[(uint8_t) (y + scy)][(uint8_t) (x + scx)]];
            }
        }
        else {
            int y = video.scan_line;
            memset(&video.display[y][0], 0, 160 * sizeof(uint8_t));
        }
        // Window
        if (video.window_enable && video.wy_trigger) {
            uint8_t wy = c->memory[WY];
            uint8_t wx = c->memory[WX];
            int y = video.scan_line;
            if (wx < 167) {
                for (uint8_t x = 0; x < 167; x++) {
                    if ((y >= wy) && ((x + wx - 7) < 160) && ((x + wx - 7) >= 0)) {
                        video.display[y][x + wx - 7] = video.bgp[video.window[video.window_internal_line][x]];
                    }
                }
                video.window_internal_line++;
            }
        }
        // Sprites
        if (video.obj_enable) {
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                if (video.sprite_line[x+8].color != 0) {
                    if (!video.sprite_line[x + 8].priority) {
                        video.display[y][x] = video.obp[video.sprite_line[x+8].palette][video.sprite_line[x + 8].color];
                    }
                    else {
                        if (video.display[y][x] == video.bgp[0]) {
                            video.display[y][x] = video.obp[video.sprite_line[x+8].palette][video.sprite_line[x + 8].color];
                        }
                    }
                }
            }
        }
    }
    else {
        memset(video.display, 0, 160*144*sizeof(uint8_t));
    }
}
