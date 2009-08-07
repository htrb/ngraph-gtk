/* 
 * $Id: object.c,v 1.38 2009/08/07 02:52:40 hito Exp $
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "mathcode.h"
#include "mathfn.h"
#include "nhash.h"

#ifdef WINDOWS
#include <math.h>
#endif

#ifdef DEBUG
#ifdef WINDOWS
#include <dos.h>
#else
#include <unistd.h>
#endif
#endif

#define USE_HASH 1

#define TRUE  1
#define FALSE 0

#define OBJ_MAX 100
#define INST_MAX 32767

static struct objlist *objroot=NULL;
static struct loopproc *looproot=NULL, *loopnext=NULL;

static struct objlist *errobj=NULL;
static char errormsg1[256]={'\0'};
static char errormsg2[256]={'\0'};
static char errormsg[256]={'\0'};
static int errcode=0;
int GlobalLock=FALSE;

int (*getstdin)(void);
int (*putstdout)(char *s);
int (*putstderr)(char *s);
int (*printfstdout)(char *fmt,...);
int (*printfstderr)(char *fmt,...);
int (*ninterrupt)(void);
int (*inputyn)(char *mes);
void (*ndisplaydialog)(char *str);
void (*ndisplaystatus)(char *str);

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
     "illegal field identifile",
     "extra object arguments",
     "not enouph object argument",
     "illegal type of object argument",
     "instance exist. cannot overwrite object",
};

#define ERRNUM (sizeof(errorlist) / sizeof(*errorlist))

static int chkobjtblpos(struct objlist *obj,char *name, struct objlist **robj);

void 
error(struct objlist *obj,int code)
{
  char *objname;
  char **errtable;
  int errnum;

  GlobalLock=TRUE;
  errobj=obj;
  errcode=code;

  if (code < 0)
    code = ERRUNKNOWN;

  if (obj==NULL) objname="kernel";
  else objname=obj->name;
  if (code==ERRUNKNOWN)
    printfstderr("%.64s: %.64s%.64s%.64s\n",
                 objname,errormsg1,errormsg,errormsg2);
  else if (code<100) {
    errtable=errorlist;
    errnum=ERRNUM;
    if ((errtable==NULL) || (code>=errnum))
      printfstderr("%.64s: %.64s(%d)%.64s\n",objname,errormsg1,code,errormsg2);
    else
      printfstderr("%.64s: %.64s%.64s%.64s\n",
                   objname,errormsg1,errtable[code],errormsg2);
  } else {
    errtable=obj->errtable;
    errnum=obj->errnum;
    code=code-100;
    if ((errtable==NULL) || (code>=errnum))
      printfstderr("%.64s: %.64s(%d)%.64s\n",objname,errormsg1,code,errormsg2);
    else
      printfstderr("%.64s: %.64s%.64s%.64s\n",
                   objname,errormsg1,errtable[code],errormsg2);
  }
  errormsg1[0]='\0';
  errormsg2[0]='\0';
  errormsg[0]='\0';
  GlobalLock=FALSE;
}

void 
error2(struct objlist *obj,int code, const char *mes)
{
  if (mes!=NULL) {
    sprintf(errormsg2," `%.64s'.",mes);
  } else {
    sprintf(errormsg2,".");
  }
  error(obj,code);
}

void 
error22(struct objlist *obj,int code, const char *mes1, const char *mes2)
{
  if (mes1!=NULL) {
    sprintf(errormsg1,"%.64s: ",mes1);
  } else {
    errormsg1[0]='\0';
  }
  if (mes2!=NULL) {
    sprintf(errormsg2," `%.64s'.",mes2);
  } else {
    sprintf(errormsg2,".");
  }
  error(obj,code);
}

void 
error3(struct objlist *obj,int code,int num)
{
  sprintf(errormsg2," `%d'.",num);
  error(obj,code);
}

static int 
vgetchar(void)
{
  return EOF;
}

static int 
vputs(char *s)
{
  return 0;
}

static int 
vnprintf(char *fmt,...)
{
  return 0;
}

int 
seputs(char *s)
{
  return fputs(s,stderr);
}

int 
seprintf(char *fmt,...)
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
vinputyn(char *mes)
{
  return FALSE;
}

static void 
vdisplaydialog(char *str)
{
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void 
vdisplaywindow(char *str)
{
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

static void 
vdisplaystatus(char *str)
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


#ifdef DEBUG
struct plist *memallocroot=NULL;
int allocnum=0;
#endif

#ifdef HEAPCHK
extern int _heapchk(void);
#define _HEAPOK 2
#endif

void *
memalloc(size_t size)
{
  void *po;
#ifdef DEBUG
  struct plist *plnew;
#endif

#ifdef HEAPCHK
  if (_heapchk()!=_HEAPOK) exit(1);
#endif
  if (size==0) po=NULL;
  else po=malloc(size);
  if ((po==NULL) && (size!=0)) error(NULL,ERRHEAP);
#ifdef DEBUG
  if (po!=NULL) {
    plnew=malloc(sizeof(struct plist));
    plnew->next=memallocroot;
    plnew->val=po;
    memallocroot=plnew;
  }
#endif
  return po;
}

void *
memrealloc(void *ptr,size_t size)
{
  void *po;
#ifdef DEBUG
  struct plist *plcur,*plprev;
  struct plist *plnew;
#endif

#ifdef HEAPCHK
  if (_heapchk()!=_HEAPOK) exit(1);
#endif
  if (size==0)
    po=NULL;
  else
    po=realloc(ptr,size);
  if ((po == NULL) && (size != 0))
    error(NULL,ERRHEAP);

#ifdef DEBUG
  if (po != NULL) {
    if (ptr != NULL) {
      plcur=memallocroot;
      plprev=NULL;
      while (plcur!=NULL) {
        if (plcur->val==ptr) break;
        plprev=plcur;
        plcur=plcur->next;
      }
      if (plcur==NULL) {
        printfconsole("*%p\n",ptr);
	sleep(30);
        exit(1);
      }
      if (plprev==NULL) memallocroot=plcur->next;
      else plprev->next=plcur->next;
      free(plcur);
    }
    plnew=malloc(sizeof(struct plist));
    plnew->next=memallocroot;
    plnew->val=po;
    memallocroot=plnew;
  }
#endif
  return po;
}

void 
memfree(void *ptr)
{
#ifdef DEBUG
  struct plist *plcur,*plprev;

  if (ptr != NULL) {
    plcur = memallocroot;
    plprev = NULL;
    while (plcur != NULL) {
      if (plcur->val == ptr) break;
      plprev = plcur;
      plcur = plcur->next;
    }
    if (plcur == NULL) {
      printfconsole("*%p\n", ptr);
      sleep(30);
      exit(1);
    }
    if (plprev == NULL) {
      memallocroot=plcur->next;
    } else {
      plprev->next=plcur->next;
    }
    free(plcur);
  }
#endif
#ifdef HEAPCHK
  if (_heapchk()!=_HEAPOK) exit(1);
#endif
  if (ptr!=NULL) free(ptr);
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

  if ((array=memalloc(sizeof(struct narray)))==NULL) return NULL;
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
  memfree(array->data);
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
  for (i=0;i<array->num;i++) memfree(data[i]);
  memfree(array->data);
  array->data=NULL;
  array->size=0;
  array->num=0;
}

void 
arrayfree(struct narray *array)
{
  if (array==NULL) return;
  memfree(array->data);
  memfree(array);
}

void 
arrayfree2(struct narray *array)
{
  unsigned int i;
  char **data;

  if (array==NULL) return;
  data=array->data;
  for (i=0;i<array->num;i++) memfree(data[i]);
  memfree(array->data);
  memfree(array);
}

struct narray *
arrayadd(struct narray *array,void *val)
{
  int size,base;
  char *data;

  if (array==NULL) return NULL;
  if (array->num==array->size) {
    size=array->size+ALLOCSIZE;
    if ((data=memrealloc(array->data,array->base*size))==NULL) {
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
arrayadd2(struct narray *array,char **val)
{
  int size;
  char **data;
  char *s;

  if (array == NULL)
    return NULL;
  if (*val == NULL) {
    return NULL;
  } else {
    s = memalloc(strlen(*val) + 1);
    if (s == NULL) {
      arraydel2(array);
      return NULL;
    }
    strcpy(s, *val);
  }
  if (array->num == array->size) {
    size = array->size+ALLOCSIZE;
    data=memrealloc(array->data,array->base*size);
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
arrayins(struct narray *array,void *val,unsigned int idx)
{
  unsigned int i;
  int size,base;
  char *data;

  if (array==NULL) return NULL;
  if (idx>array->num) return NULL;
  if (array->num==array->size) {
    size=array->size+ALLOCSIZE;
    if ((data=memrealloc(array->data,array->base*size))==NULL) {
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
arrayins2(struct narray *array, char **val, unsigned int idx)
{
  unsigned int i;
  int size;
  char **data;
  char *s;

  if (array == NULL)
    return NULL;
  if (idx > array->num)
    return NULL;
  if (*val == NULL) {
    return NULL;
  } else {
    s = memalloc(strlen(*val) + 1);
    if (s == NULL) {
      arraydel2(array);
      return NULL;
    }
    strcpy(s, *val);
  }
  if (array->num == array->size) {
    size = array->size+ALLOCSIZE;
    data = memrealloc(array->data,array->base*size);
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
arrayndel2(struct narray *array,unsigned int idx)
{
  char **data;

  if (array==NULL) return NULL;
  if (idx>=array->num) return NULL;
  data=(char **)array->data;
  memfree(data[idx]);
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
arrayput(struct narray *array,void *val,unsigned int idx)
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
arrayput2(struct narray *array, char **val, unsigned int idx)
{
  char *s;
  char **data;

  if (array == NULL)
    return NULL;
  if (idx >= array->num)
    return NULL;
  if (*val == NULL){
    return NULL;
  } else {
    s = memalloc(strlen(*val) + 1);
    if (s == NULL) {
      arraydel2(array);
      return NULL;
    }
    strcpy(s, *val);
  }
  data = (char **)array->data;
  memfree(data[idx]);
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

static int
cmp_func_int(const void *p1, const void *p2)
{
  return (* (int *) p1) - (* (int *) p2);
}

void
arraysort_int(struct narray *array)
{
  int num, *adata;

  if (array == NULL)
    return;

  num = arraynum(array);
  adata = arraydata(array);
  if (num > 1)
    qsort(adata, num, sizeof(int), cmp_func_int);

}

void
arrayuniq_int(struct narray *array)
{
  int i, val, num, *adata;

  if (array == NULL)
    return;

  num = arraynum(array);
  if (num < 2)
    return;

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
    if ((*arg=memalloc(ARGBUFNUM*sizeof(void *)))==NULL) 
      return NULL;
    (*arg)[0]=NULL;
  }
  i=getargc(*arg);
  num=i/ARGBUFNUM;
  if (i%ARGBUFNUM==ARGBUFNUM-1) {
    if ((arg2=memrealloc(*arg,ARGBUFNUM*sizeof(void *)*(num+2)))==NULL) 
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
    if ((*arg=memalloc(ARGBUFNUM*sizeof(void *)))==NULL) 
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
  for (i=0;i<argc;i++) memfree(arg[i]);
  memfree(arg);
}

void 
registerevloop(char *objname,char *evname,
                    struct objlist *obj,int idn,char *inst,
                    void *local)
{
  struct loopproc *lpcur,*lpnew;

  if (obj==NULL) return;
  if ((lpnew=memalloc(sizeof(struct loopproc)))==NULL) return;
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
unregisterevloop(struct objlist *obj,int idn,char *inst)
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
      memfree(lpdel);
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
  struct loopproc *lpcur,*lpdel;

  lpcur=looproot;
  while (lpcur!=NULL) {
    lpdel=lpcur;
    lpcur=lpcur->next;
    memfree(lpdel);
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
    argv[0]=lpcur->objname;
    argv[1]=lpcur->evname;
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
	memfree(aliasname);
	return 1;
      }
      memfree(aliasname);
    }
  }

  return 0;
}
#endif

void *
addobject(char *name,char *alias,char *parentname,char *ver,
                int tblnum,struct objtable *table,
                int errnum,char **errtable,void *local,DoneProc doneproc)
/* addobject() returns NULL on error */
{
  struct objlist *objcur,*objprev,*objnew,*parent, *ptr;
  int i,offset;
  NHASH tbl_hash = NULL;
  static int id = 0;

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
  if ((objnew=memalloc(sizeof(struct objlist)))==NULL) return NULL;

#if USE_HASH
  if (add_obj_to_hash(name, alias, objnew)) {
    memfree(objnew);
    error2(NULL,ERRHEAP,name);
    return NULL;
  }

  tbl_hash = nhash_new();
  if (tbl_hash == NULL) {
    memfree(objnew);
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
  objnew->local=local;
  objnew->doneproc=doneproc;
  if (parent==NULL) offset=0;
  else offset=parent->size;
  if (offset % ALIGNSIZE != 0) offset = offset + (ALIGNSIZE - offset % ALIGNSIZE);
  for (i=0;i<tblnum;i++) {
    table[i].offset=offset;
    switch (table[i].type) {
    case NVOID:
    case NLABEL:
    case NVFUNC:
      break;
    case NBOOL: case NBFUNC:
    case NINT:  case NIFUNC:
    case NCHAR: case NCFUNC:
    case NENUM:
      offset+=sizeof(int);
      break;
    case NDOUBLE: case NDFUNC:
      offset+=sizeof(double);
      break;
    default:
      offset+=sizeof(void *);
    }
    if (offset % ALIGNSIZE != 0) offset = offset + (ALIGNSIZE - offset % ALIGNSIZE);
    if (table[i].attrib & NEXEC) table[i].attrib&=~NWRITE;
#if USE_HASH
    if (nhash_set_int(tbl_hash, table[i].name, i)) {
      memfree(objnew);
      nhash_free(tbl_hash);
      error2(NULL,ERRHEAP,name);
      return NULL;
    }
#endif
  }
  objnew->size=offset;
  objnew->idp=chkobjoffset(objnew,"id");
  objnew->oidp=chkobjoffset(objnew,"oid");
  objnew->nextp=chkobjoffset(objnew,"next");

  id++;

  return objnew;
}

void 
hideinstance(struct objlist *obj)
{
  char *instcur,*instprev;
  int nextp,idp;

  if ((idp=obj->idp)==-1) return;
  if ((nextp=obj->nextp)==-1) return;
  if (obj->lastinst==-1) return;
  if (obj->lastinst2==-1) {
    obj->root2=obj->root;
    obj->lastinst2=obj->lastinst;
  } else {
    instcur=obj->root;
    while (instcur!=NULL) {
      *(int *)(instcur+idp)+=obj->lastinst2+1;
      instcur=*(char **)(instcur+nextp);
    }
    instcur=obj->root2;
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=*(char **)(instcur+nextp);
    }
    *(char **)(instprev+nextp)=obj->root;
    obj->lastinst2+=obj->lastinst+1;
  }
  obj->root=NULL;
  obj->lastinst=-1;
}

