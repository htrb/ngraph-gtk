#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cairo/cairo.h>
#include <gtksourceview/gtksource.h>

#include "common.h"

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) gtk_widget_get_visible(w)
#endif

#if OSX
#define USE_GTK_BUILDER 1
#else
#define USE_GTK_BUILDER 0
#endif

#if WINDOWS || OSX
#define USE_HEADER_BAR 0
#else
#define USE_HEADER_BAR 1
#endif

#define RECENT_CHOOSER_LIMIT 25

#define APPLICATION_ID "com.github.htrb.ngraph-gtk"
#define RESOURCE_PATH "/com/github/htrb/ngraph-gtk"

#define CAIRO_COORDINATE_OFFSET 1

#define LINE_NUMBER_WIDGET_NAME "line_number"

#endif

