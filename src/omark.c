/*
 * $Id: omark.c,v 1.16 2010-03-04 08:30:16 hito Exp $
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
#include "object.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"

#define NAME "mark"
#define PARENT "legend"
#define OVERSION  "1.00.00"

#define MODIFY_MARK_TYPE 0

static char *markerrorlist[]={
  "",
};

#define ERRNUM (sizeof(markerrorlist) / sizeof(*markerrorlist))

static int
markinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int size,width,r2,g2,b2,a2;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  size=DEFAULT_MARK_SIZE;
  width=DEFAULT_LINE_WIDTH;
  r2=255;
  g2=255;
  b2=255;
  a2=255;
  if (_putobj(obj,"size",inst,&size)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"R2",inst,&r2)) return 1;
  if (_putobj(obj,"G2",inst,&g2)) return 1;
  if (_putobj(obj,"B2",inst,&b2)) return 1;
  if (_putobj(obj,"A2",inst,&a2)) return 1;
  return 0;
}

static int
markdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
markdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  int x,y,type,size,width,fr,fg,fb,fa,br,bg,bb,ba,w,h;
  struct narray *style;
  int snum,*sdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"A",inst,&fa);
  _getobj(obj,"R2",inst,&br);
  _getobj(obj,"G2",inst,&bg);
  _getobj(obj,"B2",inst,&bb);
  _getobj(obj,"A2",inst,&ba);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"type",inst,&type);
  _getobj(obj,"size",inst,&size);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAlinestyle(GC,snum,sdata,width,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
  GRAmark(GC,type,x,y,size,fr,fg,fb,fa,br,bg,bb,ba);
  return 0;
}

static int
markbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,size,width;
  struct narray *array;

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"size",inst,&size);
  _getobj(obj,"width",inst,&width);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  size+=4*width;
  minx=x-size/2;
  miny=y-size/2;
  maxx=x+size/2;
  maxy=y+size/2;
  arrayins(array,&maxy,0);
  arrayins(array,&maxx,0);
  arrayins(array,&miny,0);
  arrayins(array,&minx,0);
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array=array;
  return 0;
}

static int
markmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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
markrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int angle, use_pivot, px, py, x, y;
#if MODIFY_MARK_TYPE
  int type;
#endif

  angle = *(int *) argv[2];
  use_pivot = * (int *) argv[3];
  px = *(int *) argv[4];
  py = *(int *) argv[5];

#if MODIFY_MARK_TYPE
  _getobj(obj, "type", inst, &type);
  type = mark_rotate(angle, type)
  _putobj(obj, "type", inst, &type);
#endif

  if (! use_pivot)
    return 0;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  rotate(px, py, angle, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
markflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x, y, p, use_pivot;
  enum FLIP_DIRECTION dir;
#if MODIFY_MARK_TYPE
  int type;
#endif

  dir = (* (int *) argv[2] == FLIP_DIRECTION_HORIZONTAL) ? FLIP_DIRECTION_HORIZONTAL : FLIP_DIRECTION_VERTICAL;
  use_pivot = * (int *) argv[3];

#if MODIFY_MARK_TYPE
  _getobj(obj, "type", inst, &type);
  type = mark_flip(dir, type);
  _putobj(obj, "type", inst, &type);
#endif

  if (! use_pivot)
    return 0;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);

  p = *(int *) argv[4];
  flip(p, dir, &x, &y);

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
markzoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y,size,refx,refy,width,snum,*sdata,preserve_width;
  double zoom, zoom_x, zoom_y;
  struct narray *style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom_x = (*(int *) argv[2]) / 10000.0;
  zoom_y = (*(int *) argv[3]) / 10000.0;
  zoom = MIN(zoom_x, zoom_y);
  refx = (*(int *)argv[4]);
  refy = (*(int *)argv[5]);
  preserve_width = (*(int *)argv[6]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"size",inst,&size);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  snum=arraynum(style);
  sdata=arraydata(style);
  x=(x-refx)*zoom_x+refx;
  y=(y-refy)*zoom_y+refy;
  size=size*zoom;
  if (! preserve_width) {
    int i;
    width=width*zoom;
    for (i=0;i<snum;i++) sdata[i]=sdata[i]*zoom;
  }

  if (width < 1)
    width = 1;

  if (size < 1)
    size = 1;

  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"size",inst,&size)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
markgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  char *field;
  int val;

  field = (char *) (argv[1]);
  val = * (int *) (argv[2]);

  if (strcmp(field, "width") == 0) {
    if (val < 1)
      val = 1;
  } else if (strcmp(field, "size") == 0) {
    if (val < 1)
      val = 1;
  }
  * (int *) (argv[2]) = val;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
markmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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

static struct objtable mark[] = {
  {"init",NVFUNC,NEXEC,markinit,NULL,0},
  {"done",NVFUNC,NEXEC,markdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"x",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"size",NINT,NREAD|NWRITE,markgeometry,NULL,0},

  {"type",NINT,NREAD|NWRITE,oputmarktype,NULL,0},
  {"R2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"A2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"width",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,markdraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,markbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,markmove,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,markrotate,"iiii",0},
  {"flip",NVFUNC,NREAD|NEXEC,markflip,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,markzoom,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,markmatch,"iiiii",0},

  {"hsb",NVFUNC,NREAD|NEXEC,put_hsb,"ddd",0},
  {"hsb2",NVFUNC,NREAD|NEXEC,put_hsb2,"ddd",0},
};

#define TBLNUM (sizeof(mark) / sizeof(*mark))

void *
addmark(void)
/* addmark() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,mark,ERRNUM,markerrorlist,NULL,NULL);
}
