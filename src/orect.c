/* 
 * $Id: orect.c,v 1.6 2009/02/05 08:13:08 hito Exp $
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

#define NAME "rectangle"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRNUM 1

static char *recterrorlist[ERRNUM]={
  ""
};

static int 
rectinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int width,frame;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=40;
  frame=FALSE;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"frame",inst,&frame)) return 1;
  return 0;
}

static int 
rectdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
rectdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int x1,y1,x2,y2,width,ifill,iframe,fr,fg,fb,br,bg,bb,tm,lm,w,h;
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
  _getobj(obj,"x1",inst,&x1);
  _getobj(obj,"y1",inst,&y1);
  _getobj(obj,"x2",inst,&x2);
  _getobj(obj,"y2",inst,&y2);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"frame",inst,&iframe);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  if (iframe) {
    GRAcolor(GC,fr,fg,fb);
    GRArectangle(GC,x1,y1,x2,y2,1);
    GRAcolor(GC,br,bg,bb);
    GRAlinestyle(GC,snum,sdata,width,0,0,1000);
    GRArectangle(GC,x1,y1,x2,y2,0);
  } else if (!ifill) {
    GRAcolor(GC,fr,fg,fb);
    GRAlinestyle(GC,snum,sdata,width,0,0,1000);
    GRArectangle(GC,x1,y1,x2,y2,0);
  } else {
    GRAcolor(GC,fr,fg,fb);
    GRArectangle(GC,x1,y1,x2,y2,1);
  }
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
rectbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x1,y1,x2,y2,width,fill,frame;
  struct narray *array;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"x1",inst,&x1);
  _getobj(obj,"y1",inst,&y1);
  _getobj(obj,"x2",inst,&x2);
  _getobj(obj,"y2",inst,&y2);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"fill",inst,&fill);
  _getobj(obj,"frame",inst,&frame);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  minx=(x1<x2) ? x1 : x2;
  miny=(y1<y2) ? y1 : y2;
  maxx=(x1>x2) ? x1 : x2;
  maxy=(y1>y2) ? y1 : y2;
  if ((!fill) || frame) {
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
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

static int 
rectmove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x1,y1,x2,y2;
  struct narray *array;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x1",inst,&x1);
  _getobj(obj,"y1",inst,&y1);
  _getobj(obj,"x2",inst,&x2);
  _getobj(obj,"y2",inst,&y2);
  x1+=*(int *)argv[2];
  x2+=*(int *)argv[2];
  y1+=*(int *)argv[3];
  y2+=*(int *)argv[3];
  if (_putobj(obj,"x1",inst,&x1)) return 1;
  if (_putobj(obj,"y1",inst,&y1)) return 1;
  if (_putobj(obj,"x2",inst,&x2)) return 1;
  if (_putobj(obj,"y2",inst,&y2)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
rectchange(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x1,y1,x2,y2;
  int point,x,y;
  struct narray *array;
  int *minx,*miny,*maxx,*maxy;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x1",inst,&x1);
  _getobj(obj,"y1",inst,&y1);
  _getobj(obj,"x2",inst,&x2);
  _getobj(obj,"y2",inst,&y2);
  point=*(int *)argv[2];
  x=*(int *)argv[3];
  y=*(int *)argv[4];
  minx=(x1<x2) ? &x1 : &x2;
  miny=(y1<y2) ? &y1 : &y2;
  maxx=(x1>x2) ? &x1 : &x2;
  maxy=(y1>y2) ? &y1 : &y2;
  if (point==0) {
    (*minx)+=x;
    (*miny)+=y;
  } else if (point==1) {
    (*maxx)+=x;
    (*miny)+=y;
  } else if (point==2) {
    (*maxx)+=x;
    (*maxy)+=y;
  } else if (point==3) {
    (*minx)+=x;
    (*maxy)+=y;
  }
  if (_putobj(obj,"x1",inst,&x1)) return 1;
  if (_putobj(obj,"y1",inst,&y1)) return 1;
  if (_putobj(obj,"x2",inst,&x2)) return 1;
  if (_putobj(obj,"y2",inst,&y2)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
rectzoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,snum,*sdata,refx,refy,x1,y1,x2,y2,width,preserve_width;
  double zoom;
  struct narray *array,*style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  preserve_width = (*(int *)argv[5]);
  _getobj(obj,"x1",inst,&x1);
  _getobj(obj,"y1",inst,&y1);
  _getobj(obj,"x2",inst,&x2);
  _getobj(obj,"y2",inst,&y2);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  snum=arraynum(style);
  sdata=arraydata(style);
  x1=(x1-refx)*zoom+refx;
  y1=(y1-refy)*zoom+refy;
  x2=(x2-refx)*zoom+refx;
  y2=(y2-refy)*zoom+refy;
  if (! preserve_width) {
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

  if (_putobj(obj,"x1",inst,&x1)) return 1;
  if (_putobj(obj,"y1",inst,&y1)) return 1;
  if (_putobj(obj,"x2",inst,&x2)) return 1;
  if (_putobj(obj,"y2",inst,&y2)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
rectmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;
  int ifill,iframe;

  *(int *)rval=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
  _getobj(obj,"bbox",inst,&array);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"frame",inst,&iframe);
  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if (arraynum(array)<4) return 1;
  bminx=*(int *)arraynget(array,0);
  bminy=*(int *)arraynget(array,1);
  bmaxx=*(int *)arraynget(array,2);
  bmaxy=*(int *)arraynget(array,3);
  if ((minx==maxx) && (miny==maxy)) {
    if (ifill || iframe) {
      bminx-=err;
      bminy-=err;
      bmaxx+=err;
      bmaxy+=err;
      if ((bminx<=minx) && (minx<=bmaxx)
       && (bminy<=miny) && (miny<=bmaxy)) *(int *)rval=TRUE;
    } else {
      if (( ((bminx-err<=minx) && (minx<=bminx+err))
         || ((bmaxx-err<=minx) && (minx<=bmaxx+err)))
       && (bminy-err<=miny) && (miny<=bmaxy+err)) *(int *)rval=TRUE;
      if (( ((bminy-err<=miny) && (miny<=bminy+err))
         || ((bmaxy-err<=miny) && (miny<=bmaxy+err)))
       && (bminx-err<=minx) && (minx<=bmaxx+err)) *(int *)rval=TRUE;
    }
  } else {
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) *(int *)rval=TRUE;
  }
  return 0;
}

static int 
rectgeometry(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

#define TBLNUM 20

static struct objtable rect[TBLNUM] = {
  {"init",NVFUNC,NEXEC,rectinit,NULL,0},
  {"done",NVFUNC,NEXEC,rectdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"x1",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"y1",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"x2",NINT,NREAD|NWRITE,legendgeometry,NULL,0},
  {"y2",NINT,NREAD|NWRITE,legendgeometry,NULL,0},

  {"fill",NBOOL,NREAD|NWRITE,legendgeometry,NULL,0},
  {"frame",NBOOL,NREAD|NWRITE,legendgeometry,NULL,0},
  {"R2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"width",NINT,NREAD|NWRITE,rectgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,rectdraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,rectbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,rectmove,"ii",0},
  {"change",NVFUNC,NREAD|NEXEC,rectchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,rectzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,rectmatch,"iiiii",0},
};

void *addrectangle()
/* addrectangle() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,rect,ERRNUM,recterrorlist,NULL,NULL);
}
