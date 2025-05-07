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
#if WINDOWS
static void output_emf(int type, char *file);
#endif

static void
OutputDataDialogSetupItem(GtkWidget *w, struct OutputDataDialog *d)
{
  spin_entry_set_val(d->div_entry, d->div);
}

static void
OutputDataDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct OutputDataDialog *d;

  d = (struct OutputDataDialog *) data;
  if (makewidget) {
    GtkWidget *w, *hbox;
    w = create_spin_entry(0, 200, 1, FALSE, TRUE);
    d->div_entry = w;
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    item_setup(hbox, w, _("_Div:"), TRUE);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
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
OutputImageDialogSetupItem(struct OutputImageDialog *d)
{
  int i;
  GtkWidget *vlabel;
  GtkWidget *window;
  GtkRequisition minimum_size;

  vlabel = get_mnemonic_label(d->version);

  gtk_label_set_text(GTK_LABEL(vlabel), "");

  combo_box_clear(d->version);
  switch (d->DlgType) {
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, PsVersion[i]);
    }
    set_widget_visibility_with_label(d->dpi, TRUE);
    set_widget_visibility_with_label(d->version, TRUE);
    set_widget_visibility_with_label(d->use_opacity, TRUE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_opacity), FALSE);
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(vlabel), _("_PostScript Version:"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    combo_box_set_active(d->version, Menulocal.ps_version);
    break;
  case MenuIdOutputPNGFile:
    set_widget_visibility_with_label(d->dpi, TRUE);
    set_widget_visibility_with_label(d->version, FALSE);
    set_widget_visibility_with_label(d->use_opacity, FALSE);
    gtk_widget_set_visible(d->t2p, FALSE);

    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->use_opacity), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), Menulocal.png_dpi);
    break;
  case MenuIdOutputPDFFile:
    set_widget_visibility_with_label(d->dpi, FALSE);
    set_widget_visibility_with_label(d->version, FALSE);
    set_widget_visibility_with_label(d->use_opacity, FALSE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    break;
  case MenuIdOutputSVGFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, SvgVersion[i]);
    }

    set_widget_visibility_with_label(d->dpi, FALSE);
    set_widget_visibility_with_label(d->version, TRUE);
    set_widget_visibility_with_label(d->use_opacity, FALSE);
    gtk_widget_set_visible(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(vlabel), _("_SVG Version:"));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), 72);
    combo_box_set_active(d->version, Menulocal.svg_version);
    break;
  }

  window = gtk_widget_get_parent(GTK_WIDGET(d->vbox));
  if (GTK_IS_WINDOW(window)) {
    gtk_widget_get_preferred_size(GTK_WIDGET(d->vbox), &minimum_size, NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), minimum_size.width, minimum_size.height);
  }
}

static void
OutputImageDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct OutputImageDialog *d;
  char *title;

  d = (struct OutputImageDialog *) data;
  if (makewidget) {
    GtkWidget *w, *box;
    w = gtk_check_button_new_with_mnemonic(_("_Convert texts to paths"));
    d->t2p = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_Use opacity"));
    d->use_opacity = w;
    gtk_box_append(GTK_BOX(d->vbox), w);

    w = gtk_spin_button_new_with_range(1, DPI_MAX, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w), TRUE);
    spin_button_set_activates_default(w);
    d->dpi = w;
    item_setup(GTK_WIDGET(d->vbox), w, "_DPI:", FALSE);

    box = gtk_widget_get_parent (d->dpi);
    g_object_bind_property (d->use_opacity, "active", box, "sensitive", G_BINDING_SYNC_CREATE);

    w = combo_box_create();
    d->version = w;
    item_setup(GTK_WIDGET(d->vbox), w, "", FALSE);
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
  OutputImageDialogSetupItem(d);
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
  d->UseOpacity = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->use_opacity));

  switch (d->DlgType) {
  case MenuIdOutputPSFile:
    d->text2path = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->t2p));
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
    d->text2path = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->t2p));
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
  case MenuIdOutputPDFFile:
    d->text2path = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->t2p));
    d->Version = TYPE_PDF;
    break;
  case MenuIdOutputSVGFile:
    d->text2path = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->t2p));
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

static void
OutputImageDialog(struct OutputImageDialog *data, int type)
{
  data->SetupWindow = OutputImageDialogSetup;
  data->CloseWindow = OutputImageDialogClose;
  data->DlgType = type;
}

typedef void (* draw_gra_cb) (gpointer user_data);

struct draw_gra_data {
  struct objlist *graobj;
  int id, close;
  gpointer data;
  draw_gra_cb cb;
};

