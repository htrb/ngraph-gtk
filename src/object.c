/*
 * $Id: object.c,v 1.54 2010-03-04 08:30:16 hito Exp $
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <glib.h>

#include "nstring.h"
#include "object.h"
#include "mathfn.h"
#include "nhash.h"
#include "shell.h"

#include <math.h>

#include "math/math_equation.h"

#define USE_HASH 1

#define INST_MAX 32767

static struct objlist *objroot=NULL;
static struct loopproc *looproot=NULL, *loopnext=NULL;

static struct objlist *errobj=NULL;

#define ERR_MSG_BUF_SIZE 2048
static char errormsg1[ERR_MSG_BUF_SIZE]={'\0'};
static char errormsg2[ERR_MSG_BUF_SIZE]={'\0'};
static int errcode=0;
int Globallock=FALSE;

int (*getstdin)(void);
int (*putstdout)(const char *s);
int (*putstderr)(const char *s);
int (*printfstdout)(const char *fmt,...);
int (*printfstderr)(const char *fmt,...);
int (*ninterrupt)(void);
int (*inputyn)(const char *mes);
void (*ndisplaydialog)(const char *str);
void (*ndisplaystatus)(const char *str);

#if USE_HASH
static NHASH ObjHash = NULL;
#endif

static char *errorlist[]={
     "",
     "no heap space.",
     "parent object not found",
     "duplicate field identifier",
     "object id is too large",
     "instance id is too large",
     "object not found",
     "field not found",
     "only one instance is allowed.",
     "unable to make an instance.",
     "instance not found, id",
     "instance not found, oid",
     "named instance not found",
     "not allowed to destruct.",
     "permission denied",
     "not defined current instance.",
     "instance does not exist.",
     "illegal object identifier",
     "illegal instance identifier",
     "illegal field identifier",
     "extra object arguments",
     "not enough object argument",
     "illegal type of object argument",
     "instance exist. cannot overwrite object",
};

#define ERRNUM (sizeof(errorlist) / sizeof(*errorlist))

static int chkobjtblpos(struct objlist *obj, const char *name, struct objlist **robj);

void
error(struct objlist *obj,int code)
{
  char *objname;
  char **errtable;
  int errnum;

  Globallock=TRUE;
  errobj=obj;
  errcode=code;

  if (code < 0) {
    code = ERRUNKNOWN;
  }

  if (obj==NULL) {
    objname="kernel";
  } else {
    objname=obj->name;
  }

  if (code==ERRUNKNOWN) {
    printfstderr("%.64s: %.256s%.256s\n",
                 objname,errormsg1,errormsg2);
  } else if (code<100) {
    errtable=errorlist;
    errnum=ERRNUM;
    if ((errtable==NULL) || (code>=errnum)) {
      printfstderr("%.64s: %.256s(%d)%.256s\n",objname,errormsg1,code,errormsg2);
    } else {
      printfstderr("%.64s: %.256s%.256s%.256s\n",
                   objname,errormsg1,errtable[code],errormsg2);
    }
  } else {
    if (obj) {
      errtable=obj->errtable;
      errnum=obj->errnum;
    } else {
      errtable = NULL;
      errnum = 0;
    }
    code=code-100;
    if ((errtable==NULL) || (code>=errnum)) {
      printfstderr("%.64s: %.256s(%d)%.256s\n",objname,errormsg1,code,errormsg2);
    } else {
      printfstderr("%.64s: %.256s%.256s%.256s\n",
                   objname,errormsg1,errtable[code],errormsg2);
    }
  }
  errormsg1[0]='\0';
  errormsg2[0]='\0';
  Globallock=FALSE;
}

static char *
get_localized_str(const char *str)
{
  char *local_str;
  if (g_utf8_validate(str, -1, NULL)) {
    local_str = g_locale_from_utf8(str, -1, NULL, NULL, NULL);
  } else {
    local_str = g_strdup(str);
  }
  return local_str;
}


void
error2(struct objlist *obj,int code, const char *mes)
{

  if (mes!=NULL) {
    char *local_msg;
    local_msg = get_localized_str(mes);
    snprintf(errormsg2, sizeof(errormsg2), " `%.256s'.", CHK_STR(local_msg));
    g_free(local_msg);
  } else {
    sprintf(errormsg2,".");
  }
  error(obj,code);
}

void
error22(struct objlist *obj,int code, const char *mes1, const char *mes2)
{
  char *local_msg;

  if (mes1!=NULL) {
    local_msg = get_localized_str(mes1);
    snprintf(errormsg1, sizeof(errormsg1), "%.256s: ", CHK_STR(local_msg));
    g_free(local_msg);
  } else {
    errormsg1[0]='\0';
  }
  if (mes2!=NULL) {
    local_msg = get_localized_str(mes2);
    snprintf(errormsg2, sizeof(errormsg2), " `%.256s'.", CHK_STR(local_msg));
    g_free(local_msg);
  } else {
    sprintf(errormsg2,".");
  }
  error(obj,code);
}

void
error3(struct objlist *obj,int code,int num)
{
  snprintf(errormsg2, sizeof(errormsg2), " `%d'.",num);
  error(obj,code);
}

static int
vgetchar(void)
{
  return EOF;
}

static int
vputs(const char *s)
{
  return 0;
}

static int
vnprintf(const char *fmt,...)
{
  return 0;
}

int
seputs(const char *s)
{
  return fputs(s,stderr);
}

int
seprintf(const char *fmt,...)
{
  int code;
  va_list ap;

  va_start(ap,fmt);
  code=vfprintf(stderr,fmt,ap);
  va_end(ap);
  return code;
}

int
vinterrupt(void)
{
  return FALSE;
}

int
vinputyn(const char *mes)
{
  return FALSE;
}

static void
vdisplaydialog(const char *str)
{
}

static void
vdisplaystatus(const char *str)
{
}

struct savedstdio stdiosave;

void
ignorestdio(struct savedstdio *save)
{
  if (save==NULL) savestdio(&stdiosave);
  else savestdio(save);
  getstdin=vgetchar;
  putstdout=vputs;
  putstderr=vputs;
  printfstdout=vnprintf;
  printfstderr=vnprintf;
  ndisplaydialog=vdisplaydialog;
  ndisplaystatus=vdisplaystatus;
}

void
restorestdio(struct savedstdio *save)
{
  if (save==NULL) loadstdio(&stdiosave);
  else loadstdio(save);
}

void
savestdio(struct savedstdio *save)
{
  save->getstdin=getstdin;
  save->putstdout=putstdout;
  save->putstderr=putstderr;
  save->printfstdout=printfstdout;
  save->printfstderr=printfstderr;
  save->ninterrupt=ninterrupt;
  save->inputyn=inputyn;
  save->ndisplaydialog=ndisplaydialog;
  save->ndisplaystatus=ndisplaystatus;
}

void
loadstdio(struct savedstdio *save)
{
  getstdin=save->getstdin;
  putstdout=save->putstdout;
  putstderr=save->putstderr;
  printfstdout=save->printfstdout;
  printfstderr=save->printfstderr;
  ninterrupt=save->ninterrupt;
  inputyn=save->inputyn;
  ndisplaydialog=save->ndisplaydialog;
  ndisplaystatus=save->ndisplaystatus;
}

#define ALLOCSIZE 32

void
arrayinit(struct narray *array,unsigned int base)
{
  if (array==NULL) return;
  array->base=base;
  array->num=0;
  array->size=0;
  array->data=NULL;
}

struct narray *
arraynew(unsigned int base)
{
  struct narray *array;

  if ((array=g_malloc(sizeof(struct narray)))==NULL) return NULL;
  arrayinit(array,base);
  return array;
}

void *
arraydata(struct narray *array)
{
  if (array==NULL) return NULL;
  return array->data;
}

unsigned int
arraynum(struct narray *array)
{
  if (array==NULL) return 0;
  return array->num;
}

static unsigned int
arraybase(struct narray *array)
{
  if (array==NULL) return 0;
  return array->base;
}

void
arraydel(struct narray *array)
{
  if (array==NULL) return;
  g_free(array->data);
  array->data=NULL;
  array->size=0;
  array->num=0;
}

void
arraydel2(struct narray *array)
{
  unsigned int i;
  char **data;

  if (array==NULL) return;
  data=array->data;
  for (i=0;i<array->num;i++) g_free(data[i]);
  g_free(array->data);
  array->data=NULL;
  array->size=0;
  array->num=0;
}

void
arrayclear(struct narray *array)
{
  if (array == NULL)
    return;

  array->num=0;
}

void
arrayclear2(struct narray *array)
{
  unsigned int i;
  char **data;

  if (array == NULL)
    return;

  data = array->data;
  for (i = 0; i < array->num; i++) {
    g_free(data[i]);
  }

  array->num=0;
}

int
arraycmp(struct narray *a, struct narray *b)
{
  if (a == NULL || b == NULL) {
    return 1;
  }
  if (a->base != b->base) {
    return 1;
  }
  if (a->num != b->num) {
    return 1;
  }
  if (a->data == NULL && b->data == NULL) {
    return 0;
  }
  if (a->data == NULL || b->data == NULL) {
    return 1;
  }
  return memcmp(a->data, b->data, a->base * a->num);
}

int
arraycpy(struct narray *dest, struct narray *src)
{
  void *data;
  if (dest == NULL || src == NULL) {
    return 1;
  }
  if (dest->base != src->base) {
    return 1;
  }
  if (src->num > dest->size) {
    data = g_realloc(dest->data, src->base * src->size);
    if (data == NULL) {
      return 1;
    }
    dest->data = data;
    dest->size = src->size;
  }
  memcpy(dest->data, src->data, src->num * src->base);
  dest->num = src->num;
  return 0;
}

struct narray *
arraydup(struct narray *array)
{
  struct narray *new_ary;
  if (array == NULL) {
    return NULL;
  }
  new_ary = g_malloc(sizeof(*new_ary));
  if (new_ary == NULL ) {
    return NULL;
  }
  *new_ary = *array;
  if (array->data) {
#if GLIB_CHECK_VERSION(2, 68, 0)
    new_ary->data = g_memdup2(array->data, array->base * array->size);
#else
    new_ary->data = g_memdup(array->data, array->base * array->size);
#endif
    if (new_ary->data == NULL) {
      g_free(new_ary);
      return NULL;
    }
  }
  return new_ary;
}

struct narray *
arraydup2(struct narray *array)
{
  struct narray *new_ary;
  char **data, **new_data;
  unsigned int i;
  new_ary = arraydup(array);
  if (new_ary == NULL ) {
    return NULL;
  }
  data = array->data;
  new_data = new_ary->data;
  for (i = 0; i < array->num; i++) {
    new_data[i] = g_strdup(data[i]);
  }
  return new_ary;
}

void
arrayfree(struct narray *array)
{
  if (array==NULL) return;
  g_free(array->data);
  g_free(array);
}

void
arrayfree2(struct narray *array)
{
  unsigned int i;
  char **data;

  if (array==NULL) return;
  data=array->data;
  for (i=0;i<array->num;i++) g_free(data[i]);
  g_free(array->data);
  g_free(array);
}

struct narray *
arrayadd(struct narray *array,const void *val)
{
  int base;
  char *data;

  if (array==NULL || val == NULL) return NULL;
  if (array->num==array->size) {
    int size;
    size=array->size+ALLOCSIZE;
    if ((data=g_realloc(array->data,array->base*size))==NULL) {
      return NULL;
    }
    array->size=size;
    array->data=data;
  } else data=array->data;
  base=array->base;
  memcpy(data+array->num*base,val,base);
  (array->num)++;
  return array;
}

struct narray *
arrayadd2(struct narray *array,const char *val)
{
  char **data;
  char *s;

  if (array == NULL)
    return NULL;
  if (val == NULL) {
    return NULL;
  } else {
    s = g_strdup(val);
    if (s == NULL) {
      arraydel2(array);
      return NULL;
    }
  }
  if (array->num == array->size) {
    int size;
    size = array->size+ALLOCSIZE;
    data=g_realloc(array->data,array->base*size);
    if (data == NULL) {
      return NULL;
    }
    array->size = size;
    array->data = data;
  } else {
    data = array->data;
  }
  data[array->num] = s;
  (array->num)++;
  return array;
}

struct narray *
arrayins(struct narray *array,const void *val,unsigned int idx)
{
  unsigned int i;
  int base;
  char *data;

  if (array==NULL) return NULL;
  if (idx>array->num) return NULL;
  if (array->num==array->size) {
    int size;
    size=array->size+ALLOCSIZE;
    if ((data=g_realloc(array->data,array->base*size))==NULL) {
      return NULL;
    }
    array->size=size;
    array->data=data;
  } else data=array->data;
  base=array->base;
  for (i=array->num;i>idx;i--)
    memcpy(data+i*base,data+(i-1)*base,base);
  memcpy(data+idx*base,val,base);
  (array->num)++;
  return array;
}

struct narray *
arrayins2(struct narray *array, const char *val, unsigned int idx)
{
  unsigned int i;
  char **data;
  char *s;

  if (array == NULL) {
    return NULL;
  }

  if (idx > array->num) {
    return NULL;
  }

  if (val == NULL) {
    return NULL;
  }

  s = g_strdup(val);
  if (s == NULL) {
    arraydel2(array);
    return NULL;
  }

  if (array->num == array->size) {
    int size;
    size = array->size+ALLOCSIZE;
    data = g_realloc(array->data,array->base*size);
    if (data == NULL) {
      return NULL;
    }
    array->size=size;
    array->data=data;
  } else {
    data = array->data;
  }
  for (i = array->num; i > idx; i--) {
    data[i] = data[i-1];
  }
  data[idx] = s;
  (array->num)++;
  return array;
}

struct narray *
array_reverse(struct narray *array)
{
  unsigned int i, base, num, n;
  char *data, *buf;

  if (array == NULL) {
    return NULL;
  }

  data = array->data;
  base = array->base;
  num = array->num;
  n = num / 2;
  num--;

  buf = g_malloc(base);
  if (buf == NULL) {
    return NULL;
  }

  for (i = 0; i < n; i++) {
    memcpy(buf, data + i * base, base);
    memcpy(data + i * base, data + (num - i) * base, base);
    memcpy(data + (num - i) * base, buf, base);
  }

  g_free(buf);

  return array;
}

struct narray *
arrayndel(struct narray *array,unsigned int idx)
{
  int base;
  char *data;

  if (array==NULL) return NULL;
  if (idx>=array->num) return NULL;
  data=array->data;
  base=array->base;
#if 0
  for (i=idx+1;i<array->num;i++)
    memcpy(data+(i-1)*base,data+i*base,base);
#else
  data += (idx * base);
  memmove(data, data + base, base * (array->num - idx - 1));
#endif
  (array->num)--;
  return array;
}

struct narray *
array_slice(struct narray *array, int start, int length)
{
  int base, num;
  char *data;

  if (array == NULL) {
    return NULL;
  }

  data = array->data;
  base = array->base;
  num = array->num;

  if (length < 0) {
    return NULL;
  }

  if (start < 0) {
    start = num + start;
  }

  if (start < 0) {
    return NULL;
  }

  if (start >= num) {
    return NULL;
  }

  if (start + length > num) {
    length = num - start;
  }

  if (length > 0) {
    memmove(data, data + start * base, length * base);
  }

  array->num = length;

  return array;
}

struct narray *
array_slice2(struct narray *array, int start, int length)
{
  int i, base, num;
  char *data, **sarray;

  if (array == NULL) {
    return NULL;
  }

  data = array->data;
  sarray = array->data;
  base = array->base;
  num = array->num;

  if (length < 0) {
    return NULL;
  }

  if (start < 0) {
    start = num + start;
  }

  if (start < 0) {
    return NULL;
  }

  if (start >= num) {
    return NULL;
  }

  if (start + length > num) {
    length = num - start;
  }

  for (i = 0; i < start; i++) {
    g_free(sarray[i]);
  }

  for (i = start + length; i < num; i++) {
    g_free(sarray[i]);
  }

  if (length > 0) {
    memmove(data, data + start * base, length * base);
  }

  array->num = length;

  return array;
}

struct narray *
arrayndel2(struct narray *array,unsigned int idx)
{
  char **data;

  if (array==NULL) return NULL;
  if (idx>=array->num) return NULL;
  data=(char **)array->data;
  g_free(data[idx]);
#if 0
  for (i=idx+1;i<array->num;i++)
    data[i-1]=data[i];
#else
  data += idx;
  memmove(data, data + 1, sizeof(*data) * (array->num - idx - 1));
#endif
  (array->num)--;
  return array;
}

struct narray *
arrayput(struct narray *array,const void *val,unsigned int idx)
{
  int base;
  char *data;

  if (array==NULL) return NULL;
  if (idx>=array->num) return NULL;
  data=array->data;
  base=array->base;
  memcpy(data+idx*base,val,base);
  return array;
}

struct narray *
arrayput2(struct narray *array, const char *val, unsigned int idx)
{
  char *s;
  char **data;

  if (array == NULL) {
    return NULL;
  }

  if (idx >= array->num) {
    return NULL;
  }
  if (val == NULL){
    return NULL;
  }

  s = g_strdup(val);
  if (s == NULL) {
    arraydel2(array);
    return NULL;
  }

  data = (char **)array->data;
  g_free(data[idx]);
  data[idx] = s;
  return array;
}

void *
arraynget(struct narray *array,unsigned int idx)
{
  int base;
  char *data;

  if (array==NULL) return NULL;
  if (idx>=array->num) return NULL;
  data=array->data;
  base=array->base;
  return data+idx*base;
}

int
arraynget_int(struct narray *array, unsigned int idx)
{
  void *ptr;

  ptr = arraynget(array, idx);
  return (ptr) ? * (int *) ptr : 0;
}

double
arraynget_double(struct narray *array, unsigned int idx)
{
  void *ptr;

  ptr = arraynget(array, idx);
  return (ptr) ? * (double *) ptr : 0;
}

char *
arraynget_str(struct narray *array, unsigned int idx)
{
  void *ptr;

  ptr = arraynget(array, idx);
  return (ptr) ? * (char **) ptr : NULL;
}

void *
arraylast(struct narray *array)
{
  int base;
  char *data;

  if (array==NULL) return NULL;
  if (array->num==0) return NULL;
  data=array->data;
  base=array->base;
  return data+(array->num-1)*base;
}

int
arraylast_int(struct narray *array)
{
  void *ptr;

  ptr = arraylast(array);
  return (ptr) ? * (int *) ptr : 0;
}

static int
cmp_func_int(const void *p1, const void *p2)
{
  return (* (int *) p1) - (* (int *) p2);
}

static int
cmp_func_int_r(const void *p1, const void *p2)
{
  return (* (int *) p2) - (* (int *) p1);
}

void
arraysort_int(struct narray *array)
{
  int num, *adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(int), cmp_func_int);
  }
}

void
arrayrsort_int(struct narray *array)
{
  int num, *adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(int), cmp_func_int_r);
  }
}

void
arrayuniq_int(struct narray *array)
{
  int i, val, num, *adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  if (num < 2) {
    return;
  }

  adata = arraydata(array);
  val = adata[0];
  for (i = 1; i < num;) {
    if (adata[i] == val) {
      arrayndel(array, i);
      num--;
    } else {
      val = adata[i];
      i++;
    }
  }
}

static int
cmp_func_double(const void *p1, const void *p2)
{
  double d1, d2;

  d1 = * (double *) p1;
  d2 = * (double *) p2;

  if (d1 > d2) {
    return 1;
  } else if (d1 < d2) {
    return -1;
  }

  return 0;
}

static int
cmp_func_double_r(const void *p1, const void *p2)
{
  double d1, d2;

  d1 = * (double *) p1;
  d2 = * (double *) p2;

  if (d1 > d2) {
    return -1;
  } else if (d1 < d2) {
    return 1;
  }

  return 0;
}

void
arraysort_double(struct narray *array)
{
  int num;
  double *adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(double), cmp_func_double);
  }
}

void
arrayrsort_double(struct narray *array)
{
  int num;
  double *adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(double), cmp_func_double_r);
  }
}

void
arrayuniq_double(struct narray *array)
{
  int i, num;
  double *adata, val;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  if (num < 2) {
    return;
  }

  adata = arraydata(array);
  val = adata[0];
  for (i = 1; i < num;) {
    if (adata[i] == val) {
      arrayndel(array, i);
      num--;
    } else {
      val = adata[i];
      i++;
    }
  }
}

static int
cmp_func_str(const void *p1, const void *p2)
{
  return g_strcmp0(* (char **) p1, * (char **) p2);
}

static int
cmp_func_str_r(const void *p1, const void *p2)
{
  return - g_strcmp0(* (char **) p1, * (char **) p2);
}

void
arraysort_str(struct narray *array)
{
  int num;
  char **adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(char *), cmp_func_str);
  }
}

void
arrayrsort_str(struct narray *array)
{
  int num;
  char **adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1) {
    qsort(adata, num, sizeof(char *), cmp_func_str_r);
  }
}

void
arrayuniq_str(struct narray *array)
{
  int i, num;
  char **adata, *val;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  if (num < 2) {
    return;
  }

  adata = arraydata(array);
  val = adata[0];
  for (i = 1; i < num;) {
    if (g_strcmp0(adata[i], val) == 0) {
      arrayndel2(array, i);
      num--;
    } else {
      val = adata[i];
      i++;
    }
  }
}

void
arrayuniq_all_str(struct narray *array)
{
  int i, j, num;
  char **adata;

  if (array == NULL) {
    return;
  }

  num = arraynum(array);
  if (num < 2) {
    return;
  }

  adata = arraydata(array);
  for (j = 0; j < num - 1; j++) {
    char *val;
    val = adata[j];
    for (i = j + 1; i < num;) {
      if (g_strcmp0(adata[i], val) == 0) {
	arrayndel2(array, i);
	num--;
      } else {
	i++;
      }
    }
  }
}

#define ARGBUFNUM 32

int
getargc(char **arg)
{
  int i;

  if (arg==NULL) return 0;
  for (i=0;arg[i]!=NULL;i++) ;
  return i;
}

char **
arg_add(char ***arg,void *ptr)
{
  int i,num;
  char **arg2;

  if (*arg==NULL) {
    if ((*arg=g_malloc(ARGBUFNUM*sizeof(void *)))==NULL)
      return NULL;
    (*arg)[0]=NULL;
  }
  i=getargc(*arg);
  num=i/ARGBUFNUM;
  if (i%ARGBUFNUM==ARGBUFNUM-1) {
    if ((arg2=g_realloc(*arg,ARGBUFNUM*sizeof(void *)*(num+2)))==NULL)
      return NULL;
    *arg=arg2;
  }
  (*arg)[i]=ptr;
  (*arg)[i+1]=NULL;
  return *arg;
}

static char **
arg_add2(char ***arg,int argc,...)
{
  va_list ap;
  int i;

  if (*arg==NULL) {
    if ((*arg=g_malloc(ARGBUFNUM*sizeof(void *)))==NULL)
      return NULL;
    (*arg)[0]=NULL;
  }
  va_start(ap,argc);
  for (i=0;i<argc;i++)
    if (arg_add(arg,va_arg(ap,void *))==NULL) return NULL;
  va_end(ap);
  return *arg;
}

void
arg_del(char **arg)
{
  int i,argc;

  if (arg==NULL) return;
  argc=getargc(arg);
  for (i=0;i<argc;i++) g_free(arg[i]);
  g_free(arg);
}

void
registerevloop(const char *objname, const char *evname,
                    struct objlist *obj,int idn,N_VALUE *inst,
                    void *local)
{
  struct loopproc *lpcur,*lpnew;

  if (obj==NULL) return;
  if ((lpnew=g_malloc(sizeof(struct loopproc)))==NULL) return;
  lpcur=looproot;
  if (lpcur==NULL) looproot=lpnew;
  else {
    while (lpcur->next!=NULL) lpcur=lpcur->next;
    lpcur->next=lpnew;
  }
  lpnew->next=NULL;
  lpnew->objname=objname;
  lpnew->evname=evname;
  lpnew->obj=obj;
  lpnew->idn=idn;
  lpnew->inst=inst;
  lpnew->local=local;
}

void
unregisterevloop(struct objlist *obj,int idn,N_VALUE *inst)
{
  struct loopproc *lpcur,*lpdel,*lpprev;

  lpcur=looproot;
  lpprev=NULL;
  while (lpcur!=NULL) {
    if ((lpcur->obj==obj) && (lpcur->idn==idn) && (lpcur->inst==inst)) {
      lpdel=lpcur;
      if (loopnext==lpdel) loopnext=lpdel->next;
      if (lpprev==NULL) looproot=lpcur->next;
      else lpprev->next=lpdel->next;
      lpcur=lpcur->next;
      g_free(lpdel);
    } else {
      lpprev=lpcur;
      lpcur=lpcur->next;
    }
  }
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void
unregisterallevloop(void)
{
  struct loopproc *lpcur;

  lpcur=looproot;
  while (lpcur!=NULL) {
    struct loopproc *lpdel;
    lpdel=lpcur;
    lpcur=lpcur->next;
    g_free(lpdel);
  }
  looproot=NULL;
  loopnext=NULL;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

int
has_eventloop(void) {
  return looproot != NULL;
}

void
eventloop(void)
{
  static int ineventloop = FALSE;
  struct loopproc *lpcur;
  char *argv[4];

  if (looproot==NULL) return;
  if (ineventloop) return;
  ineventloop=TRUE;
  ignorestdio(NULL);
  lpcur=looproot;
  while (lpcur!=NULL) {
    argv[0]= (char *) lpcur->objname;
    argv[1]= (char *) lpcur->evname;
    argv[2]=lpcur->local;
    argv[3]=NULL;
    loopnext=lpcur->next;
    __exeobj(lpcur->obj,lpcur->idn,lpcur->inst,3,argv);
    lpcur=loopnext;
  }
  restorestdio(NULL);
  ineventloop=FALSE;
}

struct objlist *
chkobjroot()
{
  return objroot;
}

#if USE_HASH
static int
add_obj_to_hash(char *name, char *alias, void *obj)
{
  if (ObjHash == NULL) {
    ObjHash = nhash_new();
    if (ObjHash == NULL)
      return 1;
  }

  if (nhash_set_ptr(ObjHash, name, obj))
    return 1;

  if (alias) {
    char *s, *aliasname;
    int len;

    s = alias;
    while ((aliasname = getitok2(&s, &len, ":"))) {
      if (nhash_set_ptr(ObjHash, aliasname, obj)) {
	g_free(aliasname);
	return 1;
      }
      g_free(aliasname);
    }
  }

  return 0;
}
#endif

static int
check_arglist(int type, const char *arglist)
{
  int i;

  if (type == NENUM) {
    return (arglist) ? 0 : 1;
  }

  if (arglist == NULL || arglist[0] == '\0') {
    return 0;
  }

  if (arglist[1] == 'a') {
    if (arglist[0] != 's' &&
	arglist[0] != 'i' &&
	arglist[0] != 'd' &&
	arglist[2] != '\0') {
      return 1;
    }
    return 0;
  }

  for (i = 0; arglist[i]; i++) {
    if (strchr("soidb", arglist[i]) == NULL) {
      return 1;
    }
  }

  return 0;
}

void *
addobject(char *name,char *alias,char *parentname,char *ver,
                int tblnum,struct objtable *table,
                int errnum,char **errtable,void *local,DoneProc doneproc)
/* addobject() returns NULL on error */
{
  struct objlist *objcur,*objprev,*objnew,*parent, *ptr;
  int i,offset;
  NHASH tbl_hash = NULL;
  static int id = 1;

  if (id >= OBJ_MAX) {
    error3(NULL, ERROBJNUM, id);
    return NULL;
  }

  objcur = chkobject(name);
  if (objcur) {
    error2(NULL,ERROVERWRITE,name);
    return NULL;
  }
  if (parentname==NULL) parent=NULL;
  else if ((parent=chkobject(parentname))==NULL) {
    error2(NULL,ERRPARENT,parentname);
    return NULL;
  }
  if ((objnew=g_malloc(sizeof(struct objlist)))==NULL) return NULL;

#if USE_HASH
  if (add_obj_to_hash(name, alias, objnew)) {
    g_free(objnew);
    error2(NULL,ERRHEAP,name);
    return NULL;
  }

  tbl_hash = nhash_new();
  if (tbl_hash == NULL) {
    g_free(objnew);
    error2(NULL,ERRHEAP,name);
    return NULL;
  }
#endif

  objprev = NULL;
  if (parent) {
    if (parent->child) {
      ptr = parent->child;
      while (ptr && chkobjparent(ptr) == parent) {
	objprev = ptr;
 	ptr = ptr->next;
      }
      while (objprev->child) {
	ptr = objprev->child;
	while (ptr) {
	  objprev = ptr;
	  ptr = objprev->next;
	}
      }
    } else {
      objprev = parent;
      parent->child = objnew;
    }
  }

  if (objprev == NULL) {
    objroot=objnew;
    objnew->next = NULL;
  } else if (objprev->next) {
    objnew->next = objprev->next;
    objprev->next = objnew;
  } else {
    objprev->next = objnew;
    objnew->next = NULL;
  }
  objnew->id=id;
  objnew->curinst=-1;
  objnew->lastinst=-1;
  objnew->lastinst2=-1;
  objnew->lastoid=INT_MAX;
  objnew->name=name;
  objnew->alias=alias;
  objnew->ver=ver;
  objnew->tblnum=tblnum;
  objnew->fieldnum=-1;
  objnew->table=table;
#if USE_HASH
  objnew->table_hash=tbl_hash;
#endif
  objnew->errnum=errnum;
  objnew->errtable=errtable;
  objnew->parent=parent;
  objnew->child=NULL;
  objnew->root=NULL;
  objnew->root2=NULL;
  objnew->undo = NULL;
  objnew->redo = NULL;
  objnew->local=local;
  objnew->doneproc=doneproc;
  objnew->dup_func = NULL;
  objnew->free_func = NULL;
  if (parent==NULL) offset=0;
  else offset=parent->size;
  for (i=0;i<tblnum;i++) {
    table[i].offset=offset;
    switch (table[i].type) {
    case NVOID:
#if USE_LABEL
    case NLABEL:
#endif
    case NVFUNC:
      break;
    default:
      offset++;
    }
    if (check_arglist(table[i].type, table[i].arglist)) {
      goto errexit;
    }
    if (table[i].attrib & NEXEC) {
      table[i].attrib &= ~NWRITE;
    }
#if USE_HASH
    if (nhash_set_int(tbl_hash, table[i].name, i)) {
      goto errexit;
    }
#endif
  }
  objnew->size=offset;
  objnew->idp=chkobjoffset(objnew,"id");
  objnew->oidp=chkobjoffset(objnew,"oid");
  objnew->nextp=chkobjoffset(objnew,"next");

  id++;

  return objnew;

 errexit:
  g_free(objnew);
  nhash_free(tbl_hash);
  error2(NULL,ERRHEAP,name);
  return NULL;
}

