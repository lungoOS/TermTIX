#ifndef TERMINAL_H
#define TERMINAL_H

#include <gtk/gtk.h>

GtkWidget* terminal_get_notebook(void);
void terminal_init(GtkWidget *overlay);
void change_font_size(double delta);
gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean on_window_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);

#endif