static void
draw_gra_finalize(gpointer user_data)
{
  struct draw_gra_data *data;
  data = (struct draw_gra_data *) user_data;

  ResetStatusBar();
  main_window_redraw();
  if (data->cb) {
    data->cb(data->data);
  }
  g_free(user_data);
}

static void
draw_gra_main(gpointer user_data)
{
  struct objlist *graobj;
  int id;
  struct draw_gra_data *data;
  data = (struct draw_gra_data *) user_data;

  graobj = data->graobj;
  id = data->id;
  if (exeobj(graobj, "open", id, 0, NULL) == 0) {
    exeobj(graobj, "draw", id, 0, NULL);
    exeobj(graobj, "flush", id, 0, NULL);
    if (data->close) {
      exeobj(graobj, "close", id, 0, NULL);
    }
  }
}

static void
draw_gra(struct objlist *graobj, int id, char *msg, int close, draw_gra_cb cb, gpointer user_data )
{
  struct draw_gra_data *data;
  data = g_malloc0(sizeof(*data));
  data->graobj = graobj;
  data->id = id;
  data->close = close;
  data->data = user_data;
  data->cb = cb;
  SetStatusBar(msg);
  if (cb) {
    ProgressDialogCreate(msg, draw_gra_main, draw_gra_finalize, data);
  } else {
    draw_gra_main(data);
    draw_gra_finalize(data);
  }
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
    draw_gra(graobj, id, _("Printing."), TRUE, NULL, NULL);
  }
}

static void
init_graobj(struct objlist *graobj, int id, const char *dev_name, int dev_oid)
{
  struct narray *drawrable;
  char *device;

  putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
  putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
  putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
  putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
  putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));
  putobj(graobj, "decimalsign", id, &(Menulocal.Decimalsign));
  if (arraynum(&(Menulocal.drawrable)) > 0) {
    unsigned int i, n;
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

static void
output_printer(int select_file, int show_dialog)
{
  GtkPrintOperation *print;
  GtkPrintOperationResult res;
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid, opt, landscape, w, h;
  N_VALUE *g2winst;
  GError *error;
  struct print_obj pobj;
  GtkPaperSize *paper_size;
  GtkPageSetup *page_setup;
  int use_opacity;

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

  use_opacity = TRUE;
  putobj(g2wobj, "use_opacity", g2wid, &use_opacity);

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  id = newobj(graobj);
  init_graobj(graobj, id, "gra2gtkprint", g2woid);

  print = gtk_print_operation_new();
  gtk_print_operation_set_n_pages(print, 1);
  gtk_print_operation_set_has_selection(print, FALSE);
  gtk_print_operation_set_support_selection(print, FALSE);
  gtk_print_operation_set_embed_page_setup(print, FALSE);
  gtk_print_operation_set_use_full_page(print, TRUE);

  if (PrintSettings == NULL)
    PrintSettings = gtk_print_settings_new();

  landscape = Menulocal.PaperLandscape;
  switch (Menulocal.PaperId) {
  case PAPER_ID_CUSTOM:
  case PAPER_ID_NORMAL:
  case PAPER_ID_WIDE:
  case PAPER_ID_WIDE2:
    if (landscape) {
      h = Menulocal.PaperWidth;
      w = Menulocal.PaperHeight;
    } else {
      w = Menulocal.PaperWidth;
      h = Menulocal.PaperHeight;
    }
    paper_size = gtk_paper_size_new_custom(Menulocal.PaperName,
					   Menulocal.PaperName,
					   w / 100.0,
					   h / 100.0,
					   GTK_UNIT_MM);
    break;
  default:
    paper_size = gtk_paper_size_new(Menulocal.PaperName);
  }

  page_setup = gtk_page_setup_new();
  gtk_page_setup_set_paper_size(page_setup, paper_size);
  gtk_paper_size_free(paper_size);
  if (landscape) {
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
  } else {
    gtk_page_setup_set_orientation(page_setup, GTK_PAGE_ORIENTATION_PORTRAIT);
  }

  gtk_print_operation_set_default_page_setup(print, page_setup);
  g_object_unref(page_setup);

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
    char buf[MESSAGE_BUF_SIZE];
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

  if (select_file && NgraphApp.FileWin.data.data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, TRUE);
  }
}

struct output_printer_data {
  int select_file, show_dialog;
  response_cb cb;
  gpointer data;
};

static void
output_printer_response(int res, gpointer user_data)
{
  struct output_printer_data *data;
  data = (struct output_printer_data *) user_data;
  if (res) {
    output_printer(data->select_file, data->show_dialog);
  }
  if (data->cb) {
    data->cb(res, data->data);
  }
  g_free(data);
}

