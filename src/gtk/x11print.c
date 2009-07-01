/* 
 * $Id: x11print.c,v 1.42 2009/07/01 09:50:09 hito Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "ioutil.h"
#include "mathcode.h"

#include "gtk_liststore.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "ogra2cairofile.h"
#include "x11menu.h"
#include "x11print.h"
#include "x11file.h"
#include "x11commn.h"
#include "x11view.h"

#define DEVICE_BUF_SIZE 20
#define MESSAGE_BUF_SIZE 1024

static char *PsVersion[] = {
  "PostScript Level 3",
  "PostScript Level 2",
  NULL,
};

static char *SvgVersion[] = {
  "SVG version 1.2",
  "SVG version 1.1",
  NULL,
};

struct print_obj {
  struct objlist *graobj, *g2wobj;
  int id, g2wid;
  char *g2winst;
};

static GtkPrintSettings *PrintSettings = NULL;

static void
DriverDialogSelectCB(GtkWidget *wi, gpointer client_data)
{
  int a, i, j;
  struct extprinter *pcur;
  struct DriverDialog *d;
  char buf[1024];

  d = (struct DriverDialog *) client_data;

  a = combo_box_get_active(d->driver);
  
  if (a < 0)
    return;

  pcur = Menulocal.extprinterroot;
  i = 0;
  while (pcur != NULL) {
    if (i == a) {
      if (pcur->ext && pcur->ext[0] != '\0') {
	if (NgraphApp.FileName) {
	  strncpy(buf, NgraphApp.FileName, sizeof(buf) - 1);
	  if (strchr(buf, '.') != NULL) {
	    for (j = strlen(buf) - 1; buf[j] != '.'; j--);
	    buf[j] = '\0';
	  }
	  strncat(buf, pcur->ext, sizeof(buf) - strlen(buf) - 1);
	  gtk_entry_set_text(GTK_ENTRY(d->file), buf);
	}
	d->ext = pcur->ext;
      } else {
	gtk_entry_set_text(GTK_ENTRY(d->file), "");
	d->ext = NULL;
      }

      gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(pcur->option));
      break;
    }
    pcur = pcur->next;
    i++;
  }
}

static void
DriverDialogBrowseCB(GtkWidget *wi, gpointer client_data)
{
  char *file, *buf;
  int len;
  struct DriverDialog *d;

  d = (struct DriverDialog *) client_data;

  if (d->ext) {
    len = strlen(d->ext) + 2;
    buf = memalloc(len);
    if (buf) {
      snprintf(buf, len, "*%s", d->ext);
    }
  } else {
    buf = NULL;
  }

  if (nGetSaveFileName(d->widget, _("External Driver Output"), NULL, NULL,
		       NULL, &file, buf, TRUE, Menulocal.changedirectory) == IDOK) {
    gtk_entry_set_text(GTK_ENTRY(d->file), file);
  }
  memfree(buf);
  free(file);
}

static void
DriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct DriverDialog *d;
  struct extprinter *pcur;
  int j;

  d = (struct DriverDialog *) data;
  if (makewidget) {

    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    d->driver = w;
    item_setup(hbox, w, _("_Driver:"), FALSE);
    g_signal_connect(d->driver, "changed", G_CALLBACK(DriverDialogSelectCB), d);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    d->option = w;
    item_setup(hbox, w, _("_Option:"), TRUE);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    d->file = w;
    item_setup(hbox, w, _("_File:"), TRUE);

    w = gtk_button_new_with_mnemonic(_("_Browse"));
    g_signal_connect(w, "clicked", G_CALLBACK(DriverDialogBrowseCB), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  combo_box_clear(d->driver);

  pcur = Menulocal.extprinterroot;
  while (pcur != NULL) {
    combo_box_append_text(d->driver, pcur->name);
    pcur = pcur->next;
    j++;
  }
  combo_box_set_active(d->driver, 0);
}

static void
DriverDialogClose(GtkWidget *w, void *data)
{
  int a, i, j, len, len1, len2;
  struct extprinter *pcur;
  struct DriverDialog *d;
  const char *s, *file;
  char *driver, *option;

  d = (struct DriverDialog *) data;

  if (d->ret == IDCANCEL)
    return;

  
  a = combo_box_get_active(d->driver);
  pcur = Menulocal.extprinterroot;
  i = 0;
  while (pcur != NULL) {
    if (i == a && pcur->driver) {
      driver = nstrdup(pcur->driver);
      if (driver) {
	putobj(d->Obj, "driver", d->Id, driver);
      }
      break;
    }
    pcur = pcur->next;
    i++;
  }

  s = gtk_entry_get_text(GTK_ENTRY(d->option));
  file = gtk_entry_get_text(GTK_ENTRY(d->file));

  if (s == NULL) {
    len1 = 0;
  } else {
    len1 = strlen(s);
  }

  if (file == NULL) {
    len2 = 0;
  } else {
    len2 = strlen(file);
  }

  len = len1 + len2 + 7;
  option = memalloc(len);
  if (option == NULL) {
    d->ret = IDCANCEL;
    return;
  }

  j = 0;
  option[j] = '\0';
  if (file) {
    if (check_overwrite(d->widget, file)) {
      memfree(option);
      d->ret = IDCANCEL;
      return;
    }

    snprintf(option, len, "-o '%s' ", file);
    changefilename(option);
  }

  if (len1 != 0)
    strcat(option, s);

  putobj(d->Obj, "option", d->Id, option);
}

void
DriverDialog(struct DriverDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = DriverDialogSetup;
  data->CloseWindow = DriverDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->ext = NULL;
}

static void
OutputDataDialogSetupItem(GtkWidget *w, struct OutputDataDialog *d)
{
  spin_entry_set_val(d->div_entry, d->div);
}

static void
OutputDataDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct OutputDataDialog *d;

  d = (struct OutputDataDialog *) data;
  if (makewidget) {
    w = create_spin_entry(0, 200, 1, FALSE, TRUE);
    d->div_entry = w;
    hbox = gtk_hbox_new(FALSE, 4);
    item_setup(hbox, w, _("_Div:"), TRUE);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }
  OutputDataDialogSetupItem(wi, d);
}

static void
OutputDataDialogClose(GtkWidget *w, void *data)
{
  struct OutputDataDialog *d;

  d = (struct OutputDataDialog *) data;

  if (d->ret != IDOK)
    return;

  d->div = spin_entry_get_val(d->div_entry);
}

void
OutputDataDialog(struct OutputDataDialog *data, int div)
{
  data->SetupWindow = OutputDataDialogSetup;
  data->CloseWindow = OutputDataDialogClose;
  data->div = div;
}

static void
OutputImageDialogSetupItem(GtkWidget *w, struct OutputImageDialog *d)
{
  int i;

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
  gtk_label_set_text(GTK_LABEL(d->vlabel), "");

  combo_box_clear(d->version);
  switch (d->DlgType) {
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, PsVersion[i]);
    }
    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);

    gtk_widget_set_sensitive(d->version, TRUE);
    gtk_widget_set_sensitive(d->vlabel, TRUE);
    gtk_widget_set_sensitive(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(d->vlabel), _("_PostScript Version:"));
    break;
  case MenuIdOutputPNGFile:
    combo_box_append_text(d->version, "--------");

    gtk_widget_set_sensitive(d->dlabel, TRUE);
    gtk_widget_set_sensitive(d->dpi, TRUE);

    gtk_widget_set_sensitive(d->version, FALSE);
    gtk_widget_set_sensitive(d->vlabel, FALSE);
    gtk_widget_set_sensitive(d->t2p, FALSE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), Menulocal.windpi);
    break;
  case MenuIdOutputPDFFile:
    combo_box_append_text(d->version, "--------");

    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);
    gtk_widget_set_sensitive(d->version, FALSE);
    gtk_widget_set_sensitive(d->vlabel, FALSE);

    gtk_widget_set_sensitive(d->t2p, TRUE);
    break;
  case MenuIdOutputSVGFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, SvgVersion[i]);
    }

    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);

    gtk_widget_set_sensitive(d->version, TRUE);
    gtk_widget_set_sensitive(d->vlabel, TRUE);
    gtk_widget_set_sensitive(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(d->vlabel), _("_SVG Version:"));
    break;
  }
  combo_box_set_active(d->version, 0);
}

static void
OutputImageDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w;
  struct OutputImageDialog *d;
  char *title;

  d = (struct OutputImageDialog *) data;
  if (makewidget) {
    w = gtk_check_button_new_with_mnemonic(_("_Convert texts to paths"));
    d->t2p = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_spin_button_new_with_range(1, DPI_MAX, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w), FALSE);
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
    d->dpi = w;
    d->dlabel = item_setup(GTK_WIDGET(d->vbox), w, "DPI:", FALSE);

    w = combo_box_create();
    d->version = w;
    d->vlabel = item_setup(GTK_WIDGET(d->vbox), w, "", FALSE);
  }

  switch (d->DlgType) {
  case MenuIdOutputPSFile:
    title = N_("Cairo PS Output");
    break;
  case MenuIdOutputEPSFile:
    title = N_("Cairo EPS Output");
    break;
  case MenuIdOutputPNGFile:
    title = N_("Cairo PNG Output");
    break;
  case MenuIdOutputPDFFile:
    title = N_("Cairo PDF Output");
    break;
  case MenuIdOutputSVGFile:
    title = N_("Cairo SVG Output");
    break;
  default:
    title = NULL; /* not reachable */
  }

  gtk_window_set_title(GTK_WINDOW(wi), _(title));
  OutputImageDialogSetupItem(w, d);
}

