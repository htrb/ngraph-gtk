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

#define ERRINVALID_UTF8 100

static char *stringerrorlist[]={
  "invalid UTF-8 string."
};

#define ERRNUM (sizeof(stringerrorlist) / sizeof(*stringerrorlist))

static int 
stringinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
stringdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static void
set_length(struct objlist *obj, N_VALUE *inst, char *str)
{
  int byte, len;

  if (str) {
    byte = strlen(str);
    len = g_utf8_strlen(str, -1);
  } else {
    byte = 0;
    len = 0;
  }

  _putobj(obj, "byte", inst, &byte);
  _putobj(obj, "length", inst, &len);
}

static int 
string_strip(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  _getobj(obj, "@", inst, &str);
  if (str == NULL || str[0] == '\0') {
    return 0;
  }

  g_strstrip(str);

  set_length(obj, inst, str);

  return 0;
}

static int 
string_set(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  str = argv[2];

  if (str == NULL) {
    set_length(obj, inst, str);
    return 0;
  }

  if (! g_utf8_validate(str, -1, NULL)) {
    error(obj, ERRINVALID_UTF8);
    return 1;
  }

  set_length(obj, inst, str);

  return 0;
}

int 
string_upcase(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str, *ptr;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  ptr = g_utf8_strup(str, -1);
  if (ptr == NULL) {
    return 1;
  }

  if (_putobj(obj, "@", inst, ptr) < 0) {
    return 1;
  }

  g_free(str);

  return 0;
}

int 
string_downcase(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str, *ptr;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  ptr = g_utf8_strdown(str, -1);
  if (ptr == NULL) {
    return 1;
  }

  if (_putobj(obj, "@", inst, ptr) < 0) {
    return 1;
  }

  g_free(str);

  return 0;
}

int 
string_reverse(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str, *ptr;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  ptr = g_utf8_strreverse(str, -1);
  if (ptr == NULL) {
    return 1;
  }

  if (_putobj(obj, "@", inst, ptr) < 0) {
    return 1;
  }

  g_free(str);

  return 0;
}

static struct objtable ostring[] = {
  {"init",NVFUNC,NEXEC,stringinit,NULL,0},
  {"done",NVFUNC,NEXEC,stringdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NSTR,NREAD|NWRITE,string_set,NULL,0},
  {"length",NINT,NREAD,NULL,NULL,0},
  {"byte",NINT,NREAD,NULL,NULL,0},
  {"strip",NVFUNC,NREAD|NEXEC,string_strip,NULL,0},
  {"upcase",NVFUNC,NREAD|NEXEC,string_upcase,NULL,0},
  {"downcase",NVFUNC,NREAD|NEXEC,string_downcase,NULL,0},
  {"reverse",NVFUNC,NREAD|NEXEC,string_reverse,NULL,0},
};

#define TBLNUM (sizeof(ostring) / sizeof(*ostring))

void *addstring()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,ostring,ERRNUM,stringerrorlist,NULL,NULL);
}
