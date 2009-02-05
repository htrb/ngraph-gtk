/* 
 * $Id: oline.c,v 1.4 2009/02/05 08:13:08 hito Exp $
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
#include "odraw.h"
#include "olegend.h"

#define NAME "line"
#define PARENT "legend"
#define OVERSION  "1.00.01"
#define TRUE  1
#define FALSE 0

#define ERRNUM 1

static char *arrowerrorlist[ERRNUM]={
  "",
};

static int 
arrowinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  int width,headlen,headwidth,miter;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=40;
  headlen=72426;
  headwidth=60000;
  miter=1000;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"miter_limit",inst,&miter)) return 1;
  if (_putobj(obj,"arrow_length",inst,&headlen)) return 1;
  if (_putobj(obj,"arrow_width",inst,&headwidth)) return 1;
  return 0;
}

static int 
arrowdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
arrowput(struct objlist *obj,char *inst,char *rval,
             int argc,char **argv)
{
  char *field;
  struct narray *array;

  field=argv[1];
  if (strcmp(field,"width")==0) {
    if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  } else if (strcmp(field,"arrow_length")==0) {
    if (*(int *)(argv[2])<10000) *(int *)(argv[2])=10000;
    else if (*(int *)(argv[2])>200000) *(int *)(argv[2])=200000;
  } else if (strcmp(field,"arrow_width")==0) {
    if (*(int *)(argv[2])<10000) *(int *)(argv[2])=10000;
    else if (*(int *)(argv[2])>200000) *(int *)(argv[2])=200000;
  }
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
arrowdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int width,fr,fg,fb,lm,tm,w,h,headlen,headwidth;
  int join,miter,head;
  struct narray *style;
  int snum,*sdata;
  int i,j,num;
  struct narray *points;
  int *points2;
  int *pdata;
  int x,y,x0,y0,x1,y1,x2,y2,x3,y3;
  int ax0,ay0,ap[6],ap2[6];
  double alen,alen2,awidth,len,dx,dy;
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
  _getobj(obj,"arrow",inst,&head);
  _getobj(obj,"arrow_length",inst,&headlen);
  _getobj(obj,"arrow_width",inst,&headwidth);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);
  num=arraynum(points)/2;
  pdata=arraydata(points);

  if ((points2=memalloc(sizeof(int)*num*2))==NULL) return 1;
  j=0;
  x1=y1=0;
  for (i=0;i<num;i++) {
    x0=pdata[2*i];
    y0=pdata[2*i+1];
    if ((i==0) || (x0!=x1) || (y0!=y1)) {
      points2[2*j]=x0;
      points2[2*j+1]=y0;
      j++;
      x1=x0;
      y1=y0;
    }
  }
  num=j;
  if (num<2) {
    memfree(points2);
    return 0;
  }
  x0=points2[0];
  y0=points2[1];
  x1=points2[2];
  y1=points2[3];
  x2=points2[2*num-4];
  y2=points2[2*num-3];
  x3=points2[2*num-2];
  y3=points2[2*num-1];
  alen=width*(double )headlen/10000;
  alen2=alen-10;
  if (alen2<0) alen2=0;
  awidth=width*(double )headwidth/20000;
  if ((head==2) || (head==3)) {
    dx=x0-x1;
    dy=y0-y1;
    len=sqrt(dx*dx+dy*dy);
    ax0=nround(x0-dx*alen/len);
    ay0=nround(y0-dy*alen/len);
    points2[0]=nround(x0-dx*alen2/len);
    points2[1]=nround(y0-dy*alen2/len);
    ap[0]=nround(ax0-dy/len*awidth);
    ap[1]=nround(ay0+dx/len*awidth);
    ap[2]=x0;
    ap[3]=y0;
    ap[4]=nround(ax0+dy/len*awidth);
    ap[5]=nround(ay0-dx/len*awidth);
  }
  if ((head==1) || (head==3)) {
    dx=x3-x2;
    dy=y3-y2;
    len=sqrt(dx*dx+dy*dy);
    ax0=nround(x3-dx*alen/len);
    ay0=nround(y3-dy*alen/len);
    points2[2*num-2]=nround(x3-dx*alen2/len);
    points2[2*num-1]=nround(y3-dy*alen2/len);
    ap2[0]=nround(ax0-dy/len*awidth);
    ap2[1]=nround(ay0+dx/len*awidth);
    ap2[2]=x3;
    ap2[3]=y3;
    ap2[4]=nround(ax0+dy/len*awidth);
    ap2[5]=nround(ay0-dx/len*awidth);
  }
  x=points2[0];
  y=points2[1];
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb);
  GRAlinestyle(GC,snum,sdata,width,0,join,miter);
  GRAmoveto(GC,x,y);
  for (i=1;i<num;i++) {
    x=points2[i*2];
    y=points2[i*2+1];
    GRAlineto(GC,x,y);
  }
  if ((head==2) || (head==3)) {
    GRAlinestyle(GC,0,NULL,1,0,join,miter);
    GRAdrawpoly(GC,3,ap,1);
  }
  if ((head==1) || (head==3)) {
    GRAlinestyle(GC,0,NULL,1,0,join,miter);
    GRAdrawpoly(GC,3,ap2,1);
  }
  memfree(points2);
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
arrowbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,num,num2;
  struct narray *points;
  int *pdata;
  struct narray *array;
  int i,j,width;
  int headlen,headwidth;
  int head;
  int *points2;
  int x0,y0,x1,y1,x2,y2,x3,y3;
  int ax0,ay0,ap[6],ap2[6];
  double alen,alen2,awidth,len,dx,dy;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"arrow",inst,&head);
  _getobj(obj,"arrow_length",inst,&headlen);
  _getobj(obj,"arrow_width",inst,&headwidth);
  num=arraynum(points)/2;
  pdata=arraydata(points);
  if ((points2=memalloc(sizeof(int)*num*2))==NULL) return 1;
  j=0;
  x1=y1=0;
  for (i=0;i<num;i++) {
    x0=pdata[2*i];
    y0=pdata[2*i+1];
    if ((i==0) || (x0!=x1) || (y0!=y1)) {
      points2[2*j]=x0;
      points2[2*j+1]=y0;
      j++;
      x1=x0;
      y1=y0;
    }
  }
  num2=j;
  if (num2<2) {
    memfree(points2);
    return 0;
  }
  x0=points2[0];
  y0=points2[1];
  x1=points2[2];
  y1=points2[3];
  x2=points2[2*num2-4];
  y2=points2[2*num2-3];
  x3=points2[2*num2-2];
  y3=points2[2*num2-1];
  memfree(points2);
  alen=width*(double )headlen/10000;
  alen2=alen-10;
  if (alen2<0) alen2=0;
  awidth=width*(double )headwidth/20000;
  if ((head==2) || (head==3)) {
    dx=x0-x1;
    dy=y0-y1;
    len=sqrt(dx*dx+dy*dy);
    ax0=nround(x0-dx*alen/len);
    ay0=nround(y0-dy*alen/len);
    ap[0]=nround(ax0-dy/len*awidth);
    ap[1]=nround(ay0+dx/len*awidth);
    ap[2]=x0;
    ap[3]=y0;
    ap[4]=nround(ax0+dy/len*awidth);
    ap[5]=nround(ay0-dx/len*awidth);
  }
  if ((head==1) || (head==3)) {
    dx=x3-x2;
    dy=y3-y2;
    len=sqrt(dx*dx+dy*dy);
    ax0=nround(x3-dx*alen/len);
    ay0=nround(y3-dy*alen/len);
    ap2[0]=nround(ax0-dy/len*awidth);
    ap2[1]=nround(ay0+dx/len*awidth);
    ap2[2]=x3;
    ap2[3]=y3;
    ap2[4]=nround(ax0+dy/len*awidth);
    ap2[5]=nround(ay0-dx/len*awidth);
  }
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
  if ((head==2) || (head==3)) {
    for (i=0;i<3;i++) {
      if (ap[i*2]<minx) minx=ap[i*2];
      if (ap[i*2]>maxx) maxx=ap[i*2];
      if (ap[i*2+1]<miny) miny=ap[i*2+1];
      if (ap[i*2+1]>maxy) maxy=ap[i*2+1];
    }
  }
  if ((head==1) || (head==3)) {
    for (i=0;i<3;i++) {
      if (ap2[i*2]<minx) minx=ap2[i*2];
      if (ap2[i*2]>maxx) maxx=ap2[i*2];
      if (ap2[i*2+1]<miny) miny=ap2[i*2+1];
      if (ap2[i*2+1]>maxy) maxy=ap2[i*2+1];
    }
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
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

#define TBLNUM 17

static struct objtable arrow[TBLNUM] = {
  {"init",NVFUNC,NEXEC,arrowinit,NULL,0},
  {"done",NVFUNC,NEXEC,arrowdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"points",NIARRAY,NREAD|NWRITE,legendgeometry,NULL,0},

  {"width",NINT,NREAD|NWRITE,arrowput,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"arrow",NENUM,NREAD|NWRITE,arrowput,arrowchar,0},
  {"arrow_length",NINT,NREAD|NWRITE,arrowput,NULL,0},
  {"arrow_width",NINT,NREAD|NWRITE,arrowput,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,arrowdraw,"i",0},

  {"bbox",NIAFUNC,NREAD|NEXEC,arrowbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,legendmove,"ii",0},
  {"change",NVFUNC,NREAD|NEXEC,legendchange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,legendzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,legendmatch,"iiiii",0},
};

void *addline()
/* addarrow() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,arrow,ERRNUM,arrowerrorlist,NULL,NULL);
}
