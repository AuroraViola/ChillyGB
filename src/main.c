#include "raylib.h"
#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/settings.h"
#include "includes/apu.h"
#include "includes/timer.h"
#include "includes/debug.h"
#include "includes/input.h"
#include "includes/cartridge.h"
#include "includes/open_dialog.h"
#include "includes/savestates.h"
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define RAYLIB_NUKLEAR_IMPLEMENTATION
#include "../raylib-nuklear/include/raylib-nuklear.h"

#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef enum EmuModes{
    MENU = 0,
    GAME,
    DEBUG,
}EmuModes;

cpu c = {};
bool exited = false;
bool game_started = false;
bool show_settings = false;
bool show_about = false;
char rom_name[256];
uint8_t emulator_mode = MENU;
Color pixels_screen[144][160];
Color pixels[144][160] = { 0 };
Image display_image;
Image logo_image;
Texture2D logo;
Texture2D display;
char instructions[30][50];
float scale;
int scale_integer;
debugtexts texts;
int ff_speed = 1;
struct nk_context *ctx;
char comboxes[2500] = "";

void load_cartridge(char *path) {
    strcpy(rom_name, path);
    if(!load_game(&c.cart, rom_name)){
        fprintf(stderr, "Failed to open game: %s\n", rom_name);
        return;
    }
    if (load_bootrom(&c.bootrom) && set.bootrom_enabled)
        initialize_cpu_memory(&c, &set);
    else
        initialize_cpu_memory_no_bootrom(&c, &set);
    emulator_mode = GAME;
    game_started = true;
    ResumeAudioStream(audio.ch1.stream);
    ResumeAudioStream(audio.ch2.stream);
    ResumeAudioStream(audio.ch3.stream);
    ResumeAudioStream(audio.ch4.stream);
}

void DrawNavBar() {
    if (nk_begin(ctx, "Overview", nk_rect(0, 0, GetScreenWidth(), 35), 0)) {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 5);
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Load ROM", NK_TEXT_LEFT)) {
                char *path = do_open_rom_dialog();
                if (path != NULL) {
                    load_cartridge(path);
                }
            }
            if (nk_menu_item_label(ctx, "Save SRAM", NK_TEXT_LEFT)) {
                if (game_started) {
                    save_game(&c.cart, rom_name);
                }
            }
            if (nk_menu_item_label(ctx, "Load state", NK_TEXT_LEFT)) {
                if (game_started)
                    load_state(&c, rom_name);
            }
            if (nk_menu_item_label(ctx, "Save state", NK_TEXT_LEFT)) {
                if (game_started)
                    save_state(&c, rom_name);
            }
            #ifndef PLATFORM_WEB
            if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
                exited = true;
            }
            #endif
            nk_menu_end(ctx);
        }

        nk_layout_row_push(ctx, 120);
        if (nk_menu_begin_label(ctx, "Emulation", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Reset", NK_TEXT_LEFT)) {
                if (game_started) {
                    if (load_bootrom(&c.bootrom) && set.bootrom_enabled)
                        initialize_cpu_memory(&c, &set);
                    else
                        initialize_cpu_memory_no_bootrom(&c, &set);
                    emulator_mode = GAME;
                    ResumeAudioStream(audio.ch1.stream);
                    ResumeAudioStream(audio.ch2.stream);
                    ResumeAudioStream(audio.ch3.stream);
                    ResumeAudioStream(audio.ch4.stream);
                }
            }
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 70);
        if (nk_menu_begin_label(ctx, "Tools", NK_TEXT_LEFT, nk_vec2(230, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Debugger", NK_TEXT_LEFT)) {
                if (game_started) {
                    emulator_mode = DEBUG;
                }
            }
            if (nk_menu_item_label(ctx, "Settings", NK_TEXT_LEFT)) {
                show_settings = true;
            }
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 60);
        if (nk_menu_begin_label(ctx, "Help", NK_TEXT_LEFT, nk_vec2(150, 200))) {
            static size_t prog = 40;
            static int slider = 10;
            static int check = nk_true;
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_menu_item_label(ctx, "Github", NK_TEXT_LEFT)) {
                #ifdef _WIN32
                    system("start https://github.com/AuroraViola/ChillyGB");
                #elif __APPLE__
                    system("open https://github.com/AuroraViola/ChillyGB");
                #elif __linux__
                    system("xdg-open https://github.com/AuroraViola/ChillyGB");
                #elif defined(PLATFORM_WEB)
                EM_ASM(
                    window.open("https://github.com/AuroraViola/ChillyGB").focus();
                );
                #else
                    printf("Failed to open GitHub link: Unsupported OS\n");
                #endif
            }
            if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
                show_about = true;
            }
            nk_menu_end(ctx);
        }
        nk_menubar_end(ctx);
    }
    nk_end(ctx);
}
void pause_game() {
    if (emulator_mode != GAME) return;
    emulator_mode = MENU;
    PauseAudioStream(audio.ch1.stream);
    PauseAudioStream(audio.ch2.stream);
    PauseAudioStream(audio.ch3.stream);
    PauseAudioStream(audio.ch4.stream);
}

