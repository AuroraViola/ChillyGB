/*
 * This File was based on https://github.com/LIJI32/SameBoy/blob/master/OpenDialog
 */
#if defined(__linux__) && !defined(CUSTOM_OPEN_DIALOG)
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
    gtk_file_filter_add_pattern(filter, "*.GB");
    gtk_file_filter_add_pattern(filter, "*.gB");
    gtk_file_filter_add_pattern(filter, "*.Gb");
    gtk_file_filter_add_pattern(filter, "*.gbc");
    gtk_file_filter_add_pattern(filter, "*.gbC");
    gtk_file_filter_add_pattern(filter, "*.gBc");
    gtk_file_filter_add_pattern(filter, "*.gBC");
    gtk_file_filter_add_pattern(filter, "*.Gbc");
    gtk_file_filter_add_pattern(filter, "*.GbC");
    gtk_file_filter_add_pattern(filter, "*.GBc");
    gtk_file_filter_add_pattern(filter, "*.GBC");
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
#elif defined(_WIN32) && !defined(CUSTOM_OPEN_DIALOG)
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

#elif defined(PLATFORM_WEB) && !defined(CUSTOM_OPEN_DIALOG)
#include <emscripten.h>
#include <stddef.h>

char *do_open_rom_dialog(void) {
    EM_ASM(
        openDialog();
    );
    return NULL;
}
#else
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include "raylib.h"
#include "../includes/input.h"
#include <assert.h>
#include "../includes/debug.h"
#include "../../raylib-nuklear/include/raylib-nuklear.h"

struct item_t {
    char name[256];
    struct item_t* next;
};

struct {
    char cwd[256];

    struct item_t* files;

    struct item_t* dirs;

    int total_items;
} file_manager = {.cwd = "/", .files = NULL, .dirs = NULL};
 

bool dialog_open = true;

void free_list(struct item_t* first){
    if (first == NULL) return;
    struct item_t* next;
    while((next = first->next) != NULL){
        free(first);
        first = next;
    }
    free(first);
}

struct item_t* append_list(struct item_t* last, char name[256]){
    struct item_t* new = malloc(sizeof(struct item_t));
    strcpy(new->name, name);
    new->next = NULL;
    if (last) {
        last->next = new;
    }
    file_manager.total_items++;
    return new;
}

int currently_selected = 0;
bool refreshed = false; 

void refresh_manager(){
    currently_selected = 0;
    free_list(file_manager.files);
    file_manager.files = NULL;
    free_list(file_manager.dirs);
    file_manager.dirs = NULL;
    file_manager.total_items = 0;
    struct dirent * data;
    DIR* d = opendir(file_manager.cwd);
    if (d == NULL) return;
    struct item_t* last_dir = file_manager.dirs;
    struct item_t* last_file = file_manager.files;
    while (( data = readdir(d)) != NULL) {
        if (data->d_type == DT_DIR ) {
            strcat(data->d_name, "/");
            last_dir = append_list(last_dir, data->d_name);
            if (file_manager.dirs == NULL) file_manager.dirs = last_dir;
        } else {
            char *dot = strrchr(data->d_name, '.');
            if (dot && !strcmp(dot, ".gb")) {
                last_file = append_list(last_file, data->d_name);
                if (file_manager.files == NULL) file_manager.files = last_file;
            }
        }
    }
    closedir(d); 
}

char *do_open_rom_dialog(void) {
    dialog_open = !dialog_open;
    if (dialog_open) refresh_manager();
    return NULL;
}

void open_dir(char* dir) {
    if (file_manager.cwd[strlen(file_manager.cwd) -1] != '/' )
      strcat(file_manager.cwd, "/");
    strcat(file_manager.cwd, dir);
    #ifndef PLATFORM_NX // TODO: Figure out why this broken on libnx
    char buff[256];
    realpath(file_manager.cwd, buff);
    strcpy(file_manager.cwd, buff);
    #endif
    refresh_manager();
}

void load_cartridge(char *path);

#define BUTTON_HIEGHT 54
void DrawFileManager(struct nk_context * ctx){
    bool selection_changed = false;

    if (!refreshed) { 
        refresh_manager();
        refreshed = true;
    }

    if (!dialog_open) {
        return;
    }

    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
        selection_changed = true;
        if (currently_selected < file_manager.total_items-1) {
            currently_selected++;
        }
        else {
            currently_selected = 0;
        }
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) {
        selection_changed = true;
        if (currently_selected > 0 ) {
            currently_selected--;
        } else {
            currently_selected = file_manager.total_items - 1;
        };
    }

    if (IsKeyPressed(KEY_LEFT)) {
        open_dir("../");
    }

    if(nk_begin_titled(ctx, "ctx-files",file_manager.cwd, nk_rect(24, 64, 500, 450),
    NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_CLOSABLE)) {
        //nk_label(ctx, file_manager.cwd, NK_TEXT_ALIGN_LEFT);
        nk_layout_row_dynamic(ctx, 50,1);
        int i = 0;
        struct item_t* last_dir = file_manager.dirs;
        while (last_dir != NULL) {
                struct nk_style_button style = ctx->style.button;
                if (currently_selected == i) style.normal = style.hover;
                if (nk_button_label_styled(ctx, &style, last_dir->name) 
                    || (currently_selected == i && 
                        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)))){
                    open_dir(last_dir->name);
                    // We must stop drawing the list if the user opens a dir
                    // since open_dir resets the items in the list
                    goto end;
                }
            last_dir = last_dir->next;
            i++;
        }
        
        struct item_t* last_file = file_manager.files;
        while (last_file != NULL) {
            struct nk_style_button style = ctx->style.button;
            if (currently_selected == i) style.normal = style.hover;
            if (nk_button_label_styled(ctx, &style, last_file->name) 
                || (currently_selected == i && 
                    (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)))){
                char rom_path[256];
                sprintf(rom_path, "%s/%s", file_manager.cwd, last_file->name);
                load_cartridge(rom_path);
            }
            last_file = last_file->next;
            i++;
        }

        struct nk_window *win = nk_window_find(ctx,"ctx-files");
        // Automatically scroll the window to the selected button
        if (selection_changed && win &&
            (win->scrollbar.y + win->bounds.h < (currently_selected+2)*BUTTON_HIEGHT || 
            win->scrollbar.y > currently_selected*BUTTON_HIEGHT)) {
                win->scrollbar.y = currently_selected*BUTTON_HIEGHT;
        }
    }
    end:
    nk_end(ctx);
    if (nk_window_is_hidden(ctx, "ctx-files"))
        dialog_open = false;
}
#endif
