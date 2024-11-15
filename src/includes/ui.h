#include "raylib.h"
#include "raylib-nuklear.h"
#include "cpu.h"

#ifndef CHILLYGB_UI_H
#define CHILLYGB_UI_H

typedef enum EmuModes{
    MENU = 0,
    GAME,
    DEBUG,
}EmuModes;

typedef enum SettingsView {
    SET_EMU = 0,
    SET_VIDEO,
    SET_AUDIO,
    SET_INPUT,
}SettingsView;

typedef enum CheatsView {
    CHEAT_GAMEGENIE = 0,
    CHEAT_GAMESHARK,
}CheatsView;

typedef enum InputSettingsView {
    INPUT_GAMEPAD = 0,
    INPUT_ADVANCED,
    INPUT_MOTION,
}InputSettingsView;

typedef enum ShadersList {
    SHADER_DEFAULT = 0,
    SHADER_PIXEL_GRID,
    SHADER_COLOR_CORRECTION,
}ShadersList;

typedef struct {
    bool show;
    SettingsView view;
    InputSettingsView input_view;
    uint8_t palette_color_selected;
    bool is_selected_input;
    uint8_t selected_input;
    char comboxes[2500];
    int key_order[8];
}UiSettings;

typedef struct {
    bool show;
    char version[20];
    Image logo_image;
    Texture2D logo_texture;
}UiAbout;

typedef struct {
    bool show;
    char message[256];
}UiAlert;

typedef struct {
    bool show;
    CheatsView view;
}UiCheats;

typedef struct {
    struct nk_style_button tab_button;
    struct nk_style_button tab_button_active;
    struct nk_style_button button_dpad;
    struct nk_style_button button_dpad_pressed;
    struct nk_style_button button_buttons;
    struct nk_style_button button_buttons_pressed;
}UiStyles;

typedef struct {
    char instructions[30][50];
}UiDebugger;

typedef struct {
    Color pixels[144][160];
    Image image;
    Texture2D texture;
}UiDisplay;

typedef struct {
    struct nk_context *ctx;
    UiSettings settings;
    UiCheats cheats;
    UiDebugger debugger;
    UiAbout about;
    UiAlert alert;
    UiStyles style;
    UiDisplay display;

    EmuModes emulator_mode;
    bool exited;
    bool game_started;
    char rom_name[256];

    Shader shaders[3];
    float scale;
    int scale_integer;
}Ui;

extern Ui ui;

void update_frame(cpu *c);
void load_cartridge(cpu *c, char *path);
void initialize_ui();
void deinitialize_ui(cpu *c);

#endif //CHILLYGB_UI_H