void update_frame() {
    if (IsFileDropped()) {
        FilePathList droppedFiles = LoadDroppedFiles();
        if ((int) droppedFiles.count == 1) {
            if (game_started)
                save_game(&c.cart, rom_name);
            load_cartridge(droppedFiles.paths[0]);
        }
        UnloadDroppedFiles(droppedFiles);
    }
    scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
    scale_integer = MIN((int) GetScreenWidth() / 160, (int) GetScreenHeight() / 144);
    switch (emulator_mode) {
        case MENU:
            UpdateNuklear(ctx);
            DrawNavBar();
            if (show_settings){
                if(nk_begin_titled(ctx, "ctx-settings","Settings", nk_rect(24, 64, 500, 200),
                                                 NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_CLOSABLE)) {
                    nk_layout_row_dynamic(ctx, 30, 2);
                    nk_label(ctx, "Sound Volume", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    set.volume = nk_slide_int(ctx, 0, set.volume, 100, 1);
                    nk_label(ctx, "Palette", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    struct nk_vec2 size = {250, 200};
                    int comboxes_len = 0;
                    for (int i = 0; i < set.palettes_size; i++) {
                        stpcpy(comboxes + comboxes_len, set.palettes[i].name);
                        comboxes_len += strlen(set.palettes[i].name)+1;
                    }
                    nk_combobox_string(ctx, comboxes, &set.selected_palette, set.palettes_size, 20, size);
                    nk_label(ctx, "Integer Scaling", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_checkbox_label(ctx, "", &set.integer_scaling);
                    #ifndef PLATFORM_WEB
                    nk_label(ctx, "Boot Rom", NK_TEXT_ALIGN_LEFT|NK_TEXT_ALIGN_MIDDLE);
                    nk_checkbox_label(ctx, "", &set.bootrom_enabled);
                    #endif
                }
                nk_end(ctx);
            }
            if (nk_window_is_hidden(ctx, "ctx-settings"))
                show_settings = false;

            if (show_about) {
                if(nk_begin_titled(ctx, "ctx-about","About", nk_rect((GetScreenWidth()/2-200), (GetScreenHeight()/2-250), 400, 500),
                                              NK_WINDOW_CLOSABLE)) {
                    nk_layout_row_begin(ctx, NK_STATIC, 256, 3);
                    /* padding */
                    nk_layout_row_push(ctx, (370-150)/2);
                    nk_label(ctx, "", NK_TEXT_LEFT);

                    /* header */
                    struct nk_image chillygb_logo = TextureToNuklear(logo);
                    nk_layout_row_push(ctx, 150);
                    nk_image(ctx, chillygb_logo);

                    /* padding */
                    nk_layout_row_push(ctx, (370-150)/2);
                    nk_label(ctx, "", NK_TEXT_LEFT);
                    nk_layout_row_end(ctx);
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, "",  NK_TEXT_CENTERED);
                    nk_label(ctx, "ChillyGB", NK_TEXT_CENTERED);
                    nk_label(ctx, "By AuroraViola", NK_TEXT_CENTERED);
                    nk_label(ctx, "",  NK_TEXT_CENTERED);
                    nk_label(ctx, "ChillyGB is licensed under the",  NK_TEXT_CENTERED);
                    nk_label(ctx, "GNU General Public License v3.0.",  NK_TEXT_CENTERED);
                }
                nk_end(ctx);
            }
            if (nk_window_is_hidden(ctx, "ctx-about"))
                show_about = false;
            
            if (IsKeyPressed(KEY_ESCAPE) && game_started) {
                emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            if (video.is_on) {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                    }
                }
            }
            else {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                    }
                }
            }
            UpdateTexture(display, pixels);
            BeginDrawing();
            ClearBackground(BLACK);
            if (set.integer_scaling) {
                DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, display.width, display.height},
                               (Rectangle) {(GetScreenWidth() - (160 * scale_integer)) * 0.5f,
                                            (GetScreenHeight() - (144 * scale_integer)) * 0.5f,
                                            160 * scale_integer, 144 * scale_integer}, (Vector2) {0, 0}, 0.0f,
                               WHITE);
            }
            else {
                DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                               (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                            (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                            (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
            }
            if (!game_started) {
                float fontsize = (set.integer_scaling) ? (7 * scale_integer) : (7 * scale);
                int center = MeasureText("Drop a Game Boy ROM to start playing", fontsize);
                DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, (GetScreenHeight()/2-fontsize/2), fontsize, set.palettes[set.selected_palette].colors[3]);
            }
            DrawNuklear(ctx);
            EndDrawing();
            if (game_started)
                SetWindowTitle("ChillyGB - Paused");
            break;
        case GAME:
            timer1.timer_global = 0;
            while (timer1.timer_global < ((4194304/60) * joypad1.fast_forward)) {
                execute(&c);
                Update_Audio(&c);
            }
            if (video.draw_screen) {
                if (IsKeyPressed(KEY_ESCAPE)) {
                    save_settings();
                    save_game(&c.cart, rom_name);
                    pause_game();
                }
                if (IsKeyPressed(KEY_F3)) {
                    emulator_mode = DEBUG;
                    PauseAudioStream(audio.ch1.stream);
                    PauseAudioStream(audio.ch2.stream);
                    PauseAudioStream(audio.ch3.stream);
                    PauseAudioStream(audio.ch4.stream);
                }

                if (IsKeyPressed(KEY_F2)) {
                    save_state(&c, rom_name);
                }
                if (IsKeyPressed(KEY_F1)) {
                    load_state(&c, rom_name);
                }
                video.draw_screen = false;
                if (video.is_on) {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                        }
                    }
                }
                else {
                    for (int i = 0; i < 144; i++) {
                        for (int j = 0; j < 160; j++) {
                            pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                        }
                    }
                }
                UpdateTexture(display, pixels);
                BeginDrawing();
                ClearBackground(BLACK);
                if (set.integer_scaling) {
                    DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, display.width, display.height},
                                   (Rectangle) {(GetScreenWidth() - (160 * scale_integer)) * 0.5f,
                                                (GetScreenHeight() - (144 * scale_integer)) * 0.5f,
                                                160 * scale_integer, 144 * scale_integer}, (Vector2) {0, 0}, 0.0f,
                                   WHITE);
                }
                else {
                    DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                                   (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                                (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                                (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f,
                                   WHITE);
                }
                EndDrawing();
                char str[80];
                sprintf(str, "ChillyGB - %d FPS - %.1fx", GetFPS(), (float)(GetFPS())/60);
                SetWindowTitle(str);
            }
            break;

        case DEBUG:
            UpdateNuklear(ctx);

            generate_texts(&c, &texts);
            decode_instructions(&c, instructions);

            if (nk_begin(ctx, "Registers", nk_rect(24, 24, 190, 224),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, texts.AFtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.BCtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.DEtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.HLtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.SPtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.PCtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.IMEtext, NK_TEXT_CENTERED);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "PPU Info", nk_rect(24, 272, 190, 170),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, texts.LYtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.LYCtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.PPUMode, NK_TEXT_CENTERED);
                nk_label(ctx, texts.LCDCtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.STATtext, NK_TEXT_CENTERED);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Interrupts", nk_rect(24, 466, 190, 128),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, texts.IEtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.IFtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.IMEtext, NK_TEXT_CENTERED);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Fast Forward", nk_rect(24, 618, 190, 80),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 2);
                nk_label(ctx, "Speed", NK_TEXT_CENTERED);
                ff_speed = nk_slide_int(ctx, 1, ff_speed, 1000, 1);
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Timer Info", nk_rect(238, 466, 210, 200),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, texts.DIV, NK_TEXT_CENTERED);
                nk_label(ctx, texts.TIMA, NK_TEXT_CENTERED);
                nk_label(ctx, texts.TMA, NK_TEXT_CENTERED);
                nk_label(ctx, texts.TIMER_ON, NK_TEXT_CENTERED);
                nk_label(ctx, texts.MODULE, NK_TEXT_CENTERED);
                nk_label(ctx, texts.TSTATEStext, NK_TEXT_CENTERED);
            }
            nk_end(ctx);


            if (nk_begin(ctx, "Cart Info", nk_rect(472, 466, 190, 128),
                         NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                nk_label(ctx, texts.BANKtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.RAMBANKtext, NK_TEXT_CENTERED);
                nk_label(ctx, texts.RAMENtext, NK_TEXT_CENTERED);
            }
            nk_end(ctx);


            if (nk_begin(ctx, "Memory Instructions", nk_rect(238, 24, 424, 418),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                for (int i = 0; i < 30; i++) {
                    nk_label(ctx, instructions[i], NK_TEXT_LEFT);
                }
            }
            nk_end(ctx);

            if (nk_begin(ctx, "Memory Viewer", nk_rect(686, 648, 650, 300),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, 20, 1);
                for (int i = 0; i < 4096; i++) {
                    nk_label(ctx, texts.memory[i], NK_TEXT_LEFT);
                }
            }
            nk_end(ctx);

            if (IsKeyPressed(KEY_N)) {
                execute(&c);
                Update_Audio(&c);
            }

            if (IsKeyDown(KEY_C)) {
                for (int i = 0; i < ff_speed; i++) {
                    execute(&c);
                    Update_Audio(&c);
                }
            }

            if (IsKeyPressed(KEY_R)) {
                initialize_cpu_memory_no_bootrom(&c, &set);
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                emulator_mode = GAME;
                ResumeAudioStream(audio.ch1.stream);
                ResumeAudioStream(audio.ch2.stream);
                ResumeAudioStream(audio.ch3.stream);
                ResumeAudioStream(audio.ch4.stream);
            }

            if (video.is_on) {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[video.display[i][j]];
                        if (i == video.scan_line)
                            pixels[i][j].a = 127;
                        if (i == c.memory[LYC])
                            pixels[i][j].r += 20;
                    }
                }
            }
            else {
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        pixels[i][j] = set.palettes[set.selected_palette].colors[0];
                    }
                }
            }
            UpdateTexture(display, pixels);
            struct nk_image display_debug = TextureToNuklear(display);
            if (nk_begin(ctx, "Display", nk_rect(686, 24, 650, 600),
                         NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE)) {
                nk_layout_row_dynamic(ctx, nk_window_get_height(ctx)-56, 1);
                nk_image_color(ctx, display_debug, nk_white);
            }
            nk_end(ctx);
            BeginDrawing();
            ClearBackground(BLACK);
            DrawNuklear(ctx);
            EndDrawing();
            SetWindowTitle("ChillyGB - Debug");
            break;
    }
}

