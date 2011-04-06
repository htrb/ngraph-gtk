#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cairo/cairo.h>

#include "common.h"

#define USE_ENTRY_ICON GTK_CHECK_VERSION(2, 16, 0)

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) gtk_widget_get_visible(w)
#endif

#if GTK_CHECK_VERSION(2, 18, 0)
#define GTK_WIDGET_SET_CAN_FOCUS(w) gtk_widget_set_can_focus(w, TRUE)
#else
#define GTK_WIDGET_SET_CAN_FOCUS(w) GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS)
#endif

#if GTK_CHECK_VERSION(2, 14, 0)
#define GTK_WIDGET_GET_WINDOW(w) gtk_widget_get_window(w)
#define GTK_DIALOG_GET_CONTENT_AREA(w) gtk_dialog_get_content_area(w)
#define GTK_COLOR_SELECTION_DIALOG_GET_COLOR_SELECTION(w) gtk_color_selection_dialog_get_color_selection(w)
#else
#define GTK_WIDGET_GET_WINDOW(w) (w)->window
#define GTK_DIALOG_GET_CONTENT_AREA(w) (w)->vbox
#define GTK_COLOR_SELECTION_DIALOG_GET_COLOR_SELECTION(w) (w)->colorsel
#endif

#define CAIRO_COORDINATE_OFFSET 1

#endif

