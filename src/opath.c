/* 
 * $Id: oline.c,v 1.13 2010-03-04 08:30:16 hito Exp $
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

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "spline.h"
#include "mathfn.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"
#include "opath.h"

#define NAME		"path"
#define ALIAS		"line:curve:polygon"
#define PARENT		"legend"
#define OVERSION	"1.00.01"

#define ERRSPL  100
static char *patherrorlist[]={
  "error: spline interpolation."
};

#define ERRNUM (sizeof(patherrorlist) / sizeof(*patherrorlist))

static char *path_fill_mode[]={
  "false",
  "true",
  "\0empty",                   /* for backward compatibility */
  "\0even_odd_rule",           /* for backward compatibility */
  "\0winding_rule",            /* for backward compatibility */
  NULL,
};

enum PATH_FUILL_MODE {
  PATH_FILL_MODE_TRUE,
  PATH_FILL_MODE_FALSE,
  PATH_FILL_MODE_EMPTY,
  PATH_FILL_MODE_EVEN_ODD,
  PATH_FILL_MODE_WINDING,
};

static char *path_fill_rule[]={
  N_("even_odd_rule"),
  N_("winding_rule"),
  NULL,
};

enum PATH_FUILL_RULE {
  PATH_FILL_RULE_EVEN_ODD,
  PATH_FILL_RULE_WINDING,
};

static char *path_type[]={
  N_("line"),
  N_("curve"),
  NULL,
};

static int 
arrowinit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  int width, headlen, headwidth, miter, stroke, join, prm, type, alpha;
  struct narray *expand_points;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  switch (argv[0][0]) {
  case 'l':			/* line */
    type = PATH_TYPE_LINE;
    break;
  case 'c':			/* curve */
    type = PATH_TYPE_CURVE;
    break;
  case 'p':			/* polygon or path */
    type = PATH_TYPE_LINE;

    prm = 1;
    if (strcmp(argv[0], "polygon") == 0) {
      _putobj(obj, "close_path", inst, &prm);
    }

    break;
  default:
    type = PATH_TYPE_LINE;
  }

  width = 40;
  headlen = 72426;
  headwidth = 60000;
  miter = 1000;
  join = JOIN_TYPE_BEVEL;
  stroke = 1;
  alpha = 255;

  if (_putobj(obj, "type",         inst, &type))      return 1;
  if (_putobj(obj, "stroke",       inst, &stroke))    return 1;
  if (_putobj(obj, "width",        inst, &width))     return 1;
  if (_putobj(obj, "miter_limit",  inst, &miter))     return 1;
  if (_putobj(obj, "arrow_length", inst, &headlen))   return 1;
  if (_putobj(obj, "arrow_width",  inst, &headwidth)) return 1;
  if (_putobj(obj, "join",         inst, &join))      return 1;
  if (_putobj(obj, "stroke_A",     inst, &alpha)) return 1;
  if (_putobj(obj, "fill_A",       inst, &alpha)) return 1;

  expand_points = arraynew(sizeof(int));
  if (expand_points == NULL) {
    return 1;
  }

  if (_putobj(obj, "_points", inst, expand_points)) {
    arrayfree(expand_points);
    return 1;
  }

  return 0;
}

static int 
arrowdone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

#define ARROW_SIZE_MIN 10000
#define ARROW_SIZE_MAX 200000

static int 
arrowput(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *field;
  int *val_ptr;

  field = argv[1];
  val_ptr = (int *) argv[2];

  if (strcmp(field, "width") == 0) {
    if (*val_ptr < 1) {
      *val_ptr = 1;
    }
  } else if (strcmp(field, "arrow_length") == 0) {
    if (*val_ptr < ARROW_SIZE_MIN) {
      *val_ptr = ARROW_SIZE_MIN;
    } else if (*val_ptr > ARROW_SIZE_MAX) {
      *val_ptr = ARROW_SIZE_MAX;
    }
  } else if (strcmp(field, "arrow_width") == 0) {
    if (*val_ptr < ARROW_SIZE_MIN) {
      *val_ptr = ARROW_SIZE_MIN;
    } else if (*val_ptr > ARROW_SIZE_MAX) {
      *val_ptr = ARROW_SIZE_MAX;
    }
  }

  if (clear_bbox(obj, inst)) {
    return 1;
  }

  return 0;
}

