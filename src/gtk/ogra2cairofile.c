/* 
 * $Id: ogra2cairofile.c,v 1.10 2008/07/02 13:35:09 hito Exp $
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
#include "ogra2cairofile.h"

#define NAME "gra2cairofile"
#define PARENT "gra2cairo"
#define OVERSION  "1.00.00"

#define ERRFOPEN 100

#ifndef M_PI
#define M_PI 3.141592
#endif

char *surface_type[] = {
  "ps2",
  "ps3",
  "eps2",
  "eps3",
  "pdf",
  "svg1.1",
  "svg1.2",
  "png",
  NULL,
};

char *gra2cairofile_errorlist[]={
  "I/O error: open file"
};

#define ERRNUM (sizeof(gra2cairofile_errorlist) / sizeof(*gra2cairofile_errorlist))


static int 
gra2cairofile_init(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  int dpi;
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  dpi = 72;
  
  if (_putobj(obj, "dpi", inst, &dpi) < 0)
    return 1;

  if (_putobj(obj, "dpix", inst, &dpi) < 0)
    return 1;

  if (_putobj(obj, "dpiy", inst, &dpi) < 0)
    return 1;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_x = 
  local->pixel_dot_y = dpi / (DPI_MAX * 1.0);

  return 0;
}

static int 
gra2cairofile_done(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static cairo_t *
create_cairo(struct objlist *obj, char *inst, char *fname, int iw, int ih)
{
  cairo_surface_t *ps;
  cairo_t *cairo;
  double w, h;
  int format, dpi;

  _getobj(obj, "format", inst, &format);
  _getobj(obj, "dpi", inst, &dpi);

  w = iw * dpi / 25.4 / 100;
  h = ih * dpi / 25.4 / 100;

  switch (format) {
  case TYPE_PS2:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_2);
    cairo_ps_surface_set_eps(ps, FALSE);
    break;
  case TYPE_PS3:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_3);
    cairo_ps_surface_set_eps(ps, FALSE);
    break;
  case TYPE_EPS2:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_2);
    cairo_ps_surface_set_eps(ps, TRUE);
    break;
  case TYPE_EPS3:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_3);
    cairo_ps_surface_set_eps(ps, TRUE);
    break;
  case TYPE_PDF:
    ps = cairo_pdf_surface_create(fname, w, h);
    break;
  case TYPE_SVG1_1:
    ps = cairo_svg_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_1);
    break;
  case TYPE_SVG1_2:
    ps = cairo_svg_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_2);
    break;
  case TYPE_PNG:
    ps = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    break;
  default:
    ps = cairo_ps_surface_create(fname, w, h);
  }

  if (cairo_surface_status(ps) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(ps);
    return NULL;
  }

  cairo = cairo_create(ps);
  /* cairo_create() references target, so you can immediately call cairo_surface_destroy() on it */
  cairo_surface_destroy(ps);

  if (cairo_status(cairo) != CAIRO_STATUS_SUCCESS) {
    cairo_destroy(cairo);
    return NULL;
  }

  if (format == TYPE_PNG) {
    cairo_set_source_rgb(cairo, 1, 1, 1);
    cairo_rectangle(cairo, 0, 0, w, h);
    cairo_fill(cairo);
    cairo_new_path(cairo);
  }

  return cairo;
}

static int
init_cairo(struct objlist *obj, char *inst, struct gra2cairo_local *local, int w, int h)
{
  char *fname;
  cairo_t *cairo;
  int t2p;

  _getobj(obj, "file", inst, &fname);
  if (fname == NULL)
    return 1;

  cairo = create_cairo(obj, inst, fname, w, h);

  if (cairo == NULL)
    return 1;

  if (local->cairo)
    cairo_destroy(local->cairo);

  local->cairo = cairo;

  _getobj(obj, "text2path", inst, &t2p);

  local->text2path = t2p;

  return 0;
}

static int 
gra2cairofile_output(struct objlist *obj, char *inst, char *rval, 
                 int argc, char **argv)
{
  char code, *cstr, *fname;
  int *cpar, format;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  switch (code) {
  case 'I':
    if (init_cairo(obj, inst, local, cpar[3], cpar[4]))
      return 1;
    break;
  case 'E':
    _getobj(obj, "format", inst, &format);
    if (local->cairo && format == TYPE_PNG) {
      _getobj(obj, "file", inst, &fname);
      if (fname == NULL)
	return 1;

      cairo_surface_write_to_png(cairo_get_target(local->cairo), fname);
    }
    break;
  }

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static struct objtable gra2cairofile[] = {
  {"init", NVFUNC, NEXEC, gra2cairofile_init, NULL, 0}, 
  {"done", NVFUNC, NEXEC, gra2cairofile_done, NULL, 0}, 
  {"next", NPOINTER, 0, NULL, NULL, 0}, 
  {"file", NSTR, NREAD|NWRITE, NULL, NULL,0},
  {"text2path", NBOOL, NREAD|NWRITE, NULL, NULL,0},
  {"format", NENUM, NREAD | NWRITE, NULL, surface_type, 0},
  {"_output", NVFUNC, 0, gra2cairofile_output, NULL, 0}, 
  {"_charwidth", NIFUNC, 0, gra2cairo_charwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
};

#define TBLNUM (sizeof(gra2cairofile) / sizeof(*gra2cairofile))

void *
addgra2cairofile()
/* addgra2cairofile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairofile, ERRNUM, gra2cairofile_errorlist, NULL, NULL);
}
