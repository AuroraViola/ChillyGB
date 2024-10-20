#include "../includes/cartridge.h"
#include "../includes/camera.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

void get_save_name(char rom_name[256], char save_name[256]) {
    strcpy(save_name, rom_name);

    #if defined(PLATFORM_WEB)
    char * skip = save_name + 14;
    char * save = "/saves/";
    memcpy(skip, save, 7);
    strcpy(save_name, skip);
    #endif

    char *ext = strrchr (save_name, '.');
    if (ext != NULL)
        *ext = '\0';
    char new_ext[] = ".sav";
    strcat(save_name, new_ext);
}

void save_game(cartridge *cart, char rom_name[256]) {
    char save_name[256];
    get_save_name(rom_name, save_name);

    if (cart->type == 3 || cart->type == 0x13 || cart->type == 0x1b || cart->type == 0x10 || cart->type == 0x0f) {
        FILE *save = fopen(save_name, "wb");
        if (cart->banks_ram == 1)
            fwrite(cart->ram, 0x2000, 1, save);
        else if (cart->banks_ram == 4)
            fwrite(cart->ram, 0x8000, 1, save);
        else if (cart->banks_ram == 8)
            fwrite(cart->ram, 0x10000, 1, save);
        else if (cart->banks_ram == 16)
            fwrite(cart->ram, 0x20000, 1, save);

        if (cart->type == 0x10 || cart->type == 0x0f) {
            uint32_t s = cart->rtc.time % 60;
            uint32_t m = (cart->rtc.time / 60) % 60;
            uint32_t h = (cart->rtc.time / 3600) % 24;
            uint32_t dl = (cart->rtc.time / 86400) & 0xff;
            uint32_t dh = ((cart->rtc.time / 86400) >> 8) & 1;
            uint32_t dl_latch = cart->rtc.days & 0xff;
            uint32_t dh_latch = (cart->rtc.days >> 8) & 1;
            uint64_t time_saved = (uint64_t)(time(NULL));
            fwrite(&s, 4, 1, save);
            fwrite(&m, 4, 1, save);
            fwrite(&h, 4, 1, save);
            fwrite(&dl, 4, 1, save);
            fwrite(&dh, 4, 1, save);
            fwrite(&cart->rtc.seconds, 4, 1, save);
            fwrite(&cart->rtc.minutes, 4, 1, save);
            fwrite(&cart->rtc.hours, 4, 1, save);
            fwrite(&dl_latch, 4, 1, save);
            fwrite(&dh_latch, 4, 1, save);
            fwrite(&time_saved, 8, 1, save);
        }
        fclose(save);
    }
    else if (cart->type == 6) {
        FILE *save = fopen(save_name, "wb");
        fwrite(cart->ram, 0x200, 1, save);
        fclose(save);
    }
    else if (cart->type == 0xfc) {
        FILE *save = fopen(save_name, "wb");
        fwrite(cart->ram, 0x2000, 16, save);
        fclose(save);
    }
}

bool load_game(cartridge *cart, char rom_name[256]) {
    char save_name[256];
    get_save_name(rom_name, save_name);

    FILE *file = fopen(rom_name, "rb");
    if (file == NULL) {
        return false;
    }
    fread(cart->data[0], 0x4000, 1, file);
    cart->type = cart->data[0][0x0147];
    cart->banks = (2 << cart->data[0][0x0148]);

    cart->banks_ram = 0;
    if (cart->data[0][0x0149] == 2) {
        cart->banks_ram = 1;
    }
    else if (cart->data[0][0x0149] == 3) {
        cart->banks_ram = 4;
    }
    else if (cart->data[0][0x0149] == 4) {
        cart->banks_ram = 16;
    }
    else if (cart->data[0][0x0149] == 5) {
        cart->banks_ram = 8;
    }
    cart->bank_select_ram = 0;
    cart->ram_enable = false;

    for (int i = 1; i < cart->banks; i++)
        fread(cart->data[i], 0x4000, 1, file);
    cart->bank_select = 1;
    cart->bank_select_ram = 0;
    fclose(file);

    if (cart->type == 3 || cart->type == 0x13 || cart->type == 0x1b || cart->type == 0x10 || cart->type == 0x0f) {
        FILE *save = fopen(save_name, "r");
        if (save != NULL) {
            if (cart->banks_ram == 1)
                fread(cart->ram, 0x2000, 1, save);
            else if (cart->banks_ram == 4)
                fread(cart->ram, 0x8000, 1, save);
            else if (cart->banks_ram == 8)
                fread(cart->ram, 0x10000, 1, save);
            else if (cart->banks_ram == 16)
                fread(cart->ram, 0x20000, 1, save);

            if (cart->type == 0x10 || cart->type == 0x0f) {
                uint32_t s;
                uint32_t m;
                uint32_t h;
                uint32_t dl;
                uint32_t dh;
                uint32_t dh_latch;
                uint64_t time_saved;
                fread(&s, 4, 1, save);
                fread(&m, 4, 1, save);
                fread(&h, 4, 1, save);
                fread(&dl, 4, 1, save);
                fread(&dh, 4, 1, save);
                fread(&cart->rtc.seconds, 4, 1, save);
                fread(&cart->rtc.minutes, 4, 1, save);
                fread(&cart->rtc.hours, 4, 1, save);
                fread(&cart->rtc.days, 4, 1, save);
                fread(&dh_latch, 4, 1, save);
                fread(&time_saved, 8, 1, save);

                cart->rtc.days |= (dh_latch & 1) << 8;
                dl |= (dh & 1) << 8;
                uint64_t time_diff = (uint64_t)(time(NULL)) - time_saved;
                cart->rtc.time = s + m*60 + h*3600 + dl*86400 + time_diff;
            }
            fclose(save);
        }
    }
    else if (cart->type == 6) {
        FILE *save = fopen(save_name, "r");
        if (save != NULL) {
            fread(cart->ram, 0x200, 1, save);
            fclose(save);
        }
    }
    else if (cart->type == 0xfc) {
        initialize_camera();
        FILE *save = fopen(save_name, "r");
        if (save != NULL) {
            fread(cart->ram, 0x2000, 16, save);
            fclose(save);
        }
    }
    return true;
}
