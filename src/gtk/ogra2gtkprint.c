/*
 * $Id: ogra2gtkprint.c,v 1.6 2008-09-18 08:13:43 hito Exp $
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
#include "object.h"
#include "ioutil.h"

#include "x11gui.h"
#include "ogra2cairo.h"

#define NAME "gra2gtkprint"
#define PARENT "gra2cairo"
#define OVERSION  "1.00.00"

#ifndef M_PI
#define M_PI 3.141592
#endif


static int
gra2gtkprint_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  return 0;
}

static int
gra2gtkprint_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local && local->cairo) {
    gra2cairo_draw_path(local);
    local->cairo = NULL;	/* the instance of cairo is created by GtkPrintContext */
  }

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static int
create_cairo(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  cairo_t *cairo;
  int dpi, id, r;
  struct gra2cairo_local *local;
  GtkPrintContext *gpc;
  (void) rval;

  gpc = GTK_PRINT_CONTEXT(argv[2]);

  if (gpc == NULL) {
    error(obj, CAIRO_STATUS_NULL_POINTER + 100);
    return 1;
  }

  cairo = gtk_print_context_get_cairo_context(gpc);

  r = cairo_status(cairo);
  if (r != CAIRO_STATUS_SUCCESS) {
    error(obj, r + 100);
    return 1;
  }

  _getobj(obj, "id", inst, &id);

  dpi = gtk_print_context_get_dpi_x(gpc);
  if (putobj(obj, "dpix", id, &dpi) < 0) {
    error(obj, ERRFIELD);
    return 1;
  }

  dpi = gtk_print_context_get_dpi_y(gpc);
  if (putobj(obj, "dpiy", id, &dpi) < 0) {
    error(obj, ERRFIELD);
    return 1;
  }

  _getobj(obj, "_local", inst, &local);

  local->cairo = cairo;

  return 0;
}

static struct objtable gra2gtkprint[] = {
  {"init", NVFUNC, NEXEC, gra2gtkprint_init, NULL, 0},
  {"done", NVFUNC, NEXEC, gra2gtkprint_done, NULL, 0},
  {"_context", NVFUNC, 0, create_cairo, NULL,0},
};

#define TBLNUM (sizeof(gra2gtkprint) / sizeof(*gra2gtkprint))

void *
addgra2gtkprint()
/* addgra2gtkprint() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2gtkprint, Gra2CairoErrMsgNum, Gra2CairoErrMsgs, NULL, NULL);
}
