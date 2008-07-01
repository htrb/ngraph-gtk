/* 
 * $Id: x11print.c,v 1.5 2008/07/01 07:09:39 hito Exp $
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

#include "gtk_liststore.h"
#include "gtk_combo.h"

#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "ioutil.h"
#include "mathcode.h"

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

static void
DriverDialogSelectCB(GtkWidget *wi, gpointer client_data)
{
  int a, i, j;
  char *s;
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
      if (pcur->ext && (pcur->ext[0] != '\0') && NgraphApp.FileName) {
	strncpy(buf, NgraphApp.FileName, sizeof(buf) - 1);
	if (strchr(buf, '.') != NULL) {
	  for (j = strlen(buf) - 1; buf[j] != '.'; j--);
	  buf[j] = '\0';
	}
	strncat(buf, pcur->ext, sizeof(buf) - strlen(buf) - 1);
	gtk_entry_set_text(GTK_ENTRY(d->file), buf);
      } else {
	gtk_entry_set_text(GTK_ENTRY(d->file), "");
      }

      if (pcur->option)
	s = pcur->option;
      else
	s = "";

      gtk_entry_set_text(GTK_ENTRY(d->option), s);
      break;
    }
    pcur = pcur->next;
    i++;
  }
}

static void
DriverDialogBrowseCB(GtkWidget *wi, gpointer client_data)
{
  char *file;
  struct DriverDialog *d;

  d = (struct DriverDialog *) client_data;
  if (nGetSaveFileName(TopLevel, _("External Driver Output"), NULL, NULL,
		       NULL, &file, "*", Menulocal.changedirectory) == IDOK) {

    gtk_entry_set_text(GTK_ENTRY(d->file), file);
  }
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

    w = gtk_button_new_with_mnemonic("_Browse");
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
  int a, i, j, len1, len2;
  struct extprinter *pcur;
  struct DriverDialog *d;
  const char *s, *file;
  char *buf, *driver, *option;

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

  option = memalloc(len1 + len2 + 7);
  if (option == NULL) {
    d->ret = IDCANCEL;
    return;
  }

  j = 0;
  option[j] = '\0';
  if (len2 != 0) {
    if (access(file, 04) == 0) {
      len2 += 60;
      buf = (char *) memalloc(len2);
      if (buf) {
	snprintf(buf, len2, _("`%s'\n\nOverwrite existing file?"), file);
	if (MessageBox(TopLevel, buf, "Driver", MB_YESNO) != IDYES) {
	  memfree(buf);
	  memfree(option);
	  d->ret = IDCANCEL;
	  return;
	}
	memfree(buf);
      } else {
	if (MessageBox(TopLevel, _("Overwrite existing file?"),
		       "Driver", MB_YESNO) != IDNO) {
	  memfree(option);
	  d->ret = IDCANCEL;
	  return;
	}
      }
    }
    option[j++] = '-';
    option[j++] = 'o';
    option[j++] = ' ';
    option[j++] = '\'';
    strcpy(option + j, file);
    j = strlen(option);
    option[j++] = '\'';
    option[j++] = ' ';
    option[j] = '\0';
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
}

static void
PrintDialogSelectCB(GtkWidget *wi, gpointer client_data)
{
  int a, i;
  struct prnprinter *pcur;
  struct PrintDialog *d;

  d = (struct PrintDialog *) client_data;

  a = combo_box_get_active(wi);

  if (a < 0)
    return;

  pcur = Menulocal.prnprinterroot;
  i = 0;

  while (pcur != NULL) {
    if (i == a) {
      gtk_entry_set_text(GTK_ENTRY(d->option), (pcur->option) ?  pcur->option: "");
      gtk_entry_set_text(GTK_ENTRY(d->print), (pcur->prn) ?  pcur->prn: "");
      break;
    }
    pcur = pcur->next;
    i++;
  }
}

static void
PrintDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct PrintDialog *d;
  struct prnprinter *pcur;

  d = (struct PrintDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    d->driver = w;
    item_setup(hbox, w, _("_Driver:"), FALSE);
    g_signal_connect(d->driver, "changed", G_CALLBACK(PrintDialogSelectCB), d);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    d->option = w;
    item_setup(hbox, w, _("_Option:"), TRUE);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    d->print = w;
    item_setup(hbox, w, _("_Print:"), TRUE);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  combo_box_clear(d->driver);
  pcur = Menulocal.prnprinterroot;

  while (pcur != NULL) {
    combo_box_append_text(d->driver, pcur->name);
    pcur = pcur->next;
  }
  combo_box_set_active(d->driver, 0);
}

static void
PrintDialogClose(GtkWidget *w, void *data)
{
  int a, i;
  struct prnprinter *pcur;
  struct PrintDialog *d;
  const char *s;
  char *driver, *option, *prn;

  d = (struct PrintDialog *) data;

  if (d->ret == IDCANCEL)
    return;

  a = combo_box_get_active(d->driver);
  pcur = Menulocal.prnprinterroot;

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
  if (s && strlen(s) > 0) {
    option = nstrdup(s);
    if (option) {
      putobj(d->Obj, "option", d->Id, option);
    }
  }

  s = gtk_entry_get_text(GTK_ENTRY(d->print));
  if (s && strlen(s) > 0) {
    prn = nstrdup(s);
    if (prn) {
      putobj(d->Obj, "prn", d->Id, prn);
    }
  }
}

void
PrintDialog(struct PrintDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = PrintDialogSetup;
  data->CloseWindow = PrintDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
OutputDataDialogSetupItem(GtkWidget *w, struct OutputDataDialog *d)
{
  char buf[256];

  snprintf(buf, sizeof(buf), "%d", d->div);
  gtk_entry_set_text(GTK_ENTRY(d->div_entry), buf);
}

static void
OutputDataDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct OutputDataDialog *d;

  d = (struct OutputDataDialog *) data;
  if (makewidget) {
    w = create_text_entry(FALSE, TRUE);
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
  int a;
  const char *buf;
  char *endptr;

  d = (struct OutputDataDialog *) data;

  if (d->ret != IDOK)
    return;

  buf = gtk_entry_get_text(GTK_ENTRY(d->div_entry));
  a = strtol(buf, &endptr, 10);

  if (endptr[0] == '\0')
    d->div = a;
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
    combo_box_set_active(d->version, 0);

    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);

    gtk_widget_set_sensitive(d->dlabel, TRUE);
    gtk_widget_set_sensitive(d->dpi, TRUE);
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

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->dpi), Mxlocal->windpi);
    break;
  case MenuIdOutputPDFFile:
    combo_box_append_text(d->version, "--------");

    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);
    gtk_widget_set_sensitive(d->version, FALSE);
    gtk_widget_set_sensitive(d->vlabel, FALSE);

    gtk_widget_set_sensitive(d->t2p, TRUE);
    gtk_widget_set_sensitive(d->dlabel, TRUE);
    gtk_widget_set_sensitive(d->dpi, TRUE);
    break;
  case MenuIdOutputSVGFile:
    for (i = 0; PsVersion[i]; i++) {
      combo_box_append_text(d->version, SvgVersion[i]);
    }
    combo_box_set_active(d->version, 0);

    gtk_widget_set_sensitive(d->dlabel, FALSE);
    gtk_widget_set_sensitive(d->dpi, FALSE);

    gtk_widget_set_sensitive(d->version, TRUE);
    gtk_widget_set_sensitive(d->vlabel, TRUE);
    gtk_widget_set_sensitive(d->t2p, TRUE);

    gtk_label_set_markup_with_mnemonic(GTK_LABEL(d->vlabel), _("_SVG Version:"));
    break;
  }
}

static void
OutputImageDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
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
    d->dlabel = item_setup(d->vbox, w, "DPI:", FALSE);

    w = combo_box_create();
    d->version = w;
    d->vlabel = item_setup(d->vbox, w, "", FALSE);
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


void
CmOutputDriver(int print)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *device, *g2winst;
  int GC;
  struct narray *drawrable;
  int i, ret;
  struct savedstdio stdio;

  if (Menulock || GlobalLock)
    return;

  if (! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if ((graobj = chkobject("gra")) == NULL)
    return;

  if ((g2wobj = chkobject("gra2prn")) == NULL)
    return;

  g2wid = newobj(g2wobj);
  if (g2wid < 0)
    return;

  if (print) {
    PrintDialog(&DlgPrinter, g2wobj, g2wid);
    ret = DialogExecute(TopLevel, &DlgPrinter);
  } else {
    DriverDialog(&DlgDriver, g2wobj, g2wid);
    ret = DialogExecute(TopLevel, &DlgDriver);
  }

  if (ret == IDOK) {
    SetStatusBar(_("Spawning external driver."));
    g2winst = chkobjinst(g2wobj, g2wid);
    _getobj(g2wobj, "oid", g2winst, &g2woid);
    id = newobj(graobj);
    putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
    putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
    putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
    putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
    putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));
    if (arraynum(&(Menulocal.drawrable)) > 0) {
      drawrable = arraynew(sizeof(char *));
      for (i = 0; i < arraynum(&(Menulocal.drawrable)); i++) {
	arrayadd2(drawrable,
		  (char **) arraynget(&(Menulocal.drawrable), i));
      }
    } else
      drawrable = NULL;
    putobj(graobj, "draw_obj", id, drawrable);
    device = (char *) memalloc(DEVICE_BUF_SIZE);
    snprintf(device, DEVICE_BUF_SIZE, "gra2prn:^%d", g2woid);
    putobj(graobj, "device", id, device);
    ProgressDialogCreate(_("Printing"));
    SetStatusBar(_("Printing."));
    ignorestdio(&stdio);
    getobj(graobj, "open", id, 0, NULL, &GC);
    exeobj(graobj, "draw", id, 0, NULL);
    exeobj(graobj, "flush", id, 0, NULL);
    exeobj(graobj, "close", id, 0, NULL);
    restorestdio(&stdio);
    delobj(graobj, id);
    ProgressDialogFinalize();
    ResetStatusBar();
  }
  delobj(g2wobj, g2wid);
}

void
CmOutputViewer(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *device, *g2winst;
  int GC, delgra;
  struct narray *drawrable;
  int i;

  if (Menulock || GlobalLock)
    return;

  if (! SetFileHidden())
    return;

  FileAutoScale();
  AdjustAxis();

  if ((graobj = chkobject("gra")) == NULL)
    return;

  if ((g2wobj = chkobject("gra2gtk")) == NULL)
    return;

  g2wid = newobj(g2wobj);

  if (g2wid < 0)
    return;

  SetStatusBar(_("Spawning external viewer."));

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  putobj(g2wobj, "dpi", g2wid, &(Menulocal.exwindpi));
  putobj(g2wobj, "store_in_memory", g2wid,
	 &(Menulocal.exwinbackingstore));
  putobj(g2wobj, "BR", g2wid, &(Menulocal.bg_r));
  putobj(g2wobj, "BG", g2wid, &(Menulocal.bg_g));
  putobj(g2wobj, "BB", g2wid, &(Menulocal.bg_b));
  id = newobj(graobj);
  putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
  putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
  putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
  putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
  putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));
  if (arraynum(&(Menulocal.drawrable)) > 0) {
    drawrable = arraynew(sizeof(char *));
    for (i = 0; i < arraynum(&(Menulocal.drawrable)); i++) {
      arrayadd2(drawrable,
		(char **) arraynget(&(Menulocal.drawrable), i));
    }
  } else {
    drawrable = NULL;
  }
  putobj(graobj, "draw_obj", id, drawrable);
  device = (char *) memalloc(DEVICE_BUF_SIZE);
  snprintf(device, DEVICE_BUF_SIZE, "gra2gtk:^%d", g2woid);
  putobj(graobj, "device", id, device);
  getobj(graobj, "open", id, 0, NULL, &GC);
  exeobj(graobj, "draw", id, 0, NULL);
  exeobj(graobj, "flush", id, 0, NULL);
  delgra = TRUE;
  _putobj(g2wobj, "delete_gra", g2winst, &delgra);

  ProgressDialogFinalize();
  ResetStatusBar();
}

void
CmPrintGRAFile(void)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *device, *g2winst;
  int GC;
  char *file, buf[MESSAGE_BUF_SIZE];
  struct narray *drawrable;
  int i;
  char *filebuf;

  if (Menulock || GlobalLock)
    return;

  if (nGetSaveFileName(TopLevel, _("GRA file"), "gra", NULL, NULL,
		       &filebuf, "*.gra", Menulocal.changedirectory) != IDOK)
    return;

  if (access(filebuf, 04) == 0) {
    snprintf(buf, sizeof(buf), _("`%s'\n\nOverwrite existing file?"), filebuf);
    if (MessageBox(TopLevel, buf, _("GRA file"), MB_YESNO) != IDYES) {
      free(filebuf);
      return;
    }
  }

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

  SetStatusBar(_("Making GRA file."));

  g2winst = chkobjinst(g2wobj, g2wid);
  _getobj(g2wobj, "oid", g2winst, &g2woid);
  putobj(g2wobj, "file", g2wid, file);
  id = newobj(graobj);
  putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
  putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
  putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
  putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
  putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));

  if (arraynum(&(Menulocal.drawrable)) > 0) {
    drawrable = arraynew(sizeof(char *));
    for (i = 0; i < arraynum(&(Menulocal.drawrable)); i++) {
      arrayadd2(drawrable,
		(char **) arraynget(&(Menulocal.drawrable), i));
    }
  } else {
    drawrable = NULL;
  }
  putobj(graobj, "draw_obj", id, drawrable);

  device = (char *) memalloc(DEVICE_BUF_SIZE);
  snprintf(device, DEVICE_BUF_SIZE, "gra2file:^%d", g2woid);
  putobj(graobj, "device", id, device);
  getobj(graobj, "open", id, 0, NULL, &GC);
  exeobj(graobj, "draw", id, 0, NULL);
  exeobj(graobj, "flush", id, 0, NULL);
  exeobj(graobj, "close", id, 0, NULL);
  delobj(graobj, id);
  delobj(g2wobj, g2wid);

  ProgressDialogFinalize();
  ResetStatusBar();
}

void
CmOutputImage(int type)
{
  struct objlist *graobj, *g2wobj;
  int id, g2wid, g2woid;
  char *device, *g2winst;
  int GC;
  struct narray *drawrable;
  int i, ret, format, t2p, dpi;
  struct savedstdio stdio;
  char *ext_name, *ext_str, *ext;
  char *file, *filebuf, buf[MESSAGE_BUF_SIZE];

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
  }

  if (nGetSaveFileName(TopLevel, ext_name, ext_str, NULL, NULL,
		       &filebuf, ext, Menulocal.changedirectory) != IDOK)
    return;

  if (access(filebuf, 04) == 0) {
    snprintf(buf, sizeof(buf), _("`%s'\n\nOverwrite existing file?"), filebuf);
    if (MessageBox(TopLevel, buf, _("confirm"), MB_YESNO) != IDYES) {
      free(filebuf);
      return;
    }
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

  if ((graobj = chkobject("gra")) == NULL) {
    memfree(file);
    return;
  }

  if ((g2wobj = chkobject("gra2cairofile")) == NULL) {
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

  putobj(graobj, "paper_width", id, &(Menulocal.PaperWidth));
  putobj(graobj, "paper_height", id, &(Menulocal.PaperHeight));
  putobj(graobj, "left_margin", id, &(Menulocal.LeftMargin));
  putobj(graobj, "top_margin", id, &(Menulocal.TopMargin));
  putobj(graobj, "zoom", id, &(Menulocal.PaperZoom));
  if (arraynum(&(Menulocal.drawrable)) > 0) {
    drawrable = arraynew(sizeof(char *));
    for (i = 0; i < arraynum(&(Menulocal.drawrable)); i++) {
      arrayadd2(drawrable,
		(char **) arraynget(&(Menulocal.drawrable), i));
    }
  } else {
    drawrable = NULL;
  }
  putobj(graobj, "draw_obj", id, drawrable);
  device = (char *) memalloc(DEVICE_BUF_SIZE);
  snprintf(device, DEVICE_BUF_SIZE, "gra2cairofile:^%d", g2woid);
  putobj(graobj, "device", id, device);
  ProgressDialogCreate(_("Drawing"));
  SetStatusBar(_("Drawing."));
  ignorestdio(&stdio);
  getobj(graobj, "open", id, 0, NULL, &GC);
  exeobj(graobj, "draw", id, 0, NULL);
  exeobj(graobj, "flush", id, 0, NULL);
  exeobj(graobj, "close", id, 0, NULL);
  restorestdio(&stdio);
  delobj(graobj, id);
  ProgressDialogFinalize();
  ResetStatusBar();

  delobj(g2wobj, g2wid);
}

void
CmPrintDataFile(void)
{
  struct objlist *obj;
  int id;
  int type, div;
  char buf[MESSAGE_BUF_SIZE], *file;
  char *argv[3];

  if (Menulock || GlobalLock)
    return;

  obj = chkobject("file");
  if (obj == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  CopyDialog(&DlgCopy, obj, -1, FileCB);
  if (DialogExecute(TopLevel, &DlgCopy) != IDOK)
    return;

  id = DlgCopy.sel;
  if (id < 0)
    return;

  if (id > chkobjlastinst(obj))
    return;

  div = 10;
  getobj(obj, "type", id, 0, NULL, &type);
  if (type == 3) {
    OutputDataDialog(&DlgOutputData, div);
    if (DialogExecute(TopLevel, &DlgOutputData) != IDOK)
      return;
    div = DlgOutputData.div;
  }

  if (nGetSaveFileName(TopLevel, _("Data file"), NULL, NULL, NULL,
		       &file, "*", Menulocal.changedirectory) != IDOK)
    return;

  if (access(file, 04) == 0) {
    snprintf(buf, sizeof(buf), _("`%s'\n\nOverwrite existing file?"), file);
    if (MessageBox(TopLevel, buf, _("Data file"), MB_YESNO) != IDYES) {
      free(file);
      return;
    }
  }

  ProgressDialogCreate(_("Making data file"));
  SetStatusBar(_("Making data file."));
  argv[0] = (char *) file;
  argv[1] = (char *) &div;
  argv[2] = NULL;
  exeobj(obj, "output_file", id, 2, argv);
  ProgressDialogFinalize();
  ResetStatusBar();

  free(file);
}

void
CmOutputDriverB(GtkWidget *wi, gpointer client_data)
{
  CmOutputDriver(TRUE);
}

void
CmOutputViewerB(GtkWidget *wi, gpointer client_data)
{
  CmOutputViewer();
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
    CmOutputViewer();
    break;
  case MenuIdOutputDriver:
    CmOutputDriver(FALSE);
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
