/* 
 * $Id: gra.c,v 1.2 2008/06/10 07:12:13 hito Exp $
 * 
 * This file is part of "Ngraph for X11".
 * 
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "object.h"
#include "nstring.h"
#include "jstring.h"
#include "mathfn.h"
#include "mathcode.h"
#include "gra.h"

#define TRUE 1
#define FALSE 0

#define GRAClimit 10

struct GRAC {
  int open,init;
  char *objname;
  char *outputname;
  struct objlist *obj;
  char *inst;
  int output,charwidth,charascent,chardescent;
  struct narray **list;
  void *local;
  int (*direct)(char code,int *cpar,char *cstr,void *local);

  int viewf;
  int leftm,topm,width,height;
  double zoom;
  int gminx,gminy,gmaxx,gmaxy;
  int cpx,cpy;
  int clip;

  int linef;
  int linedashn;
  int *linedash;
  int linewidth,linecap,linejoin,linemiter;

  int colorf;
  int fr,fg,fb;

  int textf;
  char *textfont;
  int textsize;
  int textdir;
  int textspace;

  int mergetop,mergeleft,mergezoom;
  int mergefont,mergept,mergesp,mergedir;

  int gdashn;
  int gdashi;
  double gdashlen;
  int gdotf;
  int *gdashlist;
  clipfunc gclipf;
  transfunc gtransf;
  diffunc gdiff;
  intpfunc gintpf;
  void *gflocal;
  double x0,y0;

  int oldFR,oldFG,oldFB,oldBR,oldBG,oldBB;

} GRAClist[GRAClimit+1]= {
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
  {FALSE,FALSE,NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL,NULL,
   FALSE,0,0,SHRT_MAX,SHRT_MAX,1,0,0,SHRT_MAX,SHRT_MAX,0,0,1,
   FALSE,0,NULL,1,0,0,0,
   FALSE,0,0,0,
   FALSE,NULL,0,0,0,
   0,0,10000,0,0,0,0,
   0,0,0,TRUE,NULL,NULL,NULL,NULL,NULL,NULL,0,0,
   0,0,0,0,0,0},
};

int _GRAopencallback(int (*direct)(char code,int *cpar,char *cstr,void *local),
                   struct narray **list,void *local)
{
  int i;

  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=NULL;
  GRAClist[i].outputname=NULL;
  GRAClist[i].obj=NULL;
  GRAClist[i].inst=NULL;
  GRAClist[i].output=-1;
  GRAClist[i].charwidth=-1;
  GRAClist[i].charascent=-1;
  GRAClist[i].chardescent=-1;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=direct;
  return i;
}

int _GRAopen(char *objname,char *outputname,
             struct objlist *obj,char *inst,
             int output,int charwidth,int charascent,int chardescent,
             struct narray **list,void *local)
{
  int i;

  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=objname;
  GRAClist[i].outputname=outputname;
  GRAClist[i].obj=obj;
  GRAClist[i].inst=inst;
  GRAClist[i].output=output;
  GRAClist[i].charwidth=charwidth;
  GRAClist[i].charascent=charascent;
  GRAClist[i].chardescent=chardescent;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=NULL;
  return i;
}

int GRAopen(char *objname,char *outputname,
            struct objlist *obj,char *inst,
            int output,int charwidth,int charascent,int chardescent,
            struct narray **list,void *local)
{
  int i,GC;

  if (obj!=NULL) {
    if (!chkobjfield(obj,"_GC")) {
      if (_getobj(obj,"_GC",inst,&GC)) return -1;
      if (GC!=-1) return -2;
    }
  }
  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GC=i;
  if (obj!=NULL) {
    if (!chkobjfield(obj,"_GC")) {
      if (_putobj(obj,"_GC",inst,&GC)) return -1;
    }
  }
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=objname;
  GRAClist[i].outputname=outputname;
  GRAClist[i].obj=obj;
  GRAClist[i].inst=inst;
  GRAClist[i].output=output;
  GRAClist[i].charwidth=charwidth;
  GRAClist[i].charascent=charascent;
  GRAClist[i].chardescent=chardescent;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=NULL;
  return i;
}

void GRAreopen(int GC)
{
  char code;
  int cpar[6];

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  memfree(GRAClist[GC].linedash);
  memfree(GRAClist[GC].gdashlist);
  memfree(GRAClist[GC].textfont);
  GRAClist[GC].linedashn=0;
  GRAClist[GC].linedash=NULL;
  GRAClist[GC].gdashlist=NULL;
  GRAClist[GC].textfont=NULL;
  GRAClist[GC].viewf=FALSE;
  GRAClist[GC].linef=FALSE;
  GRAClist[GC].colorf=FALSE;
  GRAClist[GC].textf=FALSE;
  code='I';
  cpar[0]=5;
  cpar[1]=GRAClist[GC].leftm;
  cpar[2]=GRAClist[GC].topm;
  cpar[3]=GRAClist[GC].width;
  cpar[4]=GRAClist[GC].height;
  cpar[5]=nround(GRAClist[GC].zoom*10000);
  GRAdraw(GC,code,cpar,NULL);
}

int GRAopened(int GC)
{
  if (GC<0) return -1;
  if (GC>=GRAClimit) return -1;
  if (!(GRAClist[GC].open)) return -1;
  return GC;
}

void _GRAclose(int GC)
{
  if (GC<0) return;
  if (GC>=GRAClimit) return;
  memfree(GRAClist[GC].linedash);
  memfree(GRAClist[GC].gdashlist);
  memfree(GRAClist[GC].textfont);
  GRAClist[GC]=GRAClist[GRAClimit];
}

void GRAclose(int GC)
{
  int GC2,i;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  GC2=-1;
  if (GRAClist[GC].obj!=NULL) {
    if (!chkobjfield(GRAClist[GC].obj,"_lock")) {
      _putobj(GRAClist[GC].obj,"_GC",GRAClist[GC].inst,&GC2);
    }
    i=-1;
    if (!chkobjfield(GRAClist[GC].obj,"_GC")) {
      _putobj(GRAClist[GC].obj,"_GC",GRAClist[GC].inst,&i);
    }
  }
  memfree(GRAClist[GC].linedash);
  memfree(GRAClist[GC].gdashlist);
  memfree(GRAClist[GC].textfont);
  GRAClist[GC]=GRAClist[GRAClimit];
}

void GRAaddlist2(int GC,char *draw)
{
  struct narray **array;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  if (GRAClist[GC].output==-1) return;
  array=GRAClist[GC].list;
  if (array!=NULL) {
    if (*array==NULL) *array=arraynew(sizeof(char *));
    arrayadd(*array,&draw);
  }
}

void GRAinslist2(int GC,char *draw,int n)
{
  struct narray **array;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  if (GRAClist[GC].output==-1) return;
  array=GRAClist[GC].list;
  if (array!=NULL) {
    if (*array==NULL) *array=arraynew(sizeof(char *));
    arrayins(*array,&draw,n);
  }
}

void GRAaddlist(int GC,struct objlist *obj,char *inst,
                char *objname,char *field)
{
  int oid;
  char *draw;

  if (GRAClist[GC].output==-1) return;
  if (_getobj(obj,"oid",inst,&oid)==-1) return;
  if ((draw=mkobjlist(NULL,objname,oid,field,TRUE))==NULL) return;
  GRAaddlist2(GC,draw);
}

void GRAinslist(int GC,struct objlist *obj,char *inst,
                char *objname,char *field,int n)
{
  int oid;
  char *draw;

  if (GRAClist[GC].output==-1) return;
  if (_getobj(obj,"oid",inst,&oid)==-1) return;
  if ((draw=mkobjlist(NULL,objname,oid,field,TRUE))==NULL) return;
  GRAinslist2(GC,draw,n);
}

void GRAdellist(int GC,int n)
{
  struct narray **array;

  if (GRAClist[GC].output==-1) return;
  if (GC<0) return;
  if (GC>=GRAClimit) return;
  if (GRAClist[GC].output==-1) return;
  array=GRAClist[GC].list;
  if (array!=NULL) {
    arrayndel2(*array,n);
  }
}

struct objlist *GRAgetlist(int GC,int *oid,char **field,int n)
{
  struct narray **array;
  char **sdata;
  int snum;

  if (GC<0) return NULL;
  if (GC>=GRAClimit) return NULL;
  if (GRAClist[GC].output==-1) return NULL;
  array=GRAClist[GC].list;
  snum=arraynum(*array);
  if (n>=snum) return NULL;
  sdata=arraydata(*array);
  return getobjlist(sdata[n],oid,field,NULL);
}

void _GRAredraw(int GC,int snum,char **sdata,int setredrawf,int redrawf,
                int addn,struct objlist *obj,char *inst,char *field)
{
  int i;
  char *dargv[2];
  int redrawfsave;
  struct objlist *dobj;
  int did;
  char *dfield;
  char *dinst;


  if (ninterrupt()) return;
  dargv[0]=(char *)&GC;
  if ((addn==0) && (obj!=NULL) && (inst!=NULL) && (field!=NULL))
    _exeobj(obj,field,inst,1,dargv);
  if (addn>snum) addn=snum-1;
  for (i=1;i<snum;i++) {
    if (ninterrupt()) return;
    if ((dobj=getobjlist(sdata[i],&did,&dfield,NULL))!=NULL) {
      if ((dinst=chkobjinstoid(dobj,did))!=NULL) {
        if (setredrawf) {
	  int t = TRUE;
          _getobj(dobj,"redraw_flag",dinst,&redrawfsave);
          _putobj(dobj,"redraw_flag",dinst, &t);
          _putobj(dobj,"redraw_num",dinst, &redrawf);
        }
        _exeobj(dobj,dfield,dinst,1,dargv);
        if (setredrawf) {
          _putobj(dobj,"redraw_flag",dinst,&redrawfsave);
        }
        if ((addn==i) && (obj!=NULL) && (inst!=NULL) && (field!=NULL))
          _exeobj(obj,field,inst,1,dargv);
      }
    }
  }
}

void GRAredraw(struct objlist *obj,char *inst,int setredrawf,int redrawf)
{
  GRAredraw2(obj,inst,setredrawf,redrawf,-1,NULL,NULL,NULL);
}

void GRAredraw2(struct objlist *obj,char *inst,int setredrawf,int redrawf,
                int addn,struct objlist *aobj,char *ainst,char *afield)
{
  struct narray *sarray;
  char **sdata;
  int snum;
  int oid,gid,xid,GC,GCnew;
  char *gfield,*xfield;
  char *ginst;
  struct objlist *gobj,*xobj;
  char *device;

  if (_getobj(obj,"_list",inst,&sarray)) return;
  if (_getobj(obj,"oid",inst,&oid)) return;
  if ((snum=arraynum(sarray))==0) return;
  sdata=(char **)arraydata(sarray);
  if ((_putobj(obj,"_list",inst,NULL)) || (_getobj(obj,"_GC",inst,&GC))) {
    arrayfree2(sarray);
    return;
  }
  if (((gobj=getobjlist(sdata[0],&gid,&gfield,NULL))==NULL)
  || ((ginst=getobjinstoid(gobj,gid))==NULL)) {
    arrayfree2(sarray);
    return;
  }
  if (GC!=-1) {
  /* gra is still opened */
    GRAaddlist2(GC,sdata[0]);
    GCnew=GC;
    sdata[0]=NULL;
    GRAreopen(GC);
  } else {
  /* gra is already closed */
    /* check consistency */
    if (_getobj(gobj,"_device",ginst,&device) || (device==NULL)
    || ((xobj=getobjlist(device,&xid,&xfield,NULL))==NULL)
    || (xobj!=obj) || (xid!=oid) || (strcmp(xfield,"_output")!=0)) {
      arrayfree2(sarray);
      return;
    }
    /* open GRA */
    if ((_exeobj(gobj,"open",ginst,0,NULL))
    || (_getobj(gobj,"open",ginst,&GCnew))
    || (GRAopened(GCnew)==-1)) {
      arrayfree2(sarray);
      return;
    }
  }
  _GRAredraw(GCnew,snum,sdata,setredrawf,redrawf,addn,aobj,ainst,afield);
  arrayfree2(sarray);
  if (GC==-1) _exeobj(gobj,"close",ginst,0,NULL);
}

