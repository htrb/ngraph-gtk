/*
 * $Id: omerge.c,v 1.16 2010-03-04 08:30:16 hito Exp $
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
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <glib.h>
#include <unistd.h>

#include "object.h"
#include "nstring.h"
#include "ntime.h"
#include "ioutil.h"
#include "gra.h"
#include "oroot.h"
#include "odata.h"
#include "odraw.h"
#include "osystem.h"

#define NAME "merge"
#define PARENT "draw"
#define OVERSION  "1.00.00"

#define OLD_GRA_HEADER " Ngraph GRA file"

#define ERRFILE 100
#define ERROPEN 101
#define ERRGRA  102
#define ERRGRAFM 103

static char *mergeerrorlist[]={
  "GRA file is not specified.",
  "I/O error: open file",
  "not GRA file",
  "illegal GRA format",
};

#define ERRNUM (sizeof(mergeerrorlist) / sizeof(*mergeerrorlist))

struct mergelocal {
  FILE *storefd;
  int endstore;
  time_t mtime;
  int bbox[4];
};

struct gra_info
{
  int GC, lm, tm, zmx, zmy;
  GStatBuf gstat;
  struct objlist *obj;
};

#define USE_MERGE_CACHE 1

#if USE_MERGE_CACHE
struct gra_cache
{
  struct GRAdata *data;
  GStatBuf gstat;
};

static NHASH GraCache = NULL;
#endif

static void
set_bbox(struct objlist *obj,N_VALUE *inst, struct narray *array, int l, int t, int zx, int zy)
{
  int *bbox;
  double zoom_x, zoom_y;
  struct mergelocal *mergelocal;
  _getobj(obj,"_local",inst,&mergelocal);
  if (arraynum(array) != 4) {
    return;
  }
  bbox = arraydata(array);
  zoom_x = zx / 10000.0;
  zoom_y = zy / 10000.0;
  bbox[0] = (mergelocal->bbox[0]) * zoom_x + l;
  bbox[1] = (mergelocal->bbox[1]) * zoom_y + t;
  bbox[2] = (mergelocal->bbox[2]) * zoom_x + l;
  bbox[3] = (mergelocal->bbox[3]) * zoom_y + t;
}

#if USE_MERGE_CACHE
static struct GRAdata *
gra_data_new(void)
{
  struct GRAdata *data;
  data = g_malloc(sizeof(*data));

  if (data == NULL) {
    return NULL;
  }
  data->code = '\0';
  data->cpar = NULL;
  data->cstr = NULL;
  data->next = NULL;
  return data;
}

static void
gra_data_free(struct GRAdata *data)
{
  struct GRAdata *next;
  while (data) {
    next = data->next;
    g_free(data->cpar);
    g_free(data->cstr);
    g_free(data);
    data = next;
  }
}

static struct gra_cache *
gra_cache_new(const char *file)
{
  struct gra_cache *cache;

  if (GraCache == NULL) {
    GraCache = nhash_new();
  }

  if (GraCache == NULL) {
    return NULL;
  }

  cache = g_malloc(sizeof(*cache));
  if (cache == NULL) {
    return NULL;
  }
  cache->data = NULL;
  cache->gstat.st_mtime = 0;
  cache->gstat.st_size = 0;

  nhash_set_ptr(GraCache, file, cache);
  return cache;
}

static struct gra_cache *
gra_cache_get(const char *file)
{
  void *ptr;
  if (GraCache == NULL) {
    return NULL;
  }

  nhash_get_ptr(GraCache, file, &ptr);
  return ptr;
}

static void
gra_cache_free(const char *file)
{
  struct gra_cache *cache;

  cache = gra_cache_get(file);
  if (cache == NULL) {
    return;
  }

  gra_data_free(cache->data);
  g_free(cache);
  nhash_del(GraCache, file);
}
#endif  /* USE_MERGE_CACHE */

