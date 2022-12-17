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

#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "nconfig.h"
#include "odraw.h"
#include "oaxis.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"
#include "gtk_presettings.h"

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
    w = gtk_check_button_new_with_mnemonic(_("_Viewer"));
    d->viewer = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_External Viewer"));
    d->external_viewer = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_Font aliases"));
    d->fonts = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_Add-in Script"));
    d->addin_script = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_Miscellaneous"));
    d->misc = w;
    gtk_box_append(GTK_BOX(d->vbox), w);
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->viewer), FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->external_viewer), FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->addin_script), FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->misc), FALSE);
}

static void
save_config_response(int ret, struct objlist *obj, int id, int type)
{
  if (! ret) {
    return;
  }

  if (type & SAVE_CONFIG_TYPE_X11MENU) {
    menu_save_config(type);
  }

  if (type & SAVE_CONFIG_TYPE_FONTS) {
    gra2cairo_save_config();
  }
}

static void
save_config(int type)
{
  CheckIniFile(save_config_response, NULL, 0, type);
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
    {NULL, SAVE_CONFIG_TYPE_VIEWER},
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

  btns[0].btn = d->viewer;
  btns[1].btn = d->addin_script;
  btns[2].btn = d->misc;
  btns[3].btn = d->external_viewer;
  btns[4].btn = d->fonts;

  type = 0;

  for (i = 0; i < sizeof(btns) / sizeof(*btns); i++) {
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(btns[i].btn))) {
      type |= btns[i].type;
    }
  }

  save_config(type);
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
    editable_set_init_text(d->name, "");
    editable_set_init_text(d->script, "");
    editable_set_init_text(d->option, "");
    editable_set_init_text(d->description, "");

    return;
  }

  addin = Menulocal.addin_list;
  for (i = 0; i < n - 1; i++) {
    if (addin == NULL) {
      return;
    }
    addin = addin->next;
  }

  editable_set_init_text(d->name, CHK_STR(addin->name));
  editable_set_init_text(d->script, CHK_STR(addin->script));
  editable_set_init_text(d->option, CHK_STR(addin->option));
  editable_set_init_text(d->description, CHK_STR(addin->description));

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

  combo_box_clear(d->addins);
  combo_box_append_text(d->addins, "Custom");
  if (Menulocal.addin_list == NULL) {
    set_widget_sensitivity_with_label(d->addins, FALSE);
  } else {
    set_widget_sensitivity_with_label(d->addins, TRUE);
    for (addin = Menulocal.addin_list; addin; addin = addin->next) {
      char *title;
      title = g_strdup(addin->name);
      if (title) {
	remove_char(title, '_');
	combo_box_append_text(d->addins, title);
	g_free(title);
      }
    }
    active_script_changed(GTK_COMBO_BOX(d->addins), d);
  }

  if (d->Script->name) {
    combo_box_set_active(d->addins, 0);
    editable_set_init_text(d->name, CHK_STR(d->Script->name));
    editable_set_init_text(d->script, CHK_STR(d->Script->script));
    editable_set_init_text(d->option, CHK_STR(d->Script->option));
    editable_set_init_text(d->description, CHK_STR(d->Script->description));
  } else {
    combo_box_set_active(d->addins, 1);
  }
}

static void
SetScriptDialogBrowse_response(char *file, gpointer user_data)
{
  GtkWidget *w;
  w = GTK_WIDGET(user_data);
  if (file) {
    entry_set_filename(w, file);
    g_free(file);
  }
}

static void
SetScriptDialogBrowse(GtkEntry *w, GtkEntryIconPosition icon_pos, gpointer user_data)
{
  nGetOpenFileName(TopLevel, _("Add-in Script"), "nsc", NULL, NULL, FALSE,
                   SetScriptDialogBrowse_response, w);
}

static void
SetScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) data;
  if (makewidget) {
    GtkWidget *w, *table;
    int i;
    table = gtk_grid_new();

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

    gtk_box_append(GTK_BOX(d->vbox), table);
  }
  SetScriptDialogSetupItem(wi, d);
}

