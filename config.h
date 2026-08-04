#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

void load_config(void);
void save_config(void);
void config_cleanup(void);
double config_get_font_size(void);
void config_set_font_size(double size);
const gchar* config_get_font_name(void);
const gchar* config_get_bg_color(void);

#endif