static int
curve_expand(double c[], double x0, double y0, diffunc gdiff, intpfunc gintpf, struct narray *expand_points)
{
  double d, dx, dy, ddx, ddy, dd, x, y;
  int gx2, gy2;

  d = 0;
  while (d < 1) {
    gdiff(d, c, &dx, &dy, &ddx, &ddy, NULL);
    if (fabs(dx) + fabs(ddx) / 3 <= 1E-100) {
      dx = 1;
    } else {
      dx = sqrt(fabs(2 / (fabs(dx) + fabs(ddx) / 3)));
    }

    if (fabs(dy) + fabs(ddy) / 3 <= 1E-100) {
      dy = 1;
    } else {
      dy = sqrt(fabs(2 / (fabs(dy) + fabs(ddy) / 3)));
    }

    dd = (dx < dy) ? dx : dy;
    d += dd;

    if (d > 1) {
      d = 1;
    }

    gintpf(d, c, x0, y0, &x, &y, NULL);

    gx2 = nround(x);
    gy2 = nround(y);

    arrayadd(expand_points, &gx2);
    arrayadd(expand_points, &gy2);
  }

  return TRUE;
}

static int
curve_expand_points(struct objlist *obj, char *inst, int intp, struct narray *expand_points)
{
  int i, j, num, bsize, spcond, x, y;
  struct narray *points;
  double c[8];
  double *buf;
  double bs1[7], bs2[7], bs3[4], bs4[4];
  int *pdata;

  _getobj(obj, "points", inst, &points);

  num = arraynum(points)/2;
  pdata = arraydata(points);

  arrayclear(expand_points);

  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    if (num < 2)
      return 0;

    bsize = num + 1;
    buf = g_malloc(sizeof(double) * 9 * bsize);
    if (buf == NULL)
      return 1;

    for (i = 0; i < bsize; i++)
      buf[i]=i;

    for (i = 0; i < num; i++) {
      buf[bsize + i] = pdata[i * 2];
      buf[bsize * 2 + i] = pdata[i * 2 + 1];
    }

    if (intp == INTERPOLATION_TYPE_SPLINE) {
      spcond = SPLCND2NDDIF;
    } else {
      spcond = SPLCNDPERIODIC;
      if (buf[num - 1 + bsize] != buf[bsize] || buf[num - 1 + 2 * bsize] != buf[2 * bsize]) {
        buf[num + bsize] = buf[bsize];
        buf[num + 2 * bsize] = buf[2 * bsize];
        num++;
      }
    }

    if (spline(buf,
	       buf + bsize,
	       buf + 3 * bsize,
	       buf + 4 * bsize,
	       buf + 5 * bsize,
	       num, spcond, spcond, 0, 0)) {
      g_free(buf);
      error(obj, ERRSPL);
      return 1;
    }

    if (spline(buf,
	       buf + 2 * bsize,
	       buf + 6 * bsize,
	       buf + 7 * bsize,
	       buf + 8 * bsize,
	       num, spcond, spcond, 0, 0)) {
      g_free(buf);
      error(obj, ERRSPL);
      return 1;
    }

    x = nround(buf[bsize]);
    y = nround(buf[bsize * 2]);
    arrayadd(expand_points, &x);
    arrayadd(expand_points, &y);
    for (i = 0; i < num - 1; i++) {
      for (j = 0; j < 6; j++) {
	c[j] = buf[i + (j + 3) * bsize];
      }

      if (! curve_expand(c, buf[i + bsize], buf[i + 2 * bsize], splinedif, splineint, expand_points)) {
        break;
      }
    }
    g_free(buf);
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    if (num < 7) {
      return 0;
    }

    for (i = 0; i < 7; i++) {
      bs1[i] = pdata[i * 2];
      bs2[i] = pdata[i * 2 + 1];
    }

    for (j = 0; j < 2; j++) {
      bspline(j + 1, bs1 + j, c);
      bspline(j + 1, bs2 + j, c + 4);

      if (j == 0) {
	x = nround(c[0]);
	y = nround(c[4]);
	arrayadd(expand_points, &x);
	arrayadd(expand_points, &y);
      }

      if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
	return 0;
      }
    }

    for (; i < num; i++) {
      for (j = 1; j < 7; j++) {
        bs1[j - 1] = bs1[j];
        bs2[j - 1] = bs2[j];
      }
      bs1[6] = pdata[i*2];
      bs2[6] = pdata[i*2+1];
      bspline(0, bs1 + 1, c);
      bspline(0, bs2 + 1, c + 4);
      if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
	return 0;
      }
    }      

    for (j = 0; j < 2; j++) {
      bspline(j + 3, bs1 + j + 2, c);
      bspline(j + 3, bs2 + j + 2, c + 4);
      if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
	return 0;
      }
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    if (num < 4) {
      return 0;
    }

    for (i = 0; i < 4; i++) {
      bs1[i] = pdata[i * 2];
      bs3[i] = pdata[i * 2];
      bs2[i] = pdata[i * 2 + 1];
      bs4[i] = pdata[i * 2 + 1];
    }

    bspline(0, bs1, c);
    bspline(0, bs2, c + 4);
    x = nround(c[0]);
    y = nround(c[4]);
    arrayadd(expand_points, &x);
    arrayadd(expand_points, &y);

    if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
      return 0;
    }

    for (; i < num; i++) {
      for (j = 1; j < 4; j++) {
        bs1[j - 1] = bs1[j];
        bs2[j - 1] = bs2[j];
      }

      bs1[3] = pdata[i * 2];
      bs2[3] = pdata[i * 2 + 1];
      bspline(0, bs1, c);
      bspline(0, bs2, c + 4);
      if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
	return 0;
      }
    }
    for (j = 0; j < 3; j++) {
      bs1[4 + j] = bs3[j];
      bs2[4 + j] = bs4[j];
      bspline(0, bs1 + j + 1, c);
      bspline(0, bs2 + j + 1, c + 4);
      if (! curve_expand(c, c[0], c[4], bsplinedif, bsplineint, expand_points)) {
	return 0;
      }
    }
    break;
  }

  return 0;
}