static int
set_scrpt_option(GtkWidget *entry, char **opt, char *msg)
{
  const char *buf;
  char *buf2;


  buf = gtk_editable_get_text(GTK_EDITABLE(entry));
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
    {N_("name"), G_TYPE_STRING, TRUE, FALSE, NULL},
    {N_("file"), G_TYPE_STRING, TRUE, FALSE, NULL},
    {N_("description"), G_TYPE_STRING, TRUE, FALSE, NULL},
  };

  d = (struct PrefScriptDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Save"), IDSAVE);
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
    /* fall through */
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
FontSettingDialogSetupItem(GtkWidget *w, struct FontSettingDialog *d)
{
  editable_set_init_text(d->alias, CHK_STR(d->alias_str));
  gtk_editable_set_editable(GTK_EDITABLE(d->alias), ! d->is_update);

  if (d->font_str) {
    char *tmp;

    tmp = g_strdup_printf("%s, 16", d->font_str);
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(d->font_b), tmp);
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
FontSettingDialogAddAlternative_response(GtkWidget *dialog, gint response, gpointer client_data)
{
  struct FontSettingDialog *d;
  PangoFontFamily *family;
  GtkTreeIter iter;

  if (response != GTK_RESPONSE_OK) {
    gtk_window_destroy(GTK_WINDOW(dialog));
    return;
  }

  d = (struct FontSettingDialog *) client_data;
  family = gtk_font_chooser_get_font_family(GTK_FONT_CHOOSER(dialog));
  if (family) {
    const gchar *font_name;
    font_name = pango_font_family_get_name(family);
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, font_name);
  }
  gtk_window_destroy(GTK_WINDOW(dialog));
}

static void
FontSettingDialogAddAlternative(GtkWidget *w, gpointer client_data)
{
  struct FontSettingDialog *d;
  GtkWidget *dialog;

  d = (struct FontSettingDialog *) client_data;

  dialog = gtk_font_chooser_dialog_new(_("Alternative font"), GTK_WINDOW(d->widget));
  gtk_font_chooser_set_level(GTK_FONT_CHOOSER(dialog), GTK_FONT_CHOOSER_LEVEL_FAMILY);

  g_signal_connect(dialog, "response", G_CALLBACK(FontSettingDialogAddAlternative_response), d);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_widget_show(dialog);
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
  struct FontSettingDialog *d;

  d = (struct FontSettingDialog *) data;

  if (makewidget) {
    GtkWidget *w, *hbox, *vbox, *table, *frame, *swin;
    GtkTreeSelection *sel;
    n_list_store list[] = {
      {N_("Font name"),  G_TYPE_STRING, TRUE, FALSE, NULL},
    };

    int j;

    table = gtk_grid_new();

    j = 0;
    w = gtk_entry_new();
    add_widget_to_table(table, w, _("_Alias:"), TRUE, j++);
    d->alias = w;

    w = gtk_font_button_new();
    gtk_font_button_set_use_size(GTK_FONT_BUTTON(w), FALSE);
    gtk_font_chooser_set_level(GTK_FONT_CHOOSER(w), GTK_FONT_CHOOSER_LEVEL_FAMILY);
    add_widget_to_table(table, w, _("_Font:"), TRUE, j++);
    d->font_b = w;

    gtk_box_append(GTK_BOX(d->vbox), table);


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_widget_set_hexpand(swin, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(w), TRUE);
    d->list = w;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(AlternativeFontListSelCb), d);

    gtk_box_append(GTK_BOX(hbox), swin);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w= gtk_button_new_with_mnemonic(_("_Add"));
    set_button_icon(w, "list-add");
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogAddAlternative), d);
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_button_new_with_mnemonic(_("_Remove"));
    set_button_icon(w, "list-remove");
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogRemoveAlternative), d);
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_widget_set_sensitive(w, FALSE);
    d->del_b = w;

    w = gtk_button_new_with_mnemonic(_("_Down"));
    set_button_icon(w, "go-down");
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogDownAlternative), d);
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_widget_set_sensitive(w, FALSE);
    d->down_b = w;

    w = gtk_button_new_with_mnemonic(_("_Up"));
    set_button_icon(w, "go-up");
    g_signal_connect(w, "clicked", G_CALLBACK(FontSettingDialogUpAlternative), d);
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_widget_set_sensitive(w, FALSE);
    d->up_b = w;

    gtk_box_append(GTK_BOX(hbox), vbox);

    frame = gtk_frame_new(_("Alternative fonts"));
    gtk_frame_set_child(GTK_FRAME(frame), hbox);
    gtk_box_append(GTK_BOX(d->vbox), frame);
  }

  FontSettingDialogSetupItem(wi, d);
}

