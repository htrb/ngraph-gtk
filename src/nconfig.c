/* 
 * $Id: nconfig.c,v 1.2 2008/07/04 10:52:48 hito Exp $
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
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "nconfig.h"
#ifndef WINDOWS
#include <unistd.h>
#else
#include <io.h>
#include <dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define CONF "Ngraph.ini"
#define CONFBAK "Ngraph.in~"
#define LOCK "Ngraph.lock"

#define TRUE  1
#define FALSE 0

char *getscriptname(char *file)
{
  struct objlist *sys;
  char *homedir,*s;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  s=getfilename(homedir,CONFTOP,file);
  return s;
}

char *searchscript(char *file)
{
  struct objlist *sys;
  char *libdir,*homedir,*s;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return NULL;
  if (!findfilename(homedir,CONFTOP,file)) {
    if (!findfilename(libdir,CONFTOP,file)) return NULL;
    if ((s=getfilename(libdir,CONFTOP,file))==NULL) return NULL;
  } else {
    if ((s=getfilename(homedir,CONFTOP,file))==NULL) return NULL;
  }
  return s;
}

int configlocked(char *dir)
{
  if (findfilename(dir,CONFSEP,LOCK)) return TRUE;
  else return FALSE;
}

void lockconfig(char *dir)
{
  char *file;
  FILE *fp;

  while (configlocked(dir)) ;
  if ((file=getfilename(dir,CONFSEP,LOCK))==NULL) return;
  if ((fp=fopen(file,"wt"))==NULL) {
    memfree(file);
    return;
  }
  fputs("Ngraph.ini is locked",fp);
  fclose(fp);
  memfree(file);
}

void unlockconfig(char *dir)
{
  char *file;

  if (!findfilename(dir,CONFSEP,LOCK)) return;
  if ((file=getfilename(dir,CONFSEP,LOCK))==NULL) return;
  unlink(file);
  memfree(file);
}

FILE *openconfig(char *section)
{
  struct objlist *sys;
  char *libdir,*s,*homedir,*homeconf,*libconf,*buf;
  FILE *fp;
  struct stat homestat,libstat;

  if ((sys=getobject("system"))==NULL) return NULL;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return NULL;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return NULL;
  homeconf=libconf=NULL;
  if (findfilename(homedir,CONFSEP,CONF)) {
    if ((homeconf=getfilename(homedir,CONFSEP,CONF))!=NULL) {
      if (stat(homeconf,&homestat)!=0) {
        memfree(homeconf);
        homeconf=NULL;
      }
    }
  }
  if (findfilename(libdir,CONFSEP,CONF)) {
    if ((libconf=getfilename(libdir,CONFSEP,CONF))!=NULL) {
      if (stat(libconf,&libstat)!=0) {
        memfree(libconf);
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
      memfree(libconf);
    } else {
      s=libconf;
      memfree(homeconf);
    }
#else
    s=homeconf;
    memfree(libconf);
#endif
  } else if (libconf) {
    s=libconf;
  } else {
    return NULL;
  }
  if ((fp=nfopen(s,"rt"))==NULL) {
    memfree(s);
    return NULL;
  }
  memfree(s);
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      memfree(buf);
      return fp;
    }
    memfree(buf);
  }
  fclose(fp);
  return NULL;
}

char *getconfig(FILE *fp,char **val)
{
  char *s,*tok,*buf;
  int len;

  while (TRUE) {
    if (fgetline(fp,&buf)!=0) return NULL;
    else {
      if (buf[0]=='[') {
        memfree(buf);
        return NULL;
      } else {
        s=buf;
        if ((tok=getitok2(&s,&len,"="))!=NULL) {
          if (s[0]=='=') s++;
          if ((*val=memalloc(strlen(s)+1))==NULL) {
            memfree(buf);
            memfree(tok);
            return NULL;
          }
          strcpy(*val,s);
          memfree(buf);
          return tok;
        }
        memfree(buf);
        memfree(tok);
      }
    }
  }
}

void closeconfig(FILE *fp)
{
  fclose(fp);
}

int replaceconfig(char *section,struct narray *conf)
{
  int i,j,num,num2,out,len,len2;
  char **data;
  struct objlist *sys;
  char *libdir,*homedir,*dir,*fil,*bak,*tmpfil,*s,*s2,*tok,*tok2,*buf,*pfx;
  FILE *fp,*fptmp;
  struct narray iconf;

  num=arraynum(conf);
  if (num==0) return TRUE;
  data=arraydata(conf);
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"temp_prefix",0,0,NULL,&pfx)) return FALSE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return FALSE;

  if (!findfilename(homedir,CONFSEP,CONF)) {
    if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;
    if ((fil=getfilename(libdir,CONFSEP,CONF))==NULL) return FALSE;
    dir=libdir;
  } else {
    if ((fil=getfilename(homedir,CONFSEP,CONF))==NULL) return FALSE;
    dir=homedir;
  }
  lockconfig(dir);

  tmpfil = tempnam(NULL, pfx);
  if (tmpfil == NULL)  {
    memfree(fil);
    unlockconfig(dir);
    return FALSE;
  }

  fptmp=fopen(tmpfil,"wt");
  if (fptmp == NULL) {
    free(tmpfil);
    unlockconfig(dir);
    return FALSE;
  }

  fp = fopen(fil,"rt");
  if (fp == NULL) {
    free(tmpfil);
    fclose(fptmp);
    memfree(fil);
    unlockconfig(dir);
    return FALSE;
  }

  arrayinit(&iconf,sizeof(int));
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
      memfree(buf);
      goto match;
    }
    else {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
    }
    memfree(buf);
  }
/* section not found */
  fputs("\n",fptmp);
  fputs(section,fptmp);
  fputs("\n",fptmp);
  goto flush;