static void 
curve_clear(struct objlist *obj,char *inst)
{
  struct narray *expand_points;

  _getobj(obj, "_points", inst, &expand_points);
  arrayclear(expand_points);
}

static double
distance(double x1, double y1)
{
  return sqrt(x1 * x1 + y1 * y1);
}

static void
get_arrow_pos(int *points2, int n,
	      int width, int headlen, int headwidth,
	      int x0, int y0, int x1, int y1, int *ap)
{
  int ax0, ay0, ox, oy;
  double alen,  alen2, awidth, len, dx, dy;

  alen = width * (double) headlen / 10000;
  awidth = width * (double) headwidth / 20000;

  dx = x0 - x1;
  dy = y0 - y1;
  len = distance(dx, dy);
  dx /= len;
  dy /= len;
  ax0 = nround(x0 - dx * alen);
  ay0 = nround(y0 - dy * alen);
  alen2 = alen * width / awidth / 2;

  if (len >= alen2) {
    ox = oy = 0;
    if (points2) {
      points2[n]     = nround(x0 - dx * alen2);
      points2[n + 1] = nround(y0 - dy * alen2);
    }
  } else {
    ox = nround(dx * alen2);
    oy = nround(dy * alen2);
  }
  ax0 += ox;
  ay0 += oy;

  ap[0] = nround(ax0 - dy * awidth);
  ap[1] = nround(ay0 + dx * awidth);
  ap[2] = x0 + nround(ox);
  ap[3] = y0 + nround(oy);
  ap[4] = nround(ax0 + dy * awidth);
  ap[5] = nround(ay0 - dx * awidth);
}

