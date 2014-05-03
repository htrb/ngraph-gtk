#ifndef COMMON_HEADER
#define COMMON_HEADER

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// #define USE_PLOT_OBJ
#define USE_AXIS_MATH

#ifdef HAVE_GETTEXT

#define DEFAULT_TEXT_DOMAIN PACKAGE
#include "gettext.h"
#define _(String)   gettext(String)
#define N_(String)  gettext_noop(String)

#else /* HAVE_GETTEXT */

#define _(String)   (String)
#define N_(String)  (String)

#endif /* HAVE_GETTEXT */

#include <glib.h>
#include <glib/gstdio.h>

#ifdef __MINGW32__
#define WINDOWS 1
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#endif

#ifndef CCNAME
#define CCNAME "unknown";
#endif

#ifndef __VERSION__
#define __VERSION__ ""
#endif

#define COMPILER_NAME (CCNAME " " __VERSION__)

#define USE_MEM_PROFILE 0

#define MARK_TYPE_NUM 90

int printfconsole(char *fmt,...);
int putconsole(const char *s);
int printfconsole(char *fmt,...);
void displaydialog(const char *str);
void displaystatus(const char *str);
void pausewindowconsole(char *title,char *str);

#endif	/* COMMON_HEADER */
