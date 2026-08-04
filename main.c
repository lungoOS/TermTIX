#include <gtk/gtk.h>
#include <vte/vte.h>
#include <pango/pango.h>
#include "config.h"
#include "terminal.h"

static GtkWidget *size_label;
static guint size_timeout_id = 0;

static gboolean hide_size_label_callback(gpointer data) {
    gtk_widget_hide(size_label);
    size_timeout_id = 0;
    return FALSE;
}

gboolean on_window_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) {
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(user_data));
    if (current_page >= 0) {
        GtkWidget *term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(user_data), current_page);
        long cols = vte_terminal_get_column_count(VTE_TERMINAL(term));
        long rows = vte_terminal_get_row_count(VTE_TERMINAL(term));

        gchar *size_str = g_strdup_printf(" %ld × %ld ", cols, rows);
        gtk_label_set_text(GTK_LABEL(size_label), size_str);
        g_free(size_str);
        gtk_widget_show(size_label);

        if (size_timeout_id != 0)
            g_source_remove(size_timeout_id);
        size_timeout_id = g_timeout_add(1000, hide_size_label_callback, NULL);
    }
    return FALSE;
}

void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    if (size_timeout_id != 0)
        g_source_remove(size_timeout_id);
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    load_config();

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "VteTerminal, vte-terminal, .vte-terminal { padding: 15px; border: none; }"
        "#sizelabel { background-color: #2e2e2e; color: #ffffff; font-family: monospace; font-size: 12px; padding: 4px; border: 1px solid #555555; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "TermTIX V1.2");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), overlay);

    terminal_init(overlay);

    size_label = gtk_label_new("");
    gtk_widget_set_name(size_label, "sizelabel");
    gtk_widget_set_halign(size_label, GTK_ALIGN_END);
    gtk_widget_set_valign(size_label, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(size_label, 10);
    gtk_widget_set_margin_end(size_label, 10);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), size_label);

    gtk_widget_add_events(window, GDK_SCROLL_MASK | GDK_KEY_PRESS_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(window, "scroll-event", G_CALLBACK(on_scroll), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), window);
    g_signal_connect(window, "configure-event", G_CALLBACK(on_window_configure), terminal_get_notebook());
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_window_motion), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    gtk_widget_show_all(window);
    gtk_widget_hide(size_label);
    gtk_main();

    config_cleanup();
    return 0;
}