static void
draw_stroke(struct objlist *obj, char *inst, int GC, int *points2, int *pdata, int num, int intp)
{
  int width, fr, fg, fb, fa, headlen, headwidth;
  int join, miter, head;
  int x, y, x0, y0, x1, y1, x2, y2, x3, y3, close_path;
  struct narray *style;
  int snum, *sdata;
  int i;
  int ap[6], ap2[6];

  _getobj(obj, "stroke_R", inst, &fr);
  _getobj(obj, "stroke_G", inst, &fg);
  _getobj(obj, "stroke_B", inst, &fb);
  _getobj(obj, "stroke_A", inst, &fa);

  _getobj(obj, "close_path",   inst, &close_path);
  _getobj(obj, "width",        inst, &width);
  _getobj(obj, "style",        inst, &style);
  _getobj(obj, "join",         inst, &join);
  _getobj(obj, "miter_limit",  inst, &miter);
  _getobj(obj, "arrow",        inst, &head);
  _getobj(obj, "arrow_length", inst, &headlen);
  _getobj(obj, "arrow_width",  inst, &headwidth);

  snum = arraynum(style);
  sdata = arraydata(style);

  GRAcolor(GC, fr, fg, fb, fa);
  GRAlinestyle(GC, snum, sdata, width, 0, join, miter);

  x0 = points2[0];
  y0 = points2[1];
  x1 = points2[2];
  y1 = points2[3];
  x2 = points2[2 * num - 4];
  y2 = points2[2 * num - 3];
  x3 = points2[2 * num - 2];
  y3 = points2[2 * num - 1];

  if (head == ARROW_POSITION_BEGIN || head == ARROW_POSITION_BOTH) {
    get_arrow_pos(points2, 0,
		  width, headlen, headwidth,
		  x0, y0, x1, y1, ap);
  }

  if (head == ARROW_POSITION_END || head == ARROW_POSITION_BOTH) {
    get_arrow_pos(points2, num * 2 - 2,
		  width, headlen, headwidth,
		  x3, y3, x2, y2, ap2);
  }

  if (num > 2 && 
      (close_path ||
       intp == INTERPOLATION_TYPE_SPLINE_CLOSE ||
       intp == INTERPOLATION_TYPE_BSPLINE_CLOSE)) {
    GRAdrawpoly(GC, num, pdata, 0);
  } else {
    x = points2[0];
    y = points2[1];

    GRAmoveto(GC, x, y);
    for (i = 1; i < num; i++) {
      x = points2[i * 2];
      y = points2[i * 2 + 1];
      GRAlineto(GC, x, y);
    }
  }

  if (head == ARROW_POSITION_BEGIN || head == ARROW_POSITION_BOTH) {
    GRAlinestyle(GC, 0, NULL, 1, 0, join, miter);
    GRAdrawpoly(GC, 3, ap, 1);
  }

  if (head == ARROW_POSITION_END || head == ARROW_POSITION_BOTH) {
    GRAlinestyle(GC, 0, NULL, 1, 0, join, miter);
    GRAdrawpoly(GC, 3, ap2, 1);
  }
}

static void
draw_fill(struct objlist *obj, char *inst, int GC, int *points2, int num)
{
  int br, bg, bb, ba, fill_rule;

  _getobj(obj, "fill_rule", inst, &fill_rule);
  _getobj(obj, "fill_R",    inst, &br);
  _getobj(obj, "fill_G",    inst, &bg);
  _getobj(obj, "fill_B",    inst, &bb);
  _getobj(obj, "fill_A",    inst, &ba);

  GRAcolor(GC, br, bg, bb, ba);
  GRAdrawpoly(GC, num, points2, fill_rule + 1);
}

