/* 
 * $Id: ogra2gdk.h,v 1.2 2008-09-11 07:07:22 hito Exp $
 */

#ifndef _O_GRA2GDK_HEADER
#define _O_GRA2GDK_HEADER

GdkPixmap *
gra2gdk_create_pixmap(struct objlist *obj, N_VALUE *inst, struct gra2cairo_local *local, GdkDrawable *drawable, int w, int h, double r, double g, double b);

#endif
