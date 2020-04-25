/*
 * $Id: odraw.c,v 1.14 2009-11-16 09:13:04 hito Exp $
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "object.h"
#include "ioutil.h"
#include "spline.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "nstring.h"
#include "mathfn.h"

#define NAME "draw"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILGC 100
#define ERRGCOPEN 101


static char *drawerrorlist[]={
  "illegal graphics context",
  "graphics context is not opened",
};

#define ERRNUM (sizeof(drawerrorlist) / sizeof(*drawerrorlist))

char *pathchar[]={
  N_("unchange"),
  N_("full"),
  N_("relative"),
  N_("base"),
  NULL,
};

char *joinchar[]={
  N_("miter"),
  N_("round"),
  N_("bevel"),
  NULL
};

char *fontchar[]={
  "Sans-serif",
  "Serif",
  "Monospace",
   NULL
};

char *arrowchar[]={
  N_("none"),
  N_("end"),
  N_("begin"),
  N_("both"),
  NULL
};

char *marker_type_char[]={
  N_("none"),
  N_("arrow"),
  N_("wave"),
  N_("mark"),
  N_("bar"),
  NULL
};

char *intpchar[]={
  N_("spline"),
  N_("spline_close"),
  N_("bspline"),
  N_("bspline_close"),
  NULL
};

static int
drawinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int clip,redrawf,alpha;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  clip=TRUE;
  redrawf=TRUE;
  alpha=255;
  if (_putobj(obj,"clip",inst,&clip)) return 1;
  if (_putobj(obj,"redraw_flag",inst,&redrawf)) return 1;
  if (_putobj(obj,"A",inst,&alpha)) return 1;
  return 0;
}

static int
drawdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}


static int
drawdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC,hidden;

  GC=*(int *)(argv[2]);
  if (GRAopened(GC)<0) {
    error3(obj,ERRGCOPEN,GC);
    return 1;
  }
  _getobj(obj,"hidden",inst,&hidden);
  if (hidden) GC=-1;
  if (_putobj(obj,"GC",inst,&GC)) return 1;
  return 0;
}

int
pathsave(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array,*array2;
  int anum;
  char **adata;
  int i,j;
  char *argv2[4];
  char *file,*name,*valstr;
  int path;
  GString *s;

  array=(struct narray *)argv[2];
  anum=arraynum(array);
  adata=arraydata(array);
  for (j=0;j<anum;j++)
    if (strcmp0("file",adata[j])==0) {
      if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
      return 0;
    }
  array2=arraynew(sizeof(char *));
  for (i=0;i<anum;i++) arrayadd(array2,&(adata[i]));
  file="file";
  arrayadd(array2,&file);
  argv2[0]=argv[0];
  argv2[1]=argv[1];
  argv2[2]=(char *)array2;
  argv2[3]=NULL;
  if (_exeparent(obj,(char *)argv[1],inst,rval,3,argv2)) {
    arrayfree(array2);
    return 1;
  }
  arrayfree(array2);
  name=NULL;
  if (_getobj(obj,"save_path",inst,&path)) goto errexit;
  if (_getobj(obj,"file",inst,&file)) goto errexit;
  if (file!=NULL) {
    if (path==1) name=getfullpath(file);
    else if (path==2) name=getrelativepath(file);
    else if (path==3) name=getbasename(file);
    else if (path==0) {
      if ((name=g_malloc(strlen(file)+1))==NULL) goto errexit;
      strcpy(name,file);
    }
  }

  s = g_string_sized_new(256);
  if (s == NULL) {
    goto errexit;
  }
  g_string_append(s, rval->str);
  g_string_append_c(s, '\t');
  g_string_append(s, argv[0]);
  g_string_append(s, "::file=");

  valstr = getvaluestr(obj, "file", &name, FALSE, TRUE);
  if (valstr == NULL) {
    g_free(s);
    goto errexit;
  }
  g_string_append(s, valstr);
  g_free(valstr);
  g_string_append_c(s, '\n');
  g_free(name);
  g_free(rval->str);
  rval->str = g_string_free(s, FALSE);
  return 0;

errexit:
  g_free(name);
  g_free(rval->str);
  rval->str=NULL;
  return 1;
}

int
clear_bbox(struct objlist *obj, N_VALUE *inst)
{
  struct narray *array;

  if (inst == NULL)
    return 1;

  if (_getobj(obj, "bbox", inst, &array))
    return 1;

  arrayfree(array);

  if (_putobj(obj, "bbox", inst, NULL))
    return 1;

  return 0;
}

int
put_hsb_color(struct objlist *obj, N_VALUE *inst, int argc, char **argv, char *format)
{
  double h, s, b;
  int rr, gg, bb;
  char buf[64];

  h =  arg_to_double(argv, 2);
  s =  arg_to_double(argv, 3);
  b =  arg_to_double(argv, 4);

  if (h < 0) {
    h = 0;
  } else if (h > 1) {
    h = 1;
  }

  if (s < 0) {
    s = 0;
  } else if (s > 1) {
    s = 1;
  }

  if (b < 0) {
    b = 0;
  } else if (b > 1) {
    b = 1;
  }

  HSB2RGB(h, s, b, &rr, &gg, &bb);

  snprintf(buf, sizeof(buf), format, 'R');
  _putobj(obj, buf, inst, &rr);

  snprintf(buf, sizeof(buf), format, 'G');
  _putobj(obj, buf, inst, &gg);

  snprintf(buf, sizeof(buf), format, 'B');
  _putobj(obj, buf, inst, &bb);

  return 0;
}

int
put_fill_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "fill_%c");
}

int
put_stroke_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "stroke_%c");
}

int
put_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "%c");
}

int
put_hsb2(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "%c2");
}

static int
curve_expand(double c[], double x0, double y0, diffunc gdiff, intpfunc gintpf, struct narray *expand_points)
{
  double d, dx, dy, ddx, ddy, x, y;
  int gx2, gy2;

  d = 0;
  while (d < 1) {
    double dd;
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

int
curve_expand_points(int *pdata, int num, int intp, struct narray *expand_points)
{
  int i, j, bsize, spcond, x, y;
  double c[8];
  double *buf;
  double bs1[7], bs2[7], bs3[4], bs4[4];

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
      return 1;
    }

    if (spline(buf,
	       buf + 2 * bsize,
	       buf + 6 * bsize,
	       buf + 7 * bsize,
	       buf + 8 * bsize,
	       num, spcond, spcond, 0, 0)) {
      g_free(buf);
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

void
text_get_bbox(int x, int y, char *text, char *font, int style, int pt, int dir, int space, int scriptsize, int raw, int *bbox)
{
  int gx0, gy0, gx1, gy1;
  int i, ggx[4], ggy[4];
  int minx, miny, maxx, maxy;
  double si, co, rdir;

  if (raw) {
    GRAtextextentraw(text, font, style, pt, space, &gx0, &gy0, &gx1, &gy1);
  } else {
    GRAtextextent(text, font, style, pt, space, scriptsize, &gx0, &gy0, &gx1, &gy1, FALSE);
  }
  rdir = dir / 18000.0 * MPI;
  si = -sin(rdir);
  co =  cos(rdir);
  ggx[0] = x + gx0 * co - gy0 * si;
  ggy[0] = y + gx0 * si + gy0 * co;
  ggx[1] = x + gx1 * co - gy0 * si;
  ggy[1] = y + gx1 * si + gy0 * co;
  ggx[2] = x + gx0 * co - gy1 * si;
  ggy[2] = y + gx0 * si + gy1 * co;
  ggx[3] = x + gx1 * co - gy1 * si;
  ggy[3] = y + gx1 * si + gy1 * co;
  minx = ggx[0];
  maxx = ggx[0];
  miny = ggy[0];
  maxy = ggy[0];
  for (i = 1; i < 4; i++) {
    if (ggx[i]<minx) minx = ggx[i];
    if (ggx[i]>maxx) maxx = ggx[i];
    if (ggy[i]<miny) miny = ggy[i];
    if (ggy[i]>maxy) maxy = ggy[i];
  }
  bbox[0] = minx;
  bbox[1] = miny;
  bbox[2] = maxx;
  bbox[3] = maxy;
}

static struct objtable draw[] = {
  {"init",NVFUNC,0,drawinit,NULL,0},
  {"done",NVFUNC,0,drawdone,NULL,0},
  {"GC",NINT,0,NULL,NULL,0},
  {"hidden",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"redraw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"A",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"clip",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"redraw_flag",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"redraw_num",NINT,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(draw) / sizeof(*draw))

void *
adddraw(void)
/* adddraw() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,draw,ERRNUM,drawerrorlist,NULL,NULL);
}
