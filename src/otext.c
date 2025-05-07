/*
 * $Id: otext.c,v 1.19 2010-03-04 08:30:16 hito Exp $
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
#include <math.h>
#include <ctype.h>
#include <glib.h>

#include "object.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"
#include "mathfn.h"
#include "nstring.h"
#include "nconfig.h"
#include "strconv.h"

#define NAME N_("text")
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TEXTCONF "[text]"

#define ERR_INVALID_STR   100

static char *texterrorlist[]={
  "invalid string.",
};

#define ERRNUM (sizeof(texterrorlist) / sizeof(*texterrorlist))

static struct obj_config TextConfig[] = {
  {"space", OBJ_CONFIG_TYPE_NUMERIC},
  {"direction", OBJ_CONFIG_TYPE_NUMERIC},
  {"script_size", OBJ_CONFIG_TYPE_NUMERIC},
  {"raw", OBJ_CONFIG_TYPE_NUMERIC},
};

static NHASH TextConfigHash = NULL;

static int
textloadconfig(struct objlist *obj,N_VALUE *inst)
{
  return obj_load_config(obj, inst, TEXTCONF, TextConfigHash);
}

static int
textsaveconfig(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  return obj_save_config(obj, inst, TEXTCONF, TextConfig, sizeof(TextConfig) / sizeof(*TextConfig));
}

static int
textinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int pt,scriptsize;
  char *font;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  pt=DEFAULT_FONT_PT;
  scriptsize=DEFAULT_SCRIPT_SIZE;
  if (_putobj(obj,"pt",inst,&pt)) return 1;
  if (_putobj(obj,"script_size",inst,&scriptsize)) return 1;

  font = g_strdup(fontchar[0]);
  if (font == NULL) {
    return 1;
  }
  if (_putobj(obj,"font",inst,font)) {
    g_free(font);
    return 1;
  }
  textloadconfig(obj,inst);
  return 0;
}

static int
textdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
textgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  const char *field;

  field=(char *)(argv[1]);
  if (strcmp(field,"pt") == 0) {
    if (*(int *)(argv[2])<TEXT_SIZE_MIN) {
      *(int *)(argv[2])=TEXT_SIZE_MIN;
    }
  } else if (strcmp(field,"script_size")==0) {
    if (*(int *)(argv[2]) < SCRIPT_SIZE_MIN) {
      *(int *)(argv[2])=SCRIPT_SIZE_MIN;
    } else if (*(int *)(argv[2])>SCRIPT_SIZE_MAX) {
      *(int *)(argv[2])=SCRIPT_SIZE_MAX;
    }
  } else if (strcmp(field,"style") == 0) {
    * (int *) (argv[2]) &= (GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC);
  }

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
textdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  int x,y,pt,space,dir,fr,fg,fb,fa,w,h,scriptsize,raw;
  char *font;
  char *text;
  int clip,zoom,style;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"A",inst,&fa);
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"clip",inst,&clip);
  _getobj(obj,"style",inst,&style);
  GRAregion(GC,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb, fa);
  GRAmoveto(GC,x,y);
  if (raw) {
    GRAdrawtextraw(GC,text,font,style,pt,space,dir);
  } else {
    GRAdrawtext(GC,text,font,style,pt,space,dir,scriptsize);
  }
  return 0;
}

static int
textprintf(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  char **argv2;
  int argc2;
  char *format;
  int po, arg, i, r;
  GString *ret;
  char *arg_str;

  g_free(rval->str);
  rval->str=NULL;
  array=(struct narray *)argv[2];
  argv2=arraydata(array);
  argc2=arraynum(array);
  if (argc2<1) return 0;
  format=argv2[0];

  ret = g_string_sized_new(64);
  if (ret == NULL) {
    return 1;
  }

  po=0;
  arg=1;
  while (format[po]!='\0') {
    int quote;
    quote=FALSE;
    for (i=po;(quote || (format[i]!='%')) && (format[i]!='\0');i++) {
      if (quote) {
	quote=FALSE;
      } else if (format[i]=='\\') {
	quote=TRUE;
      }
    }
    if (i > po) {
      g_string_append_len(ret, format + po, i - po);
    }
    po = i;

    if (format[po] != '%') {
      continue;
    }

    if (format[po + 1] == '%') {
      g_string_append_c(ret, '%');
      po += 2;
      continue;
    }

    arg_str = (arg < argc2 && argv2[arg]) ? argv2[arg] : NULL;
    r = add_printf_formated_str(ret, format + po, arg_str, &i);

    if (r) {
      arg++;
    }

    po += i + 1;
  }

  rval->str = g_string_free(ret, FALSE);
  return 0;
}

static int
textbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;
  int x,y,pt,space,dir,scriptsize,raw,style;
  char *font;
  char *text;
  int bbox[4];

  array=rval->array;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"style",inst,&style);
  text_get_bbox(x, y, text, font, style, pt, dir, space, scriptsize, raw, bbox);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  arrayins(array, bbox + 3, 0);
  arrayins(array, bbox + 2, 0);
  arrayins(array, bbox + 1, 0);
  arrayins(array, bbox, 0);
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array=array;
  return 0;
}

static int
textmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  x+=*(int *)argv[2];
  y+=*(int *)argv[3];
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
textrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int dir, angle, use_pivot;

  _getobj(obj, "direction", inst, &dir);

  angle = *(int *) argv[2];
  use_pivot = * (int *) argv[3];

  dir += angle;
  dir %= 36000;
  if (dir < 0)
    dir += 36000;

  if (use_pivot) {
    int px, py, x, y;

    px = *(int *) argv[4];
    py = *(int *) argv[5];
    _getobj(obj, "x", inst, &x);
    _getobj(obj, "y", inst, &y);
    rotate(px, py, angle, &x, &y);
    _putobj(obj, "x", inst, &x);
    _putobj(obj, "y", inst, &y);
  }

  _putobj(obj, "direction", inst, &dir);

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
textzoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y,pt,space,refx,refy,dir;
  double zoom, zoom_x, zoom_y, zoom_p, zoom_n;

  zoom_x = (*(int *) argv[2]) / 10000.0;
  zoom_y = (*(int *) argv[3]) / 10000.0;
  refx = (*(int *)argv[4]);
  refy = (*(int *)argv[5]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  dir = calc_zoom_direction(dir, zoom_x, zoom_y, &zoom_p, &zoom_n);
  zoom = MIN(zoom_n, zoom_p);
  x=(x-refx)*zoom_x+refx;
  y=(y-refy)*zoom_y+refy;
  pt *= zoom;
  space *= zoom_p;

  if (pt < 1)
    pt = 1;

  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"pt",inst,&pt)) return 1;
  if (_putobj(obj,"space",inst,&space)) return 1;
  if (_putobj(obj,"direction",inst,&dir)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
textmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  struct narray *array;
  int gx0,gy0,gx1,gy1;
  int x,y,pt,space,dir,scriptsize,raw, style;
  char *font;
  char *text;

  rval->i=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"style",inst,&style);
  minx=*(int *)(argv[2]);
  miny=*(int *)(argv[3]);
  maxx=*(int *)(argv[4]);
  maxy=*(int *)(argv[5]);
  err=*(int *)(argv[6]);
  if ((minx==maxx) && (miny==maxy)) {
    int px,py,px2,py2;
    double si,co;
    if (raw) {
      GRAtextextentraw(text,font,style,pt,space,&gx0,&gy0,&gx1,&gy1);
    } else {
      GRAtextextent(text,font,style,pt,space,scriptsize,&gx0,&gy0,&gx1,&gy1,FALSE);
    }

    si=sin(dir/18000.0*MPI);
    co=cos(dir/18000.0*MPI);
    px=minx-x;
    py=miny-y;
    px2=px*co-py*si;
    py2=px*si+py*co;
    if ((px2 >= gx0 - err) &&
	(px2 <= gx1 + err) &&
	(py2 >= gy0 - err) &&
	(py2 <= gy1 + err)) {
      rval->i=TRUE;
    }
  } else {
    int bminx,bminy,bmaxx,bmaxy;
    if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
    _getobj(obj,"bbox",inst,&array);
    if (arraynum(array)<4) return 1;
    bminx=arraynget_int(array,0);
    bminy=arraynget_int(array,1);
    bmaxx=arraynget_int(array,2);
    bmaxy=arraynget_int(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  return 0;
}

static int
text_set_text(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *str, *ptr;
  gsize len;

  str = argv[2];

  if (str == NULL) {
    return 0;
  }

  if (g_utf8_validate(str, -1, NULL)) {
    return textgeometry(obj, inst, rval, argc, argv);
  }

  ptr = sjis_to_utf8(str);
  if (ptr) {
    int r;
    r = textgeometry(obj, inst, rval, argc, argv);
    if (r) {
      g_free(ptr);
    } else {
      g_free(str);
      argv[2] = ptr;
    }
    return r;
  }

  ptr = g_locale_to_utf8(str, -1, NULL, &len, NULL);
  if (ptr) {
    int r;
    r = textgeometry(obj, inst, rval, argc, argv);
    if (r) {
      g_free(ptr);
    } else {
      g_free(str);
      argv[2] = ptr;
    }
    return r;
  }

  error(obj, ERR_INVALID_STR);
  return 1;
}

static struct objtable text_obj[] = {
  {"init",NVFUNC,NEXEC,textinit,NULL,0},
  {"done",NVFUNC,NEXEC,textdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"text",NSTR,NREAD|NWRITE,text_set_text,NULL,0},
  {"x",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"pt",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"font",NSTR,NREAD|NWRITE,textgeometry,NULL,0},
  {"style",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"space",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"direction",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"script_size",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"raw",NBOOL,NREAD|NWRITE,textgeometry,NULL,0},

  {"draw",NVFUNC,NREAD|NEXEC,textdraw,"i",0},

  {"printf",NSFUNC,NREAD|NEXEC,textprintf,"sa",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,textbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,textmove,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,textrotate,"iiii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,textzoom,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,textmatch,"iiiii",0},
  {"save_config",NVFUNC,NREAD|NEXEC,textsaveconfig,"",0},

  {"hsb", NVFUNC, NREAD|NEXEC, put_hsb,"ddd",0},

  /* following fields exist for backward compatibility */
  {"jfont",NSTR,NWRITE,textgeometry,NULL,0},
};

#define TBLNUM (sizeof(text_obj) / sizeof(*text_obj))

void *
addtext()
/* addtext() returns NULL on error */
{
  if (TextConfigHash == NULL) {
    unsigned int i;
    TextConfigHash = nhash_new();
    if (TextConfigHash == NULL)
      return NULL;

    for (i = 0; i < sizeof(TextConfig) / sizeof(*TextConfig); i++) {
      if (nhash_set_ptr(TextConfigHash, TextConfig[i].name, (void *) &TextConfig[i])) {
	nhash_free(TextConfigHash);
	return NULL;
      }
    }
  }

  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,text_obj,ERRNUM,texterrorlist,NULL,NULL);
}
