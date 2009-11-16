/* 
 * $Id: osystem.c,v 1.14 2009/11/16 09:13:04 hito Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <dir.h>
#include <dos.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "ioutil.h"
#include "ntime.h"

#define NAME     "system"
#define PARENT   "object"
#define SYSNAME  "Ngraph"
#define TEMPN    "NGP"
#define COPYRIGHT "Copyright (C) 2003, Satoshi ISHIZAKA."
#define EMAIL "ZXB01226@nifty.com"
#ifndef WINDOWS
#define WEB "http://homepage3.nifty.com/slokar/ngraph/ngraph-gtk.html"
#else
#define WEB "http:\/\/homepage3.nifty.com\/slokar\/ngraph\/ngraph-gtk.html"
#endif

#define ERRNODIR   100
#define ERRTMPFILE 101


void resizeconsole(int col,int row);
extern int consolecol,consolerow;

static char *syserrorlist[]={
  "no such directory"
  "can't create temporary file"
};

#define ERRNUM (sizeof(syserrorlist) / sizeof(*syserrorlist))

static int 
sysinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *wd;
  int expand;
  char *exdir;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  expand=TRUE;
  if (_putobj(obj,"expand_file",inst,&expand)) return 1;
  exdir = g_strdup("./");
  if (exdir == NULL) return 1;
  if (_putobj(obj,"expand_dir",inst,exdir)) return 1;
  if (_putobj(obj,"name",inst,SYSNAME)) return 1;
  if (_putobj(obj,"version",inst,VERSION)) return 1;
  if (_putobj(obj,"copyright",inst,COPYRIGHT)) return 1;
  if (_putobj(obj,"e-mail",inst,EMAIL)) return 1;
  if (_putobj(obj,"web",inst,WEB)) return 1;
  if (_putobj(obj,"GRAF",inst,"%Ngraph GRAF")) return 1;
  if (_putobj(obj,"temp_prefix",inst,TEMPN)) return 1;
  if ((wd=ngetcwd())==NULL) return 1;
  if (_putobj(obj,"cwd",inst,wd)) {
    g_free(wd);
    return 1;
  }
  return 0;
}

static int 
sysdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct objlist *objcur;
  int i, n;
  char *s;
  struct narray *array;
  struct objlist *objectcur,*objectdel;

#ifdef DEBUG
  struct plist *plcur;
#endif

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  objcur=chkobjroot();
  while (objcur!=NULL) {
    if (objcur!=obj) {
      recoverinstance(objcur);
      for (i=chkobjlastinst(objcur);i>=0;i--) delobj(objcur,i);
    }
    objcur=objcur->next;
  }
  _getobj(obj,"conf_dir",inst,&s);
  g_free(s);
  _getobj(obj,"lib_dir",inst,&s);
  g_free(s);
  _getobj(obj,"home_dir",inst,&s);
  g_free(s);
  _getobj(obj,"cwd",inst,&s);
  g_free(s);
  _getobj(obj,"login_shell",inst,&s);
  g_free(s);
  _getobj(obj,"time",inst,&s);
  g_free(s);
  _getobj(obj,"date",inst,&s);
  g_free(s);
  _getobj(obj,"expand_dir",inst,&s);
  g_free(s);
  _getobj(obj,"temp_file",inst,&s);
  g_free(s);
  _getobj(obj,"temp_list",inst,&array);
  n = arraynum(array);
  if (n > 0) {
    char **data;

    data = arraydata(array);
    for (i = 0; i < n; i++) {
      unlink(data[i]);
    }
  }
  arrayfree2(array);

  objectcur=chkobjroot();
  while (objectcur!=NULL) {
    objectdel=objectcur;
    objectcur=objectcur->next;

    if (objectdel->doneproc)
      objectdel->doneproc(objectdel,objectdel->local);

    if (objectdel->table_hash)
      nhash_free(objectdel->table_hash);

    g_free(objectdel);
  }
  g_free(inst);
  g_free(argv);
#ifdef DEBUG
  i=0;
  plcur=g_mallocroot;
  while (plcur!=NULL) {
    i++;
    printfconsole("g_malloc: +%p\n",plcur->val);
    plcur=plcur->next;
  }
  if (i!=0) {
    printfconsole("g_malloc remain: %d\n",i);
    sleep(30);
  }
#endif
  exit(0);
  return 0;
}

static int 
syscwd(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *wd;
  char *home;

  wd=argv[2];
  if (wd!=NULL) {
    if (chdir(wd)!=0) {
      error2(obj,ERRNODIR,wd);
      return 1;
    }
  } else {
    if ((home=getenv("HOME"))!=NULL) {
      if (chdir(home)!=0) {
        error2(obj,ERRNODIR,home);
        return 1;
      }
    }
  }
  if ((wd=ngetcwd())==NULL) return 1;
  g_free(argv[2]);
  argv[2]=wd;
  return 0;
}

static int 
systime(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  time_t t;
  int style;

  t=time(NULL);
  style=*(int *)(argv[2]);
  g_free(*(char **)rval);
  *(char **)rval=ntime(&t,style);
  return 0;
}

static int 
sysdate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  time_t t;
  int style;

  t=time(NULL);
  style=*(int *)(argv[2]);
  g_free(*(char **)rval);
  *(char **)rval=ndate(&t,style);
  return 0;
}

static int 
systemp(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *pfx, *tmpfil;
  struct narray *array;
  int fd;

  g_free(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"temp_prefix",inst,&pfx);
  _getobj(obj,"temp_list",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(char *)))==NULL) return 1;
    if (_putobj(obj,"temp_list",inst,array)) {
      arrayfree2(array);
      return 1;
    }
  }

  fd = n_mkstemp(NULL, pfx, &tmpfil);
  if (fd < 0) {
    error(obj, ERRTMPFILE);
    return 1;
  }
  close(fd);

  arrayadd2(array,&tmpfil);
  *(char **)rval=tmpfil;
  return 0;
}

static int 
sysunlink(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *tmpfil;
  struct narray *array;
  int i,num;
  char **data;

  _getobj(obj,"temp_list",inst,&array);
  if (array==NULL) return 0;
  num=arraynum(array);
  data=arraydata(array);
  if (num>0) {
    tmpfil=(char *)argv[2];
    if (tmpfil==NULL) tmpfil=data[num-1];
    for (i=num-1;i>=0;i--)
      if (strcmp(tmpfil,data[i])==0) break;
    if (i>=0) {
      unlink(data[i]);
      arrayndel2(array,i);
    }
  }
  if (arraynum(array)==0) {
    arrayfree2(array);
    _putobj(obj,"temp_list",inst,NULL);
  }
  _getobj(obj,"temp_file",inst,&tmpfil);
  g_free(tmpfil);
  _putobj(obj,"temp_file",inst,NULL);
  return 0;
}

static int 
syshideinstance(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  struct objlist *obj2;
  char **objdata;
  int j,anum;

  array=(struct narray *)argv[2];
  if (array!=NULL) {
    anum=arraynum(array);
    objdata=arraydata(array);
    for (j=0;j<anum;j++) {
      obj2=getobject(objdata[j]);
      if ((obj2!=obj) && (obj2!=NULL)) hideinstance(obj2);
    }
  }
  return 0;
}

static int 
sysrecoverinstance(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  struct objlist *obj2;
  char **objdata;
  int j,anum;

  array=(struct narray *)argv[2];
  if (array!=NULL) {
    anum=arraynum(array);
    objdata=arraydata(array);
    for (j=0;j<anum;j++) {
      obj2=getobject(objdata[j]);
      recoverinstance(obj2);
    }
  }
  return 0;
}

static int 
systemresize(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *iarray;
  int num;
  int *data;

  iarray=(struct narray *)argv[2];
  num=arraynum(iarray);
  if (num >= 2) {
    data=(int *)arraydata(iarray);
    resizeconsole(data[0], data[1]);
  }
  return 0;
}

static struct objtable nsystem[] = {
  {"init",NVFUNC,NEXEC,sysinit,NULL,0},
  {"done",NVFUNC,NEXEC,sysdone,NULL,0},
  {"name",NSTR,NREAD,NULL,NULL,0},
  {"version",NSTR,NREAD,NULL,NULL,0},
  {"copyright",NSTR,NREAD,NULL,NULL,0},
  {"e-mail",NSTR,NREAD,NULL,NULL,0},
  {"web",NSTR,NREAD,NULL,NULL,0},
  {"login_shell",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"cwd",NSTR,NREAD|NWRITE,syscwd,NULL,0},
  {"ignore_path",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"expand_file",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"expand_dir",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"GRAF",NSTR,NREAD,NULL,NULL,0},
  {"temp_prefix",NSTR,NREAD,NULL,NULL,0},
  {"conf_dir",NSTR,NREAD,NULL,NULL,0},
  {"lib_dir",NSTR,NREAD,NULL,NULL,0},
  {"home_dir",NSTR,NREAD,NULL,NULL,0},
  {"time",NSFUNC,NREAD|NEXEC,systime,"i",0},
  {"date",NSFUNC,NREAD|NEXEC,sysdate,"i",0},
  {"temp_file",NSFUNC,NREAD|NEXEC,systemp,NULL,0},
  {"temp_list",NSARRAY,NREAD,NULL,NULL,0},
  {"unlink_temp_file",NVFUNC,NREAD|NEXEC,sysunlink,NULL,0},
  {"hide_instance",NVFUNC,NREAD|NEXEC,syshideinstance,"sa",0},
  {"recover_instance",NVFUNC,NREAD|NEXEC,sysrecoverinstance,"sa",0},
  {"resize",NVFUNC,NREAD|NEXEC,systemresize,"ia",0},
};

#define TBLNUM (sizeof(nsystem) / sizeof(*nsystem))

void *
addsystem()
/* addsystem() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,VERSION,TBLNUM,nsystem,ERRNUM,syserrorlist,NULL,NULL);
}

