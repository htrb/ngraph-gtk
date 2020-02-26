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
#include <errno.h>

#include "object.h"
#include "odraw.h"
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

#include "init.h"
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
  "running.",
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

static int mxflush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		   char **argv);


enum menu_config_type {
  MENU_CONFIG_TYPE_NUMERIC,
  MENU_CONFIG_TYPE_BOOL,
  MENU_CONFIG_TYPE_STRING,
  MENU_CONFIG_TYPE_WINDOW,
  MENU_CONFIG_TYPE_COLOR,
  MENU_CONFIG_TYPE_COLOR_ARY,
  MENU_CONFIG_TYPE_SCRIPT,
  MENU_CONFIG_TYPE_DRIVER,
  MENU_CONFIG_TYPE_CHARMAP,
};

static int menu_config_set_custom_palette(char *s2, void *data);
static int menu_config_set_four_elements(char *s2, void *data);
static int menu_config_set_bgcolor(char *s2, void *data);
static int menu_config_set_ext_driver(char *s2, void *data);
static int menu_config_set_script(char *s2, void *data);
static int menu_config_set_char_map(char *s2, void *data);

static int *menu_config_menu_geometry[] = {
  &Menulocal.menux,
  &Menulocal.menuy,
  &Menulocal.menuwidth,
  &Menulocal.menuheight,
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
  {"character_map",		MENU_CONFIG_TYPE_CHARMAP, menu_config_set_char_map, &Menulocal.char_map},
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
  {"editor",			MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.editor},
  {"browser",			MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.browser},
  {"help_browser",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.help_browser},
  {"coordwin_font",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.coordwin_font},
  {"infowin_font",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.infowin_font},
  {"file_preview_font",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.file_preview_font},
  {"change_directory",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.changedirectory},
  {"save_path",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savepath},
  {"save_with_data",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithdata},
  {"save_with_merge",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.savewithmerge},
  {"expand_dir",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.expanddir},
  {"expand",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.expand},
  {"load_path",			MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.loadpath},
  {"history_size",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.hist_size},
  {"infowin_size",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.info_size},
  {"data_head_lines",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.data_head_lines},
  {"use_opacity",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.use_opacity},
  {"select_data_on_export",	MENU_CONFIG_TYPE_BOOL,    NULL, &Menulocal.select_data},
  {"use_custom_palette",	MENU_CONFIG_TYPE_BOOL,    NULL, &Menulocal.use_custom_palette},
  {"custom_palette",		MENU_CONFIG_TYPE_COLOR_ARY, menu_config_set_custom_palette, NULL},
  {"sourece_style_id",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.source_style_id},
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
  {"background_color",			MENU_CONFIG_TYPE_COLOR, menu_config_set_bgcolor, NULL},
  {NULL},
};