static int
mergeinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int zm, n;
  struct mergelocal *mergelocal;
  char *ext;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zm=10000;
  n = 0;

  ext = g_strdup("gra");

  if (_putobj(obj,"zoom_x",inst,&zm)) return 1;
  if (_putobj(obj,"zoom_y",inst,&zm)) return 1;
  if (_putobj(obj,"line_num",inst,&n)) return 1;
  if (_putobj(obj,"ext",inst,ext)) return 1;
  if ((mergelocal=g_malloc(sizeof(struct mergelocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,mergelocal)) goto errexit;

  mergelocal->storefd=NULL;
  mergelocal->endstore=FALSE;
  mergelocal->mtime = 0;
  mergelocal->bbox[0] = 0;
  mergelocal->bbox[1] = 0;
  mergelocal->bbox[2] = 0;
  mergelocal->bbox[3] = 0;
  return 0;

errexit:
  g_free(mergelocal);
  return 1;
}

static int
mergedone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

#if USE_MERGE_CACHE
static void
gra_cache_init(struct gra_cache *cache, struct gra_info *info)
{
  if (cache->data) {
    gra_data_free(cache->data);
    cache->data = NULL;
  }
  if (info) {
    cache->gstat = info->gstat;
  } else {
    cache->gstat.st_mtime = 0;
    cache->gstat.st_size = 0;
  }
}

static int *
dup_cpar(const int *cpar)
{
  gsize n;

  n = cpar[0] + 1;
  if (n < 1) {
    n = 1;
  }
#if GLIB_CHECK_VERSION(2, 68, 0)
  return g_memdup2(cpar, n * sizeof(*cpar));
#else
  return g_memdup(cpar, n * sizeof(*cpar));
#endif
}

static int
draw_gra_data(struct gra_info *info, struct GRAdata *data)
{
  int rcode, *cpar;
  cpar = dup_cpar(data->cpar);
  if (cpar == NULL) {
    return FALSE;
  }
  rcode = GRAinputdraw(info->GC, info->lm, info->tm, info->zmx, info->zmy, data->code, cpar, data->cstr);
  g_free(cpar);
  return rcode;
}

static int
read_new_gra_file(struct gra_cache *cache, struct gra_info *info, FILE *fd)
{
  int r;
  char *buf;
  int rcode;
  struct GRAdata *prev, *data;

  gra_cache_init(cache, info);

  r = 0;
  prev = NULL;
  while ((rcode = fgetline(fd, &buf)) != 1) {
    if (rcode == -1) {
      return 1;
    }
    data = gra_data_new();
    if (data == NULL) {
      goto errexit;
    }
    rcode = GRAparse(data, buf);
    if (! rcode) {
      GRAdata_free(data);
      error2(info->obj, ERRGRAFM, buf);
      goto errexit;
    }
    rcode = draw_gra_data(info, data);
    if (! rcode) {
      GRAdata_free(data);
      error2(info->obj, ERRGRAFM, buf);
      goto errexit;
    }
    g_free(buf);
    if (prev) {
      prev->next = data;
    } else {
      cache->data = data;
    }
    prev = data;
  }
  return r;

errexit:
  g_free(buf);
  gra_cache_init(cache, NULL);
  return 1;
}

static int
read_old_gra_file(struct gra_cache *cache, struct gra_info *info, FILE *fd, const char *file)
{
  int r;
  char *buf;
  int rcode;

  gra_cache_init(cache, NULL);
  rcode = fgetline(fd, &buf);
  if (rcode == 1) {
    error2(info->obj, ERRGRA, file);
  }
  g_free(buf);
  if (rcode != 0) {
    return 1;
  }

  r = 0;
  while ((rcode = fgetline(fd, &buf)) != 1) {
    if (rcode == -1) {
      return 1;
    }
    rcode = GRAinputold(info->GC, buf, info->lm, info->tm, info->zmx, info->zmy);
    if (!rcode) {
      error2(info->obj, ERRGRAFM, buf);
      g_free(buf);
      return 1;
    }
    g_free(buf);
  }
  return r;
}

static int
read_gra_file(struct gra_cache *cache, struct gra_info *info, const char *graf, const char *file)
{
  FILE *fd;
  char *buf;
  int newgra, rcode;

  fd = nfopen(file, "rt");
  if (fd == NULL) {
    error2(info->obj, ERROPEN, file);
    return 1;
  }

  rcode = fgetline(fd, &buf);
  if (rcode == 1) {
    error2(info->obj,ERRGRA,file);
  }
  if (rcode != 0) {
    error2(info->obj, ERROPEN, file);
    fclose(fd);
    return 1;
  }
  if (strcmp(graf, buf) == 0) {
    newgra = TRUE;
  } else if (strcmp(OLD_GRA_HEADER, buf) == 0) {
    newgra = FALSE;
  } else {
    error2(info->obj, ERRGRA, file);
    g_free(buf);
    fclose(fd);
    return 1;
  }
  g_free(buf);

  if (newgra) {
    rcode = read_new_gra_file(cache, info, fd);
  } else {
    rcode = read_old_gra_file(cache, info, fd, file);
  }
  fclose(fd);
  return rcode;
}

static int
draw_gra(struct gra_cache *cache, struct gra_info *info)
{
  struct GRAdata *data;

  if (cache == NULL) {
    return 0;
  }

  data = cache->data;
  if (data == NULL) {
    return 0;
  }
  while (data) {
    int rcode;
    rcode = draw_gra_data(info, data);
    if (! rcode) {
      return 1;
    }
    data = data->next;
  }
  return 0;
}

static int
free_cache(struct nhash *hash, void *data)
{
  struct gra_cache *cache;

  cache = hash->val.p;
  if (cache == NULL) {
    return 0;
  }

  gra_data_free(cache->data);
  g_free(cache);
  hash->val.p = NULL;

  return 0;
}

static int
read_gra(struct gra_info *info, const char *graf, const char *file)
{
  struct gra_cache *cache;

  cache = gra_cache_get(file);
  if (cache == NULL) {
    cache = gra_cache_new(file);
    if (cache == NULL) {
      return 1;
    }
  }
  if (info->gstat.st_mtime != cache->gstat.st_mtime ||
      info->gstat.st_size != cache->gstat.st_size) {
    if (read_gra_file(cache, info, graf, file)) {
      gra_cache_free(file);
      return 1;
    }
  } else {
    if (draw_gra(cache, info)) {
      error2(info->obj, ERRGRAFM, file);
      return 1;
    }
  }

  return 0;
}
#endif  /* USE_MERGE_CACHE */

void
merge_cache_clear(void)
{
#if USE_MERGE_CACHE
  if (GraCache == NULL) {
    return;
  }
  nhash_each(GraCache, free_cache, NULL);
  nhash_clear(GraCache);
#endif
}

static int
mergedraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct gra_info info;
  char *file,*graf;
  struct objlist *sys;
  struct mergelocal *mergelocal;
#if ! USE_MERGE_CACHE
  FILE *fd;
  int newgra,rcode;
  char *buf;
#endif

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&info.GC);
  if (info.GC<0) return 0;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"left_margin",inst,&info.lm);
  _getobj(obj,"top_margin",inst,&info.tm);
  _getobj(obj,"zoom_x",inst,&info.zmx);
  _getobj(obj,"zoom_y",inst,&info.zmy);
  _getobj(obj,"_local",inst,&mergelocal);
  info.obj = obj;

  if (file==NULL) {
    error(obj,ERRFILE);
    return 1;
  }
  if (nstat(file, &info.gstat)) {
    clear_bbox(obj, inst);
    error2(obj, ERROPEN, file);
    return 1;
  }
  if (info.gstat.st_mtime != mergelocal->mtime) {
    clear_bbox(obj, inst);
  }
  mergelocal->mtime = info.gstat.st_mtime;
  if ((sys=getobject("system"))==NULL) return 1;
  if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