static void
OutputImageDialogClose(GtkWidget *w, void *data)
{
  struct OutputImageDialog *d;
  int a;

  d = (struct OutputImageDialog *) data;

  if (d->ret != IDOK)
    return;

  switch (d->DlgType) {
  case MenuIdOutputPSFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    a = combo_box_get_active(d->version);
    switch (a) {
    case 0:
      d->Version = TYPE_PS3;
      break;
    case 1:
    default:
      d->Version = TYPE_PS2;
      break;
    }
    break;
  case MenuIdOutputEPSFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    a = combo_box_get_active(d->version);
    switch (a) {
    case 0:
      d->Version = TYPE_EPS3;
      break;
    case 1:
    default:
      d->Version = TYPE_EPS2;
      break;
    }
    break;
  case MenuIdOutputPNGFile:
    d->Dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(d->dpi));
    d->Version = TYPE_PNG;
    break;
  case MenuIdOutputPDFFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    d->Version = TYPE_PDF;
    break;
  case MenuIdOutputSVGFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    a = combo_box_get_active(d->version);
    switch (a) {
    case 0:
      d->Version = TYPE_SVG1_2;
      break;
    case 1:
    default:
      d->Version = TYPE_SVG1_2;
      break;
    }
    break;
  }
  d->Dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(d->dpi));

}