static int 
arrowdraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int GC, lm, tm, w, h, intp, i, j, num;
  struct narray *points;
  int *points2, *pdata;
  int x0, y0, x1, y1, type, stroke, fill, clip, zoom;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  _getobj(obj, "GC", inst, &GC);
  if (GC < 0) {
    return 0;
  }

  _getobj(obj, "stroke", inst, &stroke);
  _getobj(obj, "fill",   inst, &fill);
  if (fill == 0 && stroke == 0) {
    return 0;
  }

  _getobj(obj, "type",  inst, &type);
  _getobj(obj, "clip",  inst, &clip);

  if (type == PATH_TYPE_CURVE) {
    _getobj(obj, "interpolation", inst, &intp);
    _getobj(obj, "_points",       inst, &points);
    if (arraynum(points) == 0) {
      curve_expand_points(obj, inst, intp, points);
    }
  } else {
    _getobj(obj, "points", inst, &points);
  }

  num = arraynum(points) / 2;
  pdata = arraydata(points);

  points2 = g_malloc(sizeof(int) * num * 2);
  if (points2 == NULL) {
    return 1;
  }
  j = 0;
  x1 = y1 = 0;
  for (i = 0; i < num; i++) {
    x0 = pdata[2 * i];
    y0 = pdata[2 * i + 1];
    if (i == 0 || x0 != x1 || y0 != y1) {
      points2[2 * j] = x0;
      points2[2 * j + 1] = y0;
      j++;
      x1 = x0;
      y1 = y0;
    }
  }
  num = j;
  if (num < 2) {
    g_free(points2);
    return 0;
  }

  GRAregion(GC, &lm, &tm, &w, &h, &zoom);
  GRAview(GC, 0, 0, w * 10000.0 / zoom, h * 10000.0 / zoom, clip);

  if (fill) {
    draw_fill(obj, inst, GC, points2, num);
  }

  if (stroke) {
    draw_stroke(obj, inst, GC, points2, pdata, num, intp);
  }

  g_free(points2);
  GRAaddlist(GC, obj, inst, (char *)argv[0], (char *)argv[1]);
  return 0;
}

