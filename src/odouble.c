/* 
 * $Id: odouble.c,v 1.3 2009/02/05 08:40:14 hito Exp $
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

#define NAME "double"
#define PARENT "object"
#define OVERSION "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRILNAME 100

static char *doubleerrorlist[]={
""
};

#define ERRNUM (sizeof(doubleerrorlist) / sizeof(*doubleerrorlist))

static int 
doubleinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
doubledone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static struct objtable odouble[] = {
  {"init",NVFUNC,NEXEC,doubleinit,NULL,0},
  {"done",NVFUNC,NEXEC,doubledone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
};

#define TBLNUM (sizeof(odouble) / sizeof(*odouble))

void *adddouble()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,odouble,ERRNUM,doubleerrorlist,NULL,NULL);
}

