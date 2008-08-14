/* 
 * $Id: x11opt.c,v 1.15 2008/08/14 01:41:37 hito Exp $
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
  struct extprinter *pcur;
  struct script *scur;
  char *driver, *ext, *option, *script;
  GdkWindowState state;
  gint x, y, x0, y0, w, h;

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

    /*
    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "move_child_window=%d", Menulocal.movechild);
      arrayadd(&conf, &buf);
    }
    */
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->child_geometry))) {
    get_window_geometry(TopLevel, &x0, &y0, &w, &h, &state);

    if (NgraphApp.FileWin.Win && NgraphApp.FileWin.Win->window) {
      get_window_geometry(NgraphApp.FileWin.Win, &x, &y, &w, &h, &state);
      Menulocal.filewidth = w;
      Menulocal.fileheight = h;
      Menulocal.filex = x - x0;
      Menulocal.filey = y - y0;
      Menulocal.fileopen = ! state;
    } else {
      Menulocal.fileopen = 0;
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "file_win=%d,%d,%d,%d,%d",
	      Menulocal.filex, Menulocal.filey,
	      Menulocal.filewidth, Menulocal.fileheight, Menulocal.fileopen);
      arrayadd(&conf, &buf);
    }

    if (NgraphApp.AxisWin.Win && NgraphApp.AxisWin.Win->window) {
      get_window_geometry(NgraphApp.AxisWin.Win, &x, &y, &w, &h, &state);
      Menulocal.axiswidth = w;
      Menulocal.axisheight = h;
      Menulocal.axisx = x - x0;
      Menulocal.axisy = y - y0;
      Menulocal.axisopen = ! state;
    } else {
      Menulocal.axisopen = 0;
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "axis_win=%d,%d,%d,%d,%d",
	      Menulocal.axisx, Menulocal.axisy,
	      Menulocal.axiswidth, Menulocal.axisheight, Menulocal.axisopen);
      arrayadd(&conf, &buf);
    }

    if (NgraphApp.LegendWin.Win && NgraphApp.LegendWin.Win->window) {
      get_window_geometry(NgraphApp.LegendWin.Win, &x, &y, &w, &h, &state);
      Menulocal.legendwidth = w;
      Menulocal.legendheight = h;
      Menulocal.legendx = x - x0;
      Menulocal.legendy = y - y0;
      Menulocal.legendopen = ! state;
    } else {
      Menulocal.legendopen = 0;
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "legend_win=%d,%d,%d,%d,%d",
	      Menulocal.legendx, Menulocal.legendy,
	      Menulocal.legendwidth, Menulocal.legendheight,
	      Menulocal.legendopen);
      arrayadd(&conf, &buf);
    }

    if (NgraphApp.MergeWin.Win && NgraphApp.MergeWin.Win->window) {
      get_window_geometry(NgraphApp.MergeWin.Win, &x, &y, &w, &h, &state);
      Menulocal.mergewidth = w;
      Menulocal.mergeheight = h;
      Menulocal.mergex = x - x0;
      Menulocal.mergey = y - y0;
      Menulocal.mergeopen = 1;
    } else {
      Menulocal.mergeopen = 0;
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "merge_win=%d,%d,%d,%d,%d",
	      Menulocal.mergex, Menulocal.mergey,
	      Menulocal.mergewidth, Menulocal.mergeheight,
	      Menulocal.mergeopen);
      arrayadd(&conf, &buf);
    }

    if (NgraphApp.InfoWin.Win && NgraphApp.InfoWin.Win->window) {
      get_window_geometry(NgraphApp.InfoWin.Win, &x, &y, &w, &h, &state);
      Menulocal.dialogwidth = w;
      Menulocal.dialogheight = h;
      Menulocal.dialogx = x - x0;
      Menulocal.dialogy = y - y0;
      Menulocal.dialogopen = ! state;
    } else {
      Menulocal.dialogopen = 0;
    }

    buf = (char *) memalloc(BUF_SIZE);    
    if (buf) {
      snprintf(buf, BUF_SIZE, "information_win=%d,%d,%d,%d,%d",
	      Menulocal.dialogx, Menulocal.dialogy,
	      Menulocal.dialogwidth, Menulocal.dialogheight,
	      Menulocal.dialogopen);
      arrayadd(&conf, &buf);
    }

    if (NgraphApp.CoordWin.Win && NgraphApp.CoordWin.Win->window) {
      get_window_geometry(NgraphApp.CoordWin.Win, &x, &y, &w, &h, &state);
      Menulocal.coordwidth = w;
      Menulocal.coordheight = h;
      Menulocal.coordx = x - x0;
      Menulocal.coordy = y - y0;
      Menulocal.coordopen = ! state;
    } else {
      Menulocal.coordopen = 0;
    }

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
      snprintf(buf, BUF_SIZE, "background_color=%02x%02x%02x",
	       Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
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
  }
  replaceconfig("[gra2gtk]", &conf);
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
MiscDialogSetupItem(GtkWidget *w, struct MiscDialog *d)
{
  GdkColor color;

  if (Menulocal.editor != NULL)
    gtk_entry_set_text(GTK_ENTRY(d->editor), Menulocal.editor);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->directory)), Menulocal.changedirectory);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->history)), Menulocal.savehistory);

  combo_box_set_active(d->path, Menulocal.savepath);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->datafile)), Menulocal.savewithdata);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->mergefile)), Menulocal.savewithmerge);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->expand)), Menulocal.expand);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->preserve_width)), Menulocal.preserve_width);

  if (Menulocal.expanddir != NULL)
    gtk_entry_set_text(GTK_ENTRY(d->expanddir), Menulocal.expanddir);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GTK_TOGGLE_BUTTON(d->ignorepath)), Menulocal.ignorepath);

  spin_entry_set_val(d->hist_size, Menulocal.hist_size);
  spin_entry_set_val(d->info_size, Menulocal.info_size);

  color.red = Menulocal.bg_r * 257;
  color.green = Menulocal.bg_g * 257;
  color.blue = Menulocal.bg_b * 257;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(d->bgcol), &color);
}

