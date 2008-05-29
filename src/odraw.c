/* 
 * $Id: odraw.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "gra.h"
#include "odraw.h"
#include "nstring.h"

#define NAME "draw"
#define PARENT "object"
#define OVERSION "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRILGC 100
#define ERRGCOPEN 101

#define ERRNUM 2

char *drawerrorlist[ERRNUM]={
  "illegal graphics context",
  "grahics context is not opened"
};

char *pathchar[5]={
  "unchange",
  "full",
  "relative",
  "base",
  NULL,
};

char *capchar[4]={
  "butt",
  "round",
  "projecting",
  NULL
};

char *joinchar[4]={
  "miter",
  "round",
  "bevel",
  NULL
};

char *fontchar[14]={
  "Times",
  "TimesBold",
  "TimesItalic",
  "TimesBoldItalic",
  "Helvetica",
  "HelveticaBold",
  "HelveticaOblique",
  "HelveticaBoldOblique",
  "Courier",
  "CourierBold",
  "CourierOblique",
  "CourierBoldOblique",
  "Symbol",
   NULL
};

char *jfontchar[3]={
  "Mincho",
  "Gothic",
  NULL
};

char *arrowchar[5]={
  "none",
  "end",
  "begin",
  "both",
  NULL
};

char *intpchar[5]={
  "spline",
  "spline_close",
  "bspline",
  "bspline_close",
  NULL
};

int drawinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int clip,redrawf;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  clip=TRUE;
  redrawf=TRUE;
  if (_putobj(obj,"clip",inst,&clip)) return 1;
  if (_putobj(obj,"redraw_flag",inst,&redrawf)) return 1;
  return 0;
}

int drawdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}


int drawdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC,hidden;

  GC=*(int *)(argv[2]);
  if (GRAopened(GC)<0) {
    error3(obj,ERRGCOPEN,GC);
    return 1;
  }
  _getobj(obj,"hidden",inst,&hidden);
  if (hidden) GC=-1;
  if (_putobj(obj,"GC",inst,&GC)) return 1;
  return 0;
}

int pathsave(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array,*array2;
  int anum;
  char **adata;
  int i,j;
  char *argv2[4];
  char *file,*name,*s,*valstr;
  int path;

  array=(struct narray *)argv[2];
  anum=arraynum(array);
  adata=arraydata(array);
  for (j=0;j<anum;j++)
    if (strcmp0("file",adata[j])==0) {
      if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
      return 0;
    }
  array2=arraynew(sizeof(char *));
  for (i=0;i<anum;i++) arrayadd(array2,&(adata[i]));
  s="file";
  arrayadd(array2,&s);
  argv2[0]=argv[0];
  argv2[1]=argv[1];
  argv2[2]=(char *)array2;
  argv2[3]=NULL;
  if (_exeparent(obj,(char *)argv[1],inst,rval,3,argv2)) {
    arrayfree(array2);
    return 1;
  }
  arrayfree(array2);
  name=NULL;
  if (_getobj(obj,"save_path",inst,&path)) goto errexit;
  if (_getobj(obj,"file",inst,&file)) goto errexit;
  if (file!=NULL) {
    if (path==1) name=getfullpath(file);
    else if (path==2) name=getrelativepath(file);
    else if (path==3) name=getbasename(file);
    else if (path==0) {
      if ((name=memalloc(strlen(file)+1))==NULL) goto errexit;
      strcpy(name,file);
    }
  }

  if ((s=nstrnew())==NULL) goto errexit;
  if ((s=nstrcat(s,*(char **)rval))==NULL) goto errexit;
  if ((s=nstrccat(s,'\t'))==NULL) goto errexit;
  if ((s=nstrcat(s,argv[0]))==NULL) goto errexit;
  if ((s=nstrcat(s,"::file="))==NULL) goto errexit;
  if ((valstr=getvaluestr(obj,"file",&name,FALSE,TRUE))==NULL) {
    memfree(s);
    goto errexit;
  }
  if ((s=nstrcat(s,valstr))==NULL) {
    memfree(valstr);
    goto errexit;
  }
  memfree(valstr);
  if ((s=nstrccat(s,'\n'))==NULL) goto errexit;
  memfree(name);
  memfree(*(char **)rval);
  *(char **)rval=s;
  return 0;

errexit:
  memfree(name);
  memfree(*(char **)rval);
  *(char **)rval=NULL;
  return 1;
}

#define TBLNUM 11

struct objtable draw[TBLNUM] = {
  {"init",NVFUNC,0,drawinit,NULL,0},
  {"done",NVFUNC,0,drawdone,NULL,0},
  {"GC",NINT,0,NULL,NULL,0},
  {"hidden",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"redraw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"R",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"G",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"B",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"clip",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"redraw_flag",NBOOL,NREAD|NWRITE,NULL,NULL,0},
};

void *adddraw()
/* adddraw() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,draw,ERRNUM,drawerrorlist,NULL,NULL);
}
