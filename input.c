#include "cpu.h"
#include "apu.h"
#include "input.h"
#include "raylib.h"
#include <stdint.h>

uint8_t get_joypad(cpu *c, joypad *j){
    uint8_t joypad_register = 0;
    if (IsKeyPressed(KEY_W)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_D)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_A)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_S)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_L)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_K)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_BACKSPACE)) {
        c->memory[0xff0f] |= 16;
    }

    else if (IsKeyPressed(KEY_ENTER)) {
        c->memory[0xff0f] |= 16;
    }

    if (IsKeyPressed(KEY_SPACE))
        SetTargetFPS(60000);
    if (IsKeyReleased(KEY_SPACE))
        SetTargetFPS(60);

    if (IsKeyDown(KEY_W))
        j->dpad[DPAD_UP] = 1;
    else
        j->dpad[DPAD_UP] = 0;

    if (IsKeyDown(KEY_S) && j->dpad[DPAD_UP] == 0)
        j->dpad[DPAD_DOWN] = 1;
    else
        j->dpad[DPAD_DOWN] = 0;

    if (IsKeyDown(KEY_D))
        j->dpad[DPAD_RIGHT] = 1;
    else
        j->dpad[DPAD_RIGHT] = 0;

    if (IsKeyDown(KEY_A) && j->dpad[DPAD_RIGHT] == 0)
        j->dpad[DPAD_LEFT] = 1;
    else
        j->dpad[DPAD_LEFT] = 0;

    if (IsKeyDown(KEY_L))
        j->buttons[BUTTON_A] = 1;
    else
        j->buttons[BUTTON_A] = 0;

    if (IsKeyDown(KEY_K))
        j->buttons[BUTTON_B] = 1;
    else
        j->buttons[BUTTON_B] = 0;

    if (IsKeyDown(KEY_ENTER))
        j->buttons[BUTTON_START] = 1;
    else
        j->buttons[BUTTON_START] = 0;

    if (IsKeyDown(KEY_BACKSPACE))
        j->buttons[BUTTON_SELECT] = 1;
    else
        j->buttons[BUTTON_SELECT] = 0;

    if (((c->memory[0xff00] >> 4) & 3) == 0) {
        joypad_register = (c->memory[0xff00] & 0xf0);
        uint8_t btn_enc = j->buttons[BUTTON_A];
        btn_enc |= (j->buttons[BUTTON_B] << 1);
        btn_enc |= (j->buttons[BUTTON_SELECT] << 2);
        btn_enc |= (j->buttons[BUTTON_START] << 3);
        uint8_t dpad_enc = j->dpad[DPAD_RIGHT];
        dpad_enc |= (j->dpad[DPAD_LEFT] << 1);
        dpad_enc |= (j->dpad[DPAD_UP] << 2);
        dpad_enc |= (j->dpad[DPAD_DOWN] << 3);
        btn_enc |= ((~dpad_enc) & 15);
        joypad_register |= ((btn_enc) & 15);

    }
    else if (((c->memory[0xff00] >> 4) & 3) == 1) {
        joypad_register = (c->memory[0xff00] & 0xf0);
        uint8_t btn_enc = j->buttons[BUTTON_A];
        btn_enc |= (j->buttons[BUTTON_B] << 1);
        btn_enc |= (j->buttons[BUTTON_SELECT] << 2);
        btn_enc |= (j->buttons[BUTTON_START] << 3);
        joypad_register |= (~(btn_enc) & 15);
    }
    else if (((c->memory[0xff00] >> 4) & 3) == 2) {
        joypad_register = (c->memory[0xff00] & 0xf0);
        uint8_t dpad_enc = j->dpad[DPAD_RIGHT];
        dpad_enc |= (j->dpad[DPAD_LEFT] << 1);
        dpad_enc |= (j->dpad[DPAD_UP] << 2);
        dpad_enc |= (j->dpad[DPAD_DOWN] << 3);
        joypad_register |= (~(dpad_enc) & 15);
    }
    else if (((c->memory[0xff00] >> 4) & 3) == 3) {
        joypad_register = (c->memory[0xff00] & 0xf0) | 0xf;
    }
    return joypad_register;
}