static char *
get_font_alias(struct FontSettingDialog *d)
{
  const char *alias;
  char *tmp, *ptr;

  alias = gtk_editable_get_text(GTK_EDITABLE(d->alias));
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
  gchar *font_name;
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

  font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(d->font_b));
  if (font_name == NULL) {
    g_free(alias);
    return;
  }
  family = get_font_family(font_name);
  g_free(font_name);
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
pref_font_dialog_response(struct response_callback *cb)
{
  if (cb->return_value == IDOK) {
    struct PrefFontDialog *d;
    d = (struct PrefFontDialog *) cb->data;
    PrefFontDialogSetupItem(d);
  }
}

static void
PrefFontDialogUpdate(GtkWidget *w, gpointer client_data)
{
  struct PrefFontDialog *d;
  struct fontmap *fcur;
  char *fontalias;

  d = (struct PrefFontDialog *) client_data;

  fontalias = list_store_get_selected_string(d->list, 0);

  if (fontalias == NULL)
    return;

  fcur = gra2cairo_get_fontmap(fontalias);
  g_free(fontalias);
  if (fcur == NULL)
    return;

  FontSettingDialog(&DlgFontSetting, fcur->fontalias, fcur->fontname, fcur->alternative);
  response_callback_add(&DlgFontSetting, pref_font_dialog_response, NULL, d);
  DialogExecute(d->widget, &DlgFontSetting);
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

  d = (struct PrefFontDialog *) client_data;

  FontSettingDialog(&DlgFontSetting, NULL, NULL, NULL);
  response_callback_add(&DlgFontSetting, pref_font_dialog_response, NULL, d);
  DialogExecute(d->widget, &DlgFontSetting);
}

#define HAVE_UPDATE_FUNC
#define LIST_TYPE   fontmap
#define LIST_ROOT   Gra2cairoConf->fontmap_list_root
#define CREATE_NAME(a, c) a ## Font ## c
#include "x11opt_proto.h"

static void
PrefFontDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct PrefFontDialog *d;
  n_list_store list[] = {
    {N_("alias"), G_TYPE_STRING, TRUE, FALSE, NULL},
    {N_("name"),  G_TYPE_STRING, TRUE, FALSE, NULL},
    {N_("alternative fonts"),  G_TYPE_STRING, TRUE, FALSE, NULL},
  };

  d = (struct PrefFontDialog *) data;
  if (makewidget) {
    GtkWidget *vbox;
    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Save"), IDSAVE);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
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

  presetting_set_fonts();
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
  if (Menulocal.editor) {
    editable_set_init_text(d->editor, Menulocal.editor);
  }

  if (Menulocal.help_browser) {
    editable_set_init_text(d->help_browser, Menulocal.help_browser);
  }

  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->directory), Menulocal.changedirectory);
  combo_box_set_active(d->path, Menulocal.savepath);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->datafile), Menulocal.savewithdata);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->mergefile), Menulocal.savewithmerge);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->expand), Menulocal.expand);

  if (Menulocal.expanddir) {
    editable_set_init_text(d->expanddir, Menulocal.expanddir);
  }

  combo_box_set_active(d->loadpath, Menulocal.loadpath);

  spin_entry_set_val(d->hist_size, Menulocal.hist_size);
  spin_entry_set_val(d->info_size, Menulocal.info_size);
  spin_entry_set_val(d->data_head_lines, Menulocal.data_head_lines);

  if (Menulocal.coordwin_font) {
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(d->coordwin_font), Menulocal.coordwin_font);
  }

  if (Menulocal.infowin_font) {
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(d->infowin_font), Menulocal.infowin_font);
  }

  if (Menulocal.file_preview_font) {
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(d->file_preview_font), Menulocal.file_preview_font);
  }

  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_opacity), Menulocal.use_opacity);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->select_data), Menulocal.select_data);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_custom_palette), Menulocal.use_custom_palette);
  arraycpy(&(d->tmp_palette), &(Menulocal.custom_palette));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->icon_size), Menulocal.icon_size_local == GTK_ICON_SIZE_LARGE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_dark_theme), Menulocal.use_dark_theme);

  if (Menulocal.source_style_id) {
    GtkSourceStyleSchemeManager *sman;
    GtkSourceStyleScheme *style;
    sman = gtk_source_style_scheme_manager_get_default();
    style = gtk_source_style_scheme_manager_get_scheme(sman, Menulocal.source_style_id);
    gtk_source_style_scheme_chooser_set_style_scheme(GTK_SOURCE_STYLE_SCHEME_CHOOSER(d->source_style),
                                                     style);
  }
  combo_box_set_active(d->decimalsign, Menulocal.default_decimalsign);
}

