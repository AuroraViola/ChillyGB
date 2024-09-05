#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

#ifndef CHILLYGB_PPU_H
#define CHILLYGB_PPU_H

typedef struct {
    uint8_t value;
    uint8_t palette;
    uint8_t priority;
}pixel;

typedef struct {
    pixel pixels[16];
    uint8_t pixel_count;
    uint8_t tick_pause;
    uint8_t init_timer;
    uint8_t win_timer;
    uint8_t scx_init;
    bool window_wx7;
}pixel_fifo;

typedef struct {
    uint8_t mode;
    uint8_t scan_line;
    uint8_t vram[2][0x2000];
    uint8_t display[144][160];
    uint8_t line[160];
    pixel_fifo fifo;
    uint8_t current_pixel;

    uint8_t oam_buffer[40];
    uint8_t buffer_size;

    uint8_t window_internal_line;
    bool wy_trigger;
    bool in_window;

    uint8_t scx;
    uint8_t scy;
    uint8_t wx;
    uint8_t wy;

    uint8_t bgp[64];
    bool bcps_inc;
    uint8_t bgp_addr;
    uint8_t obp[64];
    bool ocps_inc;
    uint8_t obp_addr;

    bool is_on;
    bool bg_enable;
    bool window_enable;
    bool obj_enable;

    bool bg_tiles;
    bool bg_tilemap;
    bool window_tilemap;
    bool obj_size;

    bool is_scan_line;
    bool draw_screen;
    bool dma_transfer;

    bool lyc_select;
    uint8_t mode_select;
    bool ly_eq_lyc;

    bool vram_bank;
}ppu;

extern ppu video;

void load_line();
void oam_scan(cpu *c);
void operate_fifo(cpu *c);

#endif //CHILLYGB_PPU_H
