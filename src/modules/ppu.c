#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include <stdint.h>
#include <string.h>

ppu video;

void oam_scan(cpu *c) {
    video.buffer_size = 0;
    uint8_t sprite_height = 8;
    if (video.obj_size)
        sprite_height = 16;
    for (int i = 0; (i < 40) && (video.buffer_size < 10); i++) {
        uint8_t sprite_y = c->memory[0xfe00 | (i << 2)];
        if ((video.scan_line + 16) >= sprite_y && (video.scan_line + 16) < sprite_y + sprite_height) {
            video.oam_buffer[video.buffer_size] = i;
            video.buffer_size++;
        }
    }
}

void fetch_bg_to_fifo(cpu *c) {
    uint8_t bgX = video.scx + (video.current_pixel + video.fifo.pixel_count);
    uint8_t bgY = video.scy + video.scan_line;
    uint16_t tile_id = video.vram[0][(video.bg_tilemap ? 0x1c00 : 0x1800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)];
    uint8_t tile_attribute = c->is_color ? video.vram[1][(video.bg_tilemap ? 0x1c00 : 0x1800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)] : 0;
    bool flip_X = ((tile_attribute & 0x20) != 0);
    bool flip_Y = ((tile_attribute & 0x40) != 0);
    bool priority = ((tile_attribute & 0x80) != 0);
    if (flip_Y) {
        bgY = ~(bgY & 0xf);
    }

    uint16_t tile_addr = ((tile_id << 4) | (video.bg_tiles || tile_id > 0x7f ? 0x0000 : 0x1000)) + ((bgY << 1) & 0x0f);
    uint8_t lowerTileData = video.vram[(tile_attribute >> 3) & 1 ? 1 : 0][tile_addr];
    uint8_t upperTileData = video.vram[(tile_attribute >> 3) & 1 ? 1 : 0][tile_addr + 1];

    for (int i = 0; i < 8; i++) {
        if (!flip_X) {
            video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> (7 - i)) & 1;
            video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> (7 - i) & 1) << 1;
        }
        else {
            video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> i) & 1;
            video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> i & 1) << 1;
        }
        video.fifo.pixels[video.fifo.pixel_count + i].palette = tile_attribute & 7;
        video.fifo.pixels[video.fifo.pixel_count + i].priority = priority;
    }

    video.fifo.pixel_count += 8;
}

void fetch_win_to_fifo(cpu *c) {
    uint8_t bgX = ((video.current_pixel + video.fifo.pixel_count) - video.wx) + 7;
    uint8_t bgY = video.window_internal_line;
    uint16_t tile_id = video.vram[0][(video.window_tilemap ? 0x1c00 : 0x1800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)];
    uint8_t tile_attribute = c->is_color ? video.vram[1][(video.window_tilemap ? 0x1c00 : 0x1800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)] : 0;
    bool flip_X = ((tile_attribute & 0x20) != 0);
    bool flip_Y = ((tile_attribute & 0x40) != 0);
    bool priority = ((tile_attribute & 0x80) != 0);
    if (flip_Y) {
        bgY = ~(bgY & 0xf);
    }

    uint16_t tile_addr = ((tile_id << 4) | (video.bg_tiles || tile_id > 0x7f ? 0x0000 : 0x1000)) + ((bgY << 1) & 0x0f);
    uint8_t lowerTileData = video.vram[(tile_attribute >> 3) & 1 ? 1 : 0][tile_addr];
    uint8_t upperTileData = video.vram[(tile_attribute >> 3) & 1 ? 1 : 0][tile_addr + 1];

    for (int i = 0; i < 8; i++) {
        if (!flip_X) {
            video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> (7 - i)) & 1;
            video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> (7 - i) & 1) << 1;
        }
        else {
            video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> i) & 1;
            video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> i & 1) << 1;
        }
        video.fifo.pixels[video.fifo.pixel_count + i].palette = tile_attribute & 7;
        video.fifo.pixels[video.fifo.pixel_count + i].priority = priority;
    }

    video.fifo.pixel_count += 8;
    if (video.fifo.window_wx7) {
        video.fifo.window_wx7 = false;
        for (int i = 0; i < video.fifo.pixel_count-1; i++) {
            video.fifo.pixels[i].value = video.fifo.pixels[i+(7-video.wx)].value;
            video.fifo.pixels[i].palette = video.fifo.pixels[i+(7-video.wx)].palette;
        }
        video.fifo.pixel_count -= (7-video.wx);
    }
}