#if USE_MERGE_CACHE
  read_gra(&info, graf, file);
#else  /* USE_MERGE_CACHE */
  if ((fd=nfopen(file,"rt"))==NULL) {
    error2(obj,ERROPEN,file);
    return 1;
  }

  if ((rcode=fgetline(fd,&buf))==1) error2(obj,ERRGRA,file);
  if (rcode!=0) {
    fclose(fd);
    return 1;
  }
  if (strcmp(graf,buf)==0) newgra=TRUE;
  else if (strcmp(OLD_GRA_HEADER,buf)==0) newgra=FALSE;
  else {
    error2(obj,ERRGRA,file);
    g_free(buf);
    fclose(fd);
    return 1;
  }
  g_free(buf);
  if (!newgra) {
    if ((rcode=fgetline(fd,&buf))==1) error2(obj,ERRGRA,file);
    g_free(buf);
    if (rcode!=0) {
      fclose(fd);
      return 1;
    }
  }
  while ((rcode=fgetline(fd,&buf))!=1) {
    if (rcode==-1) {
      fclose(fd);
      return 1;
    }
    if (newgra) {
      rcode=GRAinput(info.GC, buf, info.lm, info.tm, info.zm);
    } else {
      rcode=GRAinputold(info.GC, buf, info.lm, info.tm, info.zm);
    }
    if (!rcode) {
      error2(obj,ERRGRAFM,buf);
      g_free(buf);
      fclose(fd);
      return 1;
    }
    g_free(buf);
  }
  fclose(fd);
