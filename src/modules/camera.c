#include "../includes/cpu.h"
#include "../includes/camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

camera gbcamera;

void callback(sr_webcam_device* device, void* data) {
    //memcpy(gbcamera.buffer, data, sr_webcam_get_format_size(device));
    memcpy(gbcamera.pixels, data, sr_webcam_get_format_size(device));
    gbcamera.hasFrame = 1;
}

void initialize_camera() {
    gbcamera.hasFrame = 0;
    sr_webcam_create(&gbcamera.device, 0);
    sr_webcam_set_format(gbcamera.device, 320, 240, 5);
    sr_webcam_set_callback(gbcamera.device, callback);
    sr_webcam_open(gbcamera.device);

    sr_webcam_get_dimensions(gbcamera.device, &gbcamera.vidW, &gbcamera.vidH);
    sr_webcam_get_framerate(gbcamera.device, &gbcamera.vidFps);
    int vidSize = sr_webcam_get_format_size(gbcamera.device);
    gbcamera.buffer	= (unsigned char*)calloc(vidSize, sizeof(unsigned char));

    sr_webcam_start(gbcamera.device);
}

void capture_image() {
    if (gbcamera.hasFrame > 0) {
        for (int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                uint8_t color = gbcamera.pixels[i+8][j+16][0] >> 6;
                gbcamera.pixels_color[i][j] = color;
            }
        }
        for(int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                uint8_t outcolor = 3- gbcamera.pixels_color[i][j];

                uint8_t *tile_base = gbcamera.gb_pixels[i >> 3][j >> 3];
                tile_base = &tile_base[(i & 7) * 2];

                if (outcolor & 1)
                    tile_base[0] |= 1 << (7 - (7 & j));
                else
                    tile_base[0] &= ~(1 << (7 - (7 & j)));
                if (outcolor & 2)
                    tile_base[1] |= 1 << (7 - (7 & j));
                else
                    tile_base[1] &= ~(1 << (7 - (7 & j)));
            }
        }
        gbcamera.hasFrame = 0;
    }
}

void take_picture(cpu *c) {
    if (gbcamera.timing <= 0) {
        int32_t N_bit = (gbcamera.reg[1] & 128) ? 0 : 512;
        int32_t exposure = ((gbcamera.reg[2] << 8) | gbcamera.reg[3]);
        gbcamera.timing = 32446 + N_bit + (exposure << 4);

        capture_image();
        memcpy(&(c->cart.ram[0][0x0100]),gbcamera.gb_pixels,sizeof(gbcamera.gb_pixels));
    }
}