void GRAdraw(int GC,char code,int *cpar,char *cstr)
{
  char *argv[7];
  double zoom;
  int i,zoomf;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  if (!(GRAClist[GC].open)) return;
  if ((GRAClist[GC].direct==NULL)
  && ((GRAClist[GC].output==-1) || (GRAClist[GC].obj==NULL))) return;
  zoom=GRAClist[GC].zoom;
  if (zoom==1) zoomf=FALSE;
  else zoomf=TRUE;
  switch (code) {
  case 'I':
    if (GRAClist[GC].init) return;
    GRAClist[GC].init=TRUE;
    break;
  case 'V':
    if (GRAClist[GC].viewf
    && (cpar[1]==GRAClist[GC].gminx) && (cpar[3]==GRAClist[GC].gmaxx)
    && (cpar[2]==GRAClist[GC].gminy) && (cpar[4]==GRAClist[GC].gmaxy)
    && (cpar[5]==GRAClist[GC].clip)) return;
    GRAClist[GC].viewf=TRUE;
    GRAClist[GC].gminx=cpar[1];
    GRAClist[GC].gminy=cpar[2];
    GRAClist[GC].gmaxx=cpar[3];
    GRAClist[GC].gmaxy=cpar[4];
    GRAClist[GC].clip=cpar[5];
    cpar[1]=cpar[1]*zoom+GRAClist[GC].leftm;
    cpar[2]=cpar[2]*zoom+GRAClist[GC].topm;
    cpar[3]=cpar[3]*zoom+GRAClist[GC].leftm;
    cpar[4]=cpar[4]*zoom+GRAClist[GC].topm;
    break;
  case 'A':
    if (GRAClist[GC].linef
    && (cpar[1]==GRAClist[GC].linedashn) && (cpar[2]==GRAClist[GC].linewidth)
    && (cpar[3]==GRAClist[GC].linecap) && (cpar[4]==GRAClist[GC].linejoin)
    && (cpar[5]==GRAClist[GC].linemiter)) {
      for (i=0;i<cpar[1];i++)
        if (cpar[6+i]!=(GRAClist[GC].linedash)[i]) break;
      if (i==cpar[1]) return;
    }
    GRAClist[GC].linef=TRUE;
    GRAClist[GC].linewidth=cpar[2];
    GRAClist[GC].linecap=cpar[3];
    GRAClist[GC].linejoin=cpar[4];
    GRAClist[GC].linemiter=cpar[5];
    GRAClist[GC].linedashn=cpar[1];
    memfree(GRAClist[GC].linedash);
    if (cpar[1]!=0) {
      if ((GRAClist[GC].linedash=memalloc(sizeof(int)*cpar[1]))!=NULL)
        memcpy(GRAClist[GC].linedash,&(cpar[6]),sizeof(int)*cpar[1]);
    } else GRAClist[GC].linedash=NULL;
    if (zoomf) {
      cpar[2]*=zoom;
      for (i=0;i<cpar[1];i++) cpar[i+5]*=zoom;
    }
    break;
  case 'G':
    if (GRAClist[GC].colorf
    && (cpar[1]==GRAClist[GC].fr) && (cpar[2]==GRAClist[GC].fg)
    && (cpar[3]==GRAClist[GC].fb)) return;
    GRAClist[GC].colorf=TRUE;
    GRAClist[GC].fr=cpar[1];
    GRAClist[GC].fg=cpar[2];
    GRAClist[GC].fb=cpar[3];
    break;
  case 'F':
    if ((GRAClist[GC].textfont!=NULL)
    && (strcmp(GRAClist[GC].textfont,cstr)==0)) return;
    memfree(GRAClist[GC].textfont);
    if ((GRAClist[GC].textfont=memalloc(strlen(cstr)+1))==NULL) return;
    strcpy(GRAClist[GC].textfont,cstr);
    GRAClist[GC].textf=FALSE;
    break;
  case 'H':
    if ((GRAClist[GC].textf) && (cpar[1]==GRAClist[GC].textsize)
    && (cpar[2]==GRAClist[GC].textspace) && (cpar[3]==GRAClist[GC].textdir))
      return;
    GRAClist[GC].textf=TRUE;
    GRAClist[GC].textsize=cpar[1];
    GRAClist[GC].textspace=cpar[2];
    GRAClist[GC].textdir=cpar[3];
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
    }
    break;
  case 'M': case 'N': case 'T': case 'P':
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
    }
    break;
  case 'L': case 'B': case 'C':
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
      cpar[3]*=zoom;
      cpar[4]*=zoom;
    }
    break;
  case 'R':
    if (zoomf) for (i=0;i<cpar[1]*2;i++) cpar[i+2]*=zoom;
    break;
  case 'D':
    if (zoomf) for (i=0;i<cpar[1]*2;i++) cpar[i+3]*=zoom;
    break;
  default:
    break;
  }
  if (GRAClist[GC].direct==NULL) {
    argv[0]=GRAClist[GC].objname;
    argv[1]=GRAClist[GC].outputname;
    argv[2]=GRAClist[GC].local;
    argv[3]=&code;
    argv[4]=(char *)cpar;
    argv[5]=cstr;
    argv[6]=NULL;
    __exeobj(GRAClist[GC].obj,GRAClist[GC].output,GRAClist[GC].inst,6,argv);
  } else {
    GRAClist[GC].direct(code,cpar,cstr,GRAClist[GC].local);
  }
}

int GRAcharwidth(unsigned int ch, char *font,int size)
{
  char *argv[7];
  int i,idp;

  for (i=GRAClimit-1;i>=0;i--)
    if ((GRAopened(i)==i) && (GRAClist[i].charwidth!=-1)) break;
  if (i==-1) return nround(25.4/72000.0*size*600);
  argv[0]=GRAClist[i].objname;
  argv[1]="_charwidth";
  argv[2]=GRAClist[i].local;
  argv[3]=(char *)&ch;
  argv[4]=(char *)&size;
  argv[5]=font;
  argv[6]=NULL;
  __exeobj(GRAClist[i].obj,GRAClist[i].charwidth,GRAClist[i].inst,6,argv);
  idp=chkobjoffset2(GRAClist[i].obj,GRAClist[i].charwidth);
  return *(int *)(GRAClist[i].inst+idp);
}

int GRAcharascent(char *font,int size)
{
  char *argv[6];
  int i,idp;

  for (i=GRAClimit-1;i>=0;i--)
    if ((GRAopened(i)==i) && (GRAClist[i].charascent!=-1)) break;
  if (i==-1) return nround(25.4/72000.0*size*563);
  argv[0]=GRAClist[i].objname;
  argv[1]="_charascent";
  argv[2]=GRAClist[i].local;
  argv[3]=(char *)&size;
  argv[4]=font;
  argv[5]=NULL;
  __exeobj(GRAClist[i].obj,GRAClist[i].charascent,GRAClist[i].inst,5,argv);
  idp=chkobjoffset2(GRAClist[i].obj,GRAClist[i].charascent);
  return *(int *)(GRAClist[i].inst+idp);
}

int GRAchardescent(char *font,int size)
{
  char *argv[6];
  int i,idp;

  for (i=GRAClimit-1;i>=0;i--)
    if ((GRAopened(i)==i) && (GRAClist[i].chardescent!=-1)) break;
  if (i==-1) return nround(25.4/72000.0*size*250);
  argv[0]=GRAClist[i].objname;
  argv[1]="_chardescent";
  argv[2]=GRAClist[i].local;
  argv[3]=(char *)&size;
  argv[4]=font;
  argv[5]=NULL;
  __exeobj(GRAClist[i].obj,GRAClist[i].chardescent,GRAClist[i].inst,5,argv);
  idp=chkobjoffset2(GRAClist[i].obj,GRAClist[i].chardescent);
  return *(int *)(GRAClist[i].inst+idp);
}

void GRAinit(int GC,int leftm,int topm,int width,int height,int zoom)
{
  char code;
  int cpar[6];

  code='I';
  cpar[0]=5;
  cpar[1]=leftm;
  cpar[2]=topm;
  cpar[3]=width;
  cpar[4]=height;
  cpar[5]=zoom;
  GRAdraw(GC,code,cpar,NULL);
  GRAClist[GC].leftm=leftm;
  GRAClist[GC].topm=topm;
  GRAClist[GC].width=width;
  GRAClist[GC].height=height;
  GRAClist[GC].zoom=zoom/10000.0;
}

void GRAregion(int GC,int *leftm,int *topm,int *width,int *height,int *zoom)
{
  *leftm=GRAClist[GC].leftm;
  *topm=GRAClist[GC].topm;
  *width=GRAClist[GC].width;
  *height=GRAClist[GC].height;
  *zoom=GRAClist[GC].zoom*10000;
}

void GRAdirect(int GC,int cpar[])
{
  char code;

  code='X';
  GRAdraw(GC,code,cpar,NULL);
}

void GRAend(int GC)
{
  char code;
  int cpar[1];

  code='E';
  cpar[0]=0;
  GRAdraw(GC,code,cpar,NULL);
}

void GRAremark(int GC,char *s)
{
  char code;
  int cpar[1];
  char *cstr;
  char s2[1];

  code='%';
  cpar[0]=-1;
  s2[0]='\0';
  if (s==NULL) cstr=s2;
  else cstr=s;
  GRAdraw(GC,code,cpar,cstr);
}

void GRAview(int GC,int x1,int y1,int x2,int y2,int clip)
{
  char code;
  int cpar[6];

  if (x1==x2) x2++;
  if (y1==y2) y2++;
  code='V';
  cpar[0]=5;
  cpar[1]=x1;
  cpar[2]=y1;
  cpar[3]=x2;
  cpar[4]=y2;
  cpar[5]=clip;
  GRAdraw(GC,code,cpar,NULL);
}

void GRAlinestyle(int GC,int num,int *type,int width,int cap,int join,
                  int miter)
{
  char code;
  int *cpar;
  int i;

  if ((cpar=memalloc(sizeof(int)*(6+num)))==NULL) return;
  code='A';
  cpar[0]=5+num;
  cpar[1]=num;
  cpar[2]=width;
  cpar[3]=cap;
  cpar[4]=join;
  cpar[5]=miter;
  for (i=0;i<num;i++) cpar[6+i]=type[i];
  GRAdraw(GC,code,cpar,NULL);
  memfree(cpar);
}

void GRAcolor(int GC,int fr,int fg,int fb)
{
  char code;
  int cpar[4];

  if (fr>255) fr=255;
  if (fr<0) fr=0;
  if (fg>255) fg=255;
  if (fg<0) fg=0;
  if (fb>255) fb=255;
  if (fb<0) fb=0;
  code='G';
  cpar[0]=3;
  cpar[1]=fr;
  cpar[2]=fg;
  cpar[3]=fb;
  GRAdraw(GC,code,cpar,NULL);
}

void GRAtextstyle(int GC,char *font,int size,int space,int dir)
{
  char code;
  int cpar[4];
  char *cstr;

  if (font==NULL) return;
  code='F';
  cpar[0]=-1;
  cstr=font;
  GRAdraw(GC,code,cpar,cstr);
  code='H';
  cpar[0]=3;
  cpar[1]=size;
  cpar[2]=space;
  cpar[3]=dir;
  GRAdraw(GC,code,cpar,NULL);
}

void GRAmoveto(int GC,int x,int y)
{
  char code;
  int cpar[3];

  code='M';
  cpar[0]=2;
  cpar[1]=x;
  cpar[2]=y;
  GRAdraw(GC,code,cpar,NULL);
  GRAClist[GC].cpx=x;
  GRAClist[GC].cpy=y;
}

void GRAmoverel(int GC,int x,int y)
{
  char code;
  int cpar[3];

  GRAClist[GC].cpx+=x;
  GRAClist[GC].cpy+=y;
  if ((x!=0) || (y!=0)) {
/*    if ((GRAClist[GC].clip==0)
    || (GRAinview(GC,GRAClist[GC].cpx,GRAClist[GC].cpy)==0)) {  */
      code='N';
      cpar[0]=2;
      cpar[1]=x;
      cpar[2]=y;
      GRAdraw(GC,code,cpar,NULL);
/*    }  */
  }
}

void GRAline(int GC,int x0,int y0,int x1,int y1)
{
  char code;
  int cpar[5];

  if ((GRAClist[GC].clip==0) || (GRAlineclip(GC,&x0,&y0,&x1,&y1)==0)) {
    code='L';
    cpar[0]=4;
    cpar[1]=x0;
    cpar[2]=y0;
    cpar[3]=x1;
    cpar[4]=y1;
    GRAdraw(GC,code,cpar,NULL);
  }
}

void GRAlineto(int GC,int x,int y)
{
  char code;
  int cpar[3],x0,y0,x1,y1;

  x0=GRAClist[GC].cpx;
  y0=GRAClist[GC].cpy;
  x1=x;
  y1=y;
  if ((GRAClist[GC].clip==0) || (GRAlineclip(GC,&x0,&y0,&x1,&y1)==0)) {
    if ((x0==GRAClist[GC].cpx) && (y0==GRAClist[GC].cpy)) {
      code='T';
      cpar[0]=2;
      cpar[1]=x1;
      cpar[2]=y1;
      GRAdraw(GC,code,cpar,NULL);
    } else {
      code='M';
      cpar[0]=2;
      cpar[1]=x0;
      cpar[2]=y0;
      GRAdraw(GC,code,cpar,NULL);
      code='T';
      cpar[0]=2;
      cpar[1]=x1;
      cpar[2]=y1;
      GRAdraw(GC,code,cpar,NULL);
    }
  }
  GRAClist[GC].cpx=x;
  GRAClist[GC].cpy=y;
}

void GRAcircle(int GC,int x,int y,int rx,int ry,int cs,int ce,int fil)
{
  char code;
  int cpar[8];

  code='C';
  cpar[0]=7;
  cpar[1]=x;
  cpar[2]=y;
  cpar[3]=rx;
  cpar[4]=ry;
  cpar[5]=cs;
  cpar[6]=ce;
  cpar[7]=fil;
  GRAdraw(GC,code,cpar,NULL);
}

void GRArectangle(int GC,int x0,int y0,int x1,int y1,int fil)
{
  char code;
  int cpar[6];

  if ((GRAClist[GC].clip==0) || (GRArectclip(GC,&x0,&y0,&x1,&y1)==0)) {
    code='B';
    cpar[0]=5;
    cpar[1]=x0;
    cpar[2]=y0;
    cpar[3]=x1;
    cpar[4]=y1;
    cpar[5]=fil;
    GRAdraw(GC,code,cpar,NULL);
  }
}

void GRAputpixel(int GC,int x,int y)
{
  char code;
  int cpar[3];

  if ((GRAClist[GC].clip==0) || (GRAinview(GC,x,y)==0)) {
    code='P';
    cpar[0]=2;
    cpar[1]=x;
    cpar[2]=y;
    GRAdraw(GC,code,cpar,NULL);
  }
}

void GRAdrawpoly(int GC,int num,int *point,int fil)
{
  char code;
  int i,*cpar,num2;

  if (num<1) return;
  if ((point[0]!=point[num*2-2]) || (point[1]!=point[num*2-1])) num2=num+1;
  else num2=num;
  if ((cpar=memalloc(sizeof(int)*(3+2*num2)))==NULL) return;
  code='D';
  cpar[0]=2+2*num2;
  cpar[1]=num2;
  cpar[2]=fil;
  for (i=0;i<2*num;i++) cpar[i+3]=point[i];
  if ((point[0]!=point[num*2-2]) || (point[1]!=point[num*2-1])) {
    cpar[2*num+3]=point[0];
    cpar[2*num+4]=point[1];
  }
  GRAdraw(GC,code,cpar,NULL);
  memfree(cpar);
}

