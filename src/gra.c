/*
 * $Id: gra.c,v 1.31 2010-03-04 08:30:16 hito Exp $
 *
 * This file is part of "Ngraph for X11".
 *
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <glib.h>

#include "object.h"
#include "strconv.h"
#include "nstring.h"
#include "mathfn.h"
#include "gra.h"
#include "ogra.h"
#include "ogra_error.h"

#include "math/math_equation.h"

#define FONT_STYLE_NORMAL '\x11'
#define FONT_STYLE_BOLD   '\x12'
#define FONT_STYLE_ITALIC '\x13'

struct GRAC {
  int open,init;
  const char *objname;
  const char *outputname;
  struct objlist *obj;
  N_VALUE *inst;
  int output,strwidth,charascent,chardescent;
  struct narray **list;
  void *local;
  directfunc direct;

  int viewf;
  int leftm,topm,width,height;
  double zoom;
  int gminx,gminy,gmaxx,gmaxy;
  int cpx,cpy;
  int clip;

  int linef;
  int linedashn;
  int *linedash;
  int linewidth,linecap,linejoin,linemiter;

  int colorf;
  int fr,fg,fb,fa;

  int textf;
  char *textfont;
  int textsize;
  int textdir;
  int textspace;
  int font_style;

  int mergetop,mergeleft,mergezoom;
  int mergefont,mergept,mergesp,mergedir;

#if EXPAND_DOTTED_LINE
  int gdashn;
  int gdashi;
  double gdashlen;
  int gdotf;
  int *gdashlist;
#endif
  clipfunc gclipf;
  transfunc gtransf;
  diffunc gdiff;
  intpfunc gintpf;
  void *gflocal;
  double x0,y0;

  int oldFR,oldFG,oldFB,oldBR,oldBG,oldBB;

};


#if EXPAND_DOTTED_LINE
#define INIT_DASH 0, 0, 0, TRUE, NULL,
#else
#define INIT_DASH
#endif

#define GRAC_INIT_VAL {FALSE, FALSE, NULL, NULL, NULL, NULL,		\
      -1, -1, -1, -1, NULL, NULL, NULL,					\
      FALSE, 0, 0, SHRT_MAX, SHRT_MAX, 1, 0, 0, SHRT_MAX, SHRT_MAX,	\
      0, 0, 1,								\
      FALSE, 0, NULL, 1, 0, 0, 0,					\
      FALSE, 0, 0, 0, 255,						\
      FALSE, NULL, 0, 0, 0, GRA_FONT_STYLE_NORMAL,			\
      0, 0, 10000, 0, 0, 0, 0, 						\
      INIT_DASH								\
      NULL, NULL, NULL, NULL, NULL, 0, 0,				\
      0, 0, 0, 0, 0, 0}


static struct GRAC GRAClist[]= {
  GRAC_INIT_VAL, /*  1 */
  GRAC_INIT_VAL, /*  2 */
  GRAC_INIT_VAL, /*  3 */
  GRAC_INIT_VAL, /*  4 */
  GRAC_INIT_VAL, /*  5 */
  GRAC_INIT_VAL, /*  6 */
  GRAC_INIT_VAL, /*  7 */
  GRAC_INIT_VAL, /*  8 */
  GRAC_INIT_VAL, /*  9 */
  GRAC_INIT_VAL, /* 10 */
  GRAC_INIT_VAL, /* 11 */
};

#define GRAClimit ((int) (sizeof(GRAClist) / sizeof(*GRAClist) - 1))

#if ! CURVE_OBJ_USE_EXPAND_BUFFER
static void GRAcmatchtod(double x,double y,struct cmatchtype *data);
#endif
static int GRAinview(int GC,int x,int y);
static int GRArectclip(int GC,int *x0,int *y0,int *x1,int *y1);
static int GRAlineclip(int GC,int *x0,int *y0,int *x1,int *y1);

int
calc_zoom_direction(int direction, double zx, double zy, double *zp, double *zn)
{
  double x, y, dir;
  int new_dir;
  dir = (direction / 100.0) / 180 * MPI;

  if (zn) {
    x = sin(dir) * zx;
    y = cos(dir) * zy;
    *zn = sqrt(x * x + y * y);
  }

  x = cos(dir) * zx;
  y = sin(dir) * zy;
  if (zp) {
    *zp = sqrt(x * x + y * y);
  }

  if (x == 0) {
    if (y > 0) {
      new_dir = 9000;
    } else {
      new_dir = 27000;
    }
  } else if (x < 0) {
    new_dir = atan(y / x) / MPI * 18000 + 18000;
    new_dir %= 36000;
  } else {
    new_dir = atan(y / x) / MPI * 18000;
  }
  return new_dir;
}

int
_GRAopencallback(directfunc direct,struct narray **list,void *local)
{
  int i;

  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=NULL;
  GRAClist[i].outputname=NULL;
  GRAClist[i].obj=NULL;
  GRAClist[i].inst=NULL;
  GRAClist[i].output=-1;
  GRAClist[i].charascent=-1;
  GRAClist[i].chardescent=-1;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=direct;
  return i;
}

int
_GRAopen(const char *objname, const char *outputname,
	 struct objlist *obj,N_VALUE *inst, int output,
	 int strwidth,int charascent,int chardescent,
	 struct narray **list,void *local)
{
  int i;

  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=(char *) objname;
  GRAClist[i].outputname=(char*) outputname;
  GRAClist[i].obj=obj;
  GRAClist[i].inst=inst;
  GRAClist[i].output=output;
  GRAClist[i].strwidth=strwidth;
  GRAClist[i].charascent=charascent;
  GRAClist[i].chardescent=chardescent;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=NULL;
  return i;
}

int
GRAopen(const char *objname,const char *outputname,
	struct objlist *obj,N_VALUE *inst, int output,
	int strwidth,int charascent,int chardescent,
	struct narray **list,void *local)
{
  int i,GC;

  if (obj!=NULL) {
    if (!chkobjfield(obj,"_GC")) {
      if (_getobj(obj,"_GC",inst,&GC)) return -1;
      if (GC!=-1) return -2;
    }
  }
  for (i=0;i<GRAClimit;i++) if (!(GRAClist[i].open)) break;
  if (i==GRAClimit) return -1;
  GC=i;
  if (obj!=NULL) {
    if (!chkobjfield(obj,"_GC")) {
      if (_putobj(obj,"_GC",inst,&GC)) return -1;
    }
  }
  GRAClist[i]=GRAClist[GRAClimit];
  GRAClist[i].open=TRUE;
  GRAClist[i].init=FALSE;
  GRAClist[i].objname=objname;
  GRAClist[i].outputname=outputname;
  GRAClist[i].obj=obj;
  GRAClist[i].inst=inst;
  GRAClist[i].output=output;
  GRAClist[i].strwidth=strwidth;
  GRAClist[i].charascent=charascent;
  GRAClist[i].chardescent=chardescent;
  GRAClist[i].list=list;
  GRAClist[i].local=local;
  GRAClist[i].direct=NULL;
  return i;
}

int
GRAreopen(int GC)
{
  char code;
  int cpar[6];

  if (GC<0) return ERRILGC;
  if (GC>=GRAClimit) return ERRILGC;
  g_free(GRAClist[GC].linedash);
#if EXPAND_DOTTED_LINE
  g_free(GRAClist[GC].gdashlist);
  GRAClist[GC].gdashlist=NULL;
#endif
  g_free(GRAClist[GC].textfont);
  GRAClist[GC].linedashn=0;
  GRAClist[GC].linedash=NULL;
  GRAClist[GC].textfont=NULL;
  GRAClist[GC].viewf=FALSE;
  GRAClist[GC].linef=FALSE;
  GRAClist[GC].colorf=FALSE;
  GRAClist[GC].textf=FALSE;
  code='I';
  cpar[0]=5;
  cpar[1]=GRAClist[GC].leftm;
  cpar[2]=GRAClist[GC].topm;
  cpar[3]=GRAClist[GC].width;
  cpar[4]=GRAClist[GC].height;
  cpar[5]=nround(GRAClist[GC].zoom*10000);

  if (GRAdraw(GC,code,cpar,NULL))
    return ERROPEN;

  return 0;
}

int
GRAopened(int GC)
{
  if (GC<0) return -1;
  if (GC>=GRAClimit) return -1;
  if (!(GRAClist[GC].open)) return -1;
  return GC;
}

void
_GRAclose(int GC)
{
  if (GC<0) return;
  if (GC>=GRAClimit) return;
  g_free(GRAClist[GC].linedash);
#if EXPAND_DOTTED_LINE
  g_free(GRAClist[GC].gdashlist);
#endif
  g_free(GRAClist[GC].textfont);
  GRAClist[GC]=GRAClist[GRAClimit];
}

void
GRAclose(int GC)
{
  int GC2,i;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  GC2=-1;
  if (GRAClist[GC].obj!=NULL) {
    if (!chkobjfield(GRAClist[GC].obj,"_lock")) {
      _putobj(GRAClist[GC].obj,"_GC",GRAClist[GC].inst,&GC2);
    }
    i=-1;
    if (!chkobjfield(GRAClist[GC].obj,"_GC")) {
      _putobj(GRAClist[GC].obj,"_GC",GRAClist[GC].inst,&i);
    }
  }
  g_free(GRAClist[GC].linedash);
#if EXPAND_DOTTED_LINE
  g_free(GRAClist[GC].gdashlist);
#endif
  g_free(GRAClist[GC].textfont);
  GRAClist[GC]=GRAClist[GRAClimit];
}

static void
GRAaddlist2(int GC,char *draw)
{
  struct narray **array;

  if (GC<0) return;
  if (GC>=GRAClimit) return;
  if (GRAClist[GC].output==-1) return;
  array=GRAClist[GC].list;
  if (array!=NULL) {
    if (*array==NULL) *array=arraynew(sizeof(char *));
    arrayadd(*array,&draw);
  }
}

void
GRAaddlist(int GC,struct objlist *obj,N_VALUE *inst,
                const char *objname, const char *field)
{
  int oid;
  char *draw;

  if (GRAClist[GC].output==-1) return;
  if (_getobj(obj,"oid",inst,&oid)==-1) return;
  if ((draw=mkobjlist(NULL,objname,oid,field,TRUE))==NULL) return;
  GRAaddlist2(GC,draw);
}

static int
add_draw_obj(const struct objlist *parent, char const **objects, int index)
{
  struct objlist *ocur;
  const char *objname;

  ocur = chkobjroot();
  while (ocur != NULL) {
    if (chkobjparent(ocur) == parent) {
      int instnum;
      instnum = chkobjlastinst(ocur);
      if (instnum != -1) {
	objname = chkobjectname(ocur);
        objects[index] = objname;
        index++;
        objects[index] = NULL;
      }
      index = add_draw_obj(ocur, objects, index);
    }
    ocur = ocur->next;
  }
  return index;
}

void
GRAredraw(struct objlist *obj,N_VALUE *inst,int setredrawf,int redraw_num)
{
  struct objlist *gobj;
  int gid, snum;
  char *gfield;
  N_VALUE *ginst;
  char const *objects[OBJ_MAX] = {NULL};
  char **sdata;
  struct narray *sarray, *array;

  if (_getobj(obj, "_list", inst, &sarray)) {
    return;
  }
  snum = arraynum(sarray);
  if (snum == 0) {
    return;
  }
  sdata = arraydata(sarray);
  gobj = getobjlist(sdata[0], &gid, &gfield, NULL);
  if (gobj == NULL) {
    return;
  }
  ginst = getobjinstoid(gobj, gid);
  if (ginst == NULL) {
    return;
  }
  _getobj(gobj, "draw_obj", ginst, &array);
  if (array) {
    int i, n;
    n = arraynum(array);
    for (i = 0; i < n; i++) {
      objects[i] = arraynget_str(array, i);
    }
    objects[i] = NULL;
  } else {
    struct objlist *draw;
    draw = getobject("draw");
    if (draw == NULL) {
      return;
    }
    add_draw_obj(draw, objects, 0);
  }
  GRAredraw_layers(obj,inst, setredrawf, redraw_num, objects);
}

void
GRAredraw_layers(struct objlist *obj, N_VALUE *inst, int setredrawf, int redraw_num, char const **objects)
{
  int i, n, snum;
  char *dargv[2], *device, **sdata;
  int redrawfsave;
  struct objlist *gobj;
  int xid, gid, oid, layer, GC, GCnew;
  char *dfield, *xfield, *gfield;
  N_VALUE *dinst, *ginst;
  struct narray *sarray;

  if (ninterrupt()) return;

  if (_getobj(obj,"_list",inst,&sarray)) return;
  if (_getobj(obj,"oid",inst,&oid)) return;
  if ((snum=arraynum(sarray))==0) return;
  sdata=arraydata(sarray);
  if (_getobj(obj,"_GC",inst,&GC)) {
    return;
  }
  if (((gobj=getobjlist(sdata[0],&gid,&gfield,NULL))==NULL)
  || ((ginst=getobjinstoid(gobj,gid))==NULL)) {
    return;
  }
  if (GC!=-1) {
    /* gra is still opened */
    GCnew=GC;
    GRAreopen(GC);
  } else {
    struct objlist *xobj;
    /* gra is already closed */
    /* check consistency */
    if (_getobj(gobj,"_device",ginst,&device) || (device==NULL)
    || ((xobj=getobjlist(device,&xid,&xfield,NULL))==NULL)
    || (xobj!=obj) || (xid!=oid) || (strcmp(xfield,"_output")!=0)) {
      return;
    }
    /* open GRA */
    if ((_exeobj(gobj,"open",ginst,0,NULL))
    || (_getobj(gobj,"open",ginst,&GCnew))
    || (GRAopened(GCnew)==-1)) {
      return;
    }
  }

  layer = GRAlayer_support(GC);
  dargv[0]=(char *)&GC;
  while (*objects) {
    struct objlist *dobj;
    dobj = getobject(*objects);
    objects++;
    if (dobj == NULL) {
      continue;
    }
    n = chkobjlastinst(dobj) + 1;
    if (n < 1) {
      continue;
    }
    if (layer) {
      GRAlayer(GC, dobj->name);
    }
    for (i = 0; i < n; i++) {
      if (ninterrupt()) {
	return;
      }
      if (chkobjfield(dobj, "file")) {
	dfield = "draw";
      } else {
	dfield = "redraw";
      }
      dinst = chkobjinst(dobj, i);
      if (dinst == NULL) {
	continue;
      }
      if (setredrawf) {
	int t = TRUE;
	_getobj(dobj, "redraw_flag", dinst, &redrawfsave);
	_putobj(dobj, "redraw_flag", dinst, &t);
	_putobj(dobj, "redraw_num", dinst, &redraw_num);
      }
      _exeobj(dobj, dfield, dinst, 1, dargv);
      if (setredrawf) {
	_putobj(dobj, "redraw_flag", dinst, &redrawfsave);
      }
    }
  }
}

int
GRAdraw(int GC,char code,int *cpar,char *cstr)
{
  double zoom;
  int i, zoomf, style, alpha;

  if (GC<0) return ERRILGC;
  if (GC>=GRAClimit) return ERRILGC;
  if (!(GRAClist[GC].open)) return ERRGRACLOSE;
  if ((GRAClist[GC].direct==NULL)
  && ((GRAClist[GC].output==-1) || (GRAClist[GC].obj==NULL))) return ERRNODEVICE;
  zoom=GRAClist[GC].zoom;
  if (zoom==1) zoomf=FALSE;
  else zoomf=TRUE;
  switch (code) {
  case 'I':
    if (GRAClist[GC].init) return 0;
    GRAClist[GC].init=TRUE;
    break;
  case 'V':
    if (GRAClist[GC].viewf
    && (cpar[1]==GRAClist[GC].gminx) && (cpar[3]==GRAClist[GC].gmaxx)
    && (cpar[2]==GRAClist[GC].gminy) && (cpar[4]==GRAClist[GC].gmaxy)
    && (cpar[5]==GRAClist[GC].clip)) return 0;
    GRAClist[GC].viewf=TRUE;
    GRAClist[GC].gminx=cpar[1];
    GRAClist[GC].gminy=cpar[2];
    GRAClist[GC].gmaxx=cpar[3];
    GRAClist[GC].gmaxy=cpar[4];
    GRAClist[GC].clip=cpar[5];
    cpar[1]=cpar[1]*zoom+GRAClist[GC].leftm;
    cpar[2]=cpar[2]*zoom+GRAClist[GC].topm;
    cpar[3]=cpar[3]*zoom+GRAClist[GC].leftm;
    cpar[4]=cpar[4]*zoom+GRAClist[GC].topm;
    break;
  case 'A':
    if (GRAClist[GC].linef
    && (cpar[1]==GRAClist[GC].linedashn) && (cpar[2]==GRAClist[GC].linewidth)
    && (cpar[3]==GRAClist[GC].linecap) && (cpar[4]==GRAClist[GC].linejoin)
    && (cpar[5]==GRAClist[GC].linemiter)) {
      for (i=0;i<cpar[1];i++)
        if (cpar[6+i]!=(GRAClist[GC].linedash)[i]) break;
      if (i==cpar[1]) return 0;
    }
    GRAClist[GC].linef=TRUE;
    GRAClist[GC].linewidth=cpar[2];
    GRAClist[GC].linecap=cpar[3];
    GRAClist[GC].linejoin=cpar[4];
    GRAClist[GC].linemiter=cpar[5];
    GRAClist[GC].linedashn=cpar[1];
    g_free(GRAClist[GC].linedash);
    if (cpar[1]!=0) {
      if ((GRAClist[GC].linedash=g_malloc(sizeof(int)*cpar[1]))!=NULL)
        memcpy(GRAClist[GC].linedash,&(cpar[6]),sizeof(int)*cpar[1]);
    } else GRAClist[GC].linedash=NULL;
    if (zoomf) {
      cpar[2]*=zoom;
      for (i=0;i<cpar[1];i++) cpar[i+5]*=zoom;
    }
    break;
  case 'G':
    alpha = (cpar[0] > 3) ? cpar[4] : 255;
    if (GRAClist[GC].colorf &&
	cpar[1] == GRAClist[GC].fr &&
	cpar[2] == GRAClist[GC].fg &&
	cpar[3] == GRAClist[GC].fb &&
	alpha   == GRAClist[GC].fa) {
      return 0;
    }
    GRAClist[GC].colorf = TRUE;
    GRAClist[GC].fr = cpar[1];
    GRAClist[GC].fg = cpar[2];
    GRAClist[GC].fb = cpar[3];
    GRAClist[GC].fa = alpha;
    break;
  case 'F':
    if ((GRAClist[GC].textfont!=NULL)
    && (strcmp(GRAClist[GC].textfont,cstr)==0)) return 0;
    g_free(GRAClist[GC].textfont);
    if ((GRAClist[GC].textfont=g_malloc(strlen(cstr)+1))==NULL) return 0;
    strcpy(GRAClist[GC].textfont,cstr);
    GRAClist[GC].textf=FALSE;
    break;
  case 'H':
    style = (cpar[0] > 3) ? cpar[4] : GRA_FONT_STYLE_NORMAL;
    if (GRAClist[GC].textf &&
	cpar[1] == GRAClist[GC].textsize &&
	cpar[2] == GRAClist[GC].textspace &&
	cpar[3] == GRAClist[GC].textdir &&
	style   == GRAClist[GC].font_style) {
      return 0;
    }
    GRAClist[GC].textf=TRUE;
    GRAClist[GC].textsize=cpar[1];
    GRAClist[GC].textspace=cpar[2];
    GRAClist[GC].textdir=cpar[3];
    GRAClist[GC].font_style = style;
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
    }
    break;
  case 'M': case 'N': case 'T': case 'P':
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
    }
    break;
  case 'L': case 'B': case 'C':
    if (zoomf) {
      cpar[1]*=zoom;
      cpar[2]*=zoom;
      cpar[3]*=zoom;
      cpar[4]*=zoom;
    }
    break;
  case 'R':
    if (zoomf) for (i=0;i<cpar[1]*2;i++) cpar[i+2]*=zoom;
    break;
  case 'D':
    if (zoomf) for (i=0;i<cpar[1]*2;i++) cpar[i+3]*=zoom;
    break;
  default:
    break;
  }
  if (GRAClist[GC].direct==NULL) {
    char *argv[7];
    argv[0]=(char *) GRAClist[GC].objname;
    argv[1]=(char *) GRAClist[GC].outputname;
    argv[2]=GRAClist[GC].local;
    argv[3]=&code;
    argv[4]=(char *)cpar;
    argv[5]=cstr;
    argv[6]=NULL;
    return __exeobj(GRAClist[GC].obj,GRAClist[GC].output,GRAClist[GC].inst,6,argv);
  } else {
    return GRAClist[GC].direct(code,cpar,cstr,GRAClist[GC].local);
  }
}

static int
GRAstrwidth(const gchar *s, char *font, int style, int size)
{
  char *argv[8];
  int i, idp;

  for (i = GRAClimit - 1; i >= 0; i--) {
    if (GRAopened(i) == i && GRAClist[i].strwidth != -1)
      break;
  }

  if (i == -1) {
    return nround(25.4 / 72000.0 * size * 600);
  }

  argv[0] = (char *) GRAClist[i].objname;
  argv[1] = "_strwidth";
  argv[2] = GRAClist[i].local;
  argv[3] = (char *) s;
  argv[4] = (char *) &size;
  argv[5] = font;
  argv[6] = (char *) &style;
  argv[7] = NULL;

  if (__exeobj(GRAClist[i].obj, GRAClist[i].strwidth, GRAClist[i].inst, 6, argv))
    return nround(25.4 / 72000.0 * size * 600);

  idp = chkobjoffset2(GRAClist[i].obj, GRAClist[i].strwidth);
  return GRAClist[i].inst[idp].i;
}

static int
GRAcharascent(char *font, int style, int size)
{
  char *argv[7];
  int i, idp;

  for (i = GRAClimit - 1; i >= 0; i--) {
    if (GRAopened(i) == i && GRAClist[i].charascent != -1)
      break;
  }
  if (i == -1)
    return nround(25.4 / 72000.0 * size * 563);

  argv[0] = (char *) GRAClist[i].objname;
  argv[1] = "_charascent";
  argv[2] = GRAClist[i].local;
  argv[3] = (char *)&size;
  argv[4] = font;
  argv[5] = (char *) &style;
  argv[6] = NULL;

  if (__exeobj(GRAClist[i].obj, GRAClist[i].charascent, GRAClist[i].inst, 5, argv))
    return nround(25.4 / 72000.0 * size * 563);

  idp = chkobjoffset2(GRAClist[i].obj, GRAClist[i].charascent);
  return GRAClist[i].inst[idp].i;
}

static int
GRAchardescent(char *font,int style,int size)
{
  char *argv[7];
  int i, idp;

  for (i = GRAClimit - 1; i >= 0; i--) {
    if (GRAopened(i) == i && GRAClist[i].chardescent != -1)
      break;
  }
  if (i == -1)
    return nround(25.4 / 72000.0 * size * 250);

  argv[0] = (char *) GRAClist[i].objname;
  argv[1] = "_chardescent";
  argv[2] = GRAClist[i].local;
  argv[3] = (char *)&size;
  argv[4] = font;
  argv[5] = (char *) &style;
  argv[6] = NULL;

  if (__exeobj(GRAClist[i].obj, GRAClist[i].chardescent, GRAClist[i].inst, 5, argv))
    return nround(25.4 / 72000.0 * size * 250);

  idp = chkobjoffset2(GRAClist[i].obj, GRAClist[i].chardescent);
  return GRAClist[i].inst[idp].i;
}

int
GRAinit(int GC,int leftm,int topm,int width,int height,int zoom)
{
  char code;
  int cpar[6], r;

  code='I';
  cpar[0]=5;
  cpar[1]=leftm;
  cpar[2]=topm;
  cpar[3]=width;
  cpar[4]=height;
  cpar[5]=zoom;
  r = GRAdraw(GC,code,cpar,NULL);
  GRAClist[GC].leftm=leftm;
  GRAClist[GC].topm=topm;
  GRAClist[GC].width=width;
  GRAClist[GC].height=height;
  GRAClist[GC].zoom=zoom/10000.0;

  return (r && GRAClist[GC].output != -1) ? ERROPEN: 0;
}

void
GRAregion(int GC, int *width, int *height, int *zoom)
{
  if (width) {
    if (GRAClist[GC].leftm < 0) {
      *width = GRAClist[GC].width;
    } else if (GRAClist[GC].leftm > GRAClist[GC].width) {
      *width = 0;
    } else{
      *width = GRAClist[GC].width - GRAClist[GC].leftm;
    }
  }

  if (height) {
    if (GRAClist[GC].topm < 0) {
      *height = GRAClist[GC].height;
    } else if (GRAClist[GC].topm > GRAClist[GC].height) {
      *height = 0;
    } else{
      *height = GRAClist[GC].height - GRAClist[GC].topm;
    }
  }

  if (zoom) {
    *zoom=GRAClist[GC].zoom*10000;
  }
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void
GRAdirect(int GC,int cpar[])
{
  char code;

  code='X';
  GRAdraw(GC,code,cpar,NULL);
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

int
GRAend(int GC)
{
  char code;
  int cpar[1];

  code='E';
  cpar[0]=0;
  return GRAdraw(GC,code,cpar,NULL);
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void
GRAremark(int GC,char *s)
{
  char code;
  int cpar[1];
  char *cstr;
  char s2[1];

  code='%';
  cpar[0]=-1;
  s2[0]='\0';
  if (s==NULL) cstr=s2;
  else cstr=s;
  GRAdraw(GC,code,cpar,cstr);
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

void
GRAview(int GC,int x1,int y1,int x2,int y2,int clip)
{
  char code;
  int cpar[6];

  if (x1==x2) x2++;
  if (y1==y2) y2++;
  code='V';
  cpar[0]=5;
  cpar[1]=x1;
  cpar[2]=y1;
  cpar[3]=x2;
  cpar[4]=y2;
  cpar[5]=clip;
  GRAdraw(GC,code,cpar,NULL);
}

void
GRAlinestyle(int GC,int num,const int *type,int width,enum GRA_LINE_CAP cap,enum GRA_LINE_JOIN join,
                  int miter)
{
  char code;
  int *cpar;
  int i;

  if ((cpar=g_malloc(sizeof(int)*(6+num)))==NULL) return;
  code='A';
  cpar[0]=5+num;
  cpar[1]=num;
  cpar[2]=width;
  cpar[3]=cap;
  cpar[4]=join;
  cpar[5]=miter;
  for (i=0;i<num;i++) cpar[6+i]=type[i];
  GRAdraw(GC,code,cpar,NULL);
  g_free(cpar);
}

void
GRA_get_linestyle(int GC, int *num, int **type, int *width, enum GRA_LINE_CAP *cap, enum GRA_LINE_JOIN *join, int *miter)
{
  int *linedash;
  size_t n;

  if (GC < 0) {
    return;
  }

  *num   = GRAClist[GC].linedashn;
  *width = GRAClist[GC].linewidth;
  *cap   = GRAClist[GC].linecap;
  *join  = GRAClist[GC].linejoin;
  *miter = GRAClist[GC].linemiter;
  *type = NULL;

  n = *num * sizeof(*linedash);
  if (n == 0 || GRAClist[GC].linedash == NULL) {
    return;
  }
  linedash = g_malloc(n);
  if (linedash == NULL) {
    return;
  }
  memcpy(linedash, GRAClist[GC].linedash, n);
  *type = linedash;
}

void
GRAcolor(int GC, int fr, int fg, int fb, int fa)
{
  char code;
  int cpar[5];

  if (fr > 255) {
    fr = 255;
  } else if (fr < 0) {
    fr = GRAClist[GC].fr;
  }
  if (fg > 255) {
    fg = 255;
  } else if (fg < 0) {
    fg = GRAClist[GC].fg;
  }
  if (fb > 255) {
    fb = 255;
  } else if (fb < 0) {
    fb = GRAClist[GC].fb;
  }
  if (fa > 255) {
    fa = 255;
  } else if (fa < 0) {
    fa = GRAClist[GC].fa;
  }
  code = 'G';
  cpar[0] = 4;
  cpar[1] = fr;
  cpar[2] = fg;
  cpar[3] = fb;
  cpar[4] = fa;
  GRAdraw(GC, code, cpar, NULL);
}

void
GRAtextstyle(int GC,char *font, int style, int size,int space,int dir)
{
  char code;
  int cpar[5];
  char *cstr;

  if (font==NULL) return;
  code='F';
  cpar[0]=-1;
  cstr=font;
  GRAdraw(GC,code,cpar,cstr);
  code='H';
  cpar[0]=4;
  cpar[1]=size;
  cpar[2]=space;
  cpar[3]=dir;
  cpar[4]=style;
  GRAdraw(GC,code,cpar,NULL);
}

void
GRAmoveto(int GC,int x,int y)
{
  char code;
  int cpar[3];

  code='M';
  cpar[0]=2;
  cpar[1]=x;
  cpar[2]=y;
  GRAdraw(GC,code,cpar,NULL);
  GRAClist[GC].cpx=x;
  GRAClist[GC].cpy=y;
}

void
GRAcurrent_point(int GC, int *x, int *y)
{
  *x = GRAClist[GC].cpx;
  *y = GRAClist[GC].cpy;
}

static void
GRAmoverel(int GC,int x,int y)
{
  GRAClist[GC].cpx+=x;
  GRAClist[GC].cpy+=y;
  if ((x!=0) || (y!=0)) {
    char code;
    int cpar[3];
/*    if ((GRAClist[GC].clip==0)
    || (GRAinview(GC,GRAClist[GC].cpx,GRAClist[GC].cpy)==0)) {  */
      code='N';
      cpar[0]=2;
      cpar[1]=x;
      cpar[2]=y;
      GRAdraw(GC,code,cpar,NULL);
/*    }  */
  }
}

void
GRAline(int GC,int x0,int y0,int x1,int y1)
{
  if ((GRAClist[GC].clip==0) || (GRAlineclip(GC,&x0,&y0,&x1,&y1)==0)) {
    char code;
    int cpar[5];
    code='L';
    cpar[0]=4;
    cpar[1]=x0;
    cpar[2]=y0;
    cpar[3]=x1;
    cpar[4]=y1;
    GRAdraw(GC,code,cpar,NULL);
  }
}

void
GRAlineto(int GC,int x,int y)
{
  int x0,y0,x1,y1;

  x0=GRAClist[GC].cpx;
  y0=GRAClist[GC].cpy;
  x1=x;
  y1=y;
  if ((GRAClist[GC].clip==0) || (GRAlineclip(GC,&x0,&y0,&x1,&y1)==0)) {
    char code;
    int cpar[3];
    if ((x0==GRAClist[GC].cpx) && (y0==GRAClist[GC].cpy)) {
      code='T';
      cpar[0]=2;
      cpar[1]=x1;
      cpar[2]=y1;
      GRAdraw(GC,code,cpar,NULL);
    } else {
      code='M';
      cpar[0]=2;
      cpar[1]=x0;
      cpar[2]=y0;
      GRAdraw(GC,code,cpar,NULL);
      code='T';
      cpar[0]=2;
      cpar[1]=x1;
      cpar[2]=y1;
      GRAdraw(GC,code,cpar,NULL);
    }
  }
  GRAClist[GC].cpx=x;
  GRAClist[GC].cpy=y;
}

void
GRAcircle(int GC,int x,int y,int rx,int ry,int cs,int ce,int fil)
{
  char code;
  int cpar[8];

  code='C';
  cpar[0]=7;
  cpar[1]=x;
  cpar[2]=y;
  cpar[3]=rx;
  cpar[4]=ry;
  cpar[5]=cs;
  cpar[6]=ce;
  cpar[7]=fil;
  GRAdraw(GC,code,cpar,NULL);
}

void
GRArectangle(int GC,int x0,int y0,int x1,int y1,int fil)
{
  if ((GRAClist[GC].clip==0) || (GRArectclip(GC,&x0,&y0,&x1,&y1)==0)) {
    char code;
    int cpar[6];
    code='B';
    cpar[0]=5;
    cpar[1]=x0;
    cpar[2]=y0;
    cpar[3]=x1;
    cpar[4]=y1;
    cpar[5]=fil;
    GRAdraw(GC,code,cpar,NULL);
  }
}

static void
GRAputpixel(int GC,int x,int y)
{
  if ((GRAClist[GC].clip==0) || (GRAinview(GC,x,y)==0)) {
    char code;
    int cpar[3];
    code='P';
    cpar[0]=2;
    cpar[1]=x;
    cpar[2]=y;
    GRAdraw(GC,code,cpar,NULL);
  }
}

void
GRAdrawpoly(int GC,int num,const int *point,enum GRA_FILL_MODE fil)
{
  char code;
  int i,*cpar,num2;

  if (num<1) return;
  if ((point[0]!=point[num*2-2]) || (point[1]!=point[num*2-1])) num2=num+1;
  else num2=num;
  if ((cpar=g_malloc(sizeof(int)*(3+2*num2)))==NULL) return;
  code='D';
  cpar[0]=2+2*num2;
  cpar[1]=num2;
  cpar[2]=fil;
  for (i=0;i<2*num;i++) cpar[i+3]=point[i];
  if ((point[0]!=point[num*2-2]) || (point[1]!=point[num*2-1])) {
    cpar[2*num+3]=point[0];
    cpar[2*num+4]=point[1];
  }
  GRAdraw(GC,code,cpar,NULL);
  g_free(cpar);
}

void
GRAlines(int GC,int num,const int *point)
{
  char code;
  int i,*cpar;

  if ((cpar=g_malloc(sizeof(int)*(2+2*num)))==NULL) return;
  code='R';
  cpar[0]=1+2*num;
  cpar[1]=num;
  for (i=0;i<2*num;i++) cpar[i+2]=point[i];
  GRAdraw(GC,code,cpar,NULL);
  g_free(cpar);
}

void
GRArotate(int x0, int y0, const int *pos, int *rpos, int n, double dx, double dy)
{
  int i;
  for (i = 0; i < n; i++) {
    double x, y;
    x = pos[i * 2] - x0;
    y = pos[i * 2 + 1] - y0;
    rpos[i * 2] = nround(dx * x - dy * y) + x0;
    rpos[i * 2 + 1] = nround(dy * x + dx * y) + y0;
  }
}

void
GRArectangle_rotate(int GC,double dx, double dy, int cx, int cy, int x0,int y0,int x1,int y1,int fil)
{
  int po[8];

  if (dy == 0) {
    GRArectangle(GC, x0, y0, x1, y1, fil);
    return;
  }
  po[0]=x0;
  po[1]=y0;
  po[2]=x0;
  po[3]=y1;
  po[4]=x1;
  po[5]=y1;
  po[6]=x1;
  po[7]=y0;
  GRArotate(cx, cy, po, po, 4, dx, dy);
  GRAdrawpoly(GC,4,po,fil);
}

void
GRAmark_rotate(int GC,int type,int x0,int y0, double dx, double dy, int size,
	int fr,int fg,int fb, int fa, int br,int bg,int bb, int ba)
{
  int x1,y1,x2,y2,r;
  int po[12],po2[12],rpo[12],rpo2[12];
  int type2,sgn;
  double d;

  if ((GRAClist[GC].clip!=0) && (GRAinview(GC,x0,y0)!=0)) {
    return;
  }
  switch (type) {
  case 0: case 1: case 2: case 3: case 4:
  case 5: case 6: case 7: case 8: case 9:
    type2=type-0;
    r=size/2;
    if (type2==0) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAcircle(GC,x0,y0,r,r,0,36000,1);
    } else if (type2==2) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
    } else if (type2==5) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAcircle(GC,x0,y0,r,r,0,36000,1);
      r/=2;
      GRAcolor(GC,br,bg,bb, ba);
      GRAcircle(GC,x0,y0,r,r,0,36000,1);
    } else {
      GRAcolor(GC,br,bg,bb, ba);
      GRAcircle(GC,x0,y0,r,r,0,36000,1);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAcircle(GC,x0,y0,r,r,0,36000,0);
      if (type2==3) {
        r/=2;
        GRAcircle(GC,x0,y0,r,r,0,36000,0);
      } else if (type!=1) {
        int ofst;
        if (dy == 0) {
          if (dx >= 0) {
            ofst = 0;
          } else {
            ofst = 18000;
          }
        } else if (dy > 0) {
          ofst = acos(-dx) / MPI * 18000 + 18000;
        } else {
          ofst = acos(dx) / MPI * 18000;
        }
        if (type2==4) {
          r/=2;
          GRAcircle(GC,x0,y0,r,r,0,36000,1);
        } else if (type2==6) {
          GRAcircle(GC,x0,y0,r,r,27000 + ofst,18000,1);
        } else if (type2==7) {
          GRAcircle(GC,x0,y0,r,r,9000 + ofst, 18000,1);
        } else if (type2==8) {
          GRAcircle(GC,x0,y0,r,r,ofst,18000,1);
        } else if (type2==9) {
          GRAcircle(GC,x0,y0,r,r,18000 + ofst, 18000,1);
        }
      }
    }
    break;
  case 10: case 11: case 12: case 13: case 14:
  case 15: case 16: case 17: case 18: case 19:
    type2=type-10;
    x1=x0-size/2;
    y1=y0-size/2;
    x2=x0+size/2;
    y2=y0+size/2;
    if (type2==0) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
    } else if (type2==2) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
    } else if (type2==5) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
      x1=x0-size/4;
      y1=y0-size/4;
      x2=x0+size/4;
      y2=y0+size/4;
      GRAcolor(GC,br,bg,bb, ba);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
    } else {
      GRAcolor(GC,br,bg,bb, ba);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
      GRAcolor(GC,fr,fg,fb, fa);
      GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
      if (type2==3) {
        x1=x0-size/4;
        y1=y0-size/4;
        x2=x0+size/4;
        y2=y0+size/4;
        GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
      } else if (type2!=1) {
        if (type2==4) {
          x1=x0-size/4;
          y1=y0-size/4;
          x2=x0+size/4;
          y2=y0+size/4;
          GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
        } else if (type2==6) {
          x1=x0;
          GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
        } else if (type2==7) {
          x2=x0;
          GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
        } else if (type2==8) {
          y2=y0;
          GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
        } else if (type2==9) {
          y1=y0;
          GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,1);
        }
      }
    }
    break;
  case 20: case 21: case 22: case 23: case 24:
  case 25: case 26: case 27: case 28: case 29:
    type2=type-20;
    po[0]=x0;
    po[1]=y0-size/2;
    po[2]=x0+size/2;
    po[3]=y0;
    po[4]=x0;
    po[5]=y0+size/2;
    po[6]=x0-size/2;
    po[7]=y0;
    po[8]=x0;
    po[9]=y0-size/2;
    GRArotate(x0, y0, po, rpo, 5, dx, dy);
    if (type2==0) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else if (type2==2) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
    } else if (type2==5) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
      po[1]=y0-size/4;
      po[2]=x0+size/4;
      po[5]=y0+size/4;
      po[6]=x0-size/4;
      po[9]=y0-size/4;
      GRArotate(x0, y0, po, rpo, 5, dx, dy);
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
      if (type2==3) {
        po[1]=y0-size/4;
        po[2]=x0+size/4;
        po[5]=y0+size/4;
        po[6]=x0-size/4;
        po[9]=y0-size/4;
        GRArotate(x0, y0, po, rpo, 5, dx, dy);
        GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
      } else if (type2!=1) {
        if (type2==4) {
          po[1]=y0-size/4;
          po[2]=x0+size/4;
          po[5]=y0+size/4;
          po[6]=x0-size/4;
          po[9]=y0-size/4;
          GRArotate(x0, y0, po, rpo, 5, dx, dy);
          GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==6) {
          po[6]=x0;
          po[7]=y0;
          GRArotate(x0, y0, po, rpo, 5, dx, dy);
          GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==7) {
          po[2]=x0;
          po[3]=y0;
          GRArotate(x0, y0, po, rpo, 5, dx, dy);
          GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==8) {
          po[4]=x0;
          po[5]=y0;
          GRArotate(x0, y0, rpo, rpo, 5, dx, dy);
          GRAdrawpoly(GC,5,po,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==9) {
          po[0]=x0;
          po[1]=y0;
          po[8]=x0;
          po[9]=y0;
          GRArotate(x0, y0, po, rpo, 5, dx, dy);
          GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_EVEN_ODD);
        }
      }
    }
    break;
  case 30: case 31: case 32: case 33: case 34:
  case 35: case 36: case 37:
  case 40: case 41: case 42: case 43: case 44:
  case 45: case 46: case 47:
    if (type>=40) {
      type2=type-40;
      sgn=-1;
    } else {
      type2=type-30;
      sgn=1;
    }
    d=sqrt(3.0);
    po[0]=x0;
    po[1]=y0-sgn*size/2;
    po[2]=x0+size*d/4;
    po[3]=y0+sgn*size/4;
    po[4]=x0-size*d/4;
    po[5]=y0+sgn*size/4;
    po[6]=x0;
    po[7]=y0-sgn*size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    if (type2==0) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else if (type2==2) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
    } else if (type2==5) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      po[1]=y0-sgn*size/d/2;
      po[2]=x0+size/4;
      po[3]=y0+sgn*size/d/4;
      po[4]=x0-size/4;
      po[5]=y0+sgn*size/d/4;
      po[7]=y0-sgn*size/d/2;
      GRArotate(x0, y0, po, rpo, 4, dx, dy);
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      if (type2==3) {
        po[1]=y0-sgn*size/d/2;
        po[2]=x0+size/4;
        po[3]=y0+sgn*size/d/4;
        po[4]=x0-size/4;
        po[5]=y0+sgn*size/d/4;
        po[7]=y0-sgn*size/d/2;
        GRArotate(x0, y0, po, rpo, 4, dx, dy);
        GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      } else if (type2!=1) {
        if (type2==4) {
          po[1]=y0-sgn*size/d/2;
          po[2]=x0+size/4;
          po[3]=y0+sgn*size/d/4;
          po[4]=x0-size/4;
          po[5]=y0+sgn*size/d/4;
          po[7]=y0-sgn*size/d/2;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==6) {
          po[4]=x0;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==7) {
          po[2]=x0;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        }
      }
    }
    break;
  case 50: case 51: case 52: case 53: case 54:
  case 55: case 56: case 57:
  case 60: case 61: case 62: case 63: case 64:
  case 65: case 66: case 67:
    if (type>=60) {
      type2=type-60;
      sgn=-1;
    } else {
      type2=type-50;
      sgn=1;
    }
    d=sqrt(3.0);
    po[0]=x0-sgn*size/2;
    po[1]=y0;
    po[2]=x0+sgn*size/4;
    po[3]=y0+size*d/4;
    po[4]=x0+sgn*size/4;
    po[5]=y0-size*d/4;
    po[6]=x0-sgn*size/2;
    po[7]=y0;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    if (type2==0) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else if (type2==2) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
    } else if (type2==5) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      po[0]=x0-sgn*size/d/2;
      po[2]=x0+sgn*size/d/4;
      po[3]=y0+size/4;
      po[4]=x0+sgn*size/d/4;
      po[5]=y0-size/4;
      po[6]=x0-sgn*size/d/2;
      GRArotate(x0, y0, po, rpo, 4, dx, dy);
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
    } else {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      if (type2==3) {
        po[0]=x0-sgn*size/d/2;
        po[2]=x0+sgn*size/d/4;
        po[3]=y0+size/4;
        po[4]=x0+sgn*size/d/4;
        po[5]=y0-size/4;
        po[6]=x0-sgn*size/d/2;
        GRArotate(x0, y0, po, rpo, 4, dx, dy);
        GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      } else if (type2!=1) {
        if (type2==4) {
          po[0]=x0-sgn*size/d/2;
          po[2]=x0+sgn*size/d/4;
          po[3]=y0+size/4;
          po[4]=x0+sgn*size/d/4;
          po[5]=y0-size/4;
          po[6]=x0-sgn*size/d/2;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==6) {
          po[5]=y0;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        } else if (type2==7) {
          po[3]=y0;
          GRArotate(x0, y0, po, rpo, 4, dx, dy);
          GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
        }
      }
    }
    break;
  case 38: case 39: case 48: case 49:
    po[0]=x0;
    po[1]=y0;
    po[2]=x0-size/2;
    po[3]=y0-size/2;
    po[4]=x0+size/2;
    po[5]=y0-size/2;
    po[6]=x0;
    po[7]=y0;
    po2[0]=x0;
    po2[1]=y0;
    po2[2]=x0-size/2;
    po2[3]=y0+size/2;
    po2[4]=x0+size/2;
    po2[5]=y0+size/2;
    po2[6]=x0;
    po2[7]=y0;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRArotate(x0, y0, po2, rpo2, 4, dx, dy);
    if (type==38) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
    } else if (type==39) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    } else if (type==48) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    } else if (type==49) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    }
    break;
  case 58: case 59: case 68: case 69:
    po[0]=x0;
    po[1]=y0;
    po[2]=x0+size/2;
    po[3]=y0-size/2;
    po[4]=x0+size/2;
    po[5]=y0+size/2;
    po[6]=x0;
    po[7]=y0;
    po2[0]=x0;
    po2[1]=y0;
    po2[2]=x0-size/2;
    po2[3]=y0-size/2;
    po2[4]=x0-size/2;
    po2[5]=y0+size/2;
    po2[6]=x0;
    po2[7]=y0;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRArotate(x0, y0, po2, rpo2, 4, dx, dy);
    if (type==58) {
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
    } else if (type==59) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    } else if (type==68) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    } else if (type==69) {
      GRAcolor(GC,br,bg,bb, ba);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_EVEN_ODD);
      GRAcolor(GC,fr,fg,fb, fa);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_EVEN_ODD);
      GRAdrawpoly(GC,4,rpo,GRA_FILL_MODE_NONE);
      GRAdrawpoly(GC,4,rpo2,GRA_FILL_MODE_NONE);
    }
    break;
  case 70:
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/2;
    po[3] = y0;
    po[4] = x0;
    po[5] = y0-size/2;
    po[6] = x0;
    po[7] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,rpo[0],rpo[1],rpo[2],rpo[3]);
    GRAline(GC,rpo[4],rpo[5],rpo[6],rpo[7]);
    break;
  case 71:
    d=sqrt(2.0);
    po[0] = x0-size/d/2;
    po[1] = y0-size/d/2;
    po[2] = x0+size/d/2;
    po[3] = y0+size/d/2;
    po[4] = x0+size/d/2;
    po[5] = y0-size/d/2;
    po[6] = x0-size/d/2;
    po[7] = y0+size/d/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,rpo[0],rpo[1],rpo[2],rpo[3]);
    GRAline(GC,rpo[4],rpo[5],rpo[6],rpo[7]);
    break;
  case 72:
    d=sqrt(3.0);
    po[0] = x0;
    po[1] = y0+size/2;
    po[2] = x0;
    po[3] = y0-size/2;
    po[4] = x0+size*d/4;
    po[5] = y0-size/4;
    po[6] = x0-size*d/4;
    po[7] = y0+size/4;
    po[8] = x0-size*d/4;
    po[9] = y0-size/4;
    po[10] = x0+size*d/4;
    po[11] = y0+size/4;
    GRArotate(x0, y0, po, rpo, 6, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,rpo[0],rpo[1],rpo[2],rpo[3]);
    GRAline(GC,rpo[4],rpo[5],rpo[6],rpo[7]);
    GRAline(GC,rpo[8],rpo[9],rpo[10],rpo[11]);
    break;
  case 73:
    d=sqrt(3.0);
    po[0] = x0+size/2;
    po[1] = y0;
    po[2] = x0-size/2;
    po[3] = y0;
    po[4] = x0-size/4;
    po[5] = y0+size*d/4;
    po[6] = x0+size/4;
    po[7] = y0-size*d/4;
    po[8] = x0-size/4;
    po[9] = y0-size*d/4;
    po[10] = x0+size/4;
    po[11] = y0+size*d/4;
    GRArotate(x0, y0, po, rpo, 6, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,rpo[0],rpo[1],rpo[2],rpo[3]);
    GRAline(GC,rpo[4],rpo[5],rpo[6],rpo[7]);
    GRAline(GC,rpo[8],rpo[9],rpo[10],rpo[11]);
    break;
  case 74:
    d=sqrt(3.0);
    po[0] = x0;
    po[1] = y0-size/2;
    po[2] = x0-size*d/4;
    po[3] = y0+size/4;
    po[4] = x0+size*d/4;
    po[5] = y0+size/4;
    GRArotate(x0, y0, po, rpo, 3, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,x0,y0,rpo[0],rpo[1]);
    GRAline(GC,x0,y0,rpo[2],rpo[3]);
    GRAline(GC,x0,y0,rpo[4],rpo[5]);
    break;
  case 75:
    d=sqrt(3.0);
    po[0] = x0;
    po[1] = y0+size/2;
    po[2] = x0-size*d/4;
    po[3] = y0-size/4;
    po[4] = x0+size*d/4;
    po[5] = y0-size/4;
    GRArotate(x0, y0, po, rpo, 3, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,x0,y0,rpo[0],rpo[1]);
    GRAline(GC,x0,y0,rpo[2],rpo[3]);
    GRAline(GC,x0,y0,rpo[4],rpo[5]);
    break;
  case 76:
    d=sqrt(3.0);
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/4;
    po[3] = y0-size*d/4;
    po[4] = x0+size/4;
    po[5] = y0+size*d/4;
    GRArotate(x0, y0, po, rpo, 3, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,x0,y0,rpo[0],rpo[1]);
    GRAline(GC,x0,y0,rpo[2],rpo[3]);
    GRAline(GC,x0,y0,rpo[4],rpo[5]);
    break;
  case 77:
    d=sqrt(3.0);
    po[0] = x0+size/2;
    po[1] = y0;
    po[2] = x0-size/4;
    po[3] = y0-size*d/4;
    po[4] = x0-size/4;
    po[5] = y0+size*d/4;
    GRArotate(x0, y0, po, rpo, 3, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAline(GC,x0,y0,rpo[0],rpo[1]);
    GRAline(GC,x0,y0,rpo[2],rpo[3]);
    GRAline(GC,x0,y0,rpo[4],rpo[5]);
    break;
  case 78:
    GRAcolor(GC,fr,fg,fb, fa);
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/2;
    po[3] = y0;
    GRArotate(x0, y0, po, rpo, 2, dx, dy);
    GRAline(GC,x0-size/2,y0,x0+size/2,y0);
    break;
  case 79:
    GRAcolor(GC,fr,fg,fb, fa);
    po[0] = x0;
    po[1] = y0-size/2;
    po[2] = x0;
    po[3] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 2, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    break;
  case 80:
    r=size/2;
    GRAcolor(GC,fr,fg,fb, fa);
    GRAcircle(GC,x0,y0,r,r,0,36000,0);
    r/=2;
    GRAcircle(GC,x0,y0,r,r,0,36000,0);
    break;
  case 81:
    r=size/2;
    GRAcolor(GC,fr,fg,fb, fa);
    GRAcircle(GC,x0,y0,r,r,0,36000,0);
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/2;
    po[3] = y0;
    po[4] = x0;
    po[5] = y0-size/2;
    po[6] = x0;
    po[7] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  case 82:
    r=size/2;
    d=sqrt(2.0);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAcircle(GC,x0,y0,r,r,0,36000,0);
    po[0] = x0-size/d/2;
    po[1] = y0-size/d/2;
    po[2] = x0+size/d/2;
    po[3] = y0+size/d/2;
    po[4] = x0-size/d/2;
    po[5] = y0+size/d/2;
    po[6] = x0+size/d/2;
    po[7] = y0-size/d/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  case 83:
    x1=x0-size/2;
    y1=y0-size/2;
    x2=x0+size/2;
    y2=y0+size/2;
    GRAcolor(GC,fr,fg,fb, fa);
    GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
    x1=x0-size/4;
    y1=y0-size/4;
    x2=x0+size/4;
    y2=y0+size/4;
    GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
    break;
  case 84:
    x1=x0-size/2;
    y1=y0-size/2;
    x2=x0+size/2;
    y2=y0+size/2;
    GRAcolor(GC,fr,fg,fb, fa);
    GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/2;
    po[3] = y0;
    po[4] = x0;
    po[5] = y0-size/2;
    po[6] = x0;
    po[7] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  case 85:
    x1=x0-size/2;
    y1=y0-size/2;
    x2=x0+size/2;
    y2=y0+size/2;
    GRAcolor(GC,fr,fg,fb, fa);
    GRArectangle_rotate(GC,dx,dy,x0,y0,x1,y1,x2,y2,0);
    po[0] = x0-size/2;
    po[1] = y0-size/2;
    po[2] = x0+size/2;
    po[3] = y0+size/2;
    po[4] = x0+size/2;
    po[5] = y0-size/2;
    po[6] = x0-size/2;
    po[7] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  case 86:
    po[0]=x0;
    po[1]=y0-size/2;
    po[2]=x0+size/2;
    po[3]=y0;
    po[4]=x0;
    po[5]=y0+size/2;
    po[6]=x0-size/2;
    po[7]=y0;
    po[8]=x0;
    po[9]=y0-size/2;
    GRArotate(x0, y0, po, rpo, 5, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
    po[1]=y0-size/4;
    po[2]=x0+size/4;
    po[5]=y0+size/4;
    po[6]=x0-size/4;
    po[9]=y0-size/4;
    GRArotate(x0, y0, po, rpo, 5, dx, dy);
    GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
    break;
  case 87:
    po[0]=x0;
    po[1]=y0-size/2;
    po[2]=x0+size/2;
    po[3]=y0;
    po[4]=x0;
    po[5]=y0+size/2;
    po[6]=x0-size/2;
    po[7]=y0;
    po[8]=x0;
    po[9]=y0-size/2;
    GRArotate(x0, y0, po, rpo, 5, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
    po[0] = x0-size/2;
    po[1] = y0;
    po[2] = x0+size/2;
    po[3] = y0;
    po[4] = x0;
    po[5] = y0-size/2;
    po[6] = x0;
    po[7] = y0+size/2;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  case 88:
    po[0]=x0;
    po[1]=y0-size/2;
    po[2]=x0+size/2;
    po[3]=y0;
    po[4]=x0;
    po[5]=y0+size/2;
    po[6]=x0-size/2;
    po[7]=y0;
    po[8]=x0;
    po[9]=y0-size/2;
    GRArotate(x0, y0, po, rpo, 5, dx, dy);
    GRAcolor(GC,fr,fg,fb, fa);
    GRAdrawpoly(GC,5,rpo,GRA_FILL_MODE_NONE);
    po[0] = x0-size/4;
    po[1] = y0-size/4;
    po[2] = x0+size/4;
    po[3] = y0+size/4;
    po[4] = x0-size/4;
    po[5] = y0+size/4;
    po[6] = x0+size/4;
    po[7] = y0-size/4;
    GRArotate(x0, y0, po, rpo, 4, dx, dy);
    GRAline(GC, rpo[0], rpo[1], rpo[2], rpo[3]);
    GRAline(GC, rpo[4], rpo[5], rpo[6], rpo[7]);
    break;
  default:
    GRAcolor(GC,fr,fg,fb, fa);
    GRAputpixel(GC,x0,y0);
    break;
  }
}

void
GRAmark(int GC,int type,int x0,int y0,int size,
	int fr,int fg,int fb, int fa, int br,int bg,int bb, int ba)
{
  GRAmark_rotate(GC, type, x0, y0, 1, 0, size, fr, fg, fb, fa, br, bg, bb, ba);
}

void
GRAouttext(int GC,char *s)
{
  char code;
  int cpar[1];
  char *cstr;

  code='S';
  cpar[0]=-1;
  cstr=s;
  GRAdraw(GC,code,cpar,cstr);
}

int
GRAlayer_support(int GC)
{
  int layer;
  if (GC < 0) {
    return FALSE;
  }
  if (GRAClist[GC].obj == NULL || GRAClist[GC].inst == NULL) {
    return FALSE;
  }
  if (_getobj(GRAClist[GC].obj, "_layer", GRAClist[GC].inst, &layer)) {
    return FALSE;
  }
  return layer;
}

void
GRAlayer(int GC,const char *s)
{
  char code;
  int cpar[1];
  char *cstr;

  g_free(GRAClist[GC].linedash);
  GRAClist[GC].linedashn=0;
  GRAClist[GC].linedash=NULL;
#if EXPAND_DOTTED_LINE
  g_free(GRAClist[GC].gdashlist);
  GRAClist[GC].gdashlist=NULL;
#endif
  g_free(GRAClist[GC].textfont);
  GRAClist[GC].textfont=NULL;
  GRAClist[GC].viewf=FALSE;
  GRAClist[GC].linef=FALSE;
  GRAClist[GC].colorf=FALSE;
  GRAClist[GC].textf=FALSE;
  code='Z';
  cpar[0]=-1;
  cstr=(char *) s;
  GRAdraw(GC,code,cpar,cstr);
}

static char *GRAexpandobj(char **s);
static char *GRAexpandmath(char **s);

static char *
GRAexpandobj(char **s)
{
  struct objlist *obj;
  int i,j,anum,*id;
  struct narray iarray;
  char *arg,*ret;
  GString *str, *garg;
  char *field;
  int len;
  int quote;

  *s=*s+2;
  field = arg = NULL;
  str = garg = NULL;
  arrayinit(&iarray,sizeof(int));

  garg = g_string_sized_new(64);
  if (garg == NULL) {
    goto errexit;
  }

  if (chkobjilist2(s,&obj,&iarray,TRUE)) goto errexit;
  anum=arraynum(&iarray);
  if (anum<=0) goto errexit;
  id=arraydata(&iarray);
  if (((*s)==NULL)
    || (strchr(":= \t",(*s)[0])!=NULL)
    || ((field=getitok2(s,&len,":= \t}"))==NULL)) goto errexit;
  if (((*s)[0]!='\0') && ((*s)[0]!='}')) (*s)++;
  while (((*s)[0]==' ') || ((*s)[0]=='\t')) (*s)++;
  quote=FALSE;
  while (((*s)[0]!='\0') && (quote || ((*s)[0]!='}'))) {
    if (!quote && ((*s)[0]=='%') && ((*s)[1]=='{')) {
      ret=GRAexpandobj(s);
      if (ret) {
	g_string_append(garg, ret);
	g_free(ret);
      }
    } else if (!quote && ((*s)[0]=='%') && ((*s)[1]=='[')) {
      ret=GRAexpandmath(s);
      if (ret) {
	g_string_append(garg, ret);
	g_free(ret);
      }
    } else if (!quote && ((*s)[0]=='\\')) {
      quote=TRUE;
      (*s)++;
    } else {
      if (quote) quote=FALSE;
      g_string_append_c(garg, (*s)[0]);
      (*s)++;
    }
  }
  if ((*s)[0]!='}') goto errexit;
  (*s)++;

  arg = g_string_free(garg, FALSE);
  garg = NULL;

  str = g_string_sized_new(64);
  if (str == NULL) {
    goto errexit;
  }

  for (i=0;i<anum;i++) {
    if (schkobjfield(obj,id[i],field,arg,&ret,TRUE,FALSE,FALSE)!=0) {
      /* limittype must be TRUE because checking "bbox" field of the
	 same text object causes infinite loop */
      goto errexit;
    }
    for (j=0;ret[j]!='\0';j++) {
      if ((ret[j]=='%') || (ret[j]=='\\')
       || (ret[j]=='_') || (ret[j]=='^') || (ret[j]=='@')) {
	g_string_append_c(str, '\\');
      }
      g_string_append_c(str, ret[j]);
    }
    g_free(ret);
  }
  arraydel(&iarray);
  g_free(field);
  g_free(arg);
  return g_string_free(str, FALSE);

errexit:
  arraydel(&iarray);
  if (str) {
    g_string_free(str, TRUE);
  }
  if (garg) {
    g_string_free(garg, TRUE);
  }
  g_free(field);
  g_free(arg);
  return NULL;
}

static char *
GRAexpandmath(char **s)
{
  char *ret;
  int quote;
  int rcode;
  double vd, uvd, mod;
  GString *str;

  *s = *s + 2;
  str = g_string_sized_new(128);
  if (str == NULL) {
    return NULL;
  }

  if (*s == NULL) {
    goto errexit;
  }

  quote = FALSE;
  while ((*s)[0] != '\0' && (quote || (*s)[0] != ']')) {
    if (! quote && (*s)[0] == '%' && (*s)[1] == '{') {
      ret = GRAexpandobj(s);
      if (ret) {
	g_string_append(str, ret);
	g_free(ret);
      }
    } else if (!quote && (*s)[0] == '%' && (*s)[1] == '[') {
      ret = GRAexpandmath(s);
      if (ret) {
	g_string_append(str, ret);
	g_free(ret);
      }
    } else if (!quote && (*s)[0] == '\\') {
      quote = TRUE;
      (*s)++;
    } else {
      if (quote) {
	quote = FALSE;
      }
      g_string_append_c(str, (*s)[0]);
      (*s)++;
    }
  }

  if ((*s)[0] != ']') {
    goto errexit;
  }
  (*s)++;
  str_calc(str->str, &vd, &rcode, NULL);
  if (rcode != MATH_VALUE_NORMAL) {
    goto errexit;
  }
  g_string_free(str, TRUE);

  mod = fabs(fmod(vd, 1));
  uvd = fabs(vd);
  if (uvd == 0) {
    ret = g_strdup("0");
  } else if (uvd < 0.1) {
    ret = g_strdup_printf("%.15e", vd);
  } else if (uvd < 1E6 && mod < DBL_EPSILON) {
    ret = g_strdup_printf("%.0f", vd);
  } else if (uvd < 10) {
    ret = g_strdup_printf("%.15f", vd);
  } else if (uvd < 100) {
    ret = g_strdup_printf("%.14f", vd);
  } else if (uvd < 1000) {
    ret = g_strdup_printf("%.13f", vd);
  } else {
    ret = g_strdup_printf("%.15e", vd);
  }
  return ret;

errexit:
  g_string_free(str, TRUE);
  return NULL;
}

static char *
GRAexpandpf(char **s)
{
  int i, len;
  char *format, *ret2;
  int quote;
  GString *str, *ret;

  *s = *s + 4;
  str = NULL;
  format = NULL;

  ret = g_string_sized_new(64);
  if (ret == NULL) {
    goto errexit;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    goto errexit;
  }
  if ((*s)==NULL) goto errexit;
  if ((format=getitok2(s,&len," \t}"))==NULL) goto errexit;
  if (((*s)[0]!='\0') && ((*s)[0]!='}')) (*s)++;
  while (((*s)[0]==' ') || ((*s)[0]=='\t')) (*s)++;
  quote=FALSE;
  while (((*s)[0]!='\0') && (quote || ((*s)[0]!='}'))) {
    if (!quote && ((*s)[0]=='%') && ((*s)[1]=='{')) {
      ret2=GRAexpandobj(s);
      if (ret2) {
	g_string_append(str, ret2);
	g_free(ret2);
      }
    } else if (!quote && ((*s)[0]=='%') && ((*s)[1]=='[')) {
      ret2=GRAexpandmath(s);
      if (ret2) {
	g_string_append(str, ret2);
	g_free(ret2);
      }
    } else if (!quote && ((*s)[0]=='\\')) {
      quote=TRUE;
      (*s)++;
    } else {
      if (quote) quote=FALSE;
      g_string_append_c(str, (*s)[0]);
      (*s)++;
    }
  }
  if ((*s)[0]!='}') goto errexit;
  (*s)++;

  add_printf_formated_str(ret, format, str->str, &i);

  g_string_free(str, TRUE);
  g_free(format);
  return g_string_free(ret, FALSE);

errexit:
  if (ret) {
    g_string_free(ret, TRUE);
  }

  if (str) {
    g_string_free(str, TRUE);
  }
  g_free(format);
  return NULL;
}

static gchar *
GRAexpandtext(char *s)
{
  int j,len;
  GString *str;
  char *snew,*s2;

  if (s == NULL) {
    return NULL;
  }

  str = g_string_sized_new(256);
  if (str == NULL) {
    return NULL;
  }

  len = strlen(s);
  j = 0;
  do {
    int i;
    i = j;
    while (j < len && s[j] != '\\' && s[j] != '%') {
      j++;
    }
    g_string_append_len(str, s + i, j - i);
    if (s[j] == '\\') {
      switch (s[j + 1]) {
      case 'n':
        g_string_append_c(str, '\n');
        j += 2;
	break;
      case 'b':
        g_string_append_c(str, '\b');
        j += 2;
	break;
      case '&':
        g_string_append_c(str, '\r');
        j += 2;
	break;
      case 'N':
        g_string_append_c(str, FONT_STYLE_NORMAL);
        j += 2;
	break;
      case 'B':
        g_string_append_c(str, FONT_STYLE_BOLD);
        j += 2;
	break;
      case 'I':
        g_string_append_c(str, FONT_STYLE_ITALIC);
        j += 2;
	break;
      case '.':
        g_string_append_c(str, ' ');
        j += 2;
	break;
      case '-':
	g_string_append(str, "‐");
        j += 2;
	break;
      case '\\':
      case '%':
      case '@':
      case '^':
      case '_':
        g_string_append_c(str, '\\');
        g_string_append_c(str, s[j + 1]);
        j += 2;
	break;
      case 'x':
	  if (g_ascii_isxdigit(s[j + 2]) && g_ascii_isxdigit(s[j + 3])) {
	    if (s[j + 2] != '0' || s[j + 3] != '0') {
	      g_string_append_c(str, '\\');
	      g_string_append_c(str, 'x');
	      g_string_append_c(str, toupper(s[j + 2]));
	      g_string_append_c(str, toupper(s[j + 3]));
	    }
	    j += 4;
	  } else {
	    j++;
	  }
	break;
      default:
        j++;
      }
    } else if ((s[j]=='%') && (s[j+1]=='{')) {
      s2=s+j;
      snew=GRAexpandobj(&s2);
      if (snew) {
	g_string_append(str, snew);
	g_free(snew);
      }
      j=(s2-s);
    } else if ((s[j]=='%') && (s[j+1]=='[')) {
      s2=s+j;
      snew=GRAexpandmath(&s2);
      if (snew) {
	g_string_append(str, snew);
	g_free(snew);
      }
      j=(s2-s);
    } else if ((s[j]=='%') && (s[j+1]=='p') && (s[j+2]=='f') && (s[j+3]=='{')) {
      s2=s+j;
      snew=GRAexpandpf(&s2);
      if (snew) {
	g_string_append(str, snew);
	g_free(snew);
      }
      j=(s2-s);
    } else if (j<len) {
      g_string_append_c(str, s[j]);
      j++;
    }
  } while (j < len);

  return g_string_free(str, FALSE);
}

void
GRAdrawtext(int GC, char *s, char *font, int style,
	    int size, int space, int dir, int scriptsize)
{
  char *c, *tok;
  GString *str;
  int len, scmovex, scmovey, scriptf, scx_max, scy_max, scdist_max;
  char *endptr;
  char *font2;
  int size2, space2, style2, vspace;
  char *font3;
  int size3, space3, style3;
  int i, k, x, y, val, x0, y0, x1, y1;
  double cs, si;
  int height;
  int alignlen, fx0, fx1, fy0, fy1;
  char ch;
  gchar *ptr;

  if (font == NULL || s== NULL) {
    return;
  }

  c = NULL;
  font2 = NULL;
  font3 = NULL;
  cs = cos(dir * MPI / 18000);
  si = sin(dir * MPI / 18000);
  x0 = GRAClist[GC].cpx;
  y0 = GRAClist[GC].cpy;
  vspace = 0;

  str = g_string_sized_new(128);
  if (str == NULL) {
    goto errexit;
  }

  c = GRAexpandtext(s);
  if (c == NULL || c[0] == '\0') {
    goto errexit;
  }

  font2 = g_strdup(font);
  if (font2 == NULL) {
    goto errexit;
  }

  style3 = style2 = style;
  size2 = size;
  space2 = space;
  size3 = size;
  space3 = space;
  scriptf = 0;
  scmovex = 0;
  scmovey = 0;
  scx_max = 0;
  scy_max = 0;
  scdist_max = 0;

  len = strlen(c);

  for (k = 0; k < len && c[k] != '\n' && c[k] != '\r'; k++);
  if (c[k] == '\r') {
    ch = c[k];
    c[k] = '\0';
    GRAtextextent(c, font2, style2, size2, space2, scriptsize,
                  &fx0, &fy0, &fx1, &fy1, TRUE);
    c[k] = ch;
    alignlen = fx1;
  } else {
    alignlen = 0;
  }

  ptr = c;
  while (*ptr) {
    g_string_set_size(str, 0);

    for (; *ptr && strchr("\n\r\b_^@%", ptr[0]) == NULL; ptr++) {
      if (ptr[0] == '\\') {
        if (ptr[1] == 'x' && g_ascii_isxdigit(ptr[2]) && g_ascii_isxdigit(ptr[3])) {
	  gunichar wc;

	  wc = g_ascii_xdigit_value(ptr[2]) * 16 + g_ascii_xdigit_value(ptr[3]);
	  g_string_append_unichar(str, wc);

          ptr += 3;
        } else if (ptr[1]=='\\') {
	  g_string_append_c(str, ptr[0]);
	  g_string_append_c(str, ptr[1]);
          ptr++;
	} else if (g_ascii_isprint(ptr[1])) {
	  g_string_append_c(str, ptr[1]);
          ptr++;
	}
      } else if (ptr[0] == FONT_STYLE_NORMAL) {
	style3 = GRA_FONT_STYLE_NORMAL;
      } else if (ptr[0] == FONT_STYLE_BOLD) {
	style3 |= GRA_FONT_STYLE_BOLD;
      } else if (ptr[0] == FONT_STYLE_ITALIC) {
	style3 |= GRA_FONT_STYLE_ITALIC;
      } else {
	g_string_append_c(str, ptr[0]);
      }

      if (style3 != style2) {
	break;
      }
    }

    if (str->len != 0) {
      GRAtextstyle(GC, font2, style2, size2, space2, dir);
      GRAouttext(GC, str->str);
    }

    if (style3 != style2) {
      style2 = style3;
      continue;
    }

    switch (ptr[0]) {
    case '\n':
      x0 += (int) (si * size * 25.4 / 72.0);
      y0 += (int) (cs * size * 25.4 / 72.0);
      if (scriptf) {
        scriptf = 0;
        g_free(font2);
        font2 = font3;
        font3 = NULL;
        size2 = size3;
        space2 = space3;
      }
      x0 += scx_max;
      y0 += scy_max + vspace;
      scdist_max = 0;
      scx_max = 0;
      scy_max = 0;
      scmovex = 0;
      scmovey = 0;
      if (alignlen != 0) {
        for (k = 1; ptr[k] != '\0' && ptr[k] != '\n' && ptr[k] != '\r'; k++);
        ch = ptr[k];
        ptr[k] = '\0';
        GRAtextextent(ptr, font2, style2, size2, space2, scriptsize,
                      &fx0, &fy0, &fx1, &fy1, TRUE);
        ptr[k] = ch;
        x1 = x0 + (int) ( cs * (alignlen - fx1));
        y1 = y0 + (int) (-si * (alignlen - fx1));
      } else {
        x1 = x0;
        y1 = y0;
      }
      GRAmoveto(GC, x1, y1);
      ptr++;
      break;
    case '\b':
      GRAtextextent("h", font2, style2, size2, space2, scriptsize,
                    &fx0, &fy0, &fx1, &fy1, TRUE);
      x1 = (int) (cs * (fx1 - fx0));
      y1 = (int) (si * (fx1 - fx0));
      GRAmoverel(GC, -x1, y1);
      ptr++;
      break;
    case '\r':
      ptr++;
      break;
    case '_':
    case '^':
      if (scriptf == 0) {
	font3 = g_strdup(font2);
	if (font3 == NULL) {
	  goto errexit;
	}
	size3 = size2;
	space3 = space2;
      }
      height = size2;
      size2 = (int) (size2 * 1e-4 * scriptsize);
      space2 = (int) (space2 * 1e-4 * scriptsize);
      if (ptr[0] == '^') {
	x = (int) (-si * (height * 0.8 - size2 * 5e-5 * scriptsize) * 25.4 / 72.0);
	y = (int) (-cs * (height * 0.8 - size2 * 5e-5 * scriptsize) * 25.4 / 72.0);
	GRAmoverel(GC, x, y);
	scmovex += x;
	scmovey += y;
	scriptf = 1;
      } else {
	x = (int) (si * size2 * 5e-5 * scriptsize * 25.4 / 72.0);
	y = (int) (cs * size2 * 5e-5 * scriptsize * 25.4 / 72.0);
	GRAmoverel(GC, x, y);
	scmovex += x;
	scmovey += y;
	scriptf = 2;
	if (scdist_max < scmovex * si + scmovey * cs) {
	  scdist_max = scmovex * si + scmovey * cs;
	  scx_max = scmovex;
	  scy_max = scmovey;
	}
      }
      ptr++;
      break;
    case '@':
      if (scriptf) {
	scriptf = 0;
	GRAmoverel(GC, -scmovex, -scmovey);
	g_free(font2);
	font2 = font3;
	font3 = NULL;
	size2 = size3;
	space2 = space3;
      }
      scmovex = 0;
      scmovey = 0;
      ptr++;
      break;
    case '%':
      if ((ptr[1]!='\0') && ( strchr("FJSPNXYCA", toupper(ptr[1]))!=NULL) && (ptr[2]=='{')) {
        for (i = 3; ptr[i] != '\0' && ptr[i] != '}'; i++);
        if (ptr[i] == '}') {
	  tok = g_strndup(ptr + 3, i - 3);
          if (tok == NULL) {
	    goto errexit;
	  }
          if (tok[0] != '\0') {
            switch (toupper(ptr[1])) {
            case 'F':
              g_free(font2);
              font2 = g_strdup(tok);
              break;
            case 'J':
              break;
            case 'S':
              val=strtol(tok, &endptr, 10);
              if (endptr[0]=='\0') size2=val * 100;
              break;
            case 'P':
              val=strtol(tok, &endptr, 10);
              if (endptr[0]=='\0') space2=val * 100;
              break;
            case 'N':
              val=strtol(tok, &endptr, 10);
              if (endptr[0]=='\0') vspace=val * 100;
              break;
            case 'X':
              val=strtol(tok, &endptr, 10);
              if (endptr[0]=='\0') GRAmoverel(GC, (int) (val * 100 * 25.4 / 72.0), 0);
              break;
            case 'Y':
              val=strtol(tok, &endptr, 10);
              if (endptr[0]=='\0') GRAmoverel(GC, 0, (int) (val * 100 * 25.4 / 72.0));
              break;
            case 'C':
	      {
		char *tmp_ptr;

		tmp_ptr = tok;
		while (*tmp_ptr == '#' || g_ascii_isspace(*tmp_ptr)) {
		  tmp_ptr++;
		}
		val = strtol(tmp_ptr, &endptr, 16);
		if (endptr[0]=='\0') {
		  GRAcolor(GC,
			   (val >> 16) & 0xff,
			   (val >> 8) & 0xff,
			   val & 0xff,
			   -1);
		}
	      }
              break;
            case 'A':
	      val = strtol(tok, &endptr, 10);
	      if (endptr[0]=='\0') {
		GRAcolor(GC, -1, -1, -1, val);
	      }
              break;
            }
          }
	  g_free(tok);
        }
        ptr += i + 1;
      } else {
	ptr = g_utf8_next_char(ptr);
      }
      break;
    }
  }

errexit:
  if (str) {
    g_string_free(str, TRUE);
  }
  g_free(c);
  g_free(font2);
  g_free(font3);
}

void
GRAdrawtextraw(int GC, char *s, char *font, int style,
	       int size, int space, int dir)
{
  GString *str;
  int len, j;

  if (font == NULL || s == NULL) {
    return;
  }

  str = g_string_sized_new(128);
  if (str == NULL) {
    return;
  }

  len = strlen(s);
  for (j = 0; j < len; j++) {
    if (s[j] == '\\') {
      g_string_append_c(str, '\\');
    }
    g_string_append_c(str, s[j]);
  }

  if (str->len > 0) {
    GRAtextstyle(GC, font, style, size, space, dir);
    GRAouttext(GC, str->str);
  }
  g_string_free(str, TRUE);
}

void
GRAtextextent(char *s, char *font, int style,
	      int size, int space, int scriptsize,
	      int *gx0, int *gy0, int *gx1, int *gy1, int raw)
{
  gchar *c, *tok;
  GString *str;
  int w, h, d, len, scmovey, scriptf, scy_max;
  char *endptr;
  char *font2;
  int size2, space2, style2, vspace;
  char *font3;
  int size3, space3, style3;
  int i, j, k, y, val, x0, y0, ofsty;
  int height;
  int alignlen, fx0, fx1, fy0, fy1;
  char ch;

  *gx0 = *gy0 = *gx1 = *gy1 = 0;
  if (font == NULL || s == NULL) {
    return;
  }
  font2 = NULL;
  font3 = NULL;
  str = NULL;
  x0 = 0;
  y0 = 0;
  ofsty = 0;

  c = GRAexpandtext(s);
  if (c == NULL) {
    goto errexit;
  }
  if (c[0] == '\0') {
    goto errexit;
  }

  style3 = style2 = style;
  font2 = g_strdup(font);
  size2 = size;
  space2 = space;
  size3 = size;
  space3 = space;
  scriptf = 0;
  scmovey = 0;
  scy_max = 0;
  vspace = 0;

  len = strlen(c);

  if (! raw) {
    for (k = 0; (k < len) && (c[k] != '\n') && (c[k] != '\r'); k++);
    if (c[k] == '\r') {
      ch = c[k];
      c[k] = '\0';
      GRAtextextent(c, font2, style2, size2, space2, scriptsize,
                    &fx0, &fy0, &fx1, &fy1, TRUE);
      c[k] = ch;
      alignlen = fx1;
    } else {
      alignlen = 0;
    }
  } else {
    alignlen = 0;			/* dummy code to avoid compile warnings */
  }

  j=0;
  str = g_string_sized_new(256);
  if (str == NULL) {
    goto errexit;
  }
  do {
    g_string_set_size(str, 0);

    while (j < len && strchr("\n\b\r_^@%",c[j]) == NULL) {
      switch (c[j]) {
      case '\\':
        if (c[j + 1]=='x' && g_ascii_isxdigit(c[j + 2]) && g_ascii_isxdigit(c[j + 3])) {
	  gunichar wc;

	  wc = g_ascii_xdigit_value(c[j + 2]) * 16 + g_ascii_xdigit_value(c[j + 3]);
	  g_string_append_unichar(str, wc);
          j += 4;
        } else if (isprint(c[j + 1])) {
	  g_string_append_c(str, c[j + 1]);
          j += 2;
        } else {
	  j++;
	}
	break;
      case FONT_STYLE_NORMAL:
	style3 = GRA_FONT_STYLE_NORMAL;
	j++;
	break;
      case FONT_STYLE_BOLD:
	style3 |= GRA_FONT_STYLE_BOLD;
	j++;
	break;
      case FONT_STYLE_ITALIC:
	style3 |= GRA_FONT_STYLE_ITALIC;
	j++;
	break;
      default:
	g_string_append_c(str, c[j]);
        j++;
      }

      if (style3 != style2) {
	break;
      }
    }

    if (str->len > 0) {
      w = GRAstrwidth(str->str, font2, style2, size2)
	+ nround(space2 / 72.0 * 25.4) * (str->len - 1);
      h = GRAcharascent(font2, style2, size2);
      d = GRAchardescent(font2, style2, size2);

      if (x0     < *gx0) *gx0 = x0;
      if (x0 + w < *gx0) *gx0 = x0 + w;
      if (x0     > *gx1) *gx1 = x0;
      if (x0 + w > *gx1) *gx1 = x0 + w;
      if (y0 - h < *gy0) *gy0 = y0 - h;
      if (y0 + d < *gy0) *gy0 = y0 + d;
      if (y0 - h > *gy1) *gy1 = y0 - h;
      if (y0 + d > *gy1) *gy1 = y0 + d;
      x0 += w;
    }

    if (style3 != style2) {
      style2 = style3;
      continue;
    }

    switch (c[j]) {
    case '\n':
      y0 += (int) (size * 25.4 / 72.0);
      if (scriptf) {
	y0 -= scmovey;
        scriptf = 0;
        g_free(font2);
        font2 = font3;
        font3 = NULL;
        size2 = size3;
        space2 = space3;
      }
      y0 += scy_max + vspace;
      scmovey = 0;
      scy_max = 0;
      if (! raw && alignlen) {
        for (k = j + 1; (k < len) && (c[k] != '\n') && (c[k] != '\r'); k++);
        ch = c[k];
        c[k] = '\0';
        GRAtextextent(c + j + 1, font2, style2, size2, space2, scriptsize,
                      &fx0, &fy0, &fx1, &fy1, TRUE);
        c[k] = ch;
        x0 = alignlen - fx1;
      } else {
	x0 = 0;
      }
      y0 -= ofsty;
      ofsty = 0;
      j++;
      break;
    case '\b':
      GRAtextextent("h", font2, style2, size2, space2, scriptsize,
                    &fx0, &fy0, &fx1, &fy1, TRUE);
      x0 -= (fx1 - fx0);
      j++;
      break;
    case '\r':
      j++;
      break;
    case '_':
    case '^':
      if (scriptf == 0) {
	font3 = g_strdup(font2);
	if (font3 == NULL) {
	  goto errexit;
	}
	size3 = size2;
	space3 = space2;
      }
      height = size2;
      size2 = (int) (size2 * 1e-4 * scriptsize);
      space2 = (int) (space2 * 1e-4 * scriptsize);
      if (c[j]=='^') {
	y = (int) (-(height * 0.8 - size2 * 5e-5 * scriptsize) * 25.4 / 72.0);
	y0 += y;
	scmovey += y;
	scriptf = 1;
      } else {
	y = (int) (size2 * 5e-5 * scriptsize * 25.4 / 72.0);
	y0 += y;
	scmovey += y;
	scriptf = 2;
	if (scy_max < scmovey) {
	  scy_max = scmovey;
	}
      }
      j++;
      break;
    case '@':
      if (scriptf) {
	scriptf = 0;
	y0 -= scmovey;
	g_free(font2);
	font2 = font3;
	font3 = NULL;
	size2 = size3;
	space2 = space3;
      }
      scmovey = 0;
      j++;
      break;
    case '%':
      if (c[j + 1] != '\0' && strchr("FJSPNXYCA", toupper(c[j + 1])) && c[j + 2] == '{') {
        for (i = j + 3; c[i] != '\0' && c[i] != '}'; i++);
        if (c[i] == '}') {
	  tok = g_strndup(c + j + 3, i - j - 3);
          if (tok == NULL) {
	    goto errexit;
	  }
          if (tok[0] != '\0') {
            switch (toupper(c[j + 1])) {
            case 'F':
              g_free(font2);
              font2 = g_strdup(tok);
              break;
            case 'J':
            case 'C':
            case 'A':
              break;
            case 'S':
              val = strtol(tok, &endptr, 10);
              if (endptr[0] == '\0') {
		size2 = val * 100;
	      }
              break;
            case 'P':
              val = strtol(tok, &endptr, 10);
              if (endptr[0] == '\0') {
		space2 = val * 100;
	      }
              break;
            case 'N':
              val = strtol(tok, &endptr, 10);
              if (endptr[0] == '\0') {
		vspace = val * 100;
	      }
              break;
            case 'X':
              val = strtol(tok, &endptr, 10);
              if (endptr[0] == '\0') {
		x0 += (int) (val * 100 * 25.4 / 72.0);
	      }
              break;
            case 'Y':
              val = strtol(tok, &endptr, 10);
              if (endptr[0] == '\0') {
		y = (int) (val * 100 * 25.4 / 72.0);
		ofsty += y;
		y0 += y;
	      }
              break;
            }
          }
	  g_free(tok);
        }
        j = i + 1;
      } else {
	j++;
      }
      break;
    }
  } while (j < len);

errexit:
  if (str) {
    g_string_free(str, TRUE);
  }
  g_free(c);
  g_free(font2);
  g_free(font3);
}

void
GRAtextextentraw(char *s,char *font, int style,
		 int size,int space,int *gx0,int *gy0,int *gx1,int *gy1)
{
  int i, n, len, ha, hd;
  *gx0 = *gy0 = *gx1 = *gy1 = 0;
  if (s == NULL || font == NULL) return;

  len = strlen(s);
  if (len < 1) {
    return;
  }
  *gx1 = GRAstrwidth(s, font, style, size)
    + nround(space / 72.0 * 25.4) * (len - 1);
  for (n = 0, i = 0; i < len; i++) {
    if (s[i] == '\n') {
      n++;
    }
  }
  ha = GRAcharascent(font, style, size);
  *gy0 = - ha;
  hd = GRAchardescent(font, style, size);
  *gy1 = hd + (ha + hd) * n;
}


static int
getintpar(const char *s,int num,int cpar[])
{
  int i,pos1;
  char s2[256];
  char *endptr;

  pos1=0;
  for (i=0;i<num;i++) {
    int pos2;
    while ((s[pos1]!='\0') &&
          ((s[pos1]==' ') || (s[pos1]=='\t') || (s[pos1]==','))) pos1++;
    if (s[pos1]=='\0') return FALSE;
    pos2=0;
    while (s[pos1] != '\0' && s[pos1] != ' '  &&
	   s[pos1] != '\t' && s[pos1] != ',') {

      if (pos2 >= (int) sizeof(s2) - 1)
	return FALSE;

      s2[pos2]=s[pos1];
      pos2++;
      pos1++;
    }
    s2[pos2]='\0';
    cpar[i]=strtol(s2,&endptr,10);
    if (endptr[0]!='\0') return FALSE;
  }
  return TRUE;
}

int
GRAinputdraw(int GC,int leftm,int topm,int rate_x,int rate_y,
                  char code,int *cpar,char *cstr)
{
  int i;
  double r, rx, ry;

  if (GRAClist[GC].mergezoom==0) {
    rx = ry = 1;
  } else {
    rx = ((double )rate_x)/GRAClist[GC].mergezoom;
    ry = ((double )rate_y)/GRAClist[GC].mergezoom;
  }
  r = MIN(rx, ry);
  switch (code) {
  case '%':
  case 'O': case 'Q': case 'F': case 'S': case 'K': case 'Z':
    break;
  case 'G':
    if (cpar[0] != 3 && cpar[0] != 4)
      return FALSE;
    break;
  case 'I':
    if (cpar[0] != 5)
      return FALSE;

    GRAClist[GC].mergeleft=cpar[1];
    GRAClist[GC].mergetop=cpar[2];
    GRAClist[GC].mergezoom=cpar[5];
    code='\0';
    break;
  case 'E':
    GRAClist[GC].mergeleft=0;
    GRAClist[GC].mergetop=0;
    GRAClist[GC].mergezoom=10000;
    code='\0';
    break;
  case 'V':
    if (cpar[0] != 5)
      return FALSE;

    cpar[1]=(int )(((cpar[1]-GRAClist[GC].mergeleft)*rx)+leftm);
    cpar[2]=(int )(((cpar[2]-GRAClist[GC].mergetop)*ry)+topm);
    cpar[3]=(int )(((cpar[3]-GRAClist[GC].mergeleft)*rx)+leftm);
    cpar[4]=(int )(((cpar[4]-GRAClist[GC].mergetop)*ry)+topm);
    break;
  case 'A':
    if (cpar[0] != cpar[1] + 5)
      return FALSE;

    cpar[2]=(int )(cpar[2]*r);
    for (i=0;i<cpar[1];i++) cpar[i+5]=(int )(cpar[i+5]*r);
    break;
  case 'H':
    if (cpar[0] != 3 && cpar[0] != 4) {
      return FALSE;
    } else {
      double rp, rn;
      cpar[3] = calc_zoom_direction(cpar[3], rx, ry, &rp, &rn);
      cpar[1]=(int )(cpar[1]*MIN(rp, rn));
      cpar[2]=(int )(cpar[2]*rp);
    }
    break;
  case 'M': case 'N': case 'T': case 'P':
    if (cpar[0] != 2)
      return FALSE;

    cpar[1]=(int )(cpar[1]*rx);
    cpar[2]=(int )(cpar[2]*ry);
    break;
  case 'L':
    if (cpar[0] != 4)
      return FALSE;

    cpar[1]=(int )(cpar[1]*rx);
    cpar[2]=(int )(cpar[2]*ry);
    cpar[3]=(int )(cpar[3]*rx);
    cpar[4]=(int )(cpar[4]*ry);
    break;
  case 'C':
    if (cpar[0] != 7)
      return FALSE;

    cpar[1]=(int )(cpar[1]*rx);
    cpar[2]=(int )(cpar[2]*ry);
    cpar[3]=(int )(cpar[3]*rx);
    cpar[4]=(int )(cpar[4]*ry);
    break;
  case 'B':
    if (cpar[0] != 5)
      return FALSE;

    cpar[1]=(int )(cpar[1]*rx);
    cpar[2]=(int )(cpar[2]*ry);
    cpar[3]=(int )(cpar[3]*rx);
    cpar[4]=(int )(cpar[4]*ry);
    break;
  case 'R':
    if (cpar[0] != cpar[1] * 2 + 1)
      return FALSE;

    for (i=0;i<(2*(cpar[1]));i++) cpar[i+2]=(int )(cpar[i+2]*((i % 2) ? ry : rx));
    break;
  case 'D':
    if (cpar[0] != cpar[1] * 2 + 2)
      return FALSE;

    for (i=0;i<(2*(cpar[1]));i++) cpar[i+3]=(int )(cpar[i+3]*((i % 2) ? ry : rx));
    break;
  }
  if (code!='\0') GRAdraw(GC,code,cpar,cstr);

  return TRUE;
}

void
GRAdata_free(struct GRAdata *data)
{
  if (data == NULL) {
    return;
  }
  if (data->cstr) {
    g_free(data->cstr);
  }
  if (data->cpar) {
    g_free(data->cpar);
  }
}

int
GRAparse(struct GRAdata *data, char *s)
{
  int pos, num, i;
  char code;
  int *cpar;
  char *cstr;

  cpar =NULL;
  cstr = NULL;
  for (i = 0; s[i] != '\0'; i++) {
    if (strchr("\n\r", s[i]) != NULL) {
      s[i] = '\0';
      break;
    }
  }

  pos = 0;
  while (s[pos] == ' ' || s[pos] == '\t') {
    pos++;
  }

  if (s[pos] == '\0') {
    return TRUE;
  }

  if (strchr("IE%VAGOMNLTCBPRDFHSK", s[pos]) == NULL) {
    return FALSE;
  }

  code = s[pos];
  if (strchr("%FSK", code) == NULL) {
    if (! getintpar(s + pos + 1, 1, &num)) {
      return FALSE;
    }
    num++;
    cpar = g_malloc(sizeof(int) * num);
    if (cpar == NULL) {
      return FALSE;
    }
    if (! getintpar(s + pos + 1, num, cpar)) {
      goto errexit;
    }
  } else {
    cpar = g_malloc(sizeof(int));
    if (cpar == NULL) {
      return FALSE;
    }
    cpar[0] = -1;
    cstr = g_strdup(s + pos + 1);
    if (cstr == NULL) {
      goto errexit;
    }
  }
  if (data) {
    data->code = code;
    data->cpar = cpar;
    data->cstr = cstr;
    data->next = NULL;
  }
  return TRUE;

errexit:
  g_free(cpar);
  g_free(cstr);
  return FALSE;
}

int
GRAinput(int GC,char *s,int leftm,int topm,int rate_x,int rate_y)
{
  int r;
  struct GRAdata data;

  if (! GRAparse(&data, s)) {
    return FALSE;
  }
  r = GRAinputdraw(GC,leftm,topm,rate_x,rate_y,data.code,data.cpar,data.cstr);
  GRAdata_free(&data);
  return r;
}

static char *fonttbl[]={
   "Times","TimesBold","TimesItalic","TimesBoldItalic",
   "Helvetica","HelveticaBold","HelveticaOblique","HelveticaBoldOblique",
   "Mincho","Mincho","Mincho","Mincho",
   "Gothic","Gothic","Gothic","Gothic",
   "Courier","CourierBold","CourierItalic","CourierBoldItalic",
   "Symbol"};

#define FONTTBL_NUM ((int) (sizeof(fonttbl) / sizeof(*fonttbl)))
#define FONTTBL_SYMBOL (FONTTBL_NUM - 1)

static char *
get_gra_font(int i)
{
  if (i < 0 || i >= FONTTBL_NUM) {
    i = 0;
  }

  return fonttbl[i];
}

int
GRAinputold(int GC,char *s,int leftm,int topm,int rate_x,int rate_y)
{
  int pos,num,i;
  char code,code2;
  int cpar[50],cpar2[50];
  char cstr[256],*ustr;
  int col,B,R,G;

  for (i=0;s[i]!='\0';i++)
    if (strchr("\n\r",s[i])!=NULL) {
      s[i]='\0';
      break;
    }
  pos=0;
  while ((s[pos]==' ') || (s[pos]=='\t')) pos++;
  if (s[pos]=='\0') return TRUE;
  if (strchr("IEX%VAOMNLTCBPDFSK",s[pos])==NULL) return FALSE;
  code=s[pos];
  if (strchr("%SK",code)==NULL) {
    if (!getintpar(s+pos+1,1,&num)) return FALSE;
    num++;
    if (!getintpar(s+pos+1,num,cpar)) return FALSE;
  } else {
    int j;
    char *po;
    cpar[0]=-1;
    if ((po=strchr(s+pos+1,','))==NULL) return FALSE;
    if ((po=strchr(po+1,','))==NULL) return FALSE;
    strcpy(cstr,po+1);
    j=0;
    for (i=0;cstr[i]!='\0';i++) {
      if (cstr[i]=='\\') {
        if ((cstr[i+1]!='n') && (cstr[i+1]!='r')) {
          cstr[j]=cstr[i+1];
          j++;
        }
        i++;
      } else {
        cstr[j]=cstr[i];
        j++;
      }
    }
    if ((j>0) && (cstr[j-1]==',')) j--;
    cstr[j]='\0';
  }
  switch (code) {
  case 'X':
    break;
  case '%': case 'S': case 'Z':
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    break;
  case 'K':
    ustr = sjis_to_utf8(cstr);
    if (ustr) {
      GRAinputdraw(GC, leftm, topm, rate_x, rate_y, 'S', cpar, ustr);
      g_free(ustr);
    }
    break;
  case 'I':
    cpar[0]=5;
    cpar[5]=nround(cpar[3]/2.1);
    cpar[3]=21000;
    cpar[4]=29700;
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    break;
  case 'E': case 'M': case 'N': case 'L': case 'T': case 'P':
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    break;
  case 'V':
    if (cpar[0]==4) {
      cpar[0]=5;
      cpar[5]=0;
    }
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    break;
  case 'A':
    col=cpar[3];
    GRAClist[GC].oldFB=(col & 1)*256;
    GRAClist[GC].oldFG=(col & 2)*128;
    GRAClist[GC].oldFR=(col & 4)*64;
    code2='G';
    cpar2[0]=3;
    cpar2[1]=GRAClist[GC].oldFR;
    cpar2[2]=GRAClist[GC].oldFG;
    cpar2[3]=GRAClist[GC].oldFB;
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    cpar[0]=5;
    cpar[3]=cpar[4];
    cpar[4]=0;
    cpar[5]=1000;
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    break;
  case 'O':
    col=cpar[1];
    GRAClist[GC].oldBB=(col & 1)*256;
    GRAClist[GC].oldBG=(col & 2)*128;
    GRAClist[GC].oldBR=(col & 4)*64;
    break;
  case 'C':
    cpar2[7]=cpar[4];
    if (cpar[0]>=5) cpar[4]=cpar[5];
    else cpar[4]=cpar[3];
    if (cpar[0]==7) {
	  cpar[5]=cpar[6]*10;
	  cpar[6]=cpar[7]*10-cpar[6]*10;
      if (cpar[6]<0) cpar[6]+=36000;
    } else {
      cpar[5]=0;
	  cpar[6]=36000;
    }
    cpar[7]=cpar2[7];
    cpar[0]=7;
    if (cpar[7]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
	}
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    if (cpar[7]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    }
    break;
  case 'B':
    if (cpar[5]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    }
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    if (cpar[5]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    }
    break;
  case 'D':
    if (cpar[2]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldBR;
      cpar2[2]=GRAClist[GC].oldBG;
      cpar2[3]=GRAClist[GC].oldBB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    }
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code,cpar,cstr);
    if (cpar[2]==1) {
      code2='G';
      cpar2[0]=3;
      cpar2[1]=GRAClist[GC].oldFR;
      cpar2[2]=GRAClist[GC].oldFG;
      cpar2[3]=GRAClist[GC].oldFB;
      GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    }
    break;
  case 'F':
    code2='F';
    cpar2[0]=-1;
    GRAClist[GC].mergefont=cpar[1]*4+cpar[2];
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,get_gra_font(cpar[1]*4+cpar[2]));
    if (cpar[6] == 1) {
      cpar[6]=9000;
    } else if (cpar[6]<0) {
      cpar[6]=abs(cpar[6]);
    }
    code2='H';
    cpar2[0]=3;
    cpar2[1]=cpar[3]*100;
    cpar2[2]=cpar[4]*100;
    cpar2[3]=cpar[6];
    GRAClist[GC].mergept=cpar2[1];
    GRAClist[GC].mergesp=cpar2[2];
    GRAClist[GC].mergedir=cpar2[3];
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    col=cpar[5];
    B=(col & 1)*256;
    G=(col & 2)*128;
    R=(col & 4)*64;
    code2='G';
    cpar2[0]=3;
    cpar2[1]=R;
    cpar2[2]=G;
    cpar2[3]=B;
    GRAinputdraw(GC,leftm,topm,rate_x,rate_y,code2,cpar2,cstr);
    break;
  }
  return TRUE;
}

static int
GRAlineclip(int GC,int *x0,int *y0,int *x1,int *y1)
{
  int xl,yl,xg,yg;
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if (*x0<*x1) {
    xl=*x0;  yl=*y0; xg=*x1;  yg=*y1;
  } else {
    xl=*x1;  yl=*y1; xg=*x0;  yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if (xg>maxx) {
    xg=maxx; yg=(*y1-*y0)*(maxx-*x0)/(*x1-*x0)+*y0;
  }
  if (xl<minx) {
    xl=minx; yl=(*y1-*y0)*(minx-*x0)/(*x1-*x0)+*y0;
  }
  if (yl>yg) {
    int a;
    a=yl;  yl=yg;  yg=a;  a=xl;  xl=xg;  xg=a;
  }
  if ((yg<miny) || (yl>maxy)) return 1;
  if (yg>maxy) {
    yg=maxy; xg=(*x1-*x0)*(maxy-*y0)/(*y1-*y0)+*x0;
  }
  if (yl<miny) {
    yl=miny; xl=(*x1-*x0)*(miny-*y0)/(*y1-*y0)+*x0;
  }
  if ((*y0<*y1) || ((*y0==*y1) && (*x0<*x1))) {
    *x0=xl; *y0=yl;   *x1=xg; *y1=yg;
  } else {
    *x0=xg; *y0=yg;   *x1=xl; *y1=yl;
  }
  return 0;
}

static int
GRArectclip(int GC,int *x0,int *y0,int *x1,int *y1)
{
  int xl,yl,xg,yg;
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if (*x0<*x1) {
    xl=*x0; xg=*x1;
  } else {
    xl=*x1; xg=*x0;
  }
  if (*y0<*y1) {
    yl=*y0; yg=*y1;
  } else {
    yl=*y1; yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if ((yg<miny) || (yl>maxy)) return 1;
  if ((xg>maxx) && (xl<minx) && (yg>maxy) && (yl<miny)) return 1;
  if (xg>maxx) xg=maxx;
  if (xl<minx) xl=minx;
  if (yg>maxy) yg=maxy;
  if (yl<miny) yl=miny;
  *x0=xl;  *y0=yl;
  *x1=xg;  *y1=yg;
  return 0;
}

static int
GRAinview(int GC,int x,int y)
{
  int minx,miny,maxx,maxy;

  if (GRAClist[GC].gminx>GRAClist[GC].gmaxx) {
    minx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
    maxx=0;
  } else {
    minx=0;
    maxx=GRAClist[GC].gmaxx-GRAClist[GC].gminx;
  }
  if (GRAClist[GC].gminy>GRAClist[GC].gmaxy) {
    miny=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
    maxy=0;
  } else {
    miny=0;
    maxy=GRAClist[GC].gmaxy-GRAClist[GC].gminy;
  }
  if ((minx<=x) && (x<=maxx) && (miny<=y) && (y<=maxy)) return 0;
  else return 1;
}

void
GRAcurvefirst(int GC,int num,const int *dashlist,
	      clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
	      double x0,double y0)
{
  int gx0,gy0;

#if EXPAND_DOTTED_LINE
  int i;

  g_free(GRAClist[GC].gdashlist);
  GRAClist[GC].gdashlist=NULL;
  if (num!=0) {
    if ((GRAClist[GC].gdashlist=g_malloc(sizeof(int)*num))==NULL) num=0;
  }
  GRAClist[GC].gdashn=num;
  for (i=0;i<num;i++)
    (GRAClist[GC].gdashlist)[i]=dashlist[i];
  GRAClist[GC].gdashlen=0;
  GRAClist[GC].gdashi=0;
  GRAClist[GC].gdotf=TRUE;
#endif
  GRAClist[GC].gclipf=clipf;
  GRAClist[GC].gtransf=transf;
  GRAClist[GC].gdiff=diff;
  GRAClist[GC].gintpf=intpf;
  GRAClist[GC].gflocal=local;
  GRAClist[GC].x0=x0;
  GRAClist[GC].y0=y0;
  if (GRAClist[GC].gtransf==NULL) {
    gx0=nround(x0);
    gy0=nround(y0);
  } else GRAClist[GC].gtransf(x0,y0,&gx0,&gy0,local);
  GRAmoveto(GC,gx0,gy0);
}

int
GRAcurve(int GC,double c[],double x0,double y0)
{
  double d,dx,dy,ddx,ddy,x,y;

  if (ninterrupt()) return FALSE;
  d=0;
  while (d<1) {
    double dd;
    GRAClist[GC].gdiff(d,c,&dx,&dy,&ddx,&ddy,GRAClist[GC].gflocal);
    if ((fabs(dx)+fabs(ddx)/3)<=1e-100) dx=1;
    else dx=sqrt(fabs(2/(fabs(dx)+fabs(ddx)/3)));
    if ((fabs(dy)+fabs(ddy)/3)<=1e-100) dy=1;
    else dy=sqrt(fabs(2/(fabs(dy)+fabs(ddy)/3)));
    dd=(dx<dy) ? dx : dy;
    d+=dd;
    if (d>1) d=1;
    GRAClist[GC].gintpf(d,c,x0,y0,&x,&y,GRAClist[GC].gflocal);
    GRAdashlinetod(GC,x,y);
  }
  return TRUE;
}

void
GRAdashlinetod(int GC,double x,double y)
{
  double x1,y1,x2,y2;
  int gx1,gy1,gx2,gy2;

  x1=GRAClist[GC].x0;
  y1=GRAClist[GC].y0;
  x2=x;
  y2=y;
  if ((GRAClist[GC].gclipf==NULL)
   || (GRAClist[GC].gclipf(&x1,&y1,&x2,&y2,GRAClist[GC].gflocal)==0)) {
    if (GRAClist[GC].gtransf==NULL) {
      gx1=nround(x1);
      gy1=nround(y1);
      gx2=nround(x2);
      gy2=nround(y2);
    } else {
      GRAClist[GC].gtransf(x1,y1,&gx1,&gy1,GRAClist[GC].gflocal);
      GRAClist[GC].gtransf(x2,y2,&gx2,&gy2,GRAClist[GC].gflocal);
    }
    if ((x1!=GRAClist[GC].x0) || (y1!=GRAClist[GC].y0))
      GRAmoveto(GC,gx1,gy1);
#if EXPAND_DOTTED_LINE
    if (GRAClist[GC].gdashn==0) GRAlineto(GC,gx2,gy2);
    else {
      double dx,dy,len,len2;
      dx=(gx2-gx1);
      dy=(gy2-gy1);
      len2=len=sqrt(dx*dx+dy*dy);
      while (len2
      >((GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]-GRAClist[GC].gdashlen)) {
        double dd;
        int gx,gy;
        dd=(len-len2+(GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]
           -GRAClist[GC].gdashlen)/len;
        gx=gx1+nround(dx*dd);
        gy=gy1+nround(dy*dd);
        if (GRAClist[GC].gdotf) GRAlineto(GC,gx,gy);
        else GRAmoveto(GC,gx,gy);
        GRAClist[GC].gdotf=GRAClist[GC].gdotf ? FALSE : TRUE;
        len2-=((GRAClist[GC].gdashlist)[GRAClist[GC].gdashi]
             -GRAClist[GC].gdashlen);
        GRAClist[GC].gdashlen=0;
        GRAClist[GC].gdashi++;
        if (GRAClist[GC].gdashi>=GRAClist[GC].gdashn) {
          GRAClist[GC].gdashi=0;
          GRAClist[GC].gdotf=TRUE;
        }
      }
      if (GRAClist[GC].gdotf) GRAlineto(GC,gx2,gy2);
      GRAClist[GC].gdashlen+=len2;
    }
#else
    GRAlineto(GC,gx2,gy2);
#endif
  }
  GRAClist[GC].x0=x;
  GRAClist[GC].y0=y;
}

#if ! CURVE_OBJ_USE_EXPAND_BUFFER
void
GRAcmatchfirst(int pointx,int pointy,int err,
                    clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
                    struct cmatchtype *data,int bbox,double x0,double y0)
{
  data->x0=x0;
  data->y0=y0;
  data->gclipf=clipf;
  data->gtransf=transf;
  data->gdiff=diff;
  data->gintpf=intpf;
  data->gflocal=local;
  data->err=err;
  data->pointx=pointx;
  data->pointy=pointy;
  data->bbox=bbox;
  data->minx=0;
  data->miny=0;
  data->maxx=0;
  data->maxy=0;
  data->bboxset=FALSE;
  data->match=FALSE;
}

static void
GRAcmatchtod(double x,double y,struct cmatchtype *data)
{
  double x1,y1,x2,y2;
  int gx1,gy1,gx2,gy2;

  x1=data->x0;
  y1=data->y0;
  x2=x;
  y2=y;
  if (data->bbox) {
    if (data->gtransf==NULL) {
      gx1=nround(x1);
      gy1=nround(y1);
      gx2=nround(x2);
      gy2=nround(y2);
    } else {
      data->gtransf(x1,y1,&gx1,&gy1,data->gflocal);
      data->gtransf(x2,y2,&gx2,&gy2,data->gflocal);
    }
    if (!data->bboxset || gx1<data->minx) data->minx=gx1;
    if (!data->bboxset || gy1<data->miny) data->miny=gy1;
    if (!data->bboxset || gx1>data->maxx) data->maxx=gx1;
    if (!data->bboxset || gy1>data->maxy) data->maxy=gy1;
    data->bboxset=TRUE;
    if (!data->bboxset || gx2<data->minx) data->minx=gx2;
    if (!data->bboxset || gy2<data->miny) data->miny=gy2;
    if (!data->bboxset || gx2>data->maxx) data->maxx=gx2;
    if (!data->bboxset || gy2>data->maxy) data->maxy=gy2;
  } else {
    double r,r2,r3,ip;
    if ((data->gclipf==NULL)
     || (data->gclipf(&x1,&y1,&x2,&y2,data->gflocal)==0)) {
      if (data->gtransf==NULL) {
        gx1=nround(x1);
        gy1=nround(y1);
        gx2=nround(x2);
        gy2=nround(y2);
      } else {
        data->gtransf(x1,y1,&gx1,&gy1,data->gflocal);
        data->gtransf(x2,y2,&gx2,&gy2,data->gflocal);
      }
      x1=gx1;
      y1=gy1;
      x2=gx2;
      y2=gy2;
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((data->pointx-x1)*(data->pointx-x1)+(data->pointy-y1)*(data->pointy-y1));
      r3=sqrt((data->pointx-x2)*(data->pointx-x2)+(data->pointy-y2)*(data->pointy-y2));
      if ((r<=data->err) || (r3<data->err)) {
        data->match=TRUE;
      } else if (r2!=0) {
        ip=((x2-x1)*(data->pointx-x1)+(y2-y1)*(data->pointy-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((data->pointx-x2)*(data->pointx-x2)+(data->pointy-y2)*(data->pointy-y2));
          if (r<data->err) data->match=TRUE;
        }
      }
    }
  }
  data->x0=x;
  data->y0=y;
}

int
GRAcmatch(double c[],double x0,double y0,struct cmatchtype *data)
{
  double d,dx,dy,ddx,ddy,x,y;

  if (ninterrupt()) return FALSE;
  d=0;
  while (d<1) {
    double dd;
    data->gdiff(d,c,&dx,&dy,&ddx,&ddy,data->gflocal);
    if ((fabs(dx)+fabs(ddx)/3)==0) dx=1;
    else dx=sqrt(fabs(2/(fabs(dx)+fabs(ddx)/3)));
    if ((fabs(dy)+fabs(ddy)/3)==0) dy=1;
    else dy=sqrt(fabs(2/(fabs(dy)+fabs(ddy)/3)));
    dd=(dx<dy) ? dx : dy;
    d+=dd;
    if (d>1) d=1;
    data->gintpf(d,c,x0,y0,&x,&y,data->gflocal);
    GRAcmatchtod(x,y,data);
  }
  return TRUE;
}
#endif

static void
setbbminmax(struct GRAbbox *bbox,int x1,int y1,int x2,int y2,int lw)
{
  if (x1>x2) {
    int x;
    x=x1; x1=x2; x2=x;
  }
  if (y1>y2) {
    int y;
    y=y1; y1=y2; y2=y;
  }
  if (lw) {
    x1-=bbox->linew;
    y1-=bbox->linew;
    x2+=bbox->linew;
    y2+=bbox->linew;
  }
  if (!bbox->clip || !((x2<0) || (y2<0)
   || (x1>bbox->clipsizex) || (y1>bbox->clipsizey))) {
    if (bbox->clip) {
      if (x1<0) x1=0;
      if (y1<0) y1=0;
      if (x2>bbox->clipsizex) x2=bbox->clipsizex;
      if (y2>bbox->clipsizey) y1=bbox->clipsizey;
    }
    x1+=bbox->offsetx;
    x2+=bbox->offsetx;
    y1+=bbox->offsety;
    y2+=bbox->offsety;
    if (!bbox->set || (x1<bbox->minx)) bbox->minx=x1;
    if (!bbox->set || (y1<bbox->miny)) bbox->miny=y1;
    if (!bbox->set || (x2>bbox->maxx)) bbox->maxx=x2;
    if (!bbox->set || (y2>bbox->maxy)) bbox->maxy=y2;
    if (!bbox->set) bbox->set=TRUE;
  }
}

void
GRAinitbbox(struct GRAbbox *bbox)
{
  bbox->set=FALSE;
  bbox->minx=0;
  bbox->miny=0;
  bbox->maxx=0;
  bbox->maxy=0;
  bbox->offsetx=0;
  bbox->offsety=0;
  bbox->posx=0;
  bbox->posy=0;
  bbox->pt=0;
  bbox->spc=0;
  bbox->dir=0;
  bbox->linew=0;
  bbox->clip=TRUE;
  bbox->clipsizex=21000;
  bbox->clipsizey=29700;
  bbox->fontalias=NULL;
  bbox->loadfont=FALSE;
}

void
GRAendbbox(struct GRAbbox *bbox)
{
  g_free(bbox->fontalias);
}

static int
get_str_bbox(struct GRAbbox *bbox, char *cstr)
{
  double x, y, csin, ccos;
  int w, h, d, x1, y1, x2, y2, x3, y3, x4, y4;
  gchar *ptr;
  GString *str;

  if (! bbox->loadfont) {
    return 0;
  }

  str = g_string_sized_new(256);
  if (str == NULL) {
    return 1;
  }

  csin = sin(bbox->dir / 18000.0 * MPI);
  ccos = cos(bbox->dir / 18000.0 * MPI);
  for (ptr = cstr; ptr[0]; ptr++) {
    if (ptr[0] == '\\') {
      if (ptr[1] == 'x' && g_ascii_isxdigit(ptr[2]) && g_ascii_isxdigit(ptr[3])) {
	gunichar wc;

	wc = g_ascii_xdigit_value(ptr[2]) * 16 + g_ascii_xdigit_value(ptr[3]);
	g_string_append_unichar(str, wc);
	ptr += 3;
      } else {
	ptr += 1;
	g_string_append_c(str, ptr[0]);
      }
    } else {
      g_string_append_c(str, ptr[0]);
    }
  }

  w = GRAstrwidth(str->str, bbox->fontalias, bbox->font_style, bbox->pt);
  h = GRAcharascent(bbox->fontalias, bbox->font_style, bbox->pt);
  d = GRAchardescent(bbox->fontalias, bbox->font_style, bbox->pt);
  x = 0;
  y = d;
  x1 = (int )(bbox->posx + ( x * ccos + y * csin));
  y1 = (int )(bbox->posy + (-x * csin + y * ccos));
  x = 0;
  y = -h;
  x2 = (int )(bbox->posx + ( x * ccos + y * csin));
  y2 = (int )(bbox->posy + (-x * csin + y * ccos));
  x = w;
  y = d;
  x3 = (int )(bbox->posx + ( x * ccos + y * csin));
  y3 = (int )(bbox->posy + (-x * csin + y * ccos));
  x = w;
  y = -h;
  x4 = (int )(bbox->posx + (int )( x * ccos + y * csin));
  y4 = (int )(bbox->posy + (int )(-x * csin + y * ccos));
  setbbminmax(bbox, x1, y1, x4, y4, FALSE);
  setbbminmax(bbox, x2, y2, x3, y3, FALSE);
  bbox->posx += (int )((w + bbox->spc * 25.4 / 72) * ccos);
  bbox->posy -= (int )((w + bbox->spc * 25.4 / 72) * csin);

  g_string_free(str, TRUE);

  return 0;
}

int
GRAboundingbox(char code, int *cpar, char *cstr, void *local)
{
  int lw, j;
  struct GRAbbox *bbox;
  char *tmp;

  bbox=local;
  switch (code) {
  case 'I': case 'X': case 'E': case '%': case 'G': case 'Z':
    break;
  case 'V':
    bbox->offsetx=cpar[1];
    bbox->offsety=cpar[2];
    bbox->posx=0;
    bbox->posy=0;
    if (cpar[5]==1) bbox->clip=TRUE;
    else bbox->clip=FALSE;
    bbox->clipsizex=cpar[3]-cpar[1];
    bbox->clipsizey=cpar[4]-cpar[2];
    break;
  case 'A':
    bbox->linew=cpar[2]/2;
    break;
  case 'M':
    bbox->posx=cpar[1];
    bbox->posy=cpar[2];
    break;
  case 'N':
    bbox->posx+=cpar[1];
    bbox->posy+=cpar[2];
    break;
  case 'L':
    setbbminmax(bbox,cpar[1],cpar[2],cpar[3],cpar[4],TRUE);
    break;
  case 'T':
    setbbminmax(bbox,bbox->posx,bbox->posy,cpar[1],cpar[2],TRUE);
    bbox->posx=cpar[1];
    bbox->posy=cpar[2];
    break;
  case 'C':
    lw = (cpar[7] == 0);
    if (cpar[7]==1) setbbminmax(bbox,cpar[1],cpar[2],cpar[1],cpar[2],lw);
    setbbminmax(bbox,cpar[1]+(int )(cpar[3]*cos(cpar[5]/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin(cpar[5]/18000.0*MPI)),
                cpar[1]+(int )(cpar[3]*cos((cpar[5]+cpar[6])/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin((cpar[5]+cpar[6])/18000.0*MPI)),lw);
    cpar[6]+=cpar[5];
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1],cpar[2]-cpar[4],cpar[1],cpar[2]-cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1]-cpar[3],cpar[2],cpar[1]-cpar[3],cpar[2],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1],cpar[2]+cpar[4],cpar[1],cpar[2]+cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(bbox,cpar[1]+cpar[3],cpar[2],cpar[1]+cpar[3],cpar[2],lw);
    break;
  case 'B':
    setbbminmax(bbox,cpar[1],cpar[2],cpar[3],cpar[4], cpar[5] == 0);
    break;
  case 'P':
    setbbminmax(bbox,cpar[1],cpar[2],cpar[1],cpar[2],FALSE);
    break;
  case 'R':
    for (j = 0; j < cpar[1] - 1; j++)
      setbbminmax(bbox, cpar[j * 2 + 2], cpar[j * 2 + 3], cpar[j * 2 + 4], cpar[j * 2 + 5], TRUE);
    break;
  case 'D':
    lw = (cpar[2] == 0);
    for (j = 0; j < cpar[1] - 1; j++)
      setbbminmax(bbox, cpar[j * 2 + 3], cpar[j * 2 + 4], cpar[j * 2 + 5], cpar[j * 2 + 6],lw);
    break;
  case 'F':
    g_free(bbox->fontalias);
    if ((bbox->fontalias=g_malloc(strlen(cstr)+1))!=NULL)
      strcpy(bbox->fontalias,cstr);
    break;
  case 'H':
    bbox->pt=cpar[1];
    bbox->spc=cpar[2];
    bbox->dir=cpar[3];
    if (cpar[0] > 3) {
      bbox->font_style = cpar[4];
    }
    bbox->loadfont=TRUE;
    break;
  case 'S':
    get_str_bbox(bbox, cstr);
    break;
  case 'K':
    tmp = sjis_to_utf8(cstr);
    if (tmp) {
      get_str_bbox(bbox, tmp);
      g_free(tmp);
    }
   break;
  }
  return 0;
}