void
OutputImageDialog(struct OutputImageDialog *data, int type)
{
  data->SetupWindow = OutputImageDialogSetup;
  data->CloseWindow = OutputImageDialogClose;
  data->DlgType = type;
}

static void
draw_gra(struct objlist *graobj, int id, char *msg, int close)
{
  ProgressDialogCreate(msg);
  SetStatusBar(msg);

  if (exeobj(graobj, "open", id, 0, NULL) == 0) {
    exeobj(graobj, "draw", id, 0, NULL);
    exeobj(graobj, "flush", id, 0, NULL);
    if (close) {
      exeobj(graobj, "close", id, 0, NULL);
    }
  }

  ProgressDialogFinalize();
  ResetStatusBar();
  if (NgraphApp.Viewer.win) {
    gdk_window_invalidate_rect(NgraphApp.Viewer.win, NULL, FALSE);
  }
}

static void
draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, gpointer user_data)
{
  struct objlist *graobj, *g2wobj;
  char *argv[2];
  struct print_obj *pobj;
  int id, g2wid, r;
  char *g2winst;

  pobj = (struct print_obj *) user_data;
  graobj = pobj->graobj;
  g2wobj = pobj->g2wobj;
  id = pobj->id;
  g2wid = pobj->g2wid;
  g2winst = pobj->g2winst;

  argv[0] = (char *) context;
  argv[1] = NULL;
  r = _exeobj(g2wobj, "_context", g2winst, 1, argv);

  if (r == 0) {
    draw_gra(graobj, id, _("Printing."), TRUE);
  }
}

static void
init_graobj(struct objlist *graobj, int id, char *dev_name, int dev_oid)
{
  struct narray *drawrable;
  unsigned int i, n;
  char *device;

  putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
  putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
  putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
  putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
  putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));
  if (arraynum(&(Menulocal.drawrable)) > 0) {
    drawrable = arraynew(sizeof(char *));
    n = arraynum(&(Menulocal.drawrable));
    for (i = 0; i < n; i++) {
      arrayadd2(drawrable,
		(char **) arraynget(&(Menulocal.drawrable), i));
    }
  } else {
    drawrable = NULL;
  }
  putobj(graobj, "draw_obj", id, drawrable);

  device = (char *) memalloc(DEVICE_BUF_SIZE);
  snprintf(device, DEVICE_BUF_SIZE, "%s:^%d", dev_name, dev_oid);
  putobj(graobj, "device", id, device);
}

