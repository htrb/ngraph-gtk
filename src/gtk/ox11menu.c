/* 
 * $Id: ox11menu.c,v 1.90 2010-03-04 08:30:17 hito Exp $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "strconv.h"

#include "gtk_widget.h"
#include "gtk_subwin.h"

#include "main.h"
#include "ogra2cairo.h"
#include "ogra2x11.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11view.h"
#include "x11graph.h"
#include "x11print.h"

#include "x11merge.h"
#include "x11lgnd.h"
#include "x11axis.h"
#include "x11file.h"
#include "x11cood.h"
#include "x11info.h"

#define NAME "menu"
#define ALIAS "winmenu:gtkmenu"
#define PARENT "gra2cairo"
#define NVERSION  "1.00.00"
#define MGTKCONF "[x11menu]"
#define G2WINCONF "[gra2gtk]"


static char *menuerrorlist[] = {
  "already running.",
  "cannot open the display.",
  "cannot open the file",
  "the GUI is not active",
};

#define ERRNUM (sizeof(menuerrorlist) / sizeof(*menuerrorlist))

enum {
  ERR_MENU_RUN = 100,
  ERR_MENU_DISPLAY,
  ERR_MENU_OPEN_FILE,
  ERR_MENU_GUI,
};

struct menulocal Menulocal;
struct savedstdio GtkIOSave;

static int mxflush(struct objlist *obj, char *inst, char *rval, int argc,
		   char **argv);


enum menu_config_type {
  MENU_CONFIG_TYPE_NUMERIC,
  MENU_CONFIG_TYPE_BOOL,
  MENU_CONFIG_TYPE_STRING,
  MENU_CONFIG_TYPE_WINDOW,
  MENU_CONFIG_TYPE_CHILD_WINDOW,
  MENU_CONFIG_TYPE_HISTORY,
  MENU_CONFIG_TYPE_COLOR,
  MENU_CONFIG_TYPE_SCRIPT,
  MENU_CONFIG_TYPE_DRIVER,
};

static int menu_config_set_four_elements(char *s2, void *data);
static int menu_config_set_child_window_geometry(char *s2, void *data);
static int menu_config_set_bgcolor(char *s2, void *data);
static int menu_config_set_ext_driver(char *s2, void *data);
static int menu_config_set_script(char *s2, void *data);

static int *menu_config_menu_geometry[] = {
  &Menulocal.menux,
  &Menulocal.menuy,
  &Menulocal.menuwidth,
  &Menulocal.menuheight,
};

struct child_win_stat {
  struct SubWin *win;
  int *stat[5];
};

static struct child_win_stat menu_config_file_geometry = {
  &NgraphApp.FileWin,
  {
    &Menulocal.filex,
    &Menulocal.filey,
    &Menulocal.filewidth,
    &Menulocal.fileheight,
    &Menulocal.fileopen,
  }
};

static struct child_win_stat menu_config_axis_geometry = {
  &NgraphApp.AxisWin,
  {
    &Menulocal.axisx,
    &Menulocal.axisy,
    &Menulocal.axiswidth,
    &Menulocal.axisheight,
    &Menulocal.axisopen,
  }
};

static struct child_win_stat menu_config_legend_geometry = {
  (struct SubWin *) &NgraphApp.LegendWin,
  {
    &Menulocal.legendx,
    &Menulocal.legendy,
    &Menulocal.legendwidth,
    &Menulocal.legendheight,
    &Menulocal.legendopen,
  }
};

static struct child_win_stat menu_config_merge_geometry = {
  &NgraphApp.MergeWin,
  {
    &Menulocal.mergex,
    &Menulocal.mergey,
    &Menulocal.mergewidth,
    &Menulocal.mergeheight,
    &Menulocal.mergeopen,
  }
};

static struct child_win_stat menu_config_dialog_geometry = {
  (struct SubWin *) &NgraphApp.InfoWin,
  {
    &Menulocal.dialogx,
    &Menulocal.dialogy,
    &Menulocal.dialogwidth,
    &Menulocal.dialogheight,
    &Menulocal.dialogopen,
  }
};

static struct child_win_stat menu_config_coord_geometry = {
  (struct SubWin * ) &NgraphApp.CoordWin,
  {
    &Menulocal.coordx,
    &Menulocal.coordy,
    &Menulocal.coordwidth,
    &Menulocal.coordheight,
    &Menulocal.coordopen,
  }
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
  {"show_tip",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.showtip},
  {"expand_to_fullpath",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.expandtofullpath},
  {"browser",			MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.browser},
  {"help_browser",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.help_browser},
  {"data_history",		MENU_CONFIG_TYPE_HISTORY, NULL, &Menulocal.datafilelist},
  {NULL},
};

static struct menu_config MenuConfigDriver[] = {
  {"ext_driver", MENU_CONFIG_TYPE_DRIVER, menu_config_set_ext_driver, NULL},
  {NULL},
};

static struct menu_config MenuConfigScript[] = {
  {"script", MENU_CONFIG_TYPE_SCRIPT, menu_config_set_script, NULL},
  {NULL},
};

static struct menu_config MenuConfigMisc[] = {
  {"editor",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.editor},
  {"coordwin_font",	MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.coordwin_font},
  {"infowin_font",	MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.infowin_font},
  {"file_preview_font",	MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.file_preview_font},
  {"change_directory",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.changedirectory},
  {"save_history",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savehistory},
  {"save_path",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savepath},
  {"save_with_data",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithdata},
  {"save_with_merge",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithmerge},
  {"expand_dir",	MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.expanddir},
  {"expand",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.expand},
  {"ignore_path",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ignorepath},
  {"history_size",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.hist_size},
  {"infowin_size",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.info_size},
  {"data_head_lines",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.data_head_lines},
  {NULL},
};

static struct menu_config MenuConfigViewer[] = {
  {"viewer_dpi",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.windpi},
  {"antialias",				MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.antialias},
  {"viewer_load_file_on_redraw",	MENU_CONFIG_TYPE_BOOL,    NULL, &Menulocal.redrawf},
  {"viewer_load_file_data_number",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.redrawf_num},
  {"viewer_grid",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.grid},
  {"focus_frame_type",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.focus_frame_type},
  {"preserve_width",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.preserve_width},
  {"do_not_use_arc_for_draft",		MENU_CONFIG_TYPE_BOOL,    NULL, &Menulocal.do_not_use_arc_for_draft},
  {"background_color",			MENU_CONFIG_TYPE_COLOR, menu_config_set_bgcolor, NULL},
  {NULL},
};

static struct menu_config MenuConfigToggleView[] = {
  {"viewer_show_ruler",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ruler},
  {"status_bar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.statusb},
  {"scrollbar",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.scrollbar},
  {"command_toolbar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ctoolbar},
  {"pointer_toolbar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ptoolbar},
  {"cross_gauge",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.show_cross},
  {NULL},
};

static struct menu_config MenuConfigExportImage[] = {
  {"png_dpi",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.png_dpi},
#ifdef WINDOWS
  {"emf_dpi",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.emf_dpi},
#endif
  {"ps_version",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ps_version},
  {"svg_version",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.svg_version},
  {NULL},
};

static struct menu_config MenuConfigGeometry[] = {
  {"menu_win", MENU_CONFIG_TYPE_WINDOW, menu_config_set_four_elements, menu_config_menu_geometry},
  {NULL},
};

static struct menu_config MenuConfigChildGeometry[] = {
  {"file_win",		MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_file_geometry},
  {"axis_win",		MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_axis_geometry},
  {"legend_win",	MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_legend_geometry},
  {"merge_win",		MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_merge_geometry},
  {"information_win",	MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_dialog_geometry},
  {"coordinate_win",	MENU_CONFIG_TYPE_CHILD_WINDOW, NULL, &menu_config_coord_geometry},
  {NULL},
};

static struct menu_config *MenuConfigArrray[] = {
  MenuConfig,
  MenuConfigDriver,
  MenuConfigScript,
  MenuConfigMisc,
  MenuConfigViewer,
  MenuConfigToggleView,
  MenuConfigExportImage,
  MenuConfigGeometry,
  MenuConfigChildGeometry,
  NULL,
};

static NHASH MenuConfigHash = NULL;

#define BUF_SIZE 64

static void
add_str_with_int_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;

  buf = (char *) g_malloc(BUF_SIZE);    
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%d", cfg->name, * (int *) cfg->data);
    arrayadd(conf, &buf);
  }
}

static void
add_child_geometry_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;
  int **data;
  struct child_win_stat *stat;

  stat = cfg->data;
  data = stat->stat;

  buf = (char *) g_malloc(BUF_SIZE);    
  if (buf) {
    sub_window_save_geometry(stat->win);
    snprintf(buf, BUF_SIZE, "%s=%d,%d,%d,%d,%d",
	     cfg->name, *data[0], *data[1], *data[2], *data[3], *data[4]);
    arrayadd(conf, &buf);
  }
}

static void
add_geometry_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;
  GdkWindowState state;
  gint x, y, w, h;

  get_window_geometry(TopLevel, &x, &y, &w, &h, &state);

  Menulocal.menux = x;
  Menulocal.menuy = y;
  Menulocal.menuwidth = w;
  Menulocal.menuheight = h;

  buf = (char *) g_malloc(BUF_SIZE);
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%d,%d,%d,%d",
	     cfg->name,
	     Menulocal.menux, Menulocal.menuy,
	     Menulocal.menuwidth, Menulocal.menuheight);
    arrayadd(conf, &buf);
  }
}

static void
add_color_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;
  buf = (char *) g_malloc(BUF_SIZE);
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%02x%02x%02x",
	     cfg->name,
	     Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
    arrayadd(conf, &buf);
  }
}

static void
add_prm_str_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf, *prm;

  prm = CHK_STR(* (char **) cfg->data);

  buf = g_strdup_printf("%s=%s", cfg->name, prm);
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
save_ext_driver_config(struct narray *conf)
{
  char *buf, *driver, *ext, *option;
  struct extprinter *pcur;

  pcur = Menulocal.extprinterroot;
  while (pcur) {
    driver = CHK_STR(pcur->driver);
    ext = CHK_STR(pcur->ext);
    option= CHK_STR(pcur->option);

    buf = g_strdup_printf("ext_driver=%s,%s,%s,%s", pcur->name, driver, ext, option);
    if (buf) {
      arrayadd(conf, &buf);
    }
    pcur = pcur->next;
  }
}

static void
save_script_config(struct narray *conf)
{
  char *buf, *script, *option, *description;
  struct script *scur;

  scur = Menulocal.scriptroot;
  while (scur) {
    script = CHK_STR(scur->script);
    option = CHK_STR(scur->option);
    description = CHK_STR(scur->description);

    buf = g_strdup_printf("script=%s,%s,%s,%s", scur->name, script, description, option);
    if (buf) {
      arrayadd(conf, &buf);
    }
    scur = scur->next;
  }
}

static void
add_str_to_array(struct narray *conf, char *str)
{
  char *buf;

  buf = g_strdup(str);
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
menu_save_config_sub(struct menu_config *cfg, struct narray *conf)
{
  int i;

  for (i = 0; cfg[i].name; i++) {
    switch (cfg[i].type) {
    case MENU_CONFIG_TYPE_NUMERIC:
    case MENU_CONFIG_TYPE_BOOL:
      add_str_with_int_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_COLOR:
      add_color_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_STRING:
      add_prm_str_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_WINDOW:
      add_geometry_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_CHILD_WINDOW:
      add_child_geometry_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_SCRIPT:
      save_script_config(conf);
      break;
    case MENU_CONFIG_TYPE_DRIVER:
      save_ext_driver_config(conf);
      break;
    case MENU_CONFIG_TYPE_HISTORY:
      break;
    }
  }
}

int
menu_save_config(int type)
{
  struct narray conf;

  arrayinit(&conf, sizeof(char *));

  if (type & SAVE_CONFIG_TYPE_GEOMETRY) {
    menu_save_config_sub(MenuConfigGeometry, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_CHILD_GEOMETRY) {
    menu_save_config_sub(MenuConfigChildGeometry, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_VIEWER) {
    menu_save_config_sub(MenuConfigViewer, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_TOGGLE_VIEW) {
    menu_save_config_sub(MenuConfigToggleView, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_EXPORT_IMAGE) {
    menu_save_config_sub(MenuConfigExportImage, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_EXTERNAL_DRIVER) {
    menu_save_config_sub(MenuConfigDriver, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_ADDIN_SCRIPT) {
    menu_save_config_sub(MenuConfigScript, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_MISC) {
    menu_save_config_sub(MenuConfigMisc, &conf);
  }

  replaceconfig(MGTKCONF, &conf);
  arraydel2(&conf);

  arrayinit(&conf, sizeof(char *));

  if (type & SAVE_CONFIG_TYPE_EXTERNAL_DRIVER) {
    if (Menulocal.extprinterroot == NULL) {
      add_str_to_array(&conf, "ext_driver");
    }
  }

  if (type & SAVE_CONFIG_TYPE_ADDIN_SCRIPT) {
    if (Menulocal.scriptroot == NULL) {
      add_str_to_array(&conf, "script");
    }
  }

  removeconfig(MGTKCONF, &conf);
  arraydel2(&conf);

  return 0;
}

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
    g_free(f[i]);
  }
  return 0;
}

static int
menu_config_set_child_window_geometry(char *s2, void *data)
{
  int len, i, val, **ary;
  char *endptr, *f[] = {NULL, NULL, NULL, NULL, NULL};

  if (data == NULL)
    return 0;

  ary = ((struct child_win_stat *) data)->stat;

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
    g_free(f[i]);
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
    Menulocal.bg_r = (val >> 16) & 0xffU;
    Menulocal.bg_g = (val >> 8) & 0xffU;
    Menulocal.bg_b = val & 0xffU;
  }
  g_free(f1);
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
    pnew = (struct extprinter *) g_malloc(sizeof(struct extprinter));
    if (pnew == NULL) {
      for (i = 0; i < 4; i++) {
	g_free(f[i]);
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
      g_free(f[i]);
    }
  }
  return 0;
}

static int
menu_config_set_script(char *s2, void *data)
{
  char *f[] = {NULL, NULL, NULL, NULL};
  int len;
  unsigned int i;
  struct script *snew, *scur, **sptr;

  sptr = (struct script **) data;
  scur = *sptr;

  f[0] = getitok2(&s2, &len, ",");
  f[1] = getitok2(&s2, &len, ",");
  f[2] = getitok2(&s2, &len, ",");

  for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
  f[3] = getitok2(&s2, &len, ",");

  if (f[0] && f[1]) {
    snew = (struct script *) g_malloc(sizeof(struct script));
    if (snew == NULL) {
      for (i = 0; i < sizeof(f) / sizeof(*f); i++) {
	g_free(f[i]);
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
      g_free(f[i]);
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
  struct script *scur;
  struct menu_config *cfg;

  fp = openconfig(MGTKCONF);
  if (fp == NULL)
    return 0;

  pcur = Menulocal.extprinterroot;
  scur = Menulocal.scriptroot;

  if (nhash_get_ptr(MenuConfigHash, "ext_driver", (void *) &cfg) == 0) {
    if (cfg) {
      cfg->data = &pcur;
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
	g_free(f1);
	break;
      case MENU_CONFIG_TYPE_BOOL:
	f1 = getitok2(&s2, &len, " \t,");
	if (f1) {
	  val = strtol(f1, &endptr, 10);
	  if (endptr[0] == '\0')
	    * (int *) (cfg->data) = (val != 0);
	}
	g_free(f1);
	break;
      case MENU_CONFIG_TYPE_STRING:
	f1 = getitok2(&s2, &len, "");
	if (f1) {
	  g_free(* (char **) (cfg->data));
	  * (char **) (cfg->data) = f1;
	}
	break;
      case MENU_CONFIG_TYPE_CHILD_WINDOW:
	menu_config_set_child_window_geometry(s2, cfg->data);
	break;
      case MENU_CONFIG_TYPE_HISTORY:
	for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
	f1 = getitok2(&s2, &len, "");
	if (f1)
	  arrayadd(* (struct narray **) cfg->data, &f1);
	break;
      case MENU_CONFIG_TYPE_COLOR:
      case MENU_CONFIG_TYPE_SCRIPT:
      case MENU_CONFIG_TYPE_DRIVER:
      case MENU_CONFIG_TYPE_WINDOW:
	if (cfg->proc && cfg->proc(s2, cfg->data)) {
	  g_free(tok);
	  g_free(str);
	  closeconfig(fp);
	  return 1;
	}
	break;
      }
    } else {
      fprintf(stderr, "configuration '%s' in section %s is not used.\n", tok, MGTKCONF);
    }
    g_free(tok);
    g_free(str);
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
    Menulocal.filewidth = DEFAULT_GEOMETRY;

  Menulocal.axisx =
    Menulocal.axisy =
    Menulocal.axisheight =
    Menulocal.axiswidth = DEFAULT_GEOMETRY;

  Menulocal.legendx =
    Menulocal.legendy =
    Menulocal.legendheight =
    Menulocal.legendwidth = DEFAULT_GEOMETRY;

  Menulocal.mergex =
    Menulocal.mergey =
    Menulocal.mergeheight =
    Menulocal.mergewidth = DEFAULT_GEOMETRY;

  Menulocal.dialogx =
    Menulocal.dialogy =
    Menulocal.dialogheight =
    Menulocal.dialogwidth = DEFAULT_GEOMETRY;

  Menulocal.coordx =
    Menulocal.coordy =
    Menulocal.coordheight =
    Menulocal.coordwidth = DEFAULT_GEOMETRY;
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
      if(cfg && cfg->type == MENU_CONFIG_TYPE_CHILD_WINDOW) {
	menu_config_set_child_window_geometry(s2, cfg->data);
      }
    }
    g_free(tok);
    g_free(str);
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
      g_free(f1);
    } else if (strcmp(tok, "win_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinwidth = val;
      g_free(f1);
    } else if (strcmp(tok, "win_height") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinheight = val;
      g_free(f1);
    } else if (strcmp(tok, "use_external_viewer") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwin_use_external = val;
      g_free(f1);
    } else {
      fprintf(stderr, "configuration '%s' in section %s is not used.\n", tok, G2WINCONF);
    }
    g_free(tok);
    g_free(str);
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
menulocal_finalize(void)
{
  struct extprinter *pcur, *pdel;
  struct script *scur, *sdel;
  int i, j;
  struct menu_config *cfg;

  for (i = 0; (cfg = MenuConfigArrray[i]); i++) {
    for (j = 0; cfg[j].name; j++) {
      if (cfg[i].type == MENU_CONFIG_TYPE_STRING) {
	g_free(* (char **) cfg[i].data);
	* (char **) cfg[i].data = NULL;
      }
    }
  }

  pcur = Menulocal.extprinterroot;
  while (pcur) {
    pdel = pcur;
    pcur = pcur->next;
    g_free(pdel->name);
    g_free(pdel->driver);
    g_free(pdel->ext);
    g_free(pdel->option);
    g_free(pdel);
  }
  Menulocal.extprinterroot = NULL;

  scur = Menulocal.scriptroot;
  while (scur) {
    sdel = scur;
    scur = scur->next;
    g_free(sdel->name);
    g_free(sdel->script);
    g_free(sdel->description);
    g_free(sdel->option);
    g_free(sdel);
  }
  Menulocal.scriptroot = NULL;

  if (Menulocal.pix)
    g_object_unref(Menulocal.pix);
  Menulocal.pix = NULL;

  arraydel2(&Menulocal.drawrable);

  Menulocal.ngpfilelist = NULL;

  arrayfree2(Menulocal.datafilelist);
  Menulocal.datafilelist = NULL;

  g_free(Menulocal.fileopendir);
  Menulocal.fileopendir = NULL;

  g_free(Menulocal.graphloaddir);
  Menulocal.graphloaddir = NULL;

  Menulocal.obj = NULL;
  Menulocal.local = NULL;
}

static int
menuinit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  int i;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (_getobj(obj, "_local", inst, &local)) {
    local = gra2cairo_free(obj, inst);
    g_free(local);
    return 1;
  }

  memset(&Menulocal, 0, sizeof(Menulocal));

  Menulocal.menux = Menulocal.menuy
    = Menulocal.menuheight = Menulocal.menuwidth = DEFAULT_GEOMETRY;
  initwindowconfig();
  Menulocal.showtip = TRUE;
  Menulocal.statusb = TRUE;
  Menulocal.ruler = TRUE;
  Menulocal.scrollbar = TRUE;
  Menulocal.ptoolbar = TRUE;
  Menulocal.ctoolbar = TRUE;
  Menulocal.show_cross = FALSE;
  Menulocal.scriptconsole = FALSE;
  Menulocal.addinconsole = TRUE;
  Menulocal.changedirectory = 1;
  set_paper_type(21000, 29700);
  Menulocal.PaperZoom = 10000;
  Menulocal.exwindpi = DEFAULT_DPI;
  Menulocal.expand = 1;
  Menulocal.expanddir = g_strdup("./");
  Menulocal.expandtofullpath = TRUE;
  Menulocal.ngpfilelist = gtk_recent_manager_get_default();
  Menulocal.datafilelist = arraynew(sizeof(char *));
  Menulocal.GRAobj = chkobject("gra");
  Menulocal.hist_size = 1000;
  Menulocal.info_size = 1000;
  Menulocal.bg_r = 0xffU;
  Menulocal.bg_g = 0xffU;
  Menulocal.bg_b = 0xffU;
  Menulocal.focus_frame_type = GDK_LINE_ON_OFF_DASH;

  arrayinit(&(Menulocal.drawrable), sizeof(char *));
  menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));

  Menulocal.windpi = DEFAULT_DPI;
  Menulocal.redrawf = TRUE;
  Menulocal.redrawf_num = 0xffU;
  Menulocal.grid = 200;
  Menulocal.data_head_lines = 20;
  Menulocal.local = local;

  Menulocal.png_dpi = 72;
#ifdef WINDOWS
  Menulocal.emf_dpi = 72;
#endif
  Menulocal.ps_version = 0;
  Menulocal.svg_version = 0;

  if (mgtkloadconfig())
    goto errexit;

  if (exwinloadconfig())
    goto errexit;

  Menulocal.local->antialias = Menulocal.antialias;
  if (_putobj(obj, "antialias", inst, &(Menulocal.antialias)))
    goto errexit;

  if (Menulocal.exwindpi < 1)
    Menulocal.exwindpi = DEFAULT_DPI;

  if (Menulocal.exwindpi > DPI_MAX)
    Menulocal.exwindpi = DPI_MAX;

  if (Menulocal.windpi < 1)
    Menulocal.windpi = DEFAULT_DPI;

  if (Menulocal.windpi > DPI_MAX)
    Menulocal.windpi = DPI_MAX;

  if (_putobj(obj, "dpi", inst, &(Menulocal.windpi)))
    goto errexit;

  if (_putobj(obj, "data_head_lines", inst, &(Menulocal.data_head_lines)))
    goto errexit;

  if (_putobj(obj, "redraw_flag", inst, &(Menulocal.redrawf)))
    goto errexit;

  if (_putobj(obj, "redraw_num", inst, &(Menulocal.redrawf_num)))
    goto errexit;

  i = 0;
  if (_putobj(obj, "modified", inst, &i))
    goto errexit;

  Menulocal.obj = obj;
  Menulocal.inst = inst;
  Menulocal.pix = NULL;
  Menulocal.win = NULL;
  Menulocal.gc = NULL;
  Menulocal.lock = 0;

  if (!OpenApplication()) {
    error(obj, ERR_MENU_DISPLAY);
    goto errexit;
  }

  return 0;

 errexit:
  menulocal_finalize();

  local = gra2cairo_free(obj, inst);
  g_free(local);

  return 1;
}

static int
menudone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (Menulocal.lock) {
    error(obj, ERR_MENU_RUN);
    return 1;
  }

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  menulocal_finalize();

  return 0;
}


void
mgtkdisplaydialog(const char *str)
{
  DisplayDialog(str);
}


void
mgtkdisplaystatus(const char *str)
{
  DisplayDialog(str);
}

int
mgtkputstderr(const char *s)
{
  return PutStderr(s);
}

int
mgtkputstdout(const char *s)
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
mgtkinputyn(const char *mes)
{
  char *ptr;
  int r;

  ptr = g_locale_to_utf8(CHK_STR(mes), -1, NULL, NULL, NULL);
  r = InputYN(ptr);
  g_free(ptr);

  return r;
}

static int
menumenu(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *file;
  int r;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  if (Menulocal.lock) {
    error(obj, ERR_MENU_RUN);
    return 1;
  }
  Menulocal.lock = 1;

  savestdio(&GtkIOSave);

  file = get_utf8_filename(argv[2]);

  hide_console();
  r = application(file);
  resotre_console();

  if (file) {
    g_free(file);
  }

  loadstdio(&GtkIOSave);
  Menulocal.lock = 0;
  return r;
}

static int
mx_evloop(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  reset_event();
  return 0;
}

static int
mxredrawflag(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  Menulocal.redrawf = *(int *) argv[2];
  return 0;
}

static int
mxredraw_num(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  int n;

  n = *(int *) argv[2];

  n = (n < 0) ? 0: n;

  Menulocal.redrawf_num = n;

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

  Menulocal.data_head_lines = n;

  *(int *) argv[2] = n;

  return 0;
}

void
mx_redraw(struct objlist *obj, char *inst)
{
  int n;

  if (Menulocal.region) {
    mx_clear(Menulocal.region);
  }

  if (Menulocal.redrawf) {
    n = Menulocal.redrawf_num;
  } else {
    n = 0;
  }

  GRAredraw(obj, inst, TRUE, n);
  draw_paper_frame();
  mxflush(obj, inst, NULL, 0, NULL);

  if (Menulocal.win) {
    gdk_window_invalidate_rect(Menulocal.win, NULL, TRUE);
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
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  mx_redraw(obj, inst);
  return 0;
}

static int
mxdpi(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int dpi;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  dpi = abs(*(int *) argv[2]);
  if (dpi < 1)
    dpi = 1;
  if (dpi > DPI_MAX)
    dpi = DPI_MAX;
  Menulocal.windpi = dpi;
  Menulocal.local->pixel_dot_x =
      Menulocal.local->pixel_dot_y =dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;

  if (Menulocal.win) {
    gdk_window_invalidate_rect(Menulocal.win, NULL, TRUE);
  }
  return 0;
}

static int
mxflush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (Menulocal.local->linetonum && Menulocal.local->cairo) {
    cairo_stroke(Menulocal.local->cairo);
    Menulocal.local->linetonum = 0;
  }

  return 0;
}

void
mx_clear(GdkRegion *region)
{
  if (Menulocal.pix) {
    gint w, h;
    GdkColor color;

    if (Menulocal.pix == NULL || Menulocal.gc == NULL)
      return;

    gdk_drawable_get_size(Menulocal.pix, &w, &h);

    if (region) {
      gdk_gc_set_clip_region(Menulocal.gc, region);
    } else {
      GdkRectangle rect;

      rect.x = 0;
      rect.y = 0;
      rect.width = w;
      rect.height = h;
      gdk_gc_set_clip_rectangle(Menulocal.gc, &rect);
    }
    color.red = Menulocal.bg_r * 0xffU;
    color.green = Menulocal.bg_g * 0xffU;
    color.blue = Menulocal.bg_b * 0xffU;
    gdk_gc_set_rgb_fg_color(Menulocal.gc, &color);
    gdk_draw_rectangle(Menulocal.pix, Menulocal.gc, TRUE, 0, 0, w, h);
    draw_paper_frame();
    gdk_gc_set_clip_region(Menulocal.gc, NULL);
  }
}

static int
mxclear(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  mx_clear(NULL);

  if (Menulocal.win) {
    gdk_window_invalidate_rect(Menulocal.win, NULL, TRUE);
  }

return 0;
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

  if (adata == NULL)
    return 0;

  n = arraynum(array);
  if (n == 0)
    return 0;

  for (i = 0; i < n; i ++) {
    if (obj == chkobject(adata[i]))
      return 0;
  }

  return 1;
}

static int
mx_get_focused(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int num, i, id;
  char *name, *ptr;
  struct narray *oarray, *sarray;
  struct Viewer *d;
  struct FocusObj **focus;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

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

  focus = (struct FocusObj **) arraydata(d->focusobj);
  for (i = 0; i < num; i++) {
    if (check_object_name(focus[i]->obj, sarray))
      continue;

    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst) {
      _getobj(focus[i]->obj, "id", inst, &id);
      name = chkobjectname(focus[i]->obj);
      ptr = g_strdup_printf("%s:%d", name, id);
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
    reset_event();
  }

  lock = Menulock;
  menu_lock(FALSE);
  CmOutputPrinter(select_file, show_dialog);
  menu_lock(lock);

  if (create_window) {
    gtk_widget_destroy(TopLevel);
    TopLevel = NULL;
    reset_event();
  }
  return 0;
}

static int
mx_echo(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (argv[2])
    PutStdout((char *) argv[2]);

  PutStdout("\n");

  return 0;
}

static int
mx_cat(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  static char buf[1024];
  int len, use_stdin = TRUE;
  int fd;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (argv[2]) {
    fd = nopen(argv[2], O_RDONLY, 0);
    if (fd == -1) {
      error(obj, ERR_MENU_OPEN_FILE);
      return 1;
    }
    use_stdin = FALSE;
  } else {
    fd = stdinfd();
  }

  while ((len = nread(fd, buf, sizeof(buf) - 1)) > 0) {
    buf[len] = '\0';
    PutStdout(buf);
  }

  if (! use_stdin) {
    nclose(fd);
  }

  return 0;
}

static int
mx_clear_info(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  InfoWinClear();
  return 0;
}

static struct _subwin_data {
  struct SubWin *w;
  void (*init)(GtkWidget *, gpointer);
} SubwinData[] = {
  {
    &NgraphApp.FileWin,
    CmFileWindow,
  },
  {
    &NgraphApp.AxisWin,
    CmAxisWindow,
  },
  {
    (struct SubWin *) &NgraphApp.LegendWin,
    CmLegendWindow,
  },
  {
    &NgraphApp.MergeWin,
    CmMergeWindow,
  },
  {
    (struct SubWin *) &NgraphApp.CoordWin,
    CmCoordinateWindow,
  },
  {
    (struct SubWin *) &NgraphApp.InfoWin,
    CmInformationWindow,
  },
};

static void
subwindow_show(struct _subwin_data *win)
{
  if (win->w->Win) {
    sub_window_show(win->w);
  } else {
    win->init(NULL, NULL);
  }
}

static void
subwindow_hide(struct _subwin_data *win)
{
  if (win->w->Win) {
    sub_window_hide(win->w);
  }
}

static int
mx_show_win(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  unsigned int win;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  win = * (unsigned int *) (argv[2]);

  if (win >= sizeof(SubwinData) / sizeof(*SubwinData))
    return 0;

  subwindow_show(&SubwinData[win]);

  return 0;
}

static int
mx_hide_win(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  unsigned int win;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  win = * (unsigned int *) (argv[2]);

  if (win >= sizeof(SubwinData) / sizeof(*SubwinData))
    return 0;

  subwindow_hide(&SubwinData[win]);

  return 0;
}

static int
mx_toggle_win(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int win;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  win = * (int *) (argv[2]);

  switch (win) {
  case 0:
    CmFileWindow(NULL, NULL);
    break;
  case 1:
    CmAxisWindow(NULL, NULL);
    break;
  case 2:
    CmLegendWindow(NULL, NULL);
    break;
  case 3:
    CmMergeWindow(NULL, NULL);
    break;
  case 4:
    CmCoordinateWindow(NULL, NULL);
    break;
  case 5:
    CmInformationWindow(NULL, NULL);
    break;
  }

  return 0;
}

static int
mxdraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  Draw(FALSE);
  return 0;
}

static void
SetCaption(int modified)
{
  char buf[1024], *file;

  getobj(Menulocal.obj, "ngp", 0, 0, NULL, &file);

  snprintf(buf, sizeof(buf), "%s%s - Ngraph",
	   (modified) ? "*" : "",
	   (file) ? file : _("Unsaved Graph"));

  gtk_window_set_title(GTK_WINDOW(TopLevel), buf);
}

static int
mxmodified(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int modified;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  modified = * (int *) argv[2];

  SetCaption(modified);
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

static void
graph_modified_sub(int a)
{
  if (Menulocal.obj == NULL)
    return;

  putobj(Menulocal.obj, "modified", 0, &a);
  SetCaption(a);
  set_modified_state(a);
}

void
set_graph_modified(void)
{
  graph_modified_sub(1);
}

void
reset_graph_modified(void)
{
  graph_modified_sub(0);
}

static struct objtable gtkmenu[] = {
  {"init", NVFUNC, NEXEC, menuinit, NULL, 0},
  {"done", NVFUNC, NEXEC, menudone, NULL, 0},
  {"menu", NVFUNC, NREAD | NEXEC, menumenu, "s", 0},
  {"ngp", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"fullpath_ngp", NSTR, NREAD | NWRITE, mxfullpathngp, NULL, 0},
  {"data_head_lines", NINT, NREAD | NWRITE, mx_data_head_lines, NULL, 0},
  {"modified", NBOOL, NREAD | NWRITE, mxmodified, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, mxdpi, NULL, 0},
  {"redraw_flag", NBOOL, NREAD | NWRITE, mxredrawflag, NULL, 0},
  {"redraw_num", NINT, NREAD | NWRITE, mxredraw_num, NULL, 0},
  {"redraw", NVFUNC, NREAD | NEXEC, mxredraw, "", 0},
  {"draw", NVFUNC, NREAD | NEXEC, mxdraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, mxflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, mxclear, "", 0},
  {"focused", NSAFUNC, NREAD | NEXEC, mx_get_focused, "sa", 0},
  {"print", NVFUNC, NREAD | NEXEC, mx_print, "bi", 0},
  {"echo", NVFUNC, NREAD | NEXEC, mx_echo, "s", 0},
  {"cat", NVFUNC, NREAD | NEXEC, mx_cat, "s", 0},
  {"clear_info", NVFUNC, NREAD | NEXEC, mx_clear_info, "", 0},
  {"show_window", NVFUNC, NREAD | NEXEC, mx_show_win, "i", 0},
  {"hide_window", NVFUNC, NREAD | NEXEC, mx_hide_win, "i", 0},
  {"toggle_window", NVFUNC, NREAD | NEXEC, mx_toggle_win, "i", 0},
  {"_evloop", NVFUNC, 0, mx_evloop, NULL, 0},
};

#define TBLNUM (sizeof(gtkmenu) / sizeof(*gtkmenu))

void *
addmenu(void)
{
  unsigned int i, j;
  struct menu_config *cfg;

  if (MenuConfigHash == NULL) {
    MenuConfigHash = nhash_new();
    if (MenuConfigHash ==NULL)
      return NULL;

    for (i = 0; (cfg = MenuConfigArrray[i]); i++) {
      for (j = 0; cfg[j].name; j++) {
	if (nhash_set_ptr(MenuConfigHash, cfg[j].name, (void *) (cfg + j))) {
	  nhash_free(MenuConfigHash);
	  return NULL;
	}
      }
    }
  }

  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gtkmenu, ERRNUM,
		   menuerrorlist, NULL, NULL);
}