void fetch_sprite_to_fifo(cpu *c) {
    uint16_t buffer[10];
    uint8_t buffer_size = 0;

    for (int i = 0; i < video.buffer_size; i++) {
        uint16_t sprite_addr = 0xfe00 | (video.oam_buffer[i] << 2);
        if (c->memory[sprite_addr | 1] == (video.current_pixel + 8)) {
            buffer[buffer_size] = sprite_addr;
            buffer_size++;
        }
    }
    if (buffer_size > 0) {
        for (int j = 0; j < buffer_size; j++) {
            uint16_t sprite_addr = buffer[j];
            uint8_t palette = c->cgb_mode ? ((c->memory[sprite_addr | 3] & 7) + 8) : (((c->memory[sprite_addr | 3] >> 4) & 1) + 1);
            bool vram_bank = c->is_color ? ((c->memory[sprite_addr | 3] & 0x08) != 0) : 0;
            bool flip_X = ((c->memory[sprite_addr | 3] & 0x20) != 0);
            bool flip_Y = ((c->memory[sprite_addr | 3] & 0x40) != 0);
            bool priority = ((c->memory[sprite_addr | 3] & 0x80) != 0);

            uint8_t tile_id = (!video.obj_size) ? c->memory[sprite_addr | 2] : c->memory[sprite_addr | 2] & 0xfe;
            uint8_t tile_y = (video.scan_line + 16) - c->memory[sprite_addr];
            if (flip_Y) {
                tile_y = (~tile_y) & ((video.obj_size) ? 0xf : 0x7);
            }
            uint16_t tile_addr = ((tile_id << 4) + ((tile_y << 1) & 0x1f));
            uint8_t lowerTileData = video.vram[vram_bank][tile_addr];
            uint8_t upperTileData = video.vram[vram_bank][tile_addr + 1];

            for (int i = 0; i < 8; i++) {
                bool oam_priority = false;
                uint8_t new_value;
                if (!flip_X) {
                    new_value = (lowerTileData >> (7 - i)) & 1;
                    new_value |= (upperTileData >> (7 - i) & 1) << 1;
                } else {
                    new_value = (lowerTileData >> i) & 1;
                    new_value |= (upperTileData >> i & 1) << 1;
                }
                if (!video.opri && (video.fifo.pixels_sprite[i].oam_priority > sprite_addr) && new_value != 0) {
                    oam_priority = true;
                }

                if (video.fifo.pixels_sprite[i].value == 0 || i >= video.fifo.pixel_sprite_count || oam_priority) {
                    video.fifo.pixels_sprite[i].value = new_value;
                    video.fifo.pixels_sprite[i].palette = palette;
                    video.fifo.pixels_sprite[i].priority = priority;
                    video.fifo.pixels_sprite[i].oam_priority = sprite_addr;
                }
            }
            video.fifo.tick_pause += 6;
            video.fifo.pixel_sprite_count = 8;
        }
    }
}

