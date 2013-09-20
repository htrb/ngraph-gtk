/* 
 * $Id: osarray.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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
#include <string.h>

#include "ngraph.h"
#include "object.h"
#include "oiarray.h"

#define NAME "sarray"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRREGEXP		100
#define ERROUTBOUND		101

static char *sarrayerrorlist[]={
  "invalid regular expression."
  "array index is out of array bounds.",
};

#define DEFAULT_DELIMITER "\\s+"

struct osarray_local {
  GRegex *regexp;
};

#define ERRNUM (sizeof(sarrayerrorlist) / sizeof(*sarrayerrorlist))

static int 
sarrayinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct osarray_local *local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  local = g_malloc0(sizeof(*local));
  if (local == NULL) {
    return 1;
  }

  if (_putobj(obj, "_local", inst, local)) {
    g_free(local);
    return 1;
  }

  local->regexp = g_regex_new(DEFAULT_DELIMITER, 0, 0, NULL);

  return 0;
}

static int 
sarraydone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct osarray_local *local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  _getobj(obj, "_local", inst, &local);

  if (local->regexp) {
    g_regex_unref(local->regexp);
  }

  return 0;
}

static int 
sarrayget(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char **po;
  char *buf;

  g_free(rval->str);
  rval->str=NULL;
  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }

  po=(char **)arraynget(array,num);
  if (po==NULL) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if ((buf=g_malloc(strlen(*po)+1))==NULL) return 1;
  strcpy(buf,*po);
  rval->str=buf;
  return 0;
}

static int 
sarrayput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char *val;

  num=*(int *)argv[2];
  val=(char *)argv[3];
  _getobj(obj,"@",inst,&array);
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if (arrayput2(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
sarrayadd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char *val;

  val=(char *)argv[2];

  array = oarray_get_array(obj, inst, sizeof(char *));
  if (array == NULL) {
    return 1;
  }

  if (arrayadd2(array,&val)==NULL) return 1;
  return 0;
}

static int 
sarraypop(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char *val;
  int n;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "@", inst, &array);
  if (array == NULL) {
    return 1;
  }

  n = arraynum(array) - 1;
  if (n < 0) {
    return 1;
  }

  val = arraynget_str(array, n);

  if (arrayndel(array, n) == NULL) {
    g_free(val);
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      g_free(val);
      return 1;
    }
  }

  rval->str = val;

  return 0;
}

static int 
sarrayins(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  char *val;

  num=*(int *)argv[2];
  val=(char *)argv[3];

  array = oarray_get_array(obj, inst, sizeof(char *));
  if (array == NULL) {
    return 1;
  }
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }

  if (arrayins2(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
sarrayunshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char *val;

  val = (char *) argv[2];

  array = oarray_get_array(obj, inst, sizeof(char *));
  if (array == NULL) {
    return 1;
  }

  if (arrayins2(array, &val, 0)==NULL) {
    return 1;
  }

  return 0;
}

static int 
sarrayshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char *val;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "@", inst, &array);
  if (array == NULL) {
    return 1;
  }

  val = arraynget_str(array, 0);

  if (arrayndel(array, 0) == NULL) {
    g_free(val);
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      g_free(val);
      return 1;
    }
  }

  rval->str = val;

  return 0;
}

static int 
sarraydel(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) return 1;
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if (arrayndel2(array,num)==NULL) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if (arraynum(array)==0) {
    arrayfree2(array);
    if (_putobj(obj,"@",inst,NULL)) return 1;
  }
  return 0;
}

static int 
set_delimiter(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct osarray_local *local;
  GRegex *regexp;
  char *str;

  str = (char *) argv[2];
  if (str == NULL || str[0] == '\0') {
    str = DEFAULT_DELIMITER;
  }

  regexp = g_regex_new(str, 0, 0, NULL);
  if (regexp == NULL) {
    error(obj, ERRREGEXP);
    return 1;
  }

  _getobj(obj, "_local", inst, &local);
  if (local->regexp) {
    g_regex_unref(local->regexp);
  }

  local->regexp = regexp;

  return 0;
}

static int 
sarraysplit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct osarray_local *local;
  struct narray *array, *array2;
  int i;
  char *str, *delimiter, **sary;

  str = (char *) argv[2];
  if (str == NULL) {
    return 0;
  }

  _getobj(obj, "_local", inst, &local);

  _getobj(obj, "delimiter", inst, &delimiter);
  if (delimiter == NULL) {
    delimiter = " ";
  }

  sary = g_regex_split(local->regexp, str, 0);
  if (sary == NULL) {
    return 1;
  }

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    g_strfreev(sary);
    return 1;
  }

  for (i = 0; sary[i]; i++) {
    if (arrayadd(array, sary + i) == NULL) {
      arrayfree(array);
      g_strfreev(sary);
      return 1;
    }
  }

  g_free(sary);			/* don't free each element i.e. don't use g_strfreev() */

  _getobj(obj, "@", inst, &array2);
  if (array2) {
    arrayfree2(array2);
  }

  if (_putobj(obj, "@", inst, array) == -1) {
    arrayfree2(array);
    return 1;
  }

  return 0;
}