void
obj_set_undo_func(struct objlist *obj, UNDO_DUP_FUNC dup_func, UNDO_FREE_FUNC free_func)
{
  if (obj == NULL) {
    return;
  }
  obj->dup_func = dup_func;
  obj->free_func = free_func;
}

void
hideinstance(struct objlist *obj)
{
  N_VALUE *instcur;
  int nextp,idp;

  if ((idp=obj->idp)==-1) return;
  if ((nextp=obj->nextp)==-1) return;
  if (obj->lastinst==-1) return;
  if (obj->lastinst2==-1) {
    obj->root2=obj->root;
    obj->lastinst2=obj->lastinst;
  } else {
    N_VALUE *instprev;
    instcur=obj->root;
    while (instcur!=NULL) {
      instcur[idp].i+=obj->lastinst2+1;
      instcur=instcur[nextp].inst;
    }
    instcur=obj->root2;
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=instcur[nextp].inst;
    }
    instprev[nextp].inst=obj->root;
    obj->lastinst2+=obj->lastinst+1;
  }
  obj->root=NULL;
  obj->lastinst=-1;
}

void
recoverinstance(struct objlist *obj)
{
  N_VALUE *instcur;
  int nextp,idp;

  if ((idp=obj->idp)==-1) return;
  if ((nextp=obj->nextp)==-1) return;
  if (obj->lastinst2==-1) return;
  if (obj->lastinst==-1) {
    obj->root=obj->root2;
    obj->lastinst=obj->lastinst2;
  } else {
    N_VALUE *instprev;
    instcur=obj->root;
    while (instcur!=NULL) {
      instcur[idp].i+=obj->lastinst2+1;
      instcur=instcur[nextp].inst;
    }
    instcur=obj->root2;
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=instcur[nextp].inst;
    }
    instprev[nextp].inst=obj->root;
    obj->root=obj->root2;
    obj->lastinst+=obj->lastinst2+1;
  }
  obj->root2=NULL;
  obj->lastinst2=-1;
}

