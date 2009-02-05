/* 
 * $Id: omark.c,v 1.6 2009/02/05 08:13:08 hito Exp $
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
#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "oroot.h"
#include "olegend.h"

#define NAME "mark"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRNUM 1

static char *markerrorlist[ERRNUM]={
  ""
};

static int 
markinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  int size,width,r2,g2,b2;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  size=200;
  width=40;
  r2=255;
  g2=255;
  b2=255;
  if (_putobj(obj,"size",inst,&size)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"R2",inst,&r2)) return 1;
  if (_putobj(obj,"G2",inst,&g2)) return 1;
  if (_putobj(obj,"B2",inst,&b2)) return 1;
  return 0;
}

static int 
markdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
markdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int x,y,type,size,width,fr,fg,fb,br,bg,bb,tm,lm,w,h;
  struct narray *style;
  int snum,*sdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"R2",inst,&br);
  _getobj(obj,"G2",inst,&bg);
  _getobj(obj,"B2",inst,&bb);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"type",inst,&type);
  _getobj(obj,"size",inst,&size);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAlinestyle(GC,snum,sdata,width,0,0,1000);
  GRAmark(GC,type,x,y,size,fr,fg,fb,br,bg,bb);
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
markbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,size,width;
  struct narray *array;

  array=*(struct narray **)rval;
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
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

static int 
markmove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x,y;
  struct narray *array;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  x+=*(int *)argv[2];
  y+=*(int *)argv[3];
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
markzoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,x,y,size,refx,refy,width,snum,*sdata,preserve_width;
  double zoom;
  struct narray *array,*style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  preserve_width = (*(int *)argv[5]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"size",inst,&size);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  snum=arraynum(style);
  sdata=arraydata(style);
  x=(x-refx)*zoom+refx;
  y=(y-refy)*zoom+refy;
  size=size*zoom;
  if (! preserve_width) {
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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
markgeometry(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  char *field;
  struct narray *array;
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

  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);

  if (_putobj(obj,"bbox",inst,NULL))
    return 1;

  return 0;
}

static int 
marktype(struct objlist *obj,char *inst,char *rval,
	     int argc,char **argv)
{
  int type;

  type = * (int *) (argv[2]);

  if (type < 0 || type > 89) {
    * (int *) (argv[2]) = 0;
  }

  return 0;
}

static int 
markmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;

  *(int *)rval=FALSE;
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
  bminx=*(int *)arraynget(array,0);
  bminy=*(int *)arraynget(array,1);
  bmaxx=*(int *)arraynget(array,2);
  bmaxy=*(int *)arraynget(array,3);
  if ((minx==maxx) && (miny==maxy)) {
    bminx-=err;
    bminy-=err;
    bmaxx+=err;
    bmaxy+=err;
    if ((bminx<=minx) && (minx<=bmaxx)
     && (bminy<=miny) && (miny<=bmaxy)) *(int *)rval=TRUE;
  } else {
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) *(int *)rval=TRUE;
  }
  return 0;
}

#define TBLNUM 17

static struct objtable mark[TBLNUM] = {
  {"init",NVFUNC,NEXEC,markinit,NULL,0},
  {"done",NVFUNC,NEXEC,markdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"x",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"size",NINT,NREAD|NWRITE,markgeometry,NULL,0},

  {"type",NINT,NREAD|NWRITE,marktype,NULL,0},
  {"R2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"width",NINT,NREAD|NWRITE,markgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,markdraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,markbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,markmove,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,markzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,markmatch,"iiiii",0},
};

void *addmark()
/* addmark() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,mark,ERRNUM,markerrorlist,NULL,NULL);
}
