/* 
 * $Id: ox11menu.c,v 1.53 2009/02/26 04:39:12 hito Exp $
 * 
 * This file is part of "Ngraph for GTK".
 * 
 * Copyright (C) 2002,  Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for GTK" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License,  or (at your option) any later version.
 * 
 * "Ngraph for GTK" is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  write to the Free Software
 * Foundation,  Inc.,  59 Temple Place - Suite 330,  Boston,  MA  02111-1307,  USA.
 * 
 */


#include "gtk_common.h"

#include <X11/Xlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "jnstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "strconv.h"

#include "main.h"
#include "ogra2cairo.h"
#include "ogra2x11.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11view.h"
#include "x11graph.h"
#include "x11print.h"

#define NAME "menu"
#define ALIAS "winmenu:gtkmenu"
#define PARENT "gra2cairo"
#define NVERSION  "1.00.00"
#define MGTKCONF "[x11menu]"
#define G2WINCONF "[gra2gtk]"


static char *menuerrorlist[] = {
  "already running.",
  "not enough color cell.",
  "cannot create font. Check `windowfont' resource.",
  "cannot open display.",
};

#define ERRNUM (sizeof(menuerrorlist) / sizeof(*menuerrorlist))

enum {
  ERR_MENU_RUN = 100,
  ERR_MENU_COLOR,
  ERR_MENU_FONT,
  ERR_MENU_DISPLAY,
};

struct menulocal Menulocal;
struct mxlocal *Mxlocal;
struct savedstdio GtkIOSave;

static int mxflush(struct objlist *obj, char *inst, char *rval, int argc,
		   char **argv);


enum menu_config_type {
  MENU_CONFIG_TYPE_NUMERIC,
  MENU_CONFIG_TYPE_BOOL,
  MENU_CONFIG_TYPE_STRING,
  MENU_CONFIG_TYPE_WINDOW,
  MENU_CONFIG_TYPE_HISTORY,
  MENU_CONFIG_TYPE_OTHER,
};

static int menu_config_set_four_elements(char *s2, void *data);
static int menu_config_set_window_geometry(char *s2, void *data);
static int menu_config_set_bgcolor(char *s2, void *data);
static int menu_config_set_ext_driver(char *s2, void *data);
static int menu_config_set_prn_driver(char *s2, void *data);
static int menu_config_set_script(char *s2, void *data);

static int *menu_config_menu_geometry[] = {
  &Menulocal.menux,
  &Menulocal.menuy,
  &Menulocal.menuwidth,
  &Menulocal.menuheight,
};

static int *menu_config_file_geometry[] = {
  &(Menulocal.filex),
  &Menulocal.filey,
  &Menulocal.filewidth,
  &Menulocal.fileheight,
  &Menulocal.fileopen,
};

static int *menu_config_axis_geometry[] = {
  &Menulocal.axisx,
  &Menulocal.axisy,
  &Menulocal.axiswidth,
  &Menulocal.axisheight,
  &Menulocal.axisopen,
};

static int *menu_config_legend_geometry[] = {
  &Menulocal.legendx,
  &Menulocal.legendy,
  &Menulocal.legendwidth,
  &Menulocal.legendheight,
  &Menulocal.legendopen,
};

static int *menu_config_merge_geometry[] = {
  &Menulocal.mergex,
  &Menulocal.mergey,
  &Menulocal.mergewidth,
  &Menulocal.mergeheight,
  &Menulocal.mergeopen,
};

static int *menu_config_dialog_geometry[] = {
  &Menulocal.dialogx,
  &Menulocal.dialogy,
  &Menulocal.dialogwidth,
  &Menulocal.dialogheight,
  &Menulocal.dialogopen,
};

static int *menu_config_coord_geometry[] = {
  &Menulocal.coordx,
  &Menulocal.coordy,
  &Menulocal.coordwidth,
  &Menulocal.coordheight,
  &Menulocal.coordopen,
};

struct menu_config {
  char *name;
  enum menu_config_type type;
  int (* proc)(char *, void *);
  void *data;
};
static struct menu_config MenuConfig[] = {
  {"script_console",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.scriptconsole},
  {"addin_console",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.addinconsole},
  {"preserve_width",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.preserve_width},
  {"change_directory",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.changedirectory},
  {"save_history",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savehistory},
  {"save_path",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savepath},
  {"save_with_data",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithdata},
  {"save_with_merge",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithmerge},
  {"status_bar",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.statusb},
  {"viewer_show_ruler",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ruler},
  {"show_tip",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.showtip},
  {"expand",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.expand},
  {"ignore_path",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ignorepath},
  {"expand_to_fullpath",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.expandtofullpath},
  {"history_size",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.hist_size},
  {"infowin_size",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.info_size},
  {"focus_frame_type",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.focus_frame_type},