void GRAlines(int GC,int num,int *point)
{
  char code;
  int i,*cpar;

  if ((cpar=memalloc(sizeof(int)*(2+2*num)))==NULL) return;
  code='R';
  cpar[0]=2+2*num;
  cpar[1]=num;
  for (i=0;i<2*num;i++) cpar[i+2]=point[i];
  GRAdraw(GC,code,cpar,NULL);
  memfree(cpar);
}

void GRAmark(int GC,int type,int x0,int y0,int size,
             int fr,int fg,int fb,int br,int bg,int bb)
{
  int x1,y1,x2,y2,r;
  int po[10],po2[10];
  int type2,sgn;
  double d;

  if ((GRAClist[GC].clip==0) || (GRAinview(GC,x0,y0)==0)) {
    switch (type) {
    case 0: case 1: case 2: case 3: case 4:
    case 5: case 6: case 7: case 8: case 9:
      type2=type-0;
      r=size/2;
      if (type2==0) {
        GRAcolor(GC,fr,fg,fb);
        GRAcircle(GC,x0,y0,r,r,0,36000,1);
      } else if (type2==2) {
        GRAcolor(GC,fr,fg,fb);
        GRAcircle(GC,x0,y0,r,r,0,36000,0);
      } else if (type2==5) {
        GRAcolor(GC,fr,fg,fb);
        GRAcircle(GC,x0,y0,r,r,0,36000,1);
        r/=2;
        GRAcolor(GC,br,bg,bb);
        GRAcircle(GC,x0,y0,r,r,0,36000,1);
      } else {
        GRAcolor(GC,br,bg,bb);
        GRAcircle(GC,x0,y0,r,r,0,36000,1);
        GRAcolor(GC,fr,fg,fb);
        GRAcircle(GC,x0,y0,r,r,0,36000,0);
        if (type2==3) {
          r/=2;
          GRAcircle(GC,x0,y0,r,r,0,36000,0);
        } else if (type!=1) {
          if (type2==4) {
            r/=2;
            GRAcircle(GC,x0,y0,r,r,0,36000,1);
          } else if (type2==6) {
            GRAcircle(GC,x0,y0,r,r,27000,18000,1);
          } else if (type2==7) {
            GRAcircle(GC,x0,y0,r,r,9000,18000,1);
          } else if (type2==8) {
            GRAcircle(GC,x0,y0,r,r,0,18000,1);
          } else if (type2==9) {
            GRAcircle(GC,x0,y0,r,r,18000,18000,1);
	  }
	}
      }
      break;
    case 10: case 11: case 12: case 13: case 14:
    case 15: case 16: case 17: case 18: case 19:
      type2=type-10;
      x1=x0-size/2;
      y1=y0-size/2;
      x2=x0+size/2;
      y2=y0+size/2;
      if (type2==0) {
        GRAcolor(GC,fr,fg,fb);
        GRArectangle(GC,x1,y1,x2,y2,1);
      } else if (type2==2) {
        GRAcolor(GC,fr,fg,fb);
        GRArectangle(GC,x1,y1,x2,y2,0);
      } else if (type2==5) {
        GRAcolor(GC,fr,fg,fb);
        GRArectangle(GC,x1,y1,x2,y2,1);
        x1=x0-size/4;
        y1=y0-size/4;
        x2=x0+size/4;
        y2=y0+size/4;
        GRAcolor(GC,br,bg,bb);
        GRArectangle(GC,x1,y1,x2,y2,1);
      } else {
        GRAcolor(GC,br,bg,bb);
        GRArectangle(GC,x1,y1,x2,y2,1);
        GRAcolor(GC,fr,fg,fb);
        GRArectangle(GC,x1,y1,x2,y2,0);
        if (type2==3) {
          x1=x0-size/4;
          y1=y0-size/4;
          x2=x0+size/4;
          y2=y0+size/4;
          GRArectangle(GC,x1,y1,x2,y2,0);
        } else if (type2!=1) {
          if (type2==4) {
            x1=x0-size/4;
            y1=y0-size/4;
            x2=x0+size/4;
            y2=y0+size/4;
            GRArectangle(GC,x1,y1,x2,y2,1);
          } else if (type2==6) {
            x1=x0;
            GRArectangle(GC,x1,y1,x2,y2,1);
          } else if (type2==7) {
            x2=x0;
            GRArectangle(GC,x1,y1,x2,y2,1);
          } else if (type2==8) {
            y2=y0;
            GRArectangle(GC,x1,y1,x2,y2,1);
          } else if (type2==9) {
            y1=y0;
            GRArectangle(GC,x1,y1,x2,y2,1);
	      }
	    }
      }
      break;
    case 20: case 21: case 22: case 23: case 24:
    case 25: case 26: case 27: case 28: case 29:
      type2=type-20;
      po[0]=x0;
      po[1]=y0-size/2;
      po[2]=x0+size/2;
      po[3]=y0;
      po[4]=x0;
      po[5]=y0+size/2;
      po[6]=x0-size/2;
      po[7]=y0;
      po[8]=x0;
      po[9]=y0-size/2;
      if (type2==0) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,5,po,1);
      } else if (type2==2) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,5,po,0);
      } else if (type2==5) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,5,po,1);
        po[1]=y0-size/4;
        po[2]=x0+size/4;
        po[5]=y0+size/4;
        po[6]=x0-size/4;
        po[9]=y0-size/4;
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,5,po,1);
      } else {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,5,po,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,5,po,0);
        if (type2==3) {
          po[1]=y0-size/4;
          po[2]=x0+size/4;
          po[5]=y0+size/4;
          po[6]=x0-size/4;
          po[9]=y0-size/4;
          GRAdrawpoly(GC,5,po,0);
        } else if (type2!=1) {
          if (type2==4) {
            po[1]=y0-size/4;
            po[2]=x0+size/4;
            po[5]=y0+size/4;
            po[6]=x0-size/4;
            po[9]=y0-size/4;
            GRAdrawpoly(GC,5,po,1);
          } else if (type2==6) {
            po[6]=x0;
            po[7]=y0;
            GRAdrawpoly(GC,5,po,1);
          } else if (type2==7) {
            po[2]=x0;
            po[3]=y0;
            GRAdrawpoly(GC,5,po,1);
          } else if (type2==8) {
            po[4]=x0;
            po[5]=y0;
            GRAdrawpoly(GC,5,po,1);
          } else if (type2==9) {
            po[0]=x0;
            po[1]=y0;
            po[8]=x0;
            po[9]=y0;
            GRAdrawpoly(GC,5,po,1);
          }
        }
      }
      break;
    case 30: case 31: case 32: case 33: case 34:
    case 35: case 36: case 37:
    case 40: case 41: case 42: case 43: case 44:
    case 45: case 46: case 47:
      if (type>=40) {
        type2=type-40;
        sgn=-1;
      } else {
        type2=type-30;
        sgn=1;
      }
      d=sqrt(3.0);
      po[0]=x0;
      po[1]=y0-sgn*size/d;
      po[2]=x0+size/2;
      po[3]=y0+sgn*size/d/2;
      po[4]=x0-size/2;
      po[5]=y0+sgn*size/d/2;
      po[6]=x0;
      po[7]=y0-sgn*size/d;
      if (type2==0) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
      } else if (type2==2) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
      } else if (type2==5) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        po[1]=y0-sgn*size/d/2;
        po[2]=x0+size/4;
        po[3]=y0+sgn*size/d/4;
        po[4]=x0-size/4;
        po[5]=y0+sgn*size/d/4;
        po[7]=y0-sgn*size/d/2;
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
      } else {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
        if (type2==3) {
          po[1]=y0-sgn*size/d/2;
          po[2]=x0+size/4;
          po[3]=y0+sgn*size/d/4;
          po[4]=x0-size/4;
          po[5]=y0+sgn*size/d/4;
          po[7]=y0-sgn*size/d/2;
          GRAdrawpoly(GC,4,po,0);
        } else if (type2!=1) {
          if (type2==4) {
            po[1]=y0-sgn*size/d/2;
            po[2]=x0+size/4;
            po[3]=y0+sgn*size/d/4;
            po[4]=x0-size/4;
            po[5]=y0+sgn*size/d/4;
            po[7]=y0-sgn*size/d/2;
            GRAdrawpoly(GC,4,po,1);
          } else if (type2==6) {
            po[4]=x0;
            GRAdrawpoly(GC,4,po,1);
          } else if (type2==7) {
            po[2]=x0;
            GRAdrawpoly(GC,4,po,1);
          }
        }
      }
      break;
    case 50: case 51: case 52: case 53: case 54:
    case 55: case 56: case 57:
    case 60: case 61: case 62: case 63: case 64:
    case 65: case 66: case 67:
      if (type>=60) {
        type2=type-60;
        sgn=-1;
      } else {
        type2=type-50;
        sgn=1;
      }
      d=sqrt(3.0);
      po[0]=x0-sgn*size/d;
      po[1]=y0;
      po[2]=x0+sgn*size/d/2;
      po[3]=y0+size/2;
      po[4]=x0+sgn*size/d/2;
      po[5]=y0-size/2;
      po[6]=x0-sgn*size/d;
      po[7]=y0;
      if (type2==0) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
      } else if (type2==2) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
      } else if (type2==5) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        po[0]=x0-sgn*size/d/2;
        po[2]=x0+sgn*size/d/4;
        po[3]=y0+size/4;
        po[4]=x0+sgn*size/d/4;
        po[5]=y0-size/4;
        po[6]=x0-sgn*size/d/2;
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
      } else {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
        if (type2==3) {
          po[0]=x0-sgn*size/d/2;
          po[2]=x0+sgn*size/d/4;
          po[3]=y0+size/4;
          po[4]=x0+sgn*size/d/4;
          po[5]=y0-size/4;
          po[6]=x0-sgn*size/d/2;
          GRAdrawpoly(GC,4,po,0);
        } else if (type2!=1) {
          if (type2==4) {
            po[0]=x0-sgn*size/d/2;
            po[2]=x0+sgn*size/d/4;
            po[3]=y0+size/4;
            po[4]=x0+sgn*size/d/4;
            po[5]=y0-size/4;
            po[6]=x0-sgn*size/d/2;
            GRAdrawpoly(GC,4,po,1);
          } else if (type2==6) {
            po[5]=y0;
            GRAdrawpoly(GC,4,po,1);
          } else if (type2==7) {
            po[3]=y0;
            GRAdrawpoly(GC,4,po,1);
          }
        }
      }
      break;
    case 38: case 39: case 48: case 49:
      po[0]=x0;
      po[1]=y0;
      po[2]=x0-size/2;
      po[3]=y0-size/2;
      po[4]=x0+size/2;
      po[5]=y0-size/2;
      po[6]=x0;
      po[7]=y0;
      po2[0]=x0;
      po2[1]=y0;
      po2[2]=x0-size/2;
      po2[3]=y0+size/2;
      po2[4]=x0+size/2;
      po2[5]=y0+size/2;
      po2[6]=x0;
      po2[7]=y0;
      if (type==38) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po2,1);
      } else if (type==39) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po2,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      } else if (type==48) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po2,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      } else if (type==49) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po2,1);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      }
      break;
    case 58: case 59: case 68: case 69:
      po[0]=x0;
      po[1]=y0;
      po[2]=x0+size/2;
      po[3]=y0-size/2;
      po[4]=x0+size/2;
      po[5]=y0+size/2;
      po[6]=x0;
      po[7]=y0;
      po2[0]=x0;
      po2[1]=y0;
      po2[2]=x0-size/2;
      po2[3]=y0-size/2;
      po2[4]=x0-size/2;
      po2[5]=y0+size/2;
      po2[6]=x0;
      po2[7]=y0;
      if (type==58) {
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po2,1);
      } else if (type==59) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po2,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      } else if (type==68) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po2,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po,1);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      } else if (type==69) {
        GRAcolor(GC,br,bg,bb);
        GRAdrawpoly(GC,4,po,1);
        GRAcolor(GC,fr,fg,fb);
        GRAdrawpoly(GC,4,po2,1);
        GRAdrawpoly(GC,4,po,0);
        GRAdrawpoly(GC,4,po2,0);
      }
      break;
    case 70:
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0-size/2,y0,x0+size/2,y0);      
      GRAline(GC,x0,y0-size/2,x0,y0+size/2);      
      break;
    case 71:
      d=sqrt(2.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0-size/d/2,y0-size/d/2,x0+size/d/2,y0+size/d/2);      
      GRAline(GC,x0+size/d/2,y0-size/d/2,x0-size/d/2,y0+size/d/2);      
      break;
    case 72:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0+size/2,x0,y0-size/2);
      GRAline(GC,x0+size*d/4,y0-size/4,x0-size*d/4,y0+size/4);      
      GRAline(GC,x0-size*d/4,y0-size/4,x0+size*d/4,y0+size/4);      
      break;
    case 73:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0+size/2,y0,x0-size/2,y0);      
      GRAline(GC,x0-size/4,y0+size*d/4,x0+size/4,y0-size*d/4);      
      GRAline(GC,x0-size/4,y0-size*d/4,x0+size/4,y0+size*d/4);      
      break;
    case 74:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0,x0,y0-size/2);      
      GRAline(GC,x0,y0,x0-size*d/4,y0+size/4);      
      GRAline(GC,x0,y0,x0+size*d/4,y0+size/4);      
      break;
    case 75:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0,x0,y0+size/2);      
      GRAline(GC,x0,y0,x0-size*d/4,y0-size/4);      
      GRAline(GC,x0,y0,x0+size*d/4,y0-size/4);      
      break;
    case 76:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0,x0-size/2,y0);      
      GRAline(GC,x0,y0,x0+size/4,y0-size*d/4);      
      GRAline(GC,x0,y0,x0+size/4,y0+size*d/4);      
      break;
    case 77:
      d=sqrt(3.0);
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0,x0+size/2,y0);      
      GRAline(GC,x0,y0,x0-size/4,y0-size*d/4);      
      GRAline(GC,x0,y0,x0-size/4,y0+size*d/4);      
      break;
    case 78:
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0-size/2,y0,x0+size/2,y0);      
      break;
    case 79:
      GRAcolor(GC,fr,fg,fb);
      GRAline(GC,x0,y0-size/2,x0,y0+size/2);      
      break;
    case 80:
      r=size/2;
      GRAcolor(GC,fr,fg,fb);
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
      r/=2;
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
      break;
    case 81:
      r=size/2;
      GRAcolor(GC,fr,fg,fb);
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
      GRAline(GC,x0-size/2,y0,x0+size/2,y0);
      GRAline(GC,x0,y0-size/2,x0,y0+size/2);
      break;
    case 82:
      r=size/2;
      d=sqrt(2.0);
      GRAcolor(GC,fr,fg,fb);
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
      GRAline(GC,x0-size/d/2,y0-size/d/2,x0+size/d/2,y0+size/d/2);
      GRAline(GC,x0-size/d/2,y0+size/d/2,x0+size/d/2,y0-size/d/2);
      break;
    case 83:
      x1=x0-size/2;
      y1=y0-size/2;
      x2=x0+size/2;
      y2=y0+size/2;
      GRAcolor(GC,fr,fg,fb);
      GRArectangle(GC,x1,y1,x2,y2,0);
      x1=x0-size/4;
      y1=y0-size/4;
      x2=x0+size/4;
      y2=y0+size/4;
      GRArectangle(GC,x1,y1,x2,y2,0);
      break;
    case 84:
      x1=x0-size/2;
      y1=y0-size/2;
      x2=x0+size/2;
      y2=y0+size/2;
      GRAcolor(GC,fr,fg,fb);
      GRArectangle(GC,x1,y1,x2,y2,0);
      GRAline(GC,x0-size/2,y0,x0+size/2,y0);
      GRAline(GC,x0,y0-size/2,x0,y0+size/2);
      break;
    case 85:
      x1=x0-size/2;
      y1=y0-size/2;
      x2=x0+size/2;
      y2=y0+size/2;
      GRAcolor(GC,fr,fg,fb);
      GRArectangle(GC,x1,y1,x2,y2,0);
      GRAline(GC,x0-size/2,y0-size/2,x0+size/2,y0+size/2);      
      GRAline(GC,x0+size/2,y0-size/2,x0-size/2,y0+size/2);      
      break;
    case 86:
      po[0]=x0;
      po[1]=y0-size/2;
      po[2]=x0+size/2;
      po[3]=y0;
      po[4]=x0;
      po[5]=y0+size/2;
      po[6]=x0-size/2;
      po[7]=y0;
      po[8]=x0;
      po[9]=y0-size/2;
      GRAcolor(GC,fr,fg,fb);
      GRAdrawpoly(GC,5,po,0);
      po[1]=y0-size/4;
      po[2]=x0+size/4;
      po[5]=y0+size/4;
      po[6]=x0-size/4;
      po[9]=y0-size/4;
      GRAdrawpoly(GC,5,po,0);
      break;
    case 87:
      po[0]=x0;
      po[1]=y0-size/2;
      po[2]=x0+size/2;
      po[3]=y0;
      po[4]=x0;
      po[5]=y0+size/2;
      po[6]=x0-size/2;
      po[7]=y0;
      po[8]=x0;
      po[9]=y0-size/2;
      GRAcolor(GC,fr,fg,fb);
      GRAdrawpoly(GC,5,po,0);
      GRAline(GC,x0-size/2,y0,x0+size/2,y0);
      GRAline(GC,x0,y0-size/2,x0,y0+size/2);
      break;
    case 88:
      po[0]=x0;
      po[1]=y0-size/2;
      po[2]=x0+size/2;
      po[3]=y0;
      po[4]=x0;
      po[5]=y0+size/2;
      po[6]=x0-size/2;
      po[7]=y0;
      po[8]=x0;
      po[9]=y0-size/2;
      GRAcolor(GC,fr,fg,fb);
      GRAdrawpoly(GC,5,po,0);
      GRAline(GC,x0-size/4,y0-size/4,x0+size/4,y0+size/4);
      GRAline(GC,x0-size/4,y0+size/4,x0+size/4,y0-size/4);
      break;
    default:
      GRAcolor(GC,fr,fg,fb);
      GRAputpixel(GC,x0,y0);
      break;
    }
  }
}

