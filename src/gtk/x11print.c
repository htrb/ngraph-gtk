/*
 * $Id: x11print.c,v 1.52 2010-03-04 08:30:17 hito Exp $
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

#include "nstring.h"
#include "object.h"
#include "ioutil.h"

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
  "SVG version 1.1",
  "SVG version 1.2",
  NULL,
};

struct print_obj {
  struct objlist *graobj, *g2wobj;
  int id;
  N_VALUE *g2winst;
};

static GtkPrintSettings *PrintSettings = NULL;

static void
DriverDialogSelectCB(GtkWidget *wi, gpointer client_data)
{
  int a, i, l, n;
  struct extprinter *pcur;
  struct DriverDialog *d;
  char *ptr, ngp_ext[] = ".ngp";

  d = (struct DriverDialog *) client_data;

  a = combo_box_get_active(d->driver);

  if (a < 0)
    return;

  pcur = Menulocal.extprinterroot;
  i = 0;
  while (pcur) {
    if (i != a) {
      pcur = pcur->next;
      i++;
      continue;
    }

    if (pcur->ext && pcur->ext[0] != '\0') {
      if (NgraphApp.FileName) {
	l = strlen(NgraphApp.FileName);
	n = sizeof(ngp_ext) - 1;
	if (l > n && strcasecmp(NgraphApp.FileName + l - n, ngp_ext) == 0) {
	  l -= n;
	}
	ptr = g_strdup_printf("%.*s.%s", l, NgraphApp.FileName, CHK_STR(pcur->ext));
	gtk_entry_set_text(GTK_ENTRY(d->file), CHK_STR(ptr));
	g_free(ptr);
      }
      d->ext = pcur->ext;
    } else {
      gtk_entry_set_text(GTK_ENTRY(d->file), "");
      d->ext = NULL;
    }

    gtk_entry_set_text(GTK_ENTRY(d->option), CHK_STR(pcur->option));
    break;
  }
}

static void
DriverDialogBrowseCB(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  char *file;
  const char *str;
  struct DriverDialog *d;

  d = (struct DriverDialog *) user_data;

  str = gtk_entry_get_text(w);

  if (nGetSaveFileName(d->widget, _("External Driver Output"), d->ext, NULL,
		       str, &file, TRUE, Menulocal.changedirectory) == IDOK) {
    gtk_entry_set_text(w, file);
  }
  g_free(file);
}

static void
DriverDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  struct DriverDialog *d;
  struct extprinter *pcur;
  int i, j;

  d = (struct DriverDialog *) data;
  if (makewidget) {

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = combo_box_create();
    d->driver = w;
    add_widget_to_table(table, w, _("_Driver:"), FALSE, i++);
    g_signal_connect(d->driver, "changed", G_CALLBACK(DriverDialogSelectCB), d);

    w = create_text_entry(FALSE, TRUE);
    d->option = w;
    add_widget_to_table(table, w, _("_Option:"), TRUE, i++);

    w = create_file_entry_with_cb(G_CALLBACK(DriverDialogBrowseCB), d);
    d->file = w;
    add_widget_to_table(table, w, _("_File:"), TRUE, i++);

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
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
  int a, i;
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
      driver = g_strdup(pcur->driver);
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

  option = NULL;

  if (s || file) {
    option = g_strdup_printf("%s%s%s%s",
		  (file) ? "-o '" : "",
		  CHK_STR(file),
		  (file) ? "' " : "",
		  CHK_STR(s));
  }

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
#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif
    item_setup(hbox, w, _("_Div:"), TRUE);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
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
  GtkWidget *vlabel;
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkWidget *window;
  GtkRequisition minimum_size;
#endif

  vlabel = get_mnemonic_label(d->version);

  gtk_label_set_text(GTK_LABEL(vlabel), "");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_opacity), FALSE);

  combo_box_clear(d->version);
  switch (d->DlgType) {
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, PsVersion[i]);
    }
    set_widget_visibility_with_label(d->dpi, FALSE);
    set_widget_visibility_with_label(d->version, TRUE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(vlabel), _("_PostScript Version:"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    combo_box_set_active(d->version, Menulocal.ps_version);
    break;
  case MenuIdOutputPNGFile:
    set_widget_visibility_with_label(d->dpi, TRUE);
    set_widget_visibility_with_label(d->version, FALSE);
    gtk_widget_set_visible(d->t2p, FALSE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), Menulocal.png_dpi);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_opacity), Menulocal.use_opacity);

    break;
  case MenuIdOutputPDFFile:
    set_widget_visibility_with_label(d->dpi, FALSE);
    set_widget_visibility_with_label(d->version, FALSE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_opacity), Menulocal.use_opacity);
    break;
  case MenuIdOutputSVGFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, SvgVersion[i]);
    }

    set_widget_visibility_with_label(d->dpi, FALSE);
    set_widget_visibility_with_label(d->version, TRUE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(vlabel), _("_SVG Version:"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    combo_box_set_active(d->version, Menulocal.svg_version);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->use_opacity), Menulocal.use_opacity);
    break;
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
    set_widget_visibility_with_label(d->dpi, TRUE);
    set_widget_visibility_with_label(d->version, FALSE);
    gtk_widget_set_visible(d->t2p, FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), Menulocal.emf_dpi);
    break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  window = gtk_widget_get_parent(GTK_WIDGET(d->vbox));
  if (GTK_IS_WINDOW(window)) {
    gtk_widget_get_preferred_size(GTK_WIDGET(d->vbox), &minimum_size, NULL);
    gtk_window_resize(GTK_WINDOW(window), minimum_size.width, minimum_size.height);
  }
#endif
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

    w = gtk_check_button_new_with_mnemonic(_("_Use opacity"));
    d->use_opacity = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);

    w = gtk_spin_button_new_with_range(1, DPI_MAX, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
    d->dpi = w;
    item_setup(GTK_WIDGET(d->vbox), w, "_DPI:", FALSE);

    w = combo_box_create();
    d->version = w;
    item_setup(GTK_WIDGET(d->vbox), w, "", FALSE);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
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
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
    title = N_("Cairo EMF Output");
    break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
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

  d->Dpi = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(d->dpi));
  d->UseOpacity = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->use_opacity));

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
    Menulocal.ps_version = a;
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
    Menulocal.ps_version = a;
    break;
  case MenuIdOutputPNGFile:
    d->Version = TYPE_PNG;
    Menulocal.png_dpi = d->Dpi;
    break;
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
    d->Version = TYPE_EMF;
    Menulocal.emf_dpi = d->Dpi;
    break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  case MenuIdOutputPDFFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    d->Version = TYPE_PDF;
    break;
  case MenuIdOutputSVGFile:
    d->text2path = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->t2p));
    a = combo_box_get_active(d->version);
    switch (a) {
    case 0:
      d->Version = TYPE_SVG1_1;
      break;
    case 1:
    default:
      d->Version = TYPE_SVG1_2;
      break;
    }
    Menulocal.svg_version = a;
    break;
  }

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
  main_window_redraw();
}

static void
draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, gpointer user_data)
{
  struct objlist *graobj, *g2wobj;
  char *argv[2];
  struct print_obj *pobj;
  int id, r;
  N_VALUE *g2winst;

  pobj = (struct print_obj *) user_data;
  graobj = pobj->graobj;
  g2wobj = pobj->g2wobj;
  id = pobj->id;
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
		*(char **) arraynget(&(Menulocal.drawrable), i));
    }
  } else {
    drawrable = NULL;
  }
  putobj(graobj, "draw_obj", id, drawrable);

  device = (char *) g_malloc(DEVICE_BUF_SIZE);
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
  N_VALUE *g2winst;
  GError *error;
  struct print_obj pobj;
  GtkPaperSize *paper_size;
  GtkPageSetup *page_setup;

  if (Menulock || Globallock)
    return;

  if (select_file && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL)
    return;

  g2wobj = chkobject("gra2gtkprint");
  if (g2wobj == NULL)
    return;

  g2wid = newobj(g2wobj);
  if (g2wid < 0)
    return;

  putobj(g2wobj, "use_opacity", g2wid, &Menulocal.use_opacity);

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  id = newobj(graobj);
  init_graobj(graobj, id, "gra2gtkprint", g2woid);

  print = gtk_print_operation_new();
  gtk_print_operation_set_n_pages(print, 1);
#if GTK_CHECK_VERSION(2, 18, 0)
  gtk_print_operation_set_has_selection(print, FALSE);
  gtk_print_operation_set_support_selection(print, FALSE);
  gtk_print_operation_set_embed_page_setup(print, FALSE);
#endif
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
    message_box(NULL, buf, _("Print"), RESPONS_ERROR);
    g_error_free(error);
  } else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
    if (PrintSettings)
      g_object_unref(PrintSettings);
    PrintSettings = g_object_ref(gtk_print_operation_get_print_settings(print));
  }
  g_object_unref(print);

  delobj(graobj, id);
  delobj(g2wobj, g2wid);

  if (select_file) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
}

void
CmOutputDriver(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  N_VALUE *g2winst;
  int ret;
  struct savedstdio stdio;

  if (Menulock || Globallock)
    return;

  if (Menulocal.select_data && ! SetFileHidden())
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

  if (Menulocal.select_data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
}

void
CmOutputViewerB(void *wi, gpointer client_data)
{
  if (Menulock || Globallock)
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
    int id, g2wid, g2woid, c;
    N_VALUE *g2winst;
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
    putobj(g2wobj, "width", g2wid, &Menulocal.exwinwidth);
    putobj(g2wobj, "height", g2wid, &Menulocal.exwinheight);
    c = Menulocal.bg_r * 255.0;
    putobj(g2wobj, "BR", g2wid, &c);
    c = Menulocal.bg_g * 255.0;
    putobj(g2wobj, "BG", g2wid, &c);
    c = Menulocal.bg_b * 255.0;
    putobj(g2wobj, "BB", g2wid, &c);
    putobj(g2wobj, "use_opacity", g2wid, &Menulocal.use_opacity);
    id = newobj(graobj);
    init_graobj(graobj, id, "gra2gtk", g2woid);
    draw_gra(graobj, id, _("Spawning external viewer."), FALSE);

    delgra = TRUE;
    _putobj(g2wobj, "delete_gra", g2winst, &delgra);

    exeobj(g2wobj, "present", g2wid, 0, NULL);
  }
}

static char *
get_base_ngp_name(void)
{
  char *ptr, *tmp;

  if (NgraphApp.FileName == NULL)
    return NULL;

  tmp = g_strdup(NgraphApp.FileName);
  if (tmp == NULL)
    return NULL;

  ptr = strrchr(tmp, '.');
  if (ptr && strcmp(ptr, ".ngp") == 0) {
    *ptr = '\0';
  }

  return tmp;
}

static void
CmPrintGRAFile(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid, ret;
  N_VALUE *g2winst;
  char *tmp, *file;

  if (Menulock || Globallock)
    return;

  tmp = get_base_ngp_name();

  ret = nGetSaveFileName(TopLevel, _("GRA file"), "gra", NULL, tmp,
			 &file, FALSE, Menulocal.changedirectory);

  if (tmp)
    g_free(tmp);

  if (ret != IDOK)
    return;

  if (file == NULL) {
    return;
  }

  if (Menulocal.select_data && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if ((graobj = chkobject("gra")) == NULL) {
    g_free(file);
    return;
  }

  if ((g2wobj = chkobject("gra2file")) == NULL) {
    g_free(file);
    return;
  }

  g2wid = newobj(g2wobj);
  if (g2wid < 0) {
    g_free(file);
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

  if (Menulocal.select_data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
}

static void
CmOutputImage(int type)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  N_VALUE *g2winst;
  int ret;
  char *title, *ext_str;
  char *file, *tmp;

  if (Menulock || Globallock)
    return;

  switch (type) {
  case MenuIdOutputPSFile:
    title = "Save as PostScript";
    ext_str = "ps";
    break;
  case MenuIdOutputEPSFile:
    title = "Save as Encapsulated PostScript";
    ext_str = "eps";
    break;
  case MenuIdOutputPDFFile:
    title = "Save as Portable Document Format (PDF)";
    ext_str = "pdf";
    break;
  case MenuIdOutputPNGFile:
    title = "Save as Portable Network Graphics (PNG)";
    ext_str = "png";
    break;
  case MenuIdOutputSVGFile:
    title = "Save as Scalable Vector Graphics (SVG)";
    ext_str = "svg";
    break;
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
    title = "Save as Windows Enhanced Metafile (EMF)";
    ext_str = "emf";
    break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  default:
    /* not reachable */
    title = NULL;
    ext_str = NULL;
  }

  tmp = get_base_ngp_name();
  ret = nGetSaveFileName(TopLevel, title, ext_str, NULL, tmp,
			 &file, FALSE, Menulocal.changedirectory);
  if (tmp) {
    g_free(tmp);
  }

  if (ret != IDOK) {
    return;
  }

  if (file == NULL) {
    return;
  }

  OutputImageDialog(&DlgImageOut, type);
  ret = DialogExecute(TopLevel, &DlgImageOut);
  if (ret != IDOK) {
    g_free(file);
    return;
  }

  if (Menulocal.select_data && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL) {
    g_free(file);
    return;
  }

  g2wobj = chkobject("gra2cairofile");
  if (g2wobj == NULL) {
    g_free(file);
    return;
  }

  g2wid = newobj(g2wobj);
  if (g2wid < 0) {
    g_free(file);
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
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
#endif	/* CAIRO_HAS_WIN32_SURFACE */
    putobj(g2wobj, "text2path", g2wid, &DlgImageOut.text2path);
    break;
  case MenuIdOutputPNGFile:
    break;
  }

  putobj(g2wobj, "use_opacity", g2wid, &DlgImageOut.UseOpacity);
  putobj(g2wobj, "dpi", g2wid, &DlgImageOut.Dpi);
  putobj(g2wobj, "format", g2wid, &DlgImageOut.Version);

  init_graobj(graobj, id, "gra2cairofile", g2woid);
  draw_gra(graobj, id, _("Drawing."), TRUE);
  delobj(graobj, id);
  delobj(g2wobj, g2wid);

  if (Menulocal.select_data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
}

