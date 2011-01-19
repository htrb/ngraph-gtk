/* 
 * $Id: odarray.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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

#define NAME "darray"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *darrayerrorlist[]={
""
};

#define ERRNUM (sizeof(darrayerrorlist) / sizeof(*darrayerrorlist))

static int 
darrayinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
darraydone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
darraynum(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"@",inst,&array);
  rval->i=arraynum(array);
  return 0;
}

static int 
darrayget(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double *po;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  po=(double *)arraynget(array,num);
  if (po==NULL) return 1;
  rval->d=*po;
  return 0;
}

static int 
darrayput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double val;

  num=*(int *)argv[2];
  val=*(double *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (arrayput(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
darrayadd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;

  val=*(double *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(double)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree(array);
      return 1;
    }
  }
  if (arrayadd(array,&val)==NULL) return 1;
  return 0;
}

static int 
darrayins(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double val;

  num=*(int *)argv[2];
  val=*(double *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(double)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree(array);
      return 1;
    }
  }
  if (arrayins(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
darraydel(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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

static struct objtable odarray[] = {
  {"init",NVFUNC,NEXEC,darrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,darraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NDARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"num",NIFUNC,NREAD|NEXEC,darraynum,NULL,0},
  {"get",NDFUNC,NREAD|NEXEC,darrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,darrayput,"id",0},
  {"add",NVFUNC,NREAD|NEXEC,darrayadd,"d",0},
  {"ins",NVFUNC,NREAD|NEXEC,darrayins,"id",0},
  {"del",NVFUNC,NREAD|NEXEC,darraydel,"i",0},
};

#define TBLNUM (sizeof(odarray) / sizeof(*odarray))

void *
adddarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,odarray,ERRNUM,darrayerrorlist,NULL,NULL);
}
