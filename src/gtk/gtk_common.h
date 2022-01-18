#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#include "common.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cairo/cairo.h>
#include <gtksourceview/gtksource.h>

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) gtk_widget_get_visible(w)
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

#define TOOLBUTTON_CLASS "toolbutton"
#define MENUBUTTON_CLASS "menutoolbutton"
#define MARKERBUTTON_CLASS "markerbutton"

#define POPOVERMEU_FLAG GTK_POPOVER_MENU_NESTED

#if GTK_CHECK_VERSION(4, 0, 0)
enum CURSOR_TYPE {
  GDK_LEFT_PTR,
  GDK_XTERM,
  GDK_CROSSHAIR,
  GDK_TOP_LEFT_CORNER,
  GDK_TOP_RIGHT_CORNER,
  GDK_BOTTOM_RIGHT_CORNER,
  GDK_BOTTOM_LEFT_CORNER,
  GDK_TARGET,
  GDK_PLUS,
  GDK_SIZING,
  GDK_WATCH,
  GDK_FLEUR,
  GDK_PENCIL,
  GDK_TCROSS,
};
#endif

#endif