void
CmOutputPrinter(int select_file, int show_dialog, response_cb cb, gpointer user_data)
{
  if (Menulock || Globallock) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    return;
  }

  if (select_file) {
    struct output_printer_data *data;
    data = g_malloc0(sizeof(*data));
    if (data) {
      data->select_file = select_file;
      data->show_dialog = show_dialog;
      data->cb = cb;
      data->data = user_data;
      SetFileHidden(output_printer_response, data);
      return;
    }
  }
  output_printer(select_file, show_dialog);
  if (cb) {
    cb(IDOK, user_data);
  }
}

struct previewer_data {
  struct objlist *g2wobj;
  N_VALUE *g2winst;
  int g2wid;
};

static void
previewer_cb(gpointer user_data)
{
  struct previewer_data *data;
  int delgra;

  data = (struct previewer_data *) user_data;
  delgra = TRUE;
  _putobj(data->g2wobj, "delete_gra", data->g2winst, &delgra);
  exeobj(data->g2wobj, "present", data->g2wid, 0, NULL);
  g_free(data);
}

void
CmOutputViewerB(void)
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
    struct previewer_data *data;
    int use_opacity;

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
    use_opacity = TRUE;
    putobj(g2wobj, "use_opacity", g2wid, &use_opacity);
    id = newobj(graobj);
    init_graobj(graobj, id, "gra2gtk", g2woid);
    data = g_malloc0(sizeof(*data));
    data->g2wobj = g2wobj;
    data->g2wid = g2wid;
    data->g2winst = g2winst;
    draw_gra(graobj, id, _("Spawning external viewer."), FALSE, previewer_cb, data);
  }
}

static char *
get_base_ngp_name(void)
{
  char *ptr, *tmp;

  if (NgraphApp.FileName == NULL)
    return g_strdup(_("untitled"));

  tmp = g_strdup(NgraphApp.FileName);
  if (tmp == NULL)
    return NULL;

  ptr = strrchr(tmp, '.');
  if (ptr && strcmp(ptr, ".ngp") == 0) {
    *ptr = '\0';
  }

  return tmp;
}

struct gra_out_data
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid;
};

static void
gra_out_cb(gpointer user_data)
{
  struct gra_out_data *data;
  data = (struct gra_out_data *) user_data;

  delobj(data->graobj, data->id);
  delobj(data->g2wobj, data->g2wid);

  if (Menulocal.select_data) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, TRUE);
  }
  g_free(data);
}

struct gra_out_data *
create_gra_out_data(struct objlist *graobj, int id, struct objlist *g2wobj, int g2wid)
{
  struct gra_out_data *data;
  data = g_malloc0(sizeof(*data));
  data->graobj = graobj;
  data->id = id;
  data->g2wobj = g2wobj;
  data->g2wid = g2wid;
  return data;
}

static void
print_gra_file(char *file)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  N_VALUE *g2winst;
  struct gra_out_data *data;

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

  data = create_gra_out_data(graobj, id, g2wobj, g2wid);
  draw_gra(graobj, id, _("Making GRA file."), TRUE, gra_out_cb, data);
}

static void
print_gra_file_response(int res, gpointer user_data)
{
  char *file;
  file = (char *) user_data;
  if (res) {
    print_gra_file(file);
  }
}

static void
CmPrintGRAFile_response(char *file, gpointer user_data)
{

  if (file == NULL) {
    return;
  }

  if (Menulocal.select_data) {
    SetFileHidden(print_gra_file_response, file);
    return;
  }
  print_gra_file(file);
}

static void
CmPrintGRAFile(void)
{
  char *tmp;
  int chd;

  if (Menulock || Globallock)
    return;

  tmp = get_base_ngp_name();

  chd = Menulocal.changedirectory;
  nGetSaveFileName(TopLevel, _("GRA file"), "gra", NULL, tmp, chd, CmPrintGRAFile_response, NULL);

  if (tmp)
    g_free(tmp);
}

struct output_image_data
{
  int type, text2path;
  char *file;
};

static void
output_image(struct output_image_data *d)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid, use_opacity;
  N_VALUE *g2winst;
  struct gra_out_data *data;
  int type, text2path;
  char *file;

  type = d->type;
  file = d->file;
  text2path = d->text2path;
  g_free (d);

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

  use_opacity = TRUE;
  switch (type) {
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
    use_opacity = DlgImageOut.UseOpacity;
    /* fall through */
  case MenuIdOutputSVGFile:
  case MenuIdOutputPDFFile:
    putobj(g2wobj, "text2path", g2wid, &text2path);
    break;
  case MenuIdOutputPNGFile:
    break;
  }

  putobj(g2wobj, "use_opacity", g2wid, &use_opacity);
  putobj(g2wobj, "dpi", g2wid, &DlgImageOut.Dpi);
  putobj(g2wobj, "format", g2wid, &DlgImageOut.Version);

  init_graobj(graobj, id, "gra2cairofile", g2woid);

  data = create_gra_out_data(graobj, id, g2wobj, g2wid);
  draw_gra(graobj, id, _("Making image file."), TRUE, gra_out_cb, data);
}