void fetch_sprite_to_fifo_minus_8(cpu *c) {
    for (int i = 1; i < 8; i++) {
        uint16_t buffer[10];
        uint8_t buffer_size = 0;

        for (int j = 0; j < video.buffer_size; j++) {
            uint16_t sprite_addr = 0xfe00 | (video.oam_buffer[j] << 2);
            if (c->memory[sprite_addr | 1] == i) {
                buffer[buffer_size] = sprite_addr;
                buffer_size++;
            }
        }
        if (buffer_size > 0) {
            for (int j = 0; j < buffer_size; j++) {
                uint16_t sprite_addr = buffer[j];
                uint8_t palette = c->cgb_mode ? ((c->memory[sprite_addr | 3] & 7) + 8) : (((c->memory[sprite_addr | 3] >> 4) & 1) + 1);
                bool vram_bank = c->is_color ? ((c->memory[sprite_addr | 3] & 0x08) != 0) : 0;
                bool flip_X = ((c->memory[sprite_addr | 3] & 0x20) != 0);
                bool flip_Y = ((c->memory[sprite_addr | 3] & 0x40) != 0);
                bool priority = ((c->memory[sprite_addr | 3] & 0x80) != 0);

                uint8_t tile_id = (!video.obj_size) ? c->memory[sprite_addr | 2] : c->memory[sprite_addr | 2] & 0xfe;
                uint8_t tile_y = (video.scan_line + 16) - c->memory[sprite_addr];
                if (flip_Y) {
                    tile_y = (~tile_y) & ((video.obj_size) ? 0xf : 0x7);
                }
                uint16_t tile_addr = ((tile_id << 4) + ((tile_y << 1) & 0x1f));
                uint8_t lowerTileData = video.vram[vram_bank][tile_addr];
                uint8_t upperTileData = video.vram[vram_bank][tile_addr + 1];

                for (int k = 0; k < i; k++) {
                    bool oam_priority = false;
                    uint8_t new_value;
                    if (!flip_X) {
                        new_value = (lowerTileData >> (i - k - 1)) & 1;
                        new_value |= (upperTileData >> (i - k - 1) & 1) << 1;
                    } else {
                        new_value = (lowerTileData >> (k+(8-i))) & 1;
                        new_value |= (upperTileData >> (k+(8-i)) & 1) << 1;
                    }
                    if (!video.opri && (video.fifo.pixels_sprite[i].oam_priority > sprite_addr) && new_value != 0) {
                        oam_priority = true;
                    }

                    if (video.fifo.pixels_sprite[k].value == 0 || k >= video.fifo.pixel_sprite_count || oam_priority) {
                        video.fifo.pixels_sprite[k].value = new_value;
                        video.fifo.pixels_sprite[k].palette = palette;
                        video.fifo.pixels_sprite[k].priority = priority;
                        video.fifo.pixels_sprite[k].oam_priority = (uint8_t) (sprite_addr);
                    }
                }
                video.fifo.tick_pause += 6;
                video.fifo.pixel_sprite_count = i;
            }
        }
    }
}

void push_pixel(cpu *c) {
    // Mix Pixel
    pixel last_pixel = video.fifo.pixels[0];
    if (c->cgb_mode) {
        if (video.fifo.pixel_sprite_count > 0) {
            if (video.fifo.pixels_sprite[0].value != 0) {
                if (!video.bg_enable || (!video.fifo.pixels_sprite[0].priority && !last_pixel.priority) || last_pixel.value == 0) {
                    last_pixel.value = video.fifo.pixels_sprite[0].value;
                    last_pixel.palette = video.fifo.pixels_sprite[0].palette;
                }
            }
        }
    }
    else {
        if (video.fifo.pixel_sprite_count > 0) {
            if (video.fifo.pixels_sprite[0].value != 0 && last_pixel.palette == 0) {
                if (!video.fifo.pixels_sprite[0].priority || last_pixel.value == 0) {
                    last_pixel.value = video.fifo.pixels_sprite[0].value;
                    last_pixel.palette = video.fifo.pixels_sprite[0].palette;
                }
            }
        }
    }

    // Copy pixel to display
    if (c->is_color) {
        if (c->cgb_mode) {
            uint8_t px_addr = (last_pixel.value + (last_pixel.palette << 2)) << 1;
            if (px_addr < 64) {
                video.line[video.current_pixel] = (video.bgp[px_addr | 1] << 8) | (video.bgp[px_addr]);
            } else {
                px_addr &= 0x3f;
                video.line[video.current_pixel] = (video.obp[px_addr | 1] << 8) | (video.obp[px_addr]);
            }
        }
        else {
            uint8_t px_addr;
            if (last_pixel.palette == 0 && video.bg_enable) {
                px_addr = video.bgp_dmg[last_pixel.value] << 1;
                video.line[video.current_pixel] = (video.bgp[px_addr | 1] << 8) | (video.bgp[px_addr]);
            }
            else if (last_pixel.palette == 1) {
                px_addr = video.obp_dmg[0][last_pixel.value] << 1;
                video.line[video.current_pixel] = (video.obp[px_addr | 1] << 8) | (video.obp[px_addr]);
            }
            else if (last_pixel.palette == 2) {
                px_addr = ((video.obp_dmg[1][last_pixel.value]) + 4) << 1;
                video.line[video.current_pixel] = (video.obp[px_addr | 1] << 8) | (video.obp[px_addr]);
            }
            else {
                px_addr = video.bgp_dmg[0] << 1;
                video.line[video.current_pixel] = (video.bgp[px_addr | 1] << 8) | (video.bgp[px_addr]);
            }
        }
    }

    else if (video.bg_enable && last_pixel.palette == 0) {
        video.line[video.current_pixel] = video.bgp_dmg[last_pixel.value];
    } else if (last_pixel.palette != 0) {
        video.line[video.current_pixel] = video.obp_dmg[last_pixel.palette - 1][last_pixel.value];
    } else {
        video.line[video.current_pixel] = video.bgp_dmg[0];
    }


    // Shift FIFO
    for (int i = 0; i < video.fifo.pixel_count-1; i++) {
        video.fifo.pixels[i].value = video.fifo.pixels[i+1].value;
        video.fifo.pixels[i].palette = video.fifo.pixels[i+1].palette;
        video.fifo.pixels[i].priority = video.fifo.pixels[i+1].priority;
    }
    video.fifo.pixel_count--;

    // Advance current pixel index
    if (video.fifo.scx_init > 0 && !video.in_window) {
        video.fifo.scx_init--;
    }
    else {
        video.current_pixel++;
        if (video.fifo.pixel_sprite_count > 0) {
            for (int i = 0; i < video.fifo.pixel_sprite_count-1; i++) {
                video.fifo.pixels_sprite[i].value = video.fifo.pixels_sprite[i+1].value;
                video.fifo.pixels_sprite[i].palette = video.fifo.pixels_sprite[i+1].palette;
                video.fifo.pixels_sprite[i].priority = video.fifo.pixels_sprite[i+1].priority;
                video.fifo.pixels_sprite[i].oam_priority = video.fifo.pixels_sprite[i+1].oam_priority;
            }
            video.fifo.pixel_sprite_count--;
        }
    }
}

