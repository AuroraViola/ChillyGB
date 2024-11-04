/*
 * This File was based on https://github.com/LIJI32/SameBoy/blob/master/OpenDialog
 */

#ifndef CHILLYGB_OPEN_DIALOG_H
#define CHILLYGB_OPEN_DIALOG_H

#if !(defined(__linux__) || defined(_WIN32) || defined(PLATFORM_WEB))
    #define CUSTOM_OPEN_DIALOG
#endif

#ifdef CUSTOM_OPEN_DIALOG
#include "../../raylib-nuklear/include/raylib-nuklear.h"
void DrawFileManager(struct nk_context *ctx);
#endif

char *do_open_rom_dialog(bool bootrom_chooser);

#endif //CHILLYGB_OPEN_DIALOG_H
