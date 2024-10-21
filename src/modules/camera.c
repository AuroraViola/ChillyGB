#include "../includes/cpu.h"
#include "../includes/camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

camera gbcamera;

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int clamp(int min, int value, int max) {
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

uint8_t gb_cam_matrix_process(int value, int x, int y) {
    x = x & 3;
    y = y & 3;

    int base = 6 + (y*4 + x) * 3;

    int r0 = gbcamera.reg[base+0];
    int r1 = gbcamera.reg[base+1];
    int r2 = gbcamera.reg[base+2];

    if(value < r0) return 0x00;
    else if(value < r1) return 0x40;
    else if(value < r2) return 0x80;
    return 0xC0;
}

void callback(sr_webcam_device* device, void* data) {
    //memcpy(gbcamera.buffer, data, sr_webcam_get_format_size(device));
    memcpy(gbcamera.pixels, data, sr_webcam_get_format_size(device));
    gbcamera.hasFrame = 1;
}

void initialize_camera() {
    gbcamera.hasFrame = 0;
    sr_webcam_create(&gbcamera.device, 0);
    sr_webcam_set_format(gbcamera.device, 320, 240, 30);
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
        // Register 0
        uint32_t P_bits = 0;
        uint32_t M_bits = 0;

        switch((gbcamera.reg[0]>>1)&3 ) {
            case 0:
                P_bits = 0x00; M_bits = 0x01;
                break;
            case 1:
                P_bits = 0x01; M_bits = 0x00;
                break;
            case 2 ... 3:
                P_bits = 0x01; M_bits = 0x02;
                break;
            default:
                break;
        }

        // Register 1
        uint32_t N_bit = (gbcamera.reg[1] & 128) >> 7;
        uint32_t VH_bits = (gbcamera.reg[1] & 96) >> 5;

        // Registers 2 and 3
        uint32_t EXPOSURE_bits = gbcamera.reg[3] | (gbcamera.reg[2]<<8);

        // Register 4
        const float edge_ratio_lut[8] = {0.50, 0.75, 1.00, 1.25, 2.00, 3.00, 4.00, 5.00};

        float EDGE_alpha = edge_ratio_lut[(gbcamera.reg[4] & 0x70)>>4];

        uint32_t E3_bit = (gbcamera.reg[4] & 128) >> 7;
        uint32_t I_bit = (gbcamera.reg[4] & 8) >> 3;

        // Retrive image from the webcam and apply filtering
        int pixels_color[112][128];
        for (int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                int value = gbcamera.pixels[i+8][j+16][0];
                value = ((value * EXPOSURE_bits) / 0x0300);
                value = 128 + ((value-128)/8);

                if(value < 0)
                    value = 0;
                else if(value > 255)
                    value = 255;

                pixels_color[i][j] = (value) - 128;
            }
        }

        // Filtering
        uint32_t filtering_mode = (N_bit<<3) | (VH_bits<<1) | E3_bit;
        printf("filtering_mode: %x\n", filtering_mode);

        int temp_buffer[112][128];
        switch (filtering_mode) {
            case 0x0:
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        temp_buffer[i][j] = pixels_color[i][j];
                    }
                }
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        int ms = temp_buffer[min(i+1,112-1)][j];
                        int px = temp_buffer[i][j];

                        int value = 0;
                        if(P_bits & 1) value += px;
                        if(P_bits & 2) value += ms;
                        if(M_bits & 1) value -= px;
                        if(M_bits & 2) value -= ms;
                        pixels_color[i][j] = clamp(-128,value,127);
                    }
                }
                break;
            case 0x2:
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        int mw = pixels_color[i][max(0,j-1)];
                        int me = pixels_color[i][min(j+1,128-1)];
                        int px = pixels_color[i][j];

                        temp_buffer[i][j] = clamp(0,px+((2*px-mw-me)*EDGE_alpha),255);
                    }
                }
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        int ms = temp_buffer[min(i+1,112-1)][j];
                        int px = temp_buffer[i][j];

                        int value = 0;
                        if(P_bits & 1) value += px;
                        if(P_bits & 2) value += ms;
                        if(M_bits & 1) value -= px;
                        if(M_bits & 2) value -= ms;
                        pixels_color[i][j] = clamp(-128,value,127);
                    }
                }
                break;
            case 0xe:
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        int ms = pixels_color[min(i + 1, 112 - 1)][j];
                        int mn = pixels_color[max(0, i - 1)][j];
                        int mw = pixels_color[i][max(0, j - 1)];
                        int me = pixels_color[i][min(j + 1, 128 - 1)];
                        int px = pixels_color[i][j];

                        temp_buffer[i][j] = clamp(-128, px + ((4 * px - mw - me - mn - ms) * EDGE_alpha), 127);
                    }
                    for (int j = 0; j < 128; j++) {
                        pixels_color[i][j] = temp_buffer[i][j];
                    }
                }
                break;
            case 0x1:
                for(int i = 0; i < 112; i++) {
                    for (int j = 0; j < 128; j++) {
                        pixels_color[i][j] = 0;
                    }
                }
                break;
            default:
                break;
        }

        for(int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                pixels_color[i][j] += 128;
            }
        }

        for(int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                pixels_color[i][j] = gb_cam_matrix_process(pixels_color[i][j], i, j);
            }
        }

        for(int i = 0; i < 112; i++) {
            for (int j = 0; j < 128; j++) {
                uint8_t outcolor = 3 - ((uint8_t)(pixels_color[i][j]) >> 6);

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
