/* 
 * $Id: odarray.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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
#include "ngraph.h"
#include "object.h"

#define NAME "darray"
#define PARENT "object"
#define OVERSION "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRILNAME 100

#define ERRNUM 1

char *darrayerrorlist[ERRNUM]={
""
};

int darrayinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int darraydone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int darraynum(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"@",inst,&array);
  *(int *)rval=arraynum(array);
  return 0;
}

int darrayget(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double *po;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  po=(double *)arraynget(array,num);
  if (po==NULL) return 1;
  *(double *)rval=*po;
  return 0;
}

int darrayput(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int darrayadd(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int darrayins(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int darraydel(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

#define TBLNUM 10

struct objtable odarray[TBLNUM] = {
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

void *adddarray()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,odarray,ERRNUM,darrayerrorlist,NULL,NULL);
}