void GRAouttext(int GC,char *s)
{
  char code;
  int cpar[1];
  char *cstr;

  code='S';
  cpar[0]=-1;
  cstr=s;
  GRAdraw(GC,code,cpar,cstr);
}

void GRAoutkanji(int GC,char *s)
{
  char code;
  int cpar[1];
  char *cstr;

  code='K';
  cpar[0]=-1;
  cstr=s;
  GRAdraw(GC,code,cpar,cstr);
}

char *GRAexpandobj(char **s);
char *GRAexpandmath(char **s);

char *GRAexpandobj(char **s)
{
  struct objlist *obj;
  int i,j,anum,*id;
  struct narray iarray;
  char *str,*arg,*ret;
  char *field;
  int len;
  int quote;

  *s=*s+2;
  str=field=arg=NULL;
  arrayinit(&iarray,sizeof(int));
  if ((str=nstrnew())==NULL) goto errexit;
  if ((arg=nstrnew())==NULL) goto errexit;
  if (chkobjilist2(s,&obj,&iarray,TRUE)) goto errexit;
  anum=arraynum(&iarray);
  if (anum<=0) goto errexit;
  id=arraydata(&iarray);
  if (((*s)==NULL)
    || (strchr(":= \t",(*s)[0])!=NULL)
    || ((field=getitok2(s,&len,":= \t}"))==NULL)) goto errexit;
  if (((*s)[0]!='\0') && ((*s)[0]!='}')) (*s)++;
  while (((*s)[0]==' ') || ((*s)[0]=='\t')) (*s)++;
  quote=FALSE;
  while (((*s)[0]!='\0') && (quote || ((*s)[0]!='}'))) {
    if (!quote && ((*s)[0]=='%') && ((*s)[1]=='{')) {
      ret=GRAexpandobj(s);
      arg=nstrcat(arg,ret);
      memfree(ret);
      if (arg==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='%') && ((*s)[1]=='[')) {
      ret=GRAexpandmath(s);
      arg=nstrcat(arg,ret);
      memfree(ret);
      if (str==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='\\')) {
      quote=TRUE;
      (*s)++;
    } else {
      if (quote) quote=FALSE;
      if ((arg=nstrccat(arg,(*s)[0]))==NULL) goto errexit;
      (*s)++;
    }
  }
  if ((*s)[0]!='}') goto errexit;
  (*s)++;
  for (i=0;i<anum;i++) {
    if (schkobjfield(obj,id[i],field,arg,&ret,TRUE,FALSE,FALSE)!=0)
      goto errexit;
    for (j=0;ret[j]!='\0';j++) {
      if ((ret[j]=='%') || (ret[j]=='\\')
       || (ret[j]=='_') || (ret[j]=='^') || (ret[j]=='@')) {
        if ((str=nstrccat(str,'\\'))==NULL) {
          memfree(ret);
          goto errexit;
        }
      }
      if ((str=nstrccat(str,ret[j]))==NULL) {
        memfree(ret);
        goto errexit;
      }
    }
    memfree(ret);
  }
  arraydel(&iarray);
  memfree(field);
  memfree(arg);
  return str;
errexit:
  arraydel(&iarray);
  memfree(str);
  memfree(field);
  memfree(arg);
  return NULL;
}

char *GRAexpandmath(char **s)
{
  int j;
  char *str,*ret;
  int quote;
  int rcode;
  char *code;
  double vd;
  double memory[MEMORYNUM];
  char memorystat[MEMORYNUM];

  *s=*s+2;
  str=NULL;
  if ((str=nstrnew())==NULL) goto errexit;
  if ((*s)==NULL) goto errexit;
  quote=FALSE;
  while (((*s)[0]!='\0') && (quote || ((*s)[0]!=']'))) {
    if (!quote && ((*s)[0]=='%') && ((*s)[1]=='{')) {
      ret=GRAexpandobj(s);
      str=nstrcat(str,ret);
      memfree(ret);
      if (str==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='%') && ((*s)[1]=='[')) {
      ret=GRAexpandmath(s);
      str=nstrcat(str,ret);
      memfree(ret);
      if (str==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='\\')) {
      quote=TRUE;
      (*s)++;
    } else {
      if (quote) quote=FALSE;
      if ((str=nstrccat(str,(*s)[0]))==NULL) goto errexit;
      (*s)++;
    }
  }
  if ((*s)[0]!=']') goto errexit;
  (*s)++;
  rcode=mathcode(str,&code,NULL,NULL,NULL,NULL,
                 FALSE,FALSE,FALSE,FALSE,FALSE,
                 FALSE,FALSE,FALSE,FALSE,FALSE,FALSE);
  if (rcode!=MCNOERR) goto errexit;
  for (j=0;j<MEMORYNUM;j++) {memory[j]=0;memorystat[j]=MNOERR;}
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
  if (rcode!=MNOERR) goto errexit;
  memfree(str);
  if ((str=memalloc(24))==NULL) goto errexit;
  sprintf(str,"%.15e",vd);
  return str;
errexit:
  memfree(str);
  return NULL;
}

char *GRAexpandpf(char **s)
{
  int i,j,len,len1,len2,err;
  char *str,*format,*format2,*s2,*ret,*ret2;
  int quote;
  int vi;
  double vd;
  char *buf;
  char *endptr;

  *s=*s+4;
  str=NULL;
  format=NULL;
  if ((ret=nstrnew())==NULL) goto errexit;
  if ((str=nstrnew())==NULL) goto errexit;
  if ((*s)==NULL) goto errexit;
  if ((format=getitok2(s,&len," \t}"))==NULL) goto errexit;
  if (((*s)[0]!='\0') && ((*s)[0]!='}')) (*s)++;
  while (((*s)[0]==' ') || ((*s)[0]=='\t')) (*s)++;
  quote=FALSE;
  while (((*s)[0]!='\0') && (quote || ((*s)[0]!='}'))) {
    if (!quote && ((*s)[0]=='%') && ((*s)[1]=='{')) {
      ret2=GRAexpandobj(s);
      str=nstrcat(str,ret2);
      memfree(ret2);
      if (str==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='%') && ((*s)[1]=='[')) {
      ret2=GRAexpandmath(s);
      str=nstrcat(str,ret2);
      memfree(ret2);
      if (str==NULL) goto errexit;
    } else if (!quote && ((*s)[0]=='\\')) {
      quote=TRUE;
      (*s)++;
    } else {
      if (quote) quote=FALSE;
      if ((str=nstrccat(str,(*s)[0]))==NULL) goto errexit;
      (*s)++;
    }
  }
  if ((*s)[0]!='}') goto errexit;
  (*s)++;

  if (format[0]=='%') {
    for (i=1;(format[i]!='\0')
         && (strchr("diouxXeEfgcs%",format[i])==NULL);i++);
    if (format[i]!='\0') {
      if ((format2=memalloc(i+2))==NULL) goto errexit;
      strncpy(format2,format,i+1);
      format2[i+1]='\0';
      len1=len2=0;
      err=FALSE;
      s2=format2;
      for (j=0;(s2[j]!='\0') && (!isdigit(s2[j]));j++);
      if (isdigit(s2[j])) {
        len1=strtol(s2+j,&endptr,10);
        if (len1<0) len1=0;
        if (endptr[0]=='.') {
          s2=endptr;
          for (j=0;(s2[j]!='\0') && (strchr(".*",s2[j])!=NULL);j++);
          if (isdigit(s2[j])) {
            len2=strtol(s2+j,&endptr,10);
            if (len2<0) len2=0;
          } else err=TRUE;
        }
      }
      if (strchr(format2,'*')!=NULL) err=TRUE;
      if (!err) {
        len=len1+len2+256;
        switch (format[i]) {
        case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
          vi=strtol(str,&endptr,10);
          if ((buf=memalloc(len))!=NULL) {
            sprintf(buf,format2,vi);
            ret=nstrcat(ret,buf);
            memfree(buf);
          }
          break;
        case 'e': case 'E': case 'f': case 'g':
          vd=strtod(str,&endptr);
          if ((buf=memalloc(len))!=NULL) {
            sprintf(buf,format2,vd);
            ret=nstrcat(ret,buf);
            memfree(buf);
          }
          break;
        case 's':
          if ((buf=memalloc(len+strlen(str)))!=NULL) {
            sprintf(buf,format2,str);
            ret=nstrcat(ret,buf);
            memfree(buf);
          }
          break;
        case 'c':
          if ((buf=memalloc(len+strlen(str)))!=NULL) {
            sprintf(buf,format2,str[0]);
            ret=nstrcat(ret,buf);
            memfree(buf);
          }
        }
      }
      memfree(format2);
      if (ret==NULL) goto errexit;
    }
  }

  memfree(str);
  memfree(format);
  return ret;
errexit:
  memfree(str);
  memfree(format);
  return NULL;
}

