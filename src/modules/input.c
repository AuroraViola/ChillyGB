#include "../includes/cpu.h"
#include "../includes/apu.h"
#include "../includes/input.h"
#include "raylib.h"

joypad joypad1;

bool update_keys() {
    for (int i = 0; i < 4; i++) {
        if (IsKeyDown(set.keyboard_keys[i]) || IsGamepadButtonDown(0, set.gamepad_keys[i]))
            joypad1.dpad[i] = 0;
        else
            joypad1.dpad[i] = 1;
        if (IsKeyDown(set.keyboard_keys[i+4]) || IsGamepadButtonDown(0, set.gamepad_keys[i+4]))
            joypad1.btn[i] = 0;
        else
            joypad1.btn[i] = 1;
    }

    if (IsKeyPressed(set.fast_forward_key))
        joypad1.fast_forward = 10;
    if (IsKeyReleased(set.fast_forward_key))
        joypad1.fast_forward = 1;

    for (int i = 0; i < 8; i++) {
        if (IsKeyPressed(set.keyboard_keys[i]))
            return true;
        if (IsGamepadButtonDown(0, set.gamepad_keys[i]))
            return true;
    }
    return false;
}