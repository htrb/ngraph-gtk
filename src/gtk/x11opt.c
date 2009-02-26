/* 
 * $Id: x11opt.c,v 1.49 2009/02/26 03:29:13 hito Exp $
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

#define BUF_SIZE 64
#define MESSAGE_BUF_SIZE 4096

#define WIN_SIZE_MIN 100
#define WIN_SIZE_MAX 2048
#define GRID_MAX 1000

#define CHK_STR(s) ((s == NULL) ? "" : s)

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

    w = gtk_check_button_new_with_mnemonic(_("_Font aliases"));
    d->fonts = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Viewer"));
    d->viewer = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_External Viewer"));
    d->external_viewer = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_External Driver"));
    d->external_driver = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Addin Script"));
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
add_str_to_array(struct narray *conf, char *str)
{
  char *buf;

  buf = nstrdup(str);
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
add_prm_str_to_array(struct narray *conf, char *str, char *prm)
{
  char *buf;
  int len;

  if (prm) {
    len = strlen(prm) + strlen(str) + 2;
    buf = (char *) memalloc(len);
    if (buf) {
      snprintf(buf, len, "%s=%s", str, prm);
      arrayadd(conf, &buf);
    }
  }
}

static void
add_str_with_int_to_array(struct narray *conf, char *str, int val)
{
  char *buf;

  buf = (char *) memalloc(BUF_SIZE);    
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%d", str, val);
    arrayadd(conf, &buf);
  }
}

static void
save_geometory_config(struct narray *conf)
{
  ;
  char *buf;
  GdkWindowState state;
  gint x, y, w, h;

  get_window_geometry(TopLevel, &x, &y, &w, &h, &state);

  Menulocal.menux = x;
  Menulocal.menuy = y;
  Menulocal.menuwidth = w;
  Menulocal.menuheight = h;

  buf = (char *) memalloc(BUF_SIZE);    
  if (buf) {
    snprintf(buf, BUF_SIZE, "menu_win=%d,%d,%d,%d",
	     Menulocal.menux, Menulocal.menuy,
	     Menulocal.menuwidth, Menulocal.menuheight);
    arrayadd(conf, &buf);
  }
}

static void
add_geometry_to_array(struct narray *conf, char *str, int x, int y, int w, int h, int stat)
{
  char *buf;

  buf = (char *) memalloc(BUF_SIZE);    
  if (buf) {
    snprintf(buf, BUF_SIZE, "%s=%d,%d,%d,%d,%d",
	     str, x, y, w, h, stat);
    arrayadd(conf, &buf);
  }
}

static void
save_child_geometory_config(struct narray *conf)
{
  sub_window_save_geometry(&(NgraphApp.FileWin));
  add_geometry_to_array(conf, "file_win",
			Menulocal.filex, Menulocal.filey,
			Menulocal.filewidth, Menulocal.fileheight,
			Menulocal.fileopen);

  sub_window_save_geometry(&(NgraphApp.AxisWin));
  add_geometry_to_array(conf, "axis_win",
			Menulocal.axisx, Menulocal.axisy,
			Menulocal.axiswidth, Menulocal.axisheight,
			Menulocal.axisopen);

  sub_window_save_geometry((struct SubWin *) &(NgraphApp.LegendWin));
  add_geometry_to_array(conf, "legend_win",
			Menulocal.legendx, Menulocal.legendy,
			Menulocal.legendwidth, Menulocal.legendheight,
			Menulocal.legendopen);

  sub_window_save_geometry(&(NgraphApp.MergeWin));
  add_geometry_to_array(conf, "merge_win",
			Menulocal.mergex, Menulocal.mergey,
			Menulocal.mergewidth, Menulocal.mergeheight,
			Menulocal.mergeopen);

  sub_window_save_geometry((struct SubWin *) &(NgraphApp.InfoWin));
  add_geometry_to_array(conf, "information_win",
			Menulocal.dialogx, Menulocal.dialogy,
			Menulocal.dialogwidth, Menulocal.dialogheight,
			Menulocal.dialogopen);

  sub_window_save_geometry((struct SubWin *) &(NgraphApp.CoordWin));
  add_geometry_to_array(conf, "coordinate_win",
			Menulocal.coordx, Menulocal.coordy,
			Menulocal.coordwidth, Menulocal.coordheight,
			Menulocal.coordopen);
}

static void
save_viewer_config(struct narray *conf)
{
  add_str_with_int_to_array(conf, "viewer_dpi", Mxlocal->windpi);
  add_str_with_int_to_array(conf, "antialias", Mxlocal->antialias);
  add_str_with_int_to_array(conf, "viewer_auto_redraw", Mxlocal->autoredraw);
  add_str_with_int_to_array(conf, "viewer_load_file_on_redraw", Mxlocal->redrawf);
  add_str_with_int_to_array(conf, "viewer_load_file_data_number", Mxlocal->redrawf_num);
  add_str_with_int_to_array(conf, "viewer_show_ruler", Mxlocal->ruler);
  add_str_with_int_to_array(conf, "viewer_grid", Mxlocal->grid);

  add_str_with_int_to_array(conf, "status_bar", Menulocal.statusb);
}

static void
save_ext_driver_config(struct narray *conf)
{
  char *buf, *driver, *ext, *option;
  struct extprinter *pcur;
  int len;

  pcur = Menulocal.extprinterroot;
  while (pcur) {
    driver = CHK_STR(pcur->driver);
    ext = CHK_STR(pcur->ext);
    option= CHK_STR(pcur->option);

    len = strlen(pcur->name) + strlen(driver) + strlen(ext) + strlen(option) + 20;

    buf = (char *) memalloc(len);
    if (buf) {
      snprintf(buf, len, "ext_driver=%s,%s,%s,%s", pcur->name, driver, ext, option);
      arrayadd(conf, &buf);
    }
    pcur = pcur->next;
  }
}

static void
save_script_config(struct narray *conf)
{
  char *buf, *script, *option, *description;
  int len;
  struct script *scur;

  scur = Menulocal.scriptroot;
  while (scur) {
    script = CHK_STR(scur->script);
    option = CHK_STR(scur->option);
    description = CHK_STR(scur->description);

    len = strlen(scur->name) + strlen(script) + strlen(description) + strlen(option) + 20;
    buf = (char *) memalloc(len);
    if (buf) {
      snprintf(buf, len, "script=%s,%s,%s,%s", scur->name, script, description, option);
      arrayadd(conf, &buf);
    }
    scur = scur->next;
  }
}

static void
save_misc_config(struct narray *conf)
{
  char *buf;

  add_prm_str_to_array(conf, "editor", Menulocal.editor);
  add_prm_str_to_array(conf, "coordwin_font", Menulocal.coordwin_font);

  add_str_with_int_to_array(conf, "change_directory", Menulocal.changedirectory);
  add_str_with_int_to_array(conf, "save_history", Menulocal.savehistory);
  add_str_with_int_to_array(conf, "save_path", Menulocal.savepath);
  add_str_with_int_to_array(conf, "save_with_data", Menulocal.savewithdata);
  add_str_with_int_to_array(conf, "save_with_merge", Menulocal.savewithmerge);

  buf = (char *) memalloc(strlen(Menulocal.expanddir) + 20);
  if (buf) {
    snprintf(buf, BUF_SIZE, "expand_dir=%s", Menulocal.expanddir);
    arrayadd(conf, &buf);
  }

  add_str_with_int_to_array(conf, "expand", Menulocal.expand);
  add_str_with_int_to_array(conf, "ignore_path", Menulocal.ignorepath);
  add_str_with_int_to_array(conf, "history_size", Menulocal.hist_size);
  add_str_with_int_to_array(conf, "preserve_width", Menulocal.preserve_width);
  add_str_with_int_to_array(conf, "infowin_size", Menulocal.info_size);
  add_str_with_int_to_array(conf, "data_head_lines", Mxlocal->data_head_lines);

  buf = (char *) memalloc(BUF_SIZE);
  if (buf) {
    snprintf(buf, BUF_SIZE, "background_color=%02x%02x%02x",
	     Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
    arrayadd(conf, &buf);
  }

  add_str_with_int_to_array(conf, "focus_frame_type", Menulocal.focus_frame_type);
}

static void
save_ext_viewer_config(struct narray *conf)
{
  add_str_with_int_to_array(conf, "win_dpi", Menulocal.exwindpi);
  add_str_with_int_to_array(conf, "win_width", Menulocal.exwinwidth);
  add_str_with_int_to_array(conf, "win_height", Menulocal.exwinheight);
  add_str_with_int_to_array(conf, "use_external_viewer", Menulocal.exwin_use_external);
}

static void
save_font_config(struct narray *conf)
{
  char *buf;
  int len;
  struct fontmap *fcur;

  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur) {
    len = strlen(fcur->fontalias) + strlen(fcur->fontname) + 64;
    buf = (char *) memalloc(len);
    if (buf) {
      snprintf(buf, len,
	       "font_map=%s,%s,%d,%s",
	       fcur->fontalias,
	       gra2cairo_get_font_type_str(fcur->type),
	       (fcur->twobyte) ? 1 : 0,
	       fcur->fontname);
      arrayadd(conf, &buf);
    }
    fcur = fcur->next;
  }
}

static void
DefaultDialogClose(GtkWidget *win, void *data)
{
  struct DefaultDialog *d;
  int ret;
  struct narray conf;

  d = (struct DefaultDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  if (!CheckIniFile()) {
    d->ret = ret;
    return;
  }

  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->geometry))) {
    save_geometory_config(&conf);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->child_geometry))) {
    save_child_geometory_config(&conf);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->viewer))) {
    save_viewer_config(&conf);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_driver))) {
    save_ext_driver_config(&conf);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->addin_script))) {
    save_script_config(&conf);
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->misc))) {
    save_misc_config(&conf);
  }

  replaceconfig("[x11menu]", &conf);
  arraydel2(&conf);


  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_viewer))) {
    save_ext_viewer_config(&conf);
  }
  replaceconfig("[gra2gtk]", &conf);
  arraydel2(&conf);


  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->fonts))) {
    save_font_config(&conf);
  }
  replaceconfig("[gra2cairo]", &conf);
  arraydel2(&conf);


  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_driver))) {
    if (Menulocal.extprinterroot == NULL) {
      add_str_to_array(&conf, "ext_driver");
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->addin_script))) {
    if (Menulocal.scriptroot == NULL) {
      add_str_to_array(&conf, "script");
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->misc))) {
    if (Menulocal.editor == NULL) {
      add_str_to_array(&conf, "editor");
    }
    if (Menulocal.coordwin_font == NULL) {
      add_str_to_array(&conf, "coordwin_font");
    }
  }
  removeconfig("[x11menu]", &conf);
  arraydel2(&conf);


  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->fonts))) {
    if (gra2cairo_get_fontmap_num() == 0) {
      add_str_to_array(&conf, "font_map");
    }
  }
  removeconfig("[gra2cairo]", &conf);
  arraydel2(&conf);

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

static void
SetScriptDialogBrowse(GtkWidget *w, gpointer client_data)
{
  char *file;
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) client_data;
  if (nGetOpenFileName(TopLevel, _("Add-in Script"), "nsc", NULL,
		       NULL, &file, "*.nsc", TRUE, FALSE) == IDOK) {
    gtk_entry_set_text(GTK_ENTRY(d->script), file);
  }
  free(file);
}

static void
SetScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) data;
  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Name:"), TRUE);
    d->name = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Script file:"), TRUE);
    d->script = w;

    w = gtk_button_new_with_mnemonic("_Browse");
    g_signal_connect(w, "clicked", G_CALLBACK(SetScriptDialogBrowse), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Option:"), TRUE);
    d->option = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Description:"), TRUE);
    d->description = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
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
    MessageBox(TopLevel, msg, NULL, MB_OK);
    return 1;
  }

  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(*opt);
    *opt = buf2;
  }

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

  if (set_scrpt_option(d->script, &(d->Script->script), _("Please specify script file name."))) {
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
  memfree(fdel->name);
  memfree(fdel->script);
  memfree(fdel->description);
  memfree(fdel->option);
  memfree(fdel);
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
    PrefScriptDialogCreateWidgets(d, NULL, sizeof(list) / sizeof(*list), list);
    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefScriptDialogSetupItem(d);
}

static void
PrefScriptDialogClose(GtkWidget *w, void *data)
{
  update_addin_menu();
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

static void
SetDriverDialogBrowse(GtkWidget *w, gpointer client_data)
{
  char *file;
  struct SetDriverDialog *d;

  d = (struct SetDriverDialog *) client_data;
  if (nGetOpenFileName(TopLevel, _("External Driver"), NULL, NULL,
		       NULL, &file, "*", TRUE, FALSE) == IDOK) {
    gtk_entry_set_text(GTK_ENTRY(d->driver), file);
  }
  free(file);
}

static void
SetDriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct SetDriverDialog *d;

  d = (struct SetDriverDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Name:"), TRUE);
    d->name = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Driver:"), TRUE);
    d->driver = w;

    w = gtk_button_new_with_mnemonic("_Browse");
    g_signal_connect(w, "clicked", G_CALLBACK(SetDriverDialogBrowse), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Option:"), TRUE);
    d->option = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Extension:"), TRUE);
    d->ext = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
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
    MessageBox(TopLevel, _("Please specify driver name."), NULL, MB_OK);
    return;
  }

  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Driver->name);
    d->Driver->name = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->driver));
  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Driver->driver);
    d->Driver->driver = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->ext));
  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Driver->ext);
    d->Driver->ext = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->option));
  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Driver->option);
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
  memfree(fdel->name);
  memfree(fdel->driver);
  memfree(fdel->option);
  memfree(fdel->ext);
  memfree(fdel);
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
    PrefDriverDialogCreateWidgets(d, NULL, sizeof(list) / sizeof(*list), list);
    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefDriverDialogSetupItem(d);
}

static void
PrefDriverDialogClose(GtkWidget *w, void *data)
{
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

  dialog = gtk_font_selection_dialog_new(_("Font"));

#ifdef JAPANESE
  vbox = GTK_DIALOG(dialog)->vbox;
  w = gtk_check_button_new_with_mnemonic(_("_Jfont"));
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  gtk_widget_show(w);
  d->two_byte = w;
#endif

  if (fcur) {
    int len, t;
    char *buf;

    len = strlen(fcur->fontname) + 64;
    buf = malloc(len);
    if (buf) {
      t = (fcur->type < 0 || fcur->type >= sizeof(type) / sizeof(*type)) ? 0 : fcur->type;

      snprintf(buf, len, "%s %s 16", fcur->fontname, type[t]);
      gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), buf);
      free(buf);
    }
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
  tmp = strdup(alias);

  if (tmp == NULL)
    return NULL;

  g_strchug(tmp);

  for (ptr = tmp; *ptr != '\0'; ptr++) {
    if (*ptr == '\t')
      *ptr = ' ';
  }

  if (tmp[0] == '\0') {
    free(tmp);
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
      free(alias);
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
  if (fcur == NULL)
    return;

  dialog = create_font_selection_dialog(d, fcur);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_CANCEL) {
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
    MessageBox(TopLevel, _("Please specify a new alias name."), NULL, MB_OK);
    return;
  }

  fmap = gra2cairo_get_fontmap(alias);
  free(alias);

  if (fmap) {
    MessageBox(TopLevel, _("Alias name already exists."), NULL, MB_OK);
    return;
  }

  dialog = create_font_selection_dialog(d, NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_CANCEL) {
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
  GdkColor color;

  if (Menulocal.editor)
    gtk_entry_set_text(GTK_ENTRY(d->editor), Menulocal.editor);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->directory)), Menulocal.changedirectory);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->history)), Menulocal.savehistory);

  combo_box_set_active(d->path, Menulocal.savepath);

  combo_box_set_active(d->fftype, (Menulocal.focus_frame_type == GDK_LINE_SOLID) ? 0 : 1);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->datafile)), Menulocal.savewithdata);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->mergefile)), Menulocal.savewithmerge);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->expand)), Menulocal.expand);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->preserve_width)), Menulocal.preserve_width);

  if (Menulocal.expanddir)
    gtk_entry_set_text(GTK_ENTRY(d->expanddir), Menulocal.expanddir);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->ignorepath)), Menulocal.ignorepath);

  spin_entry_set_val(d->hist_size, Menulocal.hist_size);
  spin_entry_set_val(d->info_size, Menulocal.info_size);
  spin_entry_set_val(d->data_head_lines, Mxlocal->data_head_lines);

  color.red = Menulocal.bg_r * 257;
  color.green = Menulocal.bg_g * 257;
  color.blue = Menulocal.bg_b * 257;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(d->bgcol), &color);

  if (Menulocal.coordwin_font) {
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(d->coordwin_font), Menulocal.coordwin_font);
  }
}

static void
MiscDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *hbox2, *vbox, *vbox2, *frame;
  struct MiscDialog *d;
  int j;

  d = (struct MiscDialog *) data;
  if (makewidget) {
    hbox2 = gtk_hbox_new(FALSE, 4);

    vbox2 = gtk_vbox_new(FALSE, 4);

    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Editor:"), TRUE);
    d->editor = w;
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Check \"change current directory\""));
    d->directory = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_check_button_new_with_mnemonic(_("_Save file history"));
    d->history = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(vbox, w, _("_Path:"), FALSE);
    d->path = w;
    for (j = 0; pathchar[j]; j++) {
      combo_box_append_text(d->path, _(pathchar[j]));
    }

    w = gtk_check_button_new_with_mnemonic(_("include _Data file"));
    d->datafile = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("include _Merge file"));
    d->mergefile = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Expand include file"));
    d->expand = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Expand directory:"), TRUE);
    d->expanddir = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Ignore file path"));
    d->ignorepath = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox2), vbox2, TRUE, TRUE, 4);



    vbox2 = gtk_vbox_new(FALSE, 4);

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_check_button_new_with_mnemonic(_("_Preserve line width and style"));
    d->preserve_width = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->bgcol = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_color_button(wi);
    item_setup(hbox, w, _("_Background Color:"), FALSE);
    d->bgcol = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    combo_box_append_text(w, _("solid"));
    combo_box_append_text(w, _("dot"));
    item_setup(hbox, w, _("_Line attribute of focus frame:"), FALSE);
    d->fftype = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_spin_entry(1, HIST_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(vbox, w, _("_Size of completion history:"), FALSE);
    d->hist_size = w;

    w = create_spin_entry(1, INFOWIN_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(vbox, w, _("_Length of information window:"), FALSE);
    d->info_size = w;

    w = create_spin_entry(0, SPIN_ENTRY_MAX, 1, FALSE, TRUE);
    item_setup(vbox, w, _("_Length of data preview:"), FALSE);
    d->data_head_lines = w;

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_font_button_new();
    item_setup(vbox, w, _("_Font of coordinate window:"), FALSE);
    d->coordwin_font = w;

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox2), vbox2, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox2, FALSE, FALSE, 4);
  }
  MiscDialogSetupItem(wi, d);
}

static void
MiscDialogClose(GtkWidget *w, void *data)
{
  struct MiscDialog *d;
  int a, ret;
  const char *buf;
  char *buf2;
  GdkColor color;

  d = (struct MiscDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->editor));
  if (buf) {
    buf2 = nstrdup(buf);
    if (buf2) {
      changefilename(buf2);
      memfree(Menulocal.editor);
    }
    Menulocal.editor = buf2;
  } else {
    memfree(Menulocal.editor);
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
    buf2 = nstrdup(buf);
    if (buf2) {
      memfree(Menulocal.expanddir);
      Menulocal.expanddir = buf2;
    }
  }

  Menulocal.ignorepath =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->ignorepath));

  Menulocal.preserve_width =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->preserve_width));

  a = combo_box_get_active(d->fftype);
  Menulocal.focus_frame_type = ((a == 0) ? GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH);

  a = spin_entry_get_val(d->hist_size);
  if (a < HIST_SIZE_MAX && a > 0) 
    Menulocal.hist_size = a;

  a = spin_entry_get_val(d->info_size);
  if (a < INFOWIN_SIZE_MAX && a > 0) 
    Menulocal.info_size = a;

  gtk_color_button_get_color(GTK_COLOR_BUTTON(d->bgcol), &color);
  Menulocal.bg_r = color.red / 256;
  Menulocal.bg_g = color.green / 256;
  Menulocal.bg_b = color.blue / 256;

  a = spin_entry_get_val(d->data_head_lines);
  putobj(d->Obj, "data_head_lines", d->Id, &a);


  buf = gtk_font_button_get_font_name(GTK_FONT_BUTTON(d->coordwin_font));
  if (Menulocal.coordwin_font) {
    if (strcmp(Menulocal.coordwin_font, buf)) {
      memfree(Menulocal.coordwin_font);
    } else {
      buf = NULL;
    }
  }

  if (buf) {
    Menulocal.coordwin_font = nstrdup(buf);
    CoordWinSetFont(buf);
  }

  d->ret = ret;
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
  GtkWidget *w, *hbox, *vbox;
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("use _External previewer"));
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "toggled", G_CALLBACK(use_external_toggled), d);
    d->use_external = w;

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(20, 620, 1);
    d->dpi = w;
    item_setup(hbox, w, "_DPI:", TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(hbox, w, _("Window _Width:"), TRUE);
    d->width = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry(WIN_SIZE_MIN, WIN_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(hbox, w, _("Window _Height:"), TRUE);
    d->height = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }
  ExViewerDialogSetupItem(wi, d);
}

static void
ExViewerDialogClose(GtkWidget *w, void *data)
{
  struct ExViewerDialog *d;
  int ret;

  d = (struct ExViewerDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  Menulocal.exwindpi = gtk_range_get_value(GTK_RANGE(d->dpi));
  Menulocal.exwinwidth = spin_entry_get_val(d->width);
  Menulocal.exwinheight = spin_entry_get_val(d->height);
  Menulocal.exwin_use_external = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_external));

  d->ret = ret;
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

  getobj(d->Obj, "dpi", d->Id, 0, NULL, &(d->dpis));
  gtk_range_set_value(GTK_RANGE(d->dpi), d->dpis);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->ruler)), Mxlocal->ruler);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->statusbar)), Menulocal.statusb);

  getobj(d->Obj, "antialias", d->Id, 0, NULL, &a);
  combo_box_set_active(d->antialias, a);

  getobj(d->Obj, "auto_redraw", d->Id, 0, NULL, &a);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->redraw)), a);

  getobj(d->Obj, "redraw_flag", d->Id, 0, NULL, &a);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->loadfile)), a);

  spin_entry_set_val(d->data_num, Mxlocal->redrawf_num);
  spin_entry_set_val(d->grid, Mxlocal->grid);
}

static void
ViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct ViewerDialog *d;
  int i;

  d = (struct ViewerDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(20, 620, 1);
    d->dpi = w;
    item_setup(hbox, w, _("_DPI:"), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, 1, GRID_MAX);
    item_setup(hbox, w, _("_Grid:"), TRUE);
    d->grid = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    for (i = 0; gra2cairo_antialias_type[i]; i++) {
      combo_box_append_text(w, _(gra2cairo_antialias_type[i]));
    }
    d->antialias = w;
    item_setup(hbox, w, _("_Antialias:"), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("show _Ruler"));
    d->ruler = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Show status bar"));
    d->statusbar = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Auto redraw"));
    d->redraw = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Load files on redraw"));
    d->loadfile = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
    item_setup(vbox, w, _("_Maximum number of data on redraw:"), FALSE);
    d->data_num = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }
  ViewerDialogSetupItem(wi, d);
}

static void
ViewerDialogClose(GtkWidget *w, void *data)
{
  struct ViewerDialog *d;
  int ret, dpi, a;

  d = (struct ViewerDialog *) data;
 
 if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  dpi = gtk_range_get_value(GTK_RANGE(d->dpi));
  if (d->dpis != dpi) {
    if (putobj(d->Obj, "dpi", d->Id, &dpi) == -1)
      return;
    d->Clear = TRUE;
  }

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->ruler));
  Mxlocal->ruler = a;

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->statusbar));
  Menulocal.statusb = a;

  a = combo_box_get_active(d->antialias);
  if (putobj(d->Obj, "antialias", d->Id, &a) == -1)
    return;
  Mxlocal->antialias = a;

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->redraw));
  a = a ? TRUE : FALSE;
  if (putobj(d->Obj, "auto_redraw", d->Id, &a) == -1)
    return;

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->loadfile));
  a = a ? TRUE : FALSE;
  if (putobj(d->Obj, "redraw_flag", d->Id, &a) == -1)
    return;

  a = spin_entry_get_val(d->data_num);
  if (putobj(d->Obj, "redraw_num", d->Id, &a) == -1)
    return;

  Mxlocal->grid = spin_entry_get_val(d->grid);

  set_widget_visibility();

  d->ret = ret;
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
    if (MessageBox(TopLevel, mes, _("Save as .Ngraph.ngp"), MB_YESNO) != IDYES) {
      memfree(ngpfile);
      return;
    }
  }

  snprintf(mes, sizeof(mes), _("Saving `%.128s'."), ngpfile);
  SetStatusBar(mes);
  SaveDrawrable(ngpfile, FALSE, FALSE);
  ResetStatusBar();
  memfree(ngpfile);
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
