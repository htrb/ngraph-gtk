/* 
 * $Id: x11opt.c,v 1.30 2009/02/03 12:04:13 hito Exp $
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
DefaultDialogClose(GtkWidget *win, void *data)
{
  struct DefaultDialog *d;
  int ret, len;
  struct narray conf;
  char *buf;
  char *driver, *ext, *option, *script;
  GdkWindowState state;
  gint x, y, w, h;

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
      arrayadd(&conf, &buf);
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "status_bar=%d", Menulocal.statusb);
      arrayadd(&conf, &buf);
    }
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->child_geometry))) {
    sub_window_save_geometry(&(NgraphApp.FileWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "file_win=%d,%d,%d,%d,%d",
	      Menulocal.filex, Menulocal.filey,
	      Menulocal.filewidth, Menulocal.fileheight, Menulocal.fileopen);
      arrayadd(&conf, &buf);
    }

    sub_window_save_geometry(&(NgraphApp.AxisWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "axis_win=%d,%d,%d,%d,%d",
	      Menulocal.axisx, Menulocal.axisy,
	      Menulocal.axiswidth, Menulocal.axisheight, Menulocal.axisopen);
      arrayadd(&conf, &buf);
    }

    sub_window_save_geometry((struct SubWin *) &(NgraphApp.LegendWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "legend_win=%d,%d,%d,%d,%d",
	      Menulocal.legendx, Menulocal.legendy,
	      Menulocal.legendwidth, Menulocal.legendheight,
	      Menulocal.legendopen);
      arrayadd(&conf, &buf);
    }

    sub_window_save_geometry(&(NgraphApp.MergeWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "merge_win=%d,%d,%d,%d,%d",
	      Menulocal.mergex, Menulocal.mergey,
	      Menulocal.mergewidth, Menulocal.mergeheight,
	      Menulocal.mergeopen);
      arrayadd(&conf, &buf);
    }

    sub_window_save_geometry((struct SubWin *) &(NgraphApp.InfoWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "information_win=%d,%d,%d,%d,%d",
	      Menulocal.dialogx, Menulocal.dialogy,
	      Menulocal.dialogwidth, Menulocal.dialogheight,
	      Menulocal.dialogopen);
      arrayadd(&conf, &buf);
    }

    sub_window_save_geometry((struct SubWin *) &(NgraphApp.CoordWin));
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "coordinate_win=%d,%d,%d,%d,%d",
	      Menulocal.coordx, Menulocal.coordy,
	      Menulocal.coordwidth, Menulocal.coordheight,
	      Menulocal.coordopen);
      arrayadd(&conf, &buf);
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->viewer))) {
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_dpi=%d", Mxlocal->windpi);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "antialias=%d", Mxlocal->antialias);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_auto_redraw=%d", Mxlocal->autoredraw);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_load_file_on_redraw=%d", Mxlocal->redrawf);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_load_file_data_number=%d", Mxlocal->redrawf_num);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_show_ruler=%d", Mxlocal->ruler);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "viewer_grid=%d", Mxlocal->grid);
      arrayadd(&conf, &buf);
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_driver))) {
    struct extprinter *pcur;

    pcur = Menulocal.extprinterroot;
    while (pcur != NULL) {
      if (pcur->driver == NULL)
	driver = "";
      else
	driver = pcur->driver;
      if (pcur->ext == NULL)
	ext = "";
      else
	ext = pcur->ext;
      if (pcur->option == NULL)
	option = "";
      else
	option = pcur->option;
      len = strlen(pcur->name) + strlen(driver) + strlen(ext) + strlen(option) + 20;
      if ((buf = (char *) memalloc(len)) != NULL) {
	snprintf(buf, len, "ext_driver=%s,%s,%s,%s", pcur->name, driver, ext, option);
	arrayadd(&conf, &buf);
      }
      pcur = pcur->next;
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->addin_script))) {
    struct script *scur;

    scur = Menulocal.scriptroot;
    while (scur != NULL) {
      if (scur->script == NULL)
	script = "";
      else
	script = scur->script;
      if (scur->option == NULL)
	option = "";
      else
	option = scur->option;
      len = strlen(scur->name) + strlen(script) + strlen(option) + 20;
      if ((buf = (char *) memalloc(len)) != NULL) {
	snprintf(buf, len, "script=%s,%s,%s", scur->name, script, option);
	arrayadd(&conf, &buf);
      }
      scur = scur->next;
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->misc))) {
    if (Menulocal.editor != NULL) {
      len = strlen(Menulocal.editor) + 10;
      if ((buf = (char *) memalloc(len)) != NULL) {
	snprintf(buf, len, "editor=%s", Menulocal.editor);
	arrayadd(&conf, &buf);
      }
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "change_directory=%d", Menulocal.changedirectory);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "save_history=%d", Menulocal.savehistory);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "save_path=%d", Menulocal.savepath);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "save_with_data=%d", Menulocal.savewithdata);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "save_with_merge=%d", Menulocal.savewithmerge);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(strlen(Menulocal.expanddir) + 20)) != NULL) {
      snprintf(buf, BUF_SIZE, "expand_dir=%s", Menulocal.expanddir);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "expand=%d", Menulocal.expand);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "ignore_path=%d", Menulocal.ignorepath);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "history_size=%d", Menulocal.hist_size);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "preserve_width=%d", Menulocal.preserve_width);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "infowin_size=%d", Menulocal.info_size);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "data_head_lines=%d", Mxlocal->data_head_lines);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "background_color=%02x%02x%02x",
	       Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "focus_frame_type=%d", Menulocal.focus_frame_type);
      arrayadd(&conf, &buf);
    }
  }
  replaceconfig("[x11menu]", &conf);
  arraydel2(&conf);

  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_viewer))) {
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "win_dpi=%d", Menulocal.exwindpi);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "win_width=%d", Menulocal.exwinwidth);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "win_height=%d", Menulocal.exwinheight);
      arrayadd(&conf, &buf);
    }
    if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
      snprintf(buf, BUF_SIZE, "use_external_viewer=%d", Menulocal.exwin_use_external);
      arrayadd(&conf, &buf);
    }
  }
  replaceconfig("[gra2gtk]", &conf);
  arraydel2(&conf);

  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->fonts))) {
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
	arrayadd(&conf, &buf);
      }
      fcur = fcur->next;
    }
  }
  replaceconfig("[gra2cairo]", &conf);
  arraydel2(&conf);

  arrayinit(&conf, sizeof(char *));
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->external_driver))) {
    if (Menulocal.extprinterroot == NULL) {
      if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
	snprintf(buf, BUF_SIZE, "ext_driver");
	arrayadd(&conf, &buf);
      }
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->addin_script))) {
    if (Menulocal.scriptroot == NULL) {
      if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
	snprintf(buf, BUF_SIZE, "script");
	arrayadd(&conf, &buf);
      }
    }
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->misc))) {
    if (Menulocal.editor == NULL) {
      if ((buf = (char *) memalloc(BUF_SIZE)) != NULL) {
	snprintf(buf, BUF_SIZE, "editor");
	arrayadd(&conf, &buf);
      }
    }
  }
  removeconfig("[x11menu]", &conf);
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
  gtk_entry_set_text(GTK_ENTRY(d->name),
		     (d->Script->name) ? d->Script->name : "");
  gtk_entry_set_text(GTK_ENTRY(d->script),
		     (d->Script->script) ? d->Script->script : "");
  gtk_entry_set_text(GTK_ENTRY(d->option),
		     (d->Script->option) ? d->Script->option : "");
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
    item_setup(hbox, w, _("_Script:"), TRUE);
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
  }
  SetScriptDialogSetupItem(wi, d);
}

static void
SetScriptDialogClose(GtkWidget *w, void *data)
{
  const char *buf;
  char *buf2;
  int ret;
  struct SetScriptDialog *d;

  d = (struct SetScriptDialog *) data;

  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->name));
  if (strlen(buf) == 0) {
    MessageBox(TopLevel, _("Please specify script name."), NULL, MB_OK);
    return;
  }

  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Script->name);
    d->Script->name = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->script));
  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Script->script);
    d->Script->script = buf2;
  }

  buf = gtk_entry_get_text(GTK_ENTRY(d->option));
  buf2 = nstrdup(buf);
  if (buf2) {
    memfree(d->Script->option);
    d->Script->option = buf2;
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
PrefScriptDialogSetupItem(GtkWidget *w, struct PrefScriptDialog *d)
{
  struct script *fcur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  fcur = Menulocal.scriptroot;
  while (fcur != NULL) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->name);
    fcur = fcur->next;
  }
}

static void
PrefScriptDialogUpdate(GtkWidget *w, gpointer client_data)
{
  struct PrefScriptDialog *d;
  int a, j;
  struct script *fcur;

  d = (struct PrefScriptDialog *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fcur = Menulocal.scriptroot;
  while (fcur != NULL) {
    if (j == a)
      break;
    fcur = fcur->next;
    j++;
  }
  if (fcur != NULL) {
    SetScriptDialog(&DlgSetScript, fcur);
    DialogExecute(d->widget, &DlgSetScript);
    PrefScriptDialogSetupItem(d->widget, d);
  }
}

static void
PrefScriptDialogRemove(GtkWidget *w, gpointer client_data)
{
  int a, j;
  struct script *fcur, *fprev, *fdel;
  struct PrefScriptDialog *d;

  d = (struct PrefScriptDialog *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fprev = NULL;
  fcur = Menulocal.scriptroot;
  while (fcur != NULL) {
    if (j == a) {
      fdel = fcur;
      if (fprev == NULL)
	Menulocal.scriptroot = fcur->next;
      else
	fprev->next = fcur->next;
      fcur = fcur->next;
      memfree(fdel->name);
      memfree(fdel->script);
      memfree(fdel->option);
      memfree(fdel);
      PrefScriptDialogSetupItem(d->widget, d);
      break;
    } else {
      fprev = fcur;
      fcur = fcur->next;
    }
    j++;
  }
}

static void
PrefScriptDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct PrefScriptDialog *d;
  struct script *fcur, *fprev, *fnew;

  d = (struct PrefScriptDialog *) client_data;
  fprev = NULL;
  fcur = Menulocal.scriptroot;
  while (fcur != NULL) {
    fprev = fcur;
    fcur = fcur->next;
  }
  if ((fnew = (struct script *) memalloc(sizeof(struct script))) != NULL) {
    fnew->next = NULL;
    fnew->name = NULL;
    fnew->script = NULL;
    fnew->option = NULL;
    if (fprev == NULL)
      Menulocal.scriptroot = fnew;
    else
      fprev->next = fnew;
    SetScriptDialog(&DlgSetScript, fnew);
    if (DialogExecute(d->widget, &DlgSetScript) != IDOK) {
      if (fprev == NULL)
	Menulocal.scriptroot = NULL;
      else
	fprev->next = NULL;
      memfree(fnew);
    }
    PrefScriptDialogSetupItem(d->widget, d);
  } else {
    memfree(fnew);
  }
}

static gboolean
script_list_defailt_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct PrefScriptDialog *d;
  int i;

  d = (struct PrefScriptDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    i = list_store_get_selected_index(d->list);
    if (i < 0)
      return FALSE;

    PrefScriptDialogUpdate(NULL, d);

    return TRUE;
  }

  return FALSE;
}

static void
PrefScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  struct PrefScriptDialog *d;
  n_list_store list[] = {
    {N_("Script"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefScriptDialog *) data;
  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    d->list = w;
    g_signal_connect(d->list, "button-press-event", G_CALLBACK(script_list_defailt_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(script_list_defailt_cb), d);
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefScriptDialogAdd), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefScriptDialogUpdate), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefScriptDialogRemove), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    d->show_cancel = FALSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefScriptDialogSetupItem(wi, d);
}

static void
PrefScriptDialogClose(GtkWidget *w, void *data)
{
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
  gtk_entry_set_text(GTK_ENTRY(d->name),
		     (d->Driver->name) ? d->Driver->name : "");

  gtk_entry_set_text(GTK_ENTRY(d->driver),
		     (d->Driver->driver) ? d->Driver->driver : "");

  gtk_entry_set_text(GTK_ENTRY(d->option),
		     (d->Driver->option) ? d->Driver->option : "");

  gtk_entry_set_text(GTK_ENTRY(d->ext),
		     (d->Driver->ext) ? d->Driver->ext : "");
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
PrefDriverDialogSetupItem(GtkWidget *w, struct PrefDriverDialog *d)
{
  struct extprinter *fcur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  fcur = Menulocal.extprinterroot;
  while (fcur != NULL) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->name);
    fcur = fcur->next;
  }
}

static void
PrefDriverDialogUpdate(GtkWidget *w, gpointer client_data)
{
  struct PrefDriverDialog *d;
  int a, j;
  struct extprinter *fcur;

  d = (struct PrefDriverDialog *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fcur = Menulocal.extprinterroot;
  while (fcur != NULL) {
    if (j == a)
      break;
    fcur = fcur->next;
    j++;
  }
  if (fcur != NULL) {
    SetDriverDialog(&DlgSetDriver, fcur);
    DialogExecute(d->widget, &DlgSetDriver);
    PrefDriverDialogSetupItem(d->widget, d);
  }
}

static void
PrefDriverDialogRemove(GtkWidget *w, gpointer client_data)
{
  int a, j;
  struct extprinter *fcur, *fprev, *fdel;
  struct PrefDriverDialog *d;

  d = (struct PrefDriverDialog *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fprev = NULL;
  fcur = Menulocal.extprinterroot;
  while (fcur != NULL) {
    if (j == a) {
      fdel = fcur;
      if (fprev == NULL)
	Menulocal.extprinterroot = fcur->next;
      else
	fprev->next = fcur->next;
      fcur = fcur->next;
      memfree(fdel->name);
      memfree(fdel->driver);
      memfree(fdel->option);
      memfree(fdel);
      PrefDriverDialogSetupItem(d->widget, d);
      break;
    } else {
      fprev = fcur;
      fcur = fcur->next;
    }
    j++;
  }
}

static void
PrefDriverDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct extprinter *fcur, *fprev, *fnew;
  struct PrefDriverDialog *d;

  d = (struct PrefDriverDialog *) client_data;

  fprev = NULL;
  fcur = Menulocal.extprinterroot;

  while (fcur != NULL) {
    fprev = fcur;
    fcur = fcur->next;
  }
  fnew = (struct extprinter *) memalloc(sizeof(struct extprinter));
  if (fnew) {
    fnew->next = NULL;
    fnew->name = NULL;
    fnew->driver = NULL;
    fnew->option = NULL;
    fnew->ext = NULL;
    if (fprev == NULL)
      Menulocal.extprinterroot = fnew;
    else
      fprev->next = fnew;
    SetDriverDialog(&DlgSetDriver, fnew);
    if (DialogExecute(d->widget, &DlgSetDriver) != IDOK) {
      if (fprev == NULL)
	Menulocal.extprinterroot = NULL;
      else
	fprev->next = NULL;
      memfree(fnew);
    }
    PrefDriverDialogSetupItem(d->widget, d);
  } else {
    memfree(fnew);
  }
}

static gboolean
driver_list_defailt_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct PrefDriverDialog *d;
  int i;

  d = (struct PrefDriverDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    i = list_store_get_selected_index(d->list);
    if (i < 0)
      return FALSE;

    PrefDriverDialogUpdate(NULL, d);

    return TRUE;
  }

  return FALSE;
}

static void
PrefDriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  struct PrefDriverDialog *d;
  n_list_store list[] = {
    {N_("Driver"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct PrefDriverDialog *) data;
  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    d->list = w;
    g_signal_connect(d->list, "button-press-event", G_CALLBACK(driver_list_defailt_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(driver_list_defailt_cb), d);
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefDriverDialogAdd), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefDriverDialogUpdate), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefDriverDialogRemove), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    d->show_cancel = FALSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), 400, 300);
  }
  PrefDriverDialogSetupItem(wi, d);
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

  list_store_clear(d->list);
  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur != NULL) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, fcur->fontalias);
    list_store_set_string(d->list, &iter, 1, fcur->fontname);
    list_store_set_string(d->list, &iter, 2, gra2cairo_get_font_type_str(fcur->type));
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
    "Italic",
    "Bold Italic",
  };

  dialog = gtk_font_selection_dialog_new("Font alias");

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

      snprintf(buf, len, "%s %s 20", fcur->fontname, type[t]);
      gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dialog), buf);
      free(buf);
    }
#ifdef JAPANESE
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->two_byte), fcur->twobyte);
#endif
  }

  return dialog;
}

static void
font_selection_dialog_set_font(GtkWidget *w, struct PrefFontDialog *d, struct fontmap *fcur)
{
  PangoFontDescription *pdesc;
  char *fname;
  PangoStyle style;
  PangoWeight weight;
  int type, two_byte;

  fname = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(w));
  pdesc = pango_font_description_from_string(fname);
  weight = pango_font_description_get_weight(pdesc);
  style =  pango_font_description_get_style(pdesc);

  switch (style) {
  case PANGO_STYLE_NORMAL:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLD;
    } else {
      type = NORMAL;
    }
    break;
  case PANGO_STYLE_OBLIQUE:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLDITALIC;
     } else {
      type = ITALIC;
    }
    break;
  case PANGO_STYLE_ITALIC:
    if (weight > PANGO_WEIGHT_NORMAL) {
      type = BOLDOBLIQUE;
    } else {
      type = OBLIQUE;
    }
    break;
  default:
    type = NORMAL;
  }

#ifdef JAPANESE
  two_byte = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->two_byte));
#else
  two_byte = FALSE;
#endif

  if (fcur) {
    gra2cairo_update_fontmap(fcur->fontalias,
			     pango_font_description_get_family(pdesc),
			     type,
			     two_byte);
  } else {
    const char *alias;

    alias = gtk_entry_get_text(GTK_ENTRY(d->alias));
    gra2cairo_add_fontmap(alias,
			     pango_font_description_get_family(pdesc),
			     type,
			     two_byte);
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
    font_selection_dialog_set_font(dialog, d, fcur);
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
  const char *alias;
  GtkWidget *dialog;

  d = (struct PrefFontDialog *) client_data;

  alias = gtk_entry_get_text(GTK_ENTRY(d->alias));
  if (alias == NULL || alias[0] == '\0')
    return;

  dialog = create_font_selection_dialog(d, NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_CANCEL) {
    font_selection_dialog_set_font(dialog, d, NULL);
    PrefFontDialogSetupItem(d);
  }
  gtk_widget_destroy (dialog);
}

static gboolean
font_list_defailt_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct PrefFontDialog *d;
  int i;

  d = (struct PrefFontDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    i = list_store_get_selected_index(d->list);
    if (i < 0)
      return FALSE;

    PrefFontDialogUpdate(NULL, d);

    return TRUE;
  }

  return FALSE;
}

static void
PrefFontDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *swin;
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
    item_setup(vbox, w, _("_Alias"), FALSE);
    d->alias = w;

    hbox = gtk_hbox_new(FALSE, 4);

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    d->list = w;
    g_signal_connect(d->list, "button-press-event", G_CALLBACK(font_list_defailt_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(font_list_defailt_cb), d);
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefFontDialogAdd), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefFontDialogUpdate), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    g_signal_connect(w, "clicked", G_CALLBACK(PrefFontDialogRemove), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    d->show_cancel = FALSE;

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

  if (Menulocal.editor != NULL)
    gtk_entry_set_text(GTK_ENTRY(d->editor), Menulocal.editor);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->directory)), Menulocal.changedirectory);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->history)), Menulocal.savehistory);

  combo_box_set_active(d->path, Menulocal.savepath);

  combo_box_set_active(d->fftype, (Menulocal.focus_frame_type == GDK_LINE_SOLID) ? 0 : 1);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->datafile)), Menulocal.savewithdata);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->mergefile)), Menulocal.savewithmerge);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->expand)), Menulocal.expand);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->preserve_width)), Menulocal.preserve_width);

  if (Menulocal.expanddir != NULL)
    gtk_entry_set_text(GTK_ENTRY(d->expanddir), Menulocal.expanddir);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->ignorepath)), Menulocal.ignorepath);

  spin_entry_set_val(d->hist_size, Menulocal.hist_size);
  spin_entry_set_val(d->info_size, Menulocal.info_size);
  spin_entry_set_val(d->data_head_lines, Mxlocal->data_head_lines);

  color.red = Menulocal.bg_r * 257;
  color.green = Menulocal.bg_g * 257;
  color.blue = Menulocal.bg_b * 257;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(d->bgcol), &color);
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
    for (j = 0; pathchar[j] != NULL; j++) {
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
    item_setup(vbox, w, _("_Length data preview:"), FALSE);
    d->data_head_lines = w;

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

    w = gtk_check_button_new_with_mnemonic(_("_Show ruler"));
    d->ruler = w;
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
  a = a ? TRUE : FALSE;
  if (Mxlocal->ruler != a)
    d->Clear = TRUE;
  Mxlocal->ruler = a;

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

  if ((ngpfile = getscriptname("Ngraph.ngp")) == NULL)
    return;
  path = 1;
  if ((obj = chkobject("file")) != NULL) {
    for (i = 0; i <= chkobjlastinst(obj); i++)
      putobj(obj, "save_path", i, &path);
  }
  if ((obj = chkobject("merge")) != NULL) {
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
