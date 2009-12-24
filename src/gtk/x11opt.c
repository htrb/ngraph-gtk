/* 
 * $Id: x11opt.c,v 1.77 2009/12/24 10:04:18 hito Exp $
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

#include "gtk_common.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "nconfig.h"
#include "odraw.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11file.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11lgnd.h"
#include "x11opt.h"
#include "x11commn.h"
#include "x11cood.h"
#include "x11info.h"

#define BUF_SIZE 64
#define MESSAGE_BUF_SIZE 4096

#define WIN_SIZE_MIN 100
#define WIN_SIZE_MAX 2048
#define GRID_MAX 1000

static void
DefaultDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct DefaultDialog *d;
  GtkWidget *w;

  d = (struct DefaultDialog *) data;

  if (makewidget) {
    w = gtk_check_button_new_with_mnemonic(_("_Geometry"));
    d->geometry = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Child Geometry"));
    d->child_geometry = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Viewer"));
    d->viewer = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_External Viewer"));
    d->external_viewer = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Font aliases"));
    d->fonts = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_External Driver"));
    d->external_driver = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Add-in Script"));
    d->addin_script = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Miscellaneous"));
    d->misc = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->geometry), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->child_geometry), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->viewer), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->external_viewer), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->external_driver), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->addin_script), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->misc), FALSE);
}

static void
add_str_with_int_to_array(struct narray *conf, char *str, int val)
{
  char *buf;

  buf = (char *) g_malloc(BUF_SIZE);    
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%d", str, val);
    arrayadd(conf, &buf);
  }
}

static void
save_ext_viewer_config(struct narray *conf)
{
  add_str_with_int_to_array(conf, "win_dpi", Menulocal.exwindpi);
  add_str_with_int_to_array(conf, "win_width", Menulocal.exwinwidth);
  add_str_with_int_to_array(conf, "win_height", Menulocal.exwinheight);
  add_str_with_int_to_array(conf, "use_external_viewer", Menulocal.exwin_use_external);
}

static int
save_config(int type)
{
  struct narray conf;

  if (!CheckIniFile()) {
    return 1;
  }

  if (type & SAVE_CONFIG_TYPE_X11MENU) {
    menu_save_config(type);
  }

  if (type & SAVE_CONFIG_TYPE_EXTERNAL_VIEWER) {
    arrayinit(&conf, sizeof(char *));
    save_ext_viewer_config(&conf);
    replaceconfig("[gra2gtk]", &conf);
    arraydel2(&conf);
  }

  if (type & SAVE_CONFIG_TYPE_FONTS) {
    gra2cairo_save_config();
  }

  return 0;
}

static void
DefaultDialogClose(GtkWidget *win, void *data)
{
  struct DefaultDialog *d;
  unsigned int i;
  int ret, type;
  struct {
    GtkWidget *btn;
    enum SAVE_CONFIG_TYPE type;
  } btns[] = {
    {NULL, SAVE_CONFIG_TYPE_GEOMETRY},
    {NULL, SAVE_CONFIG_TYPE_CHILD_GEOMETRY},
    {NULL, SAVE_CONFIG_TYPE_VIEWER},
    {NULL, SAVE_CONFIG_TYPE_EXTERNAL_DRIVER},
    {NULL, SAVE_CONFIG_TYPE_ADDIN_SCRIPT},
    {NULL, SAVE_CONFIG_TYPE_MISC},
    {NULL, SAVE_CONFIG_TYPE_EXTERNAL_VIEWER},
    {NULL, SAVE_CONFIG_TYPE_FONTS},
  };

  d = (struct DefaultDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  btns[0].btn = d->geometry;
  btns[1].btn = d->child_geometry;
  btns[2].btn = d->viewer;
  btns[3].btn = d->external_driver;
  btns[4].btn = d->addin_script;
  btns[5].btn = d->misc;
  btns[6].btn = d->external_viewer;
  btns[7].btn = d->fonts;

  type = 0;

  for (i = 0; i < sizeof(btns) / sizeof(*btns); i++) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btns[i].btn))) {
      type |= btns[i].type;
    }
  }

  if (save_config(type)) {
    d->ret = ret;
    return;
  }

  d->ret = ret;
}

void
DefaultDialog(struct DefaultDialog *data)
{
  data->SetupWindow = DefaultDialogSetup;
  data->CloseWindow = DefaultDialogClose;
}

static void
SetScriptDialogSetupItem(GtkWidget *w, struct SetScriptDialog *d)
{
  gtk_entry_set_text(GTK_ENTRY(d->name), CHK_STR(d->Script->name));
  gtk_entry_set_text(GTK_ENTRY(d->script), CHK_STR(d->Script->script));
  gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(d->Script->option));
  gtk_entry_set_text(GTK_ENTRY(d->description), CHK_STR(d->Script->description));
}

#if USE_ENTRY_ICON
static void
SetScriptDialogBrowse(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  char *file;
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) user_data;
  if (nGetOpenFileName(TopLevel, _("Add-in Script"), "nsc", NULL,
		       NULL, &file, TRUE, FALSE) == IDOK) {
    entry_set_filename(GTK_WIDGET(w), file);
  }
  g_free(file);
}
#else
GCallback SetScriptDialogBrowse = NULL;
#endif

static void
SetScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  struct SetScriptDialog *d;
  int i;

  d = (struct SetScriptDialog *) data;
  if (makewidget) {
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Name:"), TRUE, i++);
    d->name = w;

    w = create_file_entry_with_cb(G_CALLBACK(SetScriptDialogBrowse), d);
    add_widget_to_table(table, w, _("_Script file:"), TRUE, i++);
    d->script = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Option:"), TRUE, i++);
    d->option = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Description:"), TRUE, i++);
    d->description = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
  }
  SetScriptDialogSetupItem(wi, d);
}

static int
set_scrpt_option(GtkWidget *entry, char **opt, char *msg)
{
  const char *buf;
  char *buf2;
  

  buf = gtk_entry_get_text(GTK_ENTRY(entry));
  if (msg && strlen(buf) == 0) {
    MessageBox(NULL, msg, NULL, MB_OK);
    return 1;
  }

  buf2 = g_strdup(buf);
  if (buf2) {
    g_free(*opt);
    *opt = buf2;
  }

  return 0;
}

static int
set_scrpt_file(GtkWidget *entry, char **opt, char *msg)
{
  char *buf;

  buf = entry_get_filename(entry);
  if (buf == NULL) {
    MessageBox(NULL, _("Couldn't convert filename from UTF-8."), NULL, MB_OK);
    return 1;
  }

  if (msg && strlen(buf) == 0) {
    MessageBox(NULL, msg, NULL, MB_OK);
    g_free(buf);
    return 1;
  }

  g_free(*opt);
  *opt = buf;

  return 0;
}

static void
SetScriptDialogClose(GtkWidget *w, void *data)
{
  int ret;
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  if (set_scrpt_option(d->name, &(d->Script->name), _("Please specify script name."))) {
    return;
  }

  if (set_scrpt_file(d->script, &(d->Script->script), _("Please specify script file name."))) {
    return;
  }

  if (set_scrpt_option(d->option, &(d->Script->option), NULL)) {
    return;
  }

  if (set_scrpt_option(d->description, &(d->Script->description), _("Please specify script description."))) {
    return;
  }

  d->ret = ret;
}

void
SetScriptDialog(struct SetScriptDialog *data, struct script *sc)
{
  data->SetupWindow = SetScriptDialogSetup;
  data->CloseWindow = SetScriptDialogClose;
  data->Script = sc;
}

static void
PrefScriptDialogSetupItem(struct PrefScriptDialog *d)
{
  struct script *fcur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  fcur = Menulocal.scriptroot;
  while (fcur) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->name);
    list_store_set_string(d->list, &iter, 1, fcur->script);
    list_store_set_string(d->list, &iter, 2, fcur->description);
    fcur = fcur->next;
  }
}

static void
script_free(struct script *fdel)
{
  g_free(fdel->name);
  g_free(fdel->script);
  g_free(fdel->description);
  g_free(fdel->option);
  g_free(fdel);
}

static void
script_init(struct script *fnew)
{
  fnew->next = NULL;
  fnew->name = NULL;
  fnew->script = NULL;
  fnew->option = NULL;
  fnew->description = NULL;
}

#define LIST_TYPE   script
#define LIST_ROOT   Menulocal.scriptroot
#define SET_DIALOG  DlgSetScript
#define LIST_FREE   script_free
#define LIST_INIT   script_init
#define CREATE_NAME(a, c) a ## Script ## c
#include "x11opt_proto.h"


static void
PrefScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct PrefScriptDialog *d;
  n_list_store list[] = {
    {N_("name"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("file"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("description"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefScriptDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);
    PrefScriptDialogCreateWidgets(d, NULL, sizeof(list) / sizeof(*list), list);
    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefScriptDialogSetupItem(d);
}

static void
PrefScriptDialogClose(GtkWidget *w, void *data)
{
  struct PrefScriptDialog *d;

  d = (struct PrefScriptDialog *) data;

  switch (d->ret) {
  case IDSAVE:
    save_config(SAVE_CONFIG_TYPE_ADDIN_SCRIPT);
  case IDOK:
    update_addin_menu();
  }
}

void
PrefScriptDialog(struct PrefScriptDialog *data)
{
  data->SetupWindow = PrefScriptDialogSetup;
  data->CloseWindow = PrefScriptDialogClose;
}


static void
SetDriverDialogSetupItem(GtkWidget *w, struct SetDriverDialog *d)
{
  gtk_entry_set_text(GTK_ENTRY(d->name), CHK_STR(d->Driver->name));
  gtk_entry_set_text(GTK_ENTRY(d->driver), CHK_STR(d->Driver->driver));
  gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(d->Driver->option));
  gtk_entry_set_text(GTK_ENTRY(d->ext), CHK_STR(d->Driver->ext));
}

#if USE_ENTRY_ICON
static void
SetDriverDialogBrowse(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  char *file;
  struct SetDriverDialog *d;

  d = (struct SetDriverDialog *) user_data;
  if (nGetOpenFileName(d->widget, _("External Driver"), NULL, NULL,
		       NULL, &file, TRUE, FALSE) == IDOK) {
    entry_set_filename(GTK_WIDGET(w), file);
  }
  g_free(file);
}
#else
GCallback SetDriverDialogBrowse = NULL;
#endif

static void
SetDriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  struct SetDriverDialog *d;
  int i;

  d = (struct SetDriverDialog *) data;
  if (makewidget) {
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Name:"), TRUE, i++);
    d->name = w;

    w = create_file_entry_with_cb(G_CALLBACK(SetDriverDialogBrowse), d);
    add_widget_to_table(table, w, _("_Driver:"), TRUE, i++);
    d->driver = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Option:"), TRUE, i++);
    d->option = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Extension:"), TRUE, i++);
    d->ext = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
  }
  SetDriverDialogSetupItem(wi, d);
}

static void
SetDriverDialogClose(GtkWidget *w, void *data)
{
  const char *buf;
  char *buf2;
  int ret;
  struct SetDriverDialog *d;

  d = (struct SetDriverDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->name));
  if (strlen(buf) == 0) {
    MessageBox(d->widget, _("Please specify driver name."), NULL, MB_OK);
    return;
  }

  buf2 = g_strdup(buf);
  if (buf2) {
    g_free(d->Driver->name);
    d->Driver->name = buf2;
  }

  buf2 = entry_get_filename(d->driver);
  if (buf2) {
    g_free(d->Driver->driver);
    d->Driver->driver = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->ext));
  buf2 = g_strdup(buf);
  if (buf2) {
    g_free(d->Driver->ext);
    d->Driver->ext = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->option));
  buf2 = g_strdup(buf);
  if (buf2) {
    g_free(d->Driver->option);
    d->Driver->option = buf2;
  }

  d->ret = ret;
}

void
SetDriverDialog(struct SetDriverDialog *data, struct extprinter *prn)
{
  data->SetupWindow = SetDriverDialogSetup;
  data->CloseWindow = SetDriverDialogClose;
  data->Driver = prn;
}

static void
PrefDriverDialogSetupItem(struct PrefDriverDialog *d)
{
  struct extprinter *fcur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  fcur = Menulocal.extprinterroot;
  while (fcur) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->name);
    fcur = fcur->next;
  }
}

static void
extprinter_free(struct extprinter *fdel)
{
  g_free(fdel->name);
  g_free(fdel->driver);
  g_free(fdel->option);
  g_free(fdel->ext);
  g_free(fdel);
}

static void
extprinter_init(struct extprinter *fnew)
{
  fnew->next = NULL;
  fnew->name = NULL;
  fnew->driver = NULL;
  fnew->option = NULL;
  fnew->ext = NULL;
}

#define LIST_TYPE   extprinter
#define LIST_ROOT   Menulocal.extprinterroot
#define SET_DIALOG  DlgSetDriver
#define LIST_FREE   extprinter_free
#define LIST_INIT   extprinter_init
#define CREATE_NAME(a, c) a ## Driver ## c
#include "x11opt_proto.h"

static void
PrefDriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct PrefDriverDialog *d;
  n_list_store list[] = {
    {N_("Driver"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefDriverDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);
    PrefDriverDialogCreateWidgets(d, NULL, sizeof(list) / sizeof(*list), list);
    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefDriverDialogSetupItem(d);
}

static void
PrefDriverDialogClose(GtkWidget *w, void *data)
{
  struct PrefDriverDialog *d;

  d = (struct PrefDriverDialog *) data;

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_EXTERNAL_DRIVER);
  }
}

void
PrefDriverDialog(struct PrefDriverDialog *data)
{
  data->SetupWindow = PrefDriverDialogSetup;
  data->CloseWindow = PrefDriverDialogClose;
}

static void
PrefFontDialogSetupItem(struct PrefFontDialog *d)
{
  struct fontmap *fcur;
  GtkTreeIter iter;
  const char *type;

  list_store_clear(d->list);
  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur) {
    type = gra2cairo_get_font_type_str(fcur->type);
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->fontalias);
    list_store_set_string(d->list, &iter, 1, fcur->fontname);
    list_store_set_string(d->list, &iter, 2, type);
    fcur = fcur->next;
  }
}

static GtkWidget *
create_font_selection_dialog(struct PrefFontDialog *d, struct fontmap *fcur)
{
  GtkWidget *dialog, *vbox, *w;
  static char *type[] = {
    "",
    "Bold",
    "Italic",
    "Bold Italic",
    "Oblique",
    "Bold Oblique",
  };

#define TYPE_NUM ((int) (sizeof(type) / sizeof(*type)))

  dialog = gtk_font_selection_dialog_new(_("Font"));

#ifdef JAPANESE
  vbox = GTK_DIALOG(dialog)->vbox;
  w = gtk_check_button_new_with_mnemonic(_("_Jfont"));
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  gtk_widget_show(w);
  d->two_byte = w;
#endif

  if (fcur) {
    int t;
    char *buf;

    t = (fcur->type < 0 || fcur->type >= TYPE_NUM) ? 0 : fcur->type;
    buf = g_strdup_printf("%s %s 16", fcur->fontname, type[t]);
    gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), buf);
    g_free(buf);

#ifdef JAPANESE
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->two_byte), fcur->twobyte);
#endif
  }

  return dialog;
}

static char *
get_font_alias(struct PrefFontDialog *d)
{
  const char *alias;
  char *tmp, *ptr;

  alias = gtk_entry_get_text(GTK_ENTRY(d->alias));
  tmp = g_strdup(alias);

  if (tmp == NULL)
    return NULL;

  g_strchug(tmp);

  for (ptr = tmp; *ptr != '\0'; ptr++) {
    if (*ptr == '\t')
      *ptr = ' ';
  }

  if (tmp[0] == '\0') {
    g_free(tmp);
    tmp = NULL;
  }

  return tmp;
}

static void
set_font_from_font_selection_dialog(GtkWidget *w, struct PrefFontDialog *d, struct fontmap *fcur)
{
  PangoFontDescription *pdesc;
  char *fname;
  const char *family;
  PangoStyle style;
  PangoWeight weight;
  int type, two_byte;

  fname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(w));
  pdesc = pango_font_description_from_string(fname);
  weight = pango_font_description_get_weight(pdesc);
  style = pango_font_description_get_style(pdesc);
  family = pango_font_description_get_family(pdesc);

  g_free(fname);

  if (family == NULL) {
    pango_font_description_free(pdesc);
    return;
  }

  switch (style) {
  case PANGO_STYLE_OBLIQUE:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLDOBLIQUE;
     } else {
      type = OBLIQUE;
    }
    break;
  case PANGO_STYLE_ITALIC:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLDITALIC;
    } else {
      type = ITALIC;
    }
    break;
  default:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLD;
    } else {
      type = NORMAL;
    }
  }

#ifdef JAPANESE
  two_byte = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->two_byte));
#else
  two_byte = FALSE;
#endif

  if (fcur) {
    gra2cairo_update_fontmap(fcur->fontalias, family, type, two_byte);
  } else {
    char *alias;

    alias = get_font_alias(d);
    if (alias) {
      gra2cairo_add_fontmap(alias, family, type, two_byte);
      g_free(alias);
    }
  }
  pango_font_description_free(pdesc);
}

static void
PrefFontDialogUpdate(GtkWidget *w, gpointer client_data)
{
  struct PrefFontDialog *d;
  struct fontmap *fcur;
  char *fontalias;
  GtkWidget *dialog;

  d = (struct PrefFontDialog *) client_data;

  fontalias = list_store_get_selected_string(d->list, 0);

  if (fontalias == NULL)
    return;

  fcur = gra2cairo_get_fontmap(fontalias);
  g_free(fontalias);
  if (fcur == NULL)
    return;

  dialog = create_font_selection_dialog(d, fcur);
  if (ndialog_run(dialog) != GTK_RESPONSE_CANCEL) {
    set_font_from_font_selection_dialog(dialog, d, fcur);
    PrefFontDialogSetupItem(d);
  }
  gtk_widget_destroy (dialog);
}

static void
PrefFontDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct PrefFontDialog *d;
  char *fontalias;

  d = (struct PrefFontDialog *) client_data;

  fontalias = list_store_get_selected_string(d->list, 0);
  gra2cairo_remove_fontmap(fontalias);
  g_free(fontalias);
  PrefFontDialogSetupItem(d);
}

static void
PrefFontDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct PrefFontDialog *d;
  char *alias;
  GtkWidget *dialog;
  struct fontmap *fmap;

  d = (struct PrefFontDialog *) client_data;

  alias = get_font_alias(d);
  if (alias == NULL) {
    MessageBox(d->widget, _("Please specify a new alias name."), NULL, MB_OK);
    return;
  }

  fmap = gra2cairo_get_fontmap(alias);
  g_free(alias);

  if (fmap) {
    MessageBox(d->widget, _("Alias name already exists."), NULL, MB_OK);
    return;
  }

  dialog = create_font_selection_dialog(d, NULL);

  if (ndialog_run(dialog) != GTK_RESPONSE_CANCEL) {
    set_font_from_font_selection_dialog(dialog, d, NULL);
    PrefFontDialogSetupItem(d);
    gtk_entry_set_text(GTK_ENTRY(d->alias), "");
  }
  gtk_widget_destroy (dialog);
}

#define HAVE_UPDATE_FUNC
#define LIST_TYPE   fontmap
#define LIST_ROOT   Gra2cairoConf->fontmap_list_root
#define CREATE_NAME(a, c) a ## Font ## c
#include "x11opt_proto.h"

static void
PrefFontDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *vbox;
  struct PrefFontDialog *d;
  n_list_store list[] = {
    {N_("alias"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("name"),  G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("style"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefFontDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(FALSE, FALSE);
    item_setup(vbox, w, _("_New alias:"), FALSE);
    d->alias = w;

    PrefFontDialogCreateWidgets(d, vbox, sizeof(list) / sizeof(*list), list);
    gtk_window_set_default_size(GTK_WINDOW(wi), 550, 300);
  }
  PrefFontDialogSetupItem(d);
}

static void
PrefFontDialogClose(GtkWidget *w, void *data)
{
  struct PrefFontDialog *d;

  d = (struct PrefFontDialog *) data;

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_FONTS);
  }
}

void
PrefFontDialog(struct PrefFontDialog *data)
{
  data->SetupWindow = PrefFontDialogSetup;
  data->CloseWindow = PrefFontDialogClose;
}

static void
MiscDialogSetupItem(GtkWidget *w, struct MiscDialog *d)
{
  if (Menulocal.editor)
    gtk_entry_set_text(GTK_ENTRY(d->editor), Menulocal.editor);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->directory), Menulocal.changedirectory);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->history), Menulocal.savehistory);

  combo_box_set_active(d->path, Menulocal.savepath);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->datafile), Menulocal.savewithdata);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->mergefile), Menulocal.savewithmerge);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->expand), Menulocal.expand);

  if (Menulocal.expanddir)
    gtk_entry_set_text(GTK_ENTRY(d->expanddir), Menulocal.expanddir);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->ignorepath), Menulocal.ignorepath);

  spin_entry_set_val(d->hist_size, Menulocal.hist_size);
  spin_entry_set_val(d->info_size, Menulocal.info_size);
  spin_entry_set_val(d->data_head_lines, Menulocal.data_head_lines);

  if (Menulocal.coordwin_font) {
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(d->coordwin_font), Menulocal.coordwin_font);
  }

  if (Menulocal.infowin_font) {
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(d->infowin_font), Menulocal.infowin_font);
  }

  if (Menulocal.file_preview_font) {
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(d->file_preview_font), Menulocal.file_preview_font);
  }
}

static void
MiscDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox2, *vbox2, *frame, *table;
  struct MiscDialog *d;
  int i, j;

  d = (struct MiscDialog *) data;
  if (makewidget) {
    d->show_cancel = FALSE;
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_CANCEL, IDCANCEL);
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    hbox2 = gtk_hbox_new(FALSE, 4);
    vbox2 = gtk_vbox_new(FALSE, 4);

    frame = gtk_frame_new(NULL);
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Editor:"), TRUE, i++);
    d->editor = w;
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), _("Save graph"));
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Path:"), FALSE, i++);
    d->path = w;
    for (j = 0; pathchar[j]; j++) {
      combo_box_append_text(d->path, _(pathchar[j]));
    }

    w = gtk_check_button_new_with_mnemonic(_("include _Data file"));
    d->datafile = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    w = gtk_check_button_new_with_mnemonic(_("include _Merge file"));
    d->mergefile = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), _("Load graph"));
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Expand include file"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->expand = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Expand directory:"), TRUE, i++);
    d->expanddir = w;

    w = gtk_check_button_new_with_mnemonic(_("_Ignore file path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->ignorepath = w;

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(hbox2), vbox2, TRUE, TRUE, 4);


    vbox2 = gtk_vbox_new(FALSE, 4);

    frame = gtk_frame_new(NULL);
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Check \"change current directory\""));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->directory = w;

    w = gtk_check_button_new_with_mnemonic(_("_Save file history"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->history = w;

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_spin_entry(1, HIST_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Size of completion history:"), FALSE, i++);
    d->hist_size = w;

    w = create_spin_entry(1, INFOWIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Length of information window:"), FALSE, i++);
    d->info_size = w;

    w = create_spin_entry(0, SPIN_ENTRY_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Length of data preview:"), FALSE, i++);
    d->data_head_lines = w;

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("font of _Coordinate window:"), FALSE, i++);
    d->coordwin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("font of _Information window:"), FALSE, i++);
    d->infowin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("font of data _Preview:"), FALSE, i++);
    d->file_preview_font = w;

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(hbox2), vbox2, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox2, TRUE, TRUE, 4);
  }
  MiscDialogSetupItem(wi, d);
}

static int
set_font(char **cfg, GtkWidget *btn)
{
  const char *buf;

  buf = gtk_font_button_get_font_name(GTK_FONT_BUTTON(btn));
  if (buf && *cfg) {
    if (strcmp(*cfg, buf)) {
      g_free(*cfg);
    } else {
      buf = NULL;
    }
  }

  if (buf) {
    *cfg = g_strdup(buf);
    return 1;
  }

  return 0;
}

static void
MiscDialogClose(GtkWidget *w, void *data)
{
  struct MiscDialog *d;
  int a, ret;
  const char *buf;
  char *buf2;

  d = (struct MiscDialog *) data;

  if (d->ret != IDOK && d->ret != IDSAVE)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->editor));
  if (buf) {
    buf2 = g_strdup(buf);
    if (buf2) {
      changefilename(buf2);
      g_free(Menulocal.editor);
    }
    Menulocal.editor = buf2;
  } else {
    g_free(Menulocal.editor);
    Menulocal.editor = NULL;
  }

  Menulocal.changedirectory =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->directory));

  Menulocal.savehistory =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->history));

  a = combo_box_get_active(d->path);
  if (a >= 0) {
    Menulocal.savepath = a;
  }

  Menulocal.savewithdata =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->datafile));

  Menulocal.savewithmerge =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->mergefile));

  Menulocal.expand = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->expand));

  buf = gtk_entry_get_text(GTK_ENTRY(d->expanddir));
  if (buf) {
    buf2 = g_strdup(buf);
    if (buf2) {
      g_free(Menulocal.expanddir);
      Menulocal.expanddir = buf2;
    }
  }

  Menulocal.ignorepath =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->ignorepath));

  a = spin_entry_get_val(d->hist_size);
  if (a <= HIST_SIZE_MAX && a > 0) 
    Menulocal.hist_size = a;

  a = spin_entry_get_val(d->info_size);
  if (a <= INFOWIN_SIZE_MAX && a > 0) 
    Menulocal.info_size = a;

  a = spin_entry_get_val(d->data_head_lines);
  putobj(d->Obj, "data_head_lines", d->Id, &a);

  if (set_font(&Menulocal.coordwin_font, d->coordwin_font)) {
    CoordWinSetFont(Menulocal.coordwin_font);
  }

  if (set_font(&Menulocal.infowin_font, d->infowin_font)) {
    InfoWinSetFont(Menulocal.infowin_font);
  }

  set_font(&Menulocal.file_preview_font, d->file_preview_font);

  d->ret = ret;

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_MISC);
  }
}

void
MiscDialog(struct MiscDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = MiscDialogSetup;
  data->CloseWindow = MiscDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
ExViewerDialogSetupItem(GtkWidget *w, struct ExViewerDialog *d)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_external), Menulocal.exwin_use_external);
  gtk_range_set_value(GTK_RANGE(d->dpi), Menulocal.exwindpi);
  spin_entry_set_val(d->width, Menulocal.exwinwidth);
  spin_entry_set_val(d->height, Menulocal.exwinheight);
}


static void 
use_external_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  gboolean state;
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) user_data;

  state = ! gtk_toggle_button_get_active(togglebutton);

  gtk_widget_set_sensitive(gtk_widget_get_parent(d->dpi), state);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->width), state);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->height), state);
}

static void
ExViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  struct ExViewerDialog *d;
  int i;

  d = (struct ExViewerDialog *) data;
  if (makewidget) {
    d->show_cancel = FALSE;
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_CANCEL, IDCANCEL);
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    table = gtk_table_new(1, 2, FALSE);

    i  = 0;
    w = gtk_check_button_new_with_mnemonic(_("use _External previewer"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    g_signal_connect(w, "toggled", G_CALLBACK(use_external_toggled), d);
    d->use_external = w;

    w = gtk_hscale_new_with_range(20, 620, 1);
    d->dpi = w;
    add_widget_to_table(table, w, "_DPI:", TRUE, i++);

    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("Window _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("Window _Height:"), FALSE, i++);
    d->height = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
  }
  ExViewerDialogSetupItem(wi, d);
}

static void
ExViewerDialogClose(GtkWidget *w, void *data)
{
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) data;

  if (d->ret != IDOK && d->ret != IDSAVE)
    return;

  Menulocal.exwindpi = gtk_range_get_value(GTK_RANGE(d->dpi));
  Menulocal.exwinwidth = spin_entry_get_val(d->width);
  Menulocal.exwinheight = spin_entry_get_val(d->height);
  Menulocal.exwin_use_external = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_external));

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_EXTERNAL_VIEWER);
  }
}


void
ExViewerDialog(struct ExViewerDialog *data)
{
  data->SetupWindow = ExViewerDialogSetup;
  data->CloseWindow = ExViewerDialogClose;
}

static void
ViewerDialogSetupItem(GtkWidget *w, struct ViewerDialog *d)
{
  int a;
  GdkColor color;

  getobj(d->Obj, "dpi", d->Id, 0, NULL, &(d->dpis));
  gtk_range_set_value(GTK_RANGE(d->dpi), d->dpis);

  getobj(d->Obj, "antialias", d->Id, 0, NULL, &a);
  combo_box_set_active(d->antialias, a);

  getobj(d->Obj, "redraw_flag", d->Id, 0, NULL, &a);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->loadfile), a);

  spin_entry_set_val(d->data_num, Menulocal.redrawf_num);
  spin_entry_set_val(d->grid, Menulocal.grid);

  combo_box_set_active(d->fftype, (Menulocal.focus_frame_type == GDK_LINE_SOLID) ? 0 : 1);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->preserve_width), Menulocal.preserve_width);

  color.red = Menulocal.bg_r * 257;
  color.green = Menulocal.bg_g * 257;
  color.blue = Menulocal.bg_b * 257;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(d->bgcol), &color);
}

static void
ViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *table;
  struct ViewerDialog *d;
  int i, j;

  d = (struct ViewerDialog *) data;
  if (makewidget) {
    d->show_cancel = FALSE;
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_CANCEL, IDCANCEL);
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(20, 620, 1);
    d->dpi = w;
    item_setup(hbox, w, _("_DPI:"), TRUE);
    add_widget_to_table(table, hbox, NULL, TRUE, i++);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, 1, GRID_MAX);
    add_widget_to_table(table, w, _("_Grid:"), FALSE, i++);
    d->grid = w;

    w = create_color_button(wi);
    add_widget_to_table(table, w, _("_Background Color:"), FALSE, i++);
    d->bgcol = w;

    w = combo_box_create();
    combo_box_append_text(w, _("solid"));
    combo_box_append_text(w, _("dot"));
    add_widget_to_table(table, w, _("_Line attribute of focus frame:"), FALSE, i++);
    d->fftype = w;

    w = combo_box_create();
    for (j = 0; gra2cairo_antialias_type[j]; j++) {
      combo_box_append_text(w, _(gra2cairo_antialias_type[j]));
    }
    d->antialias = w;
    add_widget_to_table(table, w, _("_Antialias:"), FALSE, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Preserve line width and style"));
    d->preserve_width = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Load files on redraw"));
    d->loadfile = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Maximum number of data on redraw:"), FALSE, i++);
    d->data_num = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
  }
  ViewerDialogSetupItem(wi, d);
}

static void
ViewerDialogClose(GtkWidget *w, void *data)
{
  struct ViewerDialog *d;
  int ret, dpi, a;
  GdkColor color;

  d = (struct ViewerDialog *) data;
 
 if (d->ret != IDOK && d->ret != IDSAVE)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  dpi = gtk_range_get_value(GTK_RANGE(d->dpi));
  if (d->dpis != dpi) {
    if (putobj(d->Obj, "dpi", d->Id, &dpi) == -1)
      return;
    d->Clear = TRUE;
  }

  a = combo_box_get_active(d->antialias);
  if (putobj(d->Obj, "antialias", d->Id, &a) == -1)
    return;
  Menulocal.antialias = a;

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->loadfile));
  a = a ? TRUE : FALSE;
  if (putobj(d->Obj, "redraw_flag", d->Id, &a) == -1)
    return;

  a = spin_entry_get_val(d->data_num);
  if (putobj(d->Obj, "redraw_num", d->Id, &a) == -1)
    return;

  Menulocal.preserve_width =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->preserve_width));

  gtk_color_button_get_color(GTK_COLOR_BUTTON(d->bgcol), &color);
  Menulocal.bg_r = color.red / 256;
  Menulocal.bg_g = color.green / 256;
  Menulocal.bg_b = color.blue / 256;

  Menulocal.grid = spin_entry_get_val(d->grid);

  a = combo_box_get_active(d->fftype);
  Menulocal.focus_frame_type = ((a == 0) ? GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH);

  d->ret = ret;

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_VIEWER);
  }
}


void
ViewerDialog(struct ViewerDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = ViewerDialogSetup;
  data->CloseWindow = ViewerDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->Clear = FALSE;
}

static void
CmOptionSaveNgp(void)
{
  char *ngpfile;
  char mes[MESSAGE_BUF_SIZE];
  int i, path;
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;

  ngpfile = getscriptname("Ngraph.ngp");
  if (ngpfile == NULL)
    return;

  path = 1;

  obj = chkobject("file");
  if (obj) {
    for (i = 0; i <= chkobjlastinst(obj); i++)
      putobj(obj, "save_path", i, &path);
  }

  obj = chkobject("merge");
  if (obj) {
    for (i = 0; i <= chkobjlastinst(obj); i++)
      putobj(obj, "save_path", i, &path);
  }

  if (access(ngpfile, 04) == 0) {
    snprintf(mes, sizeof(mes), _("`%s'\n\nOverwrite existing file?"), ngpfile);
    if (MessageBox(NULL, mes, _("Save as .Ngraph.ngp"), MB_YESNO) != IDYES) {
      g_free(ngpfile);
      return;
    }
  }

  snprintf(mes, sizeof(mes), _("Saving `%.128s'."), ngpfile);
  SetStatusBar(mes);
  SaveDrawrable(ngpfile, FALSE, FALSE);
  ResetStatusBar();
  g_free(ngpfile);
  return;
}

static void
CmOptionViewer(void)
{
  if (Menulock || GlobalLock)
    return;
  ViewerDialog(&DlgViewer, Menulocal.obj, 0);
  if (DialogExecute(TopLevel, &DlgViewer) == IDOK) {
    if (DlgViewer.Clear)
      ChangeDPI(TRUE);
  }
}

static void
CmOptionExtViewer(void)
{
  if (Menulock || GlobalLock)
    return;
  ExViewerDialog(&DlgExViewer);
  DialogExecute(TopLevel, &DlgExViewer);
}

static void
CmOptionPrefFont(void)
{
  if (Menulock || GlobalLock)
    return;
  PrefFontDialog(&DlgPrefFont);
  DialogExecute(TopLevel, &DlgPrefFont);
}

static void
CmOptionPrefDriver(void)
{
  if (Menulock || GlobalLock)
    return;
  PrefDriverDialog(&DlgPrefDriver);
  DialogExecute(TopLevel, &DlgPrefDriver);
}

static void
CmOptionScript(void)
{
  if (Menulock || GlobalLock)
    return;
  PrefScriptDialog(&DlgPrefScript);
  DialogExecute(TopLevel, &DlgPrefScript);
}

static void
CmOptionMisc(void)
{
  if (Menulock || GlobalLock)
    return;
  MiscDialog(&DlgMisc, Menulocal.obj, 0);
  DialogExecute(TopLevel, &DlgMisc);
}

static void
CmOptionSaveDefault(void)
{
  if (Menulock || GlobalLock)
    return;
  DefaultDialog(&DlgDefault);
  DialogExecute(TopLevel, &DlgDefault);
}

void
CmOptionMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdOptionViewer:
    CmOptionViewer();
    break;
  case MenuIdOptionExtViewer:
    CmOptionExtViewer();
    break;
  case MenuIdOptionPrefFont:
    CmOptionPrefFont();
    break;
  case MenuIdOptionPrefDriver:
    CmOptionPrefDriver();
    break;
  case MenuIdOptionScript:
    CmOptionScript();
    break;
  case MenuIdOptionMisc:
    CmOptionMisc();
    break;
  case MenuIdOptionSaveDefault:
    CmOptionSaveDefault();
    break;
  case MenuIdOptionSaveNgp:
    CmOptionSaveNgp();
    break;
  case MenuIdOptionFileDef:
    CmOptionFileDef();
    break;
  case MenuIdOptionTextDef:
    CmOptionTextDef();
    break;
  }
}
