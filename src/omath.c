/* 
 * $Id: omath.c,v 1.8 2009/04/03 09:43:31 hito Exp $
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
#include <stdarg.h>
#include <string.h>
#include "ngraph.h"
#include "object.h"
#include "mathcode.h"
#include "mathfn.h"

#define NAME "math"
#define PARENT "object"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRSYNTAX 100
#define ERRILLEGAL 101
#define ERRNEST   102
#define ERRARG    103
#define ERRSMLARG 104

static int MathErrorCodeArray[MATH_CODE_ERROR_NUM];

static char *matherrorlist[]={
  "syntax error.",
  "not allowd function.",
  "sum() or dif(): deep nest.",
  "illegal argument",
  "not enouph argument."
};

#define ERRNUM (sizeof(matherrorlist) / sizeof(*matherrorlist))

struct mlocal {
  double x;
  double y;
  double z;
  double val;
  double memory[MEMORYNUM];
  char memorystat[MEMORYNUM];
  double sumdata[10];
  char sumstat[10];
  double difdata[10];
  char difstat[10];
  int maxdim;
  char *code;
  char *ufcodef;
  char *ufcodeg;
  char *ufcodeh;
  int rcode;
  int idpx;
  int idpy;
  int idpz;
  int idpm[MEMORYNUM];
  int idpr;
};

static void 
msettbl(char *inst,struct mlocal *mlocal)
{
  int i;

  *(double *)(inst+mlocal->idpx)=mlocal->x;
  *(double *)(inst+mlocal->idpy)=mlocal->y;
  *(double *)(inst+mlocal->idpz)=mlocal->z;

  for (i = 0; i < MEMORYNUM; i++) {
    *(double *)(inst + mlocal->idpm[i]) = mlocal->memory[i];
  }
  *(int *)(inst+mlocal->idpr)=mlocal->rcode;
}

static void 
mlocalclear(struct mlocal *mlocal,int memory)
{
  int i;
  if (memory) {
    for (i=0;i<MEMORYNUM;i++) {
      mlocal->memory[i]=0;
      mlocal->memorystat[i]=MNOERR;
    }
  }
  for (i=0;i<10;i++) {
    mlocal->sumdata[i]=0;
    mlocal->sumstat[i]=MNOERR;
    mlocal->difdata[i]=0;
    mlocal->difstat[i]=MUNDEF;
  }
}

static int 
minit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  struct mlocal *mlocal;
  int i;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if ((mlocal=memalloc(sizeof(struct mlocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,mlocal)) goto errexit;
  mlocal->x=0;
  mlocal->y=0;
  mlocal->z=0;
  mlocal->val=0;
  mlocal->code=NULL;
  mlocal->ufcodef=NULL;
  mlocal->ufcodeg=NULL;
  mlocal->ufcodeh=NULL;
  mlocal->rcode=MNOERR;
  mlocalclear(mlocal,TRUE);
  if (_putobj(obj,"formula",inst,NULL)) goto errexit;
  if (_putobj(obj,"f",inst,NULL)) goto errexit;
  if (_putobj(obj,"g",inst,NULL)) goto errexit;
  if (_putobj(obj,"h",inst,NULL)) goto errexit;
  mlocal->idpx=chkobjoffset(obj,"x");
  mlocal->idpy=chkobjoffset(obj,"y");
  mlocal->idpz=chkobjoffset(obj,"z");

  for (i = 0; i < MEMORYNUM; i++) {
    char mstr[] = "m00";

    mstr[sizeof(mstr) - 2] += i;
    mlocal->idpm[i] = chkobjoffset(obj, mstr);
  }
  mlocal->idpr=chkobjoffset(obj,"status");
  msettbl(inst,mlocal);
  return 0;

errexit:
  memfree(mlocal);
  return 1;
}

static int 
mdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct mlocal *mlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&mlocal);
  memfree(mlocal->code);
  memfree(mlocal->ufcodef);
  memfree(mlocal->ufcodeg);
  memfree(mlocal->ufcodeh);
  return 0;
}

static int 
mformula(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  enum MATH_CODE_ERROR_NO rcode;
  char *math,*code;
  struct mlocal *mlocal;
  struct narray needdata;
  int column,multi,maxdim,memory,usrfn;

  math=argv[2];
  if (math!=NULL) {
    if (strcmp("formula",argv[1])==0) {
      column=TRUE;
      multi=TRUE;
      usrfn=TRUE;
      memory=TRUE;
    } else if ((strcmp("f",argv[1])==0) || (strcmp("g",argv[1])==0)
            || (strcmp("h",argv[1])==0)) {
      column=FALSE;
      multi=FALSE;
      usrfn=FALSE;
      memory=FALSE;
    } else {
      /* not reachable */
      return 0;
    }
    arrayinit(&needdata,sizeof(int));
    rcode=mathcode(math,&code,&needdata,NULL,&maxdim,NULL,
                   TRUE,TRUE,TRUE,column,multi,FALSE,memory,usrfn,FALSE,FALSE,FALSE);
    if (column) arraydel(&needdata);

    if (mathcode_error(obj, rcode, MathErrorCodeArray)) {
      return 1;
    }
  } else code=NULL;
  _getobj(obj,"_local",inst,&mlocal);
  if (strcmp("formula",argv[1])==0) {
    memfree(mlocal->code);
    mlocal->code=code;
    mlocal->maxdim=maxdim;
  } else if (strcmp("f",argv[1])==0) {
    memfree(mlocal->ufcodef);
    mlocal->ufcodef=code;
  } else if (strcmp("g",argv[1])==0) {
    memfree(mlocal->ufcodeg);
    mlocal->ufcodeg=code;
  } else if (strcmp("h",argv[1])==0) {
    memfree(mlocal->ufcodeh);
    mlocal->ufcodeh=code;
  }
  mlocalclear(mlocal,FALSE);
  msettbl(inst,mlocal);
  return 0;
}

