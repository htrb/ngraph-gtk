/*
 * $Id: ox11menu.h,v 1.37 2010-03-04 08:30:17 hito Exp $
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
#include "nhash.h"

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

enum paper_id {
  PAPER_ID_A3,
  PAPER_ID_A4,
  PAPER_ID_A5,
  PAPER_ID_B4,
  PAPER_ID_B5,
  PAPER_ID_LETTER,
  PAPER_ID_LEGAL,
  PAPER_ID_NORMAL,
  PAPER_ID_WIDE,
  PAPER_ID_CUSTOM,
};

struct script
{
  char *name;
  char *script;
  char *description;
  char *option;
  struct script *next;
};

struct character_map_list {
  char *title, *data;
  struct character_map_list *next;
};

struct layer {
  cairo_surface_t *pix;
  cairo_t *cairo;
};

struct menulocal
{
  cairo_surface_t *pix, *bg;
  NHASH layers;
  int redrawf, redrawf_num;
  int windpi, data_head_lines;
  int grid, show_grid;
  int modified;
  int lock;
  struct gra2cairo_local *local;
  int antialias;
  char *editor, *browser, *help_browser, *help_file;
  struct objlist *obj;
  N_VALUE *inst;
  struct objlist *GRAobj;
  int GRAoid;
  int GC;
  int PaperWidth, PaperHeight, PaperLandscape;
  char *PaperName;
  enum paper_id PaperId;
  int LeftMargin, TopMargin;
  int PaperZoom;
  struct narray drawrable;
  struct script *scriptroot, *addin_list;
  int menuheight, menuwidth, menux, menuy;
  int exwindpi, exwinwidth, exwinheight, exwin_use_external;
  char *fileopendir, *graphloaddir, *expanddir, *coordwin_font, *infowin_font, *file_preview_font;
  int expand, loadpath, expandtofullpath, changedirectory, savehistory;
  int savepath, savewithdata, savewithmerge;
  int scriptconsole, addinconsole;
  int statusbar, sidebar, ruler, scrollbar, ctoolbar, ptoolbar, show_cross, showtip, preserve_width;
  int hist_size, info_size;
  double bg_r, bg_g, bg_b;
  int focus_frame_type, use_opacity, select_data;
  int side_pane1_pos, side_pane2_pos, side_pane3_pos, main_pane_pos;
  int file_tab, axis_tab, merge_tab, path_tab, rectangle_tab, arc_tab, mark_tab, text_tab, parameter_tab;
#ifdef WINDOWS
  int emf_dpi;
#endif
  int png_dpi, ps_version, svg_version;
  struct character_map_list *char_map;
  int use_custom_palette;
  struct narray custom_palette;
  char *source_style_id;
  int math_input_mode;
};

extern struct menulocal Menulocal;

enum SAVE_CONFIG_TYPE {
  SAVE_CONFIG_TYPE_GEOMETRY        = 0x0001,
  SAVE_CONFIG_TYPE_VIEWER          = 0x0002,
  SAVE_CONFIG_TYPE_ADDIN_SCRIPT    = 0x0008,
  SAVE_CONFIG_TYPE_MISC            = 0x0010,
  SAVE_CONFIG_TYPE_EXTERNAL_VIEWER = 0x0020,
  SAVE_CONFIG_TYPE_FONTS           = 0x0040,
  SAVE_CONFIG_TYPE_TOGGLE_VIEW     = 0x0080,
  SAVE_CONFIG_TYPE_OTHERS          = 0x0100,
};

#define SAVE_CONFIG_TYPE_X11MENU 	(SAVE_CONFIG_TYPE_GEOMETRY		\
				  | SAVE_CONFIG_TYPE_VIEWER		\
				  | SAVE_CONFIG_TYPE_EXTERNAL_VIEWER	\
				  | SAVE_CONFIG_TYPE_ADDIN_SCRIPT	\
				  | SAVE_CONFIG_TYPE_MISC)

void mx_redraw(struct objlist *obj, N_VALUE *inst, char const **objects);
void mx_clear(cairo_region_t *region, char const **objects);
void mgtkdisplaydialog(const char *str);
void mgtkdisplaystatus(const char *str);
int mgtkputstderr(const char *s);
int mgtkprintfstderr(const char *fmt, ...);
int mgtkinterrupt(void);
int mgtkinputyn(const char *mes);
int mgtkputstdout(const char *s);
int mgtkprintfstdout(const char *fmt, ...);
void initwindowconfig(void);
int mgtkwindowconfig(void);
void menuadddrawrable(struct objlist *parent, struct narray *drawrable);
int menu_save_config(int type);
void main_window_redraw(void);
void init_layer(const char *obj);

#endif
