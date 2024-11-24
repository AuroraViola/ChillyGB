#define NK_INCLUDE_STANDARD_BOOL
#include "raylib.h"
#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/settings.h"
#include "includes/apu.h"
#include "includes/timer.h"
#include "includes/serial.h"
#include "includes/debug.h"
#include "includes/input.h"
#include "includes/cartridge.h"
#include "includes/open_dialog.h"
#include "includes/savestates.h"
#include "includes/camera.h"
#include "includes/memory.h"
#include "includes/cheats.h"
#include "includes/ui.h"
#include "raylib-nuklear.h"
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
char *stpcpy (char *__restrict __dest, const char *__restrict __src);
#endif

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

cpu c = {};
Color pixels_screen[144][160];
float speed_mult = 1;

void update_frame_main() {
    update_frame(&c);
};

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
        load_settings();
        if (n_ticks > 0) {
            set.selected_gameboy = 0;
            set.bootrom_enabled = false;
        }
        load_cartridge(&c, argv[optind]);
    }

    if (n_ticks > 0) {
        SetTraceLogLevel(LOG_ERROR);
        test_rom(&c, n_ticks);
        Image screenshot = take_debug_screenshot(pixels_screen);
        Image expected = LoadImage(test_image_path);
        export_screenshot(screenshot, ui.rom_name);
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

    initialize_ui();
    init_axis();

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
    emscripten_set_main_loop(update_frame_main, 0, 1);
    #else
    load_settings();
    while(!WindowShouldClose() && !ui.exited) {
        update_frame(&c);
    }
    #endif

    deinitialize_ui(&c);

    return 0;
}
