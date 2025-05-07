/*
 * $Id: nconfig.c,v 1.17 2010-03-04 08:30:16 hito Exp $
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"
#include "nconfig.h"
#include <unistd.h>

#define CONF "Ngraph.ini"
#define CONFBAK "Ngraph.ini~"
#define LOCK "Ngraph.lock"

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

char *
getscriptname(char *file)
{
  struct objlist *sys;
  char *homedir,*s;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  s=getfilename(homedir,CONFTOP,file);
  return s;
}

char *
searchscript(char *file)
{
  struct objlist *sys;
  char *libdir,*homedir,*s;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return NULL;
  if (!findfilename(homedir,CONFTOP,file)) {
    if (!findfilename(libdir,CONFTOP,file)) return NULL;
    if ((s=getfilename(libdir,CONFTOP,file))==NULL) return NULL;
  } else {
    if ((s=getfilename(homedir,CONFTOP,file))==NULL) return NULL;
  }
  return s;
}

static int
configlocked(char *dir)
{
  if (findfilename(dir,CONFSEP,LOCK)) return TRUE;
  else return FALSE;
}

static void
lockconfig(char *dir)
{
  char *file;
  FILE *fp;

  while (configlocked(dir)) {
    msleep(100);
  }
  if ((file=getfilename(dir,CONFSEP,LOCK))==NULL) return;
  if ((fp=nfopen(file,"wt"))==NULL) {
    g_free(file);
    return;
  }
  fputs("Ngraph.ini is locked",fp);
  fclose(fp);
  g_free(file);
}

static void
unlockconfig(char *dir)
{
  char *file;

  if (!findfilename(dir,CONFSEP,LOCK)) return;
  if ((file=getfilename(dir,CONFSEP,LOCK))==NULL) return;
  g_unlink(file);
  g_free(file);
}

FILE *
openconfig(char *section)
{
  struct objlist *sys;
  char *libdir,*s,*homedir,*homeconf,*libconf,*buf;
  FILE *fp;
  GStatBuf homestat,libstat;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return NULL;
  homeconf=libconf=NULL;
  if (findfilename(homedir,CONFSEP,CONF)) {
    if ((homeconf=getfilename(homedir,CONFSEP,CONF))!=NULL) {
      if (nstat(homeconf,&homestat)!=0) {
        g_free(homeconf);
        homeconf=NULL;
      }
    }
  }
  if (findfilename(libdir,CONFSEP,CONF)) {
    if ((libconf=getfilename(libdir,CONFSEP,CONF))!=NULL) {
      if (nstat(libconf,&libstat)!=0) {
        g_free(libconf);
        libconf=NULL;
      }
    }
  }
  if (homeconf) {
#if 0
    if (libconf==NULL) {
      s=homeconf;
    } else if (homestat.st_mtime>=libstat.st_mtime) {
      s=homeconf;
      g_free(libconf);
    } else {
      s=libconf;
      g_free(homeconf);
    }
#else
    s=homeconf;
    g_free(libconf);
#endif
  } else if (libconf) {
    s=libconf;
  } else {
    return NULL;
  }
  if ((fp=nfopen(s,"rt"))==NULL) {
    g_free(s);
    return NULL;
  }
  g_free(s);
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      g_free(buf);
      return fp;
    }
    g_free(buf);
  }
  fclose(fp);
  return NULL;
}

char *
getconfig(FILE *fp,char **val)
{
  char *s,*tok,*buf;
  int len;

  while (fgetline(fp, &buf) == 0) {
    switch (buf[0]) {
    case '[':
      g_free(buf);
      return NULL;
    case ';':
    case '#':
      g_free(buf);
      continue;
    }

    s = buf;
    tok = getitok2(&s, &len, "=");
    if (tok) {
      s += (s[0] == '=') ? 1 : 0;
      *val = g_strdup(s);
      if (*val == NULL) {
	g_free(tok);
	tok = NULL;
      }
      g_free(buf);
      return tok;
    }
    g_free(buf);
  }
  return NULL;
}

void
closeconfig(FILE *fp)
{
  fclose(fp);
}

static int
make_backup(char *homedir, char *libdir, char *fil, FILE *fptmp)
{
  FILE *fp;
  char *buf, *bak;

  if (! findfilename(homedir, CONFSEP, CONF)) {
    bak = getfilename(libdir, CONFSEP, CONFBAK);
    if (bak && findfilename(libdir, CONFSEP, CONFBAK)) {
      g_unlink(bak);
    }
  } else {
    bak = getfilename(homedir, CONFSEP, CONFBAK);
    if (bak && findfilename(homedir, CONFSEP, CONFBAK)) {
      g_unlink(bak);
    }
  }
  if (bak) {
    rename(fil,  bak);
    g_free(bak);
  }

  fp = nfopen(fil, "wt");
  if (fp == NULL) {
    return FALSE;
  }
  rewind(fptmp);

  while (fgetline(fptmp, &buf) == 0) {
    fputs(buf, fp);
    fputs("\n", fp);
    g_free(buf);
  }

  fclose(fp);
  return TRUE;
}

static char *
replaceconfig_match(FILE *fp, FILE *fptmp, struct narray *iconf, struct narray *conf)
{
  char *s, *s2, *buf, **data;
  const char *tok2;
  int len, len2, i, j, num, num2;

  num = arraynum(conf);
  data = arraydata(conf);

  while (fgetline(fp, &buf) == 0) {
    const char *tok;
    int out;
    if (buf[0]=='[') {
      return buf;
    }

    s = buf;
    out = FALSE;
    tok = getitok(&s, &len, " \t=,");
    if (tok) {
      for (i = 0; i < num; i++) {
	s2 = data[i];
	tok2 = getitok(&s2, &len2, " \t=,");
	if (tok2 && (len == len2 && strncmp(tok, tok2, len) == 0)) {
	  out = TRUE;
	  num2 = arraynum(iconf);
	  for (j = 0; j < num2; j++) {
	    if (i == arraynget_int(iconf, j))
	      break;
	  }
	  if (j == num2) {
	    fputs(data[i], fptmp);
	    fputs("\n", fptmp);
	    arrayadd(iconf, &i);
	  }
	}
      }
    }
    if (! out && buf && buf[0] != '\0') {
      fputs(buf, fptmp);
      fputs("\n", fptmp);
    }
    g_free(buf);
  }
  return NULL;
}

int
replaceconfig(char *section,struct narray *conf)
{
  int i,j,num,num2, r;
  char **data;
  struct objlist *sys;
  char *libdir,*homedir,*dir,*fil,*buf,*pfx, *tmp_name;
  FILE *fp,*fptmp;
  struct narray iconf;

  if (arraynum(conf) == 0) return TRUE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"temp_prefix",0,0,NULL,&pfx)) return FALSE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return FALSE;
  if (!findfilename(homedir,CONFSEP,CONF)) {
    if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;
    if ((fil=getfilename(libdir,CONFSEP,CONF))==NULL) return FALSE;
    dir=libdir;
  } else {
    if ((fil=getfilename(homedir,CONFSEP,CONF))==NULL) return FALSE;
    dir=homedir;
  }
  lockconfig(dir);
  fptmp = n_tmpfile(&tmp_name);
  if (fptmp == NULL) {
    g_free(fil);
    unlockconfig(dir);
    return FALSE;
  }
  fp = nfopen(fil,"rt");
  if (fp == NULL) {
    n_tmpfile_close(fptmp, tmp_name);
    g_free(fil);
    unlockconfig(dir);
    return FALSE;
  }
  arrayinit(&iconf,sizeof(int));
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
      g_free(buf);
      buf = replaceconfig_match(fp, fptmp, &iconf, conf);
      goto flush;
    } else {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
    }
    g_free(buf);
  }
/* section not found */
  fputs("\n",fptmp);
  fputs(section,fptmp);
  fputs("\n",fptmp);