#ifdef WINDOWS
static void
CmOutputEMF(int type)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  N_VALUE *g2winst;
  int ret;
  char *title, *ext_str;
  char *file, *tmp;

  if (Menulock || Globallock)
    return;

  title = "Save as Windows Enhanced Metafile (EMF)";

  file = NULL;
  if (type == MenuIdOutputEMFFile) {
    ext_str = "emf";
    tmp = get_base_ngp_name();
    ret = nGetSaveFileName(TopLevel, title, ext_str, NULL, tmp,
			   &file, FALSE, Menulocal.changedirectory);
    if (tmp) {
      g_free(tmp);
    }

    if (ret != IDOK) {
      return;
    }

    if (file == NULL) {
      return;
    }
  }

  if (Menulocal.select_data && ! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL) {
    g_free(file);
    return;
  }

  g2wobj = chkobject("gra2emf");
  if (g2wobj == NULL) {
    g_free(file);
    return;
  }

  g2wid = newobj(g2wobj);
  if (g2wid < 0) {
    g_free(file);
    return;
  }

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  id = newobj(graobj);
  putobj(g2wobj, "file", g2wid, file);
  init_graobj(graobj, id, "gra2emf", g2woid);
  draw_gra(graobj, id, _("Drawing."), TRUE);
  delobj(graobj, id);
  delobj(g2wobj, g2wid);

  if (Menulocal.select_data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
}
#endif

void
CmOutputPrinterB(void *wi, gpointer client_data)
{
  CmOutputPrinter(FALSE, PRINT_SHOW_DIALOG_DIALOG);
}

void
CmOutputMenu(void *wi, gpointer client_data)
{
  switch (GPOINTER_TO_INT(client_data)) {
  case MenuIdOutputGRAFile:
    CmPrintGRAFile();
    break;
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
  case MenuIdOutputPDFFile:
  case MenuIdOutputPNGFile:
  case MenuIdOutputSVGFile:
#ifdef CAIRO_HAS_WIN32_SURFACE
  case MenuIdOutputCairoEMFFile:
#endif	/* CAIRO_HAS_WIN32_SURFACE */
    CmOutputImage(GPOINTER_TO_INT(client_data));
    break;
#ifdef WINDOWS
  case MenuIdOutputEMFFile:
  case MenuIdOutputEMFClipboard:
    CmOutputEMF(GPOINTER_TO_INT(client_data));
    break;
#endif
  }
}
