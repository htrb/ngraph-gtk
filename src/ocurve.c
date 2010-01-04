/* 
 * $Id: ocurve.c,v 1.11 2010/01/04 05:11:28 hito Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "ngraph.h"
#include "mathfn.h"
#include "object.h"
#include "gra.h"
#include "spline.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"

#define NAME "curve"
#define PARENT "legend"
#define OVERSION  "1.00.00"

#define ERRSPL  100

static char *curveerrorlist[]={
  "error: spline interpolation."
};

#define ERRNUM (sizeof(curveerrorlist) / sizeof(*curveerrorlist))

static int 
curveinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  int width,miter;
#if CURVE_OBJ_USE_EXPAND_BUFFER
  struct narray *expand_points;
#endif

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=40;
  miter=1000;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"miter_limit",inst,&miter)) return 1;

#if CURVE_OBJ_USE_EXPAND_BUFFER
  expand_points = arraynew(sizeof(int));
  if (expand_points == NULL) {
    return 1;
  }
  if (_putobj(obj, "_points", inst, expand_points)) {
    arrayfree(expand_points);
    return 1;
  }
#endif

  return 0;
}

static int 
curvedone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

#if CURVE_OBJ_USE_EXPAND_BUFFER
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
#endif

static int 
curve_set_points(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  curve_clear(obj, inst);
#endif

  return legendgeometry(obj, inst, rval, argc, argv);
}

static int 
curvedraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  int GC;
  int width, join, miter, intp, fr, fg, fb, lm, tm, w, h;
  struct narray *style;
  int i, num, snum;
  struct narray *points;
  int *pdata, *sdata;
  int clip, zoom;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  _getobj(obj, "GC", inst, &GC);
  if (GC < 0) {
    return 0;
  }
  _getobj(obj, "R", inst, &fr);
  _getobj(obj, "G", inst, &fg);
  _getobj(obj, "B", inst, &fb);
  _getobj(obj, "_points", inst, &points);
  _getobj(obj, "width", inst, &width);
  _getobj(obj, "style", inst, &style);
  _getobj(obj, "join", inst, &join);
  _getobj(obj, "miter_limit", inst, &miter);
  _getobj(obj, "interpolation", inst, &intp);
  _getobj(obj, "clip", inst, &clip);

  if (arraynum(points) == 0) {
    curve_expand_points(obj, inst, intp, points);
  }

  num = arraynum(points) / 2;
  pdata = arraydata(points);

  if (num < 2) {
    return 0;
  }

  snum=arraynum(style);
  sdata=arraydata(style);

  GRAregion(GC, &lm, &tm, &w, &h, &zoom);
  GRAview(GC, 0, 0, w * 10000.0 / zoom, h * 10000.0 / zoom, clip);
  GRAcolor(GC, fr, fg, fb);
  GRAlinestyle(GC, snum, sdata, width, 0, join, miter);

  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE: 
  case INTERPOLATION_TYPE_BSPLINE:
    GRAmoveto(GC, pdata[0], pdata[1]);
    for (i = 1; i < num; i++) {
      GRAlineto(GC, pdata[i * 2], pdata[i * 2 + 1]);
    }
    break;
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    GRAdrawpoly(GC, num, pdata, 0);
    break;
  }
#else
  int GC;
  int width,join,miter,intp,fr,fg,fb,lm,tm,w,h;
  struct narray *style;
  int snum,*sdata;
  int i,j,num,bsize,spcond;
  struct narray *points;
  double c[8];
  double *buf;
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int *pdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"join",inst,&join);
  _getobj(obj,"miter_limit",inst,&miter);
  _getobj(obj,"interpolation",inst,&intp);
  _getobj(obj,"clip",inst,&clip);

  snum=arraynum(style);
  sdata=arraydata(style);
  num=arraynum(points)/2;
  pdata=arraydata(points);
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    if (num<2) return 0;
    bsize=num+1;
    if ((buf=g_malloc(sizeof(double)*9*bsize))==NULL) return 1;
    for (i=0;i<bsize;i++) buf[i]=i;
    for (i=0;i<num;i++) {
      buf[bsize+i]=pdata[i*2];
      buf[bsize*2+i]=pdata[i*2+1];
    }
    if (intp==INTERPOLATION_TYPE_SPLINE) {
      spcond=SPLCND2NDDIF;
    } else {
      spcond=SPLCNDPERIODIC;
      if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
        buf[num+bsize]=buf[bsize];
        buf[num+2*bsize]=buf[2*bsize];
        num++;
      }
    }
    if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
               spcond,spcond,0,0)) {
      g_free(buf);
      error(obj,ERRSPL);
      return 1;
    }
    if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
               spcond,spcond,0,0)) {
      g_free(buf);
      error(obj,ERRSPL);
      return 1;
    }
    GRAregion(GC,&lm,&tm,&w,&h,&zoom);
    GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
    GRAcolor(GC,fr,fg,fb);
    GRAlinestyle(GC,0,NULL,width,0,join,miter);
    GRAcurvefirst(GC,snum,sdata,NULL,NULL,
                  splinedif,splineint,NULL,buf[bsize],buf[2*bsize]);
    for (i=0;i<num-1;i++) {
      for (j=0;j<6;j++) c[j]=buf[i+(j+3)*bsize];
      if (!GRAcurve(GC,c,buf[i+bsize],buf[i+2*bsize]))
        break;
    }
    g_free(buf);
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    if (num<7) return 0;
    for (i=0;i<7;i++) {
      bs1[i]=pdata[i*2];
      bs2[i]=pdata[i*2+1];
    }
    GRAregion(GC,&lm,&tm,&w,&h,&zoom);
    GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,1);
    GRAcolor(GC,fr,fg,fb);
    GRAlinestyle(GC,0,NULL,width,0,join,miter);
    for (j=0;j<2;j++) {
      bspline(j+1,bs1+j,c);
      bspline(j+1,bs2+j,c+4);
      if (j==0) GRAcurvefirst(GC,snum,sdata,NULL,NULL,
                              bsplinedif,bsplineint,NULL,c[0],c[4]);
      if (!GRAcurve(GC,c,c[0],c[4])) return 0;
    }
    for (;i<num;i++) {
      for (j=1;j<7;j++) {
        bs1[j-1]=bs1[j];
        bs2[j-1]=bs2[j];
      }
      bs1[6]=pdata[i*2];
      bs2[6]=pdata[i*2+1];
      bspline(0,bs1+1,c);
      bspline(0,bs2+1,c+4);
      if (!GRAcurve(GC,c,c[0],c[4])) 
        return 0;
    }      
    for (j=0;j<2;j++) {
      bspline(j+3,bs1+j+2,c);
      bspline(j+3,bs2+j+2,c+4);
      if (!GRAcurve(GC,c,c[0],c[4])) 
        return 0;
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    if (num<4) return 0;
    for (i=0;i<4;i++) {
      bs1[i]=pdata[i*2];
      bs3[i]=pdata[i*2];
      bs2[i]=pdata[i*2+1];
      bs4[i]=pdata[i*2+1];
    }
    GRAregion(GC,&lm,&tm,&w,&h,&zoom);
    GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
    GRAcolor(GC,fr,fg,fb);
    GRAlinestyle(GC,0,NULL,width,0,join,miter);
    bspline(0,bs1,c);
    bspline(0,bs2,c+4);
    GRAcurvefirst(GC,snum,sdata,NULL,NULL,
                  bsplinedif,bsplineint,NULL,c[0],c[4]);
    if (!GRAcurve(GC,c,c[0],c[4])) return 0;
    for (;i<num;i++) {
      for (j=1;j<4;j++) {
        bs1[j-1]=bs1[j];
        bs2[j-1]=bs2[j];
      }
      bs1[3]=pdata[i*2];
      bs2[3]=pdata[i*2+1];
      bspline(0,bs1,c);
      bspline(0,bs2,c+4);
      if (!GRAcurve(GC,c,c[0],c[4])) return 0;
    }
    for (j=0;j<3;j++) {
      bs1[4+j]=bs3[j];
      bs2[4+j]=bs4[j];
      bspline(0,bs1+j+1,c);
      bspline(0,bs2+j+1,c+4);
      if (!GRAcurve(GC,c,c[0],c[4])) return 0;
    }
    break;
  }
#endif

  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
curvebbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  int x, y, bminx, bminy, bmaxx, bmaxy;
  int i, num;
  struct narray *array;
  struct narray *points, *expand_points;
  int *pdata;
  int intp, width;

  array = * (struct narray **) rval;
  if (arraynum(array)) {
    return 0;
  }

  _getobj(obj, "_points", inst, &expand_points);
  _getobj(obj, "points", inst, &points);
  _getobj(obj, "width", inst, &width);
  _getobj(obj, "interpolation", inst, &intp);

  if (array==NULL) {
    array = arraynew(sizeof(int));
    if(array == NULL) {
      return 1;
    }
  }

  if (arraynum(expand_points) == 0) {
    curve_expand_points(obj, inst, intp, expand_points);
  }

  num = arraynum(expand_points) / 2;
  pdata = arraydata(expand_points);

  bmaxx = bminx = pdata[0];
  bmaxy = bminy = pdata[1];

  for (i = 1; i < num; i++) {
    x = pdata[i* 2];
    y = pdata[i * 2 + 1];
    if (x < bminx) bminx = x;
    if (x > bmaxx) bmaxx = x;
    if (y < bminy) bminy = y;
    if (y > bmaxy) bmaxy = y;
  }

  num = arraynum(points) / 2;
  pdata = arraydata(points);

#else
  int x,y,bminx,bminy,bmaxx,bmaxy;
  int i,num;
  struct narray *array;
  struct narray *points;
  double c[8];
  double *buf;
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int *pdata;
  int intp;
  int j,bsize,spcond;
  struct cmatchtype cmatch;
  int width;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"interpolation",inst,&intp);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  num=arraynum(points)/2;
  pdata=arraydata(points);
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    if (num<2) {
      arrayfree(array);
      *(struct narray **) rval = NULL;
      return 0;
    }
    bsize=num+1;
    if ((buf=g_malloc(sizeof(double)*9*bsize))==NULL) return 1;
    for (i=0;i<bsize;i++) buf[i]=i;
    for (i=0;i<num;i++) {
      buf[bsize+i]=pdata[i*2];
      buf[bsize*2+i]=pdata[i*2+1];
    }
    if (intp==INTERPOLATION_TYPE_SPLINE) {
      spcond=SPLCND2NDDIF;
    } else {
      spcond=SPLCNDPERIODIC;
      if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
        buf[num+bsize]=buf[bsize];
        buf[num+2*bsize]=buf[2*bsize];
        num++;
      }
    }
    if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
               spcond,spcond,0,0)) {
      g_free(buf);
      return 1;
    }
    if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
               spcond,spcond,0,0)) {
      g_free(buf);
      return 1;
    }
    GRAcmatchfirst(0,0,0,NULL,NULL,splinedif,splineint,NULL,
                   &cmatch,TRUE,buf[bsize],buf[2*bsize]);
    for (i=0;i<num-1;i++) {
      for (j=0;j<6;j++) c[j]=buf[i+(j+3)*bsize];
      GRAcmatch(c,buf[i+bsize],buf[i+2*bsize],&cmatch);
    }
    g_free(buf);
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    if (num<7) {
      arrayfree(array);
      *(struct narray **) rval = NULL;
      return 0;
    }
    for (i=0;i<7;i++) {
      bs1[i]=pdata[i*2];
      bs2[i]=pdata[i*2+1];
    }
    for (j=0;j<2;j++) {
      bspline(j+1,bs1+j,c);
      bspline(j+1,bs2+j,c+4);
      if (j==0) GRAcmatchfirst(0,0,0,NULL,NULL,bsplinedif,bsplineint,NULL,
                               &cmatch,TRUE,c[0],c[4]);
      GRAcmatch(c,c[0],c[4],&cmatch);
    }
    for (;i<num;i++) {
      for (j=1;j<7;j++) {
        bs1[j-1]=bs1[j];
        bs2[j-1]=bs2[j];
      }
      bs1[6]=pdata[i*2];
      bs2[6]=pdata[i*2+1];
      bspline(0,bs1+1,c);
      bspline(0,bs2+1,c+4);
      GRAcmatch(c,c[0],c[4],&cmatch);
    }
    for (j=0;j<2;j++) {
      bspline(j+3,bs1+j+2,c);
      bspline(j+3,bs2+j+2,c+4);
      GRAcmatch(c,c[0],c[4],&cmatch);
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    if (num<4) {
      arrayfree(array);
      *(struct narray **) rval = NULL;
      return 0;
    }
    for (i=0;i<4;i++) {
      bs1[i]=pdata[i*2];
      bs3[i]=pdata[i*2];
      bs2[i]=pdata[i*2+1];
      bs4[i]=pdata[i*2+1];
    }
    bspline(0,bs1,c);
    bspline(0,bs2,c+4);
    GRAcmatchfirst(0,0,0,NULL,NULL,bsplinedif,bsplineint,NULL,
                   &cmatch,TRUE,c[0],c[4]);
    GRAcmatch(c,c[0],c[4],&cmatch);
    for (;i<num;i++) {
      for (j=1;j<4;j++) {
        bs1[j-1]=bs1[j];
        bs2[j-1]=bs2[j];
      }
      bs1[3]=pdata[i*2];
      bs2[3]=pdata[i*2+1];
      bspline(0,bs1,c);
      bspline(0,bs2,c+4);
      GRAcmatch(c,c[0],c[4],&cmatch);
    }
    for (j=0;j<3;j++) {
      bs1[4+j]=bs3[j];
      bs2[4+j]=bs4[j];
      bspline(0,bs1+j+1,c);
      bspline(0,bs2+j+1,c+4);
      GRAcmatch(c,c[0],c[4],&cmatch);
    }
    break;
  }
  bminx=cmatch.minx-width/2;
  bminy=cmatch.miny-width/2;
  bmaxx=cmatch.maxx+width/2;
  bmaxy=cmatch.maxy+width/2;
  num=arraynum(points)/2;
#endif
  for (i=0;i<num;i++) {
    x=pdata[i*2];
    y=pdata[i*2+1];
    arrayadd(array,&x);
    arrayadd(array,&y);
  }
  arrayins(array,&(bmaxy),0);
  arrayins(array,&(bmaxx),0);
  arrayins(array,&(bminy),0);
  arrayins(array,&(bminx),0);
  if (arraynum(array)==0) {
    arrayfree(array);
    *(struct narray **) rval = NULL;
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}


static int 
curvematch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  int i,num;
  struct narray *array;
  struct narray *points;
  int *pdata;
  int intp;
#if CURVE_OBJ_USE_EXPAND_BUFFER
  double x1, y1, x2, y2;
  double r1, r2, r3, ip;
#else 
  double *buf;
  double c[8];
  double bs1[7], bs2[7], bs3[4], bs4[4];
  int j, bsize, spcond;
  struct cmatchtype cmatch;
#endif

  *(int *)rval=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if ((minx==maxx) && (miny==maxy)) {
#if CURVE_OBJ_USE_EXPAND_BUFFER
    _getobj(obj, "_points", inst, &points);
    _getobj(obj,"interpolation",inst,&intp);

    if (arraynum(points) == 0) {
      curve_expand_points(obj, inst, intp, points);
    }

    num = arraynum(points) / 2;
    pdata = arraydata(points);

    for (i = 0; i < num - 1; i++) {
      x1 = pdata[i * 2];
      y1 = pdata[i * 2 + 1];
      x2 = pdata[i * 2 + 2];
      y2 = pdata[i * 2 + 3];
      r2 = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
      r1 = sqrt((minx - x1) * (minx - x1) + (miny - y1) * (miny - y1));
      r3 = sqrt((minx - x2) * (minx - x2) + (miny - y2) * (miny - y2));
      if (r1 <= err || r3 < err) {
        *(int *)rval = TRUE;
        break;
      }
      if (r2) {
        ip = ((x2 - x1) * (minx - x1) + (y2 - y1) * (miny - y1)) / r2;
        if (0 <= ip && ip <= r2) {
          x2 = x1 + (x2 - x1) * ip / r2;
          y2 = y1 + (y2 - y1) * ip / r2;
          r1 = sqrt((minx - x2) * (minx - x2) + (miny - y2) * (miny - y2));
          if (r1 < err) {
            * (int *) rval = TRUE;
            break;
          }
        }
      }
    }
#else
    _getobj(obj,"points",inst,&points);
    _getobj(obj,"interpolation",inst,&intp);
    num=arraynum(points)/2;
    pdata=arraydata(points);
    switch (intp) {
    case INTERPOLATION_TYPE_SPLINE:
    case INTERPOLATION_TYPE_SPLINE_CLOSE:
      if (num<2) return 0;
      bsize=num+1;
      if ((buf=g_malloc(sizeof(double)*9*bsize))==NULL) return 1;
      for (i=0;i<bsize;i++) buf[i]=i;
      for (i=0;i<num;i++) {
        buf[bsize+i]=pdata[i*2];
        buf[bsize*2+i]=pdata[i*2+1];
      }
      if (intp==INTERPOLATION_TYPE_SPLINE) {
	spcond=SPLCND2NDDIF;
      } else {
        spcond=SPLCNDPERIODIC;
        if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
          buf[num+bsize]=buf[bsize];
          buf[num+2*bsize]=buf[2*bsize];
          num++;
        }
      }
      if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
                 spcond,spcond,0,0)) {
        g_free(buf);
        return 1;
      }
      if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
                 spcond,spcond,0,0)) {
        g_free(buf);
        return 1;
      }
      GRAcmatchfirst(minx,miny,err,NULL,NULL,splinedif,splineint,NULL,
                     &cmatch,FALSE,buf[bsize],buf[2*bsize]);
      for (i=0;i<num-1;i++) {
        for (j=0;j<6;j++) c[j]=buf[i+(j+3)*bsize];
        GRAcmatch(c,buf[i+bsize],buf[i+2*bsize],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          g_free(buf);
          return 0;
        }
      }
      g_free(buf);
      break;
    case INTERPOLATION_TYPE_BSPLINE:
      if (num<7) return 0;
      for (i=0;i<7;i++) {
        bs1[i]=pdata[i*2];
        bs2[i]=pdata[i*2+1];
      }
      for (j=0;j<2;j++) {
        bspline(j+1,bs1+j,c);
        bspline(j+1,bs2+j,c+4);
        if (j==0) GRAcmatchfirst(minx,miny,err,NULL,NULL,bsplinedif,bsplineint,NULL,
                                 &cmatch,FALSE,c[0],c[4]);
        GRAcmatch(c,c[0],c[4],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          return 0;
        }
      }
      for (;i<num;i++) {
        for (j=1;j<7;j++) {
          bs1[j-1]=bs1[j];
          bs2[j-1]=bs2[j];
        }
        bs1[6]=pdata[i*2];
        bs2[6]=pdata[i*2+1];
        bspline(0,bs1+1,c);
        bspline(0,bs2+1,c+4);
        GRAcmatch(c,c[0],c[4],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          return 0;
        }
      }
      for (j=0;j<2;j++) {
        bspline(j+3,bs1+j+2,c);
        bspline(j+3,bs2+j+2,c+4);
        GRAcmatch(c,c[0],c[4],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          return 0;
        }
      }
      break;
    case INTERPOLATION_TYPE_BSPLINE_CLOSE:
      if (num<4) return 0;
      for (i=0;i<4;i++) {
        bs1[i]=pdata[i*2];
        bs3[i]=pdata[i*2];
        bs2[i]=pdata[i*2+1];
        bs4[i]=pdata[i*2+1];
      }
      bspline(0,bs1,c);
      bspline(0,bs2,c+4);
      GRAcmatchfirst(minx,miny,err,NULL,NULL,bsplinedif,bsplineint,NULL,
                     &cmatch,FALSE,c[0],c[4]);
      GRAcmatch(c,c[0],c[4],&cmatch);
      if (cmatch.match) {
        *(int *)rval=TRUE;
        return 0;
      }
      for (;i<num;i++) {
        for (j=1;j<4;j++) {
          bs1[j-1]=bs1[j];
          bs2[j-1]=bs2[j];
        }
        bs1[3]=pdata[i*2];
        bs2[3]=pdata[i*2+1];
        bspline(0,bs1,c);
        bspline(0,bs2,c+4);
        GRAcmatch(c,c[0],c[4],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          return 0;
        }
      }
      for (j=0;j<3;j++) {
        bs1[4+j]=bs3[j];
        bs2[4+j]=bs4[j];
        bspline(0,bs1+j+1,c);
        bspline(0,bs2+j+1,c+4);
        GRAcmatch(c,c[0],c[4],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          return 0;
        }
      }
      break;
    }
#endif
  } else {
    if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
    _getobj(obj,"bbox",inst,&array);
    if (array==NULL) return 0;
    if (arraynum(array)<4) return 1;
    bminx=*(int *)arraynget(array,0);
    bminy=*(int *)arraynget(array,1);
    bmaxx=*(int *)arraynget(array,2);
    bmaxy=*(int *)arraynget(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) *(int *)rval=TRUE;
  }
  return 0;
}

static int 
curvegeometry(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (* (int *) (argv[2]) < 1)
    * (int *)(argv[2]) = 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
curve_flip(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  curve_clear(obj, inst);
#endif

  return legendflip(obj, inst, rval, argc, argv);
}

static int 
curve_move(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  struct narray *points;
  int i,num,*pdata;

  _getobj(obj, "_points", inst, &points);
  num = arraynum(points);
  pdata = arraydata(points);
  for (i = 0; i < num; i++) {
    if (i % 2 == 0) {
      pdata[i] += *(int *) argv[2];
    } else {
      pdata[i] += *(int *) argv[3];
    }
  }

#endif

  return legendmove(obj, inst, rval, argc, argv);
}

static int 
curve_rotate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  curve_clear(obj, inst);
#endif

  return legendrotate(obj, inst, rval, argc, argv);
}

static int 
curve_zoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  curve_clear(obj, inst);
#endif

  return legendzoom(obj, inst, rval, argc, argv);
}

static int 
curve_change(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
#if CURVE_OBJ_USE_EXPAND_BUFFER
  curve_clear(obj, inst);
#endif

  return legendchange(obj, inst, rval, argc, argv);
}

static struct objtable curve[] = {
  {"init",NVFUNC,NEXEC,curveinit,NULL,0},
  {"done",NVFUNC,NEXEC,curvedone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"points",NIARRAY,NREAD|NWRITE,curve_set_points,NULL,0},
#if CURVE_OBJ_USE_EXPAND_BUFFER
  {"_points",NIARRAY,NREAD,NULL,NULL,0},
#endif

  {"interpolation",NENUM,NREAD|NWRITE,curve_set_points,intpchar,0},
  {"width",NINT,NREAD|NWRITE,curvegeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,curvedraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,curvebbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,curve_move,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,curve_rotate,"iiii",0},
  {"flip",NVFUNC,NREAD|NEXEC,curve_flip,"iii",0},
  {"change",NVFUNC,NREAD|NEXEC,curve_change,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,curve_zoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,curvematch,"iiiii",0},
};

#define TBLNUM (sizeof(curve) / sizeof(*curve))

void *
addcurve(void)
/* addcurve() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,curve,ERRNUM,curveerrorlist,NULL,NULL);
}