flush:
  data = arraydata(conf);
  num = arraynum(conf);
  for (i=0;i<num;i++) {
    num2=arraynum(&iconf);
    for (j=0;j<num2;j++) if (i==arraynget_int(&iconf,j)) break;
    if (j==num2) {
      fputs(data[i],fptmp);
      fputs("\n",fptmp);
    }
  }
  fputs("\n",fptmp);
  if (buf!=NULL) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    g_free(buf);
  }
  while (fgetline(fp,&buf)==0) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    g_free(buf);
  }

  arraydel(&iconf);
  fclose(fp);

  /* make backup */

  r = make_backup(homedir, libdir, fil, fptmp);

  n_tmpfile_close(fptmp, tmp_name);
  g_free(fil);
  unlockconfig(dir);
  return r;
}

static int
removeconfig_match(FILE *fp, FILE *fptmp, struct narray *conf)
{
  char *s, *buf, **data;
  const char *s2, *tok;
  int out, len, len2, i, num, change;

  num = arraynum(conf);
  data = arraydata(conf);

  change = FALSE;
  while (fgetline(fp, &buf) == 0) {
    if (buf[0] == '[') {
      fputs(buf, fptmp);
      g_free(buf);
      break;
    } else {
      s = buf;
      out = TRUE;
      tok = getitok(&s, &len, " \t=,");
      if (tok != NULL) {
        for (i = 0; i < num; i++) {
          s2 = data[i];
          len2 = strlen(data[i]);
          if (len == len2 && strncmp(tok, s2, len) == 0)
	    out = FALSE;
        }
      }
      if (! out)
	change = TRUE;
      if (out && buf) {
        fputs(buf, fptmp);
        fputs("\n", fptmp);
      }
    }
    g_free(buf);
  }

  return change;
}

