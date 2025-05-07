/*
 * $Id: olegend.c,v 1.18 2010-03-04 08:30:16 hito Exp $
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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "spline.h"
#include "gra.h"
#include "mathfn.h"
#include "object.h"
#include "odraw.h"
#include "olegend.h"

#define NAME N_("legend")
#define PARENT "draw"
#define OVERSION  "1.00.00"

static char *legenderrorlist[]={
  "",
};

#define ERRNUM (sizeof(legenderrorlist) / sizeof(*legenderrorlist))

static int
legendinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
legenddone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int
legendgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                   int argc,char **argv)
{
  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
int
legendmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  struct narray *array;

  rval->i=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
  _getobj(obj,"bbox",inst,&array);
  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if ((minx==maxx) && (miny==maxy)) {
    int i, num;
    const int *data;
    num=arraynum(array)-4;
    data=arraydata(array);
    for (i=0;i<num-2;i+=2) {
      double x1, y1, x2, y2, r, r2, r3;
      x1=data[4+i];
      y1=data[5+i];
      x2=data[6+i];
      y2=data[7+i];
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((minx-x1)*(minx-x1)+(miny-y1)*(miny-y1));
      r3=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
      if ((r<=err) || (r3<err)) {
        rval->i=TRUE;
        break;
      }
      if (r2!=0) {
        int ip;
        ip=((x2-x1)*(minx-x1)+(y2-y1)*(miny-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
          if (r<err) {
            rval->i=TRUE;
            break;
          }
        }
      }
    }
  } else {
    int bminx, bminy, bmaxx, bmaxy;
    if (arraynum(array)<4) return 1;
    bminx=arraynget_int(array,0);
    bminy=arraynget_int(array,1);
    bmaxx=arraynget_int(array,2);
    bmaxy=arraynget_int(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  return 0;
}

int
legendbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,num;
  struct narray *points;
  int pnum;
  int *pdata;
  struct narray *array;
  int i,width;

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  pnum=arraynum(points);
  pdata=arraydata(points);
  num=pnum/2;
  if (num<2) return 0;
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  maxx=minx=pdata[0];
  maxy=miny=pdata[1];
  arrayadd(array,&(pdata[0]));
  arrayadd(array,&(pdata[1]));
  for (i=1;i<num;i++) {
    x=pdata[i*2];
    y=pdata[i*2+1];
    arrayadd(array,&x);
    arrayadd(array,&y);
    if (x<minx) minx=x;
    if (x>maxx) maxx=x;
    if (y<miny) miny=y;
    if (y>maxy) maxy=y;
  }
  minx-=width/2;
  miny-=width/2;
  maxx+=width/2;
  maxy+=width/2;
  arrayins(array,&(maxy),0);
  arrayins(array,&(maxx),0);
  arrayins(array,&(miny),0);
  arrayins(array,&(minx),0);
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array=array;
  return 0;
}
#endif  /* COMPILE_UNUSED_FUNCTIONS */

