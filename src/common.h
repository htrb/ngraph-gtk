#ifndef COMMON_HEADER
#define COMMON_HEADER

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#ifdef G_OS_WIN32
#define WINDOWS 1
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#else
#define WINDOWS 0
#endif

#ifdef __APPLE__
#define OSX 1
#else
#define OSX 0
#endif

#ifdef __WIN64__
#define WINDOWS 1
#include <windows.h>
#endif

#ifndef CCNAME
#define CCNAME "unknown";
#endif

#ifndef __VERSION__
#define __VERSION__ ""
#endif

#define COMPILER_NAME (CCNAME " " __VERSION__)

#define MARK_TYPE_NUM 90

int printfconsole(const char *fmt,...);
int putconsole(const char *s);
int printfconsole(const char *fmt,...);
void displaydialog(const char *str);
void displaystatus(const char *str);

#define DOUBLE_STR_FORMAT "%.16g"

#define USE_EVENT_LOOP 0

#endif	/* COMMON_HEADER */