void 
recoverinstance(struct objlist *obj)
{
  char *instcur,*instprev;
  int nextp,idp;

  if ((idp=obj->idp)==-1) return;
  if ((nextp=obj->nextp)==-1) return;
  if (obj->lastinst2==-1) return;
  if (obj->lastinst==-1) {
    obj->root=obj->root2;
    obj->lastinst=obj->lastinst2;
  } else {
    instcur=obj->root;
    while (instcur!=NULL) {
      *(int *)(instcur+idp)+=obj->lastinst2+1;
      instcur=*(char **)(instcur+nextp);
    }
    instcur=obj->root2;
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=*(char **)(instcur+nextp);
    }
    *(char **)(instprev+nextp)=obj->root;
    obj->root=obj->root2;
    obj->lastinst+=obj->lastinst2+1;
  }
  obj->root2=NULL;
  obj->lastinst2=-1;
}

struct objlist *
chkobject(char *name)
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

char *
chkobjectname(struct objlist *obj)
{
  if (obj==NULL) return NULL;
  return obj->name;
}

char *
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
  return obj->size;
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
chkobjoffset(struct objlist *obj,char *name)
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

char *
chkobjinstoid(struct objlist *obj,int oid)
/* chkobjinstoid() returns NULL when instance is not found */
{
  int oidp,nextp;
  char *inst;

  if ((oidp=obj->oidp)==-1) return NULL;
  inst=obj->root;
  if ((nextp=obj->nextp)==-1) {
    if (inst==NULL) return NULL;
    if (*(int *)(inst+oidp)==oid) return inst;
    else return NULL;
  } else {
    while (inst!=NULL) {
      if (*(int *)(inst+oidp)==oid) return inst;
      inst=*(char **)(inst+nextp);
    }
  }
  return NULL;
}

