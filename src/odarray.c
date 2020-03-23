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
#include <math.h>
#include <ctype.h>
#include "object.h"
#include "oiarray.h"

#include "math/math_equation.h"

#define NAME "darray"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100
#define ERROUTBOUND		101

static char *darrayerrorlist[]={
  "",
  "array index is out of array bounds.",
};

struct darray_local {
  double sum, ssum, min, max;
  int num;
  int modified;
};

#define ERRNUM (sizeof(darrayerrorlist) / sizeof(*darrayerrorlist))

static int
darrayinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct darray_local *local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  local = g_malloc0(sizeof(*local));
  if (local == NULL) {
    return 1;
  }

  local->modified = TRUE;
 _putobj(obj, "_local", inst, local);

  return 0;
}

static int
darraydone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
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
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }

  po=(double *)arraynget(array,num);
  if (po==NULL) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  rval->d=*po;
  return 0;
}

static int
darrayput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double val;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  num=*(int *)argv[2];
  val = arg_to_double(argv, 3);
  _getobj(obj,"@",inst,&array);
  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if (arrayput(array,&val,num)==NULL) return 1;
  return 0;
}

static int
darray_modified(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  return 0;
}

static int
darrayadd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  val = arg_to_double(argv, 2);

  array = oarray_get_array(obj, inst, sizeof(double));
  if (array==NULL) {
    return 1;
  }

  if (arrayadd(array,&val)==NULL) return 1;
  return 0;
}

static int
darraypop(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;
  int n;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  rval->d = 0.0;

  _getobj(obj,"@",inst,&array);
  if (array == NULL) {
    return 1;
  }

  n = arraynum(array) - 1;
  if (n < 0) {
    return 1;
  }

  val = arraynget_double(array, n);
  if (arrayndel(array, n) == NULL) {
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      return 1;
    }
  }

  rval->d = val;

  return 0;
}

static int
darrayins(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double val;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  num=*(int *)argv[2];
  val=*(double *)argv[3];

  array = oarray_get_array(obj, inst, sizeof(double));
  if (array==NULL) {
    return 1;
  }

  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }

  if (arrayins(array,&val,num)==NULL) return 1;
  return 0;
}

static int
darrayunshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  val = * (double *) argv[2];

  array = oarray_get_array(obj, inst, sizeof(double));
  if (array == NULL) {
    return 1;
  }

  if (arrayins(array, &val, 0)==NULL) {
    return 1;
  }

  return 0;
}

static int
darrayshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  rval->d = 0;

  _getobj(obj,"@",inst,&array);
  if (array == NULL) {
    return 1;
  }

  val = arraynget_double(array, 0);

  if (arrayndel(array, 0) == NULL) {
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      return 1;
    }
  }

  rval->d = val;

  return 0;
}

static int
darraydel(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) return 1;

  num = oarray_get_index(array, num);
  if (num < 0) {
    error(obj, ERROUTBOUND);
    return 1;
  }

  if (arrayndel(array,num)==NULL) {
    error(obj, ERROUTBOUND);
    return 1;
  }
  if (arraynum(array)==0) {
    arrayfree(array);
    if (_putobj(obj,"@",inst,NULL)) return 1;
  }
  return 0;
}

static int
darraysort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arraysort_double(array);

  return 0;
}

static int
darrayrsort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arrayrsort_double(array);

  return 0;
}

static int
darrayuniq(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

  _getobj(obj, "@", inst, &array);

  arrayuniq_double(array);

  return 0;
}

static int
darrayjoin(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  GString *str;
  int i, n;
  char *sep, *ptr;
  struct darray_local *local;

  _getobj(obj, "_local", inst, &local);
  local->modified = TRUE;

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
    double val;
    val = arraynget_double(array, i);
    g_string_append_printf(str, "%.15e%s", val, (i == n - 1) ? "" : sep);
  }

  rval->str = g_string_free(str, FALSE);

  g_free(sep);

  return 0;
}

static void
cache_data(struct objlist *obj, N_VALUE *inst, struct darray_local *local)
{
  struct narray *array;
  double min, max, sum, ssum, *data;
  int n;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  data = arraydata(array);
  min = 0;
  max = 0;
  sum = 0;
  ssum = 0;
  if (n > 0) {
    int i;
    min = max = data[0];
    for (i = 0; i < n; i++) {
      sum += data[i];
      ssum += data[i] * data[i];
      if (data[i] < min) {
	min = data[i];
      } else if (data[i] > max) {
	max = data[i];
      }
    }
  }

  local->num = n;
  local->min = min;
  local->max = max;
  local->sum = sum;
  local->ssum = ssum;
  local->modified = FALSE;
}

