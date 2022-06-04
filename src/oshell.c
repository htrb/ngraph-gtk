/*
 * $Id: oshell.c,v 1.9 2010-03-04 08:30:16 hito Exp $
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
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include "object.h"
#include "ioutil.h"
#include "shell.h"

#define NAME "shell"
#define PARENT "object"
#define OVERSION   "1.00.00"
#define MAXCLINE 256

#define ERRRUN 100
#define ERRNOCL 101
#define ERRFILEFIND 102

static char *sherrorlist[]={
  "already running.",
  "no command string is specified.",
  "no such file",
};

#define ERRNUM (sizeof(sherrorlist) / sizeof(*sherrorlist))

struct shlocal {
  int lock;
  struct nshell *nshell;
};

static int
cmdinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct shlocal *shlocal;
  struct nshell *nshell;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if ((shlocal=g_malloc(sizeof(struct shlocal)))==NULL) return 1;
  if (_putobj(obj,"_local",inst,shlocal)) {
    g_free(shlocal);
    return 1;
  }
  if ((nshell=newshell())==NULL) return 1;
  ngraphenvironment(nshell);
  shlocal->lock=0;
  shlocal->nshell=nshell;
  return 0;
}

static int
cmddone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct shlocal *shlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  _getobj(obj,"_local",inst,&shlocal);
  if (shlocal->lock) {
    shlocal->nshell->deleted = 1;
  } else {
    delshell(shlocal->nshell);
  }
  return 0;
}

#if ! WINDOWS
static void
int_handler(int sig)
{
  set_interrupt();
}
#endif

static int
cmdshell(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct shlocal *shlocal;
  struct nshell *nshell;
  struct narray *sarray;
  struct objlist *sys;
  char **sdata;
  int i,snum;
  int err;
  char *filename,*filename2;
  int fd;
  int rcode;
  char *s;
#if ! WINDOWS
  struct sigaction oldact;
  int (*save_interrupt)(void);
#endif

  _getobj(obj,"_local",inst,&shlocal);

  if (shlocal->lock) {
    error(obj,ERRRUN);
    return 1;
  }
  shlocal->lock=1;
  nshell=shlocal->nshell;

  shellsavestdio(nshell);
#if ! WINDOWS
  save_interrupt = ninterrupt;
  set_signal(SIGINT, 0, int_handler, &oldact);
  ninterrupt = check_interrupt;
#endif

  err=1;
  filename=NULL;

  sarray=(struct narray *)argv[2];
  snum=arraynum(sarray);
  sdata=arraydata(sarray);
  for (i=0;i<snum;i++) {
    s=sdata[i];
    if ((s!=NULL) && ((s[0]=='-') || (s[0]=='+'))) {
      if (s[1]=='-') {
        i++;
        break;
      }
      switch (s[1]) {
      default:
        if (setshelloption(nshell,s)==-1) goto errexit;
        break;
      }
    } else break;
  }
  if (i!=snum) {
    filename=sdata[i];
    i++;
  }

  if (set_shell_args(nshell, i, argv[1], snum, sdata)) {
    goto errexit;
  }

  fd=NOHANDLE;
  if (filename!=NULL) {
    filename2=nsearchpath(getval(nshell,"PATH"),filename,TRUE);
    if (filename2!=NULL) {
      if ((fd=nopen(filename2,O_RDONLY,NFMODE))==NOHANDLE) {
        g_free(filename2);
        error2(obj,ERRFILEFIND,filename);
        goto errexit;
      }
      g_free(filename2);
      setshhandle(nshell,fd);
    } else {
      error2(obj,ERRFILEFIND,filename);
      goto errexit;
    }
  } else setshhandle(nshell,stdinfd());
  do {
    rcode=cmdexecute(nshell,NULL);

    if (nshell->deleted)
      break;

  } while (nisatty(getshhandle(nshell)) && (rcode!=0));

  if (fd!=NOHANDLE) {
    nclose(fd);
    setshhandle(nshell,stdinfd());
    if (!getshelloption(nshell,'s')) {
      do {
        rcode=cmdexecute(nshell,NULL);
      } while (nisatty(getshhandle(nshell)) && (rcode!=0));
    }
  }
  err=0;

errexit:
  sys = getobject("system");
  if (sys) {
    N_VALUE *sys_inst;
    sys_inst = chkobjinst(sys, 0);
    if (sys_inst) {
      _putobj(sys, "shell_status", sys_inst, &(nshell->status));
    }
  }
#if ! WINDOWS
  ninterrupt = save_interrupt;
  sigaction(SIGINT, &oldact, NULL);
#endif
  shellrestorestdio(nshell);

  if (nshell->deleted) {
    delshell(nshell);
  } else {
    shlocal->lock = 0;
  }

  return err;
}

static int
cmdload(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int security;

  security = get_security();
  set_security(TRUE);
  cmdload(obj, inst, rval, argc, argv);
  set_security(security);
}

static int
cmdsecurity(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  set_security(*(int *)argv[2]);
  return 0;
}

static int
cmd_set_security(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  set_security(TRUE);
  return 0;
}

static struct objtable shell[] = {
  {"init",NVFUNC,NEXEC,cmdinit,NULL,0},
  {"done",NVFUNC,NEXEC,cmddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"shell",NVFUNC,NREAD|NEXEC,cmdshell,"sa",0},
  {"security",NVFUNC,0,cmdsecurity,"b",0},
  {"set_security",NVFUNC,NREAD|NEXEC,cmd_set_security,"",0},
  {"_local",NPOINTER,0,NULL,NULL,0}};

#define TBLNUM (sizeof(shell) / sizeof(*shell))

void *
addshell(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,shell,ERRNUM,sherrorlist,NULL,NULL);
}
