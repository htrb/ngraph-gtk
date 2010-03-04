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

#ifdef __MINGW32__
#define WINDOWS 1
#include <windows.h>
#endif

#endif	/* COMMON_HEADER */