static void
output_image_response_response(int res, gpointer user_data)
{
  struct output_image_data *data;
  data = (struct output_image_data *) user_data;
  if (res) {
    output_image(data);
  } else {
    g_free(data->file);
    g_free(user_data);
  }
}

static void
output_image_response(struct response_callback *cb)
{
  char *file;
  struct output_image_data *data;

  data = (struct output_image_data *) cb->data;
  file = data->file;

  if (cb->return_value != IDOK) {
    g_free(file);
    g_free(data);
    return;
  }

  if (Menulocal.select_data) {
    SetFileHidden(output_image_response_response, data);
    return;
  }
  output_image(data);
}

static void
CmOutputImage_response(char *file, gpointer user_data)
{
  int type;
  struct output_image_data *data;

  type = GPOINTER_TO_INT(user_data);
#if WINDOWS
  switch (type) {
  case MenuIdOutputEMFFile:
  case MenuIdOutputEMFPlusFile:
  case MenuIdOutputEMFClipboard:
  case MenuIdOutputEMFPlusClipboard:
    output_emf(type, file);
    return;
  }
#endif
  if (file == NULL) {
    return;
  }
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    g_free(file);
    return;
  }
  data->type= type;
  data->file = file;
  OutputImageDialog(&DlgImageOut, type);
  response_callback_add(&DlgImageOut, output_image_response, NULL, data);
  DialogExecute(TopLevel, &DlgImageOut);
}

static void
CmOutputImage(int type)
{
  char *title, *ext_str;
  char *tmp;
  int chd;

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
#if WINDOWS
  case MenuIdOutputEMFPlusFile:
  case MenuIdOutputEMFFile:
    title = "Save as Windows Enhanced Metafile (EMF)";
    ext_str = "emf";
    break;
  case MenuIdOutputEMFClipboard:
  case MenuIdOutputEMFPlusClipboard:
    CmOutputImage_response(NULL, GINT_TO_POINTER (type));
    return;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  default:
    /* not reachable */
    title = NULL;
    ext_str = NULL;
  }

  tmp = get_base_ngp_name();
  chd = Menulocal.changedirectory;
  nGetSaveFileName(TopLevel, title, ext_str, NULL, tmp, chd, CmOutputImage_response, GINT_TO_POINTER(type));
  if (tmp) {
    g_free(tmp);
  }
}

#if WINDOWS
static void
output_emf(int type, char *file)
{
  N_VALUE *g2winst;
  int id, g2wid, g2woid;
  struct objlist *graobj, *g2wobj;
  struct gra_out_data *data;
  char *objname;

  switch (type) {
  case MenuIdOutputEMFPlusFile:
  case MenuIdOutputEMFPlusClipboard:
    objname = "gra2emfplus";
    break;
  case MenuIdOutputEMFFile:
  case MenuIdOutputEMFClipboard:
    objname = "gra2emf";
    break;
  default:
    g_free(file);
    return;
  }

  FileAutoScale();
  AdjustAxis();

  graobj = chkobject("gra");
  if (graobj == NULL) {
    g_free(file);
    return;
  }

  g2wobj = chkobject(objname);
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
  init_graobj(graobj, id, objname, g2woid);
  data = create_gra_out_data(graobj, id, g2wobj, g2wid);
  draw_gra(graobj, id, _("Making image file."), TRUE, gra_out_cb, data);
}
#endif

void
CmOutputPrinterB(void)
{
  CmOutputPrinter(FALSE, PRINT_SHOW_DIALOG_DIALOG, NULL, NULL);
}

void
CmOutputMenu(int menu_id)
{
  switch (menu_id) {
  case MenuIdOutputGRAFile:
    CmPrintGRAFile();
    break;
  case MenuIdOutputPSFile:
  case MenuIdOutputEPSFile:
  case MenuIdOutputPDFFile:
  case MenuIdOutputPNGFile:
  case MenuIdOutputSVGFile:
#if WINDOWS
  case MenuIdOutputEMFFile:
  case MenuIdOutputEMFPlusFile:
  case MenuIdOutputEMFClipboard:
  case MenuIdOutputEMFPlusClipboard:
#endif
    CmOutputImage(menu_id);
    break;
  }
}