static int 
sarrayjoin(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  GString *str;
  int i, n;
  char *sep, *val, *ptr;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  ptr = (char *) argv[2];
  if (ptr) {
    sep = g_strcompress(ptr);
  } else {
    sep = g_strdup(",");
  }
  if (sep == NULL) {
    return 1;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    g_free(sep);
    return 1;
  }

  for (i = 0; i < n; i++) {
    val = arraynget_str(array, i);
    g_string_append_printf(str, "%s%s", val, (i == n - 1) ? "" : sep);
  }

  rval->str = g_string_free(str, FALSE);

  g_free(sep);

  return 0;
}

static int 
sarraysort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arraysort_str(array);

  return 0;
}

static int 
sarrayrsort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arrayrsort_str(array);

  return 0;
}

static int
sarrayuniq(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arrayuniq_str(array);

  return 0;
}

static int 
sarray_slice(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int start, len;

  start = * (int *) argv[2];
  len = * (int *) argv[3];

  if (_getobj(obj, "@", inst, &array)) {
    return 1;
  }

  if (array_slice2(array, start, len) == NULL) {
    return 1;
  }

  return 0;
}

static struct objtable osarray[] = {
  {"init",NVFUNC,NEXEC,sarrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,sarraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NSARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"delimiter",NSTR,NREAD|NWRITE,set_delimiter,NULL,0},
  {"get",NSFUNC,NREAD|NEXEC,sarrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,sarrayput,"is",0},
  {"add",NVFUNC,NREAD|NEXEC,sarrayadd,"s",0},
  {"push",NVFUNC,NREAD|NEXEC,sarrayadd,"s",0},
  {"pop",NSFUNC,NREAD|NEXEC,sarraypop,"",0},
  {"ins",NVFUNC,NREAD|NEXEC,sarrayins,"is",0},
  {"unshift",NVFUNC,NREAD|NEXEC,sarrayunshift,"s",0},
  {"shift",NSFUNC,NREAD|NEXEC,sarrayshift,"",0},
  {"del",NVFUNC,NREAD|NEXEC,sarraydel,"i",0},
  {"split",NVFUNC,NREAD|NEXEC,sarraysplit,"s",0},
  {"join",NSFUNC,NREAD|NEXEC,sarrayjoin,"s",0},
  {"sort",NVFUNC,NREAD|NEXEC,sarraysort,"s",0},
  {"rsort",NVFUNC,NREAD|NEXEC,sarrayrsort,"s",0},
  {"uniq", NSFUNC, NREAD|NEXEC, sarrayuniq, "", 0},
  {"num", NIFUNC, NREAD|NEXEC, oarray_num, "", 0},
  {"seq", NSFUNC, NREAD|NEXEC, oarray_seq, "", 0},
  {"rseq", NSFUNC, NREAD|NEXEC, oarray_reverse_seq, "", 0},
  {"reverse", NVFUNC, NREAD|NEXEC, oarray_reverse, "", 0},
  {"slice", NVFUNC, NREAD|NEXEC, sarray_slice, "ii", 0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(osarray) / sizeof(*osarray))

void *
addsarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,osarray,ERRNUM,sarrayerrorlist,NULL,NULL);
}