static int
darray_sum(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  rval->d = local->sum;

  return 0;
}

static int
darray_average(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  if (local->num == 0) {
    return 0;
  }

  rval->d = local->sum / local->num;

  return 0;
}

static int
darray_rms(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  if (local->num == 0) {
    return 0;
  }

  rval->d = sqrt(local->ssum / local->num);

  return 0;
}

static int
darray_sdev(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  double sum, ssum, val;
  int n;
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  n = local->num;
  if (n == 0) {
    return 0;
  }

  sum = local->sum;
  ssum = local->ssum;

  sum /= n;
  val = ssum / n - sum * sum;

  rval->d = (val < 0) ? 0 : sqrt(val);

  return 0;
}

static int
darray_min(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  rval->d = local->min;

  return 0;
}

static int
darray_max(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct darray_local *local;

  rval->d = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->modified) {
    cache_data(obj, inst, local);
  }

  rval->d = local->max;

  return 0;
}

static int
darray_map(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, n;
  double *data;
  MathEquation *code;
  MathValue val;

  if (argv[2] == NULL) {
    return 0;
  }

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 1;
  }

  code = oarray_create_math(obj, argv[1], argv[2]);
  if (code == NULL) {
    return 1;
  }

  data = arraydata(array);
  for (i = 0; i < n; i++) {
    val.val = data[i];
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_var(code, 0, &val);
    val.val = i;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_var(code, 1, &val);
    math_equation_calculate(code, &val);
    data[i] = val.val;
  }

  math_equation_free(code);

  return 0;
}

static struct objtable odarray[] = {
  {"init",NVFUNC,NEXEC,darrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,darraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NDARRAY,NREAD|NWRITE,darray_modified,NULL,0},
  {"get",NDFUNC,NREAD|NEXEC,darrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,darrayput,"id",0},
  {"add",NVFUNC,NREAD|NEXEC,darrayadd,"d",0},
  {"push",NVFUNC,NREAD|NEXEC,darrayadd,"d",0},
  {"pop",NDFUNC,NREAD|NEXEC,darraypop,"",0},
  {"ins",NVFUNC,NREAD|NEXEC,darrayins,"id",0},
  {"unshift",NVFUNC,NREAD|NEXEC,darrayunshift,"d",0},
  {"shift",NDFUNC,NREAD|NEXEC,darrayshift,"",0},
  {"del",NVFUNC,NREAD|NEXEC,darraydel,"i",0},
  {"join",NSFUNC,NREAD|NEXEC,darrayjoin,"s",0},
  {"sort",NVFUNC,NREAD|NEXEC,darraysort,"",0},
  {"rsort",NVFUNC,NREAD|NEXEC,darrayrsort,"",0},
  {"uniq",NVFUNC,NREAD|NEXEC,darrayuniq,"",0},
  {"sum", NDFUNC, NREAD|NEXEC, darray_sum, "", 0},
  {"average", NDFUNC, NREAD|NEXEC, darray_average, "", 0},
  {"sdev", NDFUNC, NREAD|NEXEC, darray_sdev, "", 0},
  {"RMS", NDFUNC, NREAD|NEXEC, darray_rms, "", 0},
  {"min", NDFUNC, NREAD|NEXEC, darray_min, "", 0},
  {"max", NDFUNC, NREAD|NEXEC, darray_max, "", 0},
  {"num", NIFUNC, NREAD|NEXEC, oarray_num, "", 0},
  {"seq", NSFUNC, NREAD|NEXEC, oarray_seq, "", 0},
  {"rseq", NSFUNC, NREAD|NEXEC, oarray_reverse_seq, "", 0},
  {"reverse", NVFUNC, NREAD|NEXEC, oarray_reverse, "", 0},
  {"slice", NVFUNC, NREAD|NEXEC, oarray_slice, "ii", 0},
  {"map", NVFUNC, NREAD|NEXEC, darray_map, "s", 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(odarray) / sizeof(*odarray))

void *
adddarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,odarray,ERRNUM,darrayerrorlist,NULL,NULL);
}