static void
set_file_in_entry_response(char *file, gpointer user_data)
{
  if (file) {
    entry_set_filename(GTK_WIDGET(user_data), file);
    g_free(file);
  }
}

static void
set_file_in_entry(GtkEntry *w, GtkEntryIconPosition icon_pos, gpointer user_data)
{
  struct MiscDialog *d;

  d = (struct MiscDialog *) user_data;
  nGetOpenFileName(d->widget, _("Select program"), NULL, NULL, NULL, FALSE,
                   set_file_in_entry_response, w);
}

#define PALETTE_COLUMN 9
static GtkWidget **
create_custom_palette_buttons(struct MiscDialog *d, GtkWidget *box)
{
  struct narray *palette;
  GdkRGBA *colors;
  GtkWidget *btn, *bbox, **btns;
  int i, n;
  GValue value = G_VALUE_INIT;

  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, TRUE);
  palette = &(d->tmp_palette);
  n = arraynum(palette);
  if (n < 1) {
    return NULL;
  }
  btns = g_malloc(sizeof(*btns) * (n + 1));
  if (btns == NULL) {
    return NULL;
  }
  colors = arraydata(palette);
  bbox = NULL;
  for (i = 0; i < n; i++) {
    if (i % PALETTE_COLUMN == 0) {
      if (bbox) {
	gtk_box_append(GTK_BOX(box), bbox);
      }
      bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    }
    btn = gtk_color_button_new_with_rgba(colors + i);
    g_object_set_property(G_OBJECT(btn), "show-editor", &value);
    gtk_box_append(GTK_BOX(bbox), btn);
    btns[i] = btn;
  }
  btns[i] = NULL;
  if (bbox) {
    gtk_box_append(GTK_BOX(box), bbox);
  }
  return btns;
}

static void
save_custom_palette(struct MiscDialog *d, GtkWidget **btns)
{
  struct narray *palette;
  GdkRGBA rgba;
  palette = &(d->tmp_palette);
  arrayclear(palette);
  while (*btns) {
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(*btns), &rgba);
    arrayadd(palette, &rgba);
    btns++;
  }
}

static void
edit_custom_palette_dialog_response(GtkDialog* self, gint response_id, gpointer user_data)
{
  struct MiscDialog *d;
  d = user_data;
  if (response_id == GTK_RESPONSE_ACCEPT) {
    save_custom_palette(d, d->palette);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_custom_palette), TRUE);
  }
  g_free(d->palette);
  d->palette = NULL;
  gtk_window_destroy(GTK_WINDOW(self));
}

static void
edit_custom_palette(GtkWidget *w, gpointer data)
{
  GtkWidget *dialog, *box, **btns;
  struct MiscDialog *d;
  d = data;
  dialog = gtk_dialog_new_with_buttons(_("custom palette"),
				       GTK_WINDOW(d->widget),
#if USE_HEADER_BAR
				       GTK_DIALOG_USE_HEADER_BAR |
#endif
				       GTK_DIALOG_MODAL,
				       _("_OK"),
				       GTK_RESPONSE_ACCEPT,
				       _("_Cancel"),
				       GTK_RESPONSE_CANCEL,
				       NULL);
  box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_orientable_set_orientation(GTK_ORIENTABLE(box), GTK_ORIENTATION_VERTICAL);
  btns = create_custom_palette_buttons(d, box);
  if (btns == NULL) {
    gtk_window_destroy(GTK_WINDOW(dialog));
    return;
  }
  d->palette = btns;
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  g_signal_connect(dialog, "response", G_CALLBACK(edit_custom_palette_dialog_response), d);
  gtk_widget_show(dialog);
}

