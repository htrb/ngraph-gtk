/*
 * $Id: oint.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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
#include "object.h"

#define NAME "int"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *interrorlist[]={
  "",
};

#define ERRNUM (sizeof(interrorlist) / sizeof(*interrorlist))

static int
intinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
intdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
int_times(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int count;

  _getobj(obj, "@", inst, &count);
  rval->i = count;
  if (count <= 0) {
    return 1;
  }

  count--;
  _putobj(obj,"@",inst,&count);

  return 0;
}

static int
int_inc(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int count;

  _getobj(obj, "@", inst, &count);
  count++;
  rval->i = count;
  _putobj(obj,"@",inst,&count);

  return 0;
}

static int
int_dec(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int count;

  _getobj(obj, "@", inst, &count);
  count--;
  rval->i = count;
  _putobj(obj,"@",inst,&count);

  return 0;
}

static struct objtable oint[] = {
  {"init",NVFUNC,NEXEC,intinit,NULL,0},
  {"done",NVFUNC,NEXEC,intdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"times",NIFUNC,NREAD|NEXEC,int_times,"",0},
  {"inc",NIFUNC,NREAD|NEXEC,int_inc,"",0},
  {"dec",NIFUNC,NREAD|NEXEC,int_dec,"",0},
};

#define TBLNUM (sizeof(oint) / sizeof(*oint))

void *
addint(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,oint,ERRNUM,interrorlist,NULL,NULL);
}
