/* 
 * $Id: olegend.c,v 1.12 2009/04/15 05:03:57 hito Exp $
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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ngraph.h"
#include "mathfn.h"
#include "object.h"
#include "olegend.h"

#define NAME "legend"
#define PARENT "draw"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

static char *legenderrorlist[]={
  "",
};

#define ERRNUM (sizeof(legenderrorlist) / sizeof(*legenderrorlist))

static int 
legendinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
legenddone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int 
legendgeometry(struct objlist *obj,char *inst,char *rval,
                   int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

int 
legendmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
    for (i=0;i<num-2;i+=2) {
      x1=data[4+i];
      y1=data[5+i];
      x2=data[6+i];
      y2=data[7+i];
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

int 
legendbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x,y,num;
  struct narray *points;
  int pnum;
  int *pdata;
  struct narray *array;
  int i,width;

  array=*(struct narray **)rval;
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
    *(struct narray **) rval = NULL;
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

int 
legendmove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *points,*array;
  int i,num,*pdata;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"points",inst,&points);
  num=arraynum(points);
  pdata=arraydata(points);
  for (i=0;i<num;i++) {
    if (i%2==0) pdata[i]+=*(int *)argv[2];
    else pdata[i]+=*(int *)argv[3];
  }
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
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

int 
legendrotate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *points, *array;
  int i, num, *pdata, angle, px, py, start, use_pivot;

  _getobj(obj, "points", inst, &points);
  num = arraynum(points);
  pdata = arraydata(points);

  if (num < 4)
    return 0;

  use_pivot = * (int *) argv[2];
  angle = *(int *) argv[3];

  if (use_pivot) {
    px = *(int *) argv[4];
    py = *(int *) argv[5];
    start = 0;
  } else {
    px = pdata[0];
    py = pdata[1];
    start = 1;
  }

  angle %= 36000;
  if (angle < 0)
    angle += 36000;

  for (i = start; i < num / 2; i++) {
    rotate(px, py, angle, &pdata[i * 2], &pdata[i * 2 + 1]);
  }

  _getobj(obj, "bbox", inst, &array);
  arrayfree(array);
  if (_putobj(obj, "bbox", inst, NULL)) return 1;

  return 0;
}

int 
legendchange(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *points,*array;
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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

int 
legendzoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *points,*array,*style;
  int i,num,width,snum,*pdata,*sdata,preserve_width;
  int refx,refy;
  double zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  preserve_width = (*(int *)argv[5]);
  _getobj(obj,"points",inst,&points);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  num=arraynum(points);
  pdata=arraydata(points);
  snum=arraynum(style);
  sdata=arraydata(style);
  if (num<4) return 0;
  for (i=0;i<num;i++) {
    if (i%2==0) pdata[i]=(pdata[i]-refx)*zoom+refx;
    else pdata[i]=(pdata[i]-refy)*zoom+refy;
  }
  if (! preserve_width) {
    width=width*zoom;
    for (i=0;i<snum;i++) sdata[i]=sdata[i]*zoom;
  }

  if (width < 1)
    width = 1;

  if (_putobj(obj,"width",inst,&width)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static struct objtable legend[] = {
  {"init",NVFUNC,0,legendinit,NULL,0},
  {"done",NVFUNC,0,legenddone,NULL,0},
  {"bbox",NIAFUNC,NREAD|NEXEC,NULL,"",0},
  {"move",NVFUNC,NREAD|NEXEC,NULL,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,NULL,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,NULL,"iiiii",0},
};

#define TBLNUM (sizeof(legend) / sizeof(*legend))

void *
addlegend(void)
/* addlegend() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,legend,ERRNUM,legenderrorlist,NULL,NULL);
}
