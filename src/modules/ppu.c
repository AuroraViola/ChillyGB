#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include <stdint.h>
#include <string.h>

ppu video;

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

void fetch_to_fifo(cpu *c) {
    uint8_t bgX = video.scx + video.current_pixel;
    uint8_t bgY = video.scy + video.scan_line;
    uint16_t tile_id = c->memory[(video.bg_tilemap ? 0x9c00 : 0x9800) | (((uint16_t)bgY & 0x00f8) << 2) | (bgX >> 3)];
    uint16_t tile_addr = (0x8000 | (tile_id << 4) | (video.bg_tiles || tile_id > 0x7f ? 0x0000 : 0x1000)) + ((bgY << 1) & 0x0f);
    uint8_t lowerTileData = c->memory[tile_addr];
    uint8_t upperTileData = c->memory[tile_addr+1];

    for (int i = 0; i < 8; i++) {
        video.fifo.values[video.fifo.pixel_count + i] = (lowerTileData >> (7-i)) & 1;
        video.fifo.values[video.fifo.pixel_count + i] |= (upperTileData >> (7-i) & 1) << 1;
    }

    if (video.fifo.pixel_count == 0) {
        video.current_pixel += 8;
    }
    video.fifo.pixel_count += 8;
}

void push_pixel() {
    video.line[video.current_pixel-8] = video.bgp[video.fifo.values[0]];
    for (int i = 0; i < video.fifo.pixel_count-1; i++) {
        video.fifo.values[i] = video.fifo.values[i+1];
    }
    video.fifo.pixel_count--;
    if (video.fifo.scx_init > 0) {
        video.fifo.scx_init--;
    }
    else {
        video.current_pixel++;
    }
}

void operate_fifo(cpu *c) {
    if (video.fifo.init_timer > 0) {
        video.fifo.init_timer--;
        if (video.fifo.init_timer == 0) {
            video.fifo.scx_init = video.scx % 8;
        }
        return;
    }
    if (video.current_pixel < 168) {
        while(video.fifo.pixel_count <= 8)
            fetch_to_fifo(c);
        push_pixel();
    }
    else {
        video.fifo.pixel_count = 0;
        video.mode = 0;
    }
}

void load_line(cpu *c) {
    if (video.is_on) {
        for (int i = 0; i < 160; i++) {
            video.display[video.scan_line][i] = video.line[i];
        }
    }
    else {
        memset(video.display, 0, 160*144*sizeof(uint8_t));
    }
}