static int 
mparam(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *arg;
  int m;
  struct mlocal *mlocal;

  _getobj(obj, "_local", inst, &mlocal);
  arg = argv[1];
  switch (arg[0]) {
  case 'x':
    mlocal->x = * (double *) argv[2];
    break;
  case 'y':
    mlocal->y = * (double *) argv[2];
    break;
  case 'z':
    mlocal->z = * (double *) argv[2];
    break;
  case 'm':
    m = (arg[1] - '0') * 10 + arg[2] - '0';
    mlocal->memory[m] = * (double *) argv[2];
    mlocal->memorystat[m] = MNOERR;
    break;
  }
  msettbl(inst, mlocal);
  return 0;
}

static int 
mcalc(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct mlocal *mlocal;
  int i,num;
  double *data,*adata;
  char *datastat;
  struct narray *darray;
  int maxdim;

  _getobj(obj,"_local",inst,&mlocal);
  maxdim=mlocal->maxdim;
  darray=(struct narray *)argv[2];
  num=arraynum(darray);
  adata=arraydata(darray);
  if (num<maxdim) {
    error(obj,ERRSMLARG);
    return 1;
  }

  data = memalloc(sizeof(double) * (num + 1));
  datastat = memalloc(sizeof(char) * (num + 1));

  if (data == NULL || datastat == NULL) {
    memfree(data);
    memfree(datastat);
    return 1;
  }
  data[0]=0;
  datastat[0]=MNOERR;
  for (i=0;i<num;i++) {
    data[i+1]=adata[i];
    datastat[i+1]=MNOERR;
  }
  mlocal->rcode=calculate(mlocal->code,1,
                  mlocal->x,MNOERR,mlocal->y,MNOERR,mlocal->z,MNOERR,
                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                  data,datastat,
                  mlocal->memory,mlocal->memorystat,
                  mlocal->sumdata,mlocal->sumstat,
                  mlocal->difdata,mlocal->difstat,
                  NULL,NULL,NULL,
                  mlocal->ufcodef,mlocal->ufcodeg,mlocal->ufcodeh,
                  0,NULL,NULL,NULL,0,
                  &(mlocal->val));
  memfree(data);
  memfree(datastat);
  msettbl(inst,mlocal);
  *(double *)rval=mlocal->val;
  if (mlocal->rcode==MSERR) {
    error(obj,ERRSYNTAX);
    return 1;
  }
  return 0;
}

static int 
mclear(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct mlocal *mlocal;

  _getobj(obj,"_local",inst,&mlocal);
  mlocalclear(mlocal,TRUE);
  msettbl(inst,mlocal);
  return 0;
}


static struct objtable math[] = {
  {"init",NVFUNC,NEXEC,minit,NULL,0},
  {"done",NVFUNC,NEXEC,mdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"formula",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"f",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"g",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"h",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"x",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"y",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"z",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m00",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m01",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m02",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m03",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m04",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m05",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m06",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m07",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m08",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m09",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m10",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m11",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m12",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m13",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m14",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m15",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m16",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m17",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m18",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"m19",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"status",NENUM,NREAD,NULL,matherrorchar,0},
  {"calc",NDFUNC,NREAD|NEXEC,mcalc,"da",0},
  {"clear",NVFUNC,NREAD|NEXEC,mclear,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(math) / sizeof(*math))

void *
addmath(void)
/* addmath() returns NULL on error */
{
  MathErrorCodeArray[MCNOERR] = 0;
  MathErrorCodeArray[MCSYNTAX] = ERRSYNTAX;
  MathErrorCodeArray[MCILLEGAL] = ERRILLEGAL;
  MathErrorCodeArray[MCNEST] = ERRNEST;

  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,math,ERRNUM,matherrorlist,NULL,NULL);
}
