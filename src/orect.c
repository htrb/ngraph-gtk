/*
 * $Id: orect.c,v 1.18 2010-03-04 08:30:16 hito Exp $
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
#include "object.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"

#define NAME "rectangle"
#define PARENT "legend"
#define OVERSION  "1.00.00"

static char *recterrorlist[]={
  "",
};

#define ERRNUM (sizeof(recterrorlist) / sizeof(*recterrorlist))

static int
rectinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int width, stroke, alpha;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  width = DEFAULT_LINE_WIDTH;
  stroke = TRUE;
  alpha = 255;

  if (_putobj(obj, "width", inst, &width)) return 1;
  if (_putobj(obj, "stroke", inst, &stroke)) return 1;
  if (_putobj(obj, "stroke_A", inst, &alpha)) return 1;
  if (_putobj(obj, "fill_A", inst, &alpha)) return 1;

  return 0;
}

static int
rectdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
set_position(struct objlist *obj, N_VALUE *inst, int x1, int y1, int x2, int y2)
{
  if (_putobj(obj, "x1", inst, &x1)) return 1;
  if (_putobj(obj, "y1", inst, &y1)) return 1;
  if (_putobj(obj, "x2", inst, &x2)) return 1;
  if (_putobj(obj, "y2", inst, &y2)) return 1;

  return 0;
}

static void
get_position(struct objlist *obj, N_VALUE *inst, int *x1, int *y1, int *x2, int *y2)
{
  _getobj(obj, "x1", inst, x1);
  _getobj(obj, "y1", inst, y1);
  _getobj(obj, "x2", inst, x2);
  _getobj(obj, "y2", inst, y2);
}

static int
rectdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  int x1,y1,x2,y2,width,ifill,stroke,fr,fg,fb,fa,br,bg,bb,ba,w,h;
  struct narray *style;
  int snum,*sdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"stroke_R",inst,&fr);
  _getobj(obj,"stroke_G",inst,&fg);
  _getobj(obj,"stroke_B",inst,&fb);
  _getobj(obj,"stroke_A",inst,&fa);
  _getobj(obj,"fill_R",inst,&br);
  _getobj(obj,"fill_G",inst,&bg);
  _getobj(obj,"fill_B",inst,&bb);
  _getobj(obj,"fill_A", inst, &ba);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"stroke",inst,&stroke);
  _getobj(obj,"clip",inst,&clip);

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);

  if (ifill) {
    GRAcolor(GC,br,bg,bb, ba);
    GRArectangle(GC,x1,y1,x2,y2,1);
  }

  if (stroke) {
    GRAcolor(GC,fr,fg,fb, fa);
    GRAlinestyle(GC,snum,sdata,width,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
    GRArectangle(GC,x1,y1,x2,y2,0);
  }

  return 0;
}

static int
rectbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x1,y1,x2,y2,width,fill,stroke;
  struct narray *array;

  array=rval->array;
  if (arraynum(array)!=0) return 0;

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  _getobj(obj,"width",inst,&width);
  _getobj(obj,"fill",inst,&fill);
  _getobj(obj,"stroke",inst,&stroke);

  if (! fill && ! stroke) {
    return 0;
  }

  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  minx=(x1<x2) ? x1 : x2;
  miny=(y1<y2) ? y1 : y2;
  maxx=(x1>x2) ? x1 : x2;
  maxy=(y1>y2) ? y1 : y2;
  if (stroke) {
    minx-=width/2;
    miny-=width/2;
    maxx+=width/2;
    maxy+=width/2;
  }
  arrayins(array,&(maxy),0);
  arrayins(array,&(maxx),0);
  arrayins(array,&(miny),0);
  arrayins(array,&(minx),0);
  minx=(x1<x2) ? x1 : x2;
  miny=(y1<y2) ? y1 : y2;
  maxx=(x1>x2) ? x1 : x2;
  maxy=(y1>y2) ? y1 : y2;
  arrayadd(array,&(minx));
  arrayadd(array,&(miny));
  arrayadd(array,&(maxx));
  arrayadd(array,&(miny));
  arrayadd(array,&(maxx));
  arrayadd(array,&(maxy));
  arrayadd(array,&(minx));
  arrayadd(array,&(maxy));
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array=array;
  return 0;
}

static int
rectrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int angle, x1, y1, x2, y2, px, py, use_pivot;

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  angle = *(int *) argv[2];
  angle %= 36000;
  if (angle < 0)
    angle += 36000;

  use_pivot = * (int *) argv[3];
  if (use_pivot) {
    px = *(int *) argv[4];
    py = *(int *) argv[5];
  } else {
    px = (x1 + x2) / 2;
    py = (y1 + y2) / 2;
  }

  switch (angle) {
  case 9000:
  case 18000:
  case 27000:
    rotate(px, py, angle, &x1, &y1);
    rotate(px, py, angle, &x2, &y2);

    if (set_position(obj, inst, x1, y1, x2, y2))
      return 1;

    break;
  default:
    return 1;
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rectflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x1, y1, x2, y2, p, use_pivot;
  enum FLIP_DIRECTION dir;

  dir = (* (int *) argv[2] == FLIP_DIRECTION_HORIZONTAL) ? FLIP_DIRECTION_HORIZONTAL : FLIP_DIRECTION_VERTICAL;
  use_pivot = * (int *) argv[3];

  if (! use_pivot)
    return 0;

  _getobj(obj, "x1", inst, &x1);
  _getobj(obj, "y1", inst, &y1);
  _getobj(obj, "x2", inst, &x2);
  _getobj(obj, "y2", inst, &y2);

  p = *(int *) argv[4];
  flip(p, dir, &x1, &y1);
  flip(p, dir, &x2, &y2);

  _putobj(obj, "x1", inst, &x1);
  _putobj(obj, "y1", inst, &y1);
  _putobj(obj, "x2", inst, &x2);
  _putobj(obj, "y2", inst, &y2);

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rectmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x1,y1,x2,y2;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  x1+=*(int *)argv[2];
  x2+=*(int *)argv[2];
  y1+=*(int *)argv[3];
  y2+=*(int *)argv[3];

  if (set_position(obj, inst, x1, y1, x2, y2))
    return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rectchange(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x1,y1,x2,y2;
  int point,x,y;
  int *minx,*miny,*maxx,*maxy;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  point=*(int *)argv[2];
  x=*(int *)argv[3];
  y=*(int *)argv[4];

  minx=(x1<x2) ? &x1 : &x2;
  miny=(y1<y2) ? &y1 : &y2;
  maxx=(x1>x2) ? &x1 : &x2;
  maxy=(y1>y2) ? &y1 : &y2;
  switch (point) {
  case 0:
    (*minx)+=x;
    (*miny)+=y;
    break;
  case 1:
    (*maxx)+=x;
    (*miny)+=y;
    break;
  case 2:
    (*maxx)+=x;
    (*maxy)+=y;
    break;
  case 3:
    (*minx)+=x;
    (*maxy)+=y;
    break;
  default:
    return 1;
  }
  if (set_position(obj, inst, x1, y1, x2, y2))
    return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rectzoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int snum,*sdata,refx,refy,x1,y1,x2,y2,width,preserve_width;
  double zoom_x, zoom_y;
  struct narray *style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom_x = (*(int *) argv[2]) / 10000.0;
  zoom_y = (*(int *) argv[3]) / 10000.0;
  refx = (*(int *)argv[4]);
  refy = (*(int *)argv[5]);
  preserve_width = (*(int *)argv[6]);

  get_position(obj, inst, &x1, &y1, &x2, &y2);

  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);

  snum=arraynum(style);
  sdata=arraydata(style);
  x1=(x1-refx)*zoom_x+refx;
  y1=(y1-refy)*zoom_y+refy;
  x2=(x2-refx)*zoom_x+refx;
  y2=(y2-refy)*zoom_y+refy;
  if (! preserve_width) {
    int i;
    double zoom;
    zoom = MIN(zoom_x, zoom_y);
    width = width * zoom;
    for (i=0;i<snum;i++) sdata[i] *= zoom;
  }

  if (width < 1)
    width = 1;

  if (x1 == x2) {
    x2 = x1 + 1;
  }

  if (y1 == y2) {
    y2 = y1 + 1;
  }

  if (set_position(obj, inst, x1, y1, x2, y2))
    return 1;

  if (_putobj(obj,"width",inst,&width))
    return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rectmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;
  int ifill,stroke;

  rval->i=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;

  _getobj(obj,"bbox",inst,&array);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"stroke",inst,&stroke);

  if (! ifill && ! stroke) {
    return 0;
  }

  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if (arraynum(array)<4) return 1;
  bminx=arraynget_int(array,0);
  bminy=arraynget_int(array,1);
  bmaxx=arraynget_int(array,2);
  bmaxy=arraynget_int(array,3);
  if ((minx==maxx) && (miny==maxy)) {
    if (ifill) {
      bminx-=err;
      bminy-=err;
      bmaxx+=err;
      bmaxy+=err;
      if ((bminx<=minx) && (minx<=bmaxx)
       && (bminy<=miny) && (miny<=bmaxy)) rval->i=TRUE;
    } else {
      if ((((bminx-err<=minx) && (minx<=bminx+err))
         || ((bmaxx-err<=minx) && (minx<=bmaxx+err)))
       && (bminy-err<=miny) && (miny<=bmaxy+err)) rval->i=TRUE;
      if ((((bminy-err<=miny) && (miny<=bminy+err))
         || ((bmaxy-err<=miny) && (miny<=bmaxy+err)))
       && (bminx-err<=minx) && (minx<=bmaxx+err)) rval->i=TRUE;
    }
  } else {
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  return 0;
}

static int
rectgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
rect_frame(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int frame, fill, stroke;

  frame = * (int *) argv[2];

  _getobj(obj, "fill", inst, &fill);

  stroke = (frame || ! fill);
  _putobj(obj, "stroke", inst, &stroke);

  fill = (frame || fill);
  _putobj(obj, "fill", inst, &fill);

  return 0;
}

static int
put_color2(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,  int argc, char **argv)
{
  int fill, frame, col, val, val2, f;

  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "frame", inst, &frame);

  if (fill == 0 && frame == 0) {
    return 0;
  }

  col = argv[1][0];
  val =  * (int *) argv[2];

  if (val < 0) {
    val = 0;
  } else if (val > 255) {
    val = 255;
  }

  if (frame) {
    f = TRUE;
    _putobj(obj, "stroke", inst, &f);
    _putobj(obj, "fill", inst, &f);
  } else {
    f = FALSE;
    _putobj(obj, "stroke", inst, &f);
  }

  switch (col) {
  case 'R':
    _getobj(obj, "stroke_R", inst, &val2);
    _putobj(obj, "stroke_R", inst, &val);
    _putobj(obj, "fill_R", inst, &val2);
    break;
  case 'G':
    _getobj(obj, "stroke_G", inst, &val2);
    _putobj(obj, "stroke_G", inst, &val);
    _putobj(obj, "fill_G", inst, &val2);
    break;
  case 'B':
    _getobj(obj, "stroke_B", inst, &val2);
    _putobj(obj, "stroke_B", inst, &val);
    _putobj(obj, "fill_B", inst, &val2);
    break;
  }
  return 0;
}

static struct objtable rect[] = {
  {"init",NVFUNC,NEXEC,rectinit,NULL,0},
  {"done",NVFUNC,NEXEC,rectdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"x1",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"y1",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"x2",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"y2",NINT,NREAD|NWRITE,legendgeometry,NULL,0},

  {"stroke_R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_A",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"fill_R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_A",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"fill",NBOOL,NREAD|NWRITE,legendgeometry,NULL,0},
  {"stroke",NBOOL,NREAD|NWRITE,legendgeometry,NULL,0},
  {"width",NINT,NREAD|NWRITE,rectgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},

  {"draw",NVFUNC,NREAD|NEXEC,rectdraw,"i",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,rectbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,rectmove,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,rectrotate,"iiii",0},
  {"flip",NVFUNC,NREAD|NEXEC,rectflip,"iii",0},
  {"change",NVFUNC,NREAD|NEXEC,rectchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,rectzoom,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,rectmatch,"iiiii",0},

  {"fill_hsb", NVFUNC, NREAD|NEXEC, put_fill_hsb,"ddd",0},
  {"stroke_hsb", NVFUNC, NREAD|NEXEC, put_stroke_hsb,"ddd",0},

  /* following fields exist for backward compatibility */
  {"frame",NBOOL,NWRITE,rect_frame,NULL,0},
  {"R",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"G",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"B",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"A",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"R2",NINT,NWRITE,put_color2,NULL,0},
  {"G2",NINT,NWRITE,put_color2,NULL,0},
  {"B2",NINT,NWRITE,put_color2,NULL,0},
};

#define TBLNUM (sizeof(rect) / sizeof(*rect))

void *
addrectangle(void)
/* addrectangle() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,rect,ERRNUM,recterrorlist,NULL,NULL);
}
