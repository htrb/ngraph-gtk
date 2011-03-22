/* 
 * $Id: ogra2gdk.c,v 1.5 2009-11-16 09:13:05 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>

#include "mathfn.h"
#include "ngraph.h"
#include "object.h"
#include "ioutil.h"

#include "x11gui.h"
#include "ogra2cairo.h"

#define NAME "gra2gdk"
#define PARENT "gra2cairo"
#define OVERSION  "1.00.00"

#ifndef M_PI
#define M_PI 3.141592
#endif

static int 
g2g_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{  
  struct gra2cairo_local *local;
  int dpi;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  dpi = DPI_MAX;

  if (_putobj(obj, "dpi", inst, &dpi) < 0)
    goto Err;

  if (_putobj(obj, "dpix", inst, &dpi) < 0)
    goto Err;

  if (_putobj(obj, "dpiy", inst, &dpi) < 0)
    goto Err;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_x = 
  local->pixel_dot_y = dpi / (DPI_MAX * 1.0);

  return 0;

 Err:
  local = gra2cairo_free(obj, inst);
  g_free(local);

  return 1;
}

static int 
g2g_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

GdkPixmap *
gra2gdk_create_pixmap(struct objlist *obj, N_VALUE *inst, struct gra2cairo_local *local,
		      GdkDrawable *drawable, int w, int h, double r, double g, double b)
{
  cairo_t *cairo;
  GdkPixmap *pix;

  if (drawable == NULL) {
    return NULL;
  }

  pix = gdk_pixmap_new(drawable, w, h, -1);
  cairo = gdk_cairo_create(pix);

  if (cairo_status(cairo) != CAIRO_STATUS_SUCCESS) {
    cairo_destroy(cairo);
    g_object_unref(pix);
    return NULL;
  }

  cairo_set_source_rgb(cairo, r, g, b);
  cairo_rectangle(cairo, 0, 0, w, h);
  cairo_fill(cairo);

  if (local->cairo) {
    cairo_destroy(local->cairo);
  }

  local->cairo = cairo;

  return pix;
}

static struct objtable gra2gdk[] = {
  {"init", NVFUNC, NEXEC, g2g_init, NULL, 0}, 
  {"done", NVFUNC, NEXEC, g2g_done, NULL, 0}, 
  {"next", NPOINTER, 0, NULL, NULL, 0}, 
};

#define TBLNUM (sizeof(gra2gdk) / sizeof(*gra2gdk))

void *
addgra2gdk()
/* addgra2gdk() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2gdk, Gra2CairoErrMsgNum, Gra2CairoErrMsgs, NULL, NULL);
}
