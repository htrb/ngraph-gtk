/* 
 * $Id: opolygon.c,v 1.5 2009/04/15 05:03:57 hito Exp $
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
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"

#define NAME "polygon"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

static char *polyfillmode[]={
  "empty",
  "even_odd_rule",
  "winding_rule",
  NULL,
};

#define ERRNUM (sizeof(polyfillmode) / sizeof(*polyfillmode))

char *polyerrorlist[ERRNUM]={
  ""
};

static int 
polyinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int width,miter;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=40;
  miter=1000;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"miter_limit",inst,&miter)) return 1;
  return 0;
}

static int 
polydone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
polymatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  int i,num,*data;
  double x1,y1,x2,y2;
  double r,r2,r3,ip;
  struct narray *array;

  *(int *)rval=FALSE;
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
    num=arraynum(array)-4;
    data=arraydata(array);
    for (i=0;i<num;i+=2) {
      if (i==num-2) {
        x1=data[4+i];
        y1=data[5+i];
        x2=data[4];
        y2=data[5];
      } else {
        x1=data[4+i];
        y1=data[5+i];
        x2=data[6+i];
        y2=data[7+i];
      }
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((minx-x1)*(minx-x1)+(miny-y1)*(miny-y1));
      r3=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
      if ((r<=err) || (r3<err)) {
        *(int *)rval=TRUE;
        break;
      }
      if (r2!=0) {
        ip=((x2-x1)*(minx-x1)+(y2-y1)*(miny-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
          if (r<err) {
            *(int *)rval=TRUE;
            break;
          }
        }
      }
    }
  } else {
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
polydraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  struct narray *points;
  int num,*pdata;
  int width,join,miter,ifill,fr,fg,fb,tm,lm,w,h;
  struct narray *style;
  int snum,*sdata;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"join",inst,&join);
  _getobj(obj,"miter_limit",inst,&miter);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"fill",inst,&ifill);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  num=arraynum(points)/2;
  pdata=arraydata(points);
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb);
  if (ifill==0) {
    GRAlinestyle(GC,snum,sdata,width,0,join,miter);
    GRAdrawpoly(GC,num,pdata,0);
  } else {
    GRAdrawpoly(GC,num,pdata,ifill);
  }
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
polygeometry(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static struct objtable opoly[] = {
  {"init",NVFUNC,NEXEC,polyinit,NULL,0},
  {"done",NVFUNC,NEXEC,polydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"points",NIARRAY,NREAD|NWRITE,legendgeometry,NULL,0},

  {"fill",NENUM,NREAD|NWRITE,NULL,polyfillmode,0},
  {"width",NINT,NREAD|NWRITE,polygeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,polydraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,legendbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,legendmove,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,legendrotate,"iiii",0},
  {"change",NVFUNC,NREAD|NEXEC,legendchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,legendzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,polymatch,"iiiii",0},
};

#define TBLNUM (sizeof(opoly) / sizeof(*opoly))

void *addpolygon()
/* addpolygon() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,opoly,ERRNUM,polyerrorlist,NULL,NULL);
}
