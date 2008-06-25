/* 
 * $Id: ogra2cairofile.c,v 1.1 2008/06/25 08:57:42 hito Exp $
 * 
 * This file is part of "Ngraph for X11".
 * 
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
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
  NULL,
};

static enum surface_type_id {
  TYPE_PS2,
  TYPE_PS3,
  TYPE_EPS2,
  TYPE_EPS3,
  TYPE_PDF,
  TYPE_SVG1_1,
  TYPE_SVG1_2,
};

char *gra2cairofile_errorlist[]={
  "I/O error: open file"
};

#define ERRNUM (sizeof(gra2cairofile_errorlist) / sizeof(*gra2cairofile_errorlist))


struct gra2cairofile_local {
  cairo_t* cairo;
  int linetonum, offsetx, offsety, cpx, cpy, region[4], region_active;
  char *fontalias;
  double pixel_dot;
};

static int 
gra2cairofile_init(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static int 
gra2cairofile_done(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  cairo_surface_t *sf;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static  cairo_surface_t *
create_surface(struct objlist *obj, char *inst, char *fname, int iw, int ih)
{
  cairo_surface_t *ps;
  double w, h;
  int format;

  _getobj(obj, "format", inst, &format);

  w = iw * 72.0 / 25.4 / 100;
  h = ih * 72.0 / 25.4 / 100;

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
    ps = cairo_ps_surface_create(fname, w, h);
    break;
  case TYPE_SVG1_1:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_1);
    break;
  case TYPE_SVG1_2:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_2);
    break;
  default:
    ps = cairo_ps_surface_create(fname, w, h);
  }

  return ps;
}

static int 
gra2cairofile_output(struct objlist *obj, char *inst, char *rval, 
                 int argc, char **argv)
{
  char code, *cstr, *fname;
  int *cpar, dpi;
  cairo_surface_t *ps;
  cairo_t *cairo;
  char *sargv[2];

  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  switch (code) {
  case 'I':
    _getobj(obj, "file", inst, &fname);
    if (fname == NULL)
      return 1;

    dpi = 72;
    if (_putobj(obj, "dpi", inst, &dpi) < 0)
      return 1;

    ps = create_surface(obj, inst, fname, cpar[3], cpar[4]);
    if (cairo_surface_status(ps) != CAIRO_STATUS_SUCCESS) {
      cairo_surface_destroy(ps);
      return 1;
    }

    cairo = cairo_create(ps);
    if (cairo_status(cairo) != CAIRO_STATUS_SUCCESS) {
      cairo_destroy(cairo);
      return 1;
    }

    /* cairo_status() references target, so you can immediately call cairo_surface_destroy() on it */
    cairo_surface_destroy(ps);

    sargv[0] = (char *) cairo;
    sargv[1] = NULL;
    _exeobj(obj, "cairo", inst, 1, sargv);

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
  {"format", NENUM, NREAD | NWRITE, NULL, surface_type, 0},
  {"_output", NVFUNC, 0, gra2cairofile_output, NULL, 0}, 
};

#define TBLNUM (sizeof(gra2cairofile) / sizeof(*gra2cairofile))

void *
addgra2cairofile()
/* addgra2cairofile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairofile, ERRNUM, gra2cairofile_errorlist, NULL, NULL);
}
