/* 
 * $Id: orawfile.c,v 1.1 2009/11/25 12:19:07 hito Exp $
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
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <errno.h>
#include <glib.h>

#ifndef WINDOWS
#include <unistd.h>
#else
#include <io.h>
#include <sys/stat.h>
#endif

#include "common.h"

#include "ngraph.h"
#include "nhash.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "oroot.h"

static char *f2derrorlist[]={
  "file is not specified.",
  "I/O error: open file",
  "I/O error: read file",
  "I/O error: write file",
};

#define ERRNUM (sizeof(f2derrorlist) / sizeof(*f2derrorlist))

#define NAME		"rawfile"
#define PARENT		"object"
#define OVERSION	"1.00.00"

#define ERRFILE		100
#define ERROPEN		101
#define ERRREAD		102
#define ERRWRITE	103

struct f2dlocal {
  FILE *fp;
};

static int 
f2dinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  f2dlocal = g_malloc0(sizeof(struct f2dlocal));
  if (f2dlocal == NULL) {
    return 1;
  }

  if (_putobj(obj, "_local", inst, f2dlocal)) {
    g_free(f2dlocal);
    return 1;
  }

  f2dlocal->fp = NULL;

  return 0;
}

static int 
f2ddone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  _getobj(obj,"_local",inst,&f2dlocal);
  if (f2dlocal->fp) {
    fclose(f2dlocal->fp);
    f2dlocal->fp = NULL;
  }

  return 0;
}

static int 
f2dopen(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;
  char *file;

  _getobj(obj, "_local", inst, &f2dlocal);

  if (f2dlocal->fp) {
    return 1;
  }

  _getobj(obj, "file", inst, &file);
  if (file == NULL) {
    return 1;
  }

  fp = nfopen(file, "rt");
  if (fp == NULL) {
    error2(obj, ERROPEN, file);
    g_free(fp);
    return 1;
  }

  f2dlocal->fp = fp;
  return 0;
}

static int 
f2dclose(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;

  _getobj(obj, "_local", inst, &f2dlocal);
  fp = f2dlocal->fp;
  if (fp == NULL) {
    return 1;
  }

  fclose(fp);
  f2dlocal->fp = NULL;

  return 0;
}

static int 
f2dgets(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;
  int rcode;
  char *buf;

  g_free(*(char **)rval);
  *(char **)rval = NULL;

  _getobj(obj, "_local", inst, &f2dlocal);
  fp = f2dlocal->fp;
  if (fp == NULL) {
    return 1;
  }

  rcode = fgetline(fp, &buf);
  if (rcode) {
    return 1;
  }

  *(char **)rval = buf;

  return 0;
}

static struct objtable file2d[] = {
  {"init",NVFUNC,NEXEC,f2dinit,NULL,0},
  {"done",NVFUNC,NEXEC,f2ddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"file",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"open",NVFUNC,NREAD|NEXEC,f2dopen,NULL,0},
  {"close",NVFUNC,NREAD|NEXEC,f2dclose,NULL,0},
  {"gets",NSFUNC,NREAD|NEXEC,f2dgets,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(file2d) / sizeof(*file2d))

void *
addrawfile(void)
/* addrawfile() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,file2d,ERRNUM,f2derrorlist,NULL,NULL);
}