static int 
chkobjtblpos(struct objlist *obj,char *name,
                 struct objlist **robj)
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

char *
chkobjinst(struct objlist *obj,int id)
/* chkobjinst() returns NULL if instance is not found */
{
  int i,nextp;
  char *instcur;

  instcur=obj->root;
  i=0;
  if ((nextp=obj->nextp)==-1) {
    if ((instcur==NULL) || (id!=0)) {
      return NULL;
    }
  } else {
    while ((instcur!=NULL) && (id!=i)) {
      instcur=*(char **)(instcur+nextp);
      i++;
    }
    if (instcur==NULL) {
      return NULL;
    }
  }
  return instcur;
}

char *
chkobjlast(struct objlist *obj)
/* chkobjlast() returns NULL if instance is not found */
{
  char *instcur,*instprev;
  int nextp;

  instcur=obj->root;
  nextp=obj->nextp;
  if (nextp!=-1) {
    instprev=NULL;
    while (instcur!=NULL) {
      instprev=instcur;
      instcur=*(char **)(instcur+nextp);
    }
    instcur=instprev;
  }
  return instcur;
}

static char *
chkobjprev(struct objlist *obj,int id,char **inst,char **prev)
/* chkobjprev() returns NULL if instance is not found */
{
  char *instcur,*instprev;
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
      instcur=*(char **)(instcur+nextp);
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
  char *inst;

  if ((oidp=obj->oidp)==-1) return -1;
  if ((idp=obj->idp)==-1) return -1;
  inst=obj->root;
  if ((nextp=obj->nextp)==-1) {
    if (inst==NULL) return -1;
    if (*(int *)(inst+oidp)==oid) return *(int *)(inst+idp);
    else return -1;
  } else {
    while (inst!=NULL) {
      if (*(int *)(inst+oidp)==oid) return *(int *)(inst+idp);
      inst=*(char **)(inst+nextp);
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
  char *inst;

  if (id==NULL) id2=0;
  else id2=*id;
  if (chkobjoffset(obj,"name")==-1) return -1;
  for (i=id2;i<=obj->lastinst;i++) {
    if ((inst=chkobjinst(obj,i))==NULL) return -1;
    if (_getobj(obj,"name",inst,&iname)==-1) return -1;
    if ((iname!=NULL) && (strcmp0(iname,name)==0)) {
      if (id!=NULL) *id=i+1;
      return i;
    }
  }
  if (id!=NULL) *id=obj->lastinst+1;
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
chkobjfield(struct objlist *obj,char *name)
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
chkobjperm(struct objlist *obj,char *name)
/* chkobjperm() returns 0 on error */
{
  struct objlist *robj;
  int idn;

  if ((idn=chkobjtblpos(obj,name,&robj))==-1) return 0;
  return robj->table[idn].attrib;
}

int 
chkobjfieldtype(struct objlist *obj,char *name)
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

char *
chkobjarglist(struct objlist *obj,char *name)
{
  int namen,type;
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
    case NCHAR:
      arglist="c";
      break;
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
getobject(char *name)
/* getobject() returns NULL when the named object is not found */
{
  struct objlist *obj;

  if ((obj=chkobject(name))==NULL) error2(NULL,ERROBJFOUND,name);
  return obj;
}

char *
getobjver(char *name)
/* getobjver() returns NULL when the named object is not found */
{
  struct objlist *obj;

  if ((obj=getobject(name))==NULL) return NULL;
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
getobjoffset(struct objlist *obj,char *name)
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
getobjtblpos(struct objlist *obj,char *name,struct objlist **robj)
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

char *
getobjinst(struct objlist *obj,int id)
/* getobjinst() returns NULL if instance is not found */
{
  char *instcur;

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

static char *
getobjprev(struct objlist *obj,int id,char **inst,char **prev)
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

char *
getobjinstoid(struct objlist *obj,int oid)
{
  char *inst;

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
  char *inst;

  if (id==NULL) id2=0;
  else id2=*id;
  if (getobjoffset(obj,"name")==-1) return -1;
  for (i=id2;i<=obj->lastinst;i++) {
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
getobjfield(struct objlist *obj,char *name)
{
  if (chkobjfield(obj,name)==-1) {
    error2(obj,ERRVALFOUND,name);
    return -1;
  } else return 0;
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

static int 
_newobj(struct objlist *obj)
/* _newobj() returns id or -1 on error */
{
  char *instcur,*instnew,*inst;
  int i,offset,nextp,id,idp,oidp;
  struct objlist *objcur;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  id=obj->lastinst+1;
  if ((id+obj->lastinst2+1)==INST_MAX) {
    error3(obj,ERRINSTNUM,INST_MAX);
    return -1;
  }
  nextp=obj->nextp;
  if ((instcur=chkobjlast(obj))!=NULL) {
    if (nextp==-1) {
      error(obj,ERRNONEXT);
      return -1;
    }
  }
  instnew=memalloc(obj->size);
  if (instnew==NULL) return -1;
  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++) {
      offset=objcur->table[i].offset;
      switch (objcur->table[i].type) {
      case NVOID:
      case NLABEL:
      case NVFUNC:
        break;
      case NBOOL:
      case NCHAR:
      case NINT:
      case NENUM:
      case NBFUNC:
      case NCFUNC:
      case NIFUNC:
        *(int *)(instnew+offset)=0;
        break;
      case NDOUBLE:
      case NDFUNC:
        *(double *)(instnew+offset)=0.0;
        break;
      default:
        *(char **)(instnew+offset)=NULL;
        break;
      }
    }
    objcur=objcur->parent;
  }
  *(int *)(instnew+idp)=id;
  if ((oidp=obj->oidp)!=-1) {
    if (obj->lastoid==INT_MAX) obj->lastoid=0;
    else obj->lastoid++;
    if (nextp!=-1) {
      do {
        inst=obj->root;
        while (inst!=NULL) {
          if (*(int *)(inst+oidp)==obj->lastoid) {
            if (obj->lastoid==INT_MAX) obj->lastoid=0;
            else obj->lastoid++;
            break;
          }
          inst=*(char **)(inst+nextp);
        }
        if (inst==NULL) {
          inst=obj->root2;
          while (inst!=NULL) {
            if (*(int *)(inst+oidp)==obj->lastoid) {
              if (obj->lastoid==INT_MAX) obj->lastoid=0;
              else obj->lastoid++;
              break;
            }
            inst=*(char **)(inst+nextp);
          }
        }
      } while (inst!=NULL);
    }
    *(int *)(instnew+oidp)=obj->lastoid;
  }

  if (instcur==NULL) obj->root=instnew;
  else *(char **)(instcur+nextp)=instnew;
  if (nextp!=-1) *(char **)(instnew+nextp)=NULL;
  obj->lastinst=id;
  return id;
}

int 
newobj(struct objlist *obj)
/* newobj() returns id or -1 on error */
{
  struct objlist *robj;
  char *instcur,*instnew,*inst;
  int i,offset,nextp,id,idp,oidp,rcode,initn,initp;
  int argc;
  char **argv;
  struct objlist *objcur;

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

  instnew = memalloc(obj->size);
  if (instnew == NULL)
    return -1;

  objcur = obj;
  while (objcur) {
    for (i = 0; i < objcur->tblnum; i++) {
      offset = objcur->table[i].offset;
      switch (objcur->table[i].type) {
      case NVOID:
      case NLABEL:
      case NVFUNC:
        break;
      case NBOOL:
      case NCHAR:
      case NINT:
      case NENUM:
      case NBFUNC:
      case NCFUNC:
      case NIFUNC:
        *(int *)(instnew + offset) = 0;
        break;
      case NDOUBLE:
      case NDFUNC:
        *(double *)(instnew + offset) = 0.0;
        break;
      default:
        *(char **)(instnew + offset) = NULL;
        break;
      }
    }
    objcur = objcur->parent;
  }
  *(int *)(instnew + idp) = id;
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
          if (*(int *)(inst + oidp) == obj->lastoid) {
            if (obj->lastoid == INT_MAX) {
	      obj->lastoid=0;
            } else {
	      obj->lastoid++;
	    }
            break;
          }
          inst = *(char **)(inst + nextp);
        }
        if (inst == NULL) {
          inst = obj->root2;
          while (inst) {
            if (*(int *)(inst + oidp) == obj->lastoid) {
              if (obj->lastoid == INT_MAX) {
		obj->lastoid = 0;
              } else {
		obj->lastoid++;
	      }
              break;
            }
            inst = *(char **)(inst + nextp);
          }
        }
      } while (inst);
    }
    *(int *)(instnew + oidp) = obj->lastoid;
  }
  if (robj->table[initn].proc) {
    argv = NULL;
    if (arg_add2(&argv, 2, obj->name, "init") == NULL) {
      memfree(argv);
      return -1;
    }
    argc = getargc(argv);
    rcode = robj->table[initn].proc(robj, instnew, instnew + initp, argc, argv);
    memfree(argv);
    if (rcode != 0) {
      memfree(instnew);
      return -1;
    }
  }
  if (instcur == NULL) {
    obj->root=instnew;
  } else {
    *(char **)(instcur + nextp) = instnew;
  }
  if (nextp != -1) {
    *(char **)(instnew + nextp) = NULL;
  }
  obj->lastinst = id;
  obj->curinst = id;
  return id;
}

static int
_delobj(struct objlist *obj,int delid)
/* delobj() returns id or -1 on error */
{
  char *instcur,*instprev,*inst;
  int nextp,idp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjprev(obj,delid,&instcur,&instprev)==NULL) return -1;
  if ((nextp=obj->nextp)==-1) obj->root=NULL;
  else {
    if (instprev==NULL) obj->root=*(char **)(instcur+nextp);
    else *(char **)(instprev+nextp)=*(char **)(instcur+nextp);
    inst=*(char **)(instcur+nextp);
    while (inst!=NULL) {
      (*(int *)(inst+idp))--;
      inst=*(char **)(inst+nextp);
    }
  }
  memfree(instcur);
  obj->lastinst--;
  return 0;
}

int 
delobj(struct objlist *obj,int delid)
/* delobj() returns id or -1 on error */
{
  struct objlist *robj,*objcur;
  char *instcur,*instprev,*inst;
  int i,nextp,idp,donen,donep,rcode,offset;
  int argc;
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
    argv=NULL;
    if (arg_add2(&argv,2,obj->name,"done")==NULL) {
      memfree(argv);
      return -1;
    }
    argc=getargc(argv);
    rcode=robj->table[donen].proc(robj,instcur,instcur+donep,argc,argv);
    memfree(argv);
    if (rcode!=0) return -1;
  }
  if ((nextp=obj->nextp)==-1) obj->root=NULL;
  else {
    if (instprev==NULL) obj->root=*(char **)(instcur+nextp);
    else *(char **)(instprev+nextp)=*(char **)(instcur+nextp);
    inst=*(char **)(instcur+nextp);
    while (inst!=NULL) {
      (*(int *)(inst+idp))--;
      inst=*(char **)(inst+nextp);
    }
    *(char **)(instcur+nextp)=NULL;
  }
  objcur=obj;
  while (objcur!=NULL) {
    for (i=0;i<objcur->tblnum;i++) {
      offset=objcur->table[i].offset;
      switch (objcur->table[i].type) {
      case NPOINTER:
      case NSTR:
      case NOBJ:
      case NSFUNC:
        memfree(*(char **)(instcur+offset));
        break;
      case NIARRAY: case NIAFUNC:
      case NDARRAY: case NDAFUNC:
        array=*(struct narray **)(instcur+offset);
        arrayfree(array);
        break;
      case NSARRAY: case NSAFUNC:
        array=*(struct narray **)(instcur+offset);
        arrayfree2(array);
        break;
      default:
        break;
      }
    }
    objcur=objcur->parent;
  }
  memfree(instcur);
  obj->lastinst--;
  obj->curinst=-1;
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
_putobj(struct objlist *obj,char *vname,char *inst,void *val)
{
  struct objlist *robj;
  int idp,idn;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  switch (robj->table[idn].type) {
  case NVOID:
  case NLABEL:
  case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT: case NIFUNC:
  case NCHAR: case NCFUNC:
  case NENUM:
    *(int *)(inst+idp)=*(int *)val;
    break;
  case NDOUBLE: case NDFUNC:
    *(double *)(inst+idp)=*(double *)val;
    break;
  default:
    *(char **)(inst+idp)=(char *)val;
    break;
  }
  return 0;
}

int 
putobj(struct objlist *obj,char *vname,int id,void *val)
/* putobj() returns id or -1 on error */
{
  struct objlist *robj;
  struct narray *array;
  char *instcur;
  int idp,idn,rcode,argc;
  char **argv;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);

  if ((instcur=getobjinst(obj,id))==NULL) return -1;

  if ((robj->table[idn].attrib & NWRITE)==0) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }

  if ((robj->table[idn].type<NVFUNC) && (robj->table[idn].proc!=NULL)) {
    argv=NULL;
    if (arg_add2(&argv,3,obj->name,vname,val)==NULL) {
      memfree(argv);
      return -1;
    }
    argc=getargc(argv);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc,argv);
    val=argv[2];
    memfree(argv);
    if (rcode!=0) return -1;
  }

  switch (robj->table[idn].type) {
  case NSTR:
  case NPOINTER:
  case NOBJ:
  case NSFUNC:
    memfree(*(char **)(instcur+idp));
    break;
  case NIARRAY: case NIAFUNC:
  case NDARRAY: case NDAFUNC:
    array=*(struct narray **)(instcur+idp);
    arrayfree(array);
    break;
  case NSARRAY: case NSAFUNC:
    array=*(struct narray **)(instcur+idp);
    arrayfree2(array);
    break;
  default:
    break;
  }

  switch (robj->table[idn].type) {
  case NVOID:
  case NLABEL:
  case NVFUNC:
    break;
  case NBOOL:
  case NINT:
  case NCHAR:
  case NENUM:
        *(int *)(instcur+idp)=*(int *)val;
        break;
  case NDOUBLE:
        *(double *)(instcur+idp)=*(double *)val;
        break;
  default:
        *(char **)(instcur+idp)=(char *)val;
        break;
  }
  obj->curinst=id;
  return id;
}

