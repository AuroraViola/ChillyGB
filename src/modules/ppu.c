#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include <stdint.h>
#include <string.h>

ppu video;

uint16_t get_tile_id(uint16_t bg_tile) {
    uint16_t tile_id = video.tilemap[video.bg_tilemap][bg_tile];
    return tile_id + ((!video.bg_tiles && tile_id < 128) ? 256 : 0);
}

void load_background() {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    video.background[(i*8)+x][(j*8)+y] = video.tiles[get_tile_id((i*32) + j)][0][x][y];
                    video.background[(i*8)+x][(j*8)+y] |= video.tiles[get_tile_id((i*32) + j)][1][x][y] << 1;
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

void push_pixels(cpu *c) {
    if (video.fifo.init_timer > 0) {
        video.fifo.init_timer--;
        if (video.fifo.init_timer == 0) {
            video.fifo.scx_init = c->memory[SCX] % 8;
        }
        return;
    }
    if (video.fifo.scx_init > 0) {
        video.fifo.scx_init--;
        return;
    }
    if (video.fifo.current_pixel < 160) {
        video.fifo.current_pixel++;
    }
    else {
        video.mode = 0;
    }
}

void load_display(cpu *c) {
    if (video.need_bg_wn_reload) {
        video.need_bg_wn_reload = false;
        load_background();
    }
    if (video.is_on) {
        // Background
        if (video.bg_enable) {
            int y = video.scan_line;
            for (uint8_t x = 0; x < 160; x++) {
                video.display[y][x] = video.bgp[video.background[(uint8_t) (y + video.scy)][(uint8_t) (x + video.scx)]];
            }
        }
        else  {
            int y = video.scan_line;
            memset(&video.display[y][0], 0, 160 * sizeof(uint8_t));
        }
    }
    else {
        memset(video.display, 0, 160*144*sizeof(uint8_t));
    }
}
