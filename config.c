#include <glib.h>
#include <stdlib.h>
#include "config.h"

#define CONFIG_FILE "config.ini"
#define DEFAULT_FONT_SIZE 12.0
#define DEFAULT_FONT_NAME "Adwaita Mono"
#define DEFAULT_BG_COLOR "#111111"

static double font_size = DEFAULT_FONT_SIZE;
static gchar *font_name = NULL;
static gchar *bg_color = NULL;

double config_get_font_size(void) { return font_size; }
void config_set_font_size(double size) {
    font_size = size < 5.0 ? 5.0 : (size > 50.0 ? 50.0 : size);
}
const gchar* config_get_font_name(void) { return font_name; }
const gchar* config_get_bg_color(void) { return bg_color; }

void save_config(void) {
    GKeyFile *key_file = g_key_file_new();
    g_key_file_set_double(key_file, "Settings", "font_size", font_size);
    g_key_file_set_string(key_file, "Settings", "font_name", font_name);
    g_key_file_set_string(key_file, "Settings", "bg_color", bg_color);

    GError *error = NULL;
    g_key_file_save_to_file(key_file, CONFIG_FILE, &error);
    if (error) g_error_free(error);
    g_key_file_free(key_file);
}

void load_config(void) {
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;

    if (g_key_file_load_from_file(key_file, CONFIG_FILE, G_KEY_FILE_NONE, &error)) {
        double val = g_key_file_get_double(key_file, "Settings", "font_size", NULL);
        if (val > 0) font_size = val;
        
        g_free(font_name);
        font_name = g_key_file_get_string(key_file, "Settings", "font_name", NULL);
        
        g_free(bg_color);
        bg_color = g_key_file_get_string(key_file, "Settings", "bg_color", NULL);
    }
    if (error) g_error_free(error);

    if (!font_name) font_name = g_strdup(DEFAULT_FONT_NAME);
    if (!bg_color) bg_color = g_strdup(DEFAULT_BG_COLOR);
    g_key_file_free(key_file);
}

void config_cleanup(void) {
    g_free(font_name);
    g_free(bg_color);
}