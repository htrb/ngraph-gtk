/* 
 * $Id: ogra2cairo.c,v 1.1 2008/06/25 08:57:41 hito Exp $
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

#include "mathfn.h"
#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "ioutil.h"

#include "x11gui.h"

#define NAME "gra2cairo"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define ERRFOPEN 100

#ifndef M_PI
#define M_PI 3.141592
#endif

char *gra2cairo_errorlist[]={
  "I/O error: open file"
};

#define ERRNUM (sizeof(gra2cairo_errorlist) / sizeof(*gra2cairo_errorlist))


struct gra2cairo_local {
  cairo_t *cairo;
  int linetonum, region_active;
  char *fontalias;
  double pixel_dot, offsetx, offsety, cpx, cpy, region[4];
};

static double
mxd2p(struct gra2cairo_local *local, int r)
{
  return r * local->pixel_dot;
}

static double
mxd2px(struct gra2cairo_local *local, int x)
{
  return x * local->pixel_dot + local->offsetx;
}

static double
mxd2py(struct gra2cairo_local *local, int y)
{
  return y * local->pixel_dot + local->offsety;
}


static int 
gra2cairo_cairo(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &local);

  if (local->cairo != NULL)
    cairo_destroy(local->cairo);

  local->cairo = (cairo_t *) argv[2];

  return 0;
}

static int
gra2cairo_init(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  local = memalloc(sizeof(struct gra2cairo_local));
  if (local == NULL)
    goto errexit;

  if (_putobj(obj, "_local", inst, local))
    goto errexit;

  local->cairo = NULL;
  local->fontalias = NULL;
  local->pixel_dot = 1;
  local->linetonum = 0;

  local->region[0] = local->region[1] =
    local->region[2] = local->region[3] = 0;

  local->region_active = FALSE;

  return 0;

 errexit:
  memfree(local);
  return 1;
}

static int 
gra2cairo_done(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &local);

  if (local->cairo) {
    cairo_destroy(local->cairo);
  }

  return 0;
}

static int 
gra2cairo_set_region(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  int i;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &local);

  if (argc == 4) {
    for (i = 0; i < 2; i++) {
      local->region[i] = mxd2px(local, *(int *) argv[i * 2 + 2]);
      local->region[i] = mxd2py(local, *(int *) argv[i * 2 + 3]);
    }
    local->region_active = TRUE;
  } else {
    for (i = 0; i < 4; i++) {
      local->region[i] = 0;
    }
    local->region_active = FALSE;
  }

  return 0;
}

static void
gra2cairo_set_dpi(struct gra2cairo_local *local, struct objlist *obj, char *inst)
{
  int dpi;

  _getobj(obj, "dpi", inst, &dpi);

  if (dpi < 1)
    dpi = 1;

  if (dpi > DPI_MAX)
    dpi = DPI_MAX;

  local->pixel_dot = dpi / (DPI_MAX * 1.0);
}

static int 
gra2cairo_output(struct objlist *obj, char *inst, char *rval, 
                 int argc, char **argv)
{
  char code, *cstr, *tmp, *tmp2;
  int *cpar, i, j;
  double x, y, w, h, l, fontcashsize, fontcashdir, fontsize,
    fontspace, fontdir, fontsin, fontcos, a1, a2;
  cairo_line_join_t join;
  cairo_line_cap_t cap;
  cairo_t *cairo;
  double *dashlist = NULL;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  if (local->cairo == NULL)
    return -1;

  if (local->linetonum != 0 && code != 'T') {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }
  switch (code) {
  case 'I':
    gra2cairo_set_dpi(local, obj, inst);
    local->linetonum = 0;
    break;
  case '%': case 'X':
    break;
  case 'E':
    break;
  case 'V':
    local->offsetx = mxd2p(local, cpar[1]);
    local->offsety = mxd2p(local, cpar[2]);
    local->cpx = 0;
    local->cpy = 0;
    if (cpar[5]) {
      x = mxd2p(local, cpar[1]);
      y = mxd2p(local, cpar[2]);
      w = mxd2p(local, cpar[3]) - x;
      h = mxd2p(local, cpar[4]) - y;

      cairo_new_path(local->cairo);
      cairo_rectangle(local->cairo, x, y, w, h);

      if (local->region_active) {
	cairo_rectangle(local->cairo,
			local->region[0], local->region[1],
			local->region[2], local->region[3]);
      }

      cairo_clip(local->cairo);
    } else {
      if (local->region) {
	cairo_rectangle(local->cairo,
			local->region[0], local->region[1],
			local->region[2], local->region[3]);
      } else {
	cairo_rectangle(local->cairo, 0, 0, SHRT_MAX, SHRT_MAX);
      }
      cairo_clip(local->cairo);
    }
    break;
  case 'A':
    if (cpar[1] == 0) {
      cairo_set_dash(local->cairo, NULL, 0, 0);
    } else {
      dashlist = memalloc(sizeof(* dashlist) * cpar[1]);
      if (dashlist == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = mxd2p(local, cpar[6 + i]);
        if (dashlist[i] <= 0) {
	  dashlist[i] = 1;
	}
      }
      cairo_set_dash(local->cairo, dashlist, cpar[1], 0);
      memfree(dashlist);
    }

    cairo_set_line_width(local->cairo, mxd2p(local, cpar[2]));

    if (cpar[3] == 2) {
      cap = CAIRO_LINE_CAP_SQUARE;
    } else if (cpar[3] == 1) {
      cap = CAIRO_LINE_CAP_ROUND;
    } else {
      cap = CAIRO_LINE_CAP_BUTT;
    }
    cairo_set_line_cap(local->cairo, cap);

    if (cpar[4] == 2) {
      join = CAIRO_LINE_JOIN_BEVEL;
    } else if (cpar[4] == 1) {
      join = CAIRO_LINE_JOIN_ROUND;
    } else {
      join = CAIRO_LINE_JOIN_MITER;
    }
    cairo_set_line_join(local->cairo, join);
    break;
  case 'G':
    cairo_set_source_rgb(local->cairo, cpar[1] / 255.0, cpar[2] / 255.0, cpar[3] / 255.0);
    break;
  case 'M':
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    break;
  case 'N':
    cairo_rel_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    break;
  case 'L':
    cairo_new_path(local->cairo);
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    cairo_line_to(local->cairo, mxd2px(local, cpar[3]), mxd2px(local, cpar[4]));
    cairo_stroke(local->cairo);
    break;
  case 'T':
    cairo_line_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    local->linetonum++;
    break;
  case 'C':
    cairo_new_path(local->cairo);
    x = mxd2px(local, cpar[1] - cpar[3]);
    y = mxd2py(local, cpar[2] - cpar[4]);
    w = mxd2p(local, cpar[3]);
    h = mxd2p(local, cpar[4]);
    a1 = cpar[5] * (M_PI / 18000.0);
    a2 = cpar[6] * (M_PI / 18000.0) + a1;

    cairo_save(local->cairo);
    cairo_translate(local->cairo, x + w, y + h);
    cairo_scale(local->cairo, w, h);
    cairo_arc_negative(local->cairo, 0., 0., 1., -a1, -a2);
    cairo_restore (local->cairo);
    if (cpar[7] == 0) {
      cairo_stroke(local->cairo);
    } else {
      if (cpar[7] == 1) {
	cairo_line_to(local->cairo, x + w, y + h);
      }
      cairo_close_path(local->cairo);
      cairo_fill(local->cairo);
    }
    break;
  case 'B':
    cairo_new_path(local->cairo);
    if (cpar[1] <= cpar[3]) {
      x = mxd2px(local, cpar[1]);
      w = mxd2p(local, cpar[3] - cpar[1]);
    } else {
      x = mxd2px(local, cpar[3]);
      w = mxd2p(local, cpar[1] - cpar[3]);
    }

    if (cpar[2] <= cpar[4]) {
      y = mxd2py(local, cpar[2]);
      h = mxd2p(local, cpar[4] - cpar[2]);
    } else {
      y = mxd2py(local, cpar[4]);
      h = mxd2p(local, cpar[2] - cpar[4]);
    }
    cairo_rectangle(local->cairo, x, y, w, h);
    if (cpar[5] == 0) {
      cairo_stroke(local->cairo);
    } else {
      cairo_fill(local->cairo);
    }
    break;
  case 'P': 
    cairo_new_path(local->cairo);
    cairo_arc(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]), mxd2p(local, 10), 0, 2 * M_PI);
    cairo_fill(local->cairo);
    break;
  case 'R': 
    cairo_new_path(local->cairo);
    if (cpar[1] == 0)
      break;

    for (i = 0; i < cpar[1]; i++) {
      cairo_line_to(local->cairo,
		    mxd2px(local, cpar[i * 2 + 2]),
		    mxd2py(local, cpar[i * 2 + 3]));
    }
    cairo_stroke(local->cairo);
    break;
  case 'D': 
    cairo_new_path(local->cairo);

    if (cpar[1] == 0) 
      break;

    for (i = 0; i < cpar[1]; i++) {
      cairo_line_to(local->cairo,
		    mxd2px(local, cpar[i * 2 + 3]),
		    mxd2py(local, cpar[i * 2 + 4]));
    }

    if (cpar[2] == 1) {
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_EVEN_ODD);
    } else {
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_WINDING);
    }
    cairo_close_path(local->cairo);
    if (cpar[2]) {
      cairo_fill(local->cairo);
    } else {
      cairo_stroke(local->cairo);
    }
    break;
  case 'F':
    memfree(local->fontalias);
    local->fontalias = nstrdup(cstr);
    break;
  case 'H':
    break;
  case 'S':
    break;
  case 'K':
    break;
  default:
    break;
  }
  return 0;
}

static struct objtable gra2cairo[] = {
  {"init", NVFUNC, NEXEC, gra2cairo_init, NULL, 0}, 
  {"done", NVFUNC, NEXEC, gra2cairo_done, NULL, 0}, 
  {"next", NPOINTER, 0, NULL, NULL, 0}, 
  {"dpi", NINT, NREAD | NWRITE, NULL, NULL, 0},
  {"region", NVFUNC, NEXEC, gra2cairo_set_region, NULL, 0}, 
  {"cairo", NVFUNC, 0, gra2cairo_cairo, NULL, 0}, 
  {"_local", NPOINTER, 0, NULL, NULL, 0}, 
  {"_output", NVFUNC, 0, gra2cairo_output, NULL, 0}, 
};

#define TBLNUM (sizeof(gra2cairo) / sizeof(*gra2cairo))

void *
addgra2cairo()
/* addgra2cairoile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairo, ERRNUM, gra2cairo_errorlist, NULL, NULL);
}
