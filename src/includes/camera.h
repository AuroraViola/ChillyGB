#include <stdint.h>
#include <stdbool.h>
#include "../../sr_webcam/include/sr_webcam.h"

#ifndef CHILLYGB_CAMERA_H
#define CHILLYGB_CAMERA_H

typedef struct {
    uint8_t reg[0x37];
    int32_t timing;

    sr_webcam_device* device;

    int vidW;
    int vidH;
    int vidFps;
    unsigned char* buffer;
    int hasFrame;
    uint8_t pixels[240][320][3];
    uint8_t pixels_color[112][128];
    uint8_t gb_pixels[14][16][16];
}camera;

extern camera gbcamera;

void initialize_camera();
void take_picture(cpu *c);

#endif //CHILLYGB_CAMERA_H