match:
  while (fgetline(fp,&buf)==0) {
    if (buf[0]=='[') goto flush;
    else {
      s=buf;
      out=FALSE;
      tok=getitok(&s,&len," \t=,");
      if (tok != NULL) {
        for (i=0;i<num;i++) {
          s2=data[i];
	  tok2=getitok(&s2,&len2," \t=,");
          if ((tok2 != NULL) && ((len==len2) && (strncmp(tok,tok2,len)==0))) {
            out=TRUE;
            num2=arraynum(&iconf);
            for (j = 0; j < num2; j++) {
	      if (i == *(int *)arraynget(&iconf, j))
		break;
	    }
            if (j == num2) {
              fputs(data[i],fptmp);
              fputs("\n",fptmp);
              arrayadd(&iconf,&i);
            }
          }
        }
      }
      if ((!out) && (buf!=NULL) && (buf[0]!='\0')) {
        fputs(buf,fptmp);
        fputs("\n",fptmp);
      }
    }
    memfree(buf);
  }

flush:
  for (i=0;i<num;i++) {
    num2=arraynum(&iconf);
    for (j=0;j<num2;j++) if (i==*(int *)arraynget(&iconf,j)) break;
    if (j==num2) {
      fputs(data[i],fptmp);
      fputs("\n",fptmp);
    }
  }
  fputs("\n",fptmp);
  if (buf!=NULL) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    memfree(buf);
  }
  while (fgetline(fp,&buf)==0) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    memfree(buf);
  }

  arraydel(&iconf);
  fclose(fp);
  fclose(fptmp);

  /* make backup */
  if (!findfilename(homedir,CONFSEP,CONF)) {
    if ((bak=getfilename(libdir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(libdir,CONFSEP,CONFBAK)) unlink(bak);
    }
  } else {
    if ((bak=getfilename(homedir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(homedir,CONFSEP,CONFBAK)) unlink(bak);
    }
  }
  if (bak!=NULL) {
    rename(fil,bak);
    memfree(bak);
  }

  if ((fp=fopen(fil,"wt"))==NULL) {
    memfree(fil);
    unlockconfig(dir);
    return FALSE;
  }
  if ((fptmp=fopen(tmpfil,"rt"))==NULL) {
    memfree(fil);
    fclose(fp);
    fclose(fptmp);
    unlink(tmpfil);
    free(tmpfil);
    unlockconfig(dir);
    return FALSE;
  }

  while (fgetline(fptmp,&buf)==0) {
    fputs(buf,fp);
    fputs("\n",fp);
    memfree(buf);
  }

  fclose(fp);
  fclose(fptmp);
  unlink(tmpfil);
  free(tmpfil);
  memfree(fil);
  unlockconfig(dir);
  return TRUE;
}

int removeconfig(char *section,struct narray *conf)
{
  int i,num,out,len,len2,change;
  char **data;
  struct objlist *sys;
  char *libdir,*homedir,*dir,*fil,*bak,*tmpfil,*s,*s2,*tok,*buf,*pfx;
  FILE *fp,*fptmp;

  num=arraynum(conf);
  if (num==0) return TRUE;
  data=arraydata(conf);
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"temp_prefix",0,0,NULL,&pfx)) return FALSE;
  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return FALSE;
  if (!findfilename(homedir,CONFSEP,CONF)) {
    if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;
    if ((fil=getfilename(libdir,CONFSEP,CONF))==NULL) return FALSE;
    dir=libdir;
  } else {
    if ((fil=getfilename(homedir,CONFSEP,CONF))==NULL) return FALSE;
    dir=homedir;
  }
  lockconfig(dir);
  if ((tmpfil=tempnam(NULL,pfx))==NULL) {
    unlockconfig(dir);
    return FALSE;
  }
  if ((fptmp=fopen(tmpfil,"wt"))==NULL) {
    free(tmpfil);
    unlockconfig(dir);
    return FALSE;
  }
  if ((fp=fopen(fil,"rt"))==NULL) {
    free(tmpfil);
    fclose(fptmp);
    memfree(fil);
    fclose(fp);
    unlockconfig(dir);
    return FALSE;
  }
  while (fgetline(fp,&buf)==0) {
    if (strcmp0(buf,section)==0) {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
      memfree(buf);
      goto match;
    } else {
      fputs(buf,fptmp);
      fputs("\n",fptmp);
    }
    memfree(buf);
  }
