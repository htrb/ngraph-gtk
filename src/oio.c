/* 
 * $Id: oio.c,v 1.1 2009/11/25 14:36:02 hito Exp $
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
  "file is closed.",
  "I/O error: open file",
  "I/O error: read file",
  "I/O error: write file",
  "invalid mode.",
};

#define ERRNUM (sizeof(f2derrorlist) / sizeof(*f2derrorlist))

#define NAME		"io"
#define PARENT		"object"
#define OVERSION	"1.00.00"

#define ERRFILE		100
#define ERRMODE		101
#define ERROPEN		102
#define ERRREAD		103
#define ERRWRITE	104
#define ERRCLOSED	105

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
  char *file, *mode;

  _getobj(obj, "_local", inst, &f2dlocal);

  if (f2dlocal->fp) {
    return 1;
  }

  file = argv[2];
  mode = argv[3];

  if (file == NULL || mode == NULL) {
    return 1;
  }

  errno = 0;
  fp = nfopen(file, mode);
  if (fp == NULL) {
    if (errno == EINVAL) {
      error(obj, ERRMODE);
    } else {
      error2(obj, ERROPEN, file);
    }
    g_free(fp);
    return 1;
  }
  _putobj(obj, "file", inst, g_strdup(file));

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

  _putobj(obj, "file", inst, NULL);
  fclose(fp);
  f2dlocal->fp = NULL;

  return 0;
}

static int 
f2dputs(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;
  int rcode;

  _getobj(obj, "_local", inst, &f2dlocal);
  fp = f2dlocal->fp;
  if (fp == NULL) {
    return 1;
  }

  if (argv[2]) {
    rcode = fputs(argv[2], fp);
  }
  rcode = fputs("\n", fp);
     
  if (rcode == EOF) {
    error(obj, ERRWRITE);
    return 1;
  }

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
    error(obj, ERRCLOSED);
    return 1;
  }

  rcode = fgetline(fp, &buf);
  if (rcode) {
    error(obj, ERRREAD);
    return 1;
  }

  *(char **)rval = buf;

  return 0;
}

static int 
f2dgetc(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;
  int rcode;

  _getobj(obj, "_local", inst, &f2dlocal);
  fp = f2dlocal->fp;
  if (fp == NULL) {
    error(obj, ERRCLOSED);
    return 1;
  }

  rcode = fgetc(fp);
  if (rcode) {
    return 1;
  }

  *(int *)rval = rcode;

  return 0;
}

static int 
f2dputc(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  FILE *fp;
  int c;

  _getobj(obj, "_local", inst, &f2dlocal);
  fp = f2dlocal->fp;
  if (fp == NULL) {
    error(obj, ERRCLOSED);
    return 1;
  }

  c = *(int *) argv[2];

  c = fputc(c, fp);
  if (c) {
    return 1;
  }

  *(int *) rval = c;

  return 0;
}

static struct objtable file2d[] = {
  {"init",NVFUNC,NEXEC,f2dinit,NULL,0},
  {"done",NVFUNC,NEXEC,f2ddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"file",NSTR,NREAD,NULL,NULL,0},
  {"open",NVFUNC,NREAD|NEXEC,f2dopen,"ss",0},
  {"popen",NVFUNC,NREAD|NEXEC,f2dopen,"ss",0},
  {"close",NVFUNC,NREAD|NEXEC,f2dclose,"",0},
  {"puts",NVFUNC,NREAD|NEXEC,f2dputs,"s",0},
  {"putc",NIFUNC,NREAD|NEXEC,f2dputc,"i",0},
  {"gets",NSFUNC,NREAD|NEXEC,f2dgets,"",0},
  {"getc",NIFUNC,NREAD|NEXEC,f2dgetc,"",0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(file2d) / sizeof(*file2d))

void *
addio(void)
/* addio() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,file2d,ERRNUM,f2derrorlist,NULL,NULL);
}