static int 
arrowbbox(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int minx, miny, maxx, maxy;
  int x, y, num, num2, type, intp, stroke, fill;
  struct narray *points;
  int *pdata;
  struct narray *array;
  int i, j, width;
  int headlen, headwidth;
  int head;
  int *points2;
  int x0, y0, x1, y1, x2, y2, x3, y3;
  int ap[6], ap2[6];

  array = * (struct narray **) rval;
  if (arraynum(array) != 0) {
    return 0;
  }

  _getobj(obj, "type", inst, &type);

  _getobj(obj, "fill",   inst, &fill);
  _getobj(obj, "stroke", inst, &stroke);
  if (fill == 0 && stroke == 0) {
    return 0;
  }

  if (type == PATH_TYPE_CURVE) {
    _getobj(obj, "interpolation", inst, &intp);
    _getobj(obj, "_points",       inst, &points);
    if (arraynum(points) == 0) {
      curve_expand_points(obj, inst, intp, points);
    }
  } else {
    _getobj(obj, "points", inst, &points);
  }

  _getobj(obj, "width",        inst, &width);
  _getobj(obj, "arrow",        inst, &head);
  _getobj(obj, "arrow_length", inst, &headlen);
  _getobj(obj, "arrow_width",  inst, &headwidth);

  num = arraynum(points) / 2;
  pdata = arraydata(points);
  points2 = g_malloc(sizeof(int) * num * 2);
  if (points2 == NULL) {
    return 1;
  }
  j = 0;
  x1 = y1 = 0;
  for (i = 0; i < num; i++) {
    x0 = pdata[2 * i];
    y0 = pdata[2 * i + 1];
    if (i == 0 || x0 != x1 || y0 != y1) {
      points2[2 * j] = x0;
      points2[2 * j + 1] = y0;
      j++;
      x1 = x0;
      y1 = y0;
    }
  }
  num2 = j;
  if (num2 < 2) {
    g_free(points2);
    return 0;
  }

  x0 = points2[0];
  y0 = points2[1];
  x1 = points2[2];
  y1 = points2[3];
  x2 = points2[2 * num2 - 4];
  y2 = points2[2 * num2 - 3];
  x3 = points2[2 * num2 - 2];
  y3 = points2[2 * num2 - 1];
  g_free(points2);

  if (head==ARROW_POSITION_BEGIN || head==ARROW_POSITION_BOTH) {
    get_arrow_pos(NULL, 0,
		  width, headlen, headwidth,
		  x0, y0, x1, y1, ap);
  }

  if (head==ARROW_POSITION_END || head==ARROW_POSITION_BOTH) {
    get_arrow_pos(NULL, 0,
		  width, headlen, headwidth,
		  x3, y3, x2, y2, ap2);
  }
  if (array == NULL && (array = arraynew(sizeof(int))) == NULL) {
    return 1;
  }

  maxx = minx = pdata[0];
  maxy = miny = pdata[1];
  arrayadd(array, &(pdata[0]));
  arrayadd(array, &(pdata[1]));

  for (i = 1; i < num; i++) {
    x = pdata[i * 2];
    y = pdata[i * 2 + 1];
    if (x < minx) minx = x;
    if (x > maxx) maxx = x;
    if (y < miny) miny = y;
    if (y > maxy) maxy = y;
  }

  if (type == PATH_TYPE_CURVE) {
    _getobj(obj, "points", inst, &points);
    num = arraynum(points) / 2;
    pdata = arraydata(points);
  }
  for (i = 1; i < num; i++) {
    x = pdata[i * 2];
    y = pdata[i * 2 + 1];
    arrayadd(array, &x);
    arrayadd(array, &y);
  }

  if (stroke) {
    if (head == ARROW_POSITION_BEGIN || head == ARROW_POSITION_BOTH) {
      for (i = 0; i < 3; i++) {
	if (ap[i * 2] < minx) minx = ap[i * 2];
	if (ap[i * 2] > maxx) maxx = ap[i * 2];
	if (ap[i * 2 + 1] < miny) miny = ap[i * 2 + 1];
	if (ap[i * 2 + 1] > maxy) maxy = ap[i * 2 + 1];
      }
    }
    if (head == ARROW_POSITION_END || head == ARROW_POSITION_BOTH) {
      for (i = 0; i < 3 ; i++) {
	if (ap2[i * 2] < minx) minx = ap2[i * 2];
	if (ap2[i * 2] > maxx) maxx = ap2[i * 2];
	if (ap2[i * 2 + 1] < miny) miny = ap2[i * 2+ 1];
	if (ap2[i * 2 + 1] > maxy) maxy = ap2[i * 2 + 1];
      }
    }

    minx -= width / 2;
    miny -= width / 2;
    maxx += width / 2;
    maxy += width / 2;
  }

  arrayins(array, &(maxy), 0);
  arrayins(array, &(maxx), 0);
  arrayins(array, &(miny), 0);
  arrayins(array, &(minx), 0);

  if (arraynum(array) == 0) {
    arrayfree(array);
    * (struct narray **) rval = NULL;
    return 1;
  }

  * (struct narray **) rval = array;

  return 0;
}

static int 
set_points(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  curve_clear(obj, inst);

  return legendgeometry(obj, inst, rval, argc, argv);
}

static int
check_point_match(int i, int j, int *pdata, int err, int x, int y)
{
  double x1, y1, x2, y2, r, ip;

  x1 = pdata[i * 2];
  y1 = pdata[i * 2 + 1];
  x2 = pdata[j * 2];
  y2 = pdata[j * 2 + 1];

  r = distance(x - x1, y - y1);
  if (r <= err) {
    return TRUE;
  }

  r = distance(x - x2, y - y2);
  if (r <= err) {
    return TRUE;
  }

  r = distance(x1 - x2, y1 - y2);
  if (r == 0) {
    return FALSE;
  }

  ip = ((x2 - x1) * (x - x1) + (y2 - y1) * (y - y1)) / r;
  if (ip < 0 || ip > r) {
    return FALSE;
  }

  x2 = x1 + (x2 - x1) * ip / r;
  y2 = y1 + (y2 - y1) * ip / r;
  r = distance(x - x2, y - y2);
  if (r <= err) {
    return TRUE;
  }

  return FALSE;
}