char *GRAexpandtext(char *s)
{
  int i,j,len;
  char *str;
  char *snew,*s2;

  if (s==NULL) return NULL;
  if ((str=nstrnew())==NULL) return NULL;
  len=strlen(s);
  j=0;
  do {
    i=j;
    while ((j<len) && (((s[j]!='\\') && (s[j]!='%')) || niskanji2(s,j)))
      j++;
    if ((str=nstrncat(str,s+i,j-i))==NULL) return NULL;
    if (s[j]=='\\') {
      if (s[j+1]=='n') {
        if ((str=nstrccat(str,'\n'))==NULL) return NULL;
        j+=2;
      } else if (s[j+1]=='b') {
        if ((str=nstrccat(str,'\b'))==NULL) return NULL;
        j+=2;
      } else if (s[j+1]=='&') {
        if ((str=nstrccat(str,'\r'))==NULL) return NULL;
        j+=2;
      } else if (s[j+1]=='.') {
        if ((str=nstrccat(str,' '))==NULL) return NULL;
        j+=2;
      } else if (s[j+1]=='-') {
        if ((str=nstrccat(str,'\\'))==NULL) return NULL;
        if ((str=nstrccat(str,'x'))==NULL) return NULL;
        if ((str=nstrccat(str,'A'))==NULL) return NULL;
        if ((str=nstrccat(str,'D'))==NULL) return NULL;
        j+=2;
      } else if ((s[j+1]=='x') && isxdigit(s[j+2]) && isxdigit(s[j+3])) {
        if ((str=nstrccat(str,'\\'))==NULL) return NULL;
        if ((str=nstrccat(str,'x'))==NULL) return NULL;
        if ((str=nstrccat(str,toupper(s[j+2])))==NULL) return NULL;
        if ((str=nstrccat(str,toupper(s[j+3])))==NULL) return NULL;
        j+=4;
      } else if (strchr("\\%@^_",s[j+1])!=NULL) {
        if ((str=nstrccat(str,'\\'))==NULL) return NULL;
        if ((str=nstrccat(str,s[j+1]))==NULL) return NULL;
        j+=2;
      } else {
        j++;
      }
    } else if ((s[j]=='%') && (s[j+1]=='{')) {
      s2=s+j;
      snew=GRAexpandobj(&s2);
      if ((str=nstrcat(str,snew))==NULL) return NULL;
      memfree(snew);
      j=(s2-s);
    } else if ((s[j]=='%') && (s[j+1]=='[')) {
      s2=s+j;
      snew=GRAexpandmath(&s2);
      if ((str=nstrcat(str,snew))==NULL) return NULL;
      memfree(snew);
      j=(s2-s);
    } else if ((s[j]=='%') && (s[j+1]=='p') && (s[j+2]=='f') && (s[j+3]=='{')) {
      s2=s+j;
      snew=GRAexpandpf(&s2);
      if ((str=nstrcat(str,snew))==NULL) return NULL;
      memfree(snew);
      j=(s2-s);
    } else if (j<len) {
      if ((str=nstrccat(str,s[j]))==NULL) return NULL;
      j++;
    }
  } while (j<len);
  return str;
}

void GRAdrawtext(int GC,char *s,char *font,char *jfont,
				 int size,int space,int dir,int scriptsize)
{
  char *c,*tok;
  char *str;
  int len,kanji,scmovex,scmovey,scriptf;
  char *endptr;
  char *font2,*jfont2;
  int size2,space2;
  char *font3,*jfont3;
  int size3,space3;
  int i,j,k,x,y,val,x0,y0,x1,y1;
  double cs,si;
  int height;
  int alignlen,fx0,fx1,fy0,fy1;
  char ch;

  if ((font==NULL) || (jfont==NULL)) return;
  font2=NULL;
  jfont2=NULL;
  font3=NULL;
  jfont3=NULL;
  str=NULL;
  cs=cos(dir*MPI/18000);
  si=sin(dir*MPI/18000);
  x0=GRAClist[GC].cpx;
  y0=GRAClist[GC].cpy;
  if (s==NULL) return;
  if ((c=GRAexpandtext(s))==NULL) goto errexit;
  if (c[0]=='\0') goto errexit;
  if ((font2=memalloc(strlen(font)+1))==NULL) goto errexit;
  strcpy(font2,font);
  if ((jfont2=memalloc(strlen(jfont)+1))==NULL) goto errexit;
  strcpy(jfont2,jfont);
  size2=size;
  space2=space;
  scriptf=0;
  scmovex=0;
  scmovey=0;

  len=strlen(c);

  for (k=0;(k<len) && (c[k]!='\n') && (c[k]!='\r');k++);
  if (c[k]=='\r') {
    ch=c[k];
    c[k]='\0';
    GRAtextextent(c,font2,jfont2,size2,space2,scriptsize,
                  &fx0,&fy0,&fx1,&fy1,TRUE);
    c[k]=ch;
    alignlen=fx1;
  } else alignlen=0;

  kanji=FALSE;
  j=0;
  do {
    if ((str=nstrnew())==NULL) goto errexit;
    kanji=niskanji((unsigned char)c[j]);
    while ( (j<len)
    && ((strchr("\n\r\b_^@%",c[j])==NULL) || niskanji2(c,j))
    && ( (kanji && niskanji((unsigned char)c[j]))
      || (!kanji && !niskanji((unsigned char)c[j])) || (!kanji && (c[j]=='\\')) ) ) {
      if (kanji) {
        if (((str=nstrccat(str,c[j]))==NULL)
         || ((str=nstrccat(str,c[j+1]))==NULL)) goto errexit;
        j+=2;
      } else if (c[j]=='\\') {
        if (c[j+1]=='x') {
          if (((str=nstrccat(str,c[j]))==NULL)
           || ((str=nstrccat(str,c[j+1]))==NULL)
           || ((str=nstrccat(str,c[j+2]))==NULL)
           || ((str=nstrccat(str,c[j+3]))==NULL)) goto errexit;
          j+=4;
        } else if (c[j+1]=='\\') {
          if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
          if ((str=nstrccat(str,c[j+1]))==NULL) goto errexit;
          j+=2;
        } else if ((c[j+1]!='\0') && !niskanji((unsigned char )c[j+1])) {
          if ((str=nstrccat(str,c[j+1]))==NULL) goto errexit;
          j+=2;
        } else j++;
      } else {
        if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
        j++;
      }
    }
    if (str==NULL) goto errexit;
    if (str[0]!='\0') {
      if (kanji) {
        GRAtextstyle(GC,jfont2,size2,space2,dir);
        GRAoutkanji(GC,str);
      } else {
        GRAtextstyle(GC,font2,size2,space2,dir);
        GRAouttext(GC,str);
      }
    }
    memfree(str);
    str=NULL;

    if (c[j]=='\n') {
      x0+=(int )(si*size*25.4/72.0);
      y0+=(int )(cs*size*25.4/72.0);
      if (scriptf!=0) {
        scriptf=0;
        memfree(font2);
        memfree(jfont2);
        font2=font3;
        jfont2=jfont3;
        font3=NULL;
        jfont3=NULL;
        size2=size3;
        space2=space3;
      }
      scmovex=0;
      scmovey=0;
      if (alignlen!=0) {
        for (k=j+1;(k<len) && (c[k]!='\n') && (c[k]!='\r');k++);
        ch=c[k];
        c[k]='\0';
        GRAtextextent(c+j+1,font2,jfont2,size2,space2,scriptsize,
                      &fx0,&fy0,&fx1,&fy1,TRUE);
        c[k]=ch;
        x1=x0+(int )(cs*(alignlen-fx1));
        y1=y0+(int )(-si*(alignlen-fx1));
      } else {
        x1=x0;
        y1=y0;
      }
      GRAmoveto(GC,x1,y1);
      j++;
    } else if (c[j]=='\b') {
      GRAtextextent("h",font2,jfont2,size2,space2,scriptsize,
                    &fx0,&fy0,&fx1,&fy1,TRUE);
      x1=(int )(cs*(fx1-fx0));
      y1=(int )(si*(fx1-fx0));
      GRAmoverel(GC,-x1,y1);
      j++;
    } else if (c[j]=='\r') {
      j++;
    } else if ((c[j]!='\0') && (strchr("_^@",c[j])!=NULL)) {
      switch (c[j]) {
      case '^':
      case '_':
        if (scriptf==0) {
          if ((font3=memalloc(strlen(font2)+1))==NULL) goto errexit;
          strcpy(font3,font2);
          if ((jfont3=memalloc(strlen(jfont2)+1))==NULL) goto errexit;
          strcpy(jfont3,jfont2);
          size3=size2;
          space3=space2;
        }
        height=size2;
        size2=(int )(size2*1e-4*scriptsize);
        space2=(int )(space2*1e-4*scriptsize);
        if (c[j]=='^') {
          x=(int )(-si*(height*0.8-size2*5e-5*scriptsize)*25.4/72.0);
          y=(int )(-cs*(height*0.8-size2*5e-5*scriptsize)*25.4/72.0);
          GRAmoverel(GC,x,y);
          scmovex+=x;
          scmovey+=y;
        } else {
          x=(int )(si*size2*5e-5*scriptsize*25.4/72.0);
          y=(int )(cs*size2*5e-5*scriptsize*25.4/72.0);
          GRAmoverel(GC,x,y);
          scmovex+=x;
          scmovey+=y;
        }
        if (c[j]=='^') scriptf=1;
        else scriptf=2;
		break;
      case '@':
        if (scriptf!=0) {
          scriptf=0;
          GRAmoverel(GC,-scmovex,-scmovey);
          memfree(font2);
          memfree(jfont2);
          font2=font3;
          jfont2=jfont3;
          font3=NULL;
          jfont3=NULL;
          size2=size3;
          space2=space3;
        }
		scmovex=0;
		scmovey=0;
        break;
      }
      j++;
    } else if (c[j]=='%') {
      if ((c[j+1]!='\0') && (strchr("FJSPXY",toupper(c[j+1]))!=NULL) && (c[j+2]=='{')) {
        for (i=j+3;(c[i]!='\0') && (c[i]!='}');i++);
        if (c[i]=='}') {
          if ((tok=memalloc(i-j-2))==NULL) goto errexit;
          strncpy(tok,c+j+3,i-j-3);
          tok[i-j-3]='\0';
          if (tok[0]!='\0') {
            switch (toupper(c[j+1])) {
            case 'F':
              memfree(font2);
              font2=tok;
              break;
            case 'J':
              memfree(jfont2);
              jfont2=tok;
              break;
            case 'S':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') size2=val*100;
              memfree(tok);
              break;
            case 'P':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') space2=val*100;
              memfree(tok);
              break;
            case 'X':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') GRAmoverel(GC,(int )(val*100*25.4/72.0),0);
              memfree(tok);
              break;
            case 'Y':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') GRAmoverel(GC,0,(int )(val*100*25.4/72.0));
              memfree(tok);
              break;
            }
          }
        }
        j=i+1;
      } else j++;
    }
  } while (j<len);

errexit:
  memfree(str);
  memfree(c);
  memfree(font2);
  memfree(jfont2);
  memfree(font3);
  memfree(jfont3);
}

void GRAdrawtextraw(int GC,char *s,char *font,char *jfont,
                    int size,int space,int dir)
{
  char *c;
  char *str;
  int len,kanji;
  int j;

  if ((font==NULL) || (jfont==NULL)) return;
  str=NULL;
  if (s==NULL) return;
  c=s;
  len=strlen(c);
  kanji=FALSE;
  j=0;
  do {
    if ((str=nstrnew())==NULL) goto errexit;
    kanji=niskanji((unsigned char)c[j]);
    while ( (j<len)
    && ( (kanji && niskanji((unsigned char)c[j]))
      || (!kanji && !niskanji((unsigned char)c[j])) || (!kanji && (c[j]=='\\')) ) ) {
      if (kanji) {
        if (((str=nstrccat(str,c[j]))==NULL)
         || ((str=nstrccat(str,c[j+1]))==NULL)) goto errexit;
        j+=2;
      } else if (c[j]=='\\') {
          if ((str=nstrccat(str,'\\'))==NULL) goto errexit;
          if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
          j++;
      } else {
        if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
        j++;
      }
    }
    if (str==NULL) goto errexit;
    if (str[0]!='\0') {
      if (kanji) {
        GRAtextstyle(GC,jfont,size,space,dir);
        GRAoutkanji(GC,str);
      } else {
        GRAtextstyle(GC,font,size,space,dir);
        GRAouttext(GC,str);
      }
    }
    memfree(str);
    str=NULL;
  } while (j<len);

errexit:
  memfree(str);
}