  {"expand_dir",		MENU_CONFIG_TYPE_STRING, NULL, &Menulocal.expanddir},
  {"editor",			MENU_CONFIG_TYPE_STRING, NULL, &Menulocal.editor},
  {"browser",			MENU_CONFIG_TYPE_STRING, NULL, &Menulocal.browser},
  {"help_browser",		MENU_CONFIG_TYPE_STRING, NULL, &Menulocal.help_browser},
  {"coordwin_font",		MENU_CONFIG_TYPE_STRING, NULL, &Menulocal.coordwin_font},

  {"ngp_history",		MENU_CONFIG_TYPE_HISTORY, NULL, &Menulocal.ngpfilelist},
  {"ngp_dir_history",		MENU_CONFIG_TYPE_HISTORY, NULL, &Menulocal.ngpdirlist},
  {"data_history",		MENU_CONFIG_TYPE_HISTORY, NULL, &Menulocal.datafilelist},

  {"background_color",		MENU_CONFIG_TYPE_OTHER, menu_config_set_bgcolor,       NULL},
  {"ext_driver",		MENU_CONFIG_TYPE_OTHER, menu_config_set_ext_driver,    NULL},
  {"prn_driver",		MENU_CONFIG_TYPE_OTHER, menu_config_set_prn_driver,    NULL},
  {"script",			MENU_CONFIG_TYPE_OTHER, menu_config_set_script,        NULL},
  {"menu_win",			MENU_CONFIG_TYPE_OTHER, menu_config_set_four_elements, menu_config_menu_geometry},

  {"file_win",			MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_file_geometry},
  {"axis_win",			MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_axis_geometry},
  {"legend_win",		MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_legend_geometry},
  {"merge_win",			MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_merge_geometry},
  {"information_win",		MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_dialog_geometry},
  {"coordinate_win",		MENU_CONFIG_TYPE_WINDOW, NULL, menu_config_coord_geometry},

  {"antialias",				MENU_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"viewer_dpi",			MENU_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"viewer_load_file_data_number",	MENU_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"viewer_grid",			MENU_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"data_head_lines",			MENU_CONFIG_TYPE_NUMERIC, NULL, NULL},

  {"viewer_auto_redraw",		MENU_CONFIG_TYPE_BOOL, NULL, NULL},
  {"viewer_load_file_on_redraw",	MENU_CONFIG_TYPE_BOOL, NULL, NULL},
};

static NHASH MenuConfigHash = NULL;

static int
menu_config_set_four_elements(char *s2, void *data)
{
  int len, i, val, **ary;
  char *endptr, *f[] = {NULL, NULL, NULL, NULL};

  if (data == NULL)
    return 0;

  ary = (int **) data;

  for (i = 0; i < 4; i++) {
    f[i] = getitok2(&s2, &len, " \t,");
    if (f[i] == NULL)
      goto End;
  }

  for (i = 0; i < 4; i++) {
    val = strtol(f[i], &endptr, 10);
    if (endptr[0] == '\0' && val != 0) {
      *(ary[i]) = val;
    }
  }

 End:
  for (i = 0; i < 4; i++) {
    memfree(f[i]);
  }
  return 0;
}

static int
menu_config_set_window_geometry(char *s2, void *data)
{
  int len, i, val, **ary;
  char *endptr, *f[] = {NULL, NULL, NULL, NULL, NULL};

  if (data == NULL)
    return 0;

  ary = (int **) data;

  for (i = 0; i < 5; i++) {
    f[i] = getitok2(&s2, &len, " \t,");
    if (f[i] == NULL)
      goto End;
  }

  for (i = 0; i < 5; i++) {
    val = strtol(f[i], &endptr, 10);
    if (endptr[0] == '\0') {
      *(ary[i]) = val;
    }
  }

 End:
  for (i = 0; i < 5; i++) {
    memfree(f[i]);
  }
  return 0;
}

static int
menu_config_set_bgcolor(char *s2, void *data)
{
  char *f1, *endptr;
  int len, val;

  f1 = getitok2(&s2, &len, " \t,");
  val = strtol(f1, &endptr, 16);
  if (endptr[0] == '\0') {
    Menulocal.bg_r = (val >> 16) & 0xff;
    Menulocal.bg_g = (val >> 8) & 0xff;
    Menulocal.bg_b = val & 0xff;
  }
  memfree(f1);
  return 0;
}

