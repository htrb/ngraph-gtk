/* 
 * $Id: oarc.c,v 1.8 2009/03/24 09:15:13 hito Exp $
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
#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "mathfn.h"
#include "oroot.h"
#include "olegend.h"

#define NAME "arc"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

static char *arcerrorlist[]={
 ""
};

#define ERRNUM (sizeof(arcerrorlist) / sizeof(*arcerrorlist))

static int 
arcinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  int angle2,width,pieslice;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  angle2=36000;
  width=40;
  pieslice=TRUE;
  if (_putobj(obj,"pieslice",inst,&pieslice)) return 1;
  if (_putobj(obj,"angle2",inst,&angle2)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
  return 0;
}

static int 
arcdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
arcdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int x,y,rx,ry,angle1,angle2,width,ifill,fr,fg,fb,tm,lm,w,h;
  int pieslice;
  struct narray *style;
  int snum,*sdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"rx",inst,&rx);
  _getobj(obj,"ry",inst,&ry);
  _getobj(obj,"pieslice",inst,&pieslice);
  _getobj(obj,"angle1",inst,&angle1);
  _getobj(obj,"angle2",inst,&angle2);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb);
  if (ifill==0) {
    GRAlinestyle(GC,snum,sdata,width,0,0,1000);
    GRAcircle(GC,x,y,rx,ry,angle1,angle2,0);
  } else {
    if (pieslice) GRAcircle(GC,x,y,rx,ry,angle1,angle2,1);
    else GRAcircle(GC,x,y,rx,ry,angle1,angle2,2);
  }
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
arcgeometry(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  char *field;
  int val;
  struct narray *array;

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

  _getobj(obj,"bbox", inst, &array);
  arrayfree(array);

  if (_putobj(obj,"bbox",inst,NULL))
    return 1;

  return 0;
}

static int 
arcbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,x1,y1;
  int x0,y0,angle1,angle2,rx,ry,pieslice,fill;
  struct narray *array;
  int i,width;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"x",inst,&x0);
  _getobj(obj,"y",inst,&y0);
  _getobj(obj,"rx",inst,&rx);
  _getobj(obj,"ry",inst,&ry);
  _getobj(obj,"angle1",inst,&angle1);
  _getobj(obj,"angle2",inst,&angle2);
  _getobj(obj,"fill",inst,&fill);
  _getobj(obj,"pieslice",inst,&pieslice);
  _getobj(obj,"width",inst,&width);
  angle2+=angle1;
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
  if (fill && pieslice) {
    x=x0;
    y=y0;
    if (x<minx) minx=x;
    if (x>maxx) maxx=x;
    if (y<miny) miny=y;
    if (y>maxy) maxy=y;
  }
  if (!fill) {
    minx-=width/2;
    miny-=width/2;
    maxx+=width/2;
    maxy+=width/2;
  }
  arrayins(array,&maxy,0);
  arrayins(array,&maxx,0);
  arrayins(array,&miny,0);
  arrayins(array,&minx,0);
  if (arraynum(array)==0) {
    arrayfree(array);
    *(struct narray **) rval = NULL;
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

static int 
arcmove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
arczoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,snum,*sdata,rx,ry,x,y,refx,refy,width,preserve_width;
  double zoom;
  struct narray *array,*style;

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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
arcmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
  {"fill",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},
  {"pieslice",NBOOL,NREAD|NWRITE,arcgeometry,NULL,0},
  {"width",NINT,NREAD|NWRITE,arcgeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,arcdraw,"i",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,arcbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,arcmove,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,arczoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,arcmatch,"iiiii",0},
};

#define TBLNUM (sizeof(arc) / sizeof(*arc))

void *
addarc(void)
/* addarc() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,arc,ERRNUM,arcerrorlist,NULL,NULL);
}