static void
MiscDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MiscDialog *d;

  d = (struct MiscDialog *) data;
  if (makewidget) {
    GtkWidget *w, *hbox2, *vbox2, *frame, *table;
    int i, j;
    arrayinit(&(d->tmp_palette), sizeof(GdkRGBA));
    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Save"), IDSAVE);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    frame = gtk_frame_new(_("External programs"));
    table = gtk_grid_new();

    i = 0;

    w = create_file_entry_with_cb(G_CALLBACK(set_file_in_entry), d);
    add_widget_to_table(table, w, _("_Editor:"), TRUE, i++);
    d->editor = w;

    w = create_file_entry_with_cb(G_CALLBACK(set_file_in_entry), d);
    add_widget_to_table(table, w, _("_Help browser:"), TRUE, i++);
    d->help_browser = w;

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), _("Save graph"));
    table = gtk_grid_new();

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Path:"), FALSE, i++);
    for (j = 0; pathchar[j]; j++) {
      combo_box_append_text(w, _(pathchar[j]));
    }
    d->path = w;

    w = gtk_check_button_new_with_mnemonic(_("include _Data file"));
    d->datafile = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    w = gtk_check_button_new_with_mnemonic(_("include _Merge file"));
    d->mergefile = w;
    add_widget_to_table(table, w, NULL, FALSE, i++);

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), _("Load graph"));
    table = gtk_grid_new();

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Path:"), FALSE, i++);
    for (j = 0; LoadPathStr[j]; j++) {
      combo_box_append_text(w, _(LoadPathStr[j]));
    }
    d->loadpath = w;

    w = gtk_check_button_new_with_mnemonic(_("_Expand include file"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->expand = w;

    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Expand directory:"), TRUE, i++);
    d->expanddir = w;

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);
    gtk_box_append(GTK_BOX(hbox2), vbox2);


    frame = gtk_frame_new(_("Size"));
    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry(1, HIST_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Size of completion history:"), FALSE, i++);
    d->hist_size = w;

    w = create_spin_entry(1, INFOWIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Length of information view:"), FALSE, i++);
    d->info_size = w;

    w = create_spin_entry(0, HIST_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Length of data preview:"), FALSE, i++);
    d->data_head_lines = w;

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);


    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    frame = gtk_frame_new(_("Font"));
    table = gtk_grid_new();

    i = 0;
    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("_Coordinate view:"), FALSE, i++);
    d->coordwin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("_Information view:"), FALSE, i++);
    d->infowin_font = w;

    w = gtk_font_button_new();
    add_widget_to_table(table, w, _("data _Preview:"), FALSE, i++);
    d->file_preview_font = w;

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);

    frame = gtk_frame_new(_("Miscellaneous"));
    table = gtk_grid_new();

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Check \"change current directory\""));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->directory = w;

    w = gtk_check_button_new_with_mnemonic(_("_Use opacity"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->use_opacity = w;

    w = gtk_check_button_new_with_mnemonic(_("_Show select data dialog on exporting"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->select_data = w;

    w = gtk_check_button_new_with_mnemonic(_("use custom _Palette"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->use_custom_palette = w;

    w = gtk_button_new_with_mnemonic(_("_Edit custom palette"));
    g_signal_connect(w, "clicked", G_CALLBACK(edit_custom_palette), d);
    add_widget_to_table(table, w, NULL, FALSE, i++);

    w = gtk_source_style_scheme_chooser_button_new();
    add_widget_to_table(table, w, _("_Source style:"), FALSE, i++);
    d->source_style = w;

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Default decimalsign:"), FALSE, i++);
    for (j = 0; decimalsign_char[j]; j++) {
      combo_box_append_text(w, _(decimalsign_char[j]));
    }
    d->decimalsign = w;

    w = gtk_check_button_new_with_mnemonic(_("use _Large Icons (requires restart)"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->icon_size = w;

    w = gtk_check_button_new_with_mnemonic(_("use _Dark theme"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->use_dark_theme = w;

    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(vbox2), frame);
    gtk_box_append(GTK_BOX(hbox2), vbox2);
    gtk_box_append(GTK_BOX(d->vbox), hbox2);
  }
  MiscDialogSetupItem(wi, d);
}

static int
set_font(char **cfg, GtkWidget *btn)
{
  char *buf;

  buf = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(btn));
  if (buf && *cfg) {
    if (strcmp(*cfg, buf)) {
      g_free(*cfg);
    } else {
      g_free(buf);
      buf = NULL;
    }
  }

  if (buf) {
    *cfg = buf;
    return 1;
  }

  return 0;
}

static void
set_program_name(GtkWidget *entry, char **prm)
{
  const char *buf;
  char *buf2;

  buf = gtk_editable_get_text(GTK_EDITABLE(entry));
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
  const char *buf, *source_style_id;
  char *buf2;
  GtkSourceStyleScheme *source_style;

  d = (struct MiscDialog *) data;

  if (d->ret != IDOK && d->ret != IDSAVE)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  set_program_name(d->editor, &Menulocal.editor);
  set_program_name(d->help_browser, &Menulocal.help_browser);

#if GTK_CHECK_VERSION(4, 0, 0)
  Menulocal.changedirectory =
    gtk_check_button_get_active(GTK_CHECK_BUTTON(d->directory));

  a = combo_box_get_active(d->path);
  if (a >= 0) {
    Menulocal.savepath = a;
  }

  Menulocal.savewithdata =
    gtk_check_button_get_active(GTK_CHECK_BUTTON(d->datafile));

  Menulocal.savewithmerge =
    gtk_check_button_get_active(GTK_CHECK_BUTTON(d->mergefile));

  Menulocal.expand = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->expand));
#else
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  buf = gtk_editable_get_text(GTK_EDITABLE(d->expanddir));
#else
  buf = gtk_entry_get_text(GTK_ENTRY(d->expanddir));
#endif
  if (buf) {
    buf2 = g_strdup(buf);
    if (buf2) {
      g_free(Menulocal.expanddir);
      Menulocal.expanddir = buf2;
    }
  }

  Menulocal.loadpath = combo_box_get_active(d->loadpath);

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

#if GTK_CHECK_VERSION(4, 0, 0)
  Menulocal.use_opacity = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->use_opacity));
  putobj(d->Obj, "use_opacity", d->Id, &Menulocal.use_opacity);

  Menulocal.select_data = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->select_data));
  Menulocal.use_custom_palette = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->use_custom_palette));
