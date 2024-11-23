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

#if defined(PLATFORM_NX)
#include <switch.h>

struct {
    uint32_t handle;
    uint32_t buffer[240][320];
    IrsImageTransferProcessorConfig config;
}switch_ir;

void initialize_camera() {
    irsInitialize();
    irsActivateIrsensor(1);
    irsGetIrCameraHandle(&switch_ir.handle, CONTROLLER_PLAYER_1);
    irsGetDefaultImageTransferProcessorConfig(&switch_ir.config);
    irsRunImageTransferProcessor(switch_ir.handle, &switch_ir.config, 0x100000);
}

bool capture_frame(uint8_t camera_matrix[112][128]) {
    IrsImageTransferProcessorState state;
    Result rc = irsGetImageTransferProcessorState(switch_ir.handle, &switch_ir.buffer, sizeof(switch_ir.buffer), &state);
    if (R_FAILED(rc)) {
        return false;
    }
    for (uint32_t y = 0; y < 112; y++) {
        for (uint32_t x = 0; x < 128; x++) {
            camera_matrix[y][x] = switch_ir.buffer[y << 1][x << 1];
        }
    }
}

void stop_camera(){
    irsStopImageProcessor(switch_ir.handle);
    irsExit();
}

#elif defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>

struct {
    uint8_t pixels[112][128][4];
    bool hasFrame;
}camera_wasm;

void cameraCallback(unsigned char* data) {
    memcpy(camera_wasm.pixels, data, sizeof(camera_wasm.pixels));
    camera_wasm.hasFrame = 1;
}

void initialize_camera(){
    camera_wasm.hasFrame = 0;
    EM_ASM(
        initCamera();
    );
}

bool capture_frame(uint8_t camera_matrix[112][128]) {
    if (!camera_wasm.hasFrame) {
        return false;
    }
    for (int y = 0; y < 112; y++) {
        for (int x = 0; x < 128; x++) {
            uint8_t r = camera_wasm.pixels[y][x][0];
            uint8_t g = camera_wasm.pixels[y][x][1];
            uint8_t b = camera_wasm.pixels[y][x][2];
            camera_matrix[y][x] = (uint8_t)(0.21 * r + 0.72 * g + 0.07 * b);
        }
    }

    return true;
}

void stop_camera(){
    // TODO: implement
}
#else
#include "openpnp-capture.h"

struct {
    CapContext context;
    CapStream stream;
    CapFormatInfo formatinfo;
    uint8_t *buffer;
    uint32_t buffer_size;
}camera_pnp;

void initialize_camera() {
    camera_pnp.context = Cap_createContext();
    camera_pnp.stream = Cap_openStream(camera_pnp.context, 0, 0);
    Cap_getFormatInfo(camera_pnp.context, 0, 0, &camera_pnp.formatinfo);
    camera_pnp.buffer_size = camera_pnp.formatinfo.width * camera_pnp.formatinfo.height * 3;
    camera_pnp.buffer = malloc(camera_pnp.buffer_size);
}

void stop_camera() {
    free(camera_pnp.buffer);
    Cap_closeStream(camera_pnp.context, camera_pnp.stream);
    Cap_releaseContext(camera_pnp.context);
}

void rgb888_to_monochrome(const uint8_t *image, int width, int height, uint8_t output[112][128]) {
    // Scaled dimensions
    int target_width = 128;
    int target_height = 112;

    // Calculate scaling factors
    float scale_x = (float)width / target_width;
    float scale_y = (float)height / target_height;

    for (int y = 0; y < target_height; y++) {
        for (int x = 0; x < target_width; x++) {
            // Calculate the corresponding pixel in the original image
            int orig_x = (int)(x * scale_x);
            int orig_y = (int)(y * scale_y);

            // Ensure we don't go out of bounds
            if (orig_x >= width) orig_x = width - 1;
            if (orig_y >= height) orig_y = height - 1;

            // Get the RGB values
            const uint8_t *pixel = image + (orig_y * width + orig_x) * 3;
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            // Convert to grayscale using the luminosity method
            uint8_t gray = (uint8_t)(0.21 * r + 0.72 * g + 0.07 * b);

            // Set the output pixel
            output[y][x] = gray;
        }
    }
}

bool capture_frame(uint8_t camera_matrix[112][128]) {
    if (!Cap_hasNewFrame(camera_pnp.context, camera_pnp.stream)) {
        return false;
    }
    Cap_captureFrame(camera_pnp.context, camera_pnp.stream, camera_pnp.buffer, camera_pnp.buffer_size);
    rgb888_to_monochrome(camera_pnp.buffer, camera_pnp.formatinfo.width, camera_pnp.formatinfo.height, camera_matrix);

    return true;
}

#endif

uint8_t pixels_color_orig[112][128];
int pixels_color[112][128];
int temp_buffer[112][128];

void capture_image() {

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

    capture_frame(pixels_color_orig);

    // Retrive image from the webcam and apply filtering
    for (int i = 0; i < 112; i++) {
        for (int j = 0; j < 128; j++) {
            int value = pixels_color_orig[i][j];
            value = ((value * EXPOSURE_bits) / 0x0300);
            value = 128 + ((value-128)/8);

            value = clamp(0, value, 255);

            pixels_color[i][j] = (value) - 128;
        }
    }

    // Filtering
    uint32_t filtering_mode = (N_bit<<3) | (VH_bits<<1) | E3_bit;

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
