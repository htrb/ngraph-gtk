/*
 * $Id: ogra2fil.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <glib.h>

#include "object.h"
#include "ioutil.h"

#define NAME "gra2file"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define ERRFOPEN 100

static char *gra2ferrorlist[]={
  "I/O error: open file",
};

#define ERRNUM (sizeof(gra2ferrorlist) / sizeof(*gra2ferrorlist))

struct gra2flocal {
  FILE *fil;
};

static int
gra2finit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct gra2flocal *gra2flocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if ((gra2flocal=g_malloc(sizeof(struct gra2flocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,gra2flocal)) goto errexit;
  gra2flocal->fil=NULL;
  return 0;

errexit:
  g_free(gra2flocal);
  return 1;
}

static int
gra2fdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct gra2flocal *gra2flocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&gra2flocal);
  if (gra2flocal->fil!=NULL) fclose(gra2flocal->fil);
  return 0;
}

static int
gra2f_output(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  struct gra2flocal *gra2flocal;
  char code;
  const int *cpar;
  const char *cstr;
  char *fname,*graf,*sname,*sver;

  gra2flocal=(struct gra2flocal *)argv[2];
  code=*(char *)(argv[3]);
  cpar=(int *)argv[4];
  cstr=argv[5];

  if (code=='I') {
    struct objlist *sys;
    if (gra2flocal->fil!=NULL) fclose(gra2flocal->fil);
    gra2flocal->fil=NULL;
    _getobj(obj,"file",inst,&fname);
    if (fname==NULL) return 1;
    if ((gra2flocal->fil=nfopen(fname,"wt"))==NULL) {
      error2(obj,ERRFOPEN,fname);
      return 1;
    }
    if ((sys=getobject("system"))==NULL) return 1;
    if (getobj(sys,"name",0,0,NULL,&sname)) return 1;
    if (getobj(sys,"version",0,0,NULL,&sver)) return 1;
    if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
    fprintf(gra2flocal->fil,"%s\n",graf);
    fprintf(gra2flocal->fil,"%%Creator: %s ver %s\n",sname,sver);
  }
  if (gra2flocal->fil!=NULL) {
    int i;
    fputc(code,gra2flocal->fil);
    if (cpar[0]==-1) {
      for (i=0;cstr[i]!='\0';i++)
        fputc(cstr[i],gra2flocal->fil);
    } else {
      fprintf(gra2flocal->fil,",%d",cpar[0]);
      for (i=1;i<=cpar[0];i++)
        fprintf(gra2flocal->fil,",%d",cpar[i]);
    }
    fputc('\n',gra2flocal->fil);
    if (code=='E') {
      fclose(gra2flocal->fil);
      gra2flocal->fil=NULL;
    }
  }
  return 0;
}

static struct objtable gra2f[] = {
  {"init",NVFUNC,NEXEC,gra2finit,NULL,0},
  {"done",NVFUNC,NEXEC,gra2fdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"file",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},
  {"_output",NVFUNC,0,gra2f_output,NULL,0},
};

#define TBLNUM (sizeof(gra2f) / sizeof(*gra2f))

void *
addgra2file(void)
/* addgra2file() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,gra2f,ERRNUM,gra2ferrorlist,NULL,NULL);
}