int
legendmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *points;
  int i,num,*pdata;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"points",inst,&points);
  num=arraynum(points);
  pdata=arraydata(points);
  for (i=0;i<num;i++) {
    if (i%2==0) pdata[i]+=*(int *)argv[2];
    else pdata[i]+=*(int *)argv[3];
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

void
rotate(int px, int py, int angle, int *x, int *y)
{
  int x0, y0;
  double t;

  t = - angle / 100.0 * MPI / 180;

  x0 = *x - px;
  y0 = *y - py;

  *x = nround(px + x0 * cos(t) - y0 * sin(t));
  *y = nround(py + x0 * sin(t) + y0 * cos(t));
}

void
flip(int pivot, enum FLIP_DIRECTION dir, int *x, int *y)
{
  switch (dir) {
  case FLIP_DIRECTION_VERTICAL:
    *y = pivot * 2 - *y;
    break;
  case FLIP_DIRECTION_HORIZONTAL:
    *x = pivot * 2 - *x;
    break;
  }
}

int
legendrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *points;
  int i, num, *pdata, angle, px, py, start, use_pivot;

  _getobj(obj, "points", inst, &points);
  num = arraynum(points);
  pdata = arraydata(points);

  if (num < 4)
    return 0;

  angle = *(int *) argv[2];
  use_pivot = * (int *) argv[3];
  if (use_pivot) {
    px = *(int *) argv[4];
    py = *(int *) argv[5];
    start = 0;
  } else {
    px = pdata[0];
    py = pdata[1];
    start = 1;
  }

  for (i = start; i < num / 2; i++) {
    rotate(px, py, angle, &pdata[i * 2], &pdata[i * 2 + 1]);
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

int
legendflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *points;
  int i, num, *pdata, p, start, use_pivot;
  enum FLIP_DIRECTION dir;

  _getobj(obj, "points", inst, &points);
  num = arraynum(points);
  pdata = arraydata(points);

  if (num < 4)
    return 0;

  dir = (* (int *) argv[2] == FLIP_DIRECTION_HORIZONTAL) ? FLIP_DIRECTION_HORIZONTAL : FLIP_DIRECTION_VERTICAL;
  use_pivot = * (int *) argv[3];

  if (use_pivot) {
    p = *(int *) argv[4];
    start = 0;
  } else {
    p = (dir == FLIP_DIRECTION_HORIZONTAL) ? pdata[0] : pdata[1];
    start = 1;
  }

  for (i = start; i < num / 2; i++) {
    flip(p, dir, &pdata[i * 2], &pdata[i * 2 + 1]);
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

int
legendchange(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *points;
  int num,*pdata;
  int point,x,y;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"points",inst,&points);
  point=*(int *)argv[2];
  x=*(int *)argv[3];
  y=*(int *)argv[4];
  num=arraynum(points);
  pdata=arraydata(points);
  if (point<num/2) {
    pdata[point*2]+=x;
    pdata[point*2+1]+=y;
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

int
legendzoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *points,*style;
  int i,num,width,snum,*pdata,*sdata,preserve_width;
  int refx,refy;
  double zoom_x, zoom_y;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom_x = (*(int *) argv[2]) / 10000.0;
  zoom_y = (*(int *) argv[3]) / 10000.0;
  refx = (*(int *)argv[4]);
  refy = (*(int *)argv[5]);
  preserve_width = (*(int *)argv[6]);
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  num=arraynum(points);
  pdata=arraydata(points);
  snum=arraynum(style);
  sdata=arraydata(style);
  if (num<4) return 0;
  for (i=0;i<num;i++) {
    if (i % 2 == 0) {
      pdata[i] = (pdata[i] - refx) * zoom_x + refx;
    } else {
      pdata[i] = (pdata[i] - refy) * zoom_y + refy;
    }
  }
  if (! preserve_width) {
    double zoom;
    zoom = MIN(zoom_x, zoom_y);
    width=width*zoom;
    for (i=0;i<snum;i++) sdata[i]=sdata[i]*zoom;
  }

  if (width < 1)
    width = 1;

  if (_putobj(obj,"width",inst,&width)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

void
draw_marker_wave(struct objlist *obj, N_VALUE *inst, int GC,
		 int width, int headlen, int headwidth, int x0, int y0, double dx, double dy, int errspl)
{
  int i;
  double awidth;
  double wx[5], wxc1[5], wxc2[5], wxc3[5];
  double wy[5], wyc1[5], wyc2[5], wyc3[5];
  double ww[5], c[6];

  awidth = width * (double) headwidth / 10000;

  dy = -dy;
  if (awidth == 0) {
    return;
  }
  for (i = 0; i < 5; i++) {
    ww[i] = i;
  }
  wx[0] = nround(x0 - dy * awidth);
  wx[1] = nround(x0 - dy * 0.5 * awidth - dx * 0.25 * awidth);
  wx[2] = x0;
  wx[3] = nround(x0 + dy * 0.5 * awidth + dx * 0.25 * awidth);
  wx[4] = nround(x0 + dy * awidth);
  if (spline(ww, wx, wxc1, wxc2, wxc3, 5, SPLCND2NDDIF, SPLCND2NDDIF, 0, 0)) {
    error(obj, errspl);
    return;
  }
  wy[0] = nround(y0 - dx * awidth);
  wy[1] = nround(y0 - dx * 0.5 * awidth + dy * 0.25 * awidth);
  wy[2] = y0;
  wy[3] = nround(y0 + dx * 0.5 * awidth - dy * 0.25 * awidth);
  wy[4] = nround(y0 + dx * awidth);
  if (spline(ww, wy, wyc1, wyc2, wyc3, 5, SPLCND2NDDIF, SPLCND2NDDIF, 0, 0)) {
    error(obj, errspl);
    return;
  }
  GRAlinestyle(GC, 0, NULL, width, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
  GRAcurvefirst(GC, 0, NULL, NULL, NULL, splinedif, splineint, NULL, wx[0], wy[0]);
  for (i = 0; i < 4; i++) {
    c[0] = wxc1[i];
    c[1] = wxc2[i];
    c[2] = wxc3[i];
    c[3] = wyc1[i];
    c[4] = wyc2[i];
    c[5] = wyc3[i];
    if (!GRAcurve(GC, c, wx[i], wy[i])) {
      break;
    }
  }
}

void
draw_marker_bar(struct objlist *obj, N_VALUE *inst, int GC,
		int width, int headlen, int headwidth, int x0, int y0, double dx, double dy)
{
  double awidth;
  int bar[4];

  awidth = width * (double) headwidth / 10000;

  dy = -dy;
  if (awidth == 0) {
    return;
  }
  bar[0] = nround(x0 - dy * awidth);
  bar[1] = nround(y0 - dx * awidth);
  bar[2] = nround(x0 + dy * awidth);
  bar[3] = nround(y0 + dx * awidth);
  GRAlinestyle(GC, 0, NULL, width, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
  GRAline(GC, bar[0], bar[1], bar[2], bar[3]);
}

void
draw_marker_mark(struct objlist *obj, N_VALUE *inst, int GC,
		 int width, int headlen, int headwidth,
		 int x0, int y0, double dx, double dy, int r, int g, int b, int a, int type)
{
  double awidth;
  int br, bg, bb, ba;

  _getobj(obj, "fill_R", inst, &br);
  _getobj(obj, "fill_G", inst, &bg);
  _getobj(obj, "fill_B", inst, &bb);
  _getobj(obj, "fill_A", inst, &ba);
  awidth = width * (double) headwidth / 10000;

  if (awidth == 0) {
    return;
  }
  GRAlinestyle(GC, 0, NULL, width, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
#if ROTATE_MARK
  GRAmark_rotate(GC, type, x0, y0, dx, dy, awidth, r, g, b, a, br, bg, bb, ba);
#else
  GRAmark(GC, type, x0, y0, awidth, r, g, b, a, br, bg, bb, ba);
#endif
}

static int
mark_rotate_cw(int mark) {
  switch (mark) {
  case 6:
  case 16:
  case 26:
  case 74:
    return mark + 3;
  case 7:
  case 17:
  case 27:
  case 72:
  case 78:
  case 75:
    return mark + 1;
  case 8:
  case 9:
  case 18:
  case 19:
  case 28:
  case 29:
  case 76:
  case 77:
    return mark - 2;
  case 30:
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
  case 36:
  case 37:
    return mark + 30;
  case 38:
  case 39:
  case 48:
  case 49:
    return mark + 20;
  case 40:
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
  case 46:
  case 47:
    return mark + 10;
  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
  case 58:
  case 59:
  case 60:
  case 61:
  case 62:
  case 63:
  case 64:
  case 65:
    return mark - 20;
  case 56:
  case 66:
  case 68:
    return mark - 19;
  case 57:
  case 67:
  case 69:
    return mark - 21;
  case 73:
  case 79:
    return mark - 1;
  default:
    return mark;
  }
}

int
mark_rotate(int angle, int type)
{
  angle %= 36000;
  if (angle < 0) {
    angle += 36000;
  }

  switch (angle) {
  case 9000:
    type = mark_rotate_cw(type);
    /* fall-through */
  case 18000:
    type = mark_rotate_cw(type);
    /* fall-through */
  case 27000:
    type = mark_rotate_cw(type);
    break;
  }
  return type;
}

static int
mark_v_flip(int mark) {
  switch (mark) {
  case 8:
  case 18:
  case 28:
  case 48:
  case 56:
  case 66:
  case 74:
    return mark + 1;
  case 9:
  case 19:
  case 29:
  case 49:
  case 57:
  case 67:
  case 75:
    return mark - 1;
  case 30:
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
  case 36:
  case 37:
    return mark + 10;
  case 40:
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
  case 46:
  case 47:
    return mark - 10;
  default:
    return mark;
  }
}

static int
mark_h_flip(int mark) {
  switch (mark) {
  case 6:
  case 16:
  case 26:
  case 36:
  case 46:
  case 68:
  case 76:
    return mark + 1;
  case 7:
  case 17:
  case 27:
  case 37:
  case 47:
  case 69:
  case 77:
    return mark - 1;
  case 50:
  case 51:
  case 52:
  case 53:
  case 54:
  case 55:
  case 56:
  case 57:
    return mark + 10;
  case 60:
  case 61:
  case 62:
  case 63:
  case 64:
  case 65:
  case 66:
  case 67:
    return mark - 10;
  default:
    return mark;
  }
}

int
mark_flip(int dir, int type)
{
  switch (dir) {
  case FLIP_DIRECTION_VERTICAL:
    type = mark_v_flip(type);
    break;
  case FLIP_DIRECTION_HORIZONTAL:
    type = mark_h_flip(type);
    break;
  }
  return type;
}

int
put_color_for_backward_compatibility(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int col, val;

  col = argv[1][0];
  val =  * (int *) argv[2];

  if (val < 0) {
    val = 0;
  } else if (val > 255) {
    val = 255;
  }

  switch (col) {
  case 'R':
    _putobj(obj, "stroke_R", inst, &val);
    _putobj(obj, "fill_R", inst, &val);
    break;
  case 'G':
    _putobj(obj, "stroke_G", inst, &val);
    _putobj(obj, "fill_G", inst, &val);
    break;
  case 'B':
    _putobj(obj, "stroke_B", inst, &val);
    _putobj(obj, "fill_B", inst, &val);
    break;
  case 'A':
    _putobj(obj, "stroke_A", inst, &val);
    _putobj(obj, "fill_A", inst, &val);
    break;
  }
  return 0;
}

static struct objtable legend[] = {
  {"init",NVFUNC,0,legendinit,NULL,0},
  {"done",NVFUNC,0,legenddone,NULL,0},
  {"bbox",NIAFUNC,NREAD|NEXEC,NULL,"",0},
  {"move",NVFUNC,NREAD|NEXEC,NULL,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,NULL,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,NULL,"iiiii",0},
};

#define TBLNUM (sizeof(legend) / sizeof(*legend))

void *
addlegend(void)
/* addlegend() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,legend,ERRNUM,legenderrorlist,NULL,NULL);
}