#endif  /* USE_MERGE_CACHE */
  return 0;
}

static int
mergeredraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int hidden;
#if ! USE_MERGE_CACHE
  int redrawf;
#endif

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"hidden",inst,&hidden);

#if USE_MERGE_CACHE
  if (! hidden) {
    mergedraw(obj,inst,rval,argc,argv);
  }
#else  /* USE_MERGE_CACHE */
  _getobj(obj,"redraw_flag",inst,&redrawf);
  if (redrawf) {
    mergedraw(obj,inst,rval,argc,argv);
  } else {
    int GC;
    struct narray *array;
    int x1, x2, y1, y2, zoom, w, h;
    if (! hidden) {
      system_draw_notify();
    }
    _getobj(obj,"GC",inst,&GC);
    if (GC<0) return 0;
    if (_exeobj(obj, "bbox", inst, 0, NULL)) {
      return 1;
    }
    _getobj(obj, "bbox", inst, &array);
    if (array) {
      x1 = arraynget_int(array,0);
      y1 = arraynget_int(array,1);
      x2 = arraynget_int(array,2);
      y2 = arraynget_int(array,3);
      GRAregion(GC, &w, &h, &zoom);
      GRAview(GC, 0, 0, w * 10000.0 / zoom, h * 10000.0 / zoom, 1);
      GRAcolor(GC, 0, 0, 0, 255);
      GRAlinestyle(GC, 0, NULL, 10, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
      GRArectangle(GC, x1, y1, x2, y2, 0);
    }
  }
#endif  /* USE_MERGE_CACHE */
  return 0;
}

static int
mergefile(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
            int argc, char **argv)
{
  struct objlist *sys;
  int ignorepath;
  char *file, *file2;

  sys=getobject("system");
  getobj(sys, "ignore_path", 0, 0, NULL, &ignorepath);

  if (argv[2] == NULL) {
    return 0;
  }

  if (clear_bbox(obj, inst)) {
    /* argv[2] cannot be freed when a field returns TRUE. */
    return 1;
  }

  file = get_utf8_filename(argv[2]);
  if (file == NULL) {
    return 1;
  }

  g_free(argv[2]);
  if (ignorepath) {
    file2 = getbasename(file);
    g_free(file);
    argv[2] = file2;
  } else {
    argv[2] = file;
  }

  return 0;
}

static int
mergetime(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *file;
  GStatBuf buf;
  int style;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (nstat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  rval->str=ntime((time_t *)&(buf.st_mtime),style);
  return 0;
}

static int
mergedate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *file;
  GStatBuf buf;
  int style;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (nstat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  rval->str=ndate((time_t *)&(buf.st_mtime),style);
  return 0;
}

static int
mergestore(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mergelocal *mergelocal;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"_local",inst,&mergelocal);
  if (mergelocal->endstore) {
    mergelocal->endstore=FALSE;
    return 1;
  }
  return store(obj, inst, rval, argc, argv, &mergelocal->endstore, &mergelocal->storefd);
}