int
obj_get_field_pos(struct objlist *obj, const char *field)
{
  int idn;
  struct objlist *robj;
  idn = getobjtblpos(obj, field, &robj);
  if (idn == -1) {
      return -1;
  }
  return chkobjoffset2(robj, idn);
}

static N_VALUE *
dup_inst(struct objlist *obj, N_VALUE *inst)
{
  N_VALUE *inst_new;
  int i, n;
  struct objlist *robj;
  enum ngraph_object_field_type type;
  inst_new = g_memdup(inst, obj->size * sizeof(N_VALUE));
  if (inst_new == NULL) {
    return NULL;
  }

  if (obj->dup_func) {
    obj->dup_func(obj, inst, inst_new);
  }
  n = chkobjfieldnum(obj);
  for (i = 0; i < n; i++) {
    int j, idn;
    const char *field;
    field = chkobjfieldname(obj, i);
    idn = getobjtblpos(obj, field, &robj);
    if (idn == -1) {
      return NULL;
    }
    j = chkobjoffset2(robj, idn);
    type = robj->table[idn].type;
    switch (type) {
    case NVOID:
#if USE_LABEL
    case NLABEL:
#endif
    case NVFUNC:
      break;
    case NPOINTER:
      /* _local data is copied by obj->dup_func(). */
      break;
    case NIARRAY:
    case NDARRAY:
    case NIAFUNC:
    case NDAFUNC:
      inst_new[j].array = arraydup(inst[j].array);
      break;
    case NSARRAY:
    case NSAFUNC:
      inst_new[j].array = arraydup2(inst[j].array);
      break;
    case NSTR:
    case NOBJ:
    case NSFUNC:
      inst_new[j].str = g_strdup(inst[j].str); /* If str is NULL g_strdup(str) returns NULL */
      break;
    default:
      break;
    }
  }
  return inst_new;
}

static N_VALUE *
dup_inst_list(struct objlist *obj)
{
  N_VALUE *inst_new, *inst_prev, *inst, *root;
  int nextp;

  if (obj->lastinst == -1) {
    return NULL;
  }

  nextp = obj->nextp;
  inst_prev = NULL;
  root = NULL;
  for (inst = obj->root; inst; inst = inst[nextp].inst) {
    inst_new = dup_inst(obj, inst);
    if (inst_new == NULL) {
      return NULL;		/* don't care about the memory leak. */
    }
    if (root == NULL) {
      root = inst_new;
    }
    if (inst_prev) {
      inst_prev[nextp].inst = inst_new;
    }
    inst_prev = inst_new;
  }
  return root;
}

static void
free_inst(struct objlist *obj, N_VALUE *inst)
{
  int i, n;
  struct objlist *robj;
  enum ngraph_object_field_type type;

  if (inst == NULL) {
    return;
  }

  if (obj->free_func) {
    obj->free_func(obj, inst);
  }
  n = chkobjfieldnum(obj);
  for (i = 0; i < n; i++) {
    int j, idn;
    const char *field;
    field = chkobjfieldname(obj, i);
    idn = getobjtblpos(obj, field, &robj);
    if (idn == -1) {
      return;
    }
    j = chkobjoffset2(robj, idn);
    type = robj->table[idn].type;
    switch (type) {
    case NVOID:
#if USE_LABEL
    case NLABEL:
#endif
    case NVFUNC:
      break;
    case NPOINTER:
      /* _local data is freed by obj->free_func(). */
      break;
    case NIARRAY:
    case NDARRAY:
    case NIAFUNC:
    case NDAFUNC:
      arrayfree(inst[j].array);
      break;
    case NSARRAY:
    case NSAFUNC:
      arrayfree2(inst[j].array);
      break;
    case NSTR:
    case NOBJ:
    case NSFUNC:
      g_free(inst[j].str);
      break;
    default:
      break;
    }
  }
  return;
}

static void
free_inst_list(struct objlist *obj, N_VALUE *inst)
{
  N_VALUE *next;
  int nextp;

  nextp = obj->nextp;
  while (inst) {
    next = inst[nextp].inst;
    free_inst(obj, inst);
    g_free(inst);
    inst = next;
  }
}

static void
free_undo_inst(struct objlist *obj, struct undo_inst *cur)
{
  struct undo_inst *next;
  while (cur) {
    free_inst_list(obj, cur->inst);
    next = cur->next;
    g_free(cur);
    cur = next;
  }
}

static void
undo_clear_redo(struct objlist *obj)
{
  free_undo_inst(obj, obj->redo);
  obj->redo = NULL;
}

int
undo_clear(struct objlist *obj)
{
  undo_clear_redo(obj);
  free_undo_inst(obj, obj->undo);
  obj->undo = NULL;
  return 0;
}

int
undo_save(struct objlist *obj)
{
  struct undo_inst *inst;

  if (obj == NULL) {
    return 1;
  }
  undo_clear_redo(obj);

  if (obj->idp == -1) {
    return 1;
  }
  if (obj->nextp == -1) {
    return 1;
  }
  if (obj->lastinst2 != -1) {
    return 1;
  }

  inst = g_malloc(sizeof(*inst));
  inst->lastinst = obj->lastinst;
  inst->lastinst2 = obj->lastinst2;
  inst->curinst = obj->curinst;
  inst->lastoid = obj->lastoid;

  inst->inst = dup_inst_list(obj);
  undo_clear_redo(obj);

  inst->next = obj->undo;
  obj->undo = inst;
  return 0;
}

int
undo_undo(struct objlist *obj)
{
  int lastoid, lastinst2, curinst, lastinst;
  N_VALUE *inst;
  struct undo_inst *undo;
  undo = obj->undo;
  if (undo == NULL) {
    return 1;
  }
  lastinst = obj->lastinst;
  lastinst2 = obj->lastinst2;
  curinst = obj->curinst;
  lastoid = obj->lastoid;
  inst = obj->root;

  obj->lastinst = undo->lastinst;
  obj->lastinst2 = undo->lastinst2;
  obj->curinst = undo->curinst;
  obj->lastoid = undo->lastoid;
  obj->root = undo->inst;
  obj->undo = undo->next;

  undo->lastinst = lastinst;
  undo->lastinst2 = lastinst2;
  undo->curinst = curinst;
  undo->lastoid = lastoid;
  undo->inst = inst;
  undo->next = obj->redo;
  obj->redo = undo;
  return 0;
}

int
undo_delete(struct objlist *obj)
{
  struct undo_inst *undo;
  undo = obj->undo;
  if (undo == NULL) {
    return 1;
  }
  obj->undo = undo->next;
  undo->next = NULL;
  free_undo_inst(obj, undo);
  return 0;
}

int
undo_check_undo(struct objlist *obj)
{
  return (obj->undo) ? 1 : 0;

}

int
undo_check_redo(struct objlist *obj)
{
  return (obj->redo) ? 1 : 0;

}

int
undo_redo(struct objlist *obj)
{
  int lastoid, lastinst2, curinst, lastinst;
  N_VALUE *inst;
  struct undo_inst *redo;
  redo = obj->redo;
  if (redo == NULL) {
    return 1;
  }
  lastinst = obj->lastinst;
  lastinst2 = obj->lastinst2;
  curinst = obj->curinst;
  lastoid = obj->lastoid;
  inst = obj->root;

  obj->lastinst = redo->lastinst;
  obj->lastinst2 = redo->lastinst2;
  obj->curinst = redo->curinst;
  obj->lastoid = redo->lastoid;
  obj->root = redo->inst;
  obj->redo = redo->next;

  redo->lastinst = lastinst;
  redo->lastinst2 = lastinst2;
  redo->curinst = curinst;
  redo->lastoid = lastoid;
  redo->inst = inst;
  redo->next = obj->undo;
  obj->undo = redo;
  return 0;
}

struct objlist *
chkobject(const char *name)
/* chkobject() returns NULL when the named object is not found */
{
#if USE_HASH
  struct objlist *obj;
  int r;

  if (ObjHash == NULL)
    return NULL;

  r = nhash_get_ptr(ObjHash, name, (void **) &obj);
  if (r)
    return NULL;

  return obj;
#else
  struct objlist *obj,*objcur;
  char *s,*aliasname;
  int len;

  objcur=objroot;
  obj=NULL;
  while (objcur!=NULL) {
    if (strcmp0(objcur->name,name)==0) obj=objcur;
    if (objcur->alias!=NULL) {
      s=objcur->alias;
      while ((aliasname=getitok(&s,&len,":"))!=NULL) {
        if (strncmp(aliasname,name,len)==0) obj=objcur;
      }
    }
    objcur=objcur->next;
  }
  return obj;
#endif
}

const char *
chkobjectname(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->name;
}

const char *
chkobjectalias(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->alias;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void *
chkobjectlocal(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->local;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

int
chkobjectid(struct objlist *obj)
{
  if (obj==NULL) return -1;
  return obj->id;
}

char *
chkobjver(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->ver;
}

struct objlist *
chkobjparent(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->parent;
}

int
chkobjchild(struct objlist *parent,struct objlist *child)
{
  struct objlist *p;

  p=child;
  do {
    if (p==parent) return TRUE;
    p=chkobjparent(p);
  } while (p!=NULL);
  return FALSE;
}

int
chkobjsize(struct objlist *obj)
{
  if (obj==NULL) return 0;
  return obj->size * sizeof(N_VALUE);
}

int
chkobjlastinst(struct objlist *obj)
{
  if (obj==NULL) return -1;
  return obj->lastinst;
}

int
chkobjcurinst(struct objlist *obj)
{
  if (obj==NULL) return -1;
  return obj->curinst;
}

int
chkobjoffset(struct objlist *obj, const char *name)
/* chkobjoffset() returns -1 on error */
{
#if USE_HASH
  struct objlist *objcur;
  int i;

  i = chkobjtblpos(obj, name, &objcur);
  if (i < 0) return -1;

  return objcur->table[i].offset;
#else
  struct objlist *objcur;
  int i;

  if (obj==NULL) return -1;
  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++)
      if (strcmp0(objcur->table[i].name,name)==0)
        return objcur->table[i].offset;
    objcur=objcur->parent;
  }
  return -1;
#endif
}

int
chkobjoffset2(struct objlist *obj,int tblpos)
{
  return obj->table[tblpos].offset;
}

N_VALUE *
chkobjinstoid(struct objlist *obj,int oid)
/* chkobjinstoid() returns NULL when instance is not found */
{
  int oidp,nextp;
  N_VALUE *inst;

  if ((oidp=obj->oidp)==-1) return NULL;
  inst=obj->root;
  if ((nextp=obj->nextp)==-1) {
    if (inst==NULL) return NULL;
    if (inst[oidp].i==oid) return inst;
    else return NULL;
  } else {
    while (inst!=NULL) {
      if (inst[oidp].i==oid) return inst;
      inst=inst[nextp].inst;
    }
  }
  return NULL;
}

static int
chkobjtblpos(struct objlist *obj, const char *name, struct objlist **robj)
/* chkobjtblpos() returns -1 on error */
{
#if USE_HASH
  struct objlist *objcur;
  int i;

  objcur = obj;
  while (objcur) {
    if (nhash_get_int(objcur->table_hash, name, &i) == 0) {
      *robj = objcur;
      return i;
    }
    objcur = objcur->parent;
  }
  *robj = NULL;
  return -1;
#else
  struct objlist *objcur;
  int i;

  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++)
      if (strcmp0(objcur->table[i].name,name)==0) {
        *robj=objcur;
        return i;
      }
    objcur=objcur->parent;
  }
  *robj=NULL;
  return -1;
#endif
}

N_VALUE *
chkobjinst(struct objlist *obj,int id)
/* chkobjinst() returns NULL if instance is not found */
{
  int i,nextp;
  N_VALUE *instcur;

  instcur=obj->root;
  i=0;
  if ((nextp=obj->nextp)==-1) {
    if ((instcur==NULL) || (id!=0)) {
      return NULL;
    }
  } else {
    while ((instcur!=NULL) && (id!=i)) {
      instcur=instcur[nextp].inst;
      i++;
    }
    if (instcur==NULL) {
      return NULL;
    }
  }
  return instcur;
}

N_VALUE *
chkobjlast(struct objlist *obj)
/* chkobjlast() returns NULL if instance is not found */
{
  N_VALUE *instcur,*instprev;
  int nextp;

  instcur=obj->root;
  nextp=obj->nextp;
  if (nextp!=-1) {
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=instcur[nextp].inst;
    }
    instcur=instprev;
  }
  return instcur;
}

static N_VALUE *
chkobjprev(struct objlist *obj,int id,N_VALUE **inst,N_VALUE **prev)
/* chkobjprev() returns NULL if instance is not found */
{
  N_VALUE *instcur,*instprev;
  int i,nextp;

  if (inst!=NULL) *inst=NULL;
  if (prev!=NULL) *prev=NULL;
  instcur=obj->root;
  instprev=NULL;
  i=0;
  if ((nextp=obj->nextp)==-1) {
    if ((instcur==NULL) || (id!=0)) {
      return NULL;
    }
  } else {
    while ((instcur!=NULL) && (id!=i)) {
      instprev=instcur;
      instcur=instcur[nextp].inst;
      i++;
    }
    if (instcur==NULL) {
      return NULL;
    }
  }
  if (inst!=NULL) *inst=instcur;
  if (prev!=NULL) *prev=instprev;
  return instcur;
}

static int
chkobjid(struct objlist *obj,int id)
/* chkobjid() returns -1 on error */
{
  if ((id>obj->lastinst) || (id<0)) return -1;
  else return id;
}

int
chkobjoid(struct objlist *obj,int oid)
/* chkobjoid() returns -1 on error */
{
  int oidp,idp,nextp;
  N_VALUE *inst;

  if ((oidp=obj->oidp)==-1) return -1;
  if ((idp=obj->idp)==-1) return -1;
  inst=obj->root;
  if ((nextp=obj->nextp)==-1) {
    if (inst==NULL) return -1;
    if (inst[oidp].i==oid) return inst[idp].i;
    else return -1;
  } else {
    while (inst!=NULL) {
      if (inst[oidp].i==oid) return inst[idp].i;
      inst=inst[nextp].inst;
    }
  }
  return -1;
}

static int
chkobjname(struct objlist *obj,int *id,char *name)
/* chkobjname() returns -1 when named object is not found*/
{
  int i,id2;
  char *iname;

  if (id == NULL) {
    id2 = 0;
  } else {
    id2 = *id;
  }

  if (chkobjoffset(obj, "name") == -1) {
    return -1;
  }

  for (i = id2; i <= obj->lastinst; i++) {
    N_VALUE *inst;
    inst = chkobjinst(obj, i);
    if (inst == NULL) {
      return -1;
    }

    if (_getobj(obj, "name", inst, &iname) == -1) {
      return -1;
    }

    if (iname && strcmp0(iname, name) == 0) {
      if (id) {
	*id = i + 1;
      }
      return i;
    }
  }

  if (id) {
    *id = obj->lastinst + 1;
  }

  return -1;
}

