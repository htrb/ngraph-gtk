/*
 * $Id: oio.c,v 1.6 2010-03-04 08:30:16 hito Exp $
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
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <unistd.h>

#include "nhash.h"
#include "shell.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "oroot.h"

static char *io_errorlist[]={
  "file is not specified.",
  "invalid mode.",
  "I/O error: open",
  "I/O error: read",
  "I/O error: write",
  "I/O error: seek",
  "IO is closed.",
  "IO is opened.",
  "I/O error:",
  "the method is forbidden for the security",
};

#define ERRNUM (sizeof(io_errorlist) / sizeof(*io_errorlist))

#define NAME		"io"
#define PARENT		"object"
#define OVERSION	"1.00.00"

#define OIO_ERRFILE		100
#define OIO_ERRMODE		101
#define OIO_ERROPEN		102
#define OIO_ERRREAD		103
#define OIO_ERRWRITE		104
#define OIO_ERRSEEK		105
#define OIO_ERRCLOSED		106
#define OIO_ERROPENED		107
#define OIO_ERRSTD		108
#define OIO_ERRSYSSECURTY	109

char *seek_whence[]={
  "set",
  "cur",
  "end",
  NULL
};

enum IO_SEEK_WHENCE {
  IO_SEEK_SET,
  IO_SEEK_CUR,
  IO_SEEK_END,
};

struct io_local {
  FILE *fp;
  int popen;
};


static void
io_error(struct objlist *obj)
{
    const char *str;

    str = g_strerror(errno);
    error2(obj, OIO_ERRSTD, CHK_STR(str));
}

static int
io_init(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  char *mode;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  mode = g_strdup("r");
  if (mode == NULL) {
    return 1;
  }
  if (_putobj(obj, "mode", inst, mode)) {
    g_free(mode);
    return 1;
  }

  io_local = g_malloc0(sizeof(struct io_local));
  if (io_local == NULL) {
    g_free(mode);
    return 1;
  }

  if (_putobj(obj, "_local", inst, io_local)) {
    g_free(mode);
    g_free(io_local);
    return 1;
  }

  io_local->fp = NULL;
  io_local->popen = FALSE;

  return 0;
}

static int
io_close_sub(struct io_local *io_local)
{
  if (io_local->fp == NULL) {
    return 1;
  }

  if (io_local->popen) {
    pclose(io_local->fp);
  } else {
    fclose(io_local->fp);
  }
  io_local->fp = NULL;

  return 0;
}

static int
io_done(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  _getobj(obj,"_local",inst,&io_local);
  if (io_local->fp) {
    io_close_sub(io_local);
  }

  return 0;
}

static int
io_open(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  char *file, *mode;

  if (get_security()) {
    error(obj, OIO_ERRSYSSECURTY);
    return 1;
  }

  _getobj(obj, "_local", inst, &io_local);
  _getobj(obj, "mode", inst, &mode);

  if (mode == NULL || mode[0] == '\0') {
    mode = "r";
  }

  if (io_local->fp) {
    error(obj, OIO_ERROPENED);
    return 1;
  }

  file = argv[2];

  if (file == NULL) {
    return 1;
  }

  errno = 0;
  if (argv[1][0] == 'p') {
    fp = popen(file, mode);
    io_local->popen = TRUE;
  } else {
    fp = nfopen(file, mode);
    io_local->popen = FALSE;
  }

  if (fp == NULL) {
    io_error(obj);
    g_free(fp);
    return 1;
  }
  _putobj(obj, "file", inst, g_strdup(file));

  io_local->fp = fp;
  return 0;
}

static int
io_close(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  char *s;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    return 1;
  }

  _getobj(obj, "file", inst, &s);
  g_free(s);
  _putobj(obj, "file", inst, NULL);

  io_close_sub(io_local);

  return 0;
}

static int
io_puts(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int rcode;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    return 1;
  }

  errno = 0;
  if (argv[2]) {
    fputs(argv[2], fp);
  }
  rcode = fputs("\n", fp);

  if (rcode == EOF) {
    io_error(obj);
    return 1;
  }

  return 0;
}

static int
io_gets(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int rcode;
  char *buf;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  errno = 0;
  rcode = fgetline(fp, &buf);
  if (rcode < -1) {
    io_error(obj);
    return 1;
  } else if (rcode) {
    return 1;
  }

  rval->str = buf;

  return 0;
}

static int
io_getc(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int rcode;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  errno = 0;
  rcode = fgetc(fp);
  if (rcode == EOF && ferror(fp)) {
    io_error(obj);
    return 1;
  }

  rval->i = rcode;

  return 0;
}

static int
io_putc(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int c;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  c = *(int *) argv[2];

  errno = 0;
  c = fputc(c, fp);
  if (c == EOF) {
    io_error(obj);
    return 1;
  }

  rval->i = c;

  return 0;
}

static int
io_read(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int l;
  size_t len, rlen;
  char *buf;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  l = * (int *) argv[2];
  if (l < 0) {
    return 1;
  } else if (l < 1) {
    return 0;
  }
  len = l;

  errno = 0;
  buf = g_malloc(len + 1);
  if (buf == NULL) {
    io_error(obj);
    return 1;
  }

  rlen = fread(buf, 1, len, fp);
  if (rlen == 0) {
    if (ferror(fp)) {
      io_error(obj);
    }
    g_free(buf);
    return 1;
  }
  buf[rlen] = '\0';

  rval->str = buf;

  return 0;
}

static int
io_write(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  size_t len, rlen;
  char *buf;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  buf = argv[2];
  if (buf == NULL) {
    return 1;
  }

  len = strlen(buf);

  errno = 0;
  rlen = fwrite(buf, 1, len, fp);
  if (rlen < len) {
    io_error(obj);
    return 1;
  }

  rval->i = rlen;

  return 0;
}

static int
io_seek(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int r, pos, w, whence;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  _getobj(obj, "whence", inst, &w);
  switch (w) {
  case IO_SEEK_SET:
    whence = SEEK_SET;
    break;
  case IO_SEEK_CUR:
    whence = SEEK_CUR;
    break;
  case IO_SEEK_END:
    whence = SEEK_END;
    break;
  default:
    return 1;
  }

  pos = *(int *) argv[2];

  errno = 0;
  r = fseek(fp, pos, whence);
  if (r) {
    io_error(obj);
    return 1;
  }

  return 0;
}

static int
io_rewind(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  rewind(fp);

  return 0;
}

static int
io_tell(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  long pos;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  errno = 0;
  pos = ftell(fp);
  if (pos < 0) {
    error(obj, OIO_ERRSEEK);
    return 1;
  }

  rval->i = pos;

  return 0;
}

static int
io_flush(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int r;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  errno = 0;
  r = fflush(fp);
  if (r) {
    io_error(obj);
    return 1;
  }

  return 0;
}

static int
io_eof(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct io_local *io_local;
  FILE *fp;
  int r;

  _getobj(obj, "_local", inst, &io_local);
  fp = io_local->fp;
  if (fp == NULL) {
    error(obj, OIO_ERRCLOSED);
    return 1;
  }

  r = feof(fp);

  rval->i = (r) ? TRUE : FALSE;

  return r ? 0 : 1;
}

static struct objtable io[] = {
  {"init",NVFUNC,NEXEC,io_init,NULL,0},
  {"done",NVFUNC,NEXEC,io_done,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"file",NSTR,NREAD,NULL,NULL,0},
  {"mode",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"open",NVFUNC,NREAD|NEXEC,io_open,"s",0},
  {"popen",NVFUNC,NREAD|NEXEC,io_open,"s",0},
  {"close",NVFUNC,NREAD|NEXEC,io_close,"",0},
  {"puts",NVFUNC,NREAD|NEXEC,io_puts,"s",0},
  {"putc",NIFUNC,NREAD|NEXEC,io_putc,"i",0},
  {"gets",NSFUNC,NREAD|NEXEC,io_gets,"",0},
  {"getc",NIFUNC,NREAD|NEXEC,io_getc,"",0},
  {"read",NSFUNC,NREAD|NEXEC,io_read,"i",0},
  {"write",NIFUNC,NREAD|NEXEC,io_write,"s",0},
  {"whence",NENUM,NREAD|NWRITE,NULL,seek_whence,0},
  {"seek",NVFUNC,NREAD|NEXEC,io_seek,"i",0},
  {"tell",NIFUNC,NREAD|NEXEC,io_tell,"",0},
  {"rewind",NVFUNC,NREAD|NEXEC,io_rewind,"",0},
  {"flush",NVFUNC,NREAD|NEXEC,io_flush,"",0},
  {"eof",NBFUNC,NREAD|NEXEC,io_eof,"",0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define OIO_TBLNUM (sizeof(io) / sizeof(*io))

void *
addio(void)
/* addio() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, OIO_TBLNUM, io, ERRNUM, io_errorlist, NULL, NULL);
}
