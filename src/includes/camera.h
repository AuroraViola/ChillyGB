#include <stdint.h>
#include <stdbool.h>

#ifndef CHILLYGB_CAMERA_H
#define CHILLYGB_CAMERA_H

typedef struct {
    uint8_t reg[0x37];
    int32_t timing;

    uint8_t gb_pixels[14][16][16];
}camera;

extern camera gbcamera;

void initialize_camera();
void stop_camera();
void take_picture(cpu *c);


#endif //CHILLYGB_CAMERA_H
