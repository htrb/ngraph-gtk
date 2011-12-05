/* 
 * $Id: dir_defs.h.in,v 1.3 2009-06-18 11:32:11 hito Exp $
 */

#ifndef _DIR_DEFS_HEADER
#define _DIR_DEFS_HEADER

#ifdef WINDOWS
extern char *DOCDIR, *LIBDIR, *CONFDIR, *PIXMAPDIR;
#define HOME_DIR "Ngraph"
#else  /* WINDOWS */
#define DOCDIR    "/usr/share/doc/ngraph-gtk"
#define LIBDIR    "/usr/lib/ngraph-gtk"
#define CONFDIR   "/etc/ngraph-gtk"
#define PIXMAPDIR "/usr/share/pixmaps/ngraph-gtk"
#define HOME_DIR  ".Ngraph"
#endif	/* WINDOWS */

#define HELP_FILE "html_ja/ngraph.html"

#endif