int 
_getobj(struct objlist *obj,char *vname,char *inst,void *val)
{
  struct objlist *robj;
  int idp,idn;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  switch (robj->table[idn].type) {
  case NVOID: case NLABEL: case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT:  case NIFUNC:
  case NCHAR: case NCFUNC:
  case NENUM:
    *(int *)val=*(int *)(inst+idp);
    break;
  case NDOUBLE: case NDFUNC:
    *(double *)val=*(double *)(inst+idp);
    break;
  default:
    *(char **)val=*(char **)(inst+idp);
    break;
  }
  return 0;
}

int 
getobj(struct objlist *obj,char *vname,int id,
           int argc,char **argv,void *val)
/* getobj() returns id or -1 on error */
{
  struct objlist *robj;
  char *instcur;
  int i,idp,idn;
  int argc2,rcode;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if ((instcur=getobjinst(obj,id))==NULL) return -1;
  if (((robj->table[idn].attrib & NREAD)==0)
  || ( (robj->table[idn].type>=NVFUNC)
   && ((robj->table[idn].attrib & NEXEC)==0))) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }
  if ((robj->table[idn].type>=NVFUNC) && (robj->table[idn].proc!=NULL)) {
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      memfree(argv2);
      return -1;
    }
    for (i=0;i<argc;i++) {
      if (arg_add(&argv2,((char **)argv)[i])==NULL) {
        memfree(argv2);
        return -1;
      }
    }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc2,argv2);
    memfree(argv2);
    if (rcode!=0) return -1;
  }
  switch (robj->table[idn].type) {
  case NVOID: case NLABEL: case NVFUNC:
    break;
  case NBOOL: case NBFUNC:
  case NINT: case NIFUNC:
  case NCHAR: case NCFUNC:
  case NENUM:
    *(int *)val=*(int *)(instcur+idp);
    break;
  case NDOUBLE: case NDFUNC:
    *(double *)val=*(double *)(instcur+idp);
    break;
  default:
    *(char **)val=*(char **)(instcur+idp);
    break;
  }
  obj->curinst=id;
  return id;
}

