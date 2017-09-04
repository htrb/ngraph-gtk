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
#include "odraw.h"

#define NAME "merge"
#define PARENT "draw"
#define OVERSION  "1.00.00"

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
};

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

  if (_putobj(obj,"zoom",inst,&zm)) return 1;
  if (_putobj(obj,"line_num",inst,&n)) return 1;
  if (_putobj(obj,"ext",inst,ext)) return 1;
  if ((mergelocal=g_malloc(sizeof(struct mergelocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,mergelocal)) goto errexit;

  mergelocal->storefd=NULL;
  mergelocal->endstore=FALSE;
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

static int
mergedraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  char *file,*graf;
  struct objlist *sys;
  FILE *fd;
  char *buf;
  int lm,tm,zm;
  int newgra,rcode,line = 0;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  _getobj(obj,"zoom",inst,&zm);

  if (file==NULL) {
    error(obj,ERRFILE);
    return 1;
  }
  if ((sys=getobject("system"))==NULL) return 1;
  if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
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
  else if (strcmp(" Ngraph GRA file",buf)==0) newgra=FALSE;
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
    if (newgra) rcode=GRAinput(GC,buf,lm,tm,zm);
    else rcode=GRAinputold(GC,buf,lm,tm,zm);
    if (!rcode) {
      error2(obj,ERRGRAFM,buf);
      g_free(buf);
      fclose(fd);
      return 1;
    }
    g_free(buf);
    line++;
  }
  fclose(fd);
  GRAaddlist(GC,obj,inst,(char *)argv[0],"redraw");
  _putobj(obj, "line_num", inst, &line);
  return 0;
}

static int
mergeredraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int redrawf, dmax, line_num;
  int GC;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"redraw_flag",inst,&redrawf);
  _getobj(obj,"redraw_num", inst, &dmax);
  _getobj(obj,"line_num", inst, &line_num);

  if (redrawf && line_num > 0 && (dmax == 0 || line_num < dmax * 10)) {
    mergedraw(obj,inst,rval,argc,argv);
  } else {
    struct narray *array;
    int x1, x2, y1, y2, zoom, w, h;
    _getobj(obj,"GC",inst,&GC);
    if (GC<0) return 0;
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
    GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  }
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

  file = get_utf8_filename(argv[2]);
  g_free(argv[2]);
  if (file == NULL) {
    argv[2] = NULL;
    return 1;
  }

  if (ignorepath) {
    file2 = getbasename(file);
    g_free(file);
    argv[2] = file2;
  } else {
    argv[2] = file;
  }

  if (clear_bbox(obj, inst)) {
    return 1;
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
  } else if (mergelocal->storefd==NULL) {
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
    if ((mergelocal->storefd=nfopen(file,"rt"))==NULL) {
      g_free(base);
      return 1;
    }
    buf = g_strdup_printf("merge::load_data '%s' '%s %s' <<'[EOF]'", base, date, time);
    g_free(base);
    if (buf == NULL) {
      fclose(mergelocal->storefd);
      mergelocal->storefd=NULL;
      return 1;
    }
    rval->str=buf;
    return 0;
  } else {
    if (fgetline(mergelocal->storefd,&buf)!=0) {
      fclose(mergelocal->storefd);
      mergelocal->storefd=NULL;
      buf = g_strdup("[EOF]\n");
      if (buf == NULL) return 1;
      mergelocal->endstore=TRUE;
      rval->str=buf;
      return 0;
    } else {
      rval->str=buf;
      return 0;
    }
  }
}

static int
mergeload(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *s;
  int len;
  char *file,*fullname,*oldfile,*mes;
  time_t ftime;
  int mkdata;
  char buf[2];
  FILE *fp;
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
    if ((mes=g_malloc(strlen(file)+256))==NULL) {
      g_free(file);
      return 1;
    }
    sprintf(mes,"`%s' Overwrite existing file?",file);
    mkdata=inputyn(mes);
    g_free(mes);
  }
  if (mkdata) {
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
  } else {
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
  int lm,tm,zm;
  int newgra,rcode;
  struct objlist *sys;
  FILE *fd;
  char *buf;
  struct GRAbbox bbox;
  int GC;

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  _getobj(obj,"zoom",inst,&zm);
  if (file==NULL) return 1;
  if ((sys=getobject("system"))==NULL) return 1;
  if (getobj(sys,"GRAF",0,0,NULL,&graf)) return 1;
  if ((fd=nfopen(file,"rt"))==NULL) return 1;
  if ((rcode=fgetline(fd,&buf))!=0) {
    fclose(fd);
    return 1;
  }
  if (strcmp(graf,buf)==0) newgra=TRUE;
  else if (strcmp(" Ngraph GRA file",buf)==0) newgra=FALSE;
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
    if (newgra) rcode=GRAinput(GC,buf,lm,tm,zm);
    else rcode=GRAinputold(GC,buf,lm,tm,zm);
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
  rval->array=array;
  return 0;
}

static int
mergemove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int lm,tm;

  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  lm+=*(int *)argv[2];
  tm+=*(int *)argv[3];
  if (_putobj(obj,"left_margin",inst,&lm)) return 1;
  if (_putobj(obj,"top_margin",inst,&tm)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

#define ZOOM_MAX 1000000

static int
mergezoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int lm,tm,zm;
  double zoom;
  int refx,refy;

  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  _getobj(obj,"left_margin",inst,&lm);
  _getobj(obj,"top_margin",inst,&tm);
  _getobj(obj,"zoom",inst,&zm);
  lm=(lm-refx)*zoom+refx;
  tm=(tm-refy)*zoom+refy;

  if (zm * zoom > ZOOM_MAX) {
    return 0;
  }

  zm=zm*zoom;
  if (_putobj(obj,"left_margin",inst,&lm)) return 1;
  if (_putobj(obj,"top_margin",inst,&tm)) return 1;
  if (_putobj(obj,"zoom",inst,&zm)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

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

  field=(char *)(argv[1]);
  if (strcmp(field,"zoom")==0) {
    int *z = (int *) (argv[2]);
    if (*z > ZOOM_MAX) {
      *z = ZOOM_MAX;
    } else if (*z < 1) {
      *z = 1;
    }
  }

  if (clear_bbox(obj, inst)){
    return 1;
  }

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
  dest[i].ptr = g_memdup(src[i].ptr, sizeof(struct mergelocal));
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
  {"zoom",NINT,NREAD|NWRITE,mergegeometry,NULL,0},
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
  {"zooming",NVFUNC,NREAD|NEXEC,mergezoom,"iiii",0},
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