static int
mergeload(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *s;
  int len;
  char *file,*fullname,*oldfile;
  time_t ftime;
  int mkdata;
  struct utimbuf tm;

  s=(char *)argv[2];
  if ((file=getitok2(&s,&len," \t"))==NULL) return 1;
  if ((fullname=getfullpath(file))==NULL) {
    g_free(file);
    return 1;
  }
  _getobj(obj,"file",inst,&oldfile);
  g_free(oldfile);
  _putobj(obj,"file",inst,fullname);
  if (gettimeval(s,&ftime)) {
    g_free(file);
    return 1;
  }
  if (naccess(file,R_OK)!=0) mkdata=TRUE;
  else {
    char *mes;
    if ((mes=g_malloc(strlen(file)+256))==NULL) {
      g_free(file);
      return 1;
    }
    sprintf(mes,"`%s' Overwrite existing file?",file);
    mkdata=inputyn(mes);
    g_free(mes);
  }
  if (mkdata) {
    char buf[2];
    FILE *fp;
    if ((fp=nfopen(file,"wt"))==NULL) {
      error2(obj,ERROPEN,file);
      g_free(file);
      return 1;
    }
    while (nread(stdinfd(),buf,1)==1) fputc(buf[0],fp);
    fclose(fp);
    tm.actime=ftime;
    tm.modtime=ftime;
    utime(file,&tm);
  }
  g_free(file);
  return 0;
}

static int
mergestoredum(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mergelocal *mergelocal;
  char *file,*base,*date,*time;
  int style;
  char *buf;
  char *argv2[2];

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"_local",inst,&mergelocal);
  if (mergelocal->endstore) {
    mergelocal->endstore=FALSE;
    return 1;
  }
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 1;
  style=3;
  argv2[0]=(char *)&style;
  argv2[1]=NULL;
  if (_exeobj(obj,"date",inst,1,argv2)) return 1;
  style=0;
  argv2[0]=(char *)&style;
  argv2[1]=NULL;
  if (_exeobj(obj,"time",inst,1,argv2)) return 1;
  _getobj(obj,"date",inst,&date);
  if(date == NULL) {
    date = "1-1-1970";
  }
  _getobj(obj,"time",inst,&time);
  if(time == NULL) {
    time = "00:00:00";
  }
  if ((base=getbasename(file))==NULL) return 1;
  buf = g_strdup_printf("merge::load_dummy '%s' '%s %s'\n", base, date, time);
  g_free(base);
  if (buf == NULL) {
    return 1;
  }
  rval->str=buf;
  mergelocal->endstore=TRUE;
  return 0;
}

static int
mergeloaddum(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *s;
  int len;
  char *file,*fullname,*oldfile;

  s=(char *)argv[2];
  if ((file=getitok2(&s,&len," \t"))==NULL) return 1;
  if ((fullname=getfullpath(file))==NULL) {
    g_free(file);
    return 1;
  }
  _getobj(obj,"file",inst,&oldfile);
  g_free(oldfile);
  _putobj(obj,"file",inst,fullname);
  return 0;
}

static int
mergebbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char *file,*graf;
  int lm,tm,zmx,zmy;
  int newgra,rcode;
  struct objlist *sys;
  FILE *fd;
  char *buf;
  struct GRAbbox bbox;
  int GC;
  struct mergelocal *mergelocal;

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  _getobj(obj,"zoom_x",inst,&zmx);
  _getobj(obj,"zoom_y",inst,&zmy);
  if (file==NULL) return 1;
  if ((sys=getobject("system"))==NULL) return 1;
  if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
  if ((fd=nfopen(file,"rt"))==NULL) return 1;
  if ((rcode=fgetline(fd,&buf))!=0) {
    fclose(fd);
    return 1;
  }
  if (strcmp(graf,buf)==0) newgra=TRUE;
  else if (strcmp(OLD_GRA_HEADER,buf)==0) newgra=FALSE;
  else {
    g_free(buf);
    fclose(fd);
    return 1;
  }
  g_free(buf);
  if (!newgra) {
    if ((rcode=fgetline(fd,&buf))!=0) {
      fclose(fd);
      return 1;
    }
    g_free(buf);
  }
  GRAinitbbox(&bbox);
  if ((GC=_GRAopencallback(GRAboundingbox,NULL,&bbox))==-1) {
    GRAendbbox(&bbox);
    fclose(fd);
    return 1;
  }
  GRAinit(GC,0,0,21000,29300,10000);
  while ((rcode=fgetline(fd,&buf))!=1) {
    if (rcode==-1) {
      fclose(fd);
      return 1;
    }
    if (newgra) rcode=GRAinput(GC,buf,0,0,10000,10000);
    else rcode=GRAinputold(GC,buf,0,0,10000,10000);
    if (!rcode) {
      g_free(buf);
      fclose(fd);
      return 1;
    }
    g_free(buf);
  }
  fclose(fd);
  _GRAclose(GC);
  GRAendbbox(&bbox);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  arrayins(array,&bbox.maxy,0);
  arrayins(array,&bbox.maxx,0);
  arrayins(array,&bbox.miny,0);
  arrayins(array,&bbox.minx,0);
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  _getobj(obj,"_local",inst,&mergelocal);
  mergelocal->bbox[0] = bbox.minx;
  mergelocal->bbox[1] = bbox.miny;
  mergelocal->bbox[2] = bbox.maxx;
  mergelocal->bbox[3] = bbox.maxy;
  set_bbox(obj, inst, array, lm, tm, zmx, zmy);
  rval->array=array;
  return 0;
}