int 
_exeparent(struct objlist *obj,char *vname,char *inst,char *rval,
               int argc,char **argv)
/* _exeparent() returns errorlevel or -1 on error */
{
  struct objlist *parent,*robj;
  int idn,idp,rcode;
  char *rval2;

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
__exeobj(struct objlist *obj,int idn,char *inst,int argc,char **argv)
/* __exeobj() returns errorlevel or -1 on error */
{
  int rcode,idp;

  if (obj->table[idn].type<NVFUNC) return -1;
  if (obj->table[idn].proc!=NULL) {
    idp=chkobjoffset2(obj,idn);
    rcode=obj->table[idn].proc(obj,inst,inst+idp,argc,argv);
  } else rcode=0;
  return rcode;
}

int 
_exeobj(struct objlist *obj,char *vname,char *inst,int argc,char **argv)
/* _exeobj() returns errorlevel or -1 on error */
{
  struct objlist *robj;
  int i,idn,idp,rcode;
  int argc2;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if (robj->table[idn].type<NVFUNC) return -1;
  if (robj->table[idn].proc!=NULL) {
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      memfree(argv2);
      return -1;
    }
    for (i=0;i<argc;i++)
      if (arg_add(&argv2,((char **)argv)[i])==NULL) {
        memfree(argv2);
        return -1;
      }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,inst,inst+idp,argc2,argv2);
    memfree(argv2);
  } else rcode=0;
  return rcode;
}

int 
exeobj(struct objlist *obj,char *vname,int id,int argc,char **argv)
/* exeobj() returns errorlevel or -1 on error */
{
  struct objlist *robj;
  char *instcur;
  int i,idn,idp,rcode;
  int argc2;
  char **argv2;

  if ((idn=getobjtblpos(obj,vname,&robj))==-1) return -1;
  idp=chkobjoffset2(robj,idn);
  if ((instcur=getobjinst(obj,id))==NULL) return -1;
  if ((robj->table[idn].type<NVFUNC)
  || ((robj->table[idn].attrib & NREAD)==0)
  || ( (robj->table[idn].type>=NVFUNC)
   && ((robj->table[idn].attrib & NEXEC)==0))) {
    error2(obj,ERRPERMISSION,vname);
    return -1;
  }
  if (robj->table[idn].proc!=NULL) {
    argv2=NULL;
    if (arg_add2(&argv2,2,obj->name,vname)==NULL) {
      memfree(argv2);
      return -1;
    }
    for (i=0;i<argc;i++)
      if (arg_add(&argv2,((char **)argv)[i])==NULL) {
        memfree(argv2);
        return -1;
      }
    argc2=getargc(argv2);
    rcode=robj->table[idn].proc(robj,instcur,instcur+idp,argc2,argv2);
    memfree(argv2);
  } else rcode=0;
  obj->curinst=id;
  return rcode;
}

