#include <gtk/gtk.h>
#include <vte/vte.h>
#include <pango/pango.h>
#include <stdlib.h>
#include "terminal.h"
#include "config.h"

static GtkWidget *notebook_widget;
static guint hide_tabs_timeout = 0;
static gboolean tabs_visible = FALSE;
static GtkWidget *main_window = NULL;

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

GtkWidget* terminal_get_notebook(void) {
    return notebook_widget;
}

static void apply_custom_background(GtkWidget *term) {
    GdkRGBA bg;
    const gchar *bg_color = config_get_bg_color();
    if (gdk_rgba_parse(&bg, bg_color))
        vte_terminal_set_colors(VTE_TERMINAL(term), NULL, &bg, NULL, 0);
}

static void update_all_tabs_font(void) {
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_widget));
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_widget), i);
        PangoFontDescription *font_desc = pango_font_description_from_string(config_get_font_name());
        pango_font_description_set_size(font_desc, config_get_font_size() * PANGO_SCALE);
        vte_terminal_set_font(VTE_TERMINAL(term), font_desc);
        pango_font_description_free(font_desc);
    }
}

void change_font_size(double delta) {
    config_set_font_size(config_get_font_size() + delta);
    update_all_tabs_font();
    save_config();
}

static void on_spawn_complete(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data) {
    if (error) {
        g_printerr("Failed to spawn shell: %s\n", error->message);
        int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook_widget), GTK_WIDGET(terminal));
        if (page_num >= 0)
            gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_widget), page_num);
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_widget)) == 0)
            gtk_widget_destroy(main_window);
    }
}

static void on_child_exited(VteTerminal *terminal, gint status, gpointer user_data) {
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook_widget), GTK_WIDGET(terminal));
    if (page_num >= 0)
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_widget), page_num);
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_widget)) == 0)
        gtk_widget_destroy(main_window);
}

static void create_new_tab(void) {
    GtkWidget *new_terminal = vte_terminal_new();
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(new_terminal), 2000);
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(new_terminal), VTE_CURSOR_BLINK_ON);
    apply_custom_background(new_terminal);

    PangoFontDescription *font_desc = pango_font_description_from_string(config_get_font_name());
    pango_font_description_set_size(font_desc, config_get_font_size() * PANGO_SCALE);
    vte_terminal_set_font(VTE_TERMINAL(new_terminal), font_desc);
    pango_font_description_free(font_desc);

    g_signal_connect(new_terminal, "child-exited", G_CALLBACK(on_child_exited), NULL);
    g_signal_connect(new_terminal, "button-press-event", G_CALLBACK(on_button_press), NULL);

    gchar *shell = vte_get_user_shell();
    if (!shell) shell = g_strdup("/bin/sh");
    gchar *argv[] = { shell, NULL };

    vte_terminal_spawn_async(VTE_TERMINAL(new_terminal), VTE_PTY_DEFAULT, NULL, argv, NULL,
                             G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, on_spawn_complete, NULL);

    GtkWidget *label = gtk_label_new("Terminal");
    int index = gtk_notebook_append_page(GTK_NOTEBOOK(notebook_widget), new_terminal, label);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_widget), index);
    gtk_widget_show_all(notebook_widget);
    g_free(shell);
}

static void on_menu_new_tab(GtkMenuItem *item, gpointer data) { create_new_tab(); }
static void on_menu_copy(GtkMenuItem *item, gpointer data) { vte_terminal_copy_clipboard_format(VTE_TERMINAL(data), VTE_FORMAT_TEXT); }
static void on_menu_paste(GtkMenuItem *item, gpointer data) { vte_terminal_paste_clipboard(VTE_TERMINAL(data)); }

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *item;

        item = gtk_menu_item_new_with_label("Copy");
        g_signal_connect(item, "activate", G_CALLBACK(on_menu_copy), widget);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = gtk_menu_item_new_with_label("Paste");
        g_signal_connect(item, "activate", G_CALLBACK(on_menu_paste), widget);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

        item = gtk_menu_item_new_with_label("Create New Tab");
        g_signal_connect(item, "activate", G_CALLBACK(on_menu_new_tab), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    }
    return FALSE;
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->state & GDK_CONTROL_MASK && event->state & GDK_SHIFT_MASK) {
        int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_widget));
        
        switch (event->keyval) {
            case GDK_KEY_T: case GDK_KEY_t:
                create_new_tab();
                return TRUE;
            case GDK_KEY_W: case GDK_KEY_w:
                if (current_page >= 0) {
                    GtkWidget *term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_widget), current_page);
                    on_child_exited(VTE_TERMINAL(term), 0, NULL);
                }
                return TRUE;
            case GDK_KEY_C: case GDK_KEY_c:
                if (current_page >= 0)
                    vte_terminal_copy_clipboard_format(VTE_TERMINAL(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_widget), current_page)), VTE_FORMAT_TEXT);
                return TRUE;
            case GDK_KEY_V: case GDK_KEY_v:
                if (current_page >= 0)
                    vte_terminal_paste_clipboard(VTE_TERMINAL(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_widget), current_page)));
                return TRUE;
        }
    }
    
    if (event->state & GDK_CONTROL_MASK && !(event->state & GDK_SHIFT_MASK)) {
        switch (event->keyval) {
            case GDK_KEY_Up:
                change_font_size(1.0);
                return TRUE;
            case GDK_KEY_Down:
                change_font_size(-1.0);
                return TRUE;
            case GDK_KEY_Page_Up:
                gtk_notebook_prev_page(GTK_NOTEBOOK(notebook_widget));
                return TRUE;
            case GDK_KEY_Page_Down:
                gtk_notebook_next_page(GTK_NOTEBOOK(notebook_widget));
                return TRUE;
        }
    }
    
    return FALSE;
}

gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data) {
    if (event->state & GDK_CONTROL_MASK) {
        switch (event->direction) {
            case GDK_SCROLL_UP:   change_font_size(1.0); return TRUE;
            case GDK_SCROLL_DOWN: change_font_size(-1.0); return TRUE;
            default: break;
        }
    }
    return FALSE;
}

static gboolean hide_tabs(gpointer data) {
    if (tabs_visible) {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook_widget), FALSE);
        tabs_visible = FALSE;
    }
    hide_tabs_timeout = 0;
    return FALSE;
}

gboolean on_window_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    if (event->y < 30) {
        if (!tabs_visible) {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook_widget), TRUE);
            tabs_visible = TRUE;
        }
        if (hide_tabs_timeout) {
            g_source_remove(hide_tabs_timeout);
            hide_tabs_timeout = 0;
        }
    } else if (tabs_visible && !hide_tabs_timeout) {
        hide_tabs_timeout = g_timeout_add(500, hide_tabs, NULL);
    }
    return FALSE;
}

void terminal_init(GtkWidget *overlay) {
    notebook_widget = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook_widget), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook_widget), FALSE);
    gtk_container_add(GTK_CONTAINER(overlay), notebook_widget);
    
    main_window = gtk_widget_get_toplevel(overlay);
    create_new_tab();
}