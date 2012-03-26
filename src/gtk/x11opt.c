/*
 * $Id: x11opt.c,v 1.81 2010-03-04 08:30:17 hito Exp $
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
active_script_changed(GtkComboBox *widget, gpointer user_data)
{
  struct SetScriptDialog *d;
  struct script *addin;
  int i, n;

  d = (struct SetScriptDialog *) user_data;

  n = gtk_combo_box_get_active(widget);
  if (n < 1) {
    gtk_entry_set_text(GTK_ENTRY(d->name), "");
    gtk_entry_set_text(GTK_ENTRY(d->script), "");
    gtk_entry_set_text(GTK_ENTRY(d->option), "");
    gtk_entry_set_text(GTK_ENTRY(d->description), "");

    return;
  }

  addin = Menulocal.addin_list;
  for (i = 0; i < n - 1; i++) {
    if (addin == NULL) {
      return;
    }
    addin = addin->next;
  }

  gtk_entry_set_text(GTK_ENTRY(d->name), CHK_STR(addin->name));
  gtk_entry_set_text(GTK_ENTRY(d->script), CHK_STR(addin->script));
  gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(addin->option));
  gtk_entry_set_text(GTK_ENTRY(d->description), CHK_STR(addin->description));

  return;
}

static void
remove_char(char *str, int c)
{
  int i, j, n;

  if (str == NULL) {
    return;
  }

  n = strlen(str);
  for (i = 0; i < n; i++) {
    if (str[i] == c) {
      for (j = i; j < n; j++) {
	str[j] = str[j + 1];
      }
      n--;
    }
  }
}

static void
SetScriptDialogSetupItem(GtkWidget *w, struct SetScriptDialog *d)
{
  struct script *addin;
  char *title;

  combo_box_clear(d->addins);
  if (d->Script->name || Menulocal.addin_list == NULL) {
    set_widget_sensitivity_with_label(d->addins, FALSE);
  } else {
    set_widget_sensitivity_with_label(d->addins, TRUE);
    combo_box_append_text(d->addins, "Custom");
    for (addin = Menulocal.addin_list; addin; addin = addin->next) {
      title = g_strdup(addin->name);
      if (title) {
	remove_char(title, '_');
	combo_box_append_text(d->addins, title);
	g_free(title);
      }
    }
    combo_box_set_active(d->addins, 1);
    active_script_changed(GTK_COMBO_BOX(d->addins), d);
  }

  if (d->Script->name) {
    gtk_entry_set_text(GTK_ENTRY(d->name), CHK_STR(d->Script->name));
    gtk_entry_set_text(GTK_ENTRY(d->script), CHK_STR(d->Script->script));
    gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(d->Script->option));
    gtk_entry_set_text(GTK_ENTRY(d->description), CHK_STR(d->Script->description));
  }
}

#if USE_ENTRY_ICON
static void
SetScriptDialogBrowse(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  char *file;

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

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Add-in:"), TRUE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(active_script_changed), d);
    d->addins = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Name:"), TRUE, i++);
    d->name = w;

    w = create_file_entry_with_cb(G_CALLBACK(SetScriptDialogBrowse), NULL);
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
    message_box(NULL, msg, NULL, RESPONS_OK);
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
    return 1;
  }

  if (msg && strlen(buf) == 0) {
    message_box(NULL, msg, NULL, RESPONS_OK);
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
    create_addin_menu();
    break;
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
    message_box(d->widget, _("Please specify driver name."), NULL, RESPONS_OK);
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
FontSettingDialogSetupItem(GtkWidget *w, struct FontSettingDialog *d)
{
  gtk_entry_set_text(GTK_ENTRY(d->alias), CHK_STR(d->alias_str));
  gtk_editable_set_editable(GTK_EDITABLE(d->alias), ! d->is_update);

  if (d->font_str) {
    char *tmp;

    tmp = g_strdup_printf("%s, 16", d->font_str);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(d->font_b), tmp);
    g_free(tmp);
  }

  list_store_clear(d->list);
  if (d->alternative_str) {
    GtkTreeIter iter;
    gchar **ary;
    int i;

    ary = g_strsplit(d->alternative_str, ",", 0);
    for (i = 0; ary[i]; i++) {
      list_store_append(d->list, &iter);
      list_store_set_string(d->list, &iter, 0, ary[i]);
    }
    g_strfreev(ary);
  }

  d->alias_str = NULL;
  d->font_str = NULL;
  d->alternative_str = NULL;
}

static gboolean
AlternativeFontListSelCb(GtkTreeSelection *sel, gpointer user_data)
{
  int a, n;
  struct FontSettingDialog *d;

  d = (struct FontSettingDialog *) user_data;

  a = list_store_get_selected_index(d->list);
  n = list_store_get_num(d->list);

  gtk_widget_set_sensitive(d->del_b, a >= 0);
  gtk_widget_set_sensitive(d->up_b, a > 0);
  gtk_widget_set_sensitive(d->down_b, a >= 0 && a < n - 1);

  return FALSE;
}

static gchar *
get_font_family(const gchar *font_name)
{
  gchar *ptr;
  const gchar *family;
  PangoFontDescription *pdesc;

  pdesc = pango_font_description_from_string(font_name);
  family = pango_font_description_get_family(pdesc);
  if (family == NULL) {
    return NULL;
  }

  ptr = g_strdup(family);

  pango_font_description_free(pdesc);

  return ptr;
}

static void
FontSettingDialogAddAlternative(GtkWidget *w, gpointer client_data)
{
  struct FontSettingDialog *d;
  GtkWidget *dialog;

  d = (struct FontSettingDialog *) client_data;

#if GTK_CHECK_VERSION(3, 2, 0)
  dialog = gtk_font_chooser_dialog_new(_("Alternative font"), NULL);
  if (ndialog_run(dialog) != GTK_RESPONSE_CANCEL) {
    const gchar *font_name;
    PangoFontFamily *family;
    GtkTreeIter iter;

    family = gtk_font_chooser_get_font_family(GTK_FONT_CHOOSER(dialog));
    if (family) {
      font_name = pango_font_family_get_name(family);
      list_store_append(d->list, &iter);
      list_store_set_string(d->list, &iter, 0, font_name);
    }
  }
#else
  dialog = gtk_font_selection_dialog_new(_("Alternative font"));
  if (ndialog_run(dialog) != GTK_RESPONSE_CANCEL) {
    gchar *font_name, *family;
    GtkTreeIter iter;

    font_name = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dialog));
    family = get_font_family(font_name);
    g_free(font_name);
    if (family) {
      list_store_append(d->list, &iter);
      list_store_set_string(d->list, &iter, 0, family);
      g_free(family);
    }
  }
#endif

  gtk_widget_destroy (dialog);
}

static void
FontSettingDialogRemoveAlternative(GtkWidget *w, gpointer client_data)
{
  struct FontSettingDialog *d;
  GtkTreeIter iter;

  d = (struct FontSettingDialog *) client_data;

  if (list_store_get_selected_iter(d->list, &iter)) {
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->list));
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }
}

static void
FontSettingDialogDownAlternative(GtkWidget *wi, gpointer data)
{
  struct FontSettingDialog *d;
  GtkTreeIter iter, next_iter;
  GtkTreeModel *model;
  GtkTreeSelection *sel;

  d = (struct FontSettingDialog *) data;

  if (! list_store_get_selected_iter(d->list, &iter)) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->list));

  next_iter = iter;
  if (! gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &next_iter)) {
    return;
  }

  gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, &next_iter);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  AlternativeFontListSelCb(sel, data);
}

static void
FontSettingDialogUpAlternative(GtkWidget *wi, gpointer data)
{
  struct FontSettingDialog *d;
  GtkTreeIter iter, prev_iter;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeSelection *sel;

  d = (struct FontSettingDialog *) data;

  if (! list_store_get_selected_iter(d->list, &iter)) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->list));

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
  if (gtk_tree_path_prev(path)) {
    gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &prev_iter, path);
    gtk_list_store_move_before(GTK_LIST_STORE(model), &iter, &prev_iter);
  }
  gtk_tree_path_free(path);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  AlternativeFontListSelCb(sel, data);
}

static void
FontSettingDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *table, *frame, *swin;
  struct FontSettingDialog *d;
  GtkTreeSelection *sel;

  d = (struct FontSettingDialog *) data;

  if (makewidget) {
    n_list_store list[] = {
      {N_("Font name"),  G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    };

    int j;

    table = gtk_table_new(1, 2, FALSE);

    j = 0;
    w = gtk_entry_new();
    add_widget_to_table(table, w, _("_Alias:"), TRUE, j++);
    d->alias = w;

    w = gtk_font_button_new();
    gtk_font_button_set_show_size(GTK_FONT_BUTTON(w), FALSE);
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(w), FALSE);
    add_widget_to_table(table, w, _("_Font:"), TRUE, j++);
    d->font_b = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(w), TRUE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));;
    g_signal_connect(sel, "changed", G_CALLBACK(AlternativeFontListSelCb), d);

    gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogAddAlternative), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogRemoveAlternative), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(w, FALSE);
    d->del_b = w;

    w = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogDownAlternative), d);
    gtk_box_pack_end(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(w, FALSE);
    d->down_b = w;

    w = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogUpAlternative), d);
    gtk_box_pack_end(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(w, FALSE);
    d->up_b = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    frame = gtk_frame_new(_("Alternative fonts"));
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 4);
  }

  FontSettingDialogSetupItem(wi, d);
}

static char *
get_font_alias(struct FontSettingDialog *d)
{
  const char *alias;
  char *tmp, *ptr;

  alias = gtk_entry_get_text(GTK_ENTRY(d->alias));
  tmp = g_strdup(alias);

  if (tmp == NULL)
    return NULL;

  g_strstrip(tmp);

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
FontSettingDialogClose(GtkWidget *wi, void *data)
{
  struct FontSettingDialog *d;
  gchar *alias, *family, *font;
  const gchar *font_name;
  struct fontmap *fmap;
  GString *alt;
  GtkTreeIter iter;

  d = (struct FontSettingDialog *) data;

  if (d->ret == IDCANCEL) {
    return;
  }

  alias = get_font_alias(d);
  if (alias == NULL) {
    if (! d->is_update) {
      message_box(d->widget, _("Please specify a new alias name."), NULL, RESPONS_OK);
    }
    d->ret = IDLOOP;
    return;
  }

  fmap = gra2cairo_get_fontmap(alias);
  if (fmap && ! d->is_update) {
    message_box(d->widget, _("Alias name already exists."), NULL, RESPONS_OK);
    d->ret = IDLOOP;
    g_free(alias);
    return;
  }

  font_name = gtk_font_button_get_font_name(GTK_FONT_BUTTON(d->font_b));
  family = get_font_family(font_name);
  if (family == NULL) {
    g_free(alias);
    return;
  }

  if (d->is_update) {
    gra2cairo_update_fontmap(alias, family);
  } else {
    gra2cairo_add_fontmap(alias, family);
  }
  g_free(family);

  if (! list_store_get_iter_first(d->list, &iter)) {
    gra2cairo_set_alternative_font(alias, NULL);
    g_free(alias);
    return;
  }

  font = list_store_get_string(d->list, &iter, 0);
  alt = g_string_new(font);
  g_free(font);
  while (list_store_iter_next(d->list, &iter)) {
    font = list_store_get_string(d->list, &iter, 0);
    g_string_append_printf(alt, ",%s", font);
    g_free(font);
  }

  gra2cairo_set_alternative_font(alias, alt->str);
  g_string_free(alt, TRUE);
  g_free(alias);
}

void
FontSettingDialog(struct FontSettingDialog *d, const char *alias, const char *font, const char *alternative)
{
  d->SetupWindow = FontSettingDialogSetup;
  d->CloseWindow = FontSettingDialogClose;
  d->alias_str = alias;
  d->font_str = font;
  d->alternative_str = alternative;
  d->is_update = (alias != NULL);
}

static void
PrefFontDialogSetupItem(struct PrefFontDialog *d)
{
  struct fontmap *fcur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->fontalias);
    list_store_set_string(d->list, &iter, 1, fcur->fontname);
    list_store_set_string(d->list, &iter, 2, fcur->alternative);
    fcur = fcur->next;
  }
}

static void
PrefFontDialogUpdate(GtkWidget *w, gpointer client_data)
{
  struct PrefFontDialog *d;
  struct fontmap *fcur;
  char *fontalias;
  int ret;

  d = (struct PrefFontDialog *) client_data;

  fontalias = list_store_get_selected_string(d->list, 0);

  if (fontalias == NULL)
    return;

  fcur = gra2cairo_get_fontmap(fontalias);
  g_free(fontalias);
  if (fcur == NULL)
    return;

  FontSettingDialog(&DlgFontSetting, fcur->fontalias, fcur->fontname, fcur->alternative);
  ret = DialogExecute(d->widget, &DlgFontSetting);
  if (ret == IDOK) {
    PrefFontDialogSetupItem(d);
  }
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
  int ret;

  d = (struct PrefFontDialog *) client_data;

  FontSettingDialog(&DlgFontSetting, NULL, NULL, NULL);
  ret = DialogExecute(d->widget, &DlgFontSetting);
  if (ret == IDOK) {
    PrefFontDialogSetupItem(d);
  }
}

#define HAVE_UPDATE_FUNC
#define LIST_TYPE   fontmap
#define LIST_ROOT   Gra2cairoConf->fontmap_list_root
#define CREATE_NAME(a, c) a ## Font ## c
#include "x11opt_proto.h"

static void
PrefFontDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *vbox;
  struct PrefFontDialog *d;
  n_list_store list[] = {
    {N_("alias"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("name"),  G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {N_("alternative fonts"),  G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefFontDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    vbox = gtk_vbox_new(FALSE, 4);
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

  if (Menulocal.help_browser)
    gtk_entry_set_text(GTK_ENTRY(d->help_browser), Menulocal.help_browser);

  if (Menulocal.browser)
    gtk_entry_set_text(GTK_ENTRY(d->browser), Menulocal.browser);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->directory), Menulocal.changedirectory);

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

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_opacity), Menulocal.use_opacity);
}

#if USE_ENTRY_ICON
static void
set_file_in_entry(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  char *file;
  struct SetDriverDialog *d;

  d = (struct SetDriverDialog *) user_data;
  if (nGetOpenFileName(d->widget, _("Select program"), NULL, NULL,
		       NULL, &file, TRUE, FALSE) == IDOK) {
    entry_set_filename(GTK_WIDGET(w), file);
  }
  g_free(file);
}
#else
GCallback set_file_in_entry = NULL;
#endif

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

    frame = gtk_frame_new(_("External programs"));
    table = gtk_table_new(1, 2, FALSE);

    i = 0;

    w = create_file_entry_with_cb(G_CALLBACK(set_file_in_entry), d);
    add_widget_to_table(table, w, _("_Editor:"), TRUE, i++);
    d->editor = w;

    w = create_file_entry_with_cb(G_CALLBACK(set_file_in_entry), d);
    add_widget_to_table(table, w, _("_Help browser:"), TRUE, i++);
    d->help_browser = w;

    w = create_file_entry_with_cb(G_CALLBACK(set_file_in_entry), d);
    add_widget_to_table(table, w, _("_Web browser:"), TRUE, i++);
    d->browser = w;

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

    frame = gtk_frame_new(_("Size"));
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


    frame = gtk_frame_new(_("Font"));
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("_Coordinate window:"), FALSE, i++);
    d->coordwin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("_Information window:"), FALSE, i++);
    d->infowin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("data _Preview:"), FALSE, i++);
    d->file_preview_font = w;

    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

    frame = gtk_frame_new(_("Miscellaneous"));
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Check \"change current directory\""));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->directory = w;

    w = gtk_check_button_new_with_mnemonic(_("_Use opacity"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->use_opacity = w;

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
set_program_name(GtkWidget *entry, char **prm)
{
  const char *buf;
  char *buf2;

  buf = gtk_entry_get_text(GTK_ENTRY(entry));
  if (buf) {
    buf2 = g_strdup(buf);
    if (buf2) {
      changefilename(buf2);
      g_free(*prm);
    }
    *prm = buf2;
  } else {
    g_free(*prm);
    *prm = NULL;
  }
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

  set_program_name(d->editor, &Menulocal.editor);
  set_program_name(d->help_browser, &Menulocal.help_browser);
  set_program_name(d->browser, &Menulocal.browser);

  Menulocal.changedirectory =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->directory));

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

  Menulocal.use_opacity = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_opacity));
  putobj(d->Obj, "use_opacity", d->Id, &Menulocal.use_opacity);

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

  set_widget_sensitivity_with_label(d->dpi, state);
  set_widget_sensitivity_with_label(d->width, state);
  set_widget_sensitivity_with_label(d->height, state);
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

  combo_box_set_active(d->fftype, (Menulocal.focus_frame_type == N_LINE_TYPE_SOLID) ? 0 : 1);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->preserve_width), Menulocal.preserve_width);

  color.red = Menulocal.bg_r * 65535;
  color.green = Menulocal.bg_g * 65535;
  color.blue = Menulocal.bg_b * 65535;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(d->bgcol), &color);
}

static void
load_file_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  gboolean state;
  struct ViewerDialog *d;

  d = (struct ViewerDialog *) user_data;

  state = gtk_toggle_button_get_active(togglebutton);

  set_widget_sensitivity_with_label(d->data_num, state);
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
    item_setup(hbox, w, "_DPI:", TRUE);
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
    g_signal_connect(w, "toggled", G_CALLBACK(load_file_toggled), d);
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
  Menulocal.bg_r = color.red / 65535.0;
  Menulocal.bg_g = color.green / 65535.0;
  Menulocal.bg_b = color.blue / 65535.0;

  Menulocal.grid = spin_entry_get_val(d->grid);

  a = combo_box_get_active(d->fftype);
  Menulocal.focus_frame_type = ((a == 0) ? N_LINE_TYPE_SOLID : N_LINE_TYPE_DOT);

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

void
CmOptionSaveNgp(GtkAction *w, gpointer client_data)
{
  char *ngpfile;
  char mes[MESSAGE_BUF_SIZE];
  int i, path;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;

  ngpfile = getscriptname("Ngraph.ngp");
  if (ngpfile == NULL)
    return;

  path = 1;

  obj = chkobject("file");
  if (obj) {
    for (i = 0; i <= chkobjlastinst(obj); i++) {
      putobj(obj, "save_path", i, &path);
    }
  }

  obj = chkobject("merge");
  if (obj) {
    for (i = 0; i <= chkobjlastinst(obj); i++) {
      putobj(obj, "save_path", i, &path);
    }
  }

  if (naccess(ngpfile, 04) == 0) {
    snprintf(mes, sizeof(mes), _("`%s'\n\nOverwrite existing file?"), ngpfile);
    if (message_box(NULL, mes, _("Save as Ngraph.ngp"), RESPONS_YESNO) != IDYES) {
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

void
CmOptionViewer(GtkAction *w, gpointer client_data)
{
  int r;

  if (Menulock || Globallock)
    return;

  ViewerDialog(&DlgViewer, Menulocal.obj, 0);
  r = DialogExecute(TopLevel, &DlgViewer);
  if (r == IDOK && DlgViewer.Clear) {
    ChangeDPI();
  }
}

void
CmOptionExtViewer(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  ExViewerDialog(&DlgExViewer);
  DialogExecute(TopLevel, &DlgExViewer);
}

void
CmOptionPrefFont(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  PrefFontDialog(&DlgPrefFont);
  DialogExecute(TopLevel, &DlgPrefFont);
}

#if 0
static void
CmOptionPrefDriver(void)
{
  if (Menulock || Globallock)
    return;
  PrefDriverDialog(&DlgPrefDriver);
  DialogExecute(TopLevel, &DlgPrefDriver);
}
#endif

void
CmOptionScript(GtkAction *w, gpointer client_datavoid)
{
  if (Menulock || Globallock)
    return;
  PrefScriptDialog(&DlgPrefScript);
  DialogExecute(TopLevel, &DlgPrefScript);
}

void
CmOptionMisc(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  MiscDialog(&DlgMisc, Menulocal.obj, 0);
  DialogExecute(TopLevel, &DlgMisc);
}

void
CmOptionSaveDefault(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  DefaultDialog(&DlgDefault);
  DialogExecute(TopLevel, &DlgDefault);
}