int 
copyobj(struct objlist *obj,char *vname,int did,int sid)
{
  struct objlist *robj;
  unsigned int i;
  int idn;
  char value[8];
  char *po;
  struct narray *array;
  char *s;

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
  case NLABEL:
  case NBOOL:
  case NINT:
  case NCHAR:
  case NENUM:
  case NDOUBLE:
    if (putobj(obj,vname,did,po)==-1) return -1;
    break;
  case NSTR:
  case NOBJ:
    if (*(char **)po==NULL) {
      if (putobj(obj,vname,did,NULL)==-1) return -1;
    } else {
      if ((s=memalloc(strlen(*(char **)po)+1))==NULL) return -1;
      strcpy(s,*(char **)po);
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
        if (arrayadd2(array,arraynget(*(struct narray **)po,i))==NULL) {
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

static int 
_copyobj(struct objlist *obj,int did,int sid)
/* _copyobj() returns id or -1 on error */
/* this function shoud not be used becase this function do "shallow" copy. */
{
  char *dinstcur;
  char *sinstcur,*instnext;
  int idp,id,nextp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if ((sinstcur=getobjinst(obj,sid))==NULL) return -1;
  if (did==sid) return did;
  if ((dinstcur=chkobjinst(obj,did))==NULL) {
    if ((did=newobj(obj))==-1) return -1;
    if ((dinstcur=getobjinst(obj,did))==NULL) return -1;
  }
  id=*(int *)(dinstcur+idp);
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  instnext=*(char **)(dinstcur+nextp);
  memcpy(dinstcur,sinstcur,obj->size);
  *(int *)(dinstcur+idp)=id;
  *(char **)(dinstcur+nextp)=instnext;
  obj->curinst=did;
  return did;
}

int 
moveobj(struct objlist *obj,int did,int sid)
/* moveobj() returns id or -1 on error */
{
  char *dinstcur;
  int id,idp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if (getobjid(obj,sid)==-1) return -1;
  if (did==sid) return did;
  if ((dinstcur=chkobjinst(obj,did))==NULL) {
    if ((id=_newobj(obj))==-1) return -1;
    if (_copyobj(obj,id,sid)==-1) {
      _delobj(obj,id);
      return -1;
    }
    if (_delobj(obj,sid)==-1) {
      _delobj(obj,id);
      return -1;
    }
  } else {
    exchobj(obj,did,sid);
    if (delobj(obj,sid)==-1) {
      exchobj(obj,did,sid);
      return -1;
    }
  }
  obj->curinst=*(int *)(dinstcur+idp);
  return *(int *)(dinstcur+idp);
}

int 
moveupobj(struct objlist *obj,int id)
/* moveupobj() returns id or -1 on error */
{
  char *instcur,*instprev,*instcur2,*instprev2;
  char *inst;
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
  inst=*(char **)(instcur+nextp);
  if (instprev2==NULL) obj->root=instcur;
  else *(char **)(instprev2+nextp)=instcur;
  *(char **)(instcur+nextp)=instprev;
  if (instprev==NULL) obj->root=inst;
  else *(char **)(instprev+nextp)=inst;
  (*(int *)(instcur+idp))--;
  (*(int *)(instprev+idp))++;
  obj->curinst=id-1;
  return id-1;
}

int 
movetopobj(struct objlist *obj,int id)
/* movetopobj() returns id or -1 on error */
{
  char *instcur,*instprev;
  char *rinst,*pinst,*inst;
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
  inst=*(char **)(instcur+nextp);
  pinst=*(char **)(instprev+nextp);
  obj->root=pinst;
  *(char **)(instcur+nextp)=rinst;
  *(char **)(instprev+nextp)=inst;
  *(int *)(instcur+idp)=0;
  instcur=rinst;
  while (instcur!=inst) {
    (*(int *)(instcur+idp))++;
    instcur=*(char **)(instcur+nextp);
  }
  obj->curinst=0;
  return 0;
}

int 
movedownobj(struct objlist *obj,int id)
/* movedownobj() returns id or -1 on error */
{
  char *instcur,*instprev;
  char *ninst,*inst;
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
  inst=*(char **)(instcur+nextp);
  ninst=*(char **)(inst+nextp);
  if (instprev==NULL) obj->root=inst;
  else *(char **)(instprev+nextp)=inst;
  *(char **)(inst+nextp)=instcur;
  *(char **)(instcur+nextp)=ninst;
  (*(int *)(instcur+idp))++;
  (*(int *)(inst+idp))--;
  obj->curinst=id+1;
  return id+1;
}

int 
movelastobj(struct objlist *obj,int id)
/* movelastobj() returns id or -1 on error */
{
  char *instcur,*instprev,*lastinst;
  char *pinst,*inst;
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
  inst=*(char **)(instcur+nextp);
  if (instprev==NULL) pinst=obj->root;
  else pinst=*(char **)(instprev+nextp);
  *(char **)(lastinst+nextp)=pinst;
  *(char **)(instcur+nextp)=NULL;
  if (instprev==NULL) obj->root=inst;
  else *(char **)(instprev+nextp)=inst;
  *(int *)(instcur+idp)=lid;
  while (inst!=instcur) {
    (*(int *)(inst+idp))--;
    inst=*(char **)(inst+nextp);
  }
  obj->curinst=lid;
  return lid;
}

int 
exchobj(struct objlist *obj,int id1,int id2)
/* exchobj() returns id or -1 on error */
{
  char *instcur1,*instprev1;
  char *instcur2,*instprev2;
  char *inst,*inst1,*inst2;
  int idp,id,nextp;

  if ((idp=obj->idp)==-1) {
    error(obj,ERRNOID);
    return -1;
  }
  if ((getobjprev(obj,id1,&instcur1,&instprev1)==NULL)
   || (getobjprev(obj,id2,&instcur2,&instprev2)==NULL)) return -1;
  if (id1==id2) return id1;
  if ((nextp=obj->nextp)==-1) {
    error2(obj,ERRVALFOUND,"next");
    return -1;
  }
  id=*(int *)(instcur1+idp);
  *(int *)(instcur1+idp)=*(int *)(instcur2+idp);
  *(int *)(instcur2+idp)=id;
  if (instprev1==NULL) inst1=obj->root;
  else inst1=*(char **)(instprev1+nextp);
  if (instprev2==NULL) inst2=obj->root;
  else inst2=*(char **)(instprev2+nextp);
  if (instprev1==NULL) obj->root=inst2;
  else *(char **)(instprev1+nextp)=inst2;
  if (instprev2==NULL) obj->root=inst1;
  else *(char **)(instprev2+nextp)=inst1;
  inst=*(char **)(instcur1+nextp);
  *(char **)(instcur1+nextp)=*(char **)(instcur2+nextp);
  *(char **)(instcur2+nextp)=inst;
  obj->curinst=id2;
  return id2;
}

/* 
char *saveobj(struct objlist *obj, int id)
{
  char *instcur,*instnew;

  if ((instcur=getobjinst(obj,id))==NULL) return NULL;
  if ((instnew=memalloc(obj->size))==NULL) return NULL;
  memcpy(instnew,instcur,obj->size);
  return instnew;
}

char *restoreobj(struct objlist *obj,int id,char *image)
{
  char *instcur;

  if (obj==NULL) return NULL;
  if ((instcur=getobjinst(obj,id))==NULL) return NULL;
  memcpy(instcur,image,obj->size);
  memfree(image);
  return instcur;
}
*/

static int 
chkilist(struct objlist *obj,char *ilist,struct narray *iarray,int def,int *spc)
/* spc  0: not found
        1: specified by id
        2: specified by oid
        3: specified by name
        4: specified by other
*/

{
  int i,len,snum,dnum,num,sid,l;
  int oid;
  char *tok,*s,*iname,*endptr;

  *spc=0;
  num=0;
  tok=NULL;
  if ((ilist==NULL) || (ilist[0]=='\0')) {
    if (def) {
      if ((snum=chkobjcurinst(obj))==-1) return -1;
      if (arrayadd(iarray,&snum)==NULL) goto errexit;
      num++;
      *spc=4;
    }
  } else {
    while ((s=getitok2(&ilist,&len," \t,"))!=NULL) {
      memfree(tok);
      tok=s;
      iname=NULL;
      if (s[0]=='@') {
        if ((snum=chkobjcurinst(obj))==-1) goto errexit;
        s++;
        *spc=4;
      } else if (s[0]=='!') {
        if ((snum=chkobjlastinst(obj))==-1) goto errexit;
        s++;
        *spc=4;
      } else {
        if (s[0]=='^') {
          oid=TRUE;
          s++;
        } else oid=FALSE;
        l=strtol(s,&endptr,10);
        if (s!=endptr) {
          if (oid) {
            snum=chkobjoid(obj,l);
            *spc=2;
          } else {
            snum=chkobjid(obj,l);
            *spc=1;
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
          *spc=4;
        } else if (s[0]=='!') {
          if ((dnum=chkobjlastinst(obj))==-1) goto errexit;
          *spc=4;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            goto errexit;
          } else {
            dnum=getobjid(obj,l);
            if (dnum==-1) goto errexit;
            *spc=1;
          }
        }
      } else if (s[0]=='+') {
        *spc=4;
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
        *spc=3;
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
  memfree(tok);
  return num;

errexit:
  memfree(tok);
  arraydel(iarray);
  return -1;
}

static int 
getilist(struct objlist *obj,char *ilist,struct narray *iarray,int def,int *spc)
/* spc  0: not found
        1: specified by id
        2: specified by oid
        3: specified by name
        4: specified by other
*/

{
  int i,len,snum,dnum,num,sid,l;
  int oid;
  char *tok,*s,*iname,*endptr;

  *spc=0;
  num=0;
  tok=NULL;
  if ((ilist==NULL) || (ilist[0]=='\0')) {
    if (def) {
      if ((snum=getobjcurinst(obj))==-1) return -1;
      if (arrayadd(iarray,&snum)==NULL) goto errexit;
      num++;
      *spc=4;
    }
  } else {
    while ((s=getitok2(&ilist,&len," \t,"))!=NULL) {
      memfree(tok);
      tok=s;
      iname=NULL;
      if (s[0]=='@') {
        if ((snum=getobjcurinst(obj))==-1) goto errexit;
        s++;
        *spc=4;
      } else if (s[0]=='!') {
        if ((snum=getobjlastinst(obj))==-1) goto errexit;
        s++;
        *spc=4;
      } else {
        if (s[0]=='^') {
          oid=TRUE;
          s++;
        } else oid=FALSE;
        l=strtol(s,&endptr,10);
        if (s!=endptr) {
          if (oid) {
            snum=getobjoid(obj,l);
            *spc=2;
          } else {
            snum=getobjid(obj,l);
            *spc=1;
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
          *spc=4;
        } else if (s[0]=='!') {
          if ((dnum=getobjlastinst(obj))==-1) goto errexit;
          *spc=4;
        } else {
          l=strtol(s,&endptr,10);
          if (endptr[0]!='\0') {
            error2(obj,ERRILINST,tok);
            goto errexit;
          } else {
            dnum=getobjid(obj,l);
            if (dnum==-1) goto errexit;
            *spc=1;
          }
        }
      } else if (s[0]=='+') {
        *spc=4;
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
        *spc=3;
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
  memfree(tok);
  return num;

errexit:
  memfree(tok);
  arraydel(iarray);
  return -1;
}

int 
chkobjilist(char *s,struct objlist **obj,struct narray *iarray,int def,int *spc)
{
  char *oname,*ilist;
  int len;
  int spc2;

  if (s==NULL) return -1;
  if ((s[0]==':') || ((oname=getitok2(&s,&len,":"))==NULL)) {
    if (len==-1) return -1;
    return ERRILOBJ;
  }
  if ((*obj=chkobject(oname))==NULL) {
    memfree(oname);
    return -1;
  }
  memfree(oname);
  if (s[0]==':') s++;
  ilist=s;
  if (def && (chkobjlastinst(*obj)==-1)) return -1;
  if (chkilist(*obj,ilist,iarray,def,&spc2)==-1) return -1;
  if (spc!=NULL) *spc=spc2;
  return 0;
}

int 
getobjilist(char *s,struct objlist **obj,struct narray *iarray,int def,int *spc)
{
  char *oname,*ilist;
  int len;
  int spc2;

  if (s==NULL) return -1;
  if ((s[0]==':') || ((oname=getitok2(&s,&len,":"))==NULL)) {
    if (len==-1) return -1;
    error2(NULL,ERRILOBJ,s);
    return ERRILOBJ;
  }
  if ((*obj=getobject(oname))==NULL) {
    memfree(oname);
    return -1;
  }
  memfree(oname);
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
    memfree(oname);
    return -1;
  }
  memfree(oname);
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
  memfree(ilist);
  if (num==-1) return -1;
  if (((*s)[0]!='\0') && ((*s)[0]==':')) (*s)++;
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
    memfree(oname);
    return -1;
  }
  memfree(oname);
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
  memfree(ilist);
  if (num==-1) return -1;
  if (((*s)[0]!='\0') && ((*s)[0]==':')) (*s)++;
  return 0;
}

char *
mkobjlist(struct objlist *obj,char *objname,int id,char *field,int oid)
{
  char ids[11];
  char *s;
  int len,flen;

  if (objname==NULL) objname=chkobjectname(obj);
  if (oid) sprintf(ids,"^%d",id);
  else sprintf(ids,"%d",id);
  if (field!=NULL) flen=strlen(field);
  else flen=0;
  if ((s=memalloc(strlen(objname)+strlen(ids)+flen+3))==NULL)
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
    memfree(objname);
    memfree(ids);
    *field=NULL;
    return NULL;
  }
  obj=chkobject(objname);
  memfree(objname);
  if (ids[0]=='^') {
    if (oid!=NULL) *oid=TRUE;
    ids2=ids+1;
  } else {
    if (oid!=NULL) *oid=FALSE;
    ids2=ids;
  }
  *id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0') || (chkobjoffset(obj,*field)==-1)) {
    memfree(ids);
    *field=NULL;
    return NULL;
  }
  memfree(ids);
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
  char *inst;

  list=olist;
  objname=getitok2(&list,&len,":");
  ids=getitok2(&list,&len,":");
  if ((objname==NULL) || (ids==NULL)) {
    memfree(objname);
    memfree(ids);
    return NULL;
  }
  if (ids[0]!='^') {
    memfree(objname);
    memfree(ids);
    if ((newlist=memalloc(strlen(olist)+1))==NULL) return NULL;
    strcpy(newlist,olist);
    return newlist;
  }
  ids2=ids+1;
  id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0')) {
    memfree(objname);
    memfree(ids);
    return NULL;
  }
  memfree(ids);
  obj=chkobject(objname);
  if ((inst=chkobjinstoid(obj,id))==NULL) {
    memfree(objname);
    return NULL;
  }
  _getobj(obj,"id",inst,&id);
  newlist=mkobjlist(obj,objname,id,NULL,FALSE);
  memfree(objname);
  return newlist;
}

char *
getvaluestr(struct objlist *obj,char *field,void *val,int cr,int quote)
{
  struct narray *array;
  void *po;
  char *bval,*s,*arglist;
  unsigned int k;
  int i,j;
  int type,len;

  arglist=chkobjarglist(obj,field);
  type=chkobjfieldtype(obj,field);
  len=0;
  po=val;
  switch (type) {
  case NBOOL: case NBFUNC:
    len+=5;
    break;
  case NCHAR: case NCFUNC:
    len+=1;
    break;
  case NINT: case NIFUNC:
    len+=11;
    break;
  case NDOUBLE: case NDFUNC:
    len+=23;
    break;
  case NSTR: case NSFUNC: case NOBJ:
    if (*(char **)po==NULL) break;
    else {
      bval=*(char **)po;
      if (quote) len++;
      for (i=0;bval[i]!='\0';i++) {
        if ((bval[i]=='\'') && quote) len+=3;
        len++;
      }
      if (quote) len++;
    }
    break;
  case NIARRAY: case NIAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) len++;
      if (arraynum(array)!=0) len=arraynum(array)*12-1;
      if (quote) len++;
    }
    break;
  case NDARRAY: case NDAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) len++;
      if (arraynum(array)!=0) len=arraynum(array)*24-1;
      if (quote) len++;
    }
    break;
  case NSARRAY: case NSAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) len++;
      for (k=0;k<arraynum(array);k++) {
        if (k!=0) len+=1;
        bval=*(char **)arraynget(array,k);
        for (i=0;bval[i]!='\0';i++) {
          if ((bval[i]=='\'') && quote) len+=3;
          len++;
        }
      }
      if (quote) len++;
    }
    break;
  case NENUM:
    len+=strlen(((char **)arglist)[*(int *)po]);
    break;
  }
  if (cr) len++;
  if ((s=memalloc(len+1))==NULL) return NULL;
  j=0;
  switch (type) {
  case NBOOL: case NBFUNC:
    if (*(int *)po) bval="true";
    else bval="false";
    j+=sprintf(s+j,"%s",bval);
    break;
  case NCHAR: case NCFUNC:
    j+=sprintf(s+j,"%c",*(char *)po);
    break;
  case NINT: case NIFUNC:
    j+=sprintf(s+j,"%d",*(int *)po);
    break;
  case NDOUBLE: case NDFUNC:
    j+=sprintf(s+j,"%.15e",*(double *)po);
    break;
  case NSTR: case NSFUNC: case NOBJ:
    if (*(char **)po==NULL) break;
    else {
      bval=*(char **)po;
      if (quote) j+=sprintf(s+j,"'");
      for (i=0;bval[i]!='\0';i++) {
        if ((bval[i]=='\'') && quote) j+=sprintf(s+j,"'\\''");
        else j+=sprintf(s+j,"%c",bval[i]);
      }
      if (quote) j+=sprintf(s+j,"'");
    }
    break;
  case NIARRAY: case NIAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) j+=sprintf(s+j,"'");
      for (k=0;k<arraynum(array);k++) {
        if (k!=0) j+=sprintf(s+j," %d",*(int *)arraynget(array,k));
        else j+=sprintf(s+j,"%d",*(int *)arraynget(array,k));
      }
      if (quote) j+=sprintf(s+j,"'");
    }
    break;
  case NDARRAY: case NDAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) j+=sprintf(s+j,"'");
      for (k=0;k<arraynum(array);k++) {
        if (k!=0) j+=sprintf(s+j," %.15e",*(double *)arraynget(array,k));
        else j+=sprintf(s+j,"%.15e",*(double *)arraynget(array,k));
      }
      if (quote) j+=sprintf(s+j,"'");
    }
    break;
  case NSARRAY: case NSAFUNC:
    array=*(struct narray **)po;
    if (array==NULL) break;
    else {
      if (quote) j+=sprintf(s+j,"'");
      for (k=0;k<arraynum(array);k++) {
        if (k!=0) j+=sprintf(s+j," ");
        bval=*(char **)arraynget(array,k);
        for (i=0;bval[i]!='\0';i++) {
          if ((bval[i]=='\'') && quote) j+=sprintf(s+j,"'\\''");
          else j+=sprintf(s+j,"%c",bval[i]);
        }
      }
      if (quote) j+=sprintf(s+j,"'");
    }
    break;
  case NENUM:
    j+=sprintf(s+j,"%s",((char **)arglist)[*(int *)po]);
    break;
  }
  if (cr) {
    s[j]='\n';
    j++;
  }
  s[j]='\0';
  return s;
}

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
  char *code;
  int i,err;
  double memory[MEMORYNUM];
  char memorystat[MEMORYNUM];
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
    memfree(s2);
    if ((s2=memalloc(len+1))==NULL) goto errexit;
    strncpy(s2,s,len);
    s2[len]='\0';
    if ((arglist!=NULL) && (arglist[alp]=='\0')) {
      err=1;
      goto errexit;
    }
    if (arglist==NULL) {
      if ((p=memalloc(strlen(s2)+1))==NULL) goto errexit;
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
      memfree(oname);
      if ((obj2==NULL) || (os[0]!=':')) {
        err=3;
        goto errexit;
      }
      for (i=1;
          (os[i]!='\0')&&(isalnum(os[i])||(strchr("_^@!+-",os[i])!=NULL));i++);
      if (os[i]!='\0') {
        err=3;
        goto errexit;
      }
      if ((p=memalloc(strlen(s2)+1))==NULL) goto errexit;
      strcpy(p,s2);
      if (arg_add(&argv,p)==NULL) goto errexit;
    } else if (arglist[alp]=='s') {
      if (arglist[1]=='a') {
        if (array==NULL) {
          if ((array=arraynew(sizeof(char *)))==NULL) goto errexit;
        }
        if (arrayadd2(array,&s2)==NULL) goto errexit;
      } else {
        if ((p=memalloc(strlen(s2)+1))==NULL) goto errexit;
        strcpy(p,s2);
        if (arg_add(&argv,p)==NULL) goto errexit;
      }
    } else if ((arglist[alp]=='i') || (arglist[alp]=='d')) {
      rcode=mathcode(s2,&code,NULL,NULL,NULL,NULL,
                     FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE);
      if (rcode!=MCNOERR) {
        err=3;
        goto errexit;
      }
      for (i=0;i<MEMORYNUM;i++) {memory[i]=0;memorystat[i]=MNOERR;}
      rcode=calculate(code,1,
                      0,MNOERR,0,MNOERR,0,MNOERR,
                      0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      NULL,NULL,
                      memory,memorystat,
                      NULL,NULL,
                      NULL,NULL,
                      NULL,NULL,NULL,
                      NULL,NULL,NULL,0,NULL,NULL,NULL,0,&vd);
      memfree(code);
      if (rcode!=MNOERR) {
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
          if ((p=memalloc(sizeof(int)))==NULL) goto errexit;
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
          if ((p=memalloc(sizeof(double)))==NULL) goto errexit;
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
        if ((p=memalloc(sizeof(int)))==NULL) goto errexit;
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
        if ((p=memalloc(sizeof(int)))==NULL) goto errexit;
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
    for (i=0;enumlist[i]!=NULL;i++)
      if (strcmp0(enumlist[i],argv[0])==0) break;
    if (enumlist[i]==NULL) {
      err=3;
      goto errexit;
    }
    memfree(argv[0]);
    if ((p=memalloc(sizeof(int)))==NULL) goto errexit;
    *(int *)(p)=i;
    argv[0] = p;
  }

  *argc=getargc(argv);
  memfree(s2);
  *rargv=argv;
  return 0;

errexit:
  arrayfree(array);
  memfree(s2);
  arg_del(argv);
  *argc=-1;
  *rargv=NULL;
  return err;
}

