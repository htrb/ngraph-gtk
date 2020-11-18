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

#ifdef GTK_SOURCE_CHECK_VERSION
#undef GTK_SOURCE_CHECK_VERSION
#define GTK_SOURCE_CHECK_VERSION(major, minor, micro) \
	(GTK_SOURCE_MAJOR_VERSION > (major) || \
	(GTK_SOURCE_MAJOR_VERSION == (major) && GTK_SOURCE_MINOR_VERSION > (minor)) || \
	(GTK_SOURCE_MAJOR_VERSION == (major) && GTK_SOURCE_MINOR_VERSION == (minor) && \
	 GTK_SOURCE_MICRO_VERSION >= (micro)))
#endif

#if WINDOWS || OSX
#define USE_HEADER_BAR 0
#else
#define USE_HEADER_BAR 1
#endif

#if OSX
#define USE_EVENT_CONTROLLER 0
#else
#if GTK_CHECK_VERSION(3, 24, 0)
#define USE_EVENT_CONTROLLER 1
#else
#define USE_EVENT_CONTROLLER 0
#endif

#define RECENT_CHOOSER_LIMIT 25

#define APPLICATION_ID "com.github.htrb.ngraph-gtk"
#define RESOURCE_PATH "/com/github/htrb/ngraph-gtk"

#define CAIRO_COORDINATE_OFFSET 1

#endif