static int
menu_config_set_ext_driver(char *s2, void *data)
{
  char *f[4] = {NULL, NULL, NULL, NULL};
  int len, i;
  struct extprinter *pnew, *pcur, **pptr;

  pptr = (struct extprinter **) data;
  pcur = *pptr;

  f[0] = getitok2(&s2, &len, ",");
  f[1] = getitok2(&s2, &len, ",");

  if (s2[1] == ',') {
    f[2] = NULL;
  } else {
    f[2] = getitok2(&s2, &len, ",");
  }

  for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);

  f[3] = getitok2(&s2, &len, "");

  if (f[0] && f[1]) {
    pnew = (struct extprinter *) memalloc(sizeof(struct extprinter));
    if (pnew == NULL) {
      for (i = 0; i < 4; i++) {
	memfree(f[i]);
      }
      return 1;
    }
    if (pcur == NULL) {
      Menulocal.extprinterroot = pnew;
    } else {
      pcur->next = pnew;
    }
    *pptr = pnew;
    pcur = pnew;
    pcur->next = NULL;
    pcur->name = f[0];
    pcur->driver = f[1];
    pcur->ext = f[2];
    pcur->option = f[3];
  } else {
    for (i = 0; i < 4; i++) {
      memfree(f[i]);
    }
  }
  return 0;
}

static int
menu_config_set_prn_driver(char *s2, void *data)
{
  char *f[4] = {NULL, NULL, NULL, NULL};
  int len, i;
  struct prnprinter *pnew, *pcur, **pptr;

  pptr = (struct prnprinter **) data;
  pcur = *pptr;

  f[0] = getitok2(&s2, &len, ",");
  f[1] = getitok2(&s2, &len, ",");
  f[2] = getitok2(&s2, &len, ",");

  for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);

  f[3] = getitok2(&s2, &len, "");

  if (f[0] && f[1]) {
    pnew = (struct prnprinter *) memalloc(sizeof(struct prnprinter));
    if (pnew == NULL) {
      for (i = 0; i < 4; i++) {
	memfree(f[i]);
      }
      return 1;
    }
    if (pcur == NULL) {
      Menulocal.prnprinterroot = pnew;
    } else {
      pcur->next = pnew;
    }
    *pptr = pnew;
    pcur = pnew;
    pcur->next = NULL;
    pcur->name = f[0];
    pcur->driver = f[1];
    pcur->prn = f[2];
    pcur->option = f[3];
  } else {
    for (i = 0; i < 4; i++) {
      memfree(f[i]);
    }
  }
  return 0;
}

static int
menu_config_set_script(char *s2, void *data)
{
  char *f[] = {NULL, NULL, NULL, NULL};
  int len, i;
  struct script *snew, *scur, **sptr;

  sptr = (struct script **) data;
  scur = *sptr;

  f[0] = getitok2(&s2, &len, ",");
  f[1] = getitok2(&s2, &len, ",");
  f[2] = getitok2(&s2, &len, ",");

  for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
  f[3] = getitok2(&s2, &len, ",");

  if (f[0] && f[1]) {
    snew = (struct script *) memalloc(sizeof(struct script));
    if (snew == NULL) {
      for (i = 0; i < sizeof(f) / sizeof(*f); i++) {
	memfree(f[i]);
      }
      return 1;
    }
    if (scur == NULL) {
      Menulocal.scriptroot = snew;
    } else {
      scur->next = snew;
    }
    *sptr = snew;
    scur = snew;
    scur->next = NULL;
    scur->name = f[0];
    scur->script = f[1];
    scur->description = f[2];
    scur->option = f[3];
  } else {
    for (i = 0; i < sizeof(f) / sizeof(*f); i++) {
      memfree(f[i]);
    }
  }
  return 0;
}

