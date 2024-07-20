#include "../includes/cartridge.h"
#include <string.h>
#include <stdio.h>

void get_save_name(char rom_name[256], char save_name[256]) {
    strcpy(save_name, rom_name);
    char *ext = strrchr (save_name, '.');
    if (ext != NULL)
        *ext = '\0';
    char new_ext[] = ".sav";
    strcat(save_name, new_ext);
}

void save_game(cartridge *cart, char rom_name[256]) {
    char save_name[256];
    get_save_name(rom_name, save_name);

    if (cart->type == 3 || cart->type == 0x13 || cart->type == 0x1b) {
        FILE *save = fopen(save_name, "w");
        if (cart->banks_ram == 1)
            fwrite(cart->ram, 0x2000, 1, save);
        else if (cart->banks_ram == 4)
            fwrite(cart->ram, 0x8000, 1, save);
        else if (cart->banks_ram == 8)
            fwrite(cart->ram, 0x10000, 1, save);
        else if (cart->banks_ram == 16)
            fwrite(cart->ram, 0x20000, 1, save);
        fclose(save);
    }

}

void load_game(cartridge *cart, char rom_name[256]) {
    char save_name[256];
    get_save_name(rom_name, save_name);

    FILE *file = fopen(rom_name, "r");
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

    if (cart->type == 3 || cart->type == 0x13 || cart->type == 0x1b) {
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
            fclose(save);
        }
    }
}
