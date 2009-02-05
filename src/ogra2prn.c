/* 
 * $Id: ogra2prn.c,v 1.2 2009/02/05 08:13:08 hito Exp $
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
#include <limits.h>
#include <stdio.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <windows.h>
#endif
#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"

#define NAME "gra2prn"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define TRUE  1
#define FALSE 0

#define ERRFOPEN 100

#define ERRNUM 1

static char *gra2perrorlist[ERRNUM]={
  "I/O error: open file"
};

struct gra2plocal {
  char *fname;
  FILE *fil;
};

static int 
gra2pinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  struct gra2plocal *gra2plocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if ((gra2plocal=memalloc(sizeof(struct gra2plocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,gra2plocal)) goto errexit;
  gra2plocal->fname=NULL;
  gra2plocal->fil=NULL;
  return 0;

errexit:
  memfree(gra2plocal);
  return 1;
}

static int 
gra2pdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct gra2plocal *gra2plocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&gra2plocal);
  if (gra2plocal->fname!=NULL) free(gra2plocal->fname);
  if (gra2plocal->fil!=NULL) fclose(gra2plocal->fil);
  return 0;
}

static int 
gra2p_output(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  struct gra2plocal *gra2plocal;
  struct objlist *sys;
  char code;
  int *cpar;
  int i;
  char *cstr;
  char *graf,*sname,*sver;
  char *pfx;
  char *s;
  char *driver,*option,*prn;
  struct nshell *nshell;

  gra2plocal=(struct gra2plocal *)argv[2];
  code=*(char *)(argv[3]);
  cpar=(int *)argv[4];
  cstr=argv[5];

  if (code=='I') {
    if (gra2plocal->fil!=NULL) fclose(gra2plocal->fil);
    gra2plocal->fil=NULL;
    if ((sys=getobject("system"))==NULL) return 1;
    if (getobj(sys,"temp_prefix",0,0,NULL,&pfx)) return 1;
    if (gra2plocal->fname!=NULL) free(gra2plocal->fname);
    if ((gra2plocal->fname=tempnam(NULL,pfx))==NULL) return 1;
    changefilename(gra2plocal->fname);
    if ((gra2plocal->fil=nfopen(gra2plocal->fname,"wt"))==NULL) {
      error2(obj,ERRFOPEN,gra2plocal->fname);
      free(gra2plocal->fname);
      gra2plocal->fname=NULL;
      return 1;
    }
    if (getobj(sys,"name",0,0,NULL,&sname)) return 1;
    if (getobj(sys,"version",0,0,NULL,&sver)) return 1;
    if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
    fprintf(gra2plocal->fil,"%s\n",graf);
    fprintf(gra2plocal->fil,"%%Creator: %s ver %s\n",sname,sver);
  }
  if (gra2plocal->fil!=NULL) {
      fputc(code,gra2plocal->fil);
      if (cpar[0]==-1) {
        for (i=0;cstr[i]!='\0';i++)
          fputc(cstr[i],gra2plocal->fil);
      } else {
        fprintf(gra2plocal->fil,",%d",cpar[0]);
        for (i=1;i<=cpar[0];i++)
        fprintf(gra2plocal->fil,",%d",cpar[i]);
      }
      fputc('\n',gra2plocal->fil);
      if (code=='E') {
        fclose(gra2plocal->fil);
        gra2plocal->fil=NULL;
        _getobj(obj,"driver",inst,&driver);
        _getobj(obj,"option",inst,&option);
        _getobj(obj,"prn",inst,&prn);
        if ((s=nstrnew())==NULL) goto errexit;
        if ((s=nstrcat(s,driver))==NULL) goto errexit;
        if ((s=nstrccat(s,' '))==NULL) goto errexit;
        if ((s=nstrcat(s,option))==NULL) goto errexit;
        if ((s=nstrcat(s," '"))==NULL) goto errexit;
        if ((s=nstrcat(s,gra2plocal->fname))==NULL) goto errexit;
        if ((s=nstrcat(s,"' "))==NULL) goto errexit;
        if ((s=nstrcat(s,prn))==NULL) goto errexit;
        if ((nshell=newshell())==NULL) {
        memfree(s);
        goto errexit;
      }
      ngraphenvironment(nshell);
      cmdexecute(nshell,s);
      delshell(nshell);
      memfree(s);
      unlink(gra2plocal->fname);
      free(gra2plocal->fname);
      gra2plocal->fname=NULL;
    }
  }
  return 0;

errexit:
  if (gra2plocal->fname!=NULL) {
    unlink(gra2plocal->fname);
    free(gra2plocal->fname);
    gra2plocal->fname=NULL;
  }
  return 1;
}

#define TBLNUM 8

static struct objtable gra2p[TBLNUM] = {
  {"init",NVFUNC,NEXEC,gra2pinit,NULL,0},
  {"done",NVFUNC,NEXEC,gra2pdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"driver",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"option",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"prn",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},
  {"_output",NVFUNC,0,gra2p_output,NULL,0},
};

void *addgra2prn()
/* addgra2prn() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,gra2p,ERRNUM,gra2perrorlist,NULL,NULL);
}