static int
mgtkloadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1;
  int val;
  char *endptr;
  int len;
  struct extprinter *pcur;
  struct prnprinter *pcur2;
  struct script *scur;
  struct menu_config *cfg;

  fp = openconfig(MGTKCONF);
  if (fp == NULL)
    return 0;

  pcur = Menulocal.extprinterroot;
  pcur2 = Menulocal.prnprinterroot;
  scur = Menulocal.scriptroot;

  if (nhash_get_ptr(MenuConfigHash, "ext_driver", (void *) &cfg) == 0) {
    if (cfg) {
      cfg->data = &pcur;
    }
  }

  if (nhash_get_ptr(MenuConfigHash, "prn_driver", (void *) &cfg) == 0) {
    if (cfg) {
      cfg->data = &pcur2;
    }
  }

  if (nhash_get_ptr(MenuConfigHash, "script", (void *) &cfg) == 0) {
    if (cfg) {
      cfg->data = &scur;
    }
  }


  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (nhash_get_ptr(MenuConfigHash, tok, (void *) &cfg) == 0 && cfg) {
      switch (cfg->type) {
      case MENU_CONFIG_TYPE_NUMERIC:
	f1 = getitok2(&s2, &len, " \t,");
	if (f1) {
	  val = strtol(f1, &endptr, 10);
	  if (endptr[0] == '\0')
	    * (int *) (cfg->data) = val;
	}
	memfree(f1);
	break;
      case MENU_CONFIG_TYPE_BOOL:
	f1 = getitok2(&s2, &len, " \t,");
	if (f1) {
	  val = strtol(f1, &endptr, 10);
	  if (endptr[0] == '\0')
	    * (int *) (cfg->data) = (val != 0);
	}
	memfree(f1);
	break;
      case MENU_CONFIG_TYPE_STRING:
	f1 = getitok2(&s2, &len, "");
	if (f1) {
	  memfree(* (char **) (cfg->data));
	  * (char **) (cfg->data) = f1;
	}
	break;
      case MENU_CONFIG_TYPE_WINDOW:
	menu_config_set_window_geometry(s2, cfg->data);
	break;
      case MENU_CONFIG_TYPE_HISTORY:
	for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
	f1 = getitok2(&s2, &len, "");
	if (f1)
	  arrayadd(* (struct narray **) cfg->data, &f1);
	break;
      case MENU_CONFIG_TYPE_OTHER:
	if (cfg->proc && cfg->proc(s2, cfg->data)) {
	  memfree(tok);
	  memfree(str);
	  closeconfig(fp);
	  return 1;
	}
	break;
      }
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

void
initwindowconfig(void)
{
  Menulocal.fileopen =
    Menulocal.axisopen =
    Menulocal.legendopen  =
    Menulocal.mergeopen =
    Menulocal.dialogopen =
    Menulocal.coordopen = FALSE;

  Menulocal.filex =
    Menulocal.filey =
    Menulocal.fileheight =
    Menulocal.filewidth = CW_USEDEFAULT;

  Menulocal.axisx =
    Menulocal.axisy =
    Menulocal.axisheight =
    Menulocal.axiswidth = CW_USEDEFAULT;

  Menulocal.legendx =
    Menulocal.legendy =
    Menulocal.legendheight =
    Menulocal.legendwidth = CW_USEDEFAULT;

  Menulocal.mergex =
    Menulocal.mergey =
    Menulocal.mergeheight =
    Menulocal.mergewidth = CW_USEDEFAULT;

  Menulocal.dialogx =
    Menulocal.dialogy =
    Menulocal.dialogheight =
    Menulocal.dialogwidth = CW_USEDEFAULT;

  Menulocal.coordx =
    Menulocal.coordy =
    Menulocal.coordheight =
    Menulocal.coordwidth = CW_USEDEFAULT;
}

int
mgtkwindowconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  struct menu_config *cfg;

  if ((fp = openconfig(MGTKCONF)) == NULL)
    return 0;
  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (nhash_get_ptr(MenuConfigHash, tok, (void *) &cfg) == 0) {
      if(cfg && cfg->type == MENU_CONFIG_TYPE_WINDOW) {
	menu_config_set_window_geometry(s2, cfg->data);
      }
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

static int
exwinloadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1;
  int val;
  char *endptr;
  int len;

  fp = openconfig(G2WINCONF);
  if (fp == NULL)
    return 0;

  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "win_dpi") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwindpi = val;
      memfree(f1);
    } else if (strcmp(tok, "win_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinwidth = val;
      memfree(f1);
    } else if (strcmp(tok, "win_height") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinheight = val;
      memfree(f1);
    } else if (strcmp(tok, "use_external_viewer") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwin_use_external = val;
      memfree(f1);
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

void
menuadddrawrable(struct objlist *parent, struct narray *drawrable)
{
  struct objlist *ocur;
  char *name;

  ocur = chkobjroot();
  while (ocur) {
    if (chkobjparent(ocur) == parent) {
      name = chkobjectname(ocur);
      arrayadd2(drawrable, &name);
      menuadddrawrable(ocur, drawrable);
    }
    ocur = ocur->next;
  }
}

static void
set_menu_config_mxlocal(void)
{
  struct menu_config *cfg;

  if (nhash_get_ptr(MenuConfigHash, "antialias", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->antialias);
  }
  if (nhash_get_ptr(MenuConfigHash, "viewer_dpi", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->windpi);
  }
  if (nhash_get_ptr(MenuConfigHash, "viewer_load_file_data_number", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->redrawf_num);
  }
  if (nhash_get_ptr(MenuConfigHash, "viewer_grid", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->grid);
  }
  if (nhash_get_ptr(MenuConfigHash, "data_head_lines", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->data_head_lines);
  }
  if (nhash_get_ptr(MenuConfigHash, "viewer_auto_redraw", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->autoredraw);
  }
  if (nhash_get_ptr(MenuConfigHash, "viewer_load_file_on_redraw", (void *) &cfg) == 0) {
    cfg->data = &(Mxlocal->redrawf);
  }
}

static int
menuinit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct objlist *robj;
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;
  struct gra2cairo_local *local;
  int i, numf, numd;
  char *dum;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (_getobj(obj, "_local", inst, &local)) {
    local = gra2cairo_free(obj, inst);
    memfree(local);
    return 1;
  }

  Mxlocal = (struct mxlocal *) memalloc(sizeof(struct mxlocal));
  if (Mxlocal == NULL) {
    local = gra2cairo_free(obj, inst);
    memfree(local);
    return 1;
  }

  set_menu_config_mxlocal();

  Menulocal.menux = Menulocal.menuy
    = Menulocal.menuheight = Menulocal.menuwidth = CW_USEDEFAULT;
  initwindowconfig();
  Menulocal.showtip = TRUE;
  Menulocal.statusb = TRUE;
  Menulocal.ruler = TRUE;
  Menulocal.scriptconsole = FALSE;
  Menulocal.addinconsole = TRUE;
  Menulocal.changedirectory = 1;
  Menulocal.editor = NULL;
  Menulocal.browser = NULL;
  Menulocal.coordwin_font = NULL;
  Menulocal.help_browser = NULL;
  set_paper_type(21000, 29700);
  Menulocal.LeftMargin = 0;
  Menulocal.TopMargin = 0;
  Menulocal.PaperZoom = 10000;
  Menulocal.exwindpi = DEFAULT_DPI;
  Menulocal.exwinwidth = 0;
  Menulocal.exwinheight = 0;
  Menulocal.fileopendir = NULL;
  Menulocal.graphloaddir = NULL;
  Menulocal.expand = 1;
  Menulocal.expanddir = (char *) memalloc(3);
  strcpy(Menulocal.expanddir, "./");
  Menulocal.ignorepath = 0;
  Menulocal.expandtofullpath=TRUE;
  Menulocal.savepath = 0;
  Menulocal.savewithdata = 0;
  Menulocal.savewithmerge = 0;
  Menulocal.ngpfilelist = arraynew(sizeof(char *));
  Menulocal.ngpdirlist = arraynew(sizeof(char *));
  Menulocal.datafilelist = arraynew(sizeof(char *));
  Menulocal.GRAobj = chkobject("gra");
  Menulocal.extprinterroot = NULL;
  Menulocal.prnprinterroot = NULL;
  Menulocal.scriptroot = NULL;
  Menulocal.hist_size = 1000;
  Menulocal.info_size = 1000;
  Menulocal.bg_r = 0xff;
  Menulocal.bg_g = 0xff;
  Menulocal.bg_b = 0xff;
  Menulocal.focus_frame_type = GDK_LINE_ON_OFF_DASH;

  arrayinit(&(Menulocal.drawrable), sizeof(char *));
  menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));

  Mxlocal->windpi = DEFAULT_DPI;
  Mxlocal->autoredraw = TRUE;
  Mxlocal->redrawf = TRUE;
  Mxlocal->redrawf_num = 0xff;
  Mxlocal->grid = 200;
  Mxlocal->data_head_lines = 20;
  Mxlocal->local = local;

  if (_putobj(obj, "_gtklocal", inst, Mxlocal))
    goto errexit;

  if (mgtkloadconfig())
    goto errexit;

  if (exwinloadconfig())
    goto errexit;

  Mxlocal->local->antialias = Mxlocal->antialias;
  if (_putobj(obj, "antialias", inst, &(Mxlocal->antialias)))
    goto errexit;

  numf = arraynum(Menulocal.ngpfilelist);
  numd = arraynum(Menulocal.ngpdirlist);
  dum = NULL;

  if (numd > numf) {
    for (i = numf; i < numd; i++) {
      arrayndel2(Menulocal.ngpdirlist, i);
    }
  } else if (numd < numf) {
    for (i = numd; i < numf; i++) {
      arrayadd(Menulocal.ngpdirlist, &dum);
    }
  }

  if (Menulocal.exwindpi < 1)
    Menulocal.exwindpi = DEFAULT_DPI;

  if (Menulocal.exwindpi > DPI_MAX)
    Menulocal.exwindpi = DPI_MAX;

  if (Mxlocal->windpi < 1)
    Mxlocal->windpi = DEFAULT_DPI;

  if (Mxlocal->windpi > DPI_MAX)
    Mxlocal->windpi = DPI_MAX;

  if (_putobj(obj, "dpi", inst, &(Mxlocal->windpi)))
    goto errexit;

  if (_putobj(obj, "data_head_lines", inst, &(Mxlocal->data_head_lines)))
    goto errexit;

  if (_putobj(obj, "auto_redraw", inst, &(Mxlocal->autoredraw)))
    goto errexit;

  if (_putobj(obj, "redraw_flag", inst, &(Mxlocal->redrawf)))
    goto errexit;

  if (_putobj(obj, "redraw_num", inst, &(Mxlocal->redrawf_num)))
    goto errexit;

  i = 0;
  if (_putobj(obj, "modified", inst, &i))
    goto errexit;

  if (!chkobjfield(obj, "_output")) {
    Menulocal.output = getobjtblpos(obj, "_output", &robj);
    if (Menulocal.output == -1)
      goto errexit;
    Menulocal.outputobj = robj;
  } else {
    Menulocal.output = -1;
  }

  Menulocal.obj = obj;
  Menulocal.inst = inst;
  Mxlocal->pix = NULL;
  Mxlocal->win = NULL;
  Mxlocal->gc = NULL;
  Mxlocal->lock = 0;

  if (!OpenApplication()) {
    error(obj, ERR_MENU_DISPLAY);
    goto errexit;
  }

  return 0;

