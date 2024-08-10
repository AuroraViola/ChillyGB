/*
 * This File was based on https://github.com/LIJI32/SameBoy/blob/master/OpenDialog
 */

#ifndef CHILLYGB_OPEN_DIALOG_H
#define CHILLYGB_OPEN_DIALOG_H
#if PLATFORM_NX
#include "../../raylib-nuklear/include/nuklear.h"
#endif

char *do_open_rom_dialog(void);
#if PLATFORM_NX
void DrawFileManager(struct nk_context *ctx);
#endif

#endif //CHILLYGB_OPEN_DIALOG_H
