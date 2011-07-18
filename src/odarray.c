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
#include "ngraph.h"
#include "object.h"
#include "oiarray.h"

#include "math/math_equation.h"

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
darrayget(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  double *po;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  num = oarray_get_index(array, num);
  if (num < 0) {
    return 1;
  }

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
  num = oarray_get_index(array, num);
  if (num < 0) {
    return 1;
  }
  if (arrayput(array,&val,num)==NULL) return 1;
  return 0;
}

static int 
darrayadd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  double val;

  val=*(double *)argv[2];

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

  num=*(int *)argv[2];
  val=*(double *)argv[3];

  array = oarray_get_array(obj, inst, sizeof(double));
  if (array==NULL) {
    return 1;
  }

  num = oarray_get_index(array, num);
  if (num < 0) {
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

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  if (array==NULL) return 1;

  num = oarray_get_index(array, num);
  if (num < 0) {
    return 1;
  }

  if (arrayndel(array,num)==NULL) return 1;
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
  double val;

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
    val = arraynget_double(array, i);
    g_string_append_printf(str, "%.15e%s", val, (i == n - 1) ? "" : sep);
  }

  rval->str = g_string_free(str, FALSE);

  g_free(sep);

  return 0;
}

static double
calc_sum(const double *d, int n)
{
  double sum;
  int i;

  sum = 0;

  for (i = 0; i < n; i++) {
    sum += d[i];
  }

  return sum;
}

static double
calc_square_sum(const double *d, int n)
{
  double sum;
  int i;

  sum = 0;

  for (i = 0; i < n; i++) {
    sum += d[i] * d[i];
  }

  return sum;
}

static int 
darray_sum(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n;
  double *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  data = arraydata(array);

  rval->d = calc_sum(data, n);

  return 0;
}

static int 
darray_average(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n;
  double val, *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  data = arraydata(array);

  val = calc_sum(data, n);
  rval->d = val / n;

  return 0;
}

static int
darray_rms(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n;
  double val, *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  data = arraydata(array);

  val = calc_square_sum(data, n);
  rval->d = sqrt(val / n);

  return 0;
}

static int
darray_sdev(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n;
  double sum, ssum, val, *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  data = arraydata(array);

  sum = calc_sum(data, n);
  ssum = calc_square_sum(data, n);

  sum /= n;
  val = ssum / n - sum * sum;

  rval->d = sqrt(val);

  return 0;
}

static int
darray_min(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, n;
  double val, *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 1;
  }

  data = arraydata(array);

  val = data[0];
  for (i = 1; i < n; i++) {
    if (data[i] < val) {
      val = data[i];
    }
  }

  rval->d = val;

  return 0;
}

static int
darray_max(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, n;
  double val, *data;

  rval->d = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 1;
  }

  data = arraydata(array);

  val = data[0];
  for (i = 1; i < n; i++) {
    if (data[i] > val) {
      val = data[i];
    }
  }

  rval->d = val;

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
  {"@",NDARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"get",NDFUNC,NREAD|NEXEC,darrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,darrayput,"id",0},
  {"add",NVFUNC,NREAD|NEXEC,darrayadd,"d",0},
  {"push",NVFUNC,NREAD|NEXEC,darrayadd,"d",0},
  {"pop",NDFUNC,NREAD|NEXEC,darraypop,NULL,0},
  {"ins",NVFUNC,NREAD|NEXEC,darrayins,"id",0},
  {"unshift",NVFUNC,NREAD|NEXEC,darrayunshift,"d",0},
  {"shift",NDFUNC,NREAD|NEXEC,darrayshift,NULL,0},
  {"del",NVFUNC,NREAD|NEXEC,darraydel,"i",0},
  {"join",NSFUNC,NREAD|NEXEC,darrayjoin,"s",0},
  {"sort",NVFUNC,NREAD|NEXEC,darraysort,NULL,0},
  {"rsort",NVFUNC,NREAD|NEXEC,darrayrsort,NULL,0},
  {"uniq",NVFUNC,NREAD|NEXEC,darrayuniq,NULL,0},
  {"sum", NDFUNC, NREAD|NEXEC, darray_sum, NULL, 0},
  {"average", NDFUNC, NREAD|NEXEC, darray_average, NULL, 0},
  {"sdev", NDFUNC, NREAD|NEXEC, darray_sdev, NULL, 0},
  {"RMS", NDFUNC, NREAD|NEXEC, darray_rms, NULL, 0},
  {"min", NDFUNC, NREAD|NEXEC, darray_min, NULL, 0},
  {"max", NDFUNC, NREAD|NEXEC, darray_max, NULL, 0},
  {"num", NIFUNC, NREAD|NEXEC, oarray_num, NULL, 0}, 
  {"seq", NSFUNC, NREAD|NEXEC, oarray_seq, NULL, 0}, 
  {"rseq", NSFUNC, NREAD|NEXEC, oarray_reverse_seq, NULL, 0}, 
  {"reverse", NVFUNC, NREAD|NEXEC, oarray_reverse, NULL, 0},
  {"slice", NVFUNC, NREAD|NEXEC, oarray_slice, "ii", 0},
  {"map", NVFUNC, NREAD|NEXEC, darray_map, "s", 0},
};

#define TBLNUM (sizeof(odarray) / sizeof(*odarray))

void *
adddarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,odarray,ERRNUM,darrayerrorlist,NULL,NULL);
}