static void 
freeargument(int type,char *arglist,int argc,char **argv,int full)
{
  int i;

  if (argv!=NULL) {
    if (arglist==NULL) {
      for (i=0;i<argc;i++) memfree(argv[i]);
      memfree(argv);
    } else if (full) {
      if ((type!=NENUM) && (argc>0)
      && (arglist[0]!='\0') && (arglist[1]=='a')) {
        if ((arglist[0]=='i') || (arglist[0]=='d'))
          arrayfree((struct narray *)(argv[0]));
        else if (arglist[0]=='s')
          arrayfree2((struct narray *)(argv[0]));
      } else for (i=0;i<argc;i++) memfree(argv[i]);
      memfree(argv);
    } else {
      if ((type==NENUM) || (arglist[0]=='\0') || (arglist[1]!='a')) {
        for (i=0;i<argc;i++)
          if ((type==NENUM) || (strchr("bcid",arglist[i])!=NULL))
            memfree(argv[i]);
      }
      memfree(argv);
    }
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
schkobjfield(struct objlist *obj,int id,char *field, char *arg,
                 char **valstr,int limittype,int cr,int quote)
{
  int err;
  char *val;
  int argc2,type;
  char *arglist;
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
sgetobjfield(struct objlist *obj,int id,char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote)
{
  int err;
  char *val;
  int argc2,type;
  char *arglist;
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
  if ((s==NULL)
  || (strchr(":= \t",s[0])!=NULL)
  || ((field=getitok2(&s,&len,":= \t"))==NULL)) {
    if (len==-1) return -1;
    return ERRFIELD;
  }
  if (s[0]!='\0') s++;
  while ((s[0]==' ') || (s[0]=='\t')) s++;
  err=schkobjfield(obj,id,field,s,valstr,limittype,cr,quote);
  memfree(field);
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
  memfree(field);
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
sputobjfield(struct objlist *obj,int id,char *field,char *arg)
{
  char *val;
  char *arglist;
  int err,type;
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
  if ((s==NULL)
  || (strchr(":=",s[0])!=NULL)
  || ((field=getitok2(&s,&len,":="))==NULL)) {
    if (len==-1) return -1;
    error2(obj,ERRFIELD,arg);
    return ERRFIELD;
  }
  if (s[0]!='\0') s++;
  err=sputobjfield(obj,id,field,s);
  memfree(field);
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
sexeobjfield(struct objlist *obj,int id,char *field,char *arg)
{
  char *val;
  int err,type;
  int argc2;
  char *arglist;
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
  if (arglist && strcmp(arglist, "s"))
    while ((val[0]==' ') || (val[0]=='\t')) val++;
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
  if ((s==NULL)
      || (strchr(":= \t",s[0])!=NULL)
      || ((field=getitok2(&s,&len,":= \t"))==NULL)) {
    if (len==-1) return -1;
    error2(obj,ERRFIELD,arg);
    return -1;
  }
  if (s[0]!='\0') s++;
  rcode=sexeobjfield(obj,id,field,s);
  memfree(field);
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
obj_do_tighten(struct objlist *obj, char *inst, char *field)
{
  char *dest, *dest2;
  struct narray iarray;
  struct objlist *dobj;
  int anum, id, oid;

  if (_getobj(obj, field, inst, &dest))
    return;

  if (dest == NULL)
    return;

  arrayinit(&iarray, sizeof(int));
  if (! getobjilist(dest, &dobj, &iarray, FALSE, NULL)) {
    anum = arraynum(&iarray);
    if (anum > 0) {
      id = * (int *) arraylast(&iarray);
      if (getobj(dobj, "oid", id, 0, NULL, &oid) != -1) {
	dest2 = (char *) memalloc(strlen(chkobjectname(dobj)) + 10);
	if (dest2) {
	  sprintf(dest2, "%s:^%d", chkobjectname(dobj), oid);
	  _putobj(obj, field, inst, dest2);
	  memfree(dest);
	}
      }
    }
  }
  arraydel(&iarray);
}

int 
copy_obj_field(struct objlist *obj, int dist, int src, char **ignore_field)
{
  int perm, type, ignore, j;
  char *field, **ptr;

  for (j = 0; j < chkobjfieldnum(obj); j++) {
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

#ifdef COMPILE_UNUSED_FUNCTIONS
static char *
getuniqname(struct objlist *obj,char *prefix,char sep)
{
  int i,j,len;
  char *iname;
  char *inst;
  char *name;
  int c[10];

  if (chkobjoffset(obj,"name")==-1) return NULL;
  if (prefix==NULL) len=0;
  else len=strlen(prefix);
  if (sep!='\0') len++;
  if ((name=memalloc(len+11))==NULL) return NULL;
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