int
chkobjfieldnum(struct objlist *obj)
{
  struct objlist *objcur,*objcur2;
  char *name;
  int i,j,num;

  if (obj->fieldnum >= 0)
    return obj->fieldnum;

  num=0;
  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++) {
      name=objcur->table[i].name;
      objcur2=obj;
      while (objcur2!=objcur) {
        for (j=0;j<objcur2->tblnum;j++)
          if (strcmp0(name,objcur2->table[j].name)==0) goto match;
        objcur2=objcur2->parent;
      }
match:
      if (objcur2==objcur) num++;
    }
    objcur=objcur->parent;
  }

  obj->fieldnum = num;
  return num;
}

char *
chkobjfieldname(struct objlist *obj,int num)
{
  struct objlist *objcur,*objcur2,*objcur3;
  char *name;
  int i,j,tnum;

  tnum=0;
  objcur=NULL;
  while (objcur!=obj) {
    objcur2=obj;
    while (objcur!=(objcur2->parent)) objcur2=objcur2->parent;
    for (i=0;i<objcur2->tblnum;i++) {
      name=objcur2->table[i].name;
      objcur3=obj;
      while (objcur3!=objcur2) {
        for (j=0;j<objcur3->tblnum;j++)
          if (strcmp0(name,objcur3->table[j].name)==0) goto match;
        objcur3=objcur3->parent;
      }
match:
      if (objcur3==objcur2) {
        if (tnum==num) return name;
        tnum++;
      }
    }
    objcur=objcur2;
  }
  return NULL;
}

int
chkobjfield(struct objlist *obj,const char *name)
{
#if USE_HASH
  struct objlist *objcur;
  int i;

  i = chkobjtblpos(obj, name, &objcur);
  if (i < 0) return -1;

  return 0;
#else
  struct objlist *objcur;
  int i;

  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++)
      if (strcmp0(objcur->table[i].name,name)==0) return 0;
    objcur=objcur->parent;
  }
  return -1;
#endif
}

int
chkobjperm(struct objlist *obj, const char *name)
/* chkobjperm() returns 0 on error */
{
  struct objlist *robj;
  int idn;

  if ((idn=chkobjtblpos(obj,name,&robj))==-1) return 0;
  return robj->table[idn].attrib;
}

enum ngraph_object_field_type
chkobjfieldtype(struct objlist *obj, const char *name)
/* chkobjperm() returns VOID on error */
{
  struct objlist *robj;
  int idn;

  if ((idn=chkobjtblpos(obj,name,&robj))==-1) return NVOID;
  return robj->table[idn].type;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void *
chkobjproc(struct objlist *obj,char *name)
{
  int namen;
  struct objlist *robj;

  if ((namen=chkobjtblpos(obj,name,&robj))==-1) return NULL;
  return robj->table[namen].proc;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

const char *
chkobjarglist(struct objlist *obj, const char *name)
{
  int namen;
  enum ngraph_object_field_type type;
  struct objlist *robj;
  char *arglist;

  if ((namen=chkobjtblpos(obj,name,&robj))==-1) return NULL;
  type=chkobjfieldtype(obj,name);
  arglist=robj->table[namen].arglist;
  if ((arglist==NULL) && (type<NVFUNC)) {
    switch (type) {
    case NVOID:
      arglist="";
      break;
    case NBOOL:
      arglist="b";
      break;
#if USE_NCHAR
    case NCHAR:
      arglist="c";
      break;
#endif
    case NINT:
      arglist="i";
      break;
    case NDOUBLE:
      arglist="d";
      break;
    case NSTR:
      arglist="s";
      break;
    case NPOINTER:
      arglist="p";
      break;
    case NIARRAY:
      arglist="ia";
      break;
    case NDARRAY:
      arglist="da";
      break;
    case NSARRAY:
      arglist="sa";
      break;
    case NOBJ:
      arglist="o";
      break;
    default:
      arglist="";
    break;
    }
  }
  return arglist;
}

struct objlist *
getobject(const char *name)
/* getobject() returns NULL when the named object is not found */
{
  struct objlist *obj;

  obj = chkobject(name);
  if (obj == NULL) {
    error2(NULL, ERROBJFOUND, name);
  }
  return obj;
}

char *
getobjver(const char *name)
/* getobjver() returns NULL when the named object is not found */
{
  struct objlist *obj;

  obj = getobject(name);
  if (obj == NULL) {
    return NULL;
  }
  return obj->ver;
}

static int
getobjcurinst(struct objlist *obj)
{
  if (obj->curinst==-1) {
    error(obj,ERROBJCINST);
    return -1;
  }
  return obj->curinst;
}

static int
getobjlastinst(struct objlist *obj)
{
  if (obj->lastinst==-1) {
    error(obj,ERRNOINST);
    return -1;
  }
  return obj->lastinst;
}

int
getobjoffset(struct objlist *obj, const char *name)
/* getoffset() returns -1 on error */
{
  int offset;

  if (obj==NULL) {
    error(NULL,ERROBJFOUND);
    return -1;
  }
  if ((offset=chkobjoffset(obj,name))==-1) {
    if (strcmp0(name,"id")==0) error(obj,ERRNOID);
    else error2(obj,ERRVALFOUND,name);
  }
  return offset;
}

int
getobjtblpos(struct objlist *obj, const char *name, struct objlist **robj)
/* getoffset() returns -1 on error */
{
  int tblnum;

  if (obj==NULL) {
    error(NULL,ERROBJFOUND);
    return -1;
  }
  if ((tblnum=chkobjtblpos(obj,name,robj))==-1) {
    if (strcmp0(name,"id")==0) error(obj,ERRNOID);
    else error2(obj,ERRVALFOUND,name);
  }
  return tblnum;
}

N_VALUE *
getobjinst(struct objlist *obj,int id)
/* getobjinst() returns NULL if instance is not found */
{
  N_VALUE *instcur;

  if (obj==NULL) {
    error(NULL,ERROBJFOUND);
    return NULL;
  }
  if ((instcur=chkobjinst(obj,id))==NULL) {
    error3(obj,ERRIDFOUND,id);
    return NULL;
  }
  return instcur;
}

N_VALUE *
getobjprev(struct objlist *obj,int id,N_VALUE **inst,N_VALUE **prev)
/* getobjprev() returns NULL if instance is not found */
{
  if (obj==NULL) {
    error(NULL,ERROBJFOUND);
    return NULL;
  }
  if (chkobjprev(obj,id,inst,prev)==NULL) {
    error3(obj,ERRIDFOUND,id);
    return NULL;
  }
  return *inst;
}

N_VALUE *
getobjinstoid(struct objlist *obj,int oid)
{
  N_VALUE *inst;

  if (obj==NULL) {
    error(NULL,ERROBJFOUND);
    return NULL;
  }
  if ((inst=chkobjinstoid(obj,oid))==NULL) {
    error3(obj,ERROIDFOUND,oid);
    return NULL;
  }
  return inst;
}

static int
getobjname(struct objlist *obj,int *id,char *name)
/* getobjname() returns -1 when named instance is not found */
{
  int i,id2;
  char *iname;

  if (id==NULL) id2=0;
  else id2=*id;
  if (getobjoffset(obj,"name")==-1) return -1;
  for (i=id2;i<=obj->lastinst;i++) {
    N_VALUE *inst;
    if ((inst=getobjinst(obj,i))==NULL) return -1;
    if (_getobj(obj,"name",inst,&iname)==-1) return -1;
    if ((iname!=NULL) && (strcmp0(iname,name)==0)) {
      if (id!=NULL) *id=i+1;
      return i;
    }
  }
  if (id!=NULL) *id=obj->lastinst+1;
  error2(obj,ERRNMFOUND,name);
  return -1;
}

static int
getobjid(struct objlist *obj,int id)
/* getobjid() returns -1 on error */
{
  if ((id>obj->lastinst) || (id<0)) {
    error3(obj,ERRIDFOUND,id);
    return -1;
  } else return id;
}

static int
getobjoid(struct objlist *obj,int oid)
/* getobjoid() returns -1 on error */
{
  int id;

  if ((id=chkobjoid(obj,oid))==-1) {
    error3(obj,ERROIDFOUND,oid);
    return -1;
  } else return id;
}

int
getobjfield(struct objlist *obj, const char *name)
{
  if (chkobjfield(obj,name)==-1) {
    error2(obj,ERRVALFOUND,name);
    return -1;
  } else {
    return 0;
  }
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static int
getobjproc(struct objlist *obj,char *vname,void *val)
{
  struct objlist *robj;
  int idn;
  Proc proc;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  proc=robj->table[idn].proc;
  *(Proc *)val=proc;
  return 0;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

static void (* NewObjCB)(struct objlist *obj) = NULL;
static void (* DelObjCB)(struct objlist *obj) = NULL;

int
newobj_alias(struct objlist *obj, const char *name)
{
  struct objlist *robj;
  N_VALUE *instcur,*instnew,*inst;
  int nextp,id,idp,oidp,initn,initp;
  char **argv;

  if (obj == NULL || name == NULL) {
    return -1;
  }

  idp = obj->idp;
  if (idp == -1) {
    error(obj, ERRNOID);
    return -1;
  }

  initn = getobjtblpos(obj, "init", &robj);
  if (initn == -1)
    return -1;

  initp = chkobjoffset2(robj, initn);
  if ((robj->table[initn].attrib & NEXEC) == 0) {
    error2(obj, ERRPERMISSION, "init");
    return -1;
  }

  id = obj->lastinst + 1;
  if ((id + obj->lastinst2 + 1) == INST_MAX) {
    error3(obj, ERRINSTNUM, INST_MAX);
    return -1;
  }

  nextp = obj->nextp;
  instcur = chkobjinst(obj, obj->lastinst);
  if (instcur != NULL && nextp == -1) {
    error(obj, ERRNONEXT);
    return -1;
  }

  instnew = g_malloc0(obj->size * sizeof(N_VALUE));
  if (instnew == NULL) {
    return -1;
  }

  instnew[idp].i = id;
  oidp = obj->oidp;
  if (oidp != -1) {
    if (obj->lastoid == INT_MAX) {
      obj->lastoid=0;
    } else {
      obj->lastoid++;
    }
    if (nextp != -1) {
      do {
        inst=obj->root;
        while (inst) {
          if (inst[oidp].i == obj->lastoid) {
            if (obj->lastoid == INT_MAX) {
	      obj->lastoid=0;
            } else {
	      obj->lastoid++;
	    }
            break;
          }
          inst = inst[nextp].inst;
        }
        if (inst == NULL) {
          inst = obj->root2;
          while (inst) {
            if (inst[oidp].i == obj->lastoid) {
              if (obj->lastoid == INT_MAX) {
		obj->lastoid = 0;
              } else {
		obj->lastoid++;
	      }
              break;
            }
            inst = inst[nextp].inst;
          }
        }
      } while (inst);
    }
    instnew[oidp].i = obj->lastoid;
  }
  if (robj->table[initn].proc) {
    int argc, rcode;
    argv = NULL;
    if (arg_add2(&argv, 2, name, "init") == NULL) {
      g_free(argv);
      return -1;
    }
    argc = getargc(argv);
    rcode = robj->table[initn].proc(robj, instnew, instnew + initp, argc, argv);
    g_free(argv);
    if (rcode != 0) {
      g_free(instnew);
      return -1;
    }
  }
  if (instcur == NULL) {
    obj->root=instnew;
  } else {
    instcur[nextp].inst = instnew; /* nextp != -1 when instcur in not NULL */
  }
  if (nextp != -1) {
    instnew[nextp].inst = NULL;
  }
  obj->lastinst = id;
  obj->curinst = id;

  if (NewObjCB) {
    NewObjCB(obj);
  }

  return id;
}

void
set_newobj_cb(void (* newobj_cb)(struct objlist *obj))
{
  NewObjCB = newobj_cb;
}

void
set_delobj_cb(void (* delobj_cb)(struct objlist *obj))
{
  DelObjCB = delobj_cb;
}

int
newobj(struct objlist *obj)
/* newobj() returns id or -1 on error */
{
  return newobj_alias(obj, obj->name);
}

int
delobj(struct objlist *obj,int delid)
/* delobj() returns id or -1 on error */
{
  struct objlist *robj,*objcur;
  N_VALUE *instcur,*instprev,*inst;
  int i,nextp,idp,donen,donep,offset;
  char **argv;
  struct narray *array;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if ((donen=chkobjtblpos(obj,"done",&robj))==-1) {
    error(obj,ERRDESTRUCT);
    return -1;
  }
  donep=chkobjoffset2(robj,donen);
  if ((robj->table[donen].attrib & NEXEC)==0) {
    error2(obj,ERRPERMISSION,"done");
    return -1;
  }
  if (getobjprev(obj,delid,&instcur,&instprev)==NULL) return -1;
  if (robj->table[donen].proc!=NULL) {
    int argc, rcode;;
    argv=NULL;
    if (arg_add2(&argv,2,obj->name,"done")==NULL) {
      g_free(argv);
      return -1;
    }
    argc=getargc(argv);
    rcode=robj->table[donen].proc(robj,instcur,instcur+donep,argc,argv);
    g_free(argv);
    if (rcode!=0) return -1;
  }
  if ((nextp=obj->nextp)==-1) obj->root=NULL;
  else {
    if (instprev==NULL) obj->root=instcur[nextp].inst;
    else instprev[nextp].inst=instcur[nextp].inst;
    inst=instcur[nextp].inst;
    while (inst!=NULL) {
      inst[idp].i--;
      inst=inst[nextp].inst;
    }
    instcur[nextp].inst=NULL;
  }
  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++) {
      offset=objcur->table[i].offset;
      switch (objcur->table[i].type) {
      case NSTR:
      case NOBJ:
      case NSFUNC:
        g_free(instcur[offset].str);
        break;
      case NPOINTER:
        g_free(instcur[offset].ptr);
        break;
      case NIARRAY: case NIAFUNC:
      case NDARRAY: case NDAFUNC:
        array=instcur[offset].array;
        arrayfree(array);
        break;
      case NSARRAY: case NSAFUNC:
        array=instcur[offset].array;
        arrayfree2(array);
        break;
      default:
        break;
      }
    }
    objcur=objcur->parent;
  }
  g_free(instcur);
  obj->lastinst--;
  obj->curinst=-1;

  if (DelObjCB) {
    DelObjCB(obj);
  }

  return 0;
}

void
delchildobj(struct objlist *parent)
{
  struct objlist *ocur;
  int i,instnum;

  ocur=chkobjroot();
  while (ocur!=NULL) {
    if (chkobjparent(ocur)==parent) {
      if ((instnum=chkobjlastinst(ocur))!=-1)
        for (i=instnum;i>=0;i--) delobj(ocur,i);
      delchildobj(ocur);
    }
    ocur=ocur->next;
  }
}

int
_putobj(struct objlist *obj, const char *vname,N_VALUE *inst,void *val)
{
  struct objlist *robj;
  int idp,idn;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  switch (robj->table[idn].type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT:  case NIFUNC:
#if USE_NCHAR
  case NCHAR: case NCFUNC:
#endif
  case NENUM:
    inst[idp].i=*(int *)val;
    break;
  case NDOUBLE: case NDFUNC:
    inst[idp].d=*(double *)val;
    break;
  default:
    inst[idp].ptr=val;
    break;
  }
  return 0;
}

int
putobj(struct objlist *obj, const char *vname,int id,void *val)
/* putobj() returns id or -1 on error */
{
  struct objlist *robj;
  struct narray *array;
  N_VALUE *instcur;
  int idp,idn;
  char **argv;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);

  if ((instcur=getobjinst(obj,id))==NULL) return -1;

  if ((robj->table[idn].attrib & NWRITE)==0) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }

  if ((robj->table[idn].type<NVFUNC) && (robj->table[idn].proc!=NULL)) {
    int rcode,argc;
    argv=NULL;
    if (arg_add2(&argv,3,obj->name,vname,val)==NULL) {
      g_free(argv);
      return -1;
    }
    argc=getargc(argv);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc,argv);
    val=argv[2];
    g_free(argv);
    if (rcode!=0) return -1;
  }

  switch (robj->table[idn].type) {
  case NSTR:
  case NOBJ:
  case NSFUNC:
    g_free(instcur[idp].str);
    break;
  case NPOINTER:
    g_free(instcur[idp].ptr);
    break;
  case NIARRAY: case NIAFUNC:
  case NDARRAY: case NDAFUNC:
    array=instcur[idp].array;
    arrayfree(array);
    break;
  case NSARRAY: case NSAFUNC:
    array=instcur[idp].array;
    arrayfree2(array);
    break;
  default:
    break;
  }

  switch (robj->table[idn].type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NBOOL:
  case NINT:
#if USE_NCHAR
  case NCHAR:
#endif
  case NENUM:
    instcur[idp].i=*(int *)val;
    break;
  case NDOUBLE:
    instcur[idp].d=*(double *)val;
    break;
  default:
    instcur[idp].ptr=val;
    break;
  }
  obj->curinst=id;
  return id;
}

