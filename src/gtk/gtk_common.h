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

#define CAIRO_COORDINATE_OFFSET 1

#define LINE_NUMBER_WIDGET_NAME "line_number"

#endif

