/* 
 * $Id: ogra2gdk.h,v 1.2 2008-09-11 07:07:22 hito Exp $
 */

#ifndef _O_GRA2GDK_HEADER
#define _O_GRA2GDK_HEADER

GdkPixmap *
gra2gdk_create_pixmap(struct objlist *obj, char *inst, struct gra2cairo_local *local, GdkDrawable *drawable, int w, int h, int r, int g, int b);

#endif