int
_getobj(struct objlist *obj, const char *vname,N_VALUE *inst,void *val)
{
  struct objlist *robj;
  int idp,idn;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  switch (robj->table[idn].type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT:  case NIFUNC:
#if USE_NCHAR
  case NCHAR: case NCFUNC:
#endif
  case NENUM:
    *(int *)val=inst[idp].i;
    break;
  case NDOUBLE: case NDFUNC:
    *(double *)val=inst[idp].d;
    break;
  case NSTR: case NSFUNC:
  case NOBJ:
    *(char **)val = inst[idp].str;
    break;
  case NIARRAY: case NIAFUNC:
  case NSARRAY: case NSAFUNC:
  case NDAFUNC: case NDARRAY:
    *(struct narray **)val = inst[idp].array;
    break;
  default:
    *(char **)val=inst[idp].ptr;
    break;
  }
  return 0;
}

int
getobj(struct objlist *obj, const char *vname,int id,
           int argc,char **argv,void *val)
/* getobj() returns id or -1 on error */
{
  struct objlist *robj;
  N_VALUE *instcur;
  int idp,idn;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if ((instcur=getobjinst(obj,id))==NULL) return -1;
  if (((robj->table[idn].attrib & NREAD)==0)
  || ((robj->table[idn].type>=NVFUNC)
   && ((robj->table[idn].attrib & NEXEC)==0))) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }
  if ((robj->table[idn].type>=NVFUNC) && (robj->table[idn].proc!=NULL)) {
    int i, argc2, rcode;
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      g_free(argv2);
      return -1;
    }
    for (i=0;i<argc;i++) {
      if (arg_add(&argv2,argv[i])==NULL) {
        g_free(argv2);
        return -1;
      }
    }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc2,argv2);
    g_free(argv2);
    if (rcode!=0) return -1;
  }
  switch (robj->table[idn].type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT: case NIFUNC:
#if USE_NCHAR
  case NCHAR: case NCFUNC:
#endif
  case NENUM:
    *(int *)val=instcur[idp].i;
    break;
  case NDOUBLE: case NDFUNC:
    *(double *)val=instcur[idp].d;
    break;
  case NSTR: case NSFUNC:
  case NOBJ:
    *(char **)val = instcur[idp].str;
    break;
  case NIARRAY: case NIAFUNC:
  case NSARRAY: case NSAFUNC:
  case NDAFUNC: case NDARRAY:
    *(struct narray **)val = instcur[idp].array;
    break;
  default:
    *(char **)val=instcur[idp].ptr;
    break;
  }
  obj->curinst=id;
  return id;
}

int
_exeparent(struct objlist *obj,const char *vname,N_VALUE *inst,N_VALUE *rval,
               int argc,char **argv)
/* _exeparent() returns errorlevel or -1 on error */
{
  struct objlist *parent,*robj;
  int idn,idp,rcode;
  N_VALUE *rval2;

  if ((parent=obj->parent)==NULL) return 0;
  if ((idn=chkobjtblpos(parent,vname,&robj))==-1) return 0;
  idp=chkobjoffset2(robj,idn);
  if (chkobjfieldtype(parent,vname)<NVFUNC) return -1;
  if (rval==NULL) rval2=inst+idp;
  else rval2=rval;
  if (robj->table[idn].proc!=NULL) {
    rcode=robj->table[idn].proc(robj,inst,rval2,argc,argv);
  } else rcode=0;
  return rcode;
}

int
__exeobj(struct objlist *obj,int idn,N_VALUE *inst,int argc,char **argv)
/* __exeobj() returns errorlevel or -1 on error */
{
  int rcode;

  if (obj->table[idn].type<NVFUNC) return -1;
  if (obj->table[idn].proc!=NULL) {
    int idp;
    idp=chkobjoffset2(obj,idn);
    rcode=obj->table[idn].proc(obj,inst,inst+idp,argc,argv);
  } else rcode=0;
  return rcode;
}

int
_exeobj(struct objlist *obj,const char *vname,N_VALUE *inst,int argc,char **argv)
/* _exeobj() returns errorlevel or -1 on error */
{
  struct objlist *robj;
  int idn,idp,rcode;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if (robj->table[idn].type<NVFUNC) return -1;
  if (robj->table[idn].proc!=NULL) {
    int i, argc2;
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      g_free(argv2);
      return -1;
    }
    for (i=0;i<argc;i++) {
      if (arg_add(&argv2,((char **)argv)[i])==NULL) {
        g_free(argv2);
        return -1;
      }
    }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,inst,inst+idp,argc2,argv2);
    g_free(argv2);
  } else rcode=0;
  return rcode;
}

int
exeobj(struct objlist *obj, const char *vname,int id,int argc,char **argv)
/* exeobj() returns errorlevel or -1 on error */
{
  struct objlist *robj;
  N_VALUE *instcur;
  int idn,idp,rcode;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if ((instcur=getobjinst(obj,id))==NULL) return -1;
  if ((robj->table[idn].type<NVFUNC)
  || ((robj->table[idn].attrib & NREAD)==0)
  || ((robj->table[idn].type>=NVFUNC)
   && ((robj->table[idn].attrib & NEXEC)==0))) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }
  if (robj->table[idn].proc!=NULL) {
    int i, argc2;
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      g_free(argv2);
      return -1;
    }
    for (i=0;i<argc;i++) {
      if (arg_add(&argv2,((char **)argv)[i])==NULL) {
        g_free(argv2);
        return -1;
      }
    }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc2,argv2);
    g_free(argv2);
  } else rcode=0;
  obj->curinst=id;
  return rcode;
}

int
copyobj(struct objlist *obj, const char *vname,int did,int sid)
{
  struct objlist *robj;
  unsigned int i;
  int idn;
  char value[8];
  char *po;
  struct narray *array;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  if (((robj->table[idn].attrib & NREAD)==0)
  || ((robj->table[idn].attrib & NWRITE)==0)
  || (robj->table[idn].type>=NVFUNC)) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }
  po=value;
  if (getobj(obj,vname,sid,0,NULL,po)==-1) return -1;
  switch (robj->table[idn].type) {
  case NVOID:
#if USE_LABEL
  case NLABEL:
#endif
  case NBOOL:
  case NINT:
#if USE_NCHAR
  case NCHAR:
#endif
  case NENUM:
  case NDOUBLE:
    if (putobj(obj,vname,did,po)==-1) return -1;
    break;
  case NSTR:
  case NOBJ:
    if (*(char **)po==NULL) {
      if (putobj(obj,vname,did,NULL)==-1) return -1;
    } else {
      char *s;
      s = g_strdup(*(char **)po);
      if (s == NULL) return -1;
      if (putobj(obj,vname,did,s)==-1) return -1;
    }
    break;
  case NIARRAY:
  case NDARRAY:
    if (*(struct narray **)po==NULL) {
      if (putobj(obj,vname,did,NULL)==-1) return -1;
    } else {
      if ((array=arraynew(arraybase(*(struct narray **)po)))==NULL) return -1;
      for (i=0;i<arraynum(*(struct narray **)po);i++) {
        if (arrayadd(array,arraynget(*(struct narray **)po,i))==NULL) {
          arrayfree(array);
          return -1;
        }
      }
      if (putobj(obj,vname,did,array)==-1) {
        arrayfree(array);
        return -1;
      }
    }
    break;
  case NSARRAY:
    if (*(struct narray **)po==NULL) {
      if (putobj(obj,vname,did,NULL)==-1) return -1;
    } else {
      if ((array=arraynew(arraybase(*(struct narray **)po)))==NULL) return -1;
      for (i=0;i<arraynum(*(struct narray **)po);i++) {
        if (arrayadd2(array,*(char **) arraynget(*(struct narray **)po,i))==NULL) {
          arrayfree2(array);
          return -1;
        }
      }
      if (putobj(obj,vname,did,array)==-1) {
        arrayfree2(array);
        return -1;
      }
    }
    break;
  default:
    if (putobj(obj,vname,did,*(char **)po)) return -1;
    break;
  }
  return did;
}

int
moveobj(struct objlist *obj,int did,int sid)
/* moveobj() returns id or -1 on error */
{
  N_VALUE *dinstcur;
  int idp;

  idp = obj->idp;
  if (idp == -1) {
    error(obj, ERRNOID);
    return -1;
  }

  if (getobjid(obj, sid) == -1) {
    return -1;
  }

  if (did == sid) {
    return did;
  }

  dinstcur = chkobjinst(obj, did);
  if (dinstcur == NULL) {
    return -1;
  }

  exchobj(obj,did,sid);
  if (delobj(obj, sid) == -1) {
    exchobj(obj, did, sid);
    return -1;
  }

  obj->curinst = dinstcur[idp].i;
  return dinstcur[idp].i;
}

int
moveupobj(struct objlist *obj,int id)
/* moveupobj() returns id or -1 on error */
{
  N_VALUE *instcur,*instprev,*instcur2,*instprev2;
  N_VALUE *inst;
  int idp,nextp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjprev(obj,id,&instcur,&instprev)==NULL) return -1;
  if (id==0) return id;
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  if (getobjprev(obj,id-1,&instcur2,&instprev2)==NULL) return -1;
  inst=instcur[nextp].inst;
  if (instprev2==NULL) obj->root=instcur;
  else instprev2[nextp].inst=instcur;
  instcur[nextp].inst=instprev;
  if (instprev==NULL) obj->root=inst;
  else instprev[nextp].inst=inst;
  instcur[idp].i--;
  instprev[idp].i++;
  obj->curinst=id-1;
  return id-1;
}

int
movetopobj(struct objlist *obj,int id)
/* movetopobj() returns id or -1 on error */
{
  N_VALUE *instcur,*instprev;
  N_VALUE *rinst,*pinst,*inst;
  int idp,nextp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjprev(obj,id,&instcur,&instprev)==NULL) return -1;
  if (id==0) return id;
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  rinst=obj->root;
  inst=instcur[nextp].inst;
  pinst=instprev[nextp].inst;
  obj->root=pinst;
  instcur[nextp].inst=rinst;
  instprev[nextp].inst=inst;
  instcur[idp].i=0;
  instcur=rinst;
  while (instcur!=inst) {
    instcur[idp].i++;
    instcur=instcur[nextp].inst;
  }
  obj->curinst=0;
  return 0;
}

int
movedownobj(struct objlist *obj,int id)
/* movedownobj() returns id or -1 on error */
{
  N_VALUE *instcur,*instprev;
  N_VALUE *ninst,*inst;
  int idp,nextp,lid;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjprev(obj,id,&instcur,&instprev)==NULL) return -1;
  lid=chkobjlastinst(obj);
  if (id==lid) return id;
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  inst=instcur[nextp].inst;
  ninst=inst[nextp].inst;
  if (instprev==NULL) obj->root=inst;
  else instprev[nextp].inst=inst;
  inst[nextp].inst=instcur;
  instcur[nextp].inst=ninst;
  instcur[idp].i++;
  inst[idp].i--;
  obj->curinst=id+1;
  return id+1;
}

int
movelastobj(struct objlist *obj,int id)
/* movelastobj() returns id or -1 on error */
{
  N_VALUE *instcur,*instprev,*lastinst;
  N_VALUE *pinst,*inst;
  int idp,nextp,lid;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjprev(obj,id,&instcur,&instprev)==NULL) return -1;
  lid=chkobjlastinst(obj);
  if ((lastinst=getobjinst(obj,lid))==NULL) return -1;
  if (id==lid) return id;
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  inst=instcur[nextp].inst;
  if (instprev==NULL) pinst=obj->root;
  else pinst=instprev[nextp].inst;
  lastinst[nextp].inst=pinst;
  instcur[nextp].inst=NULL;
  if (instprev==NULL) obj->root=inst;
  else instprev[nextp].inst=inst;
  instcur[idp].i=lid;
  while (inst!=instcur) {
    inst[idp].i--;
    inst=inst[nextp].inst;
  }
  obj->curinst=lid;
  return lid;
}

int
exchobj(struct objlist *obj,int id1,int id2)
/* exchobj() returns id or -1 on error */
{
  N_VALUE *instcur1,*instprev1;
  N_VALUE *instcur2,*instprev2;
  N_VALUE *inst,*inst1,*inst2;
  int idp,id,nextp;

  idp = obj->idp;
  if (idp == -1) {
    error(obj, ERRNOID);
    return -1;
  }
  if (getobjprev(obj, id1, &instcur1, &instprev1)==NULL ||
      getobjprev(obj, id2, &instcur2, &instprev2)==NULL) {
    return -1;
  }

  if (id1 == id2) {
    return id1;
  }

  nextp = obj->nextp;
  if (nextp == -1) {
    error2(obj, ERRVALFOUND, "next");
    return -1;
  }

  id = instcur1[idp].i;

  instcur1[idp].i = instcur2[idp].i;
  instcur2[idp].i = id;

  if (instprev1 == NULL) {
    inst1 = obj->root;
  } else {
    inst1 = instprev1[nextp].inst;
  }

  if (instprev2 == NULL) {
    inst2 = obj->root;
  } else {
    inst2 = instprev2[nextp].inst;
  }

  if (instprev1 == NULL) {
    obj->root = inst2;
  } else {
    instprev1[nextp].inst = inst2;
  }

  if (instprev2 == NULL) {
    obj->root = inst1;
  } else {
    instprev2[nextp].inst = inst1;
  }

  inst = instcur1[nextp].inst;
  instcur1[nextp].inst = instcur2[nextp].inst;
  instcur2[nextp].inst = inst;
  obj->curinst = id2;

  return id2;
}

/*
char *saveobj(struct objlist *obj, int id)
{
  N_VALUE *instcur,*instnew;

  if ((instcur=getobjinst(obj,id))==NULL) return NULL;
  if ((instnew=g_malloc(obj->size))==NULL) return NULL;
  memcpy(instnew,instcur,obj->size * sizeof(N_VALUE));
  return instnew;
}

char *restoreobj(struct objlist *obj,int id,char *image)
{
  N_VALUE *instcur;

  if (obj==NULL) return NULL;
  if ((instcur=getobjinst(obj,id))==NULL) return NULL;
  memcpy(instcur,image,obj->size * sizeof(N_VALUE));
  g_free(image);
  return instcur;
}
*/

static int
chkilist(struct objlist *obj,char *ilist,struct narray *iarray,int def,int *spc)
/* spc  OBJ_LIST_SPECIFIED_NOT_FOUND: not found
        OBJ_LIST_SPECIFIED_BY_ID:     specified by id
        OBJ_LIST_SPECIFIED_BY_OID:    specified by oid
        OBJ_LIST_SPECIFIED_BY_NAME:   specified by name
        OBJ_LIST_SPECIFIED_BY_OTHER:  specified by other
*/