errexit:
  memfree(Menulocal.editor);
  memfree(Menulocal.browser);
  memfree(Menulocal.help_browser);
  pcur = Menulocal.extprinterroot;
  while (pcur) {
    pdel = pcur;
    pcur = pcur->next;
    memfree(pdel->name);
    memfree(pdel->driver);
    memfree(pdel->ext);
    memfree(pdel->option);
    memfree(pdel);
  }
  pcur2 = Menulocal.prnprinterroot;
  while (pcur2) {
    pdel2 = pcur2;
    pcur2 = pcur2->next;
    memfree(pdel2->name);
    memfree(pdel2->driver);
    memfree(pdel2->option);
    memfree(pdel2->prn);
    memfree(pdel2);
  }
  scur = Menulocal.scriptroot;
  while (scur) {
    sdel = scur;
    scur = scur->next;
    memfree(sdel->name);
    memfree(sdel->script);
    memfree(sdel->option);
    memfree(sdel);
  }

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  memfree(Mxlocal);

  local = gra2cairo_free(obj, inst);
  memfree(local);

  return 1;
}

static int
menudone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;
  arraydel2(&(Menulocal.drawrable));
  arrayfree2(Menulocal.ngpfilelist);
  arrayfree2(Menulocal.ngpdirlist);
  arrayfree2(Menulocal.datafilelist);
  memfree(Menulocal.editor);
  memfree(Menulocal.browser);
  memfree(Menulocal.help_browser);
  free(Menulocal.fileopendir);
  free(Menulocal.graphloaddir);
  memfree(Menulocal.expanddir);
  pcur = Menulocal.extprinterroot;
  while (pcur) {
    pdel = pcur;
    pcur = pcur->next;
    memfree(pdel->name);
    memfree(pdel->driver);
    memfree(pdel->ext);
    memfree(pdel->option);
    memfree(pdel);
  }
  pcur2 = Menulocal.prnprinterroot;
  while (pcur2) {
    pdel2 = pcur2;
    pcur2 = pcur2->next;
    memfree(pdel2->name);
    memfree(pdel2->driver);
    memfree(pdel2->option);
    memfree(pdel2->prn);
    memfree(pdel2);
  }
  scur = Menulocal.scriptroot;
  while (scur) {
    sdel = scur;
    scur = scur->next;
    memfree(sdel->name);
    memfree(sdel->script);
    memfree(sdel->description);
    memfree(sdel->option);
    memfree(sdel);
  }

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  Menulocal.obj = NULL;

  return 0;
}