static void
MiscDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *frame;
  struct MiscDialog *d;
  int j;

  d = (struct MiscDialog *) data;
  if (makewidget) {
    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Editor:"), TRUE);
    d->editor = w;
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


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
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);

    vbox = gtk_vbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(vbox, w, _("_Path:"), FALSE);
    d->path = w;

    w = gtk_check_button_new_with_mnemonic(_("include _Data file"));
    d->datafile = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("include _Merge file"));
    d->mergefile = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


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
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

    for (j = 0; pathchar[j] != NULL; j++) {
      combo_box_append_text(d->path, _(pathchar[j]));
    }

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

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_spin_entry(1, HIST_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(vbox, w, _("_Size of completion history:"), FALSE);
    d->hist_size = w;

    w = create_spin_entry(1, INFOWIN_SIZE_MAX, 1, FALSE, TRUE);
    item_setup(vbox, w, _("_Length of information window:"), FALSE);
    d->info_size = w;

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


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

  d->ret = ret;
}

void
MiscDialog(struct MiscDialog *data)
{
  data->SetupWindow = MiscDialogSetup;
  data->CloseWindow = MiscDialogClose;
}

static void
ExViewerDialogSetupItem(GtkWidget *w, struct ExViewerDialog *d)
{
  gtk_range_set_value(GTK_RANGE(d->dpi), Menulocal.exwindpi);
#if 1

  spin_entry_set_val(d->width, Menulocal.exwinwidth);
  spin_entry_set_val(d->height, Menulocal.exwinheight);
#endif
}

static void
ExViewerDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct ExViewerDialog *d;

  d = (struct ExViewerDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(20, 620, 1);
    d->dpi = w;
    item_setup(hbox, w, "_DPI:", TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#if 1
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
#endif
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

#if 1
  Menulocal.exwinwidth = spin_entry_get_val(d->width);
  Menulocal.exwinheight = spin_entry_get_val(d->height);
#endif
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
  Mxlocal->autoredraw = a;

  a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->loadfile));
  a = a ? TRUE : FALSE;
  if (putobj(d->Obj, "redraw_flag", d->Id, &a) == -1)
    return;
  Mxlocal->redrawf = a;

  Mxlocal->redrawf_num = spin_entry_get_val(d->data_num);
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
    /*
    if (DlgViewer.Clear)
      ChangeDPI(TRUE);
    */
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
  MiscDialog(&DlgMisc);
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