void
CmOutputPrinter(int select_file, int show_dialog)
{
  GtkPrintOperation *print;
  GtkPrintOperationResult res;
  char buf[MESSAGE_BUF_SIZE];
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid, opt;
  char *g2winst;
  GError *error;
  struct print_obj pobj;
  GtkPaperSize *paper_size;
  GtkPageSetup *page_setup;

  if (Menulock || GlobalLock)
    return;

  if (select_file && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if ((graobj = chkobject("gra")) == NULL)
    return;

  if ((g2wobj = chkobject("gra2gtkprint")) == NULL)
    return;

  g2wid = newobj(g2wobj);
  if (g2wid < 0)
    return;

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  id = newobj(graobj);
  init_graobj(graobj, id, "gra2gtkprint", g2woid);

  print = gtk_print_operation_new();
  gtk_print_operation_set_n_pages(print, 1);
  gtk_print_operation_set_current_page(print, 0);
  gtk_print_operation_set_use_full_page(print, TRUE);

  if (PrintSettings == NULL)
    PrintSettings = gtk_print_settings_new();

  if (Menulocal.PaperId == PAPER_ID_CUSTOM) {
    paper_size = gtk_paper_size_new_custom(Menulocal.PaperName,
					   Menulocal.PaperName,
					   Menulocal.PaperWidth / 100.0,
					   Menulocal.PaperHeight / 100.0,
					   GTK_UNIT_MM);
  } else {
    paper_size = gtk_paper_size_new(Menulocal.PaperName);
  }

  page_setup = gtk_page_setup_new();
  gtk_page_setup_set_paper_size(page_setup, paper_size);
  if (Menulocal.PaperLandscape) {
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
  } else {
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
  }

  gtk_print_operation_set_default_page_setup(print, page_setup);
  gtk_print_operation_set_print_settings(print, PrintSettings);

  pobj.graobj = graobj;
  pobj.id = id;
  pobj.g2wobj = g2wobj;
  pobj.g2wid = g2wid;
  pobj.g2winst = g2winst;
  g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), &pobj);

  switch (show_dialog) {
  case PRINT_SHOW_DIALOG_NONE:
    opt = GTK_PRINT_OPERATION_ACTION_PRINT;
    break;
  case PRINT_SHOW_DIALOG_PREVIEW:
    opt = GTK_PRINT_OPERATION_ACTION_PREVIEW;
    break;
  case PRINT_SHOW_DIALOG_DIALOG:
    opt = GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG;
    break;
  default:
    opt = GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG;
  }

  res = gtk_print_operation_run(print, opt, GTK_WINDOW(TopLevel), &error);

  if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
    snprintf(buf, sizeof(buf), _("Printing error: %s"), error->message);
    MessageBox(TopLevel, buf, _("Print"), MB_ERROR);
    g_error_free(error);
  } else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
    if (PrintSettings)
      g_object_unref(PrintSettings);
    PrintSettings = g_object_ref(gtk_print_operation_get_print_settings(print));
  }
  g_object_unref(print);

  delobj(graobj, id);
  delobj(g2wobj, g2wid);
}


void
CmOutputDriver(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *g2winst;
  int ret;
  struct savedstdio stdio;

  if (Menulock || GlobalLock)
    return;

  if (! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL)
    return;

  g2wobj = chkobject("gra2prn");
  if (g2wobj == NULL)
    return;

  g2wid = newobj(g2wobj);
  if (g2wid < 0)
    return;

  DriverDialog(&DlgDriver, g2wobj, g2wid);
  ret = DialogExecute(TopLevel, &DlgDriver);

  if (ret == IDOK) {
    SetStatusBar(_("Spawning external driver."));
    g2winst = chkobjinst(g2wobj, g2wid);
    _getobj(g2wobj, "oid", g2winst, &g2woid);
    id = newobj(graobj);
    init_graobj(graobj, id, "gra2prn", g2woid);
    ignorestdio(&stdio);
    draw_gra(graobj, id, _("Drawing."), TRUE);
    restorestdio(&stdio);
    delobj(graobj, id);
    ResetStatusBar();
  }
  delobj(g2wobj, g2wid);
}

