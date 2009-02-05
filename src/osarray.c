/* 
 * $Id: osarray.c,v 1.3 2009/02/05 08:40:14 hito Exp $
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
#include <string.h>
#include "ngraph.h"
#include "object.h"

#define NAME "sarray"
#define PARENT "object"
#define OVERSION "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRILNAME 100

static char *sarrayerrorlist[]={
""
};

#define ERRNUM (sizeof(sarrayerrorlist) / sizeof(*sarrayerrorlist))

static int 
sarrayinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
sarraydone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
sarraynum(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"@",inst,&array);
  *(int *)rval=arraynum(array);
  return 0;
}

static int 
sarrayget(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char **po;
  char *buf;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  po=(char **)arraynget(array,num);
  if (po==NULL) return 1;
  if ((buf=memalloc(strlen(*po)+1))==NULL) return 1;
  strcpy(buf,*po);
  *(char **)rval=buf;
  return 0;
}

static int 
sarrayput(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char *val;

  num=*(int *)argv[2];
  val=(char *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (arrayput2(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
sarrayadd(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  char *val;

  val=(char *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(char *)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree2(array);
      return 1;
    }
  }
  if (arrayadd2(array,&val)==NULL) return 1;
  return 0;
}

static int 
sarrayins(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char *val;

  num=*(int *)argv[2];
  val=(char *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(char *)))==NULL) return 1;
    if (_putobj(obj,"@",inst,array)) {
      arrayfree2(array);
      return 1;
    }
  }
  if (arrayins2(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
sarraydel(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int num;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) return 1;
  if (arrayndel2(array,num)==NULL) return 1;
  if (arraynum(array)==0) {
    arrayfree2(array);
    if (_putobj(obj,"@",inst,NULL)) return 1;
  }
  return 0;
}

static struct objtable osarray[] = {
  {"init",NVFUNC,NEXEC,sarrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,sarraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NSARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"num",NIFUNC,NREAD|NEXEC,sarraynum,NULL,0},
  {"get",NSFUNC,NREAD|NEXEC,sarrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,sarrayput,"is",0},
  {"add",NVFUNC,NREAD|NEXEC,sarrayadd,"s",0},
  {"ins",NVFUNC,NREAD|NEXEC,sarrayins,"is",0},
  {"del",NVFUNC,NREAD|NEXEC,sarraydel,"i",0},
};

#define TBLNUM (sizeof(osarray) / sizeof(*osarray))

void *addsarray()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,osarray,ERRNUM,sarrayerrorlist,NULL,NULL);
}