void GRAtextextent(char *s,char *font,char *jfont,
                   int size,int space,int scriptsize,
                   int *gx0,int *gy0,int *gx1,int *gy1,int raw)
{
  char *c,*tok;
  char *str;
  int w,h,d,len,kanji,scmovey,scriptf;
  char *endptr;
  char *font2,*jfont2;
  int size2,space2;
  char *font3,*jfont3;
  int size3,space3;
  int i,j,k,y,val,x0,y0;
  int height;
  int alignlen,fx0,fx1,fy0,fy1;
  char ch;

  *gx0=*gy0=*gx1=*gy1=0;
  if ((font==NULL) || (jfont==NULL)) return;
  font2=NULL;
  jfont2=NULL;
  font3=NULL;
  jfont3=NULL;
  str=NULL;
  x0=0;
  y0=0;
  if (s==NULL) return;
  if ((c=GRAexpandtext(s))==NULL) goto errexit;
  if (c[0]=='\0') goto errexit;
  if ((font2=memalloc(strlen(font)+1))==NULL) goto errexit;
  strcpy(font2,font);
  if ((jfont2=memalloc(strlen(jfont)+1))==NULL) goto errexit;
  strcpy(jfont2,jfont);
  size2=size;
  space2=space;
  scriptf=0;
  scmovey=0;

  len=strlen(c);

  if (!raw) {
    for (k=0;(k<len) && (c[k]!='\n') && (c[k]!='\r');k++);
    if (c[k]=='\r') {
      ch=c[k];
      c[k]='\0';
      GRAtextextent(c,font2,jfont2,size2,space2,scriptsize,
                    &fx0,&fy0,&fx1,&fy1,TRUE);
      c[k]=ch;
      alignlen=fx1;
    } else alignlen=0;
  }

  kanji=FALSE;
  j=0;
  do {
    if ((str=nstrnew())==NULL) goto errexit;
    kanji=niskanji((unsigned char)c[j]);
    while ((j<len) && ((strchr("\n\b\r_^@%",c[j])==NULL) || niskanji2(c,j))
    && ( (kanji && niskanji((unsigned char)c[j]))
      || (!kanji && !niskanji((unsigned char)c[j])) || (!kanji && (c[j]=='\\')) ) ) {
      if (kanji) {
        if (((str=nstrccat(str,c[j]))==NULL)
         || ((str=nstrccat(str,c[j+1]))==NULL))
          goto errexit;
        j+=2;
      } else if (c[j]=='\\') {
        if (c[j+1]=='x') {
          if (((str=nstrccat(str,c[j]))==NULL)
           || ((str=nstrccat(str,c[j+1]))==NULL)
           || ((str=nstrccat(str,c[j+2]))==NULL)
           || ((str=nstrccat(str,c[j+3]))==NULL)) goto errexit;
          j+=4;
        } else if ((c[j+1]!='\0') && !niskanji((unsigned char )c[j+1])) {
          if ((str=nstrccat(str,c[j+1]))==NULL) goto errexit;
          j+=2;
        } else j++;
      } else {
        if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
        j++;
      }
    }
    if (str==NULL) goto errexit;
    if (str[0]!='\0') {
      if (kanji) {
        w=0;
        for (i=0;i<strlen(str);i+=2) {
          w+=GRAcharwidth((((unsigned char)str[i+1])<<8)+(unsigned char)str[i],
                           jfont2,size2)+nround(space2/72.0*25.4);
        }
        h=GRAcharascent(jfont2,size2);
        d=GRAchardescent(jfont2,size2);
      } else {
        w=0;
        for (i=0;i<strlen(str);i++) {
          if ((str[i]=='\\') && (str[i+1]=='x')) {
            if (toupper(str[i+2])>='A') ch=toupper(str[i+2])-'A'+10;
            else ch=str[i+2]-'0';
            if (toupper(str[i+3])>='A') ch=ch*16+toupper(str[i+3])-'A'+10;
            else ch=ch*16+str[i+3]-'0';
            str[i]=ch;
            w+=GRAcharwidth((unsigned char)str[i],font2,size2)
              +nround(space2/72.0*25.4);
            i+=3;
          } else {
            w+=GRAcharwidth((unsigned char)str[i],font2,size2)
              +nround(space2/72.0*25.4);
          }
        }
        h=GRAcharascent(font2,size2);
        d=GRAchardescent(font2,size2);
      }
      if (x0<*gx0) *gx0=x0;
      if (x0+w<*gx0) *gx0=x0+w;
      if (x0>*gx1) *gx1=x0;
      if (x0+w>*gx1) *gx1=x0+w;
      if (y0-h<*gy0) *gy0=y0-h;
      if (y0+d<*gy0) *gy0=y0+d;
      if (y0-h>*gy1) *gy1=y0-h;
      if (y0+d>*gy1) *gy1=y0+d;
      x0+=w;
    }
    memfree(str);
    str=NULL;

    if (c[j]=='\n') {
      y0+=(int )(size*25.4/72.0);
      if (scriptf!=0) {
        scriptf=0;
        memfree(font2);
        memfree(jfont2);
        font2=font3;
        jfont2=jfont3;
        font3=NULL;
        jfont3=NULL;
        size2=size3;
        space2=space3;
      }
      scmovey=0;
      if ((!raw) && (alignlen!=0)) {
        for (k=j+1;(k<len) && (c[k]!='\n') && (c[k]!='\r');k++);
        ch=c[k];
        c[k]='\0';
        GRAtextextent(c+j+1,font2,jfont2,size2,space2,scriptsize,
                      &fx0,&fy0,&fx1,&fy1,TRUE);
        c[k]=ch;
        x0=alignlen-fx1;
      } else x0=0;
      j++;
    } else if (c[j]=='\b') {
      GRAtextextent("h",font2,jfont2,size2,space2,scriptsize,
                    &fx0,&fy0,&fx1,&fy1,TRUE);
      x0-=(fx1-fx0);
      j++;
    } else if (c[j]=='\r') {
      j++;
    } else if ((c[j]!='\0') && (strchr("_^@",c[j])!=NULL)) {
      switch (c[j]) {
      case '^':
      case '_':
        if (scriptf==0) {
          if ((font3=memalloc(strlen(font2)+1))==NULL) goto errexit;
          strcpy(font3,font2);
          if ((jfont3=memalloc(strlen(jfont2)+1))==NULL) goto errexit;
          strcpy(jfont3,jfont2);
          size3=size2;
          space3=space2;
        }
        height=size2;
        size2=(int )(size2*1e-4*scriptsize);
        space2=(int )(space2*1e-4*scriptsize);
        if (c[j]=='^') {
          y=(int )(-(height*0.8-size2*5e-5*scriptsize)*25.4/72.0);
          y0+=y;
          scmovey+=y;
        } else {
          y=(int )(size2*5e-5*scriptsize*25.4/72.0);
          y0+=y;
          scmovey+=y;
        }
        if (c[j]=='^') scriptf=1;
		else scriptf=2;
		break;
      case '@':
        if (scriptf!=0) {
          scriptf=0;
          y0-=scmovey;
          memfree(font2);
          memfree(jfont2);
          font2=font3;
          jfont2=jfont3;
          font3=NULL;
		  jfont3=NULL;
          size2=size3;
          space2=space3;
		}
		scmovey=0;
        break;
      }
      j++;
    } else if (c[j]=='%') {
      if ((c[j+1]!='\0') && (strchr("FJSPXY",toupper(c[j+1]))!=NULL) && (c[j+2]=='{')) {
        for (i=j+3;(c[i]!='\0') && (c[i]!='}');i++);
        if (c[i]=='}') {
          if ((tok=memalloc(i-j-2))==NULL) goto errexit;
          strncpy(tok,c+j+3,i-j-3);
          tok[i-j-3]='\0';
          if (tok[0]!='\0') {
            switch (toupper(c[j+1])) {
            case 'F':
              memfree(font2);
              font2=tok;
              break;
            case 'J':
              memfree(jfont2);
              jfont2=tok;
              break;
            case 'S':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') size2=val*100;
              memfree(tok);
              break;
            case 'P':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') space2=val*100;
              memfree(tok);
              break;
            case 'X':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') x0+=(int )(val*100*25.4/72.0);
              memfree(tok);
              break;
            case 'Y':
              val=strtol(tok,&endptr,10);
              if (endptr[0]=='\0') y0+=(int )(val*100*25.4/72.0);
              memfree(tok);
              break;
            }
          }
        }
        j=i+1;
      } else j++;
    }
  } while (j<len);

errexit:
  memfree(str);
  memfree(c);
  memfree(font2);
  memfree(jfont2);
  memfree(font3);
  memfree(jfont3);
}

void GRAtextextentraw(char *s,char *font,char *jfont,
                   int size,int space,int *gx0,int *gy0,int *gx1,int *gy1)
{
  char *c;
  char *str;
  int w,h,len,kanji;
  int i,j,x0,y0;

  *gx0=*gy0=*gx1=*gy1=0;
  if ((font==NULL) || (jfont==NULL)) return;
  str=NULL;
  x0=0;
  y0=0;
  if (s==NULL) return;
  c=s;
  len=strlen(c);
  kanji=FALSE;
  j=0;
  do {
    if ((str=nstrnew())==NULL) goto errexit;
    kanji=niskanji((unsigned char)c[j]);
    while ((j<len)
    && ( (kanji && niskanji((unsigned char)c[j]))
      || (!kanji && !niskanji((unsigned char)c[j])) || (!kanji && (c[j]=='\\')) )) {
      if (kanji) {
        if (((str=nstrccat(str,c[j]))==NULL)
         || ((str=nstrccat(str,c[j+1]))==NULL))
          goto errexit;
        j+=2;
      } else if (c[j]=='\\') {
        if ((str=nstrccat(str,'\\'))==NULL) goto errexit;
        if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
        j++;
      } else {
        if ((str=nstrccat(str,c[j]))==NULL) goto errexit;
        j++;
      }
    }
    if (str==NULL) goto errexit;
    if (str[0]!='\0') {
      if (kanji) {
        w=0;
        for (i=0;i<strlen(str);i+=2) {
          w+=GRAcharwidth((((unsigned char)str[i+1])<<8)+(unsigned char)str[i],
                           jfont,size)+nround(space/72.0*25.4);
        }
        h=GRAcharascent(jfont,size);
      } else {
        w=0;
        for (i=0;i<strlen(str);i++)
          w+=GRAcharwidth((unsigned char)str[i],font,size)
            +nround(space/72.0*25.4);
        h=GRAcharascent(font,size);
      }
      if (x0<*gx0) *gx0=x0;
      if (x0+w<*gx0) *gx0=x0+w;
      if (x0>*gx1) *gx1=x0;
      if (x0+w>*gx1) *gx1=x0+w;
      if (y0-h<*gy0) *gy0=y0-h;
      if (y0<*gy0) *gy0=y0;
      if (y0>*gy1) *gy1=y0;
      if (y0-h>*gy1) *gy1=y0-h;
      x0+=w;
    }
    memfree(str);
    str=NULL;
  } while (j<len);

errexit:
  memfree(str);
}


int getintpar(char *s,int num,int cpar[])
{
  int i,pos1,pos2;
  char s2[256];
  char *endptr;

  pos1=0;
  for (i=0;i<num;i++) {
    while ((s[pos1]!='\0') &&
          ((s[pos1]==' ') || (s[pos1]=='\t') || (s[pos1]==','))) pos1++;
    if (s[pos1]=='\0') return FALSE;
    pos2=0;
    while ((s[pos1]!='\0') &&
           (s[pos1]!=' ') && (s[pos1]!='\t') && (s[pos1]!=',')) {
      s2[pos2]=s[pos1];
      pos2++;
      pos1++;
    }
    s2[pos2]='\0';
    cpar[i]=strtol(s2,&endptr,10);
    if (endptr[0]!='\0') return FALSE;
  }
  return TRUE;
}

void GRAinputdraw(int GC,int leftm,int topm,int rate,
                  char code,int *cpar,char *cstr)
{
  int i;
  double r;

  if (GRAClist[GC].mergezoom==0) r=1;
  else r=((double )rate)/GRAClist[GC].mergezoom;
  switch (code) {
  case '%': case 'G':
  case 'O': case 'Q': case 'F': case 'S': case 'K':
    break;
  case 'I':
    GRAClist[GC].mergeleft=cpar[1];
    GRAClist[GC].mergetop=cpar[2];
    GRAClist[GC].mergezoom=cpar[5];
    code='\0';
    break;
  case 'E':
    GRAClist[GC].mergeleft=0;
    GRAClist[GC].mergetop=0;
    GRAClist[GC].mergezoom=10000;
    code='\0';
    break;
  case 'V':
    cpar[1]=(int )(((cpar[1]-GRAClist[GC].mergeleft)*r)+leftm);
    cpar[2]=(int )(((cpar[2]-GRAClist[GC].mergetop)*r)+topm);
    cpar[3]=(int )(((cpar[3]-GRAClist[GC].mergeleft)*r)+leftm);
    cpar[4]=(int )(((cpar[4]-GRAClist[GC].mergetop)*r)+topm);
    break;
  case 'A':
    cpar[2]=(int )(cpar[2]*r);
    for (i=0;i<cpar[1];i++) cpar[i+5]=(int )(cpar[i+5]*r);
    break;
  case 'M': case 'N': case 'T': case 'P': case 'H':
    cpar[1]=(int )(cpar[1]*r);
    cpar[2]=(int )(cpar[2]*r);
    break;
  case 'L': case 'C': case 'B':
    cpar[1]=(int )(cpar[1]*r);
    cpar[2]=(int )(cpar[2]*r);
    cpar[3]=(int )(cpar[3]*r);
    cpar[4]=(int )(cpar[4]*r);
    break;
  case 'R':
    for (i=0;i<(2*(cpar[1]));i++) cpar[i+2]=(int )(cpar[i+2]*r);
    break;
  case 'D':
    for (i=0;i<(2*(cpar[1]));i++) cpar[i+3]=(int )(cpar[i+3]*r);
    break;
  }
  if (code!='\0') GRAdraw(GC,code,cpar,cstr);
}

int GRAinput(int GC,char *s,int leftm,int topm,int rate)
{
  int pos,num,i;
  char code;
  int *cpar;
  char *cstr;

  code='\0';
  cpar=NULL;
  cstr=NULL;
  for (i=0;s[i]!='\0';i++)
    if (strchr("\n\r",s[i])!=NULL) {
      s[i]='\0';
      break;
    }
  pos=0;
  while ((s[pos]==' ') || (s[pos]=='\t')) pos++;
  if (s[pos]=='\0') return TRUE;
  if (strchr("IE%VAGOMNLTCBPRDFHSK",s[pos])==NULL) return FALSE;
  code=s[pos];
  if (strchr("%FSK",code)==NULL) {
    if (!getintpar(s+pos+1,1,&num)) return FALSE;
    num++;
    if ((cpar=memalloc(sizeof(int)*num))==NULL) return FALSE;
    if (!getintpar(s+pos+1,num,cpar)) goto errexit;
  } else {
    if ((cpar=memalloc(sizeof(int)))==NULL) return FALSE;
    cpar[0]=-1;
    if ((cstr=memalloc(strlen(s)-pos))==NULL) goto errexit;
    strcpy(cstr,s+pos+1);
  }
  GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
  memfree(cpar);
  memfree(cstr);
  return TRUE;

errexit:
  memfree(cpar);
  memfree(cstr);
  return FALSE;
}

char *fonttbl[21]={
   "Times","TimesBold","TimesItalic","TimesBoldItalic",
   "Helvetica","HelveticaBold","HelveticaOblique","HelveticaBoldOblique",
   "Mincho","Mincho","Mincho","Mincho",
   "Gothic","Gothic","Gothic","Gothic",
   "Courier","CourierBold","CourierItalic","CourierBoldItalic",
   "Symbol"};