void
mgtkdisplaydialog(char *str)
{
  DisplayDialog(str);
}


void
mgtkdisplaystatus(char *str)
{
  DisplayStatus(str);
}

int
mgtkputstderr(char *s)
{
  return PutStderr(s);
}

int
mgtkputstdout(char *s)
{
  return PutStdout(s);
}

int
mgtkprintfstderr(char *fmt, ...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  PutStderr(buf);
  return len;
}

int
mgtkprintfstdout(char *fmt, ...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  PutStdout(buf);
  return len;
}

int
mgtkinterrupt(void)
{
  return ChkInterrupt();
}

int
mgtkinputyn(char *mes)
{
  return InputYN(mes);
}

static int
menumenu(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *file;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;
  if (Mxlocal->lock) {
    error(obj, ERR_MENU_RUN);
    return 1;
  }
  Mxlocal->lock = 1;

  savestdio(&GtkIOSave);

  file = (char *) argv[2];
  application(file);

  loadstdio(&GtkIOSave);
  Mxlocal->lock = 0;
  return 0;
}

static int
mx_evloop(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  ResetEvent();
  return 0;
}

static int
mxredrawflag(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  Mxlocal->redrawf = *(int *) argv[2];
  return 0;
}

static int
mxredraw_num(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  int n;

  n = *(int *) argv[2];

  n = (n < 0) ? 0: n;

  Mxlocal->redrawf_num = n;

  *(int *) argv[2] = n;

  return 0;
}