{
  int i,len,snum,dnum,num,sid,l;
  int oid;
  char *tok,*s,*iname,*endptr;

  *spc=OBJ_LIST_SPECIFIED_NOT_FOUND;
  num=0;
  tok=NULL;
  if ((ilist==NULL) || (ilist[0]=='\0')) {
    if (def) {
      if ((snum=chkobjcurinst(obj))==-1) return -1;
      if (arrayadd(iarray,&snum)==NULL) goto errexit;
      num++;
      *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
    }
  } else {
    while ((s=getitok2(&ilist,&len," \t,"))!=NULL) {
      g_free(tok);
      tok=s;
      iname=NULL;
      if (s[0]=='@') {
        if ((snum=chkobjcurinst(obj))==-1) goto errexit;
        s++;
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
      } else if (s[0]=='!') {
        if ((snum=chkobjlastinst(obj))==-1) goto errexit;
        s++;
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
      } else {
        if (s[0]=='^') {
          oid=TRUE;
          s++;
        } else oid=FALSE;
        l=strtol(s,&endptr,10);
        if (s!=endptr) {
          if (oid) {
            snum=chkobjoid(obj,l);
            *spc=OBJ_LIST_SPECIFIED_BY_OID;
          } else {
            snum=chkobjid(obj,l);
            *spc=OBJ_LIST_SPECIFIED_BY_ID;
          }
          if (snum==-1) goto errexit;
        }
        s=endptr;
      }
      if (s==tok) {
        snum=0;
        if ((dnum=chkobjlastinst(obj))==-1) goto errexit;
        iname=tok;
      } else if (s[0]=='-') {
        s++;
        if (s[0]=='@') {
          if ((dnum=chkobjcurinst(obj))==-1) goto errexit;
          *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        } else if (s[0]=='!') {
          if ((dnum=chkobjlastinst(obj))==-1) goto errexit;
          *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            goto errexit;
          } else {
            dnum=getobjid(obj,l);
            if (dnum==-1) goto errexit;
            *spc=OBJ_LIST_SPECIFIED_BY_ID;
          }
        }
      } else if (s[0]=='+') {
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        s++;
        if (s[0]=='@') {
          if ((dnum=chkobjcurinst(obj))==-1) goto errexit;
        } else if (s[0]=='!') {
          if ((dnum=chkobjlastinst(obj))==-1) goto errexit;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            goto errexit;
          } else {
            dnum=l;
          }
        }
        snum+=dnum;
        dnum=snum;
      } else if (s[0]=='\0') {
        dnum=snum;
      } else {
        goto errexit;
      }
      if (iname==NULL) {
        for (i=snum;i<=dnum;i++) {
          if (chkobjid(obj,i)==-1) goto errexit;
          if (arrayadd(iarray,&i)==NULL) goto errexit;
          num++;
        }
      } else {
        *spc=OBJ_LIST_SPECIFIED_BY_NAME;
        sid=0;
        if (chkobjname(obj,&sid,iname)==-1) goto errexit;
        sid=0;
        while ((snum=chkobjname(obj,&sid,iname))>=0) {
          if (arrayadd(iarray,&snum)==NULL) goto errexit;
          num++;
        }
      }
    }
  }
  g_free(tok);
  return num;

errexit:
  g_free(tok);
  arraydel(iarray);
  return -1;
}

static int
getilist(struct objlist *obj,char *ilist,struct narray *iarray,int def,int *spc)
/* spc  OBJ_LIST_SPECIFIED_NOT_FOUND: not found
        OBJ_LIST_SPECIFIED_BY_ID:     specified by id
        OBJ_LIST_SPECIFIED_BY_OID:    specified by oid
        OBJ_LIST_SPECIFIED_BY_NAME:   specified by name
        OBJ_LIST_SPECIFIED_BY_OTHER:  specified by other
*/
{
  int i,len,snum,dnum,num,sid,l;
  int oid;
  char *tok,*s,*iname,*endptr;

  *spc=OBJ_LIST_SPECIFIED_NOT_FOUND;
  num=0;
  tok=NULL;
  if ((ilist==NULL) || (ilist[0]=='\0')) {
    if (def) {
      if ((snum=getobjcurinst(obj))==-1) return -1;
      if (arrayadd(iarray,&snum)==NULL) goto errexit;
      num++;
      *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
    }
  } else {
    while ((s=getitok2(&ilist,&len," \t,"))!=NULL) {
      g_free(tok);
      tok=s;
      iname=NULL;
      if (s[0]=='@') {
        if ((snum=getobjcurinst(obj))==-1) goto errexit;
        s++;
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
      } else if (s[0]=='!') {
        if ((snum=getobjlastinst(obj))==-1) goto errexit;
        s++;
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
      } else {
        if (s[0]=='^') {
          oid=TRUE;
          s++;
        } else oid=FALSE;
        l=strtol(s,&endptr,10);
        if (s!=endptr) {
          if (oid) {
            snum=getobjoid(obj,l);
            *spc=OBJ_LIST_SPECIFIED_BY_OID;
          } else {
            snum=getobjid(obj,l);
            *spc=OBJ_LIST_SPECIFIED_BY_ID;
          }
          if (snum==-1) goto errexit;
        }
        s=endptr;
      }
      if (s==tok) {
        snum=0;
        if ((dnum=getobjlastinst(obj))==-1) goto errexit;
        iname=tok;
      } else if (s[0]=='-') {
        s++;
        if (s[0]=='@') {
          if ((dnum=getobjcurinst(obj))==-1) goto errexit;
          *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        } else if (s[0]=='!') {
          if ((dnum=getobjlastinst(obj))==-1) goto errexit;
          *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            error2(obj,ERRILINST,tok);
            goto errexit;
          } else {
            dnum=getobjid(obj,l);
            if (dnum==-1) goto errexit;
            *spc=OBJ_LIST_SPECIFIED_BY_ID;
          }
        }
      } else if (s[0]=='+') {
        *spc=OBJ_LIST_SPECIFIED_BY_OTHER;
        s++;
        if (s[0]=='@') {
          if ((dnum=getobjcurinst(obj))==-1) goto errexit;
        } else if (s[0]=='!') {
          if ((dnum=getobjlastinst(obj))==-1) goto errexit;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            error2(obj,ERRILINST,tok);
            goto errexit;
          } else {
            dnum=l;
          }
        }
        snum+=dnum;
        dnum=snum;
      } else if (s[0]=='\0') {
        dnum=snum;
      } else {
        error2(obj,ERRILINST,tok);
        goto errexit;
      }
      if (iname==NULL) {
        for (i=snum;i<=dnum;i++) {
          if (getobjid(obj,i)==-1) goto errexit;
          if (arrayadd(iarray,&i)==NULL) goto errexit;
          num++;
        }
      } else {
        *spc=OBJ_LIST_SPECIFIED_BY_NAME;
        sid=0;
        if (getobjname(obj,&sid,iname)==-1) goto errexit;
        sid=0;
        while ((snum=chkobjname(obj,&sid,iname))>=0) {
          if (arrayadd(iarray,&snum)==NULL) goto errexit;
          num++;
        }
      }
    }
  }
  g_free(tok);
  return num;

errexit:
  g_free(tok);
  arraydel(iarray);
  return -1;
}

int
chkobjilist(char *s,struct objlist **obj,struct narray *iarray,int def,int *spc)
{
  char *oname,*ilist;
  int r;
  int spc2;

  r = getobjiname(s, &oname, &s);
  if (r) {
    return r;
  }

  if ((*obj=chkobject(oname))==NULL) {
    g_free(oname);
    return -1;
  }
  g_free(oname);
  if (s[0]==':') s++;
  ilist=s;
  if (def && (chkobjlastinst(*obj)==-1)) return -1;
  if (chkilist(*obj,ilist,iarray,def,&spc2)==-1) return -1;
  if (spc!=NULL) *spc=spc2;
  return 0;
}

int
getobjiname(char *s, char **name, char **ptr)
{
  char *oname;
  int len;

  *name = NULL;
  if (s == NULL) {
    return -1;
  }

  len = 0;
  if (s[0] == ':' || (oname = getitok2(&s, &len, ":")) == NULL) {
    if (len == -1) {
      return -1;
    }
    error2(NULL, ERRILOBJ, s);
    return ERRILOBJ;
  }

  if (ptr) {
    *ptr = s;
  }
  *name = oname;
  return 0;
}

int
getobjilist(char *s,struct objlist **obj,struct narray *iarray,int def,int *spc)
{
  char *oname,*ilist;
  int r;
  int spc2;

  r = getobjiname(s, &oname, &s);
  if (r) {
    return r;
  }

  if ((*obj=getobject(oname))==NULL) {
    g_free(oname);
    return -1;
  }
  g_free(oname);
  if (s[0]==':') s++;
  ilist=s;
  if (def && (getobjlastinst(*obj)==-1)) return -1;
  if (getilist(*obj,ilist,iarray,def,&spc2)==-1) return -1;
  if (spc!=NULL) *spc=spc2;
  return 0;
}

int
chkobjilist2(char **s,struct objlist **obj,struct narray *iarray,int def)
{
  char *oname,*ilist;
  int len,num,spc;

  if ((oname=getitok2(s,&len,":"))==NULL) {
    if (len==-1) return -1;
    return ERRILOBJ;
  }
  if ((*obj=chkobject(oname))==NULL) {
    g_free(oname);
    return -1;
  }
  g_free(oname);
  if (def && (chkobjlastinst(*obj)==-1)) return -1;
  if ((*s)[0]=='\0') {
    return ERRILOBJ;
  } else if ((*s)[0]==':') (*s)++;
  if ((*s)[0]==':') ilist=NULL;
  else {
    if ((ilist=getitok2(s,&len,":"))==NULL) {
      if (len==-1) return -1;
      return ERRILOBJ;
    }
  }
  num=chkilist(*obj,ilist,iarray,def,&spc);
  g_free(ilist);
  if (num==-1) return -1;
  if ((*s)[0]==':') (*s)++;
  return 0;
}

int
getobjilist2(char **s,struct objlist **obj,struct narray *iarray,int def)
{
  char *oname,*ilist;
  int len,num,spc;

  if ((oname=getitok2(s,&len,":"))==NULL) {
    if (len==-1) return -1;
    error2(NULL,ERRILOBJ,*s);
    return ERRILOBJ;
  }
  if ((*obj=getobject(oname))==NULL) {
    g_free(oname);
    return -1;
  }
  g_free(oname);
  if (def && (getobjlastinst(*obj)==-1)) return -1;
  if ((*s)[0]=='\0') {
    error2(NULL,ERRILOBJ,*s);
    return ERRILOBJ;
  } else if ((*s)[0]==':') (*s)++;
  if ((*s)[0]==':') ilist=NULL;
  else {
    if ((ilist=getitok2(s,&len,":"))==NULL) {
      if (len==-1) return -1;
      error2(NULL,ERRILOBJ,*s);
      return ERRILOBJ;
    }
  }
  num=getilist(*obj,ilist,iarray,def,&spc);
  g_free(ilist);
  if (num==-1) return -1;
  if ((*s)[0]==':') (*s)++;
  return 0;
}

char *
mkobjlist(struct objlist *obj, const char *objname,int id, const char *field,int oid)
{
  char ids[11];
  char *s;
  int len,flen;

  if (objname==NULL) objname=chkobjectname(obj);
  if (oid) sprintf(ids,"^%d",id);
  else sprintf(ids,"%d",id);
  if (field!=NULL) flen=strlen(field);
  else flen=0;
  if ((s=g_malloc(strlen(objname)+strlen(ids)+flen+3))==NULL)
    return NULL;
  strcpy(s,objname);
  len=strlen(s);
  s[len]=':';
  strcpy(s+len+1,ids);
  if (field!=NULL) {
    len=strlen(s);
    s[len]=':';
    strcpy(s+len+1,field);
  }
  return s;
}

struct objlist *
getobjlist(char *list,int *id,char **field,int *oid)
{
  char *objname;
  char *ids,*ids2;
  int len;
  struct objlist *obj;
  char *endptr;

  objname=getitok2(&list,&len,":");
  ids=getitok2(&list,&len,":");
  *field=getitok(&list,&len,":");
  if ((objname==NULL) || (ids==NULL) || (*field==NULL)) {
    g_free(objname);
    g_free(ids);
    *field=NULL;
    return NULL;
  }
  obj=chkobject(objname);
  g_free(objname);
  if (ids[0]=='^') {
    if (oid!=NULL) *oid=TRUE;
    ids2=ids+1;
  } else {
    if (oid!=NULL) *oid=FALSE;
    ids2=ids;
  }
  *id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0') || (chkobjoffset(obj,*field)==-1)) {
    g_free(ids);
    *field=NULL;
    return NULL;
  }
  g_free(ids);
  return obj;
}

char *
chgobjlist(char *olist)
{
  char *list,*objname,*newlist;
  char *ids,*ids2;
  int id,len;
  struct objlist *obj;
  char *endptr;
  N_VALUE *inst;

  list=olist;
  objname=getitok2(&list,&len,":");
  ids=getitok2(&list,&len,":");
  if ((objname==NULL) || (ids==NULL)) {
    g_free(objname);
    g_free(ids);
    return NULL;
  }
  if (ids[0]!='^') {
    g_free(objname);
    g_free(ids);
    if ((newlist=g_malloc(strlen(olist)+1))==NULL) return NULL;
    strcpy(newlist,olist);
    return newlist;
  }
  ids2=ids+1;
  id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0')) {
    g_free(objname);
    g_free(ids);
    return NULL;
  }
  g_free(ids);
  obj=chkobject(objname);
  if ((inst=chkobjinstoid(obj,id))==NULL) {
    g_free(objname);
    return NULL;
  }
  _getobj(obj,"id",inst,&id);
  newlist=mkobjlist(obj,objname,id,NULL,FALSE);
  g_free(objname);
  return newlist;
}

char *
getvaluestr(struct objlist *obj,const char *field,void *val,int cr,int quote)
{
  struct narray *array;
  void *po;
  char *bval;
  const char *arglist;
  unsigned int k, n;
  int i;
  enum ngraph_object_field_type type;
  GString *str;

  str = g_string_sized_new(64);
  if (str == NULL) {
    return NULL;
  }

  arglist=chkobjarglist(obj,field);
  type=chkobjfieldtype(obj,field);
  po=val;

  switch (type) {
  case NBOOL: case NBFUNC:
    if (*(int *)po) bval="true";
    else bval="false";
    g_string_append_printf(str,"%s",bval);
    break;
#if USE_NCHAR
  case NCHAR: case NCFUNC:
    g_string_append_printf(str,"%c",*(char *)po);
    break;
#endif
  case NINT: case NIFUNC:
    g_string_append_printf(str,"%d",*(int *)po);
    break;
  case NDOUBLE: case NDFUNC:
    g_string_append_printf(str,"%.15g",*(double *)po);
    break;
  case NSTR: case NSFUNC: case NOBJ:
    if (*(char **)po==NULL) break;
    else {
      bval=*(char **)po;
      if (quote) g_string_append_printf(str,"'");
      for (i=0;bval[i]!='\0';i++) {
        if ((bval[i]=='\'') && quote) g_string_append_printf(str,"'\\''");
        else g_string_append_printf(str,"%c",bval[i]);
      }
      if (quote) g_string_append_printf(str,"'");
    }
    break;
  case NIARRAY: case NIAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) g_string_append_printf(str,"'");
      n = arraynum(array);
      for (k=0;k<n;k++) {
        if (k!=0) g_string_append_printf(str," %d",arraynget_int(array,k));
        else g_string_append_printf(str,"%d",arraynget_int(array,k));
      }
      if (quote) g_string_append_printf(str,"'");
    }
    break;
  case NDARRAY: case NDAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) g_string_append_printf(str,"'");
      n = arraynum(array);
      for (k=0;k<n;k++) {
        if (k!=0) g_string_append_printf(str," " DOUBLE_STR_FORMAT,arraynget_double(array,k));
        else g_string_append_printf(str,DOUBLE_STR_FORMAT,arraynget_double(array,k));
      }
      if (quote) g_string_append_printf(str,"'");
    }
    break;
  case NSARRAY: case NSAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) g_string_append_printf(str,"'");
      for (k=0;k<arraynum(array);k++) {
        if (k!=0) g_string_append_printf(str," ");
        bval=arraynget_str(array,k);
        for (i=0;bval[i]!='\0';i++) {
          if ((bval[i]=='\'') && quote) g_string_append_printf(str,"'\\''");
          else g_string_append_printf(str,"%c",bval[i]);
        }
      }
      if (quote) g_string_append_printf(str,"'");
    }
    break;
  case NENUM:
    g_string_append_printf(str,"%s",((char **)arglist)[*(int *)po]);
    break;
  case NVOID:
  case NPOINTER:
  case NVFUNC:
    /* nothing to do (may be ...) */
    break;
  }
  if (cr) {
    g_string_append_c(str, '\n');
  }

  return g_string_free(str, FALSE);
}

#if 1

