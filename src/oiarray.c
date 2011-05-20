/* 
 * $Id: oiarray.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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
#include "mathfn.h"
#include "object.h"
#include "oiarray.h"

#define NAME "iarray"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *iarrayerrorlist[]={
""
};

#define ERRNUM (sizeof(iarrayerrorlist) / sizeof(*iarrayerrorlist))

static int 
iarrayinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
iarraydone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
iarrayget(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num,*po;

  num=*(int *)argv[2];
  _getobj(obj,"@",inst,&array);
  po=(int *)arraynget(array,num);
  if (po==NULL) return 1;
  rval->i=*po;
  return 0;
}

static int 
iarrayput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  int val;

  num=*(int *)argv[2];
  val=*(int *)argv[3];
  _getobj(obj,"@",inst,&array);
  if (arrayput(array,&val,num)==NULL) return 1;
  return 0;
}

struct narray *
oarray_get_array(struct objlist *obj, N_VALUE *inst, unsigned int size)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);
  if (array == NULL) {
    array = arraynew(size);
    if (array == NULL) {
      return NULL;
    }
    if (_putobj(obj, "@", inst, array)) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

static int 
iarrayadd(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int val;

  val=*(int *)argv[2];

  array = oarray_get_array(obj, inst, sizeof(int));
  if (array==NULL) {
      return 1;
  }

  if (arrayadd(array,&val)==NULL) return 1;
  return 0;
}

static int 
iarraypop(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int val, n;

  rval->i = 0;

  _getobj(obj,"@",inst,&array);
  if (array == NULL) {
    return 1;
  }

  n = arraynum(array) - 1;
  if (n < 0) {
    return 1;
  }

  val = arraynget_int(array, n);
  if (arrayndel(array, n) == NULL) {
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      return 1;
    }
  }

  rval->i = val;

  return 0;
}

static int 
iarrayins(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int num;
  int val;

  num=*(int *)argv[2];
  val=*(int *)argv[3];

  array = oarray_get_array(obj, inst, sizeof(int));
  if (array == NULL) {
      return 1;
  }

  if (arrayins(array, &val, num)==NULL) {
    return 1;
  }

  return 0;
}

static int 
iarrayunshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int val;

  val = * (int *) argv[2];

  array = oarray_get_array(obj, inst, sizeof(int));
  if (array == NULL) {
    return 1;
  }

  if (arrayins(array, &val, 0)==NULL) {
    return 1;
  }

  return 0;
}

static int 
iarrayshift(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int val;

  rval->i = 0;

  _getobj(obj,"@",inst,&array);
  if (array == NULL) {
    return 1;
  }

  val = arraynget_int(array, 0);

  if (arrayndel(array, 0) == NULL) {
    return 1;
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    if (_putobj(obj, "@", inst, NULL)) {
      return 1;
    }
  }

  rval->i = val;

  return 0;
}

static int 
iarraydel(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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

int 
oarray_num(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);
  rval->i = arraynum(array);

  return 0;
}

int 
oarray_seq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  GString *str;
  int i, n;

  g_free(rval->str);
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &array)) {
    return 1;
  }

  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    return 0;
  }

  for (i = 0; i < n; i++) {
    g_string_append_printf(str, "%d%s", i, (i == n - 1) ? "" : " ");
  }

  rval->str = g_string_free(str,  FALSE);

  return 0;
}

int 
oarray_reverse_seq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  GString *str;
  int i, n;

  g_free(rval->str);
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &array)) {
    return 1;
  }

  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    return 0;
  }

  for (i = 0; i < n; i++) {
    g_string_append_printf(str, "%d%s", n - i - 1, (i == n - 1) ? "" : " ");
  }

  rval->str = g_string_free(str,  FALSE);

  return 0;
}

static int 
iarraysort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arraysort_int(array);

  return 0;
}

static int 
iarrayrsort(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arrayrsort_int(array);

  return 0;
}

static int
iarrayuniq(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc, char **argv)
{
  struct narray *array;

  _getobj(obj, "@", inst, &array);

  arrayuniq_int(array);

  return 0;
}

static int 
iarrayjoin(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  GString *str;
  int i, n;
  char *sep, *ptr;
  int val;

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
    val = arraynget_int(array, i);
    g_string_append_printf(str, "%d%s", val, (i == n - 1) ? "" : sep);
  }

  rval->str = g_string_free(str, FALSE);

  g_free(sep);

  return 0;
}

int 
oarray_reverse(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;

  if (_getobj(obj, "@", inst, &array)) {
    return 1;
  }

  array_reverse(array);
  return 0;
}

int 
oarray_slice(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int start, len;

  start = * (int *) argv[2];
  len = * (int *) argv[3];

  if (_getobj(obj, "@", inst, &array)) {
    return 1;
  }

  if (array_slice(array, start, len) == NULL) {
    return 1;
  }

  return 0;
}

static double
calc_sum(const int *d, int n)
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
calc_square_sum(const int *d, int n)
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
iarray_sum(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n, *data;
  double val;

  rval->i = 0;

  _getobj(obj, "@", inst, &array);
  n = arraynum(array);
  if (n == 0) {
    return 0;
  }

  data = arraydata(array);

  val = calc_sum(data, n);
  if (val <= G_MININT || val >= G_MAXINT) {
    return 1;
  }

  rval->i = nround(val);

  return 0;
}

static int 
iarray_average(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n, *data;
  double val;

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
iarray_rms(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n, *data;
  double val;

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
iarray_sdev(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int n, *data;
  double sum, ssum, val;

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
iarray_min(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, n, *data, val;

  rval->i = 0;

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

  rval->i = val;

  return 0;
}

static int
iarray_max(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, n, *data, val;

  rval->i = 0;

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

  rval->i = val;

  return 0;
}

static struct objtable oiarray[] = {
  {"init",NVFUNC,NEXEC,iarrayinit,NULL,0},
  {"done",NVFUNC,NEXEC,iarraydone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"get",NIFUNC,NREAD|NEXEC,iarrayget,"i",0},
  {"put",NVFUNC,NREAD|NEXEC,iarrayput,"ii",0},
  {"add",NVFUNC,NREAD|NEXEC,iarrayadd,"i",0},
  {"push",NVFUNC,NREAD|NEXEC,iarrayadd,"i",0},
  {"pop",NIFUNC,NREAD|NEXEC,iarraypop,NULL,0},
  {"ins",NVFUNC,NREAD|NEXEC,iarrayins,"ii",0},
  {"unshift",NVFUNC,NREAD|NEXEC,iarrayunshift,"i",0},
  {"shift",NIFUNC,NREAD|NEXEC,iarrayshift,NULL,0},
  {"del",NVFUNC,NREAD|NEXEC,iarraydel,"i",0},
  {"join",NSFUNC,NREAD|NEXEC,iarrayjoin,"s",0},
  {"sort",NVFUNC,NREAD|NEXEC,iarraysort,NULL,0},
  {"rsort",NVFUNC,NREAD|NEXEC,iarrayrsort,NULL,0},
  {"uniq",NVFUNC,NREAD|NEXEC,iarrayuniq,NULL,0},
  {"sum", NIFUNC, NREAD|NEXEC, iarray_sum, NULL, 0},
  {"average", NDFUNC, NREAD|NEXEC, iarray_average, NULL, 0},
  {"sdev", NDFUNC, NREAD|NEXEC, iarray_sdev, NULL, 0},
  {"RMS", NDFUNC, NREAD|NEXEC, iarray_rms, NULL, 0},
  {"min", NIFUNC, NREAD|NEXEC, iarray_min, NULL, 0},
  {"max", NIFUNC, NREAD|NEXEC, iarray_max, NULL, 0},
  {"num", NIFUNC, NREAD|NEXEC, oarray_num, NULL, 0},
  {"seq", NSFUNC, NREAD|NEXEC, oarray_seq, NULL, 0},
  {"rseq", NSFUNC, NREAD|NEXEC, oarray_reverse_seq, NULL, 0},
  {"reverse", NVFUNC, NREAD|NEXEC, oarray_reverse, NULL, 0},
  {"slice", NVFUNC, NREAD|NEXEC, oarray_slice, "ii", 0},
};

#define TBLNUM (sizeof(oiarray) / sizeof(*oiarray))

void *
addiarray(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,oiarray,ERRNUM,iarrayerrorlist,NULL,NULL);
}