struct greektbltype greektable[48]={
 {0x21,0101}, {0x22,0102}, {0x23,0107}, {0x24,0104}, {0x25,0105}, {0x26,0132},
 {0x27,0110}, {0x28,0121}, {0x29,0111}, {0x2A,0113}, {0x2B,0114}, {0x2C,0115},
 {0x2D,0116}, {0x2E,0130}, {0x2F,0117},
 {0x30,0120}, {0x31,0122}, {0x32,0123}, {0x33,0124}, {0x34,0241}, {0x35,0106},
 {0x36,0103}, {0x37,0131}, {0x38,0127},
 {0x41,0141}, {0x42,0142}, {0x43,0147}, {0x44,0144}, {0x45,0145}, {0x46,0172},
 {0x47,0150}, {0x48,0161}, {0x49,0151}, {0x4A,0153}, {0x4B,0154}, {0x4C,0155},
 {0x4D,0156}, {0x4E,0170}, {0x4F,0157}, {0x50,0160}, {0x51,0162}, {0x52,0163},
 {0x53,0164}, {0x54,0165}, {0x55,0146}, {0x56,0143}, {0x57,0171}, {0x58,0167}};

int GRAinputold(int GC,char *s,int leftm,int topm,int rate,int greek)
{
  int pos,num,i,j,k,h,m;
  char code,code2;
  int cpar[50],cpar2[50];
  char cstr[256],cstr2[256],*po;
  int col,B,R,G;
  unsigned int jiscode;
  int greekin,len;

  code='\0';
  for (i=0;s[i]!='\0';i++)
    if (strchr("\n\r",s[i])!=NULL) {
      s[i]='\0';
      break;
    }
  pos=0;
  while ((s[pos]==' ') || (s[pos]=='\t')) pos++;
  if (s[pos]=='\0') return TRUE;
  if (strchr("IEX%VAOMNLTCBPDFSK",s[pos])==NULL) return FALSE;
  code=s[pos];
  if (strchr("%SK",code)==NULL) {
    if (!getintpar(s+pos+1,1,&num)) return FALSE;
    num++;
    if (!getintpar(s+pos+1,num,cpar)) return FALSE;
  } else {
    cpar[0]=-1;
    if ((po=strchr(s+pos+1,','))==NULL) return FALSE;
    if ((po=strchr(po+1,','))==NULL) return FALSE;
    strcpy(cstr,po+1);
    j=0;
    for (i=0;cstr[i]!='\0';i++) {
      if (cstr[i]=='\\') {
        if ((cstr[i+1]!='n') && (cstr[i+1]!='r')) {
          cstr[j]=cstr[i+1];
          j++;
        }
        i++;
      } else {
        cstr[j]=cstr[i];
        j++;
      }
    }
    if ((j>0) && (cstr[j-1]==',')) j--;
    cstr[j]='\0';
  }
  switch (code) {
  case 'X':
    break;
  case '%': case 'S':
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    break;
  case 'K':
    greekin=FALSE;
    j=0;
    len=strlen(cstr);
    for (i=0;TRUE;i+=2) {
      if (cstr[i]!='\0')
        jiscode=njms2jis(((unsigned char)cstr[i] << 8)
                         +(unsigned char)cstr[i+1]);
      if (!greekin) {
        if ((i>=len) || (((jiscode>>8)==0x26) && greek)) {
          if (i!=j) {
            code2='K';
            cpar2[0]=-1;
            strncpy(cstr2,cstr+j,i-j);
            cstr2[i-j]='\0';
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr2);
            j=i;
          }
          greekin=TRUE;
        }
      } else {
        if ((i>=len) || ((jiscode>>8)!=0x26)) {
          if (i!=j) {
            code2='F';
            cpar2[0]=-1;
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,fonttbl[20]);
            code2='H';
            cpar2[0]=3;
            cpar2[1]=GRAClist[GC].mergept;
            cpar2[2]=GRAClist[GC].mergesp;
            cpar2[3]=GRAClist[GC].mergedir;
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr2);
            code2='S';
            cpar2[0]=-1;
            m=0;
            for (k=j;k<i;k+=2) {
              jiscode=njms2jis(((unsigned char)cstr[k] << 8)
                               +(unsigned char)cstr[k+1]);
              for (h=0;h<48;h++) if (greektable[h].jis==(jiscode & 0xff))
                break;
              if (h!=48) cstr2[m++]=greektable[h].symbol;
            }
            cstr2[m]='\0';
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr2);
            j=i;
            code2='F';
            cpar2[0]=-1;
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,
                         fonttbl[GRAClist[GC].mergefont]);
            code2='H';
            cpar2[0]=3;
            cpar2[1]=GRAClist[GC].mergept;
            cpar2[2]=GRAClist[GC].mergesp;
            cpar2[3]=GRAClist[GC].mergedir;
            GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr2);
          }
          greekin=FALSE;
        }
      }
      if (i>=len) break;
    }
    break;
  case 'I':
    cpar[0]=5;
    cpar[5]=nround(cpar[3]/2.1);
    cpar[3]=21000;
    cpar[4]=29700;
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    break;
  case 'E': case 'M': case 'N': case 'L': case 'T': case 'P':
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    break;
  case 'V':
    if (cpar[0]==4) {
      cpar[0]=5;
      cpar[5]=0;
    }
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    break;
  case 'A':
    col=cpar[3];
    GRAClist[GC].oldFB=(col & 1)*256;
    GRAClist[GC].oldFG=(col & 2)*128;
    GRAClist[GC].oldFR=(col & 4)*64;
    code2='G';
    cpar2[0]=3;
    cpar2[1]=GRAClist[GC].oldFR;
    cpar2[2]=GRAClist[GC].oldFG;
    cpar2[3]=GRAClist[GC].oldFB;
    GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    cpar[0]=5;
    cpar[3]=cpar[4];
    cpar[4]=0;
    cpar[5]=1000;
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    break;
  case 'O':
    col=cpar[1];
    GRAClist[GC].oldBB=(col & 1)*256;
    GRAClist[GC].oldBG=(col & 2)*128;
    GRAClist[GC].oldBR=(col & 4)*64;
    break;
  case 'C':
    cpar2[7]=cpar[4];
    if (cpar[0]>=5) cpar[4]=cpar[5];
    else cpar[4]=cpar[3];
    if (cpar[0]==7) {
	  cpar[5]=cpar[6]*10;
	  cpar[6]=cpar[7]*10-cpar[6]*10;
      if (cpar[6]<0) cpar[6]+=36000;
    } else {
      cpar[5]=0;
	  cpar[6]=36000;
    }
    cpar[7]=cpar2[7];
    cpar[0]=7;
    if (cpar[7]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
	}
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    if (cpar[7]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    }
    break;
  case 'B':
    if (cpar[5]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    }
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    if (cpar[5]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    }
    break;
  case 'D':
    if (cpar[2]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    }
    GRAinputdraw(GC,leftm,topm,rate,code,cpar,cstr);
    if (cpar[2]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    }
    break;
  case 'F':
    code2='F';
    cpar2[0]=-1;
    GRAClist[GC].mergefont=cpar[1]*4+cpar[2];
    GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,fonttbl[cpar[1]*4+cpar[2]]);
    if (cpar[6]==1) cpar[6]=9000;
    else if (cpar[6]<0) cpar[6]=abs(cpar[6]);
    code2='H';
    cpar2[0]=3;
    cpar2[1]=cpar[3]*100;
    cpar2[2]=cpar[4]*100;
    cpar2[3]=cpar[6];
    GRAClist[GC].mergept=cpar2[1];
    GRAClist[GC].mergesp=cpar2[2];
    GRAClist[GC].mergedir=cpar2[3];
    GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    col=cpar[5];
    B=(col & 1)*256;
    G=(col & 2)*128;
    R=(col & 4)*64;
    code2='G';
    cpar2[0]=3;
    cpar2[1]=R;
    cpar2[2]=G;
    cpar2[3]=B;
    GRAinputdraw(GC,leftm,topm,rate,code2,cpar2,cstr);
    break;
  }
  return TRUE;
}