void operate_fifo(cpu *c) {
    if (video.fifo.init_timer > 0) {
        video.fifo.init_timer--;
        if (video.fifo.init_timer == 0) {
            fetch_bg_to_fifo(c);
            if (video.obj_enable) {
                fetch_sprite_to_fifo_minus_8(c);
            }
            video.fifo.scx_init = video.scx % 8;
        }
        return;
    }

    if (video.fifo.tick_pause > 0) {
        video.fifo.tick_pause--;
        return;
    }
    if (video.current_pixel < 160) {
        if (!video.in_window && video.window_enable && video.wy_trigger && video.current_pixel + 8 > video.wx) {
            video.in_window = true;
            video.fifo.win_timer = 6;
            video.fifo.pixel_count = 0;
            if (video.wx < 7)
                video.fifo.window_wx7 = true;
        }
        if (!video.in_window) {
            while (video.fifo.pixel_count <= 8)
                fetch_bg_to_fifo(c);
            if (video.obj_enable) {
                fetch_sprite_to_fifo(c);
            }
            push_pixel(c);
        }
        else {
            while (video.fifo.pixel_count <= 8)
                fetch_win_to_fifo(c);
            if (video.fifo.win_timer > 0) {
                video.fifo.win_timer--;
            }
            else {
                if (video.obj_enable) {
                    fetch_sprite_to_fifo(c);
                }
                push_pixel(c);
            }
        }
    }
    else {
        if (video.mode != 0) {
            video.mode = 0;
            video.fifo.pixel_count = 0;
            video.fifo.pixel_sprite_count = 0;
            if (video.in_window)
                video.window_internal_line++;
            video.in_window = false;
            video.fifo.tick_pause = 0;
            if (!c->hdma.finished && c->hdma.mode == 1) {
                for (int i = 0; i < 16; i++) {
                    hdma_transfer(c);
                }
            }
        }
    }
}

void load_line() {
    if (video.is_on) {
        for (int i = 0; i < 160; i++) {
            video.display[video.scan_line][i] = video.line[i];
        }
    }
    else {
        memset(video.display, 0, 160*144*sizeof(uint8_t));
    }
}
