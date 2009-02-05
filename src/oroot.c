/* 
 * $Id: oroot.c,v 1.3 2009/02/05 08:13:08 hito Exp $
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
#include <string.h>
#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "oroot.h"

#define NAME "object"
#define PARENT NULL
#define OVERSION "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRILNAME 100

#define ERRNUM 1

static char *rooterrorlist[ERRNUM]={
     "illegal instance name"
};

static int 
oinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
odone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
oputname(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *arg;
  int i;

  if (argc<3) return 0;
  arg=argv[2];
  if (arg==NULL) return 0;
  if (!isalpha(arg[0]) && (arg[0]!='_')) {
    error2(obj,ERRILNAME,arg);
    return 1;
  }
  for (i=1;arg[i]!='\0';i++)
    if (!isalnum(arg[i]) && (strchr("_",arg[i])==NULL)) {
      error2(obj,ERRILNAME,arg);
      return 1;
    }
  return 0;
}

int 
oputabs(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<0) *(int *)argv[2]=0;
  return 0;
}

int 
oputge1(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  return 0;
}

int 
oputangle(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<0) *(int *)(argv[2])=0;
  else if (*(int *)(argv[2])>36000) *(int *)(argv[2])=36000;
  return 0;
}

int 
oputcolor(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (*(int *) (argv[2]) < 0) *(int *)(argv[2]) = 0;
  else if (*(int *)(argv[2]) > 255) *(int *)(argv[2]) = 255;
  return 0;
}

int 
oputstyle(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  int i,num,*adata;

  array=(struct narray *)argv[2];
  num=arraynum(array);
  adata=arraydata(array);
  for (i=0;i<num;i++)
    adata[i]=abs(adata[i]);
  return 0;
}

static void 
ochgobjlist(char **olist)
{
  char *list,*objname,*field;
  char *ids,*ids2;
  int id,len;
  struct objlist *obj;
  char *endptr;
  char *inst;

  list=*olist;
  objname=getitok2(&list,&len,":");
  ids=getitok2(&list,&len,":");
  field=list;
  if ((objname==NULL) || (ids==NULL)) {
    memfree(objname);
    memfree(ids);
    memfree(*olist);
    *olist=NULL;
    return;
  }
  obj=chkobject(objname);
  memfree(objname);
  if (ids[0]!='^') {
    memfree(ids);
    return;
  }
  ids2=ids+1;
  id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0')) {
    memfree(ids);
    memfree(*olist);
    *olist=NULL;
    return;
  }
  memfree(ids);
  if ((inst=getobjinstoid(obj,id))==NULL) {
    memfree(*olist);
    *olist=NULL;
    return;
  }
  _getobj(obj,"id",inst,&id);
  memfree(*olist);
  *olist=mkobjlist(obj,NULL,id,field,FALSE);
  return;
}

static int 
osave(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct objlist *obj2;
  char *s,*field,*name,*valstr;
  char buf[8],*list;
  int i,j,perm;
  struct narray *array;
  int anum;
  char **adata;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  array=(struct narray *)argv[2];
  anum=arraynum(array);
  adata=arraydata(array);
  if ((obj2=getobject(argv[0]))==NULL) return 1;
  if (_getobj(obj2,"name",inst,&name)) return 1;
  if ((s=nstrnew())==NULL) return 1;
  if ((s=nstrcat(s,"new "))==NULL) return 1;
  if ((s=nstrcat(s,argv[0]))==NULL) return 1;
  if (name!=NULL) {
    if ((s=nstrcat(s," name:"))==NULL) return 1;
    if ((s=nstrcat(s,name))==NULL) return 1;
  }
  if ((s=nstrccat(s,'\n'))==NULL) return 1;
  for (i=0;i<chkobjfieldnum(obj2);i++) {
    field=chkobjfieldname(obj2,i);
    for (j=0;j<anum;j++) if (strcmp0(field,adata[j])==0) break;
    if (j==anum) {
      perm=chkobjperm(obj2,field);
      if (((perm&NREAD)!=0)
       && ((perm&NWRITE)!=0) && (strcmp0(field,"name")!=0)) {
        valstr=NULL;
        if ((s=nstrccat(s,'\t'))==NULL) goto errexit;
        if ((s=nstrcat(s,argv[0]))==NULL) goto errexit;
        if ((s=nstrcat(s,"::"))==NULL) goto errexit;
        if ((s=nstrcat(s,field))==NULL) goto errexit;
        if ((s=nstrccat(s,'='))==NULL) goto errexit;
        if (chkobjfieldtype(obj2,field)==NOBJ) {
          if (_getobj(obj2,field,inst,&list)) goto errexit;
          list=chgobjlist(list);
          if ((valstr=getvaluestr(obj2,field,&list,FALSE,TRUE))==NULL)
            goto errexit;
          memfree(list);
        } else {
          if (_getobj(obj2,field,inst,(void *)buf)) goto errexit;
          if ((valstr=getvaluestr(obj2,field,buf,FALSE,TRUE))==NULL)
            goto errexit;
        }
        if ((s=nstrcat(s,valstr))==NULL) goto errexit;
        if ((s=nstrccat(s,'\n'))==NULL) goto errexit;
        memfree(valstr);
      }
    }
  }
  *(char **)rval=s;
  return 0;
errexit:
  memfree(s);
  memfree(valstr);
  return 1;
}


#define TBLNUM 6

static struct objtable objectroot[TBLNUM] = {
  {"init",NVFUNC,NEXEC,oinit,NULL,0},
  {"done",NVFUNC,NEXEC,odone,NULL,0},
  {"id",NINT,NREAD,NULL,NULL,0},
  {"oid",NINT,NREAD,NULL,NULL,0},
  {"name",NSTR,NREAD|NWRITE,oputname,NULL,0},
  {"save",NSFUNC,NREAD|NEXEC,osave,"sa",0},
};

void *addobjectroot()
/* addobjectroot() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,objectroot,ERRNUM,rooterrorlist,NULL,NULL);
}
