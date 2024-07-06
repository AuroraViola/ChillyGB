#include "cpu.h"
#include "apu.h"
#include "input.h"
#include "raylib.h"
#include <stdint.h>

joypad j1;

bool update_keys() {
    for (int i = 0; i < 4; i++) {
        if (IsKeyDown(j1.keys_dpad[i]))
            j1.dpad[i] = 0;
        else
            j1.dpad[i] = 1;
        if (IsKeyDown(j1.keys_btn[i]))
            j1.btn[i] = 0;
        else
            j1.btn[i] = 1;
    }

    if (IsKeyPressed(KEY_SPACE))
        SetTargetFPS(100000);
    if (IsKeyReleased(KEY_SPACE))
        SetTargetFPS(60);

    for (int i = 0; i < 4; i++) {
        if (IsKeyPressed(j1.keys_dpad[i]))
            return true;
        if (IsKeyPressed(j1.keys_btn[i]))
            return true;
    }
    return false;
}