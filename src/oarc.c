/* 
 * $id: oarc.c,v 1.21 2010-03-04 08:30:16 hito Exp $
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
#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "mathfn.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"
#include "oarc.h"

#define NAME "arc"
#define PARENT "legend"
#define OVERSION  "1.00.00"

static char *arcerrorlist[]={
 ""
};

#define ERRNUM (sizeof(arcerrorlist) / sizeof(*arcerrorlist))

static int 
arcinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int angle2, width, pieslice, stroke, miter, join, alpha;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  angle2 = 36000;
  width = 40;
  pieslice = TRUE;
  miter = 1000;
  join = JOIN_TYPE_BEVEL;
  stroke = TRUE;
  alpha = 255;

  if (_putobj(obj, "pieslice", inst, &pieslice)) return 1;
  if (_putobj(obj, "angle2", inst, &angle2)) return 1;
  if (_putobj(obj, "width", inst, &width)) return 1;
  if (_putobj(obj, "miter_limit", inst, &miter)) return 1;
  if (_putobj(obj, "join", inst, &join)) return 1;
  if (_putobj(obj, "stroke", inst, &stroke)) return 1;
  if (_putobj(obj, "stroke_A", inst, &alpha)) return 1;
  if (_putobj(obj, "fill_A", inst, &alpha)) return 1;

  return 0;
}

static int 
arcdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
arcdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  int x,y,rx,ry,angle1,angle2,width,ifill,fr,fg,fb,fa,w,h,stroke,close_path,br,bg,bb, ba, join, miter;
  int pieslice;
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
  _getobj(obj,"fill_A",inst,&ba);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"rx",inst,&rx);
  _getobj(obj,"ry",inst,&ry);
  _getobj(obj,"pieslice",inst,&pieslice);
  _getobj(obj,"angle1",inst,&angle1);
  _getobj(obj,"angle2",inst,&angle2);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj, "join", inst, &join);
  _getobj(obj, "miter_limit", inst, &miter);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"stroke",inst,&stroke);
  _getobj(obj,"close_path",inst,&close_path);
  _getobj(obj,"clip",inst,&clip);

  if (! ifill && ! stroke) {
    return 0;
  }

  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);

  if (ifill) {
    GRAcolor(GC,br,bg,bb, ba);
    GRAcircle(GC, x, y, rx, ry, angle1, angle2,
	      (pieslice) ? 1 : 2);
  }

  if (stroke) {
    GRAcolor(GC,fr,fg,fb, fa);
    GRAlinestyle(GC, snum, sdata, width, 0, join, miter);
    GRAcircle(GC, x, y, rx, ry, angle1, angle2,
	      (close_path) ? ((pieslice) ? 3 : 4) : 0);
  }

  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
arcgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  char *field;
  int val;

  field = (char *) (argv[1]);
  val = * (int *) (argv[2]);

  if (strcmp(field,"width")==0) {
    if (val < 1)
      val = 1;
  } else if (strcmp(field, "rx") == 0 || strcmp(field, "ry") == 0) {
    if (val < 1)
      val = 1;
  } else if (strcmp(field, "angle1") == 0){
    if (val < 0) {
      val %= 36000;
      if (val < 0) {
	val += 36000;
      }
    } else if (val > 36000) {
      val %= 36000;
    }
  } else if (strcmp(field, "angle2") == 0) {
    if (val < 0) {
      val = 0;
    } else if (val > 36000) {
      val = 36000;
    }
  }
  * (int *)(argv[2]) = val;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arcbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,x1,y1;
  int x0,y0,angle1,angle2,rx,ry,pieslice,fill,stroke,close_path;
  struct narray *array;
  int i,width;

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"x",inst,&x0);
  _getobj(obj,"y",inst,&y0);
  _getobj(obj,"rx",inst,&rx);
  _getobj(obj,"ry",inst,&ry);
  _getobj(obj,"angle1",inst,&angle1);
  _getobj(obj,"angle2",inst,&angle2);
  _getobj(obj,"fill",inst,&fill);
  _getobj(obj,"stroke",inst,&stroke);
  _getobj(obj,"pieslice",inst,&pieslice);
  _getobj(obj,"close_path",inst,&close_path);
  _getobj(obj,"width",inst,&width);
  angle2+=angle1;

  if (! fill && ! stroke) {
    return 0;
  }

  if (angle2<angle1) angle2+=36000;
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;

  x1=x0+rx*cos(angle1*MPI/18000);
  y1=y0-ry*sin(angle1*MPI/18000);
  minx=x1;
  miny=y1;
  maxx=x1;
  maxy=y1;
  x=x0+rx*cos(angle2*MPI/18000);
  y=y0-ry*sin(angle2*MPI/18000);
  if (x<minx) minx=x;
  if (x>maxx) maxx=x;
  if (y<miny) miny=y;
  if (y>maxy) maxy=y;

  for (i=angle1/9000+1;i<=angle2/9000;i++) {
    x=x0+rx*cos(i*MPI/2);
    y=y0-ry*sin(i*MPI/2);
    if (x<minx) minx=x;
    if (x>maxx) maxx=x;
    if (y<miny) miny=y;
    if (y>maxy) maxy=y;
  }

  if (pieslice && (fill || close_path)) {
    x=x0;
    y=y0;
    if (x<minx) minx=x;
    if (x>maxx) maxx=x;
    if (y<miny) miny=y;
    if (y>maxy) maxy=y;
  }

  if (stroke) {
    minx-=width/2;
    miny-=width/2;
    maxx+=width/2;
    maxy+=width/2;
  }

  arrayins(array,&maxy,0);
  arrayins(array,&maxx,0);
  arrayins(array,&miny,0);
  arrayins(array,&minx,0);

  /* ARC_POINT_TYPE_R */
  x = x0 - rx;
  y = y0 - ry;
  arrayadd(array, &x);
  arrayadd(array, &y);

  /* ARC_POINT_TYPE_ANGLE1 */
  arrayadd(array, &x1);
  arrayadd(array, &y1);

  /* ARC_POINT_TYPE_ANGLE2 */
  if (angle2 - angle1 < 36000) {
    x = x0 + rx * cos(angle2 * MPI / 18000);
    y = y0 - ry * sin(angle2 * MPI / 18000);
    arrayadd(array, &x);
    arrayadd(array, &y);
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array = array;
  return 0;
}