static int
add_arg_object(char *s2, char ***argv)
{
  char *p,*os;
  int olen;

  if (s2 && s2[0]) {
    char *oname;
    int i;
    struct objlist *obj2;
    os = s2;
    oname = getitok2(&os, &olen, ":");
    if (oname == NULL) {
      return 1;
    }

    obj2 = chkobject(oname);
    g_free(oname);
    if (obj2 == NULL || os[0] != ':') {
      return 1;
    }

    for (i = 1; os[i] && (isalnum(os[i]) || strchr("_^@!+-,", os[i])); i++);
    if (os[i] != '\0') {
      return 1;
    }

    p = g_strdup(s2);
    if (p == NULL) {
      return 1;
    }
  } else {
    p = NULL;
  }

  if (arg_add(argv, p) == NULL) {
    g_free(p);
    return 1;
  }

  return 0;
}

static int
add_arg_int(char *s2, char ***argv)
{
  int rcode, *p;
  double vd;

  str_calc(s2, &vd, &rcode, NULL);
  if (rcode != MATH_VALUE_NORMAL) {
    return 3;
  }

  p = g_malloc(sizeof(int));
  if (p == NULL) {
    return -1;
  }

  *p = nround(vd);
  if (arg_add(argv, p) == NULL) {
    g_free(p);
    return -1;
  }

  return 0;
}

static int
add_arg_double(char *s2, char ***argv)
{
  int rcode;
  double vd, *p;

  str_calc(s2, &vd, &rcode, NULL);
  if (rcode != MATH_VALUE_NORMAL) {
    return 3;
  }

  p = g_malloc(sizeof(double));
  if (p == NULL) {
    return -1;
  }

  *p = vd;
  if (arg_add(argv, p) == NULL) {
    g_free(p);
    return -1;
  }

  return 0;
}

static int
add_arg_bool(char *s2, char ***argv)
{
  int vi, *p;


  if (g_ascii_strcasecmp("t", s2) == 0||
      g_ascii_strcasecmp("true", s2) == 0) {
    vi = TRUE;
  } else if (g_ascii_strcasecmp("f", s2) == 0 ||
	     g_ascii_strcasecmp("false", s2) == 0) {
    vi = FALSE;
  } else {
    return 3;
  }

  p = g_malloc(sizeof(int));
  if (p == NULL) {
    return -1;
  }

  *p = vi;
  if (arg_add(argv, p) == NULL) {
    g_free(p);
    return -1;
  }

  return 0;
}

static int
add_arg_num(int type, char *s2, char ***argv)
{
  int r = -1;
  switch (type) {
  case 'i':
    r = add_arg_int(s2, argv);
    break;
  case 'd':
    r = add_arg_double(s2, argv);
    break;
  case 'b':
    r = add_arg_bool(s2, argv);
    break;
  }

  return r;
}


static int
add_arg_sarray(struct narray **sary, int argc, char **argv)
{
  int i;
  struct narray *array;

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    return -1;
  }

  for (i = 0; i < argc; i++) {
    if (arrayadd2(array, argv[i]) == NULL) {
      arrayfree2(array);
      return -1;
    }
  }

  *sary = array;

  return 0;
}

static int
add_arg_iarray(struct narray **iary, int argc, char **argv)
{
  int i, vi, rcode;
  double vd;
  struct narray *array;

  array = arraynew(sizeof(int));
  if (array == NULL) {
    return -1;
  }

  for (i = 0; i < argc; i++) {
    str_calc(argv[i], &vd, &rcode, NULL);
    if (rcode != MATH_VALUE_NORMAL) {
      arrayfree(array);
      return 3;
    }
    vi = nround(vd);
    if (arrayadd(array, &vi) == NULL) {
      arrayfree(array);
      return -1;
    }
  }

  *iary = array;

  return 0;
}

static int
add_arg_darray(struct narray **dary, int argc, char **argv)
{
  int i, rcode;
  double vd;
  struct narray *array;

  array = arraynew(sizeof(double));
  if (array == NULL) {
    return -1;
  }

  for (i = 0; i < argc; i++) {
    str_calc(argv[i], &vd, &rcode, NULL);
    if (rcode != MATH_VALUE_NORMAL) {
      arrayfree(array);
      return 3;
    }
    if (arrayadd(array, &vd) == NULL) {
      arrayfree(array);
      return -1;
    }
  }

  *dary = array;

  return 0;
}

static int
set_arg_enum(char **enumlist, char **argv, char *s)
{
  int i, *p;

  if (s == NULL || enumlist == NULL) {
    return 3;
  }

  s = g_strstrip(s);
  if (s[0] == '\0') {
    return 3;
  }

  for (i = 0; enumlist[i]; i++) {
    int ofst;

    ofst = (enumlist[i][0] == '\0') ? 1 : 0;
    if (strcmp0(enumlist[i] + ofst, s) == 0) {
      break;
    }
  }

  if (enumlist[i] == NULL) {
    return 3;
  }

  p = g_malloc(sizeof(int));
  if (p == NULL) {
    return -1;
  }

  *p = i;
  if (arg_add(&argv, p) == NULL) {
    g_free(p);
    return -1;
  }

  return 0;
}

static int
get_array_argument(int type, char *val, struct narray **array)
{
  int r, argc;
  char **argv;

  *array = NULL;

  if (val == NULL || val[0] == '\0') {
    return 0;
  }

  argv = NULL;
  r = g_shell_parse_argv(val, &argc, &argv, NULL);
  if (! r) {
    g_strfreev(argv);
    return -1;
  }

  r = 0;
  switch (type) {
  case 'i':
    r = add_arg_iarray(array, argc, argv);
    break;
  case 'd':
    r = add_arg_darray(array, argc, argv);
    break;
  case 's':
    r = add_arg_sarray(array, argc, argv);
    break;
  }

  g_strfreev(argv);

  return r;
}

static int
getargument(int type, const char *arglist, char *val, int *argc, char ***rargv)
{
  char **argv, *p, *s, **sargv;
  int i,err, r, sargc;

  argv = NULL;
  sargv = NULL;

  if (arg_add(&argv, NULL) == NULL) {
    err = 1;
    goto errexit;
  }

  if (val == NULL) {
    *argc = getargc(argv);
    *rargv = argv;
    return 0;
  }

  err = -1;

  if (type == NENUM) {
    r = set_arg_enum((char **) arglist, argv, val);
    if (r) {
      return r;
    }

    *argc = getargc(argv);
    *rargv = argv;
    return 0;
  }

  if (arglist == NULL) {
    if (val[0] == '\0') {
      *argc = getargc(argv);
      *rargv = argv;
      return 0;
    }

    r = g_shell_parse_argv(val, &sargc, &sargv, NULL);
    if (! r) {
      goto errexit;
    }

    for (i = 0; i < sargc; i++) {
      p = g_strdup(sargv[i]);
      if (arg_add(&argv, p) == NULL) {
	goto errexit;
      }
    }
  } else if (arglist[0] == '\0') {
    if (val[0]) {
      err = 1;
      goto errexit;
    }
    *argc = getargc(argv);
    *rargv = argv;
  } else if (arglist[1] == 'a') {
    struct narray *array;
    r = get_array_argument(arglist[0], val, &array);
    if (r) {
      err = r;
      goto errexit;
    }

    if (arg_add(&argv, array) == NULL) {
      if (arglist[0] == 's') {
	arrayfree2(array);
      } else {
	arrayfree(array);
      }
      err = -1;
      goto errexit;
    }
  } else if (strcmp0(arglist, "s") == 0) {
    if (val[0]) {
      p = g_strdup(val);
      if (p == NULL) {
	goto errexit;
      }
    } else {
      p = NULL;
    }
    if (arg_add(&argv, p) == NULL) {
      goto errexit;
    }
  } else if (strcmp0(arglist, "o") == 0) {
    if (add_arg_object(val, &argv)) {
      err = 3;
      goto errexit;
    }
  } else {
    int j;

    sargc = 0;
    if (val[0]) {
      r = g_shell_parse_argv(val, &sargc, &sargv, NULL);
      if (! r) {
	goto errexit;
      }
    }

    for (j = 0; arglist[j]; j++) {
      switch (arglist[j]) {
      case 's':
	if (j < sargc) {
	  if (arglist[j + 1] == '\0') {
	    p = g_strjoinv(" ", sargv + j);
	  } else {
	    p = g_strdup(sargv[j]);
	  }
	} else if (arglist[j + 1] == '\0') {
	  p = NULL;
	} else {
	  err = 2;
	  goto errexit;
	}
	if (arg_add(&argv, p) == NULL) {
	  goto errexit;
	}
	break;
      case 'o':
	if (sargc > 0 && j >= sargc && arglist[j + 1]) {
	  err = 1;
	  goto errexit;
	}
	if (sargc == 0 || (j >= sargc && arglist[j + 1] == '\0')) {
	  s = NULL;
	} else {
	  s = sargv[j];
	}
	if (add_arg_object(s, &argv)) {
	  err = 3;
	  goto errexit;
	}
	break;
      case 'i':
      case 'd':
      case 'b':
	if (j >= sargc) {
	  err = 2;
	  goto errexit;
	}
	r = add_arg_num(arglist[j], sargv[j], &argv);
	if (r) {
	  err = r;
	  goto errexit;
	}
	break;
      }
    }
  }

  if (sargv) {
    g_strfreev(sargv);
  }
  *argc = getargc(argv);
  *rargv = argv;
  return 0;

errexit:
  if (sargv) {
    g_strfreev(sargv);
  }
  arg_del(argv);
  *argc = -1;
  *rargv = NULL;
  return err;
}
#else
static int
getargument(int type,char *arglist, char *val,int *argc, char ***rargv)
{
  struct narray *array;
  int len,alp;
  char *list,*s, *s2;
  int vi;
  double vd;
  char **argv,*p;
  int rcode;
  int i,err;
  char **enumlist;
  char *oname,*os;
  int olen;
  struct objlist *obj2;

  array = NULL;
  argv = NULL;
  if (arg_add(&argv,NULL) == NULL) {
    err = 1;
    goto errexit;
  }

  s2 = NULL;
  alp = 0;

  if (val == NULL) {
    *argc = getargc(argv);
    *rargv = argv;
    return 0;
  }
  list = val;
  err = -1;
  if (type == NENUM) {
    enumlist = (char **)arglist;
    arglist = "s";
  } else {
    /* initialize to avoid warning nessage */
    enumlist = NULL;
  }
  while (TRUE) {
    if ((arglist != NULL)
	&& ((strcmp0(arglist + alp, "s") == 0) || (strcmp0(arglist + alp, "o") == 0))) {
      if (list[0] == '\0') {
        s = NULL;
        len = 0;
        alp++;
      } else {
        s = list;
        len = strlen(list);
        list += len;
      }
    } else {
      s=getitok(&list,&len," \t\n\r");
      for (;(list[0]!='\0') && (strchr(" \t\n\r",list[0])!=NULL);list++);
    }
    if (s==NULL) break;
    g_free(s2);
    if ((s2=g_malloc(len+1))==NULL) goto errexit;
    strncpy(s2,s,len);
    s2[len]='\0';
    if ((arglist!=NULL) && (arglist[alp]=='\0')) {
      err=1;
      goto errexit;
    }
    if (arglist==NULL) {
      if ((p=g_malloc(strlen(s2)+1))==NULL) goto errexit;
      strcpy(p,s2);
      if (arg_add(&argv,p)==NULL) goto errexit;
    } else if (arglist[alp]=='o') {
      if (arglist[1]=='a') {
        err=3;
        goto errexit;
      }
      os=s2;
      if ((oname=getitok2(&os,&olen,":"))==NULL) {
        err=3;
        goto errexit;
      }
      obj2=chkobject(oname);
      g_free(oname);
      if ((obj2==NULL) || (os[0]!=':')) {
        err=3;
        goto errexit;
      }
      for (i=1;
          (os[i]!='\0')&&(isalnum(os[i])||(strchr("_^@!+-,",os[i])!=NULL));i++);
      if (os[i]!='\0') {
        err=3;
        goto errexit;
      }
      if ((p=g_malloc(strlen(s2)+1))==NULL) goto errexit;
      strcpy(p,s2);
      if (arg_add(&argv,p)==NULL) goto errexit;
    } else if (arglist[alp]=='s') {
      if (arglist[1]=='a') {
        if (array==NULL) {
          if ((array=arraynew(sizeof(char *)))==NULL) goto errexit;
        }
        if (arrayadd2(array,s2)==NULL) goto errexit;
      } else {
        if ((p=g_malloc(strlen(s2)+1))==NULL) goto errexit;
        strcpy(p,s2);
        if (arg_add(&argv,p)==NULL) goto errexit;
      }
    } else if ((arglist[alp]=='i') || (arglist[alp]=='d')) {
      str_calc(s2, &vd, &rcode, NULL);
      if (rcode!=MATH_VALUE_NORMAL) {
        err=3;
        goto errexit;
      }
      if (arglist[alp]=='i') {
        if (arglist[1]=='a') {
          vi=nround(vd);
          if (array==NULL) {
            if ((array=arraynew(sizeof(int)))==NULL) goto errexit;
          }
          if (arrayadd(array,&vi)==NULL) goto errexit;
        } else {
          if ((p=g_malloc(sizeof(int)))==NULL) goto errexit;
          *(int *)(p)=nround(vd);
          if (arg_add(&argv,p)==NULL) goto errexit;
        }
      } else {
        if (arglist[1]=='a') {
          if (array==NULL) {
            if ((array=arraynew(sizeof(double)))==NULL) goto errexit;
          }
          if (arrayadd(array,&vd)==NULL) goto errexit;
        } else {
          if ((p=g_malloc(sizeof(double)))==NULL) goto errexit;
          *(double *)(p)=vd;
          if (arg_add(&argv,p)==NULL) goto errexit;
        }
      }
    } else if (arglist[alp]=='b') {
      if ((strcmp0("T",s2)==0)
      || (strcmp0("True",s2)==0)
      || (strcmp0("TRUE",s2)==0)
      || (strcmp0("t",s2)==0)
      || (strcmp0("true",s2)==0)) vi=TRUE;
      else if ((strcmp0("F",s2)==0)
      || (strcmp0("False",s2)==0)
      || (strcmp0("FALSE",s2)==0)
      || (strcmp0("f",s2)==0)
      || (strcmp0("false",s2)==0)) vi=FALSE;
      else {
        err=3;
        goto errexit;
      }
      if (arglist[1]=='a') {
        err=3;
        goto errexit;
      } else {
        if ((p=g_malloc(sizeof(int)))==NULL) goto errexit;
        *(int *)(p)=vi;
        if (arg_add(&argv,p)==NULL) goto errexit;
      }
    } else if (arglist[alp]=='c') {
      if (strlen(s2)>1) {
        err=3;
        goto errexit;
      }
      vi=s2[0];
      if (arglist[1]=='a') {
        err=3;
        goto errexit;
      } else {
        if ((p=g_malloc(sizeof(int)))==NULL) goto errexit;
        *(int *)(p)=vi;
        if (arg_add(&argv,p)==NULL) goto errexit;
      }
    } else {
      err=3;
      goto errexit;
    }
    if ((arglist!=NULL) && (arglist[1]!='a')) alp++;
  }
  if ((arglist != NULL) && (arglist[1] == 'a')) {
    if (arg_add(&argv,array) == NULL) {
      goto errexit;
    }
  } else {
    if ((arglist != NULL) && (arglist[alp] != '\0')) {
      err = 2;
      goto errexit;
    }
  }
  if (type == NENUM) {
    if (argv[0] == NULL) {
      err = 3;
      goto errexit;
    }
    for (i=0;enumlist[i]!=NULL;i++) {
      int ofst;
      ofst = (enumlist[i][0] == '\0') ? 1 : 0;
      if (strcmp0(enumlist[i] + ofst, argv[0]) == 0) {
	break;
      }
    }
    if (enumlist[i]==NULL) {
      err=3;
      goto errexit;
    }
    g_free(argv[0]);
    if ((p=g_malloc(sizeof(int)))==NULL) goto errexit;
    *(int *)(p)=i;
    argv[0] = p;
  }

  *argc=getargc(argv);
  g_free(s2);
  *rargv=argv;
  return 0;

errexit:
  arrayfree(array);
  g_free(s2);
  arg_del(argv);
  *argc=-1;
  *rargv=NULL;
  return err;
}
#endif