static int
point_match(struct objlist *obj, char *inst, int type, int fill, int err, int x, int y)
{
  struct narray *points;
  int *pdata, num, r, i;

  if (type == PATH_TYPE_CURVE) {
    int intp;

    _getobj(obj, "interpolation", inst, &intp);
    _getobj(obj, "_points", inst, &points);
    if (arraynum(points) == 0) {
      curve_expand_points(obj, inst, intp, points);
    }
  } else {
    _getobj(obj, "points", inst, &points);
  }

  num = arraynum(points) / 2;
  pdata = arraydata(points);
  if (num == 0 || pdata == NULL) {
    return FALSE;
  }

  r = FALSE;
  for (i = 0; i < num - 1; i++) {
    r = check_point_match(i, i + 1, pdata, err, x, y);
    if (r) {
      break;
    }
  }

  if (r == FALSE) {
    int close_path;

    _getobj(obj, "close_path", inst, &close_path);
    if (fill || close_path) {
      r = check_point_match(0, num - 1, pdata, err, x, y);
    }
  }

  return r;
}

static int 
curvematch(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int minx, miny, maxx, maxy, err;
  int bminx, bminy, bmaxx, bmaxy;
  int fill, stroke, type;
  struct narray *array;

  * (int *) rval = FALSE;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) {
    return 1;
  }

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "stroke", inst, &stroke);
  if (fill == 0 && stroke == 0) {
    return 0;
  }

  minx = * (int *) argv[2];
  miny = * (int *) argv[3];
  maxx = * (int *) argv[4];
  maxy = * (int *) argv[5];
  err  = * (int *) argv[6];

  if (minx == maxx && miny == maxy) {
    * (int *) rval = point_match(obj, inst, type, fill, err, minx, miny);
  } else {
    if (_exeobj(obj, "bbox", inst, 0, NULL)) {
      return 1;
    }
    _getobj(obj, "bbox", inst, &array);

    if (array == NULL) {
      return 0;
    }

    if (arraynum(array) < 4) {
      return 1;
    }

    bminx = * (int *) arraynget(array, 0);
    bminy = * (int *) arraynget(array, 1);
    bmaxx = * (int *) arraynget(array, 2);
    bmaxy = * (int *) arraynget(array, 3);
    if (minx <= bminx && bminx <= maxx && 
	minx <= bmaxx && bmaxx <= maxx && 
	miny <= bminy && bminy <= maxy && 
	miny <= bmaxy && bmaxy <= maxy) {
      * (int *) rval = TRUE;
    }
  }
  return 0;
}

static int 
curve_flip(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  curve_clear(obj, inst);

  return legendflip(obj, inst, rval, argc, argv);
}

static int 
curve_move(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct narray *points;
  int i, num, *pdata, type;

  _getobj(obj, "type", inst, &type);
  if (type  == PATH_TYPE_CURVE) {
    _getobj(obj, "_points", inst, &points);
    num = arraynum(points);
    pdata = arraydata(points);
    for (i = 0; i < num; i++) {
      if (i % 2 == 0) {
	pdata[i] += * (int *) argv[2];
      } else {
	pdata[i] += * (int *) argv[3];
      }
    }
  }

  return legendmove(obj, inst, rval, argc, argv);
}

static int 
curve_rotate(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  curve_clear(obj, inst);

  return legendrotate(obj, inst, rval, argc, argv);
}

static int 
curve_zoom(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  curve_clear(obj, inst);

  return legendzoom(obj, inst, rval, argc, argv);
}

static int 
curve_change(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  curve_clear(obj, inst);

  return legendchange(obj, inst, rval, argc, argv);
}