int
removeconfig(char *section,struct narray *conf)
{
  int change, r;
  struct objlist *sys;
  char *libdir,*homedir,*dir,*fil,*buf,*pfx, *tmp_name;
  FILE *fp,*fptmp;

  change=FALSE;
  if (arraynum(conf) == 0) return TRUE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"temp_prefix",0,0,NULL,&pfx)) return FALSE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return FALSE;
  if (!findfilename(homedir,CONFSEP,CONF)) {
    if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;
    if ((fil=getfilename(libdir,CONFSEP,CONF))==NULL) return FALSE;
    dir=libdir;
  } else {
    if ((fil=getfilename(homedir,CONFSEP,CONF))==NULL) return FALSE;
    dir=homedir;
  }
  lockconfig(dir);
  fptmp = n_tmpfile(&tmp_name);
  if (fptmp == NULL) {
    g_free(fil);
    unlockconfig(dir);
    return FALSE;
  }
  fp = nfopen(fil,"rt");
  if (fp == NULL) {
    n_tmpfile_close(fptmp, tmp_name);
    g_free(fil);
    unlockconfig(dir);
    return FALSE;
  }
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
      g_free(buf);
      change = removeconfig_match(fp, fptmp, conf);
      goto flush;
    } else {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
    }
    g_free(buf);
  }

flush:
  if (!change) {
    fclose(fp);
    n_tmpfile_close(fptmp, tmp_name);
    g_free(fil);
    unlockconfig(dir);
    return TRUE;
  }
  while (fgetline(fp,&buf)==0) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    g_free(buf);
  }

  fclose(fp);

  /* make backup */
  r = make_backup(homedir, libdir, fil, fptmp);

  n_tmpfile_close(fptmp, tmp_name);
  g_free(fil);
  unlockconfig(dir);
  return r;
}

int
writecheckconfig(void)
{
/* write OK in home : 1 */
/* write OK in lib: 2 */
/* write NG in home : -1 */
/* write NG in lib: -2 */
/* write OK in home but old: 3 */
/* write NG in home but old: -3 */
/* not find: 0 */
  struct objlist *sys;
  char *s,*libdir,*homedir,*homeconf,*libconf;
  GStatBuf homestat,libstat;
  int dir,ret;

  if ((sys=getobject("system"))==NULL) return 0;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return 0;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return 0;
  homeconf=libconf=NULL;
  if (findfilename(homedir,CONFSEP,CONF)) {
    if ((homeconf=getfilename(homedir,CONFSEP,CONF))!=NULL) {
      if (nstat(homeconf,&homestat)!=0) {
        g_free(homeconf);
        homeconf=NULL;
      }
    }
  }
  if (findfilename(libdir,CONFSEP,CONF)) {
    if ((libconf=getfilename(libdir,CONFSEP,CONF))!=NULL) {
      if (nstat(libconf,&libstat)!=0) {
        g_free(libconf);
        libconf=NULL;
      }
    }
  }
  if (homeconf!=NULL) {
    if (libconf==NULL) {
      dir=1;
      s=homeconf;
    } else if (homestat.st_mtime>=libstat.st_mtime) {
      dir=1;
      s=homeconf;
      g_free(libconf);
    } else {
      dir=3;
      s=homeconf;
      g_free(libconf);
    }
  } else if (libconf!=NULL) {
    dir=2;
    s=libconf;
  } else return 0;
  ret=naccess(s,W_OK);
  g_free(s);
  if (ret==0) return dir;
  return -dir;
}

int
copyconfig(void)
/* copy configuration file from libdir to home dir */
{
  struct objlist *sys;
  char *libdir,*homedir,*buf;
  char *libname,*homename;
  FILE *libfp,*homefp;
  int r;
  GStatBuf sbuf;

  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"conf_dir",0,0,NULL,&libdir)==-1) return FALSE;

  r = nstat(homedir, &sbuf);
  if (r && errno == ENOENT) {
    r = g_mkdir(homedir, 0755);
    if (r)
      return FALSE;
  } else if (! S_ISDIR(sbuf.st_mode)) {
    return FALSE;
  }

  if (findfilename(homedir,CONFSEP,CONF)) {
    char *bak;
    if ((bak=getfilename(homedir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(homedir,CONFSEP,CONFBAK)) g_unlink(bak);
      if ((homename=getfilename(homedir,CONFSEP,CONF))!=NULL) {
        rename(homename,bak);
        g_free(homename);
      }
      g_free(bak);
    }
  }
  if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;

  homename = getfilename(homedir,CONFSEP,CONF);
  libname = getfilename(libdir,CONFSEP,CONF);
  if (homename == NULL || libname == NULL) {
    g_free(homename);
    g_free(libname);
    return FALSE;
  }
  if (strcmp0(homename,libname)==0) {
    g_free(homename);
    g_free(libname);
    return FALSE;
  }

  if ((homefp=nfopen(homename,"wt"))==NULL) {
    return FALSE;
  }
  if ((libfp=nfopen(libname,"rt"))==NULL) {
    g_free(homename);
    fclose(homefp);
    g_free(libname);
    return FALSE;
  }
  g_free(homename);
  g_free(libname);
  while (fgetline(libfp,&buf)==0) {
    fputs(buf,homefp);
    fputs("\n",homefp);
    g_free(buf);
  }
  fclose(libfp);
  fclose(homefp);
  return TRUE;
}