static void
freeargument(int type,const char *arglist,int argc,char **argv,int full)
{
  int i;

  if (argv == NULL) {
    return;
  }
  if (arglist==NULL) {
    for (i=0;i<argc;i++) g_free(argv[i]);
    g_free(argv);
  } else if (full) {
    if ((type!=NENUM) && (argc>0)
        && (arglist[0]!='\0') && (arglist[1]=='a')) {
      if ((arglist[0]=='i') || (arglist[0]=='d'))
        arrayfree((struct narray *)(argv[0]));
      else if (arglist[0]=='s')
        arrayfree2((struct narray *)(argv[0]));
    } else for (i=0;i<argc;i++) g_free(argv[i]);
    g_free(argv);
  } else {
    if ((type==NENUM) || (arglist[0]=='\0') || (arglist[1]!='a')) {
      for (i=0;i<argc;i++)
        if ((type==NENUM) || (strchr("bcid",arglist[i])!=NULL))
          g_free(argv[i]);
    }
    g_free(argv);
  }
}

int
isobject(char **s)
{
  char *po;
  int i;

  po=*s;
  for (i=0;(po[i]!='\0')&&(isalnum(po[i])||(strchr("_@",po[i])!=NULL));i++);
  if (po[i]!=':') return FALSE;
  for (i++;(po[i]!='\0')&&(isalnum(po[i])||(strchr("_./-,@!.",po[i])!=NULL));
       i++);
  if (po[i]!=':') return FALSE;
  if (po[i+1]=='\0') return FALSE;
  for (i++;(po[i]!='\0')&&(isalnum(po[i])||(strchr("_@",po[i])!=NULL));i++);
  *s=po+i;
  return TRUE;
}

int
schkobjfield(struct objlist *obj,int id, const char *field, char *arg,
                 char **valstr,int limittype,int cr,int quote)
{
  int err;
  char *val;
  int argc2;
  enum ngraph_object_field_type type;
  const char *arglist;
  char **argv2;
  char value[8];
  char *po;

  *valstr=NULL;
  val=arg;
  if (chkobjfield(obj,field)==-1) return -1;
  argv2=NULL;
  type=chkobjfieldtype(obj,field);
  if (limittype) {
    if ((type>NVFUNC) && (type!=NSFUNC) && (type!=NSAFUNC)) {
      return -1;
    }
  }
  if (type>=NVFUNC) {
    arglist=chkobjarglist(obj,field);
  } else {
    arglist="";
    type=NVOID;
  }
  po=value;
  err=getargument(type,arglist,val,&argc2,&argv2);
  if (err==1) return ERROEXTARG;
  else if (err==2) return ERROSMLARG;
  else if (err==3) return ERROVALUE;
  else if (err!=0) return -1;
  if (getobj(obj,field,id,argc2,argv2,po)==-1) err=4;
  freeargument(type,arglist,argc2,argv2,TRUE);
  if (err==0) {
    *valstr=getvaluestr(obj,field,po,cr,quote);
    if (*valstr==NULL) return -1;
    return 0;
  } else return -2;
}

int
sgetobjfield(struct objlist *obj,int id, const char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote)
{
  int err;
  char *val;
  int argc2;
  enum ngraph_object_field_type type;
  const char *arglist;
  char **argv2;
  char value[8];
  char *po;

  *valstr=NULL;
  val=arg;
  if (getobjfield(obj,field)==-1) return -1;
  argv2=NULL;
  type=chkobjfieldtype(obj,field);
  if (limittype) {
    if ((type>NVFUNC) && (type!=NSFUNC) && (type!=NSAFUNC)) {
      return -1;
    }
  }
  if (type>=NVFUNC) {
    arglist=chkobjarglist(obj,field);
  } else {
    arglist="";
    type=NVOID;
  }
  po=value;
  err=getargument(type,arglist,val,&argc2,&argv2);
  if (err==1) {
    error22(obj,ERROEXTARG,field,arg);
    return ERROEXTARG;
  } else if (err==2) {
    error22(obj,ERROSMLARG,field,arg);
    return ERROSMLARG;
  } else if (err==3) {
    error22(obj,ERROVALUE,field,arg);
    return ERROVALUE;
  } else if (err!=0) return -1;
  if (getobj(obj,field,id,argc2,argv2,po)==-1) err=4;
  freeargument(type,arglist,argc2,argv2,TRUE);
  if (err==0) {
    *valstr=getvaluestr(obj,field,po,cr,quote);
    if (*valstr==NULL) return -1;
    return 0;
  } else return -2;
}

static int
schkfield(struct objlist *obj,int id,char *arg,char **valstr,
              int limittype,int cr,int quote)
{
  int err;
  char *field;
  int len;
  char *s;

  s=arg;
  *valstr=NULL;
  len = 0;
  if ((s==NULL)
  || (strchr(":= \t",s[0])!=NULL)
  || ((field=getitok2(&s,&len,":= \t"))==NULL)) {
    if (len==-1) return -1;
    return ERRFIELD;
  }
  if (s[0]!='\0') s++;
  while ((s[0]==' ') || (s[0]=='\t')) s++;
  err=schkobjfield(obj,id,field,s,valstr,limittype,cr,quote);
  g_free(field);
  return err;
}

int
sgetfield(struct objlist *obj,int id,char *arg,char **valstr,
              int limittype,int cr,int quote)
{
  int err;
  char *field;
  int len;
  char *s;

  s=arg;
  *valstr=NULL;
  len = 0;
  if ((s==NULL)
  || (strchr(":= \t",s[0])!=NULL)
  || ((field=getitok2(&s,&len,":= \t"))==NULL)) {
    if (len==-1) return -1;
    error2(obj,ERRFIELD,arg);
    return ERRFIELD;
  }
  if (s[0]!='\0') s++;
  while ((s[0]==' ') || (s[0]=='\t')) s++;
  err=sgetobjfield(obj,id,field,s,valstr,limittype,cr,quote);
  g_free(field);
  return err;
}

struct narray *
sgetobj(char *arg,int limittype,int cr,int quote)
{
  struct objlist *obj;
  int i,anum,*id;
  struct narray iarray,*sarray;
  char *valstr;

  arrayinit(&iarray,sizeof(int));
  if ((sarray=arraynew(sizeof(char *)))==NULL) return NULL;
  if (chkobjilist2(&arg,&obj,&iarray,TRUE)) {
    arrayfree2(sarray);
    return NULL;
  }
  anum=arraynum(&iarray);
  id=arraydata(&iarray);
  for (i=0;i<anum;i++) {
    if (schkfield(obj,id[i],arg,&valstr,limittype,cr,quote)!=0) {
      arraydel(&iarray);
      arrayfree2(sarray);
      return NULL;
    }
    if (arrayadd(sarray,&valstr)==NULL) {
      arraydel(&iarray);
      arrayfree2(sarray);
      return NULL;
    }
  }
  arraydel(&iarray);
  return sarray;
}

int
sputobjfield(struct objlist *obj,int id, const char *field,char *arg)
{
  char *val;
  const char *arglist;
  int err;
  enum ngraph_object_field_type type;
  int argc2;
  char **argv2;

  val=arg;
  if (getobjfield(obj,field)==-1) return -1;
  if ((type=chkobjfieldtype(obj,field))<NVFUNC) {
    arglist=chkobjarglist(obj,field);
  } else {
    arglist="";
    type=NVOID;
  }
  err=getargument(type,arglist,val,&argc2,&argv2);
  if (err==1) {
    error22(obj,ERROEXTARG,field,arg);
    return ERROEXTARG;
  } else if (err==2) {
    error22(obj,ERROSMLARG,field,arg);
    return ERROSMLARG;
  } else if (err==3) {
    error22(obj,ERROVALUE,field,arg);
    return ERROVALUE;
  } else if (err!=0) return -1;
  if (argv2!=NULL) {
    if (putobj(obj,field,id,argv2[0])==-1) err=4;
  } else {
    if (putobj(obj,field,id,NULL)==-1) err=4;
  }
  if (err==0) {
    freeargument(type,arglist,argc2,argv2,FALSE);
    return 0;
  } else {
    freeargument(type,arglist,argc2,argv2,TRUE);
    return -2;
  }
}

int
sputfield(struct objlist *obj,int id,char *arg)
{
  char *field;
  char *s;
  int err;
  int len;

  s=arg;
  len = 0;
  if ((s==NULL)
  || (strchr(":=",s[0])!=NULL)
  || ((field=getitok2(&s,&len,":="))==NULL)) {
    if (len==-1) return -1;
    error2(obj,ERRFIELD,arg);
    return ERRFIELD;
  }
  if (s[0]!='\0') s++;
  err=sputobjfield(obj,id,field,s);
  g_free(field);
  return err;
}

int
sputobj(char *arg)
{
  struct objlist *obj;
  int i,anum,*id;
  struct narray iarray;

  arrayinit(&iarray,sizeof(int));
  if (getobjilist2(&arg,&obj,&iarray,TRUE)) return -1;
  anum=arraynum(&iarray);
  id=arraydata(&iarray);
  for (i=0;i<anum;i++)
    if (sputfield(obj,id[i],arg)!=0) {
      arraydel(&iarray);
      return -1;
    }
  arraydel(&iarray);
  return 0;
}

static int
sexeobjfield(struct objlist *obj,int id,const char *field,char *arg)
{
  char *val;
  int err;
  enum ngraph_object_field_type type;
  int argc2;
  const char *arglist;
  char **argv2;
  int rcode;

  val=arg;
  if (getobjfield(obj,field)==-1) return -1;
  if ((type=chkobjfieldtype(obj,field))>=NVFUNC) {
    arglist=chkobjarglist(obj,field);
  } else {
    arglist="";
    type=NVOID;
  }
  if (arglist && strcmp(arglist, "s")) {
    while ((val[0]==' ') || (val[0]=='\t')) {
      val++;
    }
  }
  err=getargument(type,arglist,val,&argc2,&argv2);
  if (err==1) {
    error22(NULL,ERROEXTARG,field,arg);
    return -1;
  } else if (err==2) {
    error22(obj,ERROSMLARG,field,arg);
    return -1;
  } else if (err==3) {
    error22(obj,ERROVALUE,field,arg);
    return -1;
  } else if (err!=0) return -1;
  rcode=exeobj(obj,field,id,argc2,argv2);
  freeargument(type,arglist,argc2,argv2,TRUE);
  return rcode;
}

int
sexefield(struct objlist *obj,int id,char *arg)
{
  char *field;
  int rcode;
  int len;
  char *s;

  s=arg;
  len = 0;
  if ((s==NULL)
      || (strchr(":= \t",s[0])!=NULL)
      || ((field=getitok2(&s,&len,":= \t"))==NULL)) {
    if (len==-1) return -1;
    error2(obj,ERRFIELD,arg);
    return -1;
  }
  if (s[0]!='\0') s++;
  rcode=sexeobjfield(obj,id,field,s);
  g_free(field);
  return rcode;
}

int
sexeobj(char *arg)
{
  struct objlist *obj;
  int i,anum,*id;
  struct narray iarray;
  int rcode;

  arrayinit(&iarray,sizeof(int));
  if (getobjilist2(&arg,&obj,&iarray,TRUE)) return -1;
  anum=arraynum(&iarray);
  id=arraydata(&iarray);
  rcode=0;
  for (i=0;i<anum;i++) {
    if ((rcode=sexefield(obj,id[i],arg))==-1) {
      arraydel(&iarray);
      return -1;
    }
  }
  arraydel(&iarray);
  return rcode;
}

void
obj_do_tighten(struct objlist *obj, N_VALUE *inst, const char *field)
{
  char *dest;
  struct narray iarray;
  struct objlist *dobj;

  if (_getobj(obj, field, inst, &dest))
    return;

  if (dest == NULL)
    return;

  arrayinit(&iarray, sizeof(int));
  if (! getobjilist(dest, &dobj, &iarray, FALSE, NULL)) {
    int anum;
    anum = arraynum(&iarray);
    if (anum > 0) {
      int id, oid;
      id = arraylast_int(&iarray);
      if (getobj(dobj, "oid", id, 0, NULL, &oid) != -1) {
        char *dest2;
	dest2 = (char *) g_malloc(strlen(chkobjectname(dobj)) + 10);
	if (dest2) {
	  sprintf(dest2, "%s:^%d", chkobjectname(dobj), oid);
	  _putobj(obj, field, inst, dest2);
	  g_free(dest);
	}
      }
    }
  }
  arraydel(&iarray);
}

void
obj_do_tighten_all(struct objlist *obj, N_VALUE *inst, const char *field)
{
  char *dest;
  struct narray iarray;
  struct objlist *dobj;
  GString *dest2;

  if (_getobj(obj, field, inst, &dest))
    return;

  if (dest == NULL)
    return;

  dest2 = g_string_sized_new(1024);
  if (dest2 == NULL) {
    return;
  }
  arrayinit(&iarray, sizeof(int));
  if (! getobjilist(dest, &dobj, &iarray, FALSE, NULL)) {
    char *ptr;
    int anum, oid, i;
    anum = arraynum(&iarray);
    g_string_printf(dest2, "%s:", chkobjectname(dobj));
    for (i = 0; i < anum; i++) {
      int id;
      id = arraynget_int(&iarray, i);
      if (getobj(dobj, "oid", id, 0, NULL, &oid) != -1) {
	g_string_append_printf(dest2, "%s^%d", (i == 0) ? "" : ",", oid);
      }
    }
    ptr = g_string_free(dest2, FALSE);
    _putobj(obj, field, inst, ptr);
  }
  arraydel(&iarray);
}

int
copy_obj_field(struct objlist *obj, int dist, int src, char **ignore_field)
{
  int perm, ignore, j;
  enum ngraph_object_field_type type;
  char **ptr;

  for (j = 0; j < chkobjfieldnum(obj); j++) {
    char *field;
    field = chkobjfieldname(obj, j);
    if (field == NULL) {
      continue;
    }
    perm = chkobjperm(obj, field);
    type = chkobjfieldtype(obj, field);
    ignore = FALSE;
    for (ptr = ignore_field; ptr && *ptr; ptr++) {
      if (strcmp2(field, *ptr) == 0) {
	ignore = TRUE;
	break;
      }
    }

    if (ignore)
      continue;

    if ((perm & NREAD) && (perm & NWRITE) && (type < NVFUNC)) {
      if (copyobj(obj, field, dist, src) == -1) {
	return 1;
      }
    }
  }

  return 0;
}

double
arg_to_double(char **argv, int index)
{
  void *ptr;
  double val;
  ptr = argv[index];
  val = * (double *) ptr;
  return val;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static char *
getuniqname(struct objlist *obj,char *prefix,char sep)
{
  int i,j,len;
  char *iname;
  N_VALUE *inst;
  char *name;
  int c[10];

  if (chkobjoffset(obj,"name")==-1) return NULL;
  if (prefix==NULL) len=0;
  else len=strlen(prefix);
  if (sep!='\0') len++;
  if ((name=g_malloc(len+11))==NULL) return NULL;
  for (i=0;i<10;i++) c[i]=0;
  while (TRUE) {
match:
    i=0;
    while (TRUE) {
      c[i]++;
      if (c[i]==26) {
        c[i]=1;
        i++;
        if (i==10) return NULL;
      } else break;
    }
    for (i=9;i>=0;i--) if (c[i]!=0) break;
    if (prefix!=NULL) strcpy(name,prefix);
    if (sep!='\0') name[len-1]=sep;
    for (j=len;j<=len+i;j++) name[j]=c[i-j+len]-1+'a';
    name[j]='\0';
    for (i=0;i<=obj->lastinst;i++) {
      if ((inst=chkobjinst(obj,i))==NULL) return NULL;
      if (_getobj(obj,"name",inst,&iname)==-1) return NULL;
      if ((iname!=NULL) && (strcmp0(iname,name)==0)) goto match;
    }
    break;
  }
  return name;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */
