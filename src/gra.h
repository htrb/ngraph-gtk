/*
 * $Id: gra.h,v 1.5 2010-01-04 05:11:28 hito Exp $
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

#include "object.h"

#ifndef GRA_HEADER
#define GRA_HEADER

#define EXPAND_DOTTED_LINE          0
#define CURVE_OBJ_USE_EXPAND_BUFFER 1

struct greektbltype {
  unsigned int jis,symbol;
};

extern struct greektbltype greektable[];

struct GRAbbox {
  int set;
  int offsetx;
  int offsety;
  int minx;
  int miny;
  int maxx;
  int maxy;
  int posx;
  int posy;
  int pt;
  int spc;
  int dir;
  int linew;
  int clip;
  int clipsizex;
  int clipsizey;
  char *fontalias;
  int font_style;
  int loadfont;
};

struct GRAdata
{
  char code;
  int *cpar;
  char *cstr;
  struct GRAdata *next;
};

typedef int (*clipfunc)(double *x0,double *y0,double *x1,double *y1,
                        void *local);
typedef void (*transfunc)(double x0,double y0,int *x1,int *y1,void *local);
typedef void (*diffunc)(double d,double c[],
                        double *dx,double *dy,double *ddx,double *ddy,
                        void *local);
typedef void (*intpfunc)(double d,double c[],
                        double x0,double y0,double *x,double *y,void *local);

typedef int (*directfunc)(char code,int *cpar,char *cstr,void *local);

struct cmatchtype {
  double x0,y0;
  int minx,miny,maxx,maxy;
  int pointx,pointy;
  int err;
  clipfunc gclipf;
  transfunc gtransf;
  diffunc gdiff;
  intpfunc gintpf;
  void *gflocal;
  int bbox,bboxset;
  int match;
};

enum GRA_FONT_STYLE {
  GRA_FONT_STYLE_NORMAL = 0,
  GRA_FONT_STYLE_BOLD   = 1,
  GRA_FONT_STYLE_ITALIC = 2,
};

#define GRA_FONT_STYLE_MAX (GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC)

enum GRA_LINE_CAP {
  GRA_LINE_CAP_BUTT = 0,
  GRA_LINE_CAP_ROUND = 1,
  GRA_LINE_CAP_PROJECTING = 2,
};

enum GRA_LINE_JOIN {
  GRA_LINE_JOIN_MITER = 0,
  GRA_LINE_JOIN_ROUND = 1,
  GRA_LINE_JOIN_BEVEL = 2,
};

enum GRA_FILL_MODE {
  GRA_FILL_MODE_NONE = 0,
  GRA_FILL_MODE_EVEN_ODD = 1,
  GRA_FILL_MODE_WINDING = 2,
};

extern struct greektbltype greektable[48];
int _GRAopencallback(directfunc direct,struct narray **list,void *local);
int _GRAopen(const char *objname,const char *outputname,
            struct objlist *obj,N_VALUE *inst, int output,
	     int strwidth,int charascent,int chardescent,
            struct narray **list,void *local);
int GRAopen(const char *objname,const char *outputname,
            struct objlist *obj,N_VALUE *inst, int output,
	    int strwidth,int charascent,int chardescent,
            struct narray **list,void *local);
int GRAreopen(int GC);
int GRAopened(int GC);
void _GRAclose(int GC);
void GRAclose(int GC);
void GRAredraw(struct objlist *obj,N_VALUE *inst,int setredrawf,int redrawf);
void GRAredraw_layers(struct objlist *obj, N_VALUE *inst, int setredrawf, int redraw_num, char const **objects);
void GRAaddlist(int GC,struct objlist *obj,N_VALUE *inst,
                const char *objname, const char *field);
int GRAdraw(int GC,char code,int *cpar,char *cstr);
int GRAinit(int GC,int leftm,int topm,int width,int height,int zoom);
void GRAregion(int GC,int *width,int *height,int *zoom);
int GRAend(int GC);
void GRAview(int GC,int x1,int y1,int x2,int y2,int clip);
void GRAwindow(int GC,double minx,double miny,double maxx,double maxy);
void GRAlinestyle(int GC,int num,const int *type,
                  int width,enum GRA_LINE_CAP cap,enum GRA_LINE_JOIN join,int miter);
void GRAcolor(int GC, int fr, int fg, int fb, int fa);
void GRAmoveto(int GC,int x,int y);
void GRAline(int GC,int x0,int y0,int x1,int y1);
void GRAlineto(int GC,int x,int y);
void GRAcircle(int GC,int x,int y,int rx,int ry,int cs,int ce,int fil);
void GRArectangle(int GC,int x0,int y0,int x1,int y1,int fil);
void GRAdrawpoly(int GC,int num,const int *point,enum GRA_FILL_MODE fil);
void GRAlines(int GC,int num,const int *point);
void GRArotate(int x0, int y0, const int *pos, int *rpos, int n, double dx, double dy);
void GRAmark_rotate(int GC,int type,int x0,int y0, double dx, double dy, int size,
	     int fr,int fg,int fb, int fa, int br,int bg,int bb, int ba);
void GRAmark(int GC,int type,int x0,int y0,int size,
	     int fr,int fg,int fb, int fa, int br,int bg,int bb, int ba);
void GRAdrawtext(int GC,char *s,char *font, int style,
                 int size, int space, int dir, int scriptsize);
void GRAdrawtextraw(int GC,char *s,char *font, int style,
                 int size,int space,int dir);
void GRAtextextent(char *s,char *font, int style,
                 int size,int space,int scriptsize,
                 int *gx0,int *gy0,int *gx1,int *gy1,int raw);
void GRAtextextentraw(char *s,char *font, int style,
                 int size,int space,int *gx0,int *gy0,int *gx1,int *gy1);
int GRAinput(int GC,char *s,int leftm,int topm,int rate_x,int rate_y);
int GRAinputold(int GC,char *s,int leftm,int topm,int rate_x,int rate_y);
void GRAcurvefirst(int GC,int num,int *dashlist,
      clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
             double x0,double y0);
int GRAcurve(int GC,double c[],double x0,double y0);
void GRAdashmovetod(int GC,double x1,double y1);
void GRAdashlinetod(int GC,double x1,double y1);
void GRAcmatchfirst(int pointx,int pointy,int err,
                   clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
                   struct cmatchtype *data,int bbox,double x0,double y0);
int GRAcmatch(double c[],double x0,double y0,struct cmatchtype *data);
void GRAinitbbox(struct GRAbbox *bbox);
void GRAendbbox(struct GRAbbox *bbox);
int GRAboundingbox(char code,int *cpar,char *cstr,void *local);
void GRAtextstyle(int GC,char *font,int style, int size,int space,int dir);
void GRAouttext(int GC,char *s);
void GRAlayer(int GC,const char *s);
int GRAlayer_support(int GC);
void GRAcurrent_point(int GC, int *x, int *y);
void GRAdata_free(struct GRAdata *data);
int GRAparse(struct GRAdata *data, char *s);
int GRAinputdraw(int GC,int leftm,int topm,int rate_x,int rate_y,char code,int *cpar,char *cstr);

int calc_zoom_direction(int direction, double zx, double zy, double *zp, double *zn);
void GRA_get_linestyle(int GC, int *num, int **type, int *width, enum GRA_LINE_CAP *cap, enum GRA_LINE_JOIN *join, int *miter);

#endif