static int
mergemove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int lm, tm, zmx, zmy, x, y;
  struct narray *array;

  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  x = *(int *)argv[2];
  y = *(int *)argv[3];
  lm += x;
  tm += y;
  if (_putobj(obj,"left_margin",inst,&lm)) return 1;
  if (_putobj(obj,"top_margin",inst,&tm)) return 1;

#if 0
  if (clear_bbox(obj, inst))
    return 1;
#else
  _getobj(obj, "bbox", inst, &array);
  if (arraynum(array) == 4) {
    _getobj(obj, "zoom_x", inst, &zmx);
    _getobj(obj, "zoom_y", inst, &zmy);
    set_bbox(obj, inst, array, lm, tm, zmx, zmy);
  }
#endif

  return 0;
}

#define ZOOM_MAX 1000000

static int
mergezoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int lm,tm,zmx,zmy;
  double zoom_x, zoom_y;
  int refx,refy;
  struct narray *array;

  zoom_x=(*(int *)argv[2])/10000.0;
  zoom_y=(*(int *)argv[3])/10000.0;
  refx=(*(int *)argv[4]);
  refy=(*(int *)argv[5]);
  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  _getobj(obj,"zoom_x",inst,&zmx);
  _getobj(obj,"zoom_y",inst,&zmy);
  lm=(lm-refx)*zoom_x+refx;
  tm=(tm-refy)*zoom_y+refy;

  if (zmx * zoom_x > ZOOM_MAX) {
    return 0;
  }

  if (zmy * zoom_y > ZOOM_MAX) {
    return 0;
  }

  zmx*=zoom_x;
  zmy*=zoom_y;
  if (_putobj(obj,"left_margin",inst,&lm)) return 1;
  if (_putobj(obj,"top_margin",inst,&tm)) return 1;
  if (_putobj(obj,"zoom_x",inst,&zmx)) return 1;
  if (_putobj(obj,"zoom_y",inst,&zmy)) return 1;

#if 0
  if (clear_bbox(obj, inst))
    return 1;
#else
  _getobj(obj, "bbox", inst, &array);
  if (arraynum(array) == 4) {
    set_bbox(obj, inst, array, lm, tm, zmx, zmy);
  }
#endif

  return 0;
}

static int
mergematch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;

  rval->i=FALSE;
  if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
  _getobj(obj,"bbox",inst,&array);
  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if (arraynum(array)<4) return 1;
  bminx=arraynget_int(array,0);
  bminy=arraynget_int(array,1);
  bmaxx=arraynget_int(array,2);
  bmaxy=arraynget_int(array,3);
  if ((minx==maxx) && (miny==maxy)) {
    bminx-=err;
    bminy-=err;
    bmaxx+=err;
    bmaxy+=err;
    if ((bminx<=minx) && (minx<=bmaxx)
     && (bminy<=miny) && (miny<=bmaxy)) rval->i=TRUE;
  } else {
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  return 0;
}

static int
mergegeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  char *field;
  int zoom, val;
  struct narray *array;

  field=(char *)(argv[1]);
  if (field[0] == 'z') {
    zoom = * (int *) (argv[2]);
    if (zoom > ZOOM_MAX) {
      zoom = ZOOM_MAX;
    } else if (zoom < 1) {
      zoom = 1;
    }
    if (strcmp(field, "zoom") == 0) {
      _putobj(obj, "zoom_x", inst, &zoom);
      _putobj(obj, "zoom_y", inst, &zoom);
    } else {
      * (int *) (argv[2]) = zoom;
    }
  }

