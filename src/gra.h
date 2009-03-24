/* 
 * $Id: gra.h,v 1.3 2009/03/24 09:55:53 hito Exp $
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
  int loadfont;
};


typedef int (*clipfunc)(double *x0,double *y0,double *x1,double *y1,
                        void *local);
typedef void (*transfunc)(double x0,double y0,int *x1,int *y1,void *local);
typedef void (*diffunc)(double d,double c[],
                        double *dx,double *dy,double *ddx,double *ddy,
                        void *local);
typedef void (*intpfunc)(double d,double c[],
                        double x0,double y0,double *x,double *y,void *local);

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

extern struct greektbltype greektable[48];
int _GRAopencallback(int (*direct)(char code,int *cpar,char *cstr,void *local),
                   struct narray **list,void *local);
int _GRAopen(char *objname,char *outputname,
            struct objlist *obj,char *inst,
            int output,int charwidth,int charascent,int chardescent,
            struct narray **list,void *local);
int GRAopen(char *objname,char *outputname,
            struct objlist *obj,char *inst,
            int output,int charwidth,int charascent,int chardescent,
            struct narray **list,void *local);
int GRAreopen(int GC);
int GRAopened(int GC);
void _GRAclose(int GC);
void GRAclose(int GC);
void GRAaddlist2(int GC,char *draw);
void GRAinslist2(int GC,char *draw,int n);
void _GRAredraw(int GC,int snum,char **sdata,int setredrawf,int redrawf,
                int addn,struct objlist *obj,char *inst,char *field);
void GRAredraw(struct objlist *obj,char *inst,int setredrawf,int redrawf);
void GRAredraw2(struct objlist *obj,char *inst,int setredrawf,int redrawf,
                int addn,struct objlist *aobj,char *ainst,char *afield);
void GRAaddlist(int GC,struct objlist *obj,char *inst,
                char *objname,char *field);
void GRAinslist(int GC,struct objlist *obj,char *inst,
                char *objname,char *field,int n);
void GRAdellist(int GC,int n);
struct objlist *GRAgetlist(int GC,int *oid,char **field,int n);
int GRAdraw(int GC,char code,int *cpar,char *cstr);
int GRAinit(int GC,int leftm,int topm,int width,int height,int zoom);
void GRAregion(int GC,int *leftm,int *topm,int *width,int *height,int *zoom);
void GRAdirect(int GC,int cpar[]);
int GRAend(int GC);
void GRAremark(int GC,char *s);
void GRAview(int GC,int x1,int y1,int x2,int y2,int clip);
void GRAwindow(int GC,double minx,double miny,double maxx,double maxy);
void GRAlinestyle(int GC,int num,int *type,
                  int width,int cap,int join,int miter);
void GRAcolor(int GC,int fr,int fg,int fb);
void GRAmoveto(int GC,int x,int y);
void GRAmoverel(int GC,int x,int y);
void GRAline(int GC,int x0,int y0,int x1,int y1);
void GRAlineto(int GC,int x,int y);
void GRAcircle(int GC,int x,int y,int rx,int ry,int cs,int ce,int fil);
void GRArectangle(int GC,int x0,int y0,int x1,int y1,int fil);
void GRAputpixel(int GC,int x,int y);
void GRAdrawpoly(int GC,int num,int *point,int fil);
void GRAlines(int GC,int num,int *point);
void GRAmark(int GC,int type,int x0,int y0,int size,
              int fr,int fg,int fb,int br,int bg,int bb);
void GRAtextstyle(int GC,char *font,int size,int space,int dir);
void GRAouttext(int GC,char *s);
void GRAoutkanji(int GC,char *s);
char *GRAexpandtext(char *s);
void GRAdrawtext(int GC,char *s,char *font,char *jfont,
                 int size,int space,int dir,int scriptsize);
void GRAdrawtextraw(int GC,char *s,char *font,char *jfont,
                 int size,int space,int dir);
void GRAtextextent(char *s,char *font,char *jfont,
                 int size,int space,int scriptsize,
                 int *gx0,int *gy0,int *gx1,int *gy1,int raw);
void GRAtextextentraw(char *s,char *font,char *jfont,
                 int size,int space,int *gx0,int *gy0,int *gx1,int *gy1);
int GRAlineclip(int GC,int *x0,int *y0,int *x1,int *y1);
int GRArectclip(int GC,int *x0,int *y0,int *x1,int *y1);
int GRAinview(int GC,int x,int y);
int GRAinput(int GC,char *s,int leftm,int topm,int rate);
int GRAinputold(int GC,char *s,int leftm,int topm,int rate,int greek);
void GRAcurvefirst(int GC,int num,int *dashlist,
      clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
             double x0,double y0);
int GRAcurve(int GC,double c[],double x0,double y0);
void GRAdashmovetod(int GC,double x1,double y1);
void GRAdashlinetod(int GC,double x1,double y1);
void GRAcmatchfirst(int pointx,int pointy,int err,
                   clipfunc clipf,transfunc transf,diffunc diff,intpfunc intpf,void *local,
                   struct cmatchtype *data,int bbox,double x0,double y0);
void GRAcmatchtod(double x,double y,struct cmatchtype *data);
int GRAcmatch(double c[],double x0,double y0,struct cmatchtype *data);
void GRAinitbbox(struct GRAbbox *bbox);
void GRAendbbox(struct GRAbbox *bbox);
int GRAboundingbox(char code,int *cpar,char *cstr,void *local);