#else
  Menulocal.use_opacity = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_opacity));
  putobj(d->Obj, "use_opacity", d->Id, &Menulocal.use_opacity);

  Menulocal.select_data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->select_data));
  Menulocal.use_custom_palette = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_custom_palette));
#endif
  if (arraycmp(&(Menulocal.custom_palette), &(d->tmp_palette))) {
    arraycpy(&(Menulocal.custom_palette), &(d->tmp_palette));
  }
  arraydel(&(d->tmp_palette));

#if GTK_CHECK_VERSION(4, 0, 0)
  Menulocal.icon_size_local = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->icon_size)) ? GTK_ICON_SIZE_LARGE : GTK_ICON_SIZE_NORMAL;
  menu_use_dark_theme_set(gtk_check_button_get_active(GTK_CHECK_BUTTON(d->use_dark_theme)));
#else
  menu_use_dark_theme_set(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_dark_theme)));
#endif

  d->ret = ret;

  source_style = gtk_source_style_scheme_chooser_get_style_scheme(GTK_SOURCE_STYLE_SCHEME_CHOOSER(d->source_style));
  source_style_id = gtk_source_style_scheme_get_id(source_style);
  if (g_strcmp0(Menulocal.source_style_id, source_style_id)) {
    if (Menulocal.source_style_id) {
      g_free(Menulocal.source_style_id);
    }
    Menulocal.source_style_id = g_strdup(source_style_id);
  }
  Menulocal.default_decimalsign = combo_box_get_active(d->decimalsign);
  gra_set_default_decimalsign(get_gra_decimalsign_type(Menulocal.default_decimalsign));
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_external), Menulocal.exwin_use_external);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_external), Menulocal.exwin_use_external);
#endif
  gtk_range_set_value(GTK_RANGE(d->dpi), Menulocal.exwindpi);
  spin_entry_set_val(d->width, Menulocal.exwinwidth);
  spin_entry_set_val(d->height, Menulocal.exwinheight);
}