static int
mx_data_head_lines(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  int n;

  n = *(int *) argv[2];

  n = (n < 0) ? 0: n;

  Mxlocal->data_head_lines = n;

  *(int *) argv[2] = n;

  return 0;
}

void
mx_redraw(struct objlist *obj, char *inst)
{
  int n;

  if (Mxlocal->region) {
    mx_clear(Mxlocal->region);
  }

  if (Mxlocal->redrawf) {
    n = Mxlocal->redrawf_num;
  } else {
    n = 0;
  }

  GRAredraw(obj, inst, TRUE, n);
  draw_paper_frame();
  mxflush(obj, inst, NULL, 0, NULL);

  if (Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }
}

void
mx_inslist(struct objlist *obj, char *inst,
	   struct objlist *aobj, char *ainst, char *afield, int addn)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAinslist(gc, aobj, ainst, chkobjectname(aobj), afield, addn);
}

void
mx_dellist(struct objlist *obj, char *inst, int deln)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAdellist(gc, deln);
}

static int
mxredraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  mx_redraw(obj, inst);
  return 0;
}

static int
mxautoredraw(struct objlist *obj, char *inst, char *rval,
	     int argc, char **argv)
{
  if (!(Mxlocal->autoredraw) && (*(int *) argv[2]))
    mx_redraw(obj, inst);
  Mxlocal->autoredraw = *(int *) argv[2];
  return 0;
}

static int
mxdpi(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int dpi;

  dpi = abs(*(int *) argv[2]);
  if (dpi < 1)
    dpi = 1;
  if (dpi > DPI_MAX)
    dpi = DPI_MAX;
  Mxlocal->windpi = dpi;
  Mxlocal->local->pixel_dot_x =
      Mxlocal->local->pixel_dot_y =dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;

  if (Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }
  return 0;
}

static int
mxflush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (Mxlocal->local->linetonum && Mxlocal->local->cairo) {
    cairo_stroke(Mxlocal->local->cairo);
    Mxlocal->local->linetonum = 0;
  }

  return 0;
}

void
mx_clear(GdkRegion *region)
{
  if (Mxlocal->pix) {
    gint w, h;
    GdkColor color;

    gdk_drawable_get_size(Mxlocal->pix, &w, &h);

    if (region) {
      gdk_gc_set_clip_region(Mxlocal->gc, region);
    } else {
      GdkRectangle rect;

      rect.x = 0;
      rect.y = 0;
      rect.width = w;
      rect.height = h;
      gdk_gc_set_clip_rectangle(Mxlocal->gc, &rect);
    }
    color.red = Menulocal.bg_r * 0xff;
    color.green = Menulocal.bg_g * 0xff;
    color.blue = Menulocal.bg_b * 0xff;
    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &color);
    gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, TRUE, 0, 0, w, h);
    gdk_gc_set_clip_region(Mxlocal->gc, NULL);
  }
}

static int
mxclear(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  mx_clear(NULL);

  if (Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }

return 0;
}

int
mxd2p(int r)
{
  return nround(r * Mxlocal->local->pixel_dot_x);
}

int
mxd2px(int x)
{
  return nround(x * Mxlocal->local->pixel_dot_x + Mxlocal->local->offsetx);
  //  return nround(x * Mxlocal->pixel_dot + Mxlocal->offsetx - Mxlocal->scrollx);
}

int
mxd2py(int y)
{
  return nround(y * Mxlocal->local->pixel_dot_y + Mxlocal->local->offsety);
  //  return nround(y * Mxlocal->pixel_dot + Mxlocal->offsety - Mxlocal->scrolly);
}

int
mxp2d(int r)
{
  return ceil(r / Mxlocal->local->pixel_dot_x);
  //  return nround(r / Mxlocal->pixel_dot);
}

static int
mxfullpathngp(struct objlist *obj, char *inst, char *rval, int argc,
	      char **argv)
{
  char *name, *ngp2;

  name = (char *) argv[2];
  if (name == NULL)
    ngp2 = NULL;
  else
    ngp2 = getbasename(name);
  putobj(Menulocal.obj, "ngp", 0, ngp2);
  return 0;
}

static int
check_object_name(struct objlist *obj, struct narray *array)
{
  int i, n;
  char **adata;

  if (array == NULL)
    return 0;

  adata = arraydata(array);

  n = arraynum(array);
  for (i = 0; i < n; i ++) {
    if (adata[i] && strcmp(obj->name, adata[i]) == 0) {
      return 0;
    }
  }

  return 1;
}

