/* 
 * $Id: odraw.c,v 1.14 2009-11-16 09:13:04 hito Exp $
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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "object.h"
#include "ioutil.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "nstring.h"

#define NAME "draw"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILGC 100
#define ERRGCOPEN 101


static char *drawerrorlist[]={
  "illegal graphics context",
  "graphics context is not opened"
};

#define ERRNUM (sizeof(drawerrorlist) / sizeof(*drawerrorlist))

char *pathchar[]={
  N_("unchange"),
  N_("full"),
  N_("relative"),
  N_("base"),
  NULL,
};

char *capchar[]={
  "butt",
  "round",
  "projecting",
  NULL
};

char *joinchar[]={
  N_("miter"),
  N_("round"),
  N_("bevel"),
  NULL
};

char *fontchar[]={
  "Sans-serif",
  "Serif",
  "Monospace",
   NULL
};

char *arrowchar[]={
  N_("none"),
  N_("end"),
  N_("begin"),
  N_("both"),
  NULL
};

char *intpchar[]={
  N_("spline"),
  N_("spline_close"),
  N_("bspline"),
  N_("bspline_close"),
  NULL
};

static int 
drawinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int clip,redrawf,alpha;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  clip=TRUE;
  redrawf=TRUE;
  alpha=255;
  if (_putobj(obj,"clip",inst,&clip)) return 1;
  if (_putobj(obj,"redraw_flag",inst,&redrawf)) return 1;
  if (_putobj(obj,"A",inst,&alpha)) return 1;
  return 0;
}

static int 
drawdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}


static int 
drawdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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

int 
pathsave(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array,*array2;
  int anum;
  char **adata;
  int i,j;
  char *argv2[4];
  char *file,*name,*valstr;
  int path;
  GString *s;

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
  file="file";
  arrayadd(array2,&file);
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
      if ((name=g_malloc(strlen(file)+1))==NULL) goto errexit;
      strcpy(name,file);
    }
  }

  s = g_string_sized_new(256);
  if (s == NULL) {
    goto errexit;
  }
  g_string_append(s, rval->str);
  g_string_append_c(s, '\t');
  g_string_append(s, argv[0]);
  g_string_append(s, "::file=");

  valstr = getvaluestr(obj, "file", &name, FALSE, TRUE);
  if (valstr == NULL) {
    g_free(s);
    goto errexit;
  }
  g_string_append(s, valstr);
  g_free(valstr);
  g_string_append_c(s, '\n');
  g_free(name);
  g_free(rval->str);
  rval->str = g_string_free(s, FALSE);
  return 0;

errexit:
  g_free(name);
  g_free(rval->str);
  rval->str=NULL;
  return 1;
}

int
clear_bbox(struct objlist *obj, N_VALUE *inst)
{
  struct narray *array;

  if (inst == NULL)
    return 1;

  if (_getobj(obj, "bbox", inst, &array))
    return 1;

  arrayfree(array);

  if (_putobj(obj, "bbox", inst, NULL))
    return 1;

  return 0;
}

static struct objtable draw[] = {
  {"init",NVFUNC,0,drawinit,NULL,0},
  {"done",NVFUNC,0,drawdone,NULL,0},
  {"GC",NINT,0,NULL,NULL,0},
  {"hidden",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"redraw",NVFUNC,NREAD|NEXEC,drawdraw,"i",0},
  {"R",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"A",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"clip",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"redraw_flag",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"redraw_num",NINT,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(draw) / sizeof(*draw))

void *
adddraw(void)
/* adddraw() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,draw,ERRNUM,drawerrorlist,NULL,NULL);
}
