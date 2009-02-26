/* 
 * $Id: ox11menu.h,v 1.25 2009/02/26 08:51:49 hito Exp $
 * 
 * This file is part of "Ngraph for GTK".
 * 
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for GTK" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * "Ngraph for GTK" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#ifndef _O_X11_MENU_HEADER
#define _O_X11_MENU_HEADER

#define LINETOLIMIT 500

#define CW_USEDEFAULT -100000

#define ERRRUN 100
#define ERRCMAP 101
#define ERRFONT 102

#define HIST_SIZE_MAX 10000
#define INFOWIN_SIZE_MAX 10000

extern int GlobalLock;
extern struct savedstdio GtkIOSave;

extern void mgtkdisplaydialog(char *str);
extern void mgtkdisplaystatus(char *str);
extern int mgtkputstderr(char *s);
extern int mgtkprintfstderr(char *fmt, ...);
extern int mgtkputstdout(char *s);
extern int mgtkprintfstdout(char *fmt, ...);
extern int mgtkinterrupt(void);
extern int mgtkinputyn(char *mes);

enum paper_id {
  PAPER_ID_A3,
  PAPER_ID_A4,
  PAPER_ID_A5,
  PAPER_ID_B4,
  PAPER_ID_B5,
  PAPER_ID_LETTER,
  PAPER_ID_LEGAL,
  PAPER_ID_CUSTOM,
};

struct extprinter
{
  char *name;
  char *driver;
  char *ext;
  char *option;
  struct extprinter *next;
};

struct prnprinter
{
  char *name;
  char *driver;
  char *option;
  char *prn;
  struct prnprinter *next;
};

struct script
{
  char *name;
  char *script;
  char *description;
  char *option;
  struct script *next;
};

struct menulocal
{
  GdkDrawable *win, *pix;
  GdkGC *gc;
  int scrollx, scrolly;
  int autoredraw, redrawf, redrawf_num;
  int windpi, data_head_lines;
  int grid;
  double pixel_dot, offsetx, offsety;
  GdkRegion *region;
  int lock;
  struct gra2cairo_local *local;
  int antialias;
  char *editor, *browser, *help_browser;
  struct objlist *obj;
  char *inst;
  struct objlist *GRAobj;
  int GRAoid;
  char *GRAinst;
  int GC;
  int PaperWidth, PaperHeight, PaperLandscape;
  char *PaperName;
  enum paper_id PaperId;
  int LeftMargin, TopMargin;
  int PaperZoom;
  struct narray drawrable;
  struct extprinter *extprinterroot;
  struct prnprinter *prnprinterroot;
  struct script *scriptroot;
  int menuheight, menuwidth, menux, menuy;
  int fileheight, filewidth, filex, filey, fileopen;
  int axisheight, axiswidth, axisx, axisy, axisopen;
  int legendheight, legendwidth, legendx, legendy, legendopen;
  int mergeheight, mergewidth, mergex, mergey, mergeopen;
  int dialogheight, dialogwidth, dialogx, dialogy, dialogopen;
  int coordheight, coordwidth, coordx, coordy, coordopen;
  int exwindpi, exwinwidth, exwinheight, exwin_use_external;
  char *fileopendir, *graphloaddir, *expanddir, *coordwin_font;
  int expand, ignorepath, expandtofullpath, changedirectory, savehistory;
  int savepath, savewithdata, savewithmerge;
  struct narray *ngpfilelist;
  struct narray *ngpdirlist;
  struct narray *datafilelist;
  int scriptconsole, addinconsole;
  int statusb, ruler, showtip, preserve_width;
  int hist_size, info_size, bg_r, bg_g, bg_b;
  int focus_frame_type;
};

extern struct menulocal Menulocal;

int mxd2p(int r);
int mxd2px(int x);
int mxd2py(int y);
int mxp2d(int r);
unsigned long RGB(int R, int G, int B);
void mx_redraw(struct objlist *obj, char *inst);
void mx_clear(GdkRegion *region);
void mx_inslist(struct objlist *obj, char *inst,
		struct objlist *aobj, char *ainst, char *afield, int addn);
void mx_dellist(struct objlist *obj, char *inst, int deln);
void mgtkdisplaydialog(char *str);
void mgtkdisplaystatus(char *str);
int mgtkputstderr(char *s);
int mgtkprintfstderr(char *fmt, ...);
int mgtkinterrupt(void);
int mgtkinputyn(char *mes);
void initwindowconfig(void);
int mgtkwindowconfig(void);
void menuadddrawrable(struct objlist *parent, struct narray *drawrable);
int get_graph_modified(void);
void set_graph_modified(void);
void reset_graph_modified(void);

#endif
