/* 
 * $Id: oiarray.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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
#include "ngraph.h"
#include "object.h"

#define NAME "iarray"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *iarrayerrorlist[]={
""
};

#define ERRNUM (sizeof(iarrayerrorlist) / sizeof(*iarrayerrorlist))

static int 
iarrayinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
iarraydone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
iarraynum(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"@",inst,&array);
  *(int *)rval=arraynum(array);
  return 0;
}

static int 
iarrayget(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num,*po;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  po=(int *)arraynget(array,num);
  if (po==NULL) return 1;
  *(int *)rval=*po;
  return 0;
}

static int 
iarrayput(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  int val;

  num=*(int *)argv[2];
  val=*(int *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (arrayput(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
iarrayadd(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int val;

  val=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(int)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree(array);
      return 1;
    }
  }
  if (arrayadd(array,&val)==NULL) return 1;
  return 0;
}

static int 
iarrayins(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  int val;

  num=*(int *)argv[2];
  val=*(int *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(int)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree(array);
      return 1;
    }
  }
  if (arrayins(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
iarraydel(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) return 1;
  if (arrayndel(array,num)==NULL) return 1;
  if (arraynum(array)==0) {
    arrayfree(array);
    if (_putobj(obj,"@",inst,NULL)) return 1;
  }
  return 0;
}

static struct objtable oiarray[] = {
  {"init",NVFUNC,NEXEC,iarrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,iarraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"num",NIFUNC,NREAD|NEXEC,iarraynum,NULL,0},
  {"get",NIFUNC,NREAD|NEXEC,iarrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,iarrayput,"ii",0},
  {"add",NVFUNC,NREAD|NEXEC,iarrayadd,"i",0},
  {"ins",NVFUNC,NREAD|NEXEC,iarrayins,"ii",0},
  {"del",NVFUNC,NREAD|NEXEC,iarraydel,"i",0},
};

#define TBLNUM (sizeof(oiarray) / sizeof(*oiarray))

void *
addiarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,oiarray,ERRNUM,iarrayerrorlist,NULL,NULL);
}
