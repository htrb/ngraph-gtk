#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cairo/cairo.h>

#include "common.h"

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) gtk_widget_get_visible(w)
#endif

#define USE_HEADER_BAR 0
#define USE_APP_MENU GTK_CHECK_VERSION(3, 12, 0)
#define USE_GTK_BUILDER 0

#if GTK_CHECK_VERSION(3, 2, 0)
#define gtk_hbox_new(h, s)      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, s)
#define gtk_vbox_new(h, s)      gtk_box_new(GTK_ORIENTATION_VERTICAL, s)
#define gtk_hbutton_box_new()   gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_hseparator_new()    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_hscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adj)
#define gtk_vscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adj)
#define gtk_hscale_new_with_range(min, max, step) gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step)
#endif

#define CAIRO_COORDINATE_OFFSET 1

#define LINE_NUMBER_WIDGET_NAME "line_number"

#endif

