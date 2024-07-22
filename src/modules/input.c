#include "../includes/cpu.h"
#include "../includes/apu.h"
#include "../includes/input.h"
#include "raylib.h"
#include <stdint.h>

joypad joypad1;

bool update_keys() {
    for (int i = 0; i < 4; i++) {
        if (IsKeyDown(joypad1.keys_dpad[i]))
            joypad1.dpad[i] = 0;
        else
            joypad1.dpad[i] = 1;
        if (IsKeyDown(joypad1.keys_btn[i]))
            joypad1.btn[i] = 0;
        else
            joypad1.btn[i] = 1;
    }

    if (IsKeyPressed(KEY_SPACE))
        joypad1.fast_forward = 5;
    if (IsKeyReleased(KEY_SPACE))
        joypad1.fast_forward = 1;

    for (int i = 0; i < 4; i++) {
        if (IsKeyPressed(joypad1.keys_dpad[i]))
            return true;
        if (IsKeyPressed(joypad1.keys_btn[i]))
            return true;
    }
    return false;
}