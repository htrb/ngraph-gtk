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

#ifdef __MINGW32__
#define WINDOWS 1
#define _WIN32_WINNT 0x0500
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

#endif	/* COMMON_HEADER */
