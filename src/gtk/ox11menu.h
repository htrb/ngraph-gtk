/* 
 * $Id: ox11menu.h,v 1.38 2010/04/01 06:08:23 hito Exp $
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

#define DEFAULT_GEOMETRY -100000

#define ERRRUN 100
#define ERRCMAP 101
#define ERRFONT 102

#define HIST_SIZE_MAX 10000
#define INFOWIN_SIZE_MAX 10000

extern int Globallock;
extern struct savedstdio GtkIOSave;

extern void mgtkdisplaydialog(const char *str);
extern void mgtkdisplaystatus(const char *str);
extern int mgtkputstderr(const char *s);
extern int mgtkprintfstderr(char *fmt, ...);
extern int mgtkputstdout(const char *s);
extern int mgtkprintfstdout(char *fmt, ...);
extern int mgtkinterrupt(void);
extern int mgtkinputyn(const char *mes);

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

struct script
{
  char *name;
  char *script;
  char *description;
  char *option;
  struct script *next;
};

struct prnprinter
{
  char *name;
  char *driver;
  char *option;
  char *prn;
  struct prnprinter *next;
};

struct menulocal
{
  GdkDrawable *win, *pix;
  GdkGC *gc;
  int scrollx, scrolly;
  int redrawf, redrawf_num;
  int windpi, data_head_lines;
  int grid;
  double pixel_dot, offsetx, offsety;
  GdkRegion *region;
  int lock;
  struct gra2cairo_local *local;
  int antialias, do_not_use_arc_for_draft;
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
  struct script *scriptroot;
  int menuheight, menuwidth, menux, menuy;
  int fileheight, filewidth, filex, filey, fileopen;
  int axisheight, axiswidth, axisx, axisy, axisopen;
  int legendheight, legendwidth, legendx, legendy, legendopen;
  int mergeheight, mergewidth, mergex, mergey, mergeopen;
  int dialogheight, dialogwidth, dialogx, dialogy, dialogopen;
  int coordheight, coordwidth, coordx, coordy, coordopen;
  int exwindpi, exwinwidth, exwinheight, exwin_use_external;
  char *fileopendir, *graphloaddir, *expanddir, *coordwin_font, *infowin_font, *file_preview_font;
  int expand, ignorepath, expandtofullpath, changedirectory, savehistory;
  int savepath, savewithdata, savewithmerge;
  GtkRecentManager *ngpfilelist;
  struct narray *datafilelist;
  int scriptconsole, addinconsole;
  int statusb, ruler, scrollbar, ctoolbar, ptoolbar, show_cross, showtip, preserve_width;
  int hist_size, info_size, bg_r, bg_g, bg_b;
  int focus_frame_type;
};

extern struct menulocal Menulocal;

enum SAVE_CONFIG_TYPE {
  SAVE_CONFIG_TYPE_GEOMETRY        = 0x0001,
  SAVE_CONFIG_TYPE_CHILD_GEOMETRY  = 0x0002,
  SAVE_CONFIG_TYPE_VIEWER          = 0x0004,
  SAVE_CONFIG_TYPE_EXTERNAL_DRIVER = 0x0008,
  SAVE_CONFIG_TYPE_ADDIN_SCRIPT    = 0x0010,
  SAVE_CONFIG_TYPE_MISC            = 0x0020,
  SAVE_CONFIG_TYPE_EXTERNAL_VIEWER = 0x0040,
  SAVE_CONFIG_TYPE_FONTS           = 0x0080,
  SAVE_CONFIG_TYPE_TOGGLE_VIEW     = 0x0100,
};

#define SAVE_CONFIG_TYPE_X11MENU 	(SAVE_CONFIG_TYPE_GEOMETRY		\
				  | SAVE_CONFIG_TYPE_CHILD_GEOMETRY	\
				  | SAVE_CONFIG_TYPE_VIEWER		\
				  | SAVE_CONFIG_TYPE_EXTERNAL_DRIVER	\
				  | SAVE_CONFIG_TYPE_ADDIN_SCRIPT	\
				  | SAVE_CONFIG_TYPE_MISC)

void mx_redraw(struct objlist *obj, char *inst);
void mx_clear(GdkRegion *region);
void mx_inslist(struct objlist *obj, char *inst,
		struct objlist *aobj, char *ainst, char *afield, int addn);
void mx_dellist(struct objlist *obj, char *inst, int deln);
void mgtkdisplaydialog(const char *str);
void mgtkdisplaystatus(const char *str);
int mgtkputstderr(const char *s);
int mgtkprintfstderr(char *fmt, ...);
int mgtkinterrupt(void);
int mgtkinputyn(const char *mes);
void initwindowconfig(void);
int mgtkwindowconfig(void);
void menuadddrawrable(struct objlist *parent, struct narray *drawrable);
int get_graph_modified(void);
void set_graph_modified(void);
void reset_graph_modified(void);
int menu_save_config(int type);

#endif