static int 
arcmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  x+=*(int *)argv[2];
  y+=*(int *)argv[3];
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arcchange(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int point, a1, a2, rx, ry, ret;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  point = * (int *) argv[2];
  a1 = * (int *) argv[3];
  a2 = * (int *) argv[4];

  ret = 1;
  switch (point) {
  case ARC_POINT_TYPE_R:
    _getobj(obj, "rx", inst, &rx);
    _getobj(obj, "ry", inst, &ry);
    rx -= a1;
    ry -= a2;

    if (rx > 0)
      ret = _putobj(obj, "rx", inst, &rx);

    if (ry > 0)
      ret = _putobj(obj, "ry", inst, &ry);

    break;
  case ARC_POINT_TYPE_ANGLE1:
  case ARC_POINT_TYPE_ANGLE2:
    ret = _putobj(obj, "angle1", inst, &a1);
    ret = _putobj(obj, "angle2", inst, &a2);
    break;
  default:
    return 1;
  }

  if (ret)
    return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arcrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int tmp, angle, rx, ry, a, use_pivot;

  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a);

  angle = *(int *) argv[2];

  angle %= 36000;
  if (angle < 0)
    angle += 36000;

  switch (angle) {
  case 9000:
  case 27000:
    tmp = rx;
    rx = ry;
    ry = tmp;
    _putobj(obj, "rx", inst, &rx);
    _putobj(obj, "ry", inst, &ry);
  case 18000:
    a += angle;
    break;
  default:
    return 1;
  }

  a %= 36000;
  _putobj(obj, "angle1", inst, &a);

  use_pivot = * (int *) argv[3];
  if (use_pivot) {
    int x, y, px, py;

    px = *(int *) argv[4];
    py = *(int *) argv[5];
    _getobj(obj, "x", inst, &x);
    _getobj(obj, "y", inst, &y);
    rotate(px, py, angle, &x, &y);
    _putobj(obj, "x", inst, &x);
    _putobj(obj, "y", inst, &y);
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arcflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int rx, ry, a1, a2, use_pivot;
  enum FLIP_DIRECTION dir;

  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a1);
  _getobj(obj, "angle2", inst, &a2);

  dir = (* (int *) argv[2] == FLIP_DIRECTION_HORIZONTAL) ? FLIP_DIRECTION_HORIZONTAL : FLIP_DIRECTION_VERTICAL;

  switch (dir) {
  case FLIP_DIRECTION_VERTICAL:
    a1 = - a1 - a2;
    break;
  case FLIP_DIRECTION_HORIZONTAL:
    a1 = 18000 - a1 - a2;
    break;
  }

  a1 %= 36000;
  if (a1 < 0)
    a1 += 36000;

  _putobj(obj, "angle1", inst, &a1);

  use_pivot = * (int *) argv[3];
  if (use_pivot) {
    int x, y, p;

    p = *(int *) argv[4];
    _getobj(obj, "x", inst, &x);
    _getobj(obj, "y", inst, &y);
    flip(p, dir, &x, &y);
    _putobj(obj, "x", inst, &x);
    _putobj(obj, "y", inst, &y);
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arczoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i,snum,*sdata,rx,ry,x,y,refx,refy,width,preserve_width;
  double zoom;
  struct narray *style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  preserve_width = (*(int *)argv[5]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"rx",inst,&rx);
  _getobj(obj,"ry",inst,&ry);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  snum=arraynum(style);
  sdata=arraydata(style);
  x=(x-refx)*zoom+refx;
  y=(y-refy)*zoom+refy;
  rx=rx*zoom;
  ry=ry*zoom;

  if (rx < 1)
    rx = 1;

  if (ry < 1)
    ry = 1;

  if (! preserve_width) {
    width=width*zoom;
    for (i=0;i<snum;i++) sdata[i]=sdata[i]*zoom;
  }

  if (width < 1)
    width = 1;

  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"rx",inst,&rx)) return 1;
  if (_putobj(obj,"ry",inst,&ry)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int 
arcmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;

  rval->i=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
  _getobj(obj,"bbox",inst,&array);
  if (array==NULL) return 0;
  minx=*(int *)(argv[2]);
  miny=*(int *)(argv[3]);
  maxx=*(int *)(argv[4]);
  maxy=*(int *)(argv[5]);
  err=*(int *)(argv[6]);
  if (arraynum(array)<4) return 1;
  bminx=arraynget_int(array,0);
  bminy=arraynget_int(array,1);
  bmaxx=arraynget_int(array,2);
  bmaxy=arraynget_int(array,3);
  if ((minx==maxx) && (miny==maxy)) {
    bminx-=err;
    bminy-=err;
    bmaxx+=err;
    bmaxy+=err;
    if ((bminx<=minx) && (minx<=bmaxx)
     && (bminy<=miny) && (miny<=bmaxy)) rval->i=TRUE;
  } else {
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  return 0;
}

static struct objtable arc[] = {
  {"init",NVFUNC,NEXEC,arcinit,NULL,0},
  {"done",NVFUNC,NEXEC,arcdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"x",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"rx",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"ry",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"angle1",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"angle2",NINT,NREAD|NWRITE,arcgeometry,NULL,0},

  {"pieslice",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},

  {"fill_R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"fill_A",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"stroke_R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"stroke_A",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"fill",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},
  {"stroke",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},
  {"close_path",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},
  {"width",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},

  {"draw",NVFUNC,NREAD|NEXEC,arcdraw,"i",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,arcbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,arcmove,"ii",0}, 
  {"rotate",NVFUNC,NREAD|NEXEC,arcrotate,"iiii",0},
  {"flip",NVFUNC,NREAD|NEXEC,arcflip,"iii",0},
  {"change",NVFUNC,NREAD|NEXEC,arcchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,arczoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,arcmatch,"iiiii",0},


  /* following fields exist for backward compatibility */
  {"R",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"G",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"B",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
  {"A",NINT,NWRITE,put_color_for_backward_compatibility,NULL,0},
};

#define TBLNUM (sizeof(arc) / sizeof(*arc))

void *
addarc(void)
/* addarc() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,arc,ERRNUM,arcerrorlist,NULL,NULL);
}