#if 0
  if (clear_bbox(obj, inst)){
    return 1;
  }
#else
  val = * (int *) (argv[2]);
  _getobj(obj, "bbox", inst, &array);
  if (arraynum(array) == 4) {
    int t, l, zx, zy;
    _getobj(obj, "top_margin", inst, &t);
    _getobj(obj, "left_margin", inst, &l);
    _getobj(obj, "zoom_x", inst, &zx);
    _getobj(obj, "zoom_y", inst, &zy);
    switch (field[0]) {
    case 't':
      t = val;
      break;
    case 'l':
      l = val;
      break;
    case 'z':
      if (strcmp(field, "zoom") == 0) {
	zx = zoom;
	zy = zoom;
      } else if (strcmp(field, "zoom_x") == 0) {
	zx = zoom;
      } else {
	zy = zoom;
      }
      break;
    }
    set_bbox(obj, inst, array, l, t, zx, zy);
  }
#endif

  return 0;
}

static int
merge_inst_dup(struct objlist *obj, N_VALUE *src, N_VALUE *dest)
{
  int i;

  i = obj_get_field_pos(obj, "_local");
  if (i < 0) {
    return 1;
  }
#if GLIB_CHECK_VERSION(2, 68, 0)
  dest[i].ptr = g_memdup2(src[i].ptr, sizeof(struct mergelocal));
#else
  dest[i].ptr = g_memdup(src[i].ptr, sizeof(struct mergelocal));
#endif
  /*   mergelocal->storefd may be NULL */
  if (dest[i].ptr == NULL) {
    return 1;
  }
  return 0;
}

static int
merge_inst_free(struct objlist *obj, N_VALUE *inst)
{
  int i;

  i = obj_get_field_pos(obj, "_local");
  if (i == -1) {
      return 1;
  }
  g_free(inst[i].ptr);
  /* mergelocal->storefd may be NULL */
  return 0;
}

static struct objtable merge[] = {
  {"init",NVFUNC,NEXEC,mergeinit,NULL,0},
  {"done",NVFUNC,NEXEC,mergedone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"file",NSTR,NREAD|NWRITE,mergefile,NULL,0},
  {"save_path",NENUM,NREAD|NWRITE,NULL,pathchar,0},
  {"top_margin",NINT,NREAD|NWRITE,mergegeometry,NULL,0},
  {"left_margin",NINT,NREAD|NWRITE,mergegeometry,NULL,0},
  {"zoom_x",NINT,NREAD|NWRITE,mergegeometry,NULL,0},
  {"zoom_y",NINT,NREAD|NWRITE,mergegeometry,NULL,0},
  {"zoom",NINT,NWRITE,mergegeometry,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,mergedraw,"i",0},
  {"redraw",NVFUNC,NREAD|NEXEC,mergeredraw,"i",0},
  {"save",NSFUNC,NREAD|NEXEC,pathsave,"sa",0},
  {"store_data",NSFUNC,NREAD|NEXEC,mergestore,"",0},
  {"load_data",NVFUNC,NREAD|NEXEC,mergeload,"s",0},
  {"store_dummy",NSFUNC,NREAD|NEXEC,mergestoredum,"",0},
  {"load_dummy",NVFUNC,NREAD|NEXEC,mergeloaddum,"s",0},
  {"time",NSFUNC,NREAD|NEXEC,mergetime,"i",0},
  {"date",NSFUNC,NREAD|NEXEC,mergedate,"i",0},
  {"line_num",NINT,NREAD,NULL,NULL,0},

  {"bbox",NIAFUNC,NREAD|NEXEC,mergebbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,mergemove,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,mergezoom,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,mergematch,"iiiii",0},

  {"ext",NSTR,NREAD,NULL,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},

  {"symbol_greek",NBOOL,NWRITE,NULL,NULL,0},
};

#define TBLNUM (sizeof(merge) / sizeof(*merge))

void *
addmerge(void)
/* addmerge() returns NULL on error */
{
  struct objlist *merge_obj;
  merge_obj = addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,merge,ERRNUM,mergeerrorlist,NULL,NULL);
  obj_set_undo_func(merge_obj, merge_inst_dup, merge_inst_free);
  return merge_obj;
}