void
CmOutputViewer(int select_file)
{
  if (Menulock || GlobalLock)
    return;

  if (select_file && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if (Menulocal.exwin_use_external) {
    struct objlist *menuobj;
    int show_dialog, s;
    char *argv[3];

    menuobj = chkobject("menu");
    if (menuobj == NULL)
      return;

    s = FALSE;
    show_dialog = PRINT_SHOW_DIALOG_PREVIEW;
    argv[0] = (char *) &s;
    argv[1] = (char *) &show_dialog;
    argv[2] = NULL;

    exeobj(menuobj, "print", 0, 2, argv);
  } else {
    struct objlist *graobj, *g2wobj;
    int id, g2wid, g2woid;
    char *g2winst;
    int delgra;

    if ((graobj = chkobject("gra")) == NULL)
      return;

    if ((g2wobj = chkobject("gra2gtk")) == NULL)
      return;

    g2wid = newobj(g2wobj);

    if (g2wid < 0)
      return;

    g2winst = chkobjinst(g2wobj, g2wid);
    _getobj(g2wobj, "oid", g2winst, &g2woid);
    putobj(g2wobj, "dpi", g2wid, &(Menulocal.exwindpi));
    putobj(g2wobj, "BR", g2wid, &(Menulocal.bg_r));
    putobj(g2wobj, "BG", g2wid, &(Menulocal.bg_g));
    putobj(g2wobj, "BB", g2wid, &(Menulocal.bg_b));
    id = newobj(graobj);
    init_graobj(graobj, id, "gra2gtk", g2woid);
    draw_gra(graobj, id, _("Spawning external viewer."), FALSE);

    delgra = TRUE;
    _putobj(g2wobj, "delete_gra", g2winst, &delgra);
  }
}

static char *
get_base_ngp_name(void)
{
  char *ptr, *tmp;

  if (NgraphApp.FileName == NULL)
    return NULL;

  tmp = strdup(NgraphApp.FileName);
  if (tmp == NULL)
    return NULL;

  ptr = strrchr(tmp, '.');
  if (ptr && strcmp(ptr, ".ngp") == 0) {
    *ptr = '\0';
  }

  return tmp;
}

void
CmPrintGRAFile(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid, ret;
  char *g2winst, *tmp, *file, *filebuf;

  if (Menulock || GlobalLock)
    return;

  tmp = get_base_ngp_name();

  ret = nGetSaveFileName(TopLevel, _("GRA file"), "gra", NULL, tmp,
			 &filebuf, "*.gra", FALSE, Menulocal.changedirectory);

  if (tmp)
    free(tmp);

  if (ret != IDOK)
    return;

  file = nstrdup(filebuf);
  free(filebuf);
  if (file == NULL) {
    return;
  }

  if (! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if ((graobj = chkobject("gra")) == NULL) {
    memfree(file);
    return;
  }

  if ((g2wobj = chkobject("gra2file")) == NULL) {
    memfree(file);
    return;
  }

  g2wid = newobj(g2wobj);
  if (g2wid < 0) {
    memfree(file);
    return;
  }

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  putobj(g2wobj, "file", g2wid, file);
  id = newobj(graobj);
  init_graobj(graobj, id, "gra2file", g2woid);
  draw_gra(graobj, id, _("Making GRA file."), TRUE);
  delobj(graobj, id);
  delobj(g2wobj, g2wid);
}

void
CmOutputImage(int type)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *g2winst;
  int ret, format, t2p, dpi;
  char *ext_name, *ext_str, *ext;
  char *file, *filebuf, *tmp;

  if (Menulock || GlobalLock)
    return;

  switch (type) {
  case MenuIdOutputPSFile:
    ext_name = "PostScript (*.ps)";
    ext_str = "ps";
    ext = "*.ps";
    break;
  case MenuIdOutputEPSFile:
    ext_name = "Encapsulated PostScript (*.eps)";
    ext_str = "eps";
    ext = "*.eps";
    break;
  case MenuIdOutputPDFFile:
    ext_name = "PDF (*.pdf)";
    ext_str = "pdf";
    ext = "*.pdf";
    break;
  case MenuIdOutputPNGFile:
    ext_name = "PNG (*.png)";
    ext_str = "png";
    ext = "*.png";
    break;
  case MenuIdOutputSVGFile:
    ext_name = "SVG (*.svg)";
    ext_str = "svg";
    ext = "*.svg";
    break;
  default:
    /* not reachable */
    ext_name = NULL;
    ext_str = NULL;
    ext = NULL;
  }

  tmp = get_base_ngp_name();

  ret = nGetSaveFileName(TopLevel, ext_name, ext_str, NULL, tmp,
			 &filebuf, ext, FALSE, Menulocal.changedirectory);
  if (tmp)
    free(tmp);

  if (ret != IDOK) {
    return;
  }

  file = nstrdup(filebuf);
  free(filebuf);
  if (file == NULL) {
    return;
  }

  OutputImageDialog(&DlgImageOut, type);
  ret = DialogExecute(TopLevel, &DlgImageOut);
  if (ret != IDOK) {
    memfree(file);
    return;
  }

  if (! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL) {
    memfree(file);
    return;
  }

  g2wobj = chkobject("gra2cairofile");
  if (g2wobj == NULL) {
    memfree(file);
    return;
  }

  g2wid = newobj(g2wobj);
  if (g2wid < 0) {
    memfree(file);
    return;
  }

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  id = newobj(graobj);
  putobj(g2wobj, "file", g2wid, file);

  switch (type) {
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
  case MenuIdOutputSVGFile:
  case MenuIdOutputPDFFile:
    t2p = DlgImageOut.text2path;
    putobj(g2wobj, "text2path", g2wid, &t2p);
    break;
  case MenuIdOutputPNGFile:
    break;
  }
  dpi = DlgImageOut.Dpi;
  putobj(g2wobj, "dpi", g2wid, &dpi);
  format = DlgImageOut.Version;
  putobj(g2wobj, "format", g2wid, &format);
  init_graobj(graobj, id,  "gra2cairofile", g2woid);
  draw_gra(graobj, id, _("Drawing."), TRUE);
  delobj(graobj, id);
  delobj(g2wobj, g2wid);
}

void
CmPrintDataFile(void)
{
  struct narray farray;
  struct objlist *obj;
  int i, num, onum, type, div, curve = FALSE, *array, append;
  char *file;
  char *argv[4];

  if (Menulock || GlobalLock)
    return;

  if (GetDrawFiles(&farray))
    return;

  obj = chkobject("file");
  if (obj == NULL)
    return;

  onum = chkobjlastinst(obj);
  num = arraynum(&farray);

  if (num == 0) {
    arraydel(&farray);
    return;
  }

  array = (int *) arraydata(&farray);
  for (i = 0; i < num; i++) {
    if (array[i] < 0 || array[i] > onum)
      continue;

    getobj(obj, "type", array[i], 0, NULL, &type);
    if (type == 3) {
      curve = TRUE;
    }
  }

  div = 10;

  if (curve) {
    OutputDataDialog(&DlgOutputData, div);
    if (DialogExecute(TopLevel, &DlgOutputData) != IDOK) {
      arraydel(&farray);
      return;
    }
    div = DlgOutputData.div;
  }

  if (nGetSaveFileName(TopLevel, _("Data file"), NULL, NULL, NULL,
		       &file, NULL, FALSE, Menulocal.changedirectory) != IDOK) {
    arraydel(&farray);
    return;
  }

  ProgressDialogCreate(_("Making data file"));
  SetStatusBar(_("Making data file."));

  argv[0] = (char *) file;
  argv[1] = (char *) &div;
  argv[3] = NULL;
  for (i = 0; i < num; i++) {
    if (array[i] < 0 || array[i] > onum)
      continue;

    append = (i == 0) ? FALSE : TRUE;
    argv[2] = (char *) &append;
    if (exeobj(obj, "output_file", array[i], 3, argv))
      break;
  }
  ProgressDialogFinalize();
  ResetStatusBar();
  gdk_window_invalidate_rect(NgraphApp.Viewer.win, NULL, FALSE);

  arraydel(&farray);
  free(file);
}

void
CmOutputDriverB(GtkWidget *wi, gpointer client_data)
{
  CmOutputDriver();
}

void
CmOutputPrinterB(GtkWidget *wi, gpointer client_data)
{
  CmOutputPrinter(FALSE, PRINT_SHOW_DIALOG_DIALOG);
}

void
CmOutputViewerB(GtkWidget *wi, gpointer client_data)
{
  CmOutputViewer(FALSE);
}

void
CmOutputMenu(GtkWidget *wi, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdViewerDraw:
    CmViewerDraw();
    break;
  case MenuIdViewerClear:
    CmViewerClear();
    break;
  case MenuIdOutputViewer:
    CmOutputViewer(TRUE);
    break;
  case MenuIdOutputDriver:
    CmOutputDriver();
    break;
  case MenuIdOutputGRAFile:
    CmPrintGRAFile();
    break;
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
  case MenuIdOutputPDFFile:
  case MenuIdOutputPNGFile:
  case MenuIdOutputSVGFile:
    CmOutputImage((int) client_data);
    break;
  case MenuIdPrintDataFile:
    CmPrintDataFile();
    break;
  }
}