static void
#if GTK_CHECK_VERSION(4, 0, 0)
use_external_toggled(GtkCheckButton *checkbutton, gpointer user_data)
#else
use_external_toggled(GtkToggleButton *togglebutton, gpointer user_data)
#endif
{
  gboolean state;
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) user_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  state = ! gtk_check_button_get_active(checkbutton);
#else
  state = ! gtk_toggle_button_get_active(togglebutton);
#endif

  set_widget_sensitivity_with_label(d->dpi, state);
  set_widget_sensitivity_with_label(d->width, state);
  set_widget_sensitivity_with_label(d->height, state);
}

static void
ExViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) data;
  if (makewidget) {
    GtkWidget *w, *table;
    int i;
    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Save"), IDSAVE);

    table = gtk_grid_new();

    i  = 0;
    w = gtk_check_button_new_with_mnemonic(_("use _External previewer"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    g_signal_connect(w, "toggled", G_CALLBACK(use_external_toggled), d);
    d->use_external = w;

    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 20, 620, 1);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scale_set_draw_value(GTK_SCALE(w), TRUE);
#endif
    d->dpi = w;
    add_widget_to_table(table, w, "_DPI:", TRUE, i++);

    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("Window _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("Window _Height:"), FALSE, i++);
    d->height = w;

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), table);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  Menulocal.exwin_use_external = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->use_external));
#else
  Menulocal.exwin_use_external = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_external));
#endif

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
  GdkRGBA color;

  getobj(d->Obj, "dpi", d->Id, 0, NULL, &(d->dpis));
  gtk_range_set_value(GTK_RANGE(d->dpi), d->dpis);

  getobj(d->Obj, "antialias", d->Id, 0, NULL, &a);
  combo_box_set_active(d->antialias, a);

  getobj(d->Obj, "redraw_flag", d->Id, 0, NULL, &a);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->loadfile), a);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->loadfile), a);
#endif

  spin_entry_set_val(d->data_num, Menulocal.redrawf_num);
  spin_entry_set_val(d->grid, Menulocal.grid);

  combo_box_set_active(d->fftype, (Menulocal.focus_frame_type == N_LINE_TYPE_SOLID) ? 0 : 1);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->preserve_width), Menulocal.preserve_width);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->preserve_width), Menulocal.preserve_width);
#endif

  color.red = Menulocal.bg_r;
  color.green = Menulocal.bg_g;
  color.blue = Menulocal.bg_b;
  color.alpha = 1;
  gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(d->bgcol), FALSE);
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(d->bgcol), &color);
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
load_file_toggled(GtkCheckButton *checkbutton, gpointer user_data)
#else
load_file_toggled(GtkToggleButton *togglebutton, gpointer user_data)
#endif
{
  gboolean state;
  struct ViewerDialog *d;

  d = (struct ViewerDialog *) user_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  state = gtk_check_button_get_active(checkbutton);
#else
  state = gtk_toggle_button_get_active(togglebutton);
#endif

  set_widget_sensitivity_with_label(d->data_num, state);
}

static void
ViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct ViewerDialog *d;

  d = (struct ViewerDialog *) data;
  if (makewidget) {
    GtkWidget *w, *table;
    int i, j;
    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Save"), IDSAVE);

    table = gtk_grid_new();

    i = 0;
    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 20, 620, 1);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scale_set_draw_value(GTK_SCALE(w), TRUE);
#endif
    d->dpi = w;
    add_widget_to_table(table, w, "_DPI:", TRUE, i++);

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

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), table);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
  ViewerDialogSetupItem(wi, d);
}

static void
ViewerDialogClose(GtkWidget *w, void *data)
{
  struct ViewerDialog *d;
  int ret, dpi, a, bg;
  GdkRGBA color;

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

#if GTK_CHECK_VERSION(4, 0, 0)
  a = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->loadfile));
#else
  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->loadfile));
#endif
  a = a ? TRUE : FALSE;
  if (putobj(d->Obj, "redraw_flag", d->Id, &a) == -1)
    return;

  a = spin_entry_get_val(d->data_num);
  if (putobj(d->Obj, "redraw_num", d->Id, &a) == -1)
    return;

