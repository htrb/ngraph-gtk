/* 
 * $Id: ocurve.c,v 1.5 2009/02/19 09:47:30 hito Exp $
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
#include "spline.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"

#define NAME "curve"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRSPL  100

char *curveerrorlist[]={
  "error: spline interpolation."
};

#define ERRNUM (sizeof(curveerrorlist) / sizeof(*curveerrorlist))

static int 
curveinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
curvedone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
curvedraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
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
  case 0: case 1:
    if (num<2) return 0;
    bsize=num+1;
    if ((buf=memalloc(sizeof(double)*9*bsize))==NULL) return 1;
    for (i=0;i<bsize;i++) buf[i]=i;
    for (i=0;i<num;i++) {
      buf[bsize+i]=pdata[i*2];
      buf[bsize*2+i]=pdata[i*2+1];
    }
    if (intp==0) spcond=SPLCND2NDDIF;
    else {
      spcond=SPLCNDPERIODIC;
      if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
        buf[num+bsize]=buf[bsize];
        buf[num+2*bsize]=buf[2*bsize];
        num++;
      }
    }
    if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
               spcond,spcond,0,0)) {
      memfree(buf);
      error(obj,ERRSPL);
      return 1;
    }
    if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
               spcond,spcond,0,0)) {
      memfree(buf);
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
    memfree(buf);
    break;
  case 2:
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
  case 3:
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
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
curvebbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
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
  case 0: case 1:
    if (num<2) {
      arrayfree(array);
      *(struct narray **) rval = NULL;
      return 0;
    }
    bsize=num+1;
    if ((buf=memalloc(sizeof(double)*9*bsize))==NULL) return 1;
    for (i=0;i<bsize;i++) buf[i]=i;
    for (i=0;i<num;i++) {
      buf[bsize+i]=pdata[i*2];
      buf[bsize*2+i]=pdata[i*2+1];
    }
    if (intp==0) spcond=SPLCND2NDDIF;
    else {
      spcond=SPLCNDPERIODIC;
      if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
        buf[num+bsize]=buf[bsize];
        buf[num+2*bsize]=buf[2*bsize];
        num++;
      }
    }
    if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
               spcond,spcond,0,0)) {
      memfree(buf);
      return 1;
    }
    if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
               spcond,spcond,0,0)) {
      memfree(buf);
      return 1;
    }
    GRAcmatchfirst(0,0,0,NULL,NULL,splinedif,splineint,NULL,
                   &cmatch,TRUE,buf[bsize],buf[2*bsize]);
    for (i=0;i<num-1;i++) {
      for (j=0;j<6;j++) c[j]=buf[i+(j+3)*bsize];
      GRAcmatch(c,buf[i+bsize],buf[i+2*bsize],&cmatch);
    }
    memfree(buf);
    break;
  case 2:
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
  case 3:
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
  double c[8];
  double *buf;
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int *pdata;
  int intp;
  int j,bsize,spcond;
  struct cmatchtype cmatch;

  *(int *)rval=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if ((minx==maxx) && (miny==maxy)) {
    _getobj(obj,"points",inst,&points);
    _getobj(obj,"interpolation",inst,&intp);
    num=arraynum(points)/2;
    pdata=arraydata(points);
    switch (intp) {
    case 0: case 1:
      if (num<2) return 0;
      bsize=num+1;
      if ((buf=memalloc(sizeof(double)*9*bsize))==NULL) return 1;
      for (i=0;i<bsize;i++) buf[i]=i;
      for (i=0;i<num;i++) {
        buf[bsize+i]=pdata[i*2];
        buf[bsize*2+i]=pdata[i*2+1];
      }
      if (intp==0) spcond=SPLCND2NDDIF;
      else {
        spcond=SPLCNDPERIODIC;
        if ((buf[num-1+bsize]!=buf[bsize]) || buf[num-1+2*bsize]!=buf[2*bsize]) {
          buf[num+bsize]=buf[bsize];
          buf[num+2*bsize]=buf[2*bsize];
          num++;
        }
      }
      if (spline(buf,buf+bsize,buf+3*bsize,buf+4*bsize,buf+5*bsize,num,
                 spcond,spcond,0,0)) {
        memfree(buf);
        return 1;
      }
      if (spline(buf,buf+2*bsize,buf+6*bsize,buf+7*bsize,buf+8*bsize,num,
                 spcond,spcond,0,0)) {
        memfree(buf);
        return 1;
      }
      GRAcmatchfirst(minx,miny,err,NULL,NULL,splinedif,splineint,NULL,
                     &cmatch,FALSE,buf[bsize],buf[2*bsize]);
      for (i=0;i<num-1;i++) {
        for (j=0;j<6;j++) c[j]=buf[i+(j+3)*bsize];
        GRAcmatch(c,buf[i+bsize],buf[i+2*bsize],&cmatch);
        if (cmatch.match) {
          *(int *)rval=TRUE;
          memfree(buf);
          return 0;
        }
      }
      memfree(buf);
      break;
    case 2:
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
    case 3:
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
  struct narray *array;

  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static struct objtable curve[] = {
  {"init",NVFUNC,NEXEC,curveinit,NULL,0},
  {"done",NVFUNC,NEXEC,curvedone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"points",NIARRAY,NREAD|NWRITE,legendgeometry,NULL,0},

  {"interpolation",NENUM,NREAD|NWRITE,legendgeometry,intpchar,0},
  {"width",NINT,NREAD|NWRITE,curvegeometry,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,curvedraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,curvebbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,legendmove,"ii",0},
  {"change",NVFUNC,NREAD|NEXEC,legendchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,legendzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,curvematch,"iiiii",0},
};

#define TBLNUM (sizeof(curve) / sizeof(*curve))

void *addcurve()
/* addcurve() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,curve,ERRNUM,curveerrorlist,NULL,NULL);
}