static int 
put_fill_mode(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int mode, rule;

  mode = * (int *) argv[2];
  switch (mode) {
  case PATH_FILL_MODE_TRUE:
    break;
  case PATH_FILL_MODE_FALSE:
    break;
  case PATH_FILL_MODE_EMPTY:
    * (int *) argv[2] = FALSE;
    break;
  case PATH_FILL_MODE_EVEN_ODD:
    * (int *) argv[2] = TRUE;
    rule = PATH_FILL_RULE_EVEN_ODD;
    _putobj(obj, "fill_rule", inst, &rule);
    break;
  case PATH_FILL_MODE_WINDING:
    * (int *) argv[2] = TRUE;
    rule = PATH_FILL_RULE_WINDING;
    _putobj(obj, "fill_rule", inst, &rule);
    break;
  default:
    * (int *) argv[2] = FALSE;
  }

  return legendgeometry(obj, inst, rval, argc, argv);
}

static struct objtable arrow[] = {
  {"init", NVFUNC, NEXEC, arrowinit, NULL, 0},
  {"done", NVFUNC, NEXEC, arrowdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},

  {"type", NENUM, NREAD|NWRITE, set_points, path_type, 0},

  {"points", NIARRAY, NREAD|NWRITE, set_points, NULL, 0},
  {"_points", NIARRAY, NREAD, NULL, NULL, 0},

  {"interpolation", NENUM, NREAD|NWRITE, set_points, intpchar, 0},

  {"fill_R", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"fill_G", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"fill_B", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"fill_A", NINT, NREAD|NWRITE, oputcolor, NULL, 0},

  {"stroke_R", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"stroke_G", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"stroke_B", NINT, NREAD|NWRITE, oputcolor, NULL, 0},
  {"stroke_A", NINT, NREAD|NWRITE, oputcolor, NULL, 0},

  {"fill", NENUM, NREAD|NWRITE, put_fill_mode, path_fill_mode, 0},
  {"fill_rule", NENUM, NREAD|NWRITE, NULL, path_fill_rule, 0},
  {"stroke", NBOOL, NREAD|NWRITE, NULL, NULL, 0},
  {"close_path", NBOOL, NREAD|NWRITE, NULL, NULL, 0},
  {"width", NINT, NREAD|NWRITE, arrowput, NULL, 0},
  {"style", NIARRAY, NREAD|NWRITE, oputstyle, NULL, 0},
  {"join", NENUM, NREAD|NWRITE, NULL, joinchar, 0},
  {"miter_limit", NINT, NREAD|NWRITE, oputge1, NULL, 0},

  {"arrow", NENUM, NREAD|NWRITE, arrowput, arrowchar, 0},
  {"arrow_length", NINT, NREAD|NWRITE, arrowput, NULL, 0},
  {"arrow_width", NINT, NREAD|NWRITE, arrowput, NULL, 0},

  {"draw", NVFUNC, NREAD|NEXEC, arrowdraw, "i", 0},
  {"bbox", NIAFUNC, NREAD|NEXEC, arrowbbox, "", 0},
  {"move", NVFUNC, NREAD|NEXEC, curve_move, "ii", 0},
  {"rotate", NVFUNC, NREAD|NEXEC, curve_rotate, "iiii", 0},
  {"flip", NVFUNC, NREAD|NEXEC, curve_flip, "iii", 0},
  {"change", NVFUNC, NREAD|NEXEC, curve_change, "iii", 0},
  {"zooming", NVFUNC, NREAD|NEXEC, curve_zoom, "iiii", 0},
  {"match", NBFUNC, NREAD|NEXEC, curvematch, "iiiii", 0},


  /* following fields exist for backward compatibility */
  {"R", NINT, NWRITE, put_color_for_backward_compatibility, NULL, 0},
  {"G", NINT, NWRITE, put_color_for_backward_compatibility, NULL, 0},
  {"B", NINT, NWRITE, put_color_for_backward_compatibility, NULL, 0},
  {"A", NINT, NWRITE, put_color_for_backward_compatibility, NULL, 0},
};

#define TBLNUM (sizeof(arrow) / sizeof(*arrow))

void *
addpath(void)
/* addarrow() returns NULL on error */
{
  return addobject(NAME, ALIAS, PARENT, OVERSION, TBLNUM, arrow, ERRNUM, patherrorlist, NULL, NULL);
}

