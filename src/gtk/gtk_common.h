#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#ifdef HAVE_GETTEXT

#define DEFAULT_TEXT_DOMAIN PACKAGE
#include "gettext.h"
#define _(String)   gettext(String)
#define N_(String)  gettext_noop(String)

#else /* HAVE_GETTEXT */

#define _(String)   (String)
#define N_(String)  (String)

#endif /* HAVE_GETTEXT */

#define PLATFORM "for GTK+"

#define USE_ENTRY_ICON (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 16))


#endif