int GRAlineclip(int GC,int *x0,int *y0,int *x1,int *y1)
{
  int a,xl,yl,xg,yg;
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if (*x0<*x1) {
    xl=*x0;  yl=*y0; xg=*x1;  yg=*y1;
  } else {
    xl=*x1;  yl=*y1; xg=*x0;  yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if (xg>maxx) {
    xg=maxx; yg=(*y1-*y0)*(maxx-*x0)/(*x1-*x0)+*y0;
  }
  if (xl<minx) {
    xl=minx; yl=(*y1-*y0)*(minx-*x0)/(*x1-*x0)+*y0;
  }
  if (yl>yg) {
    a=yl;  yl=yg;  yg=a;  a=xl;  xl=xg;  xg=a;
  }
  if ((yg<miny) || (yl>maxy)) return 1;
  if (yg>maxy) {
    yg=maxy; xg=(*x1-*x0)*(maxy-*y0)/(*y1-*y0)+*x0;
  }
  if (yl<miny) {
    yl=miny; xl=(*x1-*x0)*(miny-*y0)/(*y1-*y0)+*x0;
  }
  if ((*y0<*y1) || ((*y0==*y1) && (*x0<*x1))) {
    *x0=xl; *y0=yl;   *x1=xg; *y1=yg;
  } else {
    *x0=xg; *y0=yg;   *x1=xl; *y1=yl;
  }
  return 0;
}

int GRArectclip(int GC,int *x0,int *y0,int *x1,int *y1)
{
  int xl,yl,xg,yg;
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if (*x0<*x1) {
    xl=*x0; xg=*x1;
  } else {
    xl=*x1; xg=*x0;
  }
  if (*y0<*y1) {
    yl=*y0; yg=*y1;
  } else {
    yl=*y1; yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if ((yg<miny) || (yl>maxy)) return 1;
  if ((xg>maxx) && (xl<minx) && (yg>maxy) && (yl<miny)) return 1;
  if (xg>maxx) xg=maxx;
  if (xl<minx) xl=minx;
  if (yg>maxy) yg=maxy;
  if (yl<miny) yl=miny;
  *x0=xl;  *y0=yl;
  *x1=xg;  *y1=yg;
  return 0;
}

int GRAinview(int GC,int x,int y)
{
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if ((minx<=x) && (x<=maxx) && (miny<=y) && (y<=maxy)) return 0;
  else return 1;
}

void GRAcurvefirst(int GC,int num,int *dashlist,
      clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
                   double x0,double y0)
{
  int i,gx0,gy0;

  memfree(GRAClist[GC].gdashlist);
  GRAClist[GC].gdashlist=NULL;
  if (num!=0) {
    if ((GRAClist[GC].gdashlist=memalloc(sizeof(int)*num))==NULL) num=0;
  }
  GRAClist[GC].gdashn=num;
  for (i=0;i<num;i++) 
    (GRAClist[GC].gdashlist)[i]=dashlist[i];
  GRAClist[GC].gclipf=clipf;
  GRAClist[GC].gtransf=transf;
  GRAClist[GC].gdiff=diff;
  GRAClist[GC].gintpf=intpf;
  GRAClist[GC].gflocal=local;
  GRAClist[GC].gdashlen=0;
  GRAClist[GC].gdashi=0;
  GRAClist[GC].gdotf=TRUE;
  GRAClist[GC].x0=x0;
  GRAClist[GC].y0=y0;
  if (GRAClist[GC].gtransf==NULL) {
    gx0=nround(x0);
    gy0=nround(y0);
  } else GRAClist[GC].gtransf(x0,y0,&gx0,&gy0,local);
  GRAmoveto(GC,gx0,gy0);
}

int GRAcurve(int GC,double c[],double x0,double y0)
{
  double d,dx,dy,ddx,ddy,dd,x,y;

  if (ninterrupt()) return FALSE;
  d=0;
  while (d<1) {
    GRAClist[GC].gdiff(d,c,&dx,&dy,&ddx,&ddy,GRAClist[GC].gflocal);
    if ((fabs(dx)+fabs(ddx)/3)<=1e-100) dx=1;
    else dx=sqrt(fabs(2/(fabs(dx)+fabs(ddx)/3)));
    if ((fabs(dy)+fabs(ddy)/3)<=1e-100) dy=1;
    else dy=sqrt(fabs(2/(fabs(dy)+fabs(ddy)/3)));
    dd=(dx<dy) ? dx : dy;
    d+=dd;
    if (d>1) d=1;
    GRAClist[GC].gintpf(d,c,x0,y0,&x,&y,GRAClist[GC].gflocal);
    GRAdashlinetod(GC,x,y);
  }
  return TRUE;
}

void GRAdashlinetod(int GC,double x,double y)
{
  double dx,dy,dd,len,len2,x1,y1,x2,y2;
  int gx,gy,gx1,gy1,gx2,gy2;

  x1=GRAClist[GC].x0;
  y1=GRAClist[GC].y0;
  x2=x;
  y2=y;
  if ((GRAClist[GC].gclipf==NULL)
   || (GRAClist[GC].gclipf(&x1,&y1,&x2,&y2,GRAClist[GC].gflocal)==0)) {
    if (GRAClist[GC].gtransf==NULL) {
      gx1=nround(x1);
      gy1=nround(y1);
      gx2=nround(x2);
      gy2=nround(y2);
    } else {
      GRAClist[GC].gtransf(x1,y1,&gx1,&gy1,GRAClist[GC].gflocal);
      GRAClist[GC].gtransf(x2,y2,&gx2,&gy2,GRAClist[GC].gflocal);
    }
    if ((x1!=GRAClist[GC].x0) || (y1!=GRAClist[GC].y0))
      GRAmoveto(GC,gx1,gy1);
    if (GRAClist[GC].gdashn==0) GRAlineto(GC,gx2,gy2);
    else {
      dx=(gx2-gx1);
      dy=(gy2-gy1);
      len2=len=sqrt(dx*dx+dy*dy);
      while (len2
      >((GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]-GRAClist[GC].gdashlen)) {
        dd=(len-len2+(GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]
           -GRAClist[GC].gdashlen)/len;
        gx=gx1+nround(dx*dd);
        gy=gy1+nround(dy*dd);
        if (GRAClist[GC].gdotf) GRAlineto(GC,gx,gy);
        else GRAmoveto(GC,gx,gy);
        GRAClist[GC].gdotf=GRAClist[GC].gdotf ? FALSE : TRUE;
        len2-=((GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]
             -GRAClist[GC].gdashlen);
        GRAClist[GC].gdashlen=0;
        GRAClist[GC].gdashi++;
        if (GRAClist[GC].gdashi>=GRAClist[GC].gdashn) {
          GRAClist[GC].gdashi=0;
          GRAClist[GC].gdotf=TRUE;
        }
      }
      if (GRAClist[GC].gdotf) GRAlineto(GC,gx2,gy2);
      GRAClist[GC].gdashlen+=len2;
    }
  }
  GRAClist[GC].x0=x;
  GRAClist[GC].y0=y;
}

void GRAcmatchfirst(int pointx,int pointy,int err,
                    clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
                    struct cmatchtype *data,int bbox,double x0,double y0)
{
  data->x0=x0;
  data->y0=y0;
  data->gclipf=clipf;
  data->gtransf=transf;
  data->gdiff=diff;
  data->gintpf=intpf;
  data->gflocal=local;
  data->err=err;
  data->pointx=pointx;
  data->pointy=pointy;
  data->bbox=bbox;
  data->minx=0;
  data->miny=0;
  data->maxx=0;
  data->maxy=0;
  data->bboxset=FALSE;
  data->match=FALSE;
}

void GRAcmatchtod(double x,double y,struct cmatchtype *data)
{
  double x1,y1,x2,y2;
  int gx1,gy1,gx2,gy2;
  double r,r2,r3,ip;

  x1=data->x0;
  y1=data->y0;
  x2=x;
  y2=y;
  if (data->bbox) {
    if (data->gtransf==NULL) {
      gx1=nround(x1);
      gy1=nround(y1);
      gx2=nround(x2);
      gy2=nround(y2);
    } else {
      data->gtransf(x1,y1,&gx1,&gy1,data->gflocal);
      data->gtransf(x2,y2,&gx2,&gy2,data->gflocal);
    }
    if (!data->bboxset || gx1<data->minx) data->minx=gx1;
    if (!data->bboxset || gy1<data->miny) data->miny=gy1;
    if (!data->bboxset || gx1>data->maxx) data->maxx=gx1;
    if (!data->bboxset || gy1>data->maxy) data->maxy=gy1;
    data->bboxset=TRUE;
    if (!data->bboxset || gx2<data->minx) data->minx=gx2;
    if (!data->bboxset || gy2<data->miny) data->miny=gy2;
    if (!data->bboxset || gx2>data->maxx) data->maxx=gx2;
    if (!data->bboxset || gy2>data->maxy) data->maxy=gy2;
  } else {
    if ((data->gclipf==NULL)
     || (data->gclipf(&x1,&y1,&x2,&y2,data->gflocal)==0)) {
      if (data->gtransf==NULL) {
        gx1=nround(x1);
        gy1=nround(y1);
        gx2=nround(x2);
        gy2=nround(y2);
      } else {
        data->gtransf(x1,y1,&gx1,&gy1,data->gflocal);
        data->gtransf(x2,y2,&gx2,&gy2,data->gflocal);
      }
      x1=gx1;
      y1=gy1;
      x2=gx2;
      y2=gy2;
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((data->pointx-x1)*(data->pointx-x1)+(data->pointy-y1)*(data->pointy-y1));
      r3=sqrt((data->pointx-x2)*(data->pointx-x2)+(data->pointy-y2)*(data->pointy-y2));
      if ((r<=data->err) || (r3<data->err)) {
        data->match=TRUE;
      } else if (r2!=0) {
        ip=((x2-x1)*(data->pointx-x1)+(y2-y1)*(data->pointy-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((data->pointx-x2)*(data->pointx-x2)+(data->pointy-y2)*(data->pointy-y2));
          if (r<data->err) data->match=TRUE;
        }
      }
    }
  }
  data->x0=x;
  data->y0=y;
}

int GRAcmatch(double c[],double x0,double y0,struct cmatchtype *data)
{
  double d,dx,dy,ddx,ddy,dd,x,y;

  if (ninterrupt()) return FALSE;
  d=0;
  while (d<1) {
    data->gdiff(d,c,&dx,&dy,&ddx,&ddy,data->gflocal);
    if ((fabs(dx)+fabs(ddx)/3)==0) dx=1;
    else dx=sqrt(fabs(2/(fabs(dx)+fabs(ddx)/3)));
    if ((fabs(dy)+fabs(ddy)/3)==0) dy=1;
    else dy=sqrt(fabs(2/(fabs(dy)+fabs(ddy)/3)));
    dd=(dx<dy) ? dx : dy;
    d+=dd;
    if (d>1) d=1;
    data->gintpf(d,c,x0,y0,&x,&y,data->gflocal);
    GRAcmatchtod(x,y,data);
  }
  return TRUE;
}

void setbbminmax(struct GRAbbox *bbox,int x1,int y1,int x2,int y2,int lw)
{
  int x,y;

  if (x1>x2) {
    x=x1; x1=x2; x2=x;
  }
  if (y1>y2) {
    y=y1; y1=y2; y2=y;
  }
  if (lw) {
    x1-=bbox->linew;
    y1-=bbox->linew;
    x2+=bbox->linew;
    y2+=bbox->linew;
  }
  if (!bbox->clip || !((x2<0) || (y2<0)
   || (x1>bbox->clipsizex) || (y1>bbox->clipsizey))) {
    if (bbox->clip) {
      if (x1<0) x1=0;
      if (y1<0) y1=0;
      if (x2>bbox->clipsizex) x2=bbox->clipsizex;
      if (y2>bbox->clipsizey) y1=bbox->clipsizey;
    }
    x1+=bbox->offsetx;
    x2+=bbox->offsetx;
    y1+=bbox->offsety;
    y2+=bbox->offsety;
    if (!bbox->set || (x1<bbox->minx)) bbox->minx=x1;
    if (!bbox->set || (y1<bbox->miny)) bbox->miny=y1;
    if (!bbox->set || (x2>bbox->maxx)) bbox->maxx=x2;
    if (!bbox->set || (y2>bbox->maxy)) bbox->maxy=y2;
    if (!bbox->set) bbox->set=TRUE;
  }
}

void GRAinitbbox(struct GRAbbox *bbox)
{
  bbox->set=FALSE;
  bbox->minx=0;
  bbox->miny=0;
  bbox->maxx=0;
  bbox->maxy=0;
  bbox->offsetx=0;
  bbox->offsety=0;
  bbox->posx=0;
  bbox->posy=0;
  bbox->pt=0;
  bbox->spc=0;
  bbox->dir=0;
  bbox->linew=0;
  bbox->clip=TRUE;
  bbox->clipsizex=21000;
  bbox->clipsizey=29700;
  bbox->fontalias=NULL;
  bbox->loadfont=FALSE;
}

void GRAendbbox(struct GRAbbox *bbox)
{
  memfree(bbox->fontalias);
}

int GRAboundingbox(char code,int *cpar,char *cstr,void *local)
{
  int i,lw;
  double x,y,csin,ccos;
  int w,h,d,x1,y1,x2,y2,x3,y3,x4,y4;
  char ch;
  int c1,c2;
  struct GRAbbox *bbox;

  bbox=local;
  switch (code) {
  case 'I': case 'X': case 'E': case '%': case 'G':
    break;
  case 'V':
    bbox->offsetx=cpar[1];
    bbox->offsety=cpar[2];
    bbox->posx=0;
    bbox->posy=0;
    if (cpar[5]==1) bbox->clip=TRUE;
    else bbox->clip=FALSE;
    bbox->clipsizex=cpar[3]-cpar[1];
    bbox->clipsizey=cpar[4]-cpar[2];
    break;
  case 'A':
    bbox->linew=cpar[2]/2;
    break;
  case 'M':
    bbox->posx=cpar[1];
    bbox->posy=cpar[2];
    break;
  case 'N':
    bbox->posx+=cpar[1];
    bbox->posy+=cpar[2];
    break;
  case 'L':
    setbbminmax(bbox,cpar[1],cpar[2],cpar[3],cpar[4],TRUE);
    break;
  case 'T':
    setbbminmax(bbox,bbox->posx,bbox->posy,cpar[1],cpar[2],TRUE);
    bbox->posx=cpar[1];
    bbox->posy=cpar[2];
    break;
  case 'C':
    if (cpar[7]==0) lw=TRUE;
    else lw=FALSE;
    if (cpar[7]==1) setbbminmax(bbox,cpar[1],cpar[2],cpar[1],cpar[2],lw);
    setbbminmax(bbox,cpar[1]+(int )(cpar[3]*cos(cpar[5]/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin(cpar[5]/18000.0*MPI)),
                cpar[1]+(int )(cpar[3]*cos((cpar[5]+cpar[6])/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin((cpar[5]+cpar[6])/18000.0*MPI)),lw);
    cpar[6]+=cpar[5];
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1],cpar[2]-cpar[4],cpar[1],cpar[2]-cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1]-cpar[3],cpar[2],cpar[1]-cpar[3],cpar[2],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1],cpar[2]+cpar[4],cpar[1],cpar[2]+cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1]+cpar[3],cpar[2],cpar[1]+cpar[3],cpar[2],lw);
    break;
  case 'B':
    if (cpar[5]==1) lw=FALSE;
    else lw=TRUE;
    setbbminmax(bbox,cpar[1],cpar[2],cpar[3],cpar[4],lw);
    break;
  case 'P':
    setbbminmax(bbox,cpar[1],cpar[2],cpar[1],cpar[2],FALSE);
    break;
  case 'R':
    for (i=0;i<(cpar[1]-1);i++)
      setbbminmax(bbox,cpar[i*2+2],cpar[i*2+3],cpar[i*2+4],cpar[i*2+5],lw);
    break;
  case 'D':
    if (cpar[2]==0) lw=TRUE;
    else lw=FALSE;
    for (i=0;i<(cpar[1]-1);i++)
      setbbminmax(bbox,cpar[i*2+3],cpar[i*2+4],cpar[i*2+5],cpar[i*2+6],lw);
    break;
  case 'F':
    memfree(bbox->fontalias);
    if ((bbox->fontalias=memalloc(strlen(cstr)+1))!=NULL)
      strcpy(bbox->fontalias,cstr);
    break;
  case 'H':
    bbox->pt=cpar[1];
    bbox->spc=cpar[2];
    bbox->dir=cpar[3];
    bbox->loadfont=TRUE;
    break;
  case 'S':
    if (!bbox->loadfont) break;
    csin=sin(bbox->dir/18000.0*MPI);
    ccos=cos(bbox->dir/18000.0*MPI);
    i=0;
    while (i<strlen(cstr)) {
      if ((cstr[i]=='\\') && (cstr[i+1]=='x')) {
        if (toupper(cstr[i+2])>='A') c1=toupper(cstr[i+2])-'A'+10;
        else c1=cstr[i+2]-'0';
        if (toupper(cstr[i+3])>='A') c2=toupper(cstr[i+3])-'A'+10;
        else c2=cstr[i+3]-'0';
        ch=c1*16+c2;
        i+=4;
      } else {
        ch=cstr[i];
        i++;
      }
      w=GRAcharwidth((unsigned int)ch,bbox->fontalias,bbox->pt);
      h=GRAcharascent(bbox->fontalias,bbox->pt);
      d=GRAchardescent(bbox->fontalias,bbox->pt);
      x=0;
      y=d;
      x1=(int )(bbox->posx+(x*ccos+y*csin));
      y1=(int )(bbox->posy+(-x*csin+y*ccos));
      x=0;
      y=-h;
      x2=(int )(bbox->posx+(x*ccos+y*csin));
      y2=(int )(bbox->posy+(-x*csin+y*ccos));
      x=w;
      y=d;
      x3=(int )(bbox->posx+(x*ccos+y*csin));
      y3=(int )(bbox->posy+(-x*csin+y*ccos));
      x=w;
      y=-h;
      x4=(int )(bbox->posx+(int )(x*ccos+y*csin));
      y4=(int )(bbox->posy+(int )(-x*csin+y*ccos));
      setbbminmax(bbox,x1,y1,x4,y4,FALSE);
      setbbminmax(bbox,x2,y2,x3,y3,FALSE);
      bbox->posx+=(int )((w+bbox->spc*25.4/72)*ccos);
      bbox->posy-=(int )((w+bbox->spc*25.4/72)*csin);
    }
    break;
  case 'K':
    if (!bbox->loadfont) break;
    csin=sin(bbox->dir/18000.0*MPI);
    ccos=cos(bbox->dir/18000.0*MPI);
    i=0;
    while (i<strlen(cstr)) {
      if (niskanji((unsigned char)cstr[i]) && (cstr[i+1]!='\0')) {
        i+=2;
        w=GRAcharwidth((((unsigned char)cstr[i+1])<<8)+(unsigned char)cstr[i],
                       bbox->fontalias,bbox->pt);
        h=GRAcharascent(bbox->fontalias,bbox->pt);
        d=GRAchardescent(bbox->fontalias,bbox->pt);
        x=0;
        y=d;
        x1=(int )(bbox->posx+(x*ccos+y*csin));
        y1=(int )(bbox->posy+(-x*csin+y*ccos));
        x=0;
        y=-h;
        x2=(int )(bbox->posx+(x*ccos+y*csin));
        y2=(int )(bbox->posy+(-x*csin+y*ccos));
        x=w;
        y=d;
        x3=(int )(bbox->posx+(x*ccos+y*csin));
        y3=(int )(bbox->posy+(-x*csin+y*ccos));
        x=w;
        y=-h;
        x4=(int )(bbox->posx+(int )(x*ccos+y*csin));
        y4=(int )(bbox->posy+(int )(-x*csin+y*ccos));
        setbbminmax(bbox,x1,y1,x4,y4,FALSE);
        setbbminmax(bbox,x2,y2,x3,y3,FALSE);
        bbox->posx+=(int )((w+bbox->spc*25.4/72)*ccos);
        bbox->posy-=(int )((w+bbox->spc*25.4/72)*csin);
      } else i++;
    }
    break;
  }
  return 0;
}