#if GTK_CHECK_VERSION(4, 0, 0)
  Menulocal.preserve_width =
    gtk_check_button_get_active(GTK_CHECK_BUTTON(d->preserve_width));
#else
  Menulocal.preserve_width =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->preserve_width));
#endif

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(d->bgcol), &color);
  bg = (Menulocal.bg_r != color.red || Menulocal.bg_g != color.green || Menulocal.bg_b != color.blue);
  Menulocal.bg_r = color.red;
  Menulocal.bg_g = color.green;
  Menulocal.bg_b = color.blue;

  Menulocal.grid = spin_entry_get_val(d->grid);

  a = combo_box_get_active(d->fftype);
  Menulocal.focus_frame_type = ((a == 0) ? N_LINE_TYPE_SOLID : N_LINE_TYPE_DOT);

  d->ret = ret;

  if (d->ret == IDSAVE) {
    save_config(SAVE_CONFIG_TYPE_VIEWER);
  }
  if (d->ret == IDOK && d->Clear) {
    ChangeDPI();
  }
  if (bg) {
    update_bg();
    UpdateAll2(NULL, TRUE);
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

#if GTK_CHECK_VERSION(4, 0, 0)
static void
save_default_axis_config_response(int ret, struct objlist *obj, int id, int modified)
{
  if (ret) {
    exeobj(obj, "save_config", id, 0, NULL);
  }
}
#endif

static void
save_default_axis_config(void)
{
  int n;
  struct objlist *obj;

  obj = chkobject("axis");
  if (obj == NULL) {
    return;
  }
  n = chkobjlastinst(obj);
  if (n < 0) {
    return;
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  CheckIniFile(save_default_axis_config_response, obj, 0, 0);
#else
  if (CheckIniFile()) {
    exeobj(obj, "save_config", 0, 0, NULL);
  }
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
CmOptionSaveNgp_response(int ret, gpointer user_data)
{
  char *ngpfile, mes[MESSAGE_BUF_SIZE];
  ngpfile = (char *) user_data;

  if (ret != IDYES) {
    return;
  }
  snprintf(mes, sizeof(mes), _("Saving `%.128s'."), ngpfile);
  SetStatusBar(mes);
  SaveDrawrable(ngpfile, FALSE, FALSE, FALSE);
  ResetStatusBar();
  menu_default_axis_size(&Menulocal);
  save_default_axis_config();
  g_free(ngpfile);
}
#endif

void
CmOptionSaveNgp(void *w, gpointer client_data)
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

  obj = chkobject("data");
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
#if GTK_CHECK_VERSION(4, 0, 0)
    response_message_box(NULL, mes, _("Save as Ngraph.ngp"), RESPONS_YESNO, CmOptionSaveNgp_response, ngpfile);
    return;
#else
    if (message_box(NULL, mes, _("Save as Ngraph.ngp"), RESPONS_YESNO) != IDYES) {
      g_free(ngpfile);
      return;
    }
#endif
  }

  snprintf(mes, sizeof(mes), _("Saving `%.128s'."), ngpfile);
  SetStatusBar(mes);
  SaveDrawrable(ngpfile, FALSE, FALSE, FALSE);
  ResetStatusBar();
  menu_default_axis_size(&Menulocal);
  save_default_axis_config();
  g_free(ngpfile);
  return;
}

void
CmOptionViewer(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  ViewerDialog(&DlgViewer, Menulocal.obj, 0);
  DialogExecute(TopLevel, &DlgViewer);
}

void
CmOptionExtViewer(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  ExViewerDialog(&DlgExViewer);
  DialogExecute(TopLevel, &DlgExViewer);
}

void
CmOptionPrefFont(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  PrefFontDialog(&DlgPrefFont);
  DialogExecute(TopLevel, &DlgPrefFont);
}

void
CmOptionScript(void *w, gpointer client_datavoid)
{
  if (Menulock || Globallock)
    return;
  PrefScriptDialog(&DlgPrefScript);
  DialogExecute(TopLevel, &DlgPrefScript);
}

void
CmOptionMisc(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  MiscDialog(&DlgMisc, Menulocal.obj, 0);
  DialogExecute(TopLevel, &DlgMisc);
}

void
CmOptionSaveDefault(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  DefaultDialog(&DlgDefault);
  DialogExecute(TopLevel, &DlgDefault);
}