int main(int argc, char **argv) {
    int n_ticks = 0;
    char *test_image_path;

    int arg_count;
    const char *short_opt = "i:t:h";
    struct option long_opt[] = {
            {"image",          required_argument, NULL, 'i'},
            {"ticks",          required_argument, NULL, 't'},
            {"help",          no_argument, NULL, 'h'},
            {NULL,            0,                 NULL, 0  }
    };

    while((arg_count = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch(arg_count) {
            case -1:
            case 0:
                break;

            case 'i':
                test_image_path = optarg;
                break;

            case 't':
                n_ticks = atoi(optarg);
                break;

            case 'h':
                printf("Usage: ChillyGB [ROM] [OPTIONS]\n");
                printf("  -h, --help                      Print this help and exit\n");
                printf("  -i filename, --image filename   The file path to the image for comparing the final result\n");
                printf("  -t, --ticks, n_ticks            Number of t-cycles to do before exiting the test\n");
                printf("\n");
                return(0);

            case ':':
            case '?':
                fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
                return(-2);

            default:
                fprintf(stderr, "%s: invalid option -- %c\n", argv[0], arg_count);
                return(-2);
        };
    };

    if (optind < argc) {
        load_cartridge(argv[optind]);
    }

    if (n_ticks > 0) {
        SetTraceLogLevel(LOG_ERROR);
        test_rom(&c, n_ticks);
        Image screenshot = take_debug_screenshot(pixels_screen);
        Image expected = LoadImage(test_image_path);
        export_screenshot(screenshot, rom_name);
        for (int i = 0; i < 144; i++) {
            for (int j = 0; j < 160; j++) {
                if (GetImageColor(screenshot, j, i).r != GetImageColor(expected, j, i).r) {
                    printf("Test FAILED\n");
                    printf("Reason: Image mismatch\n");
                    return 1;
                }
            }
        }
        printf("Test PASSED");
        return 0;
    }

    // Initialize Raylib and Nuklear
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    ctx = InitNuklearEx(LoadFontEx("res/fonts/UbuntuMono.ttf", 20, 0, 250), 20);
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color) {255, 255, 255, 255};
    display_image = (Image){
            .data = pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    display = LoadTextureFromImage(display_image);
    logo_image = LoadImage("res/icons/ChillyGB-256.png");
    logo = LoadTextureFromImage(logo_image);

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    joypad1.fast_forward = 1;

    #if defined(PLATFORM_WEB)
    EM_ASM(
        FS.mkdir('/file_picker_uploads');
        // Make a directory other than '/'
        FS.mkdir('/saves');
        // Then mount with IDBFS type
        FS.mount(IDBFS, {autoPersist:true}, '/saves');

        // Then sync
        FS.syncfs(true, function (err) {
            console.log(err);
            ccall("load_settings", "v");
        });
    );
    emscripten_set_main_loop(update_frame, 0, 1);
    #else
    load_settings();
    while(!WindowShouldClose() && !exited) {
        update_frame();
    }
    #endif

    save_game(&c.cart, rom_name);
    save_settings();
    UnloadTexture(display);
    UnloadTexture(logo);
    UnloadNuklear(ctx);
    CloseWindow();

    return 0;
}