/* section not found */
  fclose(fp);
  fclose(fptmp);
  unlink(tmpfil);
  free(tmpfil);
  memfree(fil);
  unlockconfig(dir);
  return TRUE;

match:
  change=FALSE;
  while (fgetline(fp,&buf)==0) {
    if (buf[0]=='[') goto flush;
    else {
      s=buf;
      out=TRUE;
      if ((tok=getitok(&s,&len," \t=,"))!=NULL) {
        for (i=0;i<num;i++) {
          s2=data[i];
          len2=strlen(data[i]);
          if ((len==len2) && (strncmp(tok,s2,len)==0)) out=FALSE;
        }
      }
      if (!out) change=TRUE;
      if (out && (buf!=NULL)) {
        fputs(buf,fptmp);
        fputs("\n",fptmp);
      }
    }
    memfree(buf);
  }

flush:
  if (!change) {
    fclose(fp);
    fclose(fptmp);
    unlink(tmpfil);
    free(tmpfil);
    memfree(fil);
    unlockconfig(dir);
    return TRUE;
  }
  if (buf!=NULL) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    memfree(buf);
  }
  while (fgetline(fp,&buf)==0) {
    fputs(buf,fptmp);
    fputs("\n",fptmp);
    memfree(buf);
  }

  fclose(fp);
  fclose(fptmp);

  /* make backup */
  if (!findfilename(homedir,CONFSEP,CONF)) {
    if ((bak=getfilename(libdir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(libdir,CONFSEP,CONFBAK)) unlink(bak);
    }
  } else {
    if ((bak=getfilename(homedir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(homedir,CONFSEP,CONFBAK)) unlink(bak);
    }
  }
  if (bak!=NULL) {
    rename(fil,bak);
    memfree(bak);
  }

  if ((fp=fopen(fil,"wt"))==NULL) {
    memfree(fil);
    unlockconfig(dir);
    return FALSE;
  }
  if ((fptmp=fopen(tmpfil,"rt"))==NULL) {
    memfree(fil);
    fclose(fp);
    fclose(fptmp);
    unlink(tmpfil);
    free(tmpfil);
    unlockconfig(dir);
    return FALSE;
  }

  while (fgetline(fptmp,&buf)==0) {
    fputs(buf,fp);
    fputs("\n",fp);
    memfree(buf);
  }

  fclose(fp);
  fclose(fptmp);
  unlink(tmpfil);
  free(tmpfil);
  memfree(fil);
  unlockconfig(dir);
  return TRUE;
}

int writecheckconfig()
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
  struct stat homestat,libstat;
  int dir,ret;

  if ((sys=getobject("system"))==NULL) return 0;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return 0;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return 0;
  homeconf=libconf=NULL;
  if (findfilename(homedir,CONFSEP,CONF)) {
    if ((homeconf=getfilename(homedir,CONFSEP,CONF))!=NULL) {
      if (stat(homeconf,&homestat)!=0) {
        memfree(homeconf);
        homeconf=NULL;
      }
    }
  }
  if (findfilename(libdir,CONFSEP,CONF)) {
    if ((libconf=getfilename(libdir,CONFSEP,CONF))!=NULL) {
      if (stat(libconf,&libstat)!=0) {
        memfree(libconf);
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
      memfree(libconf);
    } else {
      dir=3;
      s=homeconf;
      memfree(libconf);
    }
  } else if (libconf!=NULL) {
    dir=2;
    s=libconf;
  } else return 0;
  ret=access(s,02);
  memfree(s);
  if (ret==0) return dir;
  return -dir;
}

int copyconfig()
/* copy configuration file from libdir to home dir */
{
  struct objlist *sys;
  char *libdir,*homedir,*buf;
  char *libname,*homename,*bak;
  FILE *libfp,*homefp;
  int r;
  struct stat sbuf;

  if ((sys=getobject("system"))==NULL) return FALSE;
  if (getobj(sys,"home_dir",0,0,NULL,&homedir)==-1) return FALSE;
  if (getobj(sys,"lib_dir",0,0,NULL,&libdir)==-1) return FALSE;

  r = stat(homedir, &sbuf);
  if (r && errno == ENOENT) {
    r = mkdir(homedir, 0755);
    if (r)
      return FALSE;
  } else if (! S_ISDIR(sbuf.st_mode)) {
    return FALSE;
  }

  if (findfilename(homedir,CONFSEP,CONF)) {
    if ((bak=getfilename(homedir,CONFSEP,CONFBAK))!=NULL) {
      if (findfilename(homedir,CONFSEP,CONFBAK)) unlink(bak);
      if ((homename=getfilename(homedir,CONFSEP,CONF))!=NULL) {
        rename(homename,bak);
        memfree(homename);
      }
      memfree(bak);
    }
  }
  if (!findfilename(libdir,CONFSEP,CONF)) return FALSE;
  if (((homename=getfilename(homedir,CONFSEP,CONF))==NULL) ||
      ((libname=getfilename(libdir,CONFSEP,CONF))==NULL)) {
    memfree(homename);
    memfree(libname);
    return FALSE;
  }
  if (strcmp0(homename,libname)==0) {
    memfree(homename);
    memfree(libname);
    return FALSE;
  }

  if ((homefp=fopen(homename,"wt"))==NULL) {
    return FALSE;
  }
  if ((libfp=fopen(libname,"rt"))==NULL) {
    memfree(homename);
    fclose(homefp);
    memfree(libname);
    fclose(libfp);
    return FALSE;
  }
  memfree(homename);
  memfree(libname);
  while (fgetline(libfp,&buf)==0) {
    fputs(buf,homefp);
    fputs("\n",homefp);
    memfree(buf);
  }
  fclose(libfp);
  fclose(homefp);
  return TRUE;
}