static struct menu_config MenuConfigToggleView[] = {
  {"viewer_show_ruler",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ruler},
  {"sidebar",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.sidebar},
  {"status_bar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.statusbar},
  {"scrollbar",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.scrollbar},
  {"command_toolbar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ctoolbar},
  {"pointer_toolbar",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ptoolbar},
  {"cross_gauge",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.show_cross},
  {"show_grid",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.show_grid},
  {NULL},
};

static struct menu_config MenuConfigOthers[] = {
  {"png_dpi",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.png_dpi},
#if WINDOWS
  {"emf_dpi",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.emf_dpi},
#endif
  {"ps_version",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.ps_version},
  {"svg_version",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.svg_version},
  {"palette",		MENU_CONFIG_TYPE_STRING,  NULL, &Menulocal.Palette},
  {"main_pane",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.main_pane_pos},
  {"side_pane1",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.side_pane1_pos},
  {"side_pane2",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.side_pane2_pos},
  {"side_pane3",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.side_pane3_pos},

  {"file_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.file_tab},
  {"axis_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.axis_tab},
  {"merge_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.merge_tab},
  {"path_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.path_tab},
  {"rectangle_tab",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.rectangle_tab},
  {"arc_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.arc_tab},
  {"mark_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.mark_tab},
  {"text_tab",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.text_tab},
  {"math_input_mode",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.math_input_mode},
  {NULL},
};

static struct menu_config MenuConfigGeometry[] = {
  {"menu_win", MENU_CONFIG_TYPE_WINDOW, menu_config_set_four_elements, menu_config_menu_geometry},
  {NULL},
};

static struct menu_config MenuConfigExtView[] = {
  {"extwin_dpi",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.exwindpi},
  {"extwin_width",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.exwinwidth},
  {"extwin_height",		MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.exwinheight},
  {"use_external_viewer",	MENU_CONFIG_TYPE_NUMERIC, NULL, &Menulocal.exwin_use_external},
  {NULL},
};

static struct menu_config *MenuConfigArrray[] = {
  MenuConfig,
  MenuConfigDriver,
  MenuConfigScript,
  MenuConfigMisc,
  MenuConfigViewer,
  MenuConfigToggleView,
  MenuConfigOthers,
  MenuConfigGeometry,
  MenuConfigExtView,
  NULL,
};

static NHASH MenuConfigHash = NULL;

static void
add_str_with_int_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;

  buf = g_strdup_printf("%s=%d", cfg->name, * (int *) cfg->data);
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
add_geometry_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;
  gint x, y, w, h;

  get_window_geometry(TopLevel, &x, &y, &w, &h);

  Menulocal.menux = x;
  Menulocal.menuy = y;
  Menulocal.menuwidth = w;
  Menulocal.menuheight = h;

  buf = g_strdup_printf("%s=%d,%d,%d,%d",
			cfg->name,
			Menulocal.menux, Menulocal.menuy,
			Menulocal.menuwidth, Menulocal.menuheight);
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
add_color_to_array(struct menu_config *cfg, struct narray *conf)
{
  char *buf;
  buf = g_strdup_printf("%s=%02x%02x%02x",
			cfg->name,
			(int) (Menulocal.bg_r * 255),
			(int) (Menulocal.bg_g * 255),
			(int) (Menulocal.bg_b * 255));
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
add_color_ary_to_array(struct menu_config *cfg, struct narray *conf)
{
  GString *str;
  char *buf;
  struct narray *palette;
  int i, n;
  GdkRGBA *color;
  palette = &(Menulocal.custom_palette);
  n = arraynum(palette);
  if (n < 1) {
    return;
  }

  str = g_string_new(cfg->name);
  g_string_append_c(str, '=');
  for (i = 0; i < n; i++) {
    unsigned int r, g, b, a;
    color = arraynget(palette, i);
    r = color->red * 0xff;
    g = color->green * 0xff;
    b = color->blue * 0xff;
    a = color->alpha * 0xff;
    g_string_append_printf(str, "%02x%02x%02x%02x%s", r, g, b, a, (i == n -1) ? "\n" : ",");
  }
  buf = g_string_free(str, FALSE);
  if (buf) {
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
save_char_map_config(struct narray *conf)
{
  char *buf;
  struct character_map_list *pcur;

  pcur = Menulocal.char_map;
  while (pcur) {
    char *title, *data;
    title = CHK_STR(pcur->title);
    data = CHK_STR(pcur->data);

    buf = g_strdup_printf("character_map=%s,%s", title, data);
    if (buf) {
      arrayadd(conf, &buf);
    }
    pcur = pcur->next;
  }
}

static void
save_ext_driver_config(struct narray *conf)
{
  char *buf;
  struct extprinter *pcur;

  pcur = Menulocal.extprinterroot;
  while (pcur) {
    char *driver, *ext, *option;
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
  char *buf;
  struct script *scur;

  scur = Menulocal.scriptroot;
  while (scur) {
    char *script, *option, *description;
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
    case MENU_CONFIG_TYPE_COLOR_ARY:
      add_color_ary_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_STRING:
      add_prm_str_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_WINDOW:
      add_geometry_to_array(cfg + i, conf);
      break;
    case MENU_CONFIG_TYPE_SCRIPT:
      save_script_config(conf);
      break;
    case MENU_CONFIG_TYPE_DRIVER:
      save_ext_driver_config(conf);
      break;
    case MENU_CONFIG_TYPE_CHARMAP:
      save_char_map_config(conf);
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

  if (type & SAVE_CONFIG_TYPE_VIEWER) {
    menu_save_config_sub(MenuConfigViewer, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_EXTERNAL_VIEWER) {
    menu_save_config_sub(MenuConfigExtView, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_TOGGLE_VIEW) {
    menu_save_config_sub(MenuConfigToggleView, &conf);
  }

  if (type & SAVE_CONFIG_TYPE_OTHERS) {
    menu_save_config_sub(MenuConfigOthers, &conf);
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
menu_config_set_bgcolor(char *s2, void *data)
{
  char *f1, *endptr;
  int len, val;

  f1 = getitok2(&s2, &len, " \t,");
  val = strtol(f1, &endptr, 16);
  if (endptr[0] == '\0') {
    Menulocal.bg_r = ((val >> 16) & 0xffU) / 255.0;
    Menulocal.bg_g = ((val >> 8) & 0xffU) / 255.0;
    Menulocal.bg_b = (val & 0xffU) / 255.0;
  }
  g_free(f1);
  return 0;
}

static int
menu_config_set_custom_palette(char *s2, void *data)
{
  gchar **colors, **ptr;
  unsigned int r, g, b, a;
  struct narray *palette;
  GdkRGBA color;

  colors = g_strsplit(s2, ",", 0);
  if (colors == NULL) {
    return 0;
  }
  palette = &(Menulocal.custom_palette);
  arrayclear(palette);
  ptr = colors;
  while (*ptr) {
    r = g = b = a = 0xff;
    sscanf(*ptr, "%02x%02x%02x%02x", &r, &g, &b, &a);
    color.red = r / 255.0;
    color.green = g / 255.0;
    color.blue = b / 255.0;
    color.alpha = a / 255.0;
    arrayadd(palette, &color);
    ptr++;
  }
  g_strfreev(colors);
  return 0;
}

static int
menu_config_set_char_map(char *s2, void *data)
{
  char *title;
  int len;
  struct character_map_list *pcur, **pptr;

  pptr = (struct character_map_list **) data;

  if (! g_utf8_validate(s2, -1, NULL)) {
    return 0;
  }

  title = getitok2(&s2, &len, ",");
  g_strstrip(title);

  for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
  g_strstrip(s2);

  pcur = g_malloc(sizeof(*pcur));
  if (pcur == NULL) {
    g_free(title);
    return 1;
  }
  pcur->title = title;
  pcur->data = g_strdup(s2);
  pcur->next = *pptr;

  *pptr = pcur;

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
	  if (endptr[0] == '\0') {
	    * (int *) (cfg->data) = val;
	  }
	}
	g_free(f1);
	break;
      case MENU_CONFIG_TYPE_BOOL:
	f1 = getitok2(&s2, &len, " \t,");
	if (f1) {
	  val = strtol(f1, &endptr, 10);
	  if (endptr[0] == '\0') {
	    * (int *) (cfg->data) = (val ? 1 : 0);
	  }
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
      case MENU_CONFIG_TYPE_CHARMAP:
      case MENU_CONFIG_TYPE_COLOR:
      case MENU_CONFIG_TYPE_COLOR_ARY:
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
      fprintf(stderr, "(%s): configuration '%s' in section %s is not used.\n", AppName, tok, MGTKCONF);
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
  const char *name;

  ocur = chkobjroot();
  while (ocur) {
    if (chkobjparent(ocur) == parent) {
      name = chkobjectname(ocur);
      arrayadd2(drawrable, name);
      menuadddrawrable(ocur, drawrable);
    }
    ocur = ocur->next;
  }
}

static void
free_script_list(struct script *script)
{
  struct script *scur;

  scur = script;
  while (scur) {
    struct script *sdel;
    sdel = scur;
    scur = scur->next;
    g_free(sdel->name);
    g_free(sdel->script);
    g_free(sdel->description);
    g_free(sdel->option);
    g_free(sdel);
  }
}

static int
free_layers(struct nhash *layers, void *ptr)
{
  struct layer *layer;
  layer = layers->val.p;
  if (layer == NULL) {
    return 0;
  }
  cairo_surface_destroy(layer->pix);
  cairo_destroy(layer->cairo);
  g_free(layer);
  return 0;
}

int
select_layer(const char *id)
{
  struct layer *layer;
  void *ptr;
  int r;
  r = nhash_get_ptr(Menulocal.layers, id, &ptr);
  if (r) {
    return 1;
  }
  layer = ptr;
  Menulocal.local->cairo = layer->cairo;
  set_cairo_antialias(layer->cairo, Menulocal.antialias);
  return 0;
}

static void
create_layer(struct layer *layer, int w, int h)
{
  layer->pix = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  layer->cairo = cairo_create(layer->pix);
}

void
init_layer(const char *obj)
{
  struct layer *layer;
  void *ptr;
  int r, w, h;
  if (Menulocal.pix == NULL) {
    return;
  }
  if (obj == NULL) {
    return;
  }
  w = cairo_image_surface_get_width(Menulocal.pix);
  h = cairo_image_surface_get_height(Menulocal.pix);
  r = nhash_get_ptr(Menulocal.layers, obj, &ptr);
  if (r) {
    layer = g_malloc(sizeof(*layer));
    if (layer == NULL) {
      return;
    }
    create_layer(layer, w, h);
    nhash_set_ptr(Menulocal.layers, obj, layer);
  } else {
    int lw, lh;
    layer = ptr;
    lw = cairo_image_surface_get_width(layer->pix);
    lh = cairo_image_surface_get_height(layer->pix);
    if (lw != w || lh != h) {
      cairo_destroy(layer->cairo);
      cairo_surface_destroy(layer->pix);
      create_layer(layer, w, h);
    }
  }
  set_cairo_antialias(layer->cairo, Menulocal.antialias);
}

static void
menulocal_finalize(void)
{
  struct extprinter *pcur;
  struct character_map_list *cmap, *cmap_tmp;
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

  cmap = Menulocal.char_map;
  while (cmap) {
    cmap_tmp = cmap;
    cmap = cmap_tmp->next;
    g_free(cmap_tmp->title);
    g_free(cmap_tmp->data);
    g_free(cmap_tmp);
  }
  Menulocal.char_map = NULL;

  pcur = Menulocal.extprinterroot;
  while (pcur) {
    struct extprinter *pdel;
    pdel = pcur;
    pcur = pcur->next;
    g_free(pdel->name);
    g_free(pdel->driver);
    g_free(pdel->ext);
    g_free(pdel->option);
    g_free(pdel);
  }
  Menulocal.extprinterroot = NULL;

  free_script_list(Menulocal.scriptroot);
  Menulocal.scriptroot = NULL;

  free_script_list(Menulocal.addin_list);
  Menulocal.addin_list = NULL;

  if (Menulocal.pix) {
    cairo_surface_destroy(Menulocal.pix);
    Menulocal.pix = NULL;
  }
  if (Menulocal.bg) {
    cairo_surface_destroy(Menulocal.bg);
    Menulocal.bg = NULL;
  }
  if (Menulocal.layers) {
    nhash_each(Menulocal.layers, free_layers, NULL);
    nhash_free(Menulocal.layers);
    Menulocal.layers = NULL;
  }

  arraydel2(&Menulocal.drawrable);
  arraydel(&Menulocal.custom_palette);

  g_free(Menulocal.fileopendir);
  Menulocal.fileopendir = NULL;

  g_free(Menulocal.graphloaddir);
  Menulocal.graphloaddir = NULL;

  Menulocal.obj = NULL;
  Menulocal.local = NULL;
}

static void
init_custom_palette(void)
{
  struct narray *palette;
  int i, j, k;
  unsigned int c[] = {0xff, 0xcc, 0x99};
  GdkRGBA color;

  palette = &(Menulocal.custom_palette);
  arrayclear(palette);
  add_default_color(palette);
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      for (k = 0; k < 3; k++) {
	color.red = c[j] / 255.0;
	color.green = c[i] / 255.0;
	color.blue = c[k] / 255.0;
	color.alpha = 1.0;
	arrayadd(palette, &color);
      }
    }
  }
  add_default_gray(palette);
}

static int
menuinit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  int layer;

  if (!OpenApplication()) {
    error(obj, ERR_MENU_DISPLAY);
    goto errexit;
  }

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv)) {
    return 1;
  }

#if GTK_SOURCE_CHECK_VERSION(4, 0, 0)
  gtk_source_init();
#endif

  layer = TRUE;
  if (_putobj(obj, "_layer", inst, &layer)) return 1;

  if (_getobj(obj, "_local", inst, &local)) {
    local = gra2cairo_free(obj, inst);
    g_free(local);
    return 1;
  }

  memset(&Menulocal, 0, sizeof(Menulocal));

  Menulocal.menux = Menulocal.menuy
    = Menulocal.menuheight = Menulocal.menuwidth = DEFAULT_GEOMETRY;
  Menulocal.showtip = TRUE;
  Menulocal.sidebar = TRUE;
  Menulocal.statusbar = TRUE;
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
  Menulocal.exwindpi = DEFAULT_DPI / 2;
  Menulocal.exwinwidth = 400;
  Menulocal.exwinheight = 600;
  Menulocal.exwin_use_external = TRUE;
  Menulocal.expand = 1;
  Menulocal.expanddir = g_strdup("./");
  Menulocal.source_style_id = NULL;
  Menulocal.loadpath = SAVE_PATH_FULL;
  Menulocal.GRAobj = chkobject("gra");
  Menulocal.hist_size = 1000;
  Menulocal.info_size = 1000;
  Menulocal.bg_r = 1.0;
  Menulocal.bg_g = 1.0;
  Menulocal.bg_b = 1.0;
  Menulocal.focus_frame_type = N_LINE_TYPE_DOT;
  Menulocal.main_pane_pos = 600;
  Menulocal.side_pane1_pos = 500;
  Menulocal.side_pane2_pos = 300;
  Menulocal.side_pane3_pos = 200;
  Menulocal.file_tab = 0;
  Menulocal.axis_tab = 100;
  Menulocal.merge_tab = 101;
  Menulocal.path_tab = 1;
  Menulocal.rectangle_tab = 2;
  Menulocal.arc_tab = 3;
  Menulocal.mark_tab = 4;
  Menulocal.text_tab = 5;
  Menulocal.math_input_mode = 1;

  arrayinit(&(Menulocal.drawrable), sizeof(char *));
  menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));

  arrayinit(&(Menulocal.custom_palette), sizeof(GdkRGBA));
  init_custom_palette();
  Menulocal.use_custom_palette = TRUE;

  Menulocal.windpi = DEFAULT_DPI;
  Menulocal.redrawf = TRUE;
  Menulocal.redrawf_num = 0xffU;
  Menulocal.grid = 200;
  Menulocal.show_grid = TRUE;
  Menulocal.data_head_lines = 100;
  Menulocal.use_opacity = FALSE;
  Menulocal.select_data = TRUE;
  Menulocal.local = local;

  Menulocal.png_dpi = 72;
#if WINDOWS
  Menulocal.emf_dpi = 576;
#endif
  Menulocal.ps_version = 0;
  Menulocal.svg_version = 0;

  if (mgtkloadconfig())
    goto errexit;

  gra2cairo_set_antialias(Menulocal.local, Menulocal.antialias);
  if (_putobj(obj, "antialias", inst, &(Menulocal.antialias)))
    goto errexit;

  if (Menulocal.exwindpi < 1)
    Menulocal.exwindpi = DEFAULT_DPI / 2;

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

  Menulocal.local->use_opacity = Menulocal.use_opacity;
  if (_putobj(obj, "use_opacity", inst, &Menulocal.use_opacity))
    goto errexit;

  Menulocal.modified = 0;
  if (_putobj(obj, "modified", inst, &Menulocal.modified))
    goto errexit;

  Menulocal.obj = obj;
  Menulocal.inst = inst;
  Menulocal.pix = NULL;
  Menulocal.bg = NULL;
  Menulocal.layers = nhash_new();
  Menulocal.lock = 0;

  return 0;

 errexit:
  menulocal_finalize();

  local = gra2cairo_free(obj, inst);
  g_free(local);

  return 1;
}

static int
menudone(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (Menulocal.lock) {
    error(obj, ERR_MENU_RUN);
    return 1;
  }

  Menulocal.local->cairo = NULL;
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
  int r;
  r = PutStdout(s);
  PutStdout("\n");
  return r;
}

int
mgtkprintfstderr(const char *fmt, ...)
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
mgtkprintfstdout(const char *fmt, ...)
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
menumenu(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
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
mx_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  reset_event();
  return 0;
}

static int
mxredrawflag(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
	     char **argv)
{
  Menulocal.redrawf = *(int *) argv[2];
  return 0;
}

static int
mxredraw_num(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
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
mxuse_opacity(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
	      char **argv)
{
  int n;

  n = *(int *) argv[2];

  Menulocal.local->use_opacity = Menulocal.use_opacity = n;

  return 0;
}

static int
mx_data_head_lines(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
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
main_window_redraw(void)
{
  GdkWindow *win;
  if (NgraphApp.Viewer.Win == NULL) {
    return;
  }

  win = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if(win == NULL) {
    return;
  }

  gdk_window_invalidate_rect(win, NULL, FALSE);
}

void
mx_redraw(struct objlist *obj, N_VALUE *inst, char **objects)
{
  int n;
  char *objs[OBJ_MAX];
  struct savedstdio save;

  if (Menulocal.region) {
    mx_clear(Menulocal.region, objects);
  }

  if (Menulocal.redrawf) {
    n = Menulocal.redrawf_num;
  } else {
    n = 0;
  }

  if (objects == NULL) {
    int i, num;
    struct narray *array;
    array = &Menulocal.drawrable;
    num = arraynum(array);
    for (i = 0; i < num; i++) {
      objs[i] = arraynget_str(array, i);
    }
    objs[i] = NULL;
    objects = objs;
  }

  ignorestdio(&save);
  GRAredraw_layers(obj, inst, TRUE, n, objects);
  restorestdio(&save);
  mxflush(obj, inst, NULL, 0, NULL);

  main_window_redraw();
}

void
mx_inslist(struct objlist *obj, N_VALUE *inst,
	   struct objlist *aobj, N_VALUE *ainst, char *afield, int addn)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAinslist(gc, aobj, ainst, chkobjectname(aobj), afield, addn);
}

void
mx_dellist(struct objlist *obj, N_VALUE *inst, int deln)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAdellist(gc, deln);
}

static int
mxredraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  mx_redraw(obj, inst, NULL);
  return 0;
}

static int
mxdpi(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
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
    Menulocal.local->pixel_dot_y = dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;

  main_window_redraw();

  return 0;
}

static void
flush_layers(cairo_t *cr)
{
  int i, n;
  struct narray *array;
  struct layer *layer;
  void *ptr;

  array = &Menulocal.drawrable;
  n = arraynum(array);
  for (i = 0; i < n; i++) {
    int r;
    char *obj;
    obj = arraynget_str(array, i);
    r = nhash_get_ptr(Menulocal.layers, obj, &ptr);
    if (r) {
      continue;
    }
    layer = ptr;
    if (layer->pix) {
      cairo_surface_flush(layer->pix);
    }
    cairo_set_source_surface(cr, layer->pix, 0, 0);
    cairo_paint(cr);
  }
}

static void
clear_region(cairo_t *cr, cairo_region_t *region)
{
  cairo_save(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  if (region) {
    gdk_cairo_region(cr, region);
    cairo_fill(cr);
  } else {
    cairo_reset_clip(cr);
    cairo_paint(cr);
  }
  cairo_restore(cr);
}

static int
mxflush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (Menulocal.local->cairo) {
    cairo_surface_t *surface;
    gra2cairo_draw_path(Menulocal.local);

    surface = cairo_get_target(Menulocal.local->cairo);
    if (surface) {
      cairo_surface_flush(surface);
    }
  }

  if (Menulocal.pix) {
    cairo_t *cr;
    cr = cairo_create(Menulocal.pix);
    clear_region(cr, NULL);
    flush_layers(cr);
    cairo_destroy(cr);
  }

  return 0;
}

static int
clear_layer(const char *obj, cairo_region_t *region)
{
  struct layer *layer;
  void *ptr;
  int r;
  r = nhash_get_ptr(Menulocal.layers, obj, &ptr);
  if (r) {
    return 1;
  }
  layer = ptr;
  clear_region(layer->cairo, region);
  return 0;
}

void
mx_clear(cairo_region_t *region, char **objects)
{
  if (objects) {
    while(*objects) {
      clear_layer(*objects, region);
      objects++;
    }
  } else {
    int i, n;
    struct narray *array;
    array = &Menulocal.drawrable;
    n = arraynum(array);
    for (i = 0; i < n; i++) {
      char *obj;
      obj = arraynget_str(array, i);
      clear_layer(obj, region);
    }
  }
}

static int
mxclear(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  mx_clear(NULL, NULL);

  main_window_redraw();

  return 0;
}

static int
mxfullpathngp(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
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
mx_get_focused(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int num, i, id;
  const char *name;
  char *ptr;
  struct narray *oarray, *sarray;
  struct Viewer *d;
  struct FocusObj **focus;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  arrayfree2(rval->array);
  rval->array = NULL;

  d = &(NgraphApp.Viewer);

  num = arraynum(d->focusobj);
  if (num < 1)
    return 0;

  oarray = arraynew(sizeof(char *));
  if (oarray == NULL)
    return 1;

  sarray = (argc > 2) ? (struct narray *) argv[2] : NULL;

  focus = arraydata(d->focusobj);
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

  rval->array = oarray;

  return 0;

}

static int
mx_print(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int show_dialog, select_file, create_window = FALSE, lock;

  select_file = * (int *) argv[2];
  show_dialog = * (int *) argv[3];

  if (TopLevel == NULL) {
    GtkWidget *label;
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
mx_echo(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
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
mx_cat(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char buf[1024];
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
mx_clear_info(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  InfoWinClear();
  return 0;
}

static int
mx_get_accel_map(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
#if USE_GTK_BUILDER
  char **actions;
  int i, j;
  GString *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  str = g_string_new("");
  actions = gtk_application_list_action_descriptions(GtkApp);
  for (i = 0; actions[i]; i++) {
    char **accels;
    accels = gtk_application_get_accels_for_action(GtkApp, actions[i]);
    for (j = 0; accels[j]; j++) {
      g_string_append_printf(str, "%s %s\n", actions[i], accels[j]);
    }
    g_strfreev(accels);
  }
  g_strfreev(actions);
#else
  FILE *fp;
  int fd;
  char buf[1024], *ptr;
  GString *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  fp = tmpfile();

  if (fp == NULL) {
    putstderr(g_strerror(errno));
    return 1;
  }

  fd = fileno(fp);
  gtk_accel_map_save_fd(fd);

  rewind(fp);

  str = g_string_new("");
  while ((ptr = fgets(buf, sizeof(buf), fp))) {
    buf[sizeof(buf) - 1] = '\0';
    g_string_append(str, buf);
  }
  fclose(fp);
#endif

  rval->str = g_string_free(str, FALSE);

  return 0;
}

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif

#ifdef HAVE_LIBGSL
#include <gsl/gsl_version.h>
#endif

static int
mx_show_lib_version(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *s, h[256];
  int i, n;
  GString *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  n = 0;
  s = argv[2];
  if (s) {
    n = s[0] - '0';
    n = (n < (int) sizeof(h) - 1) ? n : (int) sizeof(h) - 1;
  }
  for (i = 0; i < n; i++) {
    h[i] = ' ';
  }
  h[i] = '\0';

  str = g_string_new("");
  g_string_append_printf(str, "%sGTK+\n"
	       "%s compile: %d.%d.%d\n"
	       "%s  linked: %d.%d.%d\n"
	       "\n",
	       h,
	       h,
	       GTK_MAJOR_VERSION,
	       GTK_MINOR_VERSION,
	       GTK_MICRO_VERSION,
	       h,
	       gtk_major_version,
	       gtk_minor_version,
	       gtk_micro_version);

  g_string_append_printf(str, "%sGLib\n"
	       "%s compile: %d.%d.%d\n"
	       "%s  linked: %d.%d.%d\n"
	       "\n",
	       h,
	       h,
	       GLIB_MAJOR_VERSION,
	       GLIB_MINOR_VERSION,
	       GLIB_MICRO_VERSION,
	       h,
	       glib_major_version,
	       glib_minor_version,
	       glib_micro_version);

  g_string_append_printf(str, "%sCairo\n"
	       "%s compile: %s\n"
	       "%s  linked: %s\n"
	       "\n",
	       h,
	       h,
	       CAIRO_VERSION_STRING,
	       h,
	       cairo_version_string());

  g_string_append_printf(str, "%sPango\n"
	       "%s compile: %s\n"
	       "%s  linked: %s\n"
	       "\n",
	       h,
	       h,
	       PANGO_VERSION_STRING,
	       h,
	       pango_version_string());

  g_string_append_printf(str, "%sGtkSourceView\n"
	       "%s compile: %d.%d.%d\n"
	       "%s  linked: %d.%d.%d\n",
	       h,
	       h,
               GTK_SOURCE_MAJOR_VERSION,
	       GTK_SOURCE_MINOR_VERSION,
	       GTK_SOURCE_MICRO_VERSION,
	       h,
               gtk_source_get_major_version(),
               gtk_source_get_minor_version(),
               gtk_source_get_micro_version());

#ifdef RL_VERSION_MAJOR
  g_string_append(str, "\n");
  g_string_append_printf(str, "%sreadline\n"
	       "%s compile: %d.%d\n"
	       "%s  linked: %s\n",
	       h,
	       h,
	       RL_VERSION_MAJOR,
	       RL_VERSION_MINOR,
	       h,
	       rl_library_version);
#endif

#ifdef HAVE_LIBGSL
  g_string_append(str, "\n");
  g_string_append_printf(str, "%sGSL\n"
#ifdef GSL_VERSION
	       "%s compile: %s\n"
#else  /* GSL_VERSION */
	       "%s compile: %d.%d\n"
#endif	/* GSL_VERSION */
	       "%s  linked: %s\n",
	       h,
	       h,
#ifdef GSL_VERSION
	       GSL_VERSION,
#else  /* GSL_VERSION */
	       GSL_MAJOR_VERSION,
	       GSL_MINOR_VERSION,
#endif	/* GSL_VERSION */
	       h,
	       gsl_version);
#endif	/* HAVE_LIBGSL */

  rval->str = g_string_free(str, FALSE);

  return 0;
}

static int
mx_show_source_view_search_path(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  const gchar * const *dirs;
  int i;
  GtkSourceLanguageManager *lm;
  GString *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  str = g_string_new("");
  lm = gtk_source_language_manager_get_default();
  dirs = gtk_source_language_manager_get_search_path(lm);
  for (i = 0; dirs[i]; i++) {
    g_string_append_printf(str, "%s\n", dirs[i]);
  }
  rval->str = g_string_free(str, FALSE);

  return 0;
}

static int
mxdraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
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
mxmodified(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int modified;

  if (TopLevel == NULL) {
    error(obj, ERR_MENU_GUI);
    return 1;
  }

  modified = * (int *) argv[2];

  if (modified) {
    Menulocal.modified |= modified;
  } else {
    Menulocal.modified = modified;
  }
  SetCaption(modified);
  set_modified_state(modified);

  return 0;
}

static int
mx_focus_obj(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int n, i, *id_array;
  char *legend;
  struct objlist *lobj;
  struct narray iarray;

  legend = (char *) argv[2];
  if (legend == NULL) {
    return 0;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(legend, &lobj, &iarray, FALSE, NULL)) {
    return 0;
  }

  n = arraynum(&iarray);
  if (n < 1) {
    arraydel(&iarray);
    return 0;
  }

  id_array = arraydata(&iarray);

  for (i = 0; i < n; i++) {
    Focus(lobj, id_array[i], TRUE);
  }

  arraydel(&iarray);

  return 0;
}

static int
mx_unfocus_obj(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{

  UnFocus();

  return 0;
}

static int
mx_get_locale(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  const char *locale;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  locale = n_getlocale();
  if (locale) {
    rval->str = g_strdup(locale);
  }

  return 0;
}

static int
mx_get_active(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (TopLevel) {
    rval->i = TRUE;
  } else {
    rval->i = FALSE;
  }

  return 0;
}

static int
mx_addin_list_append(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int n, id, i;
  char *sarray, *name, *script, *description, *option, *argv2[2];
  struct objlist *sa_obj;
  struct narray iarray;
  struct script *addin, *list;

  sarray = (char *) argv[2];
  if (sarray  == NULL) {
    return 0;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(sarray, &sa_obj, &iarray, FALSE, NULL)) {
    return 0;
  }

  if (g_strcmp0(chkobjectname(sa_obj), "sarray") != 0) {
    return 1;
  }

  n = arraynum(&iarray);
  if (n < 1) {
    arraydel(&iarray);
    return 0;
  }

  id = arraynget_int(&iarray, 0);

  if (getobj(sa_obj, "num", id, 0, NULL, &n) < 0) {
    return 0;
  }

  if (n < 4) {
    arraydel(&iarray);
    return 0;
  }

  addin = g_malloc0(sizeof(*addin));
  if (addin == NULL) {
    return 1;
  }

  argv2[0] = (char *) &i;
  argv2[1] = NULL;

  i = 0;
  getobj(sa_obj, "get", id, 1, argv2, &script);
  if (! g_utf8_validate(script, -1, NULL)) {
    goto Err;
  }
  addin->script = g_strdup(script);

  i = 1;
  getobj(sa_obj, "get", id, 1, argv2, &name);
  if (! g_utf8_validate(name, -1, NULL)) {
    goto Err;
  }
  addin->name = g_strdup(name);

  i = 2;
  getobj(sa_obj, "get", id, 1, argv2, &description);
  if (! g_utf8_validate(description, -1, NULL)) {
    goto Err;
  }
  addin->description = g_strdup(description);

  i = 3;
  getobj(sa_obj, "get", id, 1, argv2, &option);
  if (! g_utf8_validate(option, -1, NULL)) {
    goto Err;
  }
  addin->option = g_strdup(option);

  addin->next = NULL;

  if (Menulocal.addin_list == NULL) {
    Menulocal.addin_list = addin;
  } else {
    for (list = Menulocal.addin_list; list != addin; list = list->next) {
      if (list->next == NULL) {
	list->next = addin;
      }
    }
  }

  arraydel(&iarray);

  return 0;

 Err:
  if (addin->script) {
    g_free(addin->script);
  }
  if (addin->name) {
    g_free(addin->name);
  }
  if (addin->description) {
    g_free(addin->description);
  }
  if (addin->option) {
    g_free(addin->option);
  }
  g_free(addin);

  return 0;
}

static int
mx_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char code, *cstr, *layer;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *)argv[2];
  code = *(char *) (argv[3]);
  cstr = argv[5];

  switch (code) {
  case 'I':
    layer = arraynget_str(&Menulocal.drawrable, 0);
    if (layer){
      select_layer(layer);
    }
    break;
  case 'Z':
    gra2cairo_draw_path(local);
    select_layer(cstr);
    break;
  }

  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) {
    return 1;
  }
  return 0;
}

static int
mx_exeparent(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return _exeparent(obj, argv[1], inst, rval, argc, argv);
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
  {"use_opacity", NBOOL, NREAD | NWRITE, mxuse_opacity, NULL,0},
  {"redraw", NVFUNC, NREAD | NEXEC, mxredraw, "", 0},
  {"draw", NVFUNC, NREAD | NEXEC, mxdraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, mxflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, mxclear, "", 0},
  {"focused", NSAFUNC, NREAD | NEXEC, mx_get_focused, "sa", 0},
  {"print", NVFUNC, NREAD | NEXEC, mx_print, "bi", 0},
  {"echo", NVFUNC, NREAD | NEXEC, mx_echo, "s", 0},
  {"cat", NVFUNC, NREAD | NEXEC, mx_cat, "s", 0},
  {"clear_info", NVFUNC, NREAD | NEXEC, mx_clear_info, "", 0},
  {"get_accel_map", NSFUNC, NREAD | NEXEC, mx_get_accel_map, "", 0},
  {"lib_version", NSFUNC, NREAD | NEXEC, mx_show_lib_version, NULL, 0},
  {"source_view_search_path", NSFUNC, NREAD | NEXEC, mx_show_source_view_search_path, NULL, 0},
  {"focus", NVFUNC, NREAD | NEXEC, mx_focus_obj, "o", 0},
  {"unfocus", NVFUNC, NREAD | NEXEC, mx_unfocus_obj, "", 0},
  {"locale", NSFUNC, NREAD | NEXEC, mx_get_locale, "", 0},
  {"active", NBFUNC, NREAD | NEXEC, mx_get_active, "", 0},
  {"addin_list_append", NVFUNC, NREAD | NEXEC, mx_addin_list_append, "o", 0},
  {"_evloop", NVFUNC, 0, mx_evloop, NULL, 0},
  {"_output", NVFUNC, 0, mx_output, NULL, 0},
  {"_strwidth", NIFUNC, 0, mx_exeparent, NULL, 0},
  {"_charascent", NIFUNC, 0, mx_exeparent, NULL, 0},
  {"_chardescent", NIFUNC, 0, mx_exeparent, NULL, 0},
};

#define TBLNUM (sizeof(gtkmenu) / sizeof(*gtkmenu))

void *
addmenu(void)
{
  if (MenuConfigHash == NULL) {
    unsigned int i, j;
    struct menu_config *cfg;
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
