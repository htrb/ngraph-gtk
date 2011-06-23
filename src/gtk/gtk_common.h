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

#define CAIRO_COORDINATE_OFFSET 1

#endif

