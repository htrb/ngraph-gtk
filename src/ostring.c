/* 
 * $Id: ostring.c,v 1.5 2010-03-04 08:30:16 hito Exp $
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

#define NAME "string"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *stringerrorlist[]={
""
};

#define ERRNUM (sizeof(stringerrorlist) / sizeof(*stringerrorlist))

static int 
stringinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
stringdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static struct objtable ostring[] = {
  {"init",NVFUNC,NEXEC,stringinit,NULL,0},
  {"done",NVFUNC,NEXEC,stringdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NSTR,NREAD|NWRITE,NULL,NULL,0},
};

#define TBLNUM (sizeof(ostring) / sizeof(*ostring))

void *addstring()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,ostring,ERRNUM,stringerrorlist,NULL,NULL);
}
