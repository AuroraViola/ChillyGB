/*
 * This File was based on https://github.com/LIJI32/SameBoy/blob/master/OpenDialog
 */

#ifdef __linux__

//#include "../includes/open_dialog.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <string.h>

#define GTK_FILE_CHOOSER_ACTION_OPEN 0
#define GTK_RESPONSE_ACCEPT -3
#define GTK_RESPONSE_CANCEL -6


void *_gtk_file_chooser_dialog_new (const char *title,
                                    void *parent,
                                    int action,
                                    const char *first_button_text,
                                    ...);
bool _gtk_init_check (int *argc, char ***argv);
int _gtk_dialog_run(void *);
void _g_free(void *);
void _gtk_widget_destroy(void *);
char *_gtk_file_chooser_get_filename(void *);
void _g_log_set_default_handler (void *function, void *data);
void *_gtk_file_filter_new(void);
void _gtk_file_filter_add_pattern(void *filter, const char *pattern);
void _gtk_file_filter_set_name(void *filter, const char *name);
void _gtk_file_chooser_add_filter(void *dialog, void *filter);
void _gtk_main_iteration(void);
bool _gtk_events_pending(void);
unsigned long _g_signal_connect_data(void *instance,
                                     const char *detailed_signal,
                                     void *c_handler,
                                     void *data,
                                     void *destroy_data,
                                     unsigned connect_flags);
void _gtk_file_chooser_set_current_name(void *dialog,
                                        const char *name);
void *_gtk_file_chooser_get_filter(void *dialog);
const char *_gtk_file_filter_get_name (void *dialog);



#define LAZY(symbol) static typeof(_##symbol) *symbol = NULL;\
if (symbol == NULL) symbol = dlsym(handle, #symbol);\
if (symbol == NULL) goto lazy_error
#define TRY_DLOPEN(name) handle = handle? handle : dlopen(name, RTLD_NOW)

void _nop(void){}

char *do_open_rom_dialog(void) {
    static void *handle = NULL;

    TRY_DLOPEN("libgtk-3.so");
    TRY_DLOPEN("libgtk-3.so.0");
    TRY_DLOPEN("libgtk-2.so");
    TRY_DLOPEN("libgtk-2.so.0");

    if (!handle) {
        goto lazy_error;
    }


    LAZY(gtk_init_check);
    LAZY(gtk_file_chooser_dialog_new);
    LAZY(gtk_dialog_run);
    LAZY(g_free);
    LAZY(gtk_widget_destroy);
    LAZY(gtk_file_chooser_get_filename);
    LAZY(g_log_set_default_handler);
    LAZY(gtk_file_filter_new);
    LAZY(gtk_file_filter_add_pattern);
    LAZY(gtk_file_filter_set_name);
    LAZY(gtk_file_chooser_add_filter);
    LAZY(gtk_events_pending);
    LAZY(gtk_main_iteration);

    g_log_set_default_handler(_nop, NULL);

    gtk_init_check(0, 0);


    void *dialog = gtk_file_chooser_dialog_new("Open ROM",
                                               0,
                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               "_Open", GTK_RESPONSE_ACCEPT,
                                               NULL );


    void *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.gb");
    gtk_file_filter_add_pattern(filter, "*.gbc");
    gtk_file_filter_set_name(filter, "Game Boy ROMs");
    gtk_file_chooser_add_filter(dialog, filter);

    int res = gtk_dialog_run(dialog);
    char *ret = NULL;

    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        filename = gtk_file_chooser_get_filename(dialog);
        ret = strdup(filename);
        g_free(filename);
    }

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    gtk_widget_destroy(dialog);

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    return ret;

    lazy_error:
    fprintf(stderr, "Failed to display GTK dialog\n");
    return NULL;
}
#elif _WIN32
#define COBJMACROS
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

static char *wc_to_utf8_alloc(const wchar_t *wide) {
    unsigned int cb = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (cb) {
        char *buffer = (char*) malloc(cb);
        if (buffer) {
            WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer, cb, NULL, NULL);
            return buffer;
        }
    }
    return NULL;
}

char *do_open_rom_dialog(void) {
    OPENFILENAMEW dialog;
    wchar_t filename[MAX_PATH];

    filename[0] = '\0';
    memset(&dialog, 0, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = filename;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = L"Game Boy ROMs\0*.gb;*.gbc;*.sgb;*.isx\0All files\0*.*\0\0";
    dialog.nFilterIndex = 1;
    dialog.lpstrFileTitle = NULL;
    dialog.nMaxFileTitle = 0;
    dialog.lpstrInitialDir = NULL;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&dialog)) {
        return wc_to_utf8_alloc(filename);
    }

    return NULL;
}

#elif PLATFORM_WEB
#include <emscripten.h>
#include <stddef.h>

char *do_open_rom_dialog(void) {
    EM_ASM(
        openDialog();
    );
    return NULL;
}
#else 
#include <stddef.h>
char *do_open_rom_dialog(void) {
    return NULL;
}
#endif