static int
mx_get_focused(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int num, i, id;
  char buf[256], *name, *ptr;
  struct narray *oarray, *sarray;
  struct Viewer *d;
  struct focuslist **focus;

  arrayfree2(*(struct narray **)rval);
  *(char **)rval = NULL;

  d = &(NgraphApp.Viewer);

  num = arraynum(d->focusobj);
  if (num < 1)
    return 0;

  oarray = arraynew(sizeof(char *));
  if (oarray == NULL)
    return 1;

  sarray = (argc > 2) ? (struct narray *) argv[2] : NULL;

  focus = (struct focuslist **) arraydata(d->focusobj);
  for (i = 0; i < num; i++) {
    if (check_object_name(focus[i]->obj, sarray))
      continue;

    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst) {
      _getobj(focus[i]->obj, "id", inst, &id);
      name = chkobjectname(focus[i]->obj);
      snprintf(buf, sizeof(buf), "%s:%d", name, id);
      ptr = nstrdup(buf);
      if (ptr) {
	arrayadd(oarray, &ptr);
      }
    }
  }

  *(struct narray **)rval = oarray;

  return 0;

}

static int
mx_print(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int show_dialog, select_file, create_window = FALSE, lock;
  GtkWidget *label;

  select_file = * (int *) argv[2];
  show_dialog = * (int *) argv[3];

  if (TopLevel == NULL) {
    create_window = TRUE;
    TopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(TopLevel), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_signal_connect(TopLevel, "delete-event", G_CALLBACK(gtk_true), NULL);
    label = gtk_label_new(" Ngraph ");
    gtk_container_add(GTK_CONTAINER(TopLevel), label);
    gtk_widget_show_all(TopLevel);
    ResetEvent();
  }

  lock = Menulock;
  menu_lock(FALSE);
  CmOutputPrinter(select_file, show_dialog);
  menu_lock(lock);

  if (create_window) {
    gtk_widget_destroy(TopLevel);
    TopLevel = NULL;
    ResetEvent();
  }
  return 0;
}

static int
mx_echo(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (argv[2])
    PutStdout((char *) argv[2]);

  PutStdout("\n");

  return 0;
}

static int
mxdraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  Draw(FALSE);
  return 0;
}

int
get_graph_modified(void)
{
  int a;

  if (Menulocal.obj == NULL)
    return FALSE;

  getobj(Menulocal.obj, "modified", 0, 0, NULL, &a);

  return a;
}

void
set_graph_modified(void)
{
  int a = 1;

  if (Menulocal.obj == NULL)
    return;

  putobj(Menulocal.obj, "modified", 0, &a);
}

void
reset_graph_modified(void)
{
  int a = 0;

  if (Menulocal.obj == NULL)
    return;

  putobj(Menulocal.obj, "modified", 0, &a);
}

static struct objtable gtkmenu[] = {
  {"init", NVFUNC, NEXEC, menuinit, NULL, 0},
  {"done", NVFUNC, NEXEC, menudone, NULL, 0},
  {"menu", NVFUNC, NREAD | NEXEC, menumenu, NULL, 0},
  {"ngp", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"fullpath_ngp", NSTR, NREAD | NWRITE, mxfullpathngp, NULL, 0},
  {"data_head_lines", NINT, NREAD | NWRITE, mx_data_head_lines, NULL, 0},
  {"modified", NBOOL, NREAD | NWRITE, NULL, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, mxdpi, NULL, 0},
  {"auto_redraw", NBOOL, NREAD | NWRITE, mxautoredraw, NULL, 0},
  {"redraw_flag", NBOOL, NREAD | NWRITE, mxredrawflag, NULL, 0},
  {"redraw_num", NINT, NREAD | NWRITE, mxredraw_num, NULL, 0},
  {"redraw", NVFUNC, NREAD | NEXEC, mxredraw, "", 0},
  {"draw", NVFUNC, NREAD | NEXEC, mxdraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, mxflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, mxclear, "", 0},
  {"focused", NSAFUNC, NREAD | NEXEC, mx_get_focused, "sa", 0},
  {"print", NVFUNC, NREAD | NEXEC, mx_print, "bi", 0},
  {"echo", NVFUNC, NREAD | NEXEC, mx_echo, "s", 0},
  {"_gtklocal", NPOINTER, 0, NULL, NULL, 0},
  {"_evloop", NVFUNC, 0, mx_evloop, NULL, 0},
};

#define TBLNUM (sizeof(gtkmenu) / sizeof(*gtkmenu))

void *
addmenu(void)
{
  unsigned int i;

  if (MenuConfigHash == NULL) {
    MenuConfigHash = nhash_new();
    if (MenuConfigHash ==NULL)
      return NULL;

    for (i = 0; i < sizeof(MenuConfig) / sizeof(*MenuConfig); i++) {
      if (nhash_set_ptr(MenuConfigHash, MenuConfig[i].name, (void *) &MenuConfig[i])) {
	nhash_free(MenuConfigHash);
	return NULL;
      }
    }
  }

  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gtkmenu, ERRNUM,
		   menuerrorlist, NULL, NULL);
}
