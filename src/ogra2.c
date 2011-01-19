/* 
 * $Id: ogra2.c,v 1.7 2010-03-04 08:30:16 hito Exp $
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
#include <glib.h>

#include "ngraph.h"
#include "object.h"

#define NAME "gra2"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRLOCK 100

static char *gra2errorlist[]={
  "device is locked"
};

#define ERRNUM (sizeof(gra2errorlist) / sizeof(*gra2errorlist))

static int 
gra2init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  GC=-1;
  if (_putobj(obj,"_GC",inst,&GC)) return 1;
  return 0;
}

static int 
gra2done(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  struct narray *sarray;
  struct objlist *gobj;
  int gid,deletegra,id;
  char *gfield;
  N_VALUE *ginst;
  char *device;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_GC",inst,&GC);
  _getobj(obj,"delete_gra",inst,&deletegra);
  _getobj(obj,"_list",inst,&sarray);
  if ((GC!=-1) && !deletegra) {
    error2(obj,ERRLOCK,argv[0]);
    return 1;
  }
  if (arraynum(sarray)!=0) {
    gobj=getobjlist(*(char **)arraynget(sarray,0),&gid,&gfield,NULL);
    if (gobj==NULL) return 0;
    if ((ginst=getobjinstoid(gobj,gid))==NULL) return 0;
    if (GC!=-1) _exeobj(gobj,"close",ginst,0,NULL);
    if (!_getobj(gobj,"_device",ginst,&device)) {
      g_free(device);
      _putobj(gobj,"_device",ginst,NULL);
    }
    if (deletegra) {
      if (_getobj(gobj,"id",ginst,&id)) return 0;
      delobj(gobj,id);
    }
  }
  return 0;
}

static int 
gra2clear(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i,num;
  struct narray *sarray;

  _getobj(obj,"_list",inst,&sarray);
  num=arraynum(sarray);
  for (i=num-1;i>0;i--) arrayndel2(sarray,i);
  return 0;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static int 
gra2disconnect(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  struct narray *sarray;
  struct objlist *gobj;
  int gid;
  char *gfield;
  char *ginst,*device;

  _getobj(obj,"_GC",inst,&GC);
  _getobj(obj,"_list",inst,&sarray);
  _putobj(obj,"_list",inst,NULL);
  if (arraynum(sarray)!=0) {
    gobj=getobjlist(*(char **)arraynget(sarray,0),&gid,&gfield,NULL);
    if (gobj!=NULL) {
      if ((ginst=getobjinstoid(gobj,gid))==NULL) return 0;
      if (GC!=-1) _exeobj(gobj,"close",ginst,0,NULL);
      if (!_getobj(gobj,"_device",ginst,&device)) {
        g_free(device);
        _putobj(gobj,"_device",ginst,NULL);
      }
    }
  }
  arrayfree2(sarray);
  return 0;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

static struct objtable gra2[] = {
  {"init",NVFUNC,NEXEC,gra2init,NULL,0},
  {"done",NVFUNC,NEXEC,gra2done,NULL,0},
  {"clear",NVFUNC,NREAD|NEXEC,gra2clear,"",0},
  {"_list",NSARRAY,NREAD,NULL,NULL,0},
  {"_GC",NINT,NREAD,NULL,NULL,0},
  {"delete_gra",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"disconnect",NVFUNC,NREAD|NEXEC,NULL,"",0},
};

#define TBLNUM (sizeof(gra2) / sizeof(*gra2))

void *
addgra2(void)
/* addgra2() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,gra2,ERRNUM,gra2errorlist,NULL,NULL);
}
