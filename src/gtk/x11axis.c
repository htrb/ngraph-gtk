/* 
 * $Id: x11axis.c,v 1.10 2008/06/12 09:04:25 hito Exp $
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
#include "object.h"
#include "nstring.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11graph.h"
#include "x11file.h"
#include "x11view.h"
#include "x11axis.h"
#include "x11commn.h"

static n_list_store Alist[] = {
  {"",     G_TYPE_BOOLEAN, TRUE, FALSE, "hidden"},
  {"#",    G_TYPE_INT,     TRUE, FALSE, "id"},
  {"name", G_TYPE_STRING,  TRUE, FALSE, "group"},
  {"min",  G_TYPE_STRING,  TRUE, FALSE, "min"},
  {"max",  G_TYPE_STRING,  TRUE, FALSE, "max"},
  {"inc",  G_TYPE_STRING,  TRUE, FALSE, "inc"},
  {"type", G_TYPE_STRING,  TRUE, FALSE, "type"},
  {"x",    G_TYPE_INT,     TRUE, FALSE, "x"},
  {"y",    G_TYPE_INT,     TRUE, FALSE, "y"},
  {"dir",  G_TYPE_INT,     TRUE, FALSE, "direction"},
  {"len",  G_TYPE_INT,     TRUE, FALSE, "length"},
  {"^#",   G_TYPE_INT,     TRUE, FALSE, "oid"},
};

#define AXIS_WIN_COL_NUM (sizeof(Alist)/sizeof(*Alist))
#define AXIS_WIN_COL_OID (AXIS_WIN_COL_NUM - 1)
#define AXIS_WIN_COL_ID 1

static struct subwin_popup_list Popup_list[] = {
  {N_("_Focus"),          G_CALLBACK(list_sub_window_focus), FALSE, NULL},
  {GTK_STOCK_PROPERTIES,  G_CALLBACK(list_sub_window_update), TRUE, NULL},
  {GTK_STOCK_DELETE,      G_CALLBACK(list_sub_window_delete), TRUE, NULL},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(list_sub_window_move_top), TRUE, NULL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(list_sub_window_move_up), TRUE, NULL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(list_sub_window_move_down), TRUE, NULL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(list_sub_window_move_last), TRUE, NULL},
  {N_("_Duplicate"),      G_CALLBACK(list_sub_window_copy), FALSE, NULL},
  {N_("_Hide"),            G_CALLBACK(list_sub_window_hide), FALSE, NULL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))

#define CB_BUF_SIZE 128
#define ID_BUF_SIZE 16
#define TITLE_BUF_SIZE 128 

static gboolean AxisWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data);
static void axis_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row);

char *
AxisCB(struct objlist *obj, int id)
{
  char *s;
  int dir;
  char *valstr;
  char *name;

  if ((s = (char *) memalloc(CB_BUF_SIZE)) == NULL)
    return NULL;
  getobj(obj, "direction", id, 0, NULL, &dir);
  getobj(obj, "group", id, 0, NULL, &name);
  if (name == NULL)
    name = "";
  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);
  snprintf(s, CB_BUF_SIZE, "%-5d %-10s %.6s dir:%d", id, name, valstr, dir);
  memfree(valstr);
  return s;
}

static char *
GridCB(struct objlist *obj, int id)
{
  char *s, *s1, *s2;

  if ((s = (char *) memalloc(CB_BUF_SIZE)) == NULL)
    return NULL;
  getobj(obj, "axis_x", id, 0, NULL, &s1);
  getobj(obj, "axis_y", id, 0, NULL, &s2);
  snprintf(s, CB_BUF_SIZE, "%-5d %.8s %.8s", id, (s1)? s1: "-----", (s2)? s2: "-----");
  return s;
}

static void
GridDialogSetupItem(GtkWidget *w, struct GridDialog *d, int id)
{
  char *valstr;
  int i, j;
  int lastinst;
  struct objlist *aobj;
  char *name;

  aobj = getobject("axis");
  lastinst = chkobjlastinst(aobj);

  combo_box_clear(d->axisx);
  combo_box_clear(d->axisy);

  for (j = 0; j <= lastinst; j++) {
    getobj(aobj, "group", j, 0, NULL, &name);
    if (name == NULL)
      name = "";

    combo_box_append_text(d->axisx, name);
    combo_box_append_text(d->axisy, name);
  }

  sgetobjfield(d->Obj, id, "axis_x", NULL, &valstr, FALSE, FALSE, FALSE);

  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->axisx, valstr + i);
  memfree(valstr);

  sgetobjfield(d->Obj, id, "axis_y", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->axisy, valstr + i);
  memfree(valstr);

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1"; 

    style[sizeof(style) - 2] += i;
    SetStyleFromObjField(d->style[i], d->Obj, id, style);

    width[sizeof(width) - 2] += i;
    SetComboList(d->width[i], CbLineWidth, CbLineWidthNum);
    SetTextFromObjField(GTK_BIN(d->width[i])->child, d->Obj, id, width);
  }
  SetToggleFromObjField(d->background, d->Obj, id, "background");

  set_color(d->color, d->Obj, id, NULL);
  set_color(d->bcolor, d->Obj, id, "B");
}

static void
GridDialogCopy(struct GridDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, GridCB)) != -1)
    GridDialogSetupItem(d->widget, d, sel);
}

static void
GridDialogAxis(GtkWidget *w, gpointer client_data)
{
  struct GridDialog *d;
  char buf[10];
  int a, oid;
  struct objlist *aobj;

  aobj = getobject("axis");
  d = (struct GridDialog *) client_data;
  a = combo_box_get_active(w);
  if (a < 0)
    return;
  getobj(aobj, "oid", a, 0, NULL, &oid);
  snprintf(buf, sizeof(buf), "^%d", oid);
  combo_box_entry_set_text(w, buf);
}

static void
gauge_syle_setup(struct GridDialog *d, GtkWidget *box, int n)
{
  GtkWidget *hbox, *w;
  char buf[TITLE_BUF_SIZE]; 

  if (n < 0 || n >= GRID_DIALOG_STYLE_NUM)
    return;

  hbox = gtk_hbox_new(FALSE, 4);

  snprintf(buf, sizeof(buf), _("_Style %d:"), n + 1);
  w = combo_box_entry_create();
  item_setup(hbox, w, buf, TRUE);
  d->style[n] = w;

  snprintf(buf, sizeof(buf), _("_Width %d:"), n + 1);
  w = combo_box_entry_create();
  item_setup(hbox, w, buf, TRUE);
  d->width[n] = w;

  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 4);
}

static void
GridDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *frame, *w, *vbox, *hbox;
  struct GridDialog *d;
  char title[TITLE_BUF_SIZE];
  int i;

  d = (struct GridDialog *) data;
  snprintf(title, sizeof(title), _("Grid %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  d = (struct GridDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_entry_create();
    item_setup(hbox, w, _("Axis (_X):"), FALSE);
    g_signal_connect(w, "changed", G_CALLBACK(GridDialogAxis), d);
    d->axisx = w;

    w = combo_box_entry_create();
    item_setup(hbox, w, _("Axis (_Y):"), FALSE);
    g_signal_connect(w, "changed", G_CALLBACK(GridDialogAxis), d);
    d->axisy = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
      gauge_syle_setup(d, vbox, i);
    }

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_color_button(wi);
    item_setup(hbox, w, _("_Color:"), FALSE);
    d->color = w;

    w = gtk_check_button_new_with_mnemonic(_("_Background"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->background = w;

    w = create_color_button(wi);
    item_setup(hbox, w, _("_Background Color:"), FALSE);
    d->bcolor = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  GridDialogSetupItem(wi, d, d->Id);
}

static void
GridDialogClose(GtkWidget *w, void *data)
{
  struct GridDialog *d;
  int ret;
  const char *s;
  char *buf;
  int len, i;

  d = (struct GridDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    GridDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  s = combo_box_entry_get_text(d->axisx);
  if ((s == NULL) || (strlen(s) == 0)) {
    if (sputobjfield(d->Obj, d->Id, "axis_x", NULL) != 0)
      return;
  } else {
    len = strlen(s) + 6;
    if ((buf = (char *) memalloc(len)) != NULL) {
      snprintf(buf, len, "axis:%s", (s)? s: "");
      if (sputobjfield(d->Obj, d->Id, "axis_x", buf) != 0) {
	memfree(buf);
	return;
      }
      memfree(buf);
    }
  }

  s = combo_box_entry_get_text(d->axisy);
  if ((s == NULL) || (strlen(s) == 0)) {
    if (sputobjfield(d->Obj, d->Id, "axis_y", NULL) != 0)
      return;
  } else {
    len = strlen(s) + 6;
    if ((buf = (char *) memalloc(len)) != NULL) {
      strcpy(buf, "axis:");
      snprintf(buf, len, "axis:%s", (s)? s: "");
      if (sputobjfield(d->Obj, d->Id, "axis_y", buf) != 0) {
	memfree(buf);
	return;
      }
      memfree(buf);
    }
  }

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1"; 

    style[sizeof(style) - 2] += i;
    if (SetObjFieldFromStyle(d->style[i], d->Obj, d->Id, style))
      return;

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromText(GTK_BIN(d->width[i])->child, d->Obj, d->Id, width))
      return;
  }

  if (SetObjFieldFromToggle(d->background, d->Obj, d->Id, "background"))
    return;

  if (putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

  if (putobj_color(d->bcolor, d->Obj, d->Id, "B"))
    return;

  d->ret = ret;
}

void
GridDialog(struct GridDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = GridDialogSetup;
  data->CloseWindow = GridDialogClose;
  data->Obj = obj;
  data->Id = id;
}


static void 
set_axis_id(GtkWidget *w, int id)
{
  char buf[256];

  if (id != -1) {
    snprintf(buf, sizeof(buf), "id:%d", id);
  } else {
    buf[0] = '\0';
  }
  gtk_label_set_text(GTK_LABEL(w), buf);
}

static void
SectionDialogSetupItem(GtkWidget *w, struct SectionDialog *d)
{
  char buf[256];

  snprintf(buf, sizeof(buf), "%d", d->X);
  gtk_entry_set_text(GTK_ENTRY(d->x), buf);

  snprintf(buf, sizeof(buf), "%d", d->Y);
  gtk_entry_set_text(GTK_ENTRY(d->y), buf);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);
  set_axis_id(d->uid, d->IDU);
  set_axis_id(d->rid, d->IDR);
  set_axis_id(d->gid, *(d->IDG));

  snprintf(buf, sizeof(buf), "%d", d->LenX);
  gtk_entry_set_text(GTK_ENTRY(d->width), buf);

  snprintf(buf, sizeof(buf), "%d", d->LenY);
  gtk_entry_set_text(GTK_ENTRY(d->height), buf);
}

static void
SectionDialogAxisX(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDX >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDX, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisY(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDY >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDY, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisU(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDU >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDU, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisR(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDR >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDR, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogGrid(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;
  char *ref;
  int oidx, oidy;

  d = (struct SectionDialog *) client_data;
  if (*(d->IDG) == -1) {
    if ((*(d->IDG) = newobj(d->Obj2)) >= 0) {
      if ((ref = (char *) memalloc(ID_BUF_SIZE)) != NULL) {
	getobj(d->Obj, "oid", d->IDX, 0, NULL, &oidx);
	snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
	putobj(d->Obj2, "axis_x", *(d->IDG), ref);
      }
      if ((ref = (char *) memalloc(ID_BUF_SIZE)) != NULL) {
	getobj(d->Obj, "oid", d->IDY, 0, NULL, &oidy);
	snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidy);
	putobj(d->Obj2, "axis_y", *(d->IDG), ref);
      }
    }
  }
  if (*(d->IDG) >= 0) {
    GridDialog(&DlgGrid, d->Obj2, *(d->IDG));
    if (DialogExecute(d->widget, &DlgGrid) == IDDELETE) {
      delobj(d->Obj2, *(d->IDG));
      *(d->IDG) = -1;
    }
  }
  SectionDialogSetupItem(d->widget, d);
  NgraphApp.Changed = TRUE;
}


static void
SectionDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct SectionDialog *d;

  d = (struct SectionDialog *) data;

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, "_X:", FALSE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, "_Y:", FALSE);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("Graph _Width:"), FALSE);
    d->width = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("Graph _Height:"), FALSE);
    d->height = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_X axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisX), d);
    d->xaxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->xid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Y axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisY), d);
    d->yaxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->yid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_U axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisU), d);
    d->uaxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->uid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_R axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisR), d);
    d->raxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->rid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Grid"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogGrid), d);
    d->grid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->gid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  SectionDialogSetupItem(wi, d);
}

static void
SectionDialogClose(GtkWidget *w, void *data)
{
  struct SectionDialog *d;
  int ret;
  int type, a;
  const char *buf;
  char *endptr;
  struct narray group;
  char *argv[2];

  d = (struct SectionDialog *) data;
  if (d->ret != IDOK)
    return;
  ret = d->ret;

  d->ret = IDLOOP;
  buf = gtk_entry_get_text(GTK_ENTRY(d->x));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->X = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->y));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->Y = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->width));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->LenX = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->height));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->LenY = a;

  if ((d->X != d->X0) || (d->Y != d->Y0)
      || (d->LenX0 != d->LenX) || (d->LenY0 != d->LenY)) {
    arrayinit(&group, sizeof(int));
    if (d->Section)
      type = 2;
    else
      type = 1;
    arrayadd(&group, &type);
    arrayadd(&group, &(d->IDX));
    arrayadd(&group, &(d->IDY));
    arrayadd(&group, &(d->IDU));
    arrayadd(&group, &(d->IDR));
    arrayadd(&group, &(d->X));
    arrayadd(&group, &(d->Y));
    arrayadd(&group, &(d->LenX));
    arrayadd(&group, &(d->LenY));
    argv[0] = (char *) &group;
    argv[1] = NULL;
    exeobj(d->Obj, "group_position", d->IDX, 1, argv);
    arraydel(&group);
  }
  d->ret = ret;
}

void
SectionDialog(struct SectionDialog *data,
	      int x, int y, int lenx, int leny,
	      struct objlist *obj, int idx, int idy, int idu, int idr,
	      struct objlist *obj2, int *idg, int section)
{
  data->SetupWindow = SectionDialogSetup;
  data->CloseWindow = SectionDialogClose;
  data->X0 = data->X = x;
  data->Y0 = data->Y = y;
  data->LenX0 = data->LenX = lenx;
  data->LenY0 = data->LenY = leny;
  data->Obj = obj;
  data->Obj2 = obj2;
  data->IDX = idx;
  data->IDY = idy;
  data->IDU = idu;
  data->IDR = idr;
  data->IDG = idg;
  data->Section = section;
  data->MaxX = Menulocal.PaperWidth * (10000.0 / Menulocal.PaperZoom);
  data->MaxY = Menulocal.PaperHeight * (10000.0 / Menulocal.PaperZoom);
}

static void
CrossDialogSetupItem(GtkWidget *w, struct CrossDialog *d)
{
  char buf[256];

  snprintf(buf, sizeof(buf), "%d", d->X);
  gtk_entry_set_text(GTK_ENTRY(d->x), buf);

  snprintf(buf, sizeof(buf), "%d", d->Y);
  gtk_entry_set_text(GTK_ENTRY(d->y), buf);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);

  snprintf(buf, sizeof(buf), "%d", d->LenX);
  gtk_entry_set_text(GTK_ENTRY(d->width), buf);

  snprintf(buf, sizeof(buf), "%d", d->LenY);
  gtk_entry_set_text(GTK_ENTRY(d->height), buf);
}

static void
CrossDialogAxisX(GtkWidget *w, gpointer client_data)
{
  struct CrossDialog *d;

  d = (struct CrossDialog *) client_data;
  if (d->IDX >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDX, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
CrossDialogAxisY(GtkWidget *w, gpointer client_data)
{
  struct CrossDialog *d;

  d = (struct CrossDialog *) client_data;
  if (d->IDY >= 0) {
    AxisDialog(&DlgAxis, d->Obj, d->IDY, FALSE);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
CrossDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct CrossDialog *d;

  d = (struct CrossDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, "_X:", FALSE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, "_Y:", FALSE);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("Graph _Width:"), FALSE);
    d->width = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("Graph _Height:"), FALSE);
    d->height = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_X axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(CrossDialogAxisX), d);
    d->xaxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->xid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Y axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(CrossDialogAxisY), d);
    d->yaxis = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_label_new(NULL);
    d->yid = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }
  CrossDialogSetupItem(wi, d);
}

static void
CrossDialogClose(GtkWidget *w, void *data)
{
  struct CrossDialog *d;
  int ret;
  int type, a;
  const char *buf;
  char *endptr;
  struct narray group;
  char *argv[2];

  d = (struct CrossDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;

  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->x));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->X = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->y));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->Y = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->width));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->LenX = a;

  buf = gtk_entry_get_text(GTK_ENTRY(d->height));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') {
    return;
  }
  d->LenY = a;

  if ((d->X != d->X0) || (d->Y != d->Y0)
      || (d->LenX != d->LenX0) || (d->LenY != d->LenY0)) {
    arrayinit(&group, sizeof(int));
    type = 3;
    arrayadd(&group, &type);
    arrayadd(&group, &(d->IDX));
    arrayadd(&group, &(d->IDY));
    arrayadd(&group, &(d->X));
    arrayadd(&group, &(d->Y));
    arrayadd(&group, &(d->LenX));
    arrayadd(&group, &(d->LenY));
    argv[0] = (char *) &group;
    argv[1] = NULL;
    exeobj(d->Obj, "group_position", d->IDX, 1, argv);
    arraydel(&group);
  }
  if ((d->IDX != -1) && (d->IDY != -1)) {
    exeobj(d->Obj, "adjust", d->IDX, 0, NULL);
    exeobj(d->Obj, "adjust", d->IDY, 0, NULL);
  }
  d->ret = ret;
}

void
CrossDialog(struct CrossDialog *data,
	    int x, int y, int lenx, int leny,
	    struct objlist *obj, int idx, int idy)
{
  data->SetupWindow = CrossDialogSetup;
  data->CloseWindow = CrossDialogClose;
  data->X0 = data->X = x;
  data->Y0 = data->Y = y;
  data->LenX0 = data->LenX = lenx;
  data->LenY0 = data->LenY = leny;
  data->Obj = obj;
  data->IDX = idx;
  data->IDY = idy;
  data->MaxX = Menulocal.PaperWidth * (10000.0 / Menulocal.PaperZoom);
  data->MaxY = Menulocal.PaperHeight * (10000.0 / Menulocal.PaperZoom);
}

static void
ZoomDialogSetupItem(GtkWidget *w, struct ZoomDialog *d)
{
  char buf[TITLE_BUF_SIZE];

  snprintf(buf, sizeof(buf), "%d", d->zoom);
  gtk_entry_set_text(GTK_ENTRY(d->zoom_entry), buf);
}

static void
ZoomDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *vbox;
  struct ZoomDialog *d;

  d = (struct ZoomDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("_Zoom:"), TRUE);
    d->zoom_entry = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }
  ZoomDialogSetupItem(wi, d);
}

static void
ZoomDialogClose(GtkWidget *w, void *data)
{
  struct ZoomDialog *d;
  int ret, a;
  const char *buf;
  char *endptr;

  d = (struct ZoomDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;

  d->ret = IDLOOP;

  buf = gtk_entry_get_text(GTK_ENTRY(d->zoom_entry));
  a = strtol(buf, &endptr, 10);
  if (endptr[0] == '\0') {
    d->zoom = a;
  } else {
    return;
  }
  d->ret = ret;
}

void
ZoomDialog(struct ZoomDialog *data)
{
  data->SetupWindow = ZoomDialogSetup;
  data->CloseWindow = ZoomDialogClose;
  data->zoom = 20000;
}

static void
AxisBaseDialogSetupItem(GtkWidget *w, struct AxisBaseDialog *d, int id)
{
  SetStyleFromObjField(d->style, d->Obj, id, "style");

  SetComboList(d->width, CbLineWidth, CbLineWidthNum);
  SetTextFromObjField(GTK_BIN(d->width)->child, d->Obj, id, "width");

  SetToggleFromObjField(d->baseline, d->Obj, id, "baseline");

  SetListFromObjField(d->arrow, d->Obj, id, "arrow");

  SetTextFromObjField(d->arrowlen, d->Obj, id, "arrow_length");

  SetTextFromObjField(d->arrowwid, d->Obj, id, "arrow_width");

  SetListFromObjField(d->wave, d->Obj, id, "wave");

  SetTextFromObjField(d->wavelen, d->Obj, id, "wave_length");

  SetComboList(d->wavewid, CbLineWidth, CbLineWidthNum);
  SetTextFromObjField(GTK_BIN(d->wavewid)->child, d->Obj, id, "wave_width");

  set_color(d->color, d->Obj, id, NULL);
}

static void
AxisBaseDialogCopy(struct AxisBaseDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    AxisBaseDialogSetupItem(d->widget, d, sel);
}


static void
AxisBaseDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct AxisBaseDialog *d;

  d = (struct AxisBaseDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_check_button_new_with_mnemonic(_("_Baseline:"));
    d->baseline = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
    item_setup(hbox, w, _("_Style:"), TRUE);
    d->style = w;

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
    item_setup(hbox, w, _("_Width:"), TRUE);
    d->width = w;

    w = create_color_button(wi);
    item_setup(hbox, w, _("_Color:"), FALSE);
    d->color = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Arrow:"), FALSE);
    d->arrow = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Arrow length:"), TRUE);
    d->arrowlen = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Arrow width:"), TRUE);
    d->arrowwid = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Wave:"), FALSE);
    d->wave = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Wave length:"), TRUE);
    d->wavelen = w;

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
    item_setup(hbox, w, _("_Wave width:"), FALSE);
    d->wavewid = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }
  AxisBaseDialogSetupItem(wi, d, d->Id);
}

static void
AxisBaseDialogClose(GtkWidget *w, void *data)
{
  struct AxisBaseDialog *d;
  int ret;

  d = (struct AxisBaseDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    AxisBaseDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "style"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->width)->child, d->Obj, d->Id, "width"))
    return;

  if (SetObjFieldFromToggle(d->baseline, d->Obj, d->Id, "baseline"))
    return;

  if (SetObjFieldFromList(d->arrow, d->Obj, d->Id, "arrow"))
    return;

  if (SetObjFieldFromText(d->arrowlen, d->Obj, d->Id, "arrow_length"))
    return;

  if (SetObjFieldFromText(d->arrowwid, d->Obj, d->Id, "arrow_width"))
    return;

  if (SetObjFieldFromList(d->wave, d->Obj, d->Id, "wave"))
    return;

  if (SetObjFieldFromText(d->wavelen, d->Obj, d->Id, "wave_length"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->wavewid)->child, d->Obj, d->Id, "wave_width"))
    return;

  if (putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

  d->ret = ret;
}

void
AxisBaseDialog(struct AxisBaseDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = AxisBaseDialogSetup;
  data->CloseWindow = AxisBaseDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
AxisPosDialogSetupItem(GtkWidget *w, struct AxisPosDialog *d, int id)
{
  char *valstr;
  int i, j;
  int lastinst;
  char *name;

  SetTextFromObjField(d->x, d->Obj, id, "x");

  SetTextFromObjField(d->y, d->Obj, id, "y");

  SetTextFromObjField(d->len, d->Obj, id, "length");

  SetComboList(d->direction, CbDirection, CbDirectionNum);
  SetTextFromObjField(GTK_BIN(d->direction)->child, d->Obj, id, "direction");

  lastinst = chkobjlastinst(d->Obj);
  combo_box_clear(d->adjust);
  for (j = 0; j <= lastinst; j++) {
    getobj(d->Obj, "group", j, 0, NULL, &name);
    if (name == NULL)
      name = "";
    combo_box_append_text(d->adjust, name);
  }

  sgetobjfield(d->Obj, id, "adjust_axis", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;

  combo_box_entry_set_text(d->adjust, valstr + i);
  memfree(valstr);

  SetTextFromObjField(d->adjustpos, d->Obj, id, "adjust_position");
}

static void
AxisPosDialogRef(GtkWidget *w, gpointer client_data)
{
  struct AxisPosDialog *d;
  char buf[10];
  int a, oid;

  d = (struct AxisPosDialog *) client_data;
  a = combo_box_get_active(w);
  if (a < 0)
    return;
  getobj(d->Obj, "oid", a, 0, NULL, &oid);
  snprintf(buf, sizeof(buf), "^%d", oid);
  combo_box_entry_set_text(w, buf);
}

static void
AxisPosDialogCopy(struct AxisPosDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    AxisPosDialogSetupItem(d->widget, d, sel);
}

static void
AxisPosDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct AxisPosDialog *d;

  d = (struct AxisPosDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, "_X:", TRUE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, "_Y:", TRUE);
    d->y = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Length:"), TRUE);
    d->len = w;

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 2, -1);
    item_setup(hbox, w, _("_Direction:"), FALSE);
    d->direction = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 2, -1);
    g_signal_connect(w, "changed", G_CALLBACK(AxisPosDialogRef), d);
    item_setup(hbox, w, _("_Adjust:"), FALSE);
    d->adjust = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("Adjust _Position:"), FALSE);
    d->adjustpos = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }

  AxisPosDialogSetupItem(wi, d, d->Id);
}

static void
AxisPosDialogClose(GtkWidget *w, void *data)
{
  struct AxisPosDialog *d;
  int ret;
  const char *s;
  char *buf;
  int len;

  d = (struct AxisPosDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    AxisPosDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromText(d->x, d->Obj, d->Id, "x"))
    return;

  if (SetObjFieldFromText(d->y, d->Obj, d->Id, "y"))
    return;

  if (SetObjFieldFromText(d->len, d->Obj, d->Id, "length"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->direction)->child,
			  d->Obj, d->Id, "direction"))
    return;

  s = combo_box_entry_get_text(d->adjust);
  if ((s == NULL) || (strlen(s) == 0)) {
    if (sputobjfield(d->Obj, d->Id, "adjust_axis", NULL) != 0)
      return;
  } else {
    len = strlen(s) + 6;
    if ((buf = (char *) memalloc(len)) != NULL) {
      snprintf(buf, len, "axis:%s", (s)? s: "");
      if (sputobjfield(d->Obj, d->Id, "adjust_axis", buf) != 0) {
	memfree(buf);
	return;
      }
      memfree(buf);
    }
  }

  if (SetObjFieldFromText(d->adjustpos, d->Obj, d->Id, "adjust_position"))
    return;

  d->ret = ret;
}

void
AxisPosDialog(struct AxisPosDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = AxisPosDialogSetup;
  data->CloseWindow = AxisPosDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
NumDialogSetupItem(GtkWidget *w, struct NumDialog *d, int id)
{
  char *format, *endptr;
  int j, a;

  SetListFromObjField(d->num, d->Obj, id, "num");

  SetTextFromObjField(d->begin, d->Obj, id, "num_begin");

  SetTextFromObjField(d->step, d->Obj, id, "num_step");

  SetTextFromObjField(d->numnum, d->Obj, id, "num_num");

  SetTextFromObjField(d->head, d->Obj, id, "num_head");

  combo_box_clear(d->fraction);
  for (j = 0; j < FwNumStyleNum; j++) {
    combo_box_append_text(d->fraction, _(FwNumStyle[j]));
  }

  getobj(d->Obj, "num_format", id, 0, NULL, &format);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->add_plus), strchr(format, '+') != NULL);

  if ((strchr(format, 'f') == NULL) || (strchr(format, '.') == NULL)) {
    a = 0;
  } else {
    a = strtol(strchr(format, '.') + 1, &endptr, 10) + 1;
  }

  if (a < 0) {
    a = 0;
  } else if (a > 10) {
    a = 10;
  }

  combo_box_set_active(d->fraction, a);

  SetTextFromObjField(d->tail, d->Obj, id, "num_tail");

  SetListFromObjField(d->align, d->Obj, id, "num_align");

  SetListFromObjField(d->direction, d->Obj, id, "num_direction");

  SetTextFromObjField(d->shiftp, d->Obj, id, "num_shift_p");

  SetTextFromObjField(d->shiftn, d->Obj, id, "num_shift_n");

  SetToggleFromObjField(d->log_power, d->Obj, id, "num_log_pow");

  SetToggleFromObjField(d->no_zero, d->Obj, id, "num_no_zero");

  SetTextFromObjField(d->norm, d->Obj, id, "num_auto_norm");
}

static void
NumDialogCopy(struct NumDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    NumDialogSetupItem(d->widget, d, sel);
}

static void
NumDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *frame;
  struct NumDialog *d;

  d = (struct NumDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Numbering:"), FALSE);
    d->num = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Begin:"), TRUE);
    d->begin = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Step:"), TRUE);
    d->step = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Num:"), TRUE);
    d->numnum = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Add plus"));
    d->add_plus = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);


    w = combo_box_create();
    item_setup(hbox, w, _("_Fraction:"), FALSE);
    d->fraction = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);


    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Head:"), TRUE);
    d->head = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Tail:"), TRUE);
    d->tail = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);


    w = combo_box_create();
    item_setup(hbox, w, _("_Align:"), FALSE);
    d->align = w;

    w = combo_box_create();
    item_setup(hbox, w, _("_Direction:"), FALSE);
    d->direction = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);


    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("shift (_P):"), TRUE);
    d->shiftp = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("shift (_N):"), TRUE);
    d->shiftn = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Log power"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->log_power = w;

    w = gtk_check_button_new_with_mnemonic(_("no _Zero"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->no_zero = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Auto normalization:"), TRUE);
    d->norm = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);
  }
  NumDialogSetupItem(wi, d, d->Id);
}


#define FORMAT_LENGTH 10
static void
NumDialogClose(GtkWidget *w, void *data)
{
  struct NumDialog *d;
  int ret;
  int a, j;
  char *format;

  d = (struct NumDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    NumDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromList(d->num, d->Obj, d->Id, "num"))
    return;

  if (SetObjFieldFromText(d->begin, d->Obj, d->Id, "num_begin"))
    return;

  if (SetObjFieldFromText(d->step, d->Obj, d->Id, "num_step"))
    return;

  if (SetObjFieldFromText(d->numnum, d->Obj, d->Id, "num_num"))
    return;

  if (SetObjFieldFromText(d->head, d->Obj, d->Id, "num_head"))
    return;

  format = (char *) memalloc(FORMAT_LENGTH);
  if (format == NULL)
    return;

  j = snprintf(format, FORMAT_LENGTH, "%%");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->add_plus)))
    j += snprintf(format + j, FORMAT_LENGTH - j, "%c", '+');

  a = combo_box_get_active(d->fraction);
  if (a == 0) {
    j += snprintf(format + j, FORMAT_LENGTH - j, "%c", 'g');
  } else if (a > 0) {
    j += snprintf(format + j, FORMAT_LENGTH - j, ".%df", a - 1);
  }
  if (putobj(d->Obj, "num_format", d->Id, format) == -1)
    return;

  if (SetObjFieldFromText(d->tail, d->Obj, d->Id, "num_tail"))
    return;

  if (SetObjFieldFromList(d->align, d->Obj, d->Id, "num_align"))
    return;

  if (SetObjFieldFromList(d->direction, d->Obj, d->Id, "num_direction"))
    return;

  if (SetObjFieldFromText(d->shiftp, d->Obj, d->Id, "num_shift_p"))
    return;

  if (SetObjFieldFromText(d->shiftn, d->Obj, d->Id, "num_shift_n"))
    return;

  if (SetObjFieldFromToggle(d->log_power, d->Obj, d->Id, "num_log_pow"))
    return;

  if (SetObjFieldFromToggle(d->no_zero, d->Obj, d->Id, "num_no_zero"))
    return;

  if (SetObjFieldFromText(d->norm, d->Obj, d->Id, "num_auto_norm"))
    return;

  d->ret = ret;
}

void
NumDialog(struct NumDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = NumDialogSetup;
  data->CloseWindow = NumDialogClose;
  data->Obj = obj;
  data->Id = id;
}



static void
AxisFontDialogSetupItem(GtkWidget *w, struct AxisFontDialog *d, int id)
{
  SetTextFromObjField(d->space, d->Obj, id, "num_space");

  SetComboList(d->pt, CbTextPt, CbTextPtNum);

  SetTextFromObjField(GTK_BIN(d->pt)->child, d->Obj, id, "num_pt");

  SetTextFromObjField(d->script, d->Obj, id, "num_script_size");

  SetFontListFromObj(d->font, d->Obj, d->Id, "num_font", FALSE);

#ifdef JAPANESE
  SetFontListFromObj(d->jfont, d->Obj, d->Id, "num_jfont", TRUE);
#endif

  set_color(d->color, d->Obj, id, "num_");
}

static void
AxisFontDialogCopy(struct AxisFontDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    AxisFontDialogSetupItem(d->widget, d, sel);
}

static void
AxisFontDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct AxisFontDialog *d;

  d = (struct AxisFontDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Point:"), TRUE);
    d->pt = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Space:"), TRUE);
    d->space = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Script size:"), TRUE);
    d->script = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    item_setup(hbox, w, _("_Font:"), FALSE);
    d->font = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

#ifdef JAPANESE
    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    item_setup(hbox, w, _("_Jfont:"), FALSE);
    d->jfont = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif
    
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_color_button(wi);
    item_setup(hbox, w, _("_Color:"), FALSE);
    d->color = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }
  AxisFontDialogSetupItem(wi, d, d->Id);
}

static void
AxisFontDialogClose(GtkWidget *w, void *data)
{
  struct AxisFontDialog *d;
  int ret;

  d = (struct AxisFontDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    AxisFontDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromText(d->space, d->Obj, d->Id, "num_space"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->pt)->child, d->Obj, d->Id, "num_pt"))
    return;

  if (SetObjFieldFromText(d->script, d->Obj, d->Id, "num_script_size"))
    return;

  SetObjFieldFromFontList(d->font, d->Obj, d->Id, "num_font", FALSE);

#ifdef JAPANESE
  SetObjFieldFromFontList(d->jfont, d->Obj, d->Id, "num_jfont", TRUE);
#endif

  if (putobj_color(d->color, d->Obj, d->Id, "num_"))
    return;

  d->ret = ret;
}

void
AxisFontDialog(struct AxisFontDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = AxisFontDialogSetup;
  data->CloseWindow = AxisFontDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
GaugeDialogSetupItem(GtkWidget *w, struct GaugeDialog *d, int id)
{
  int i;

  SetListFromObjField(d->gauge, d->Obj, id, "gauge");

  SetTextFromObjField(d->min, d->Obj, id, "gauge_min");

  SetTextFromObjField(d->max, d->Obj, id, "gauge_max");

  SetStyleFromObjField(d->style, d->Obj, id, "gauge_style");

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1"; 

    width[sizeof(width) - 2] += i;
    SetComboList(d->width[i], CbLineWidth, CbLineWidthNum);
    SetTextFromObjField(GTK_BIN(d->width[i])->child, d->Obj, id, width);

    length[sizeof(length) - 2] += i;
    SetTextFromObjField(d->length[i], d->Obj, id, length);
  }

  set_color(d->color, d->Obj, id, "gauge_");
}

static void
GaugeDialogCopy(struct GaugeDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    GaugeDialogSetupItem(d->widget, d, sel);
}

static void
GaugeDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *frame;
  struct GaugeDialog *d;
  char buf[TITLE_BUF_SIZE];
  int i;

  d = (struct GaugeDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Gauge:"), FALSE);
    d->gauge = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;

    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Style:"), TRUE);
    d->style = w;

    w = create_color_button(wi);
    item_setup(hbox, w, _("_Color:"), FALSE);
    d->color = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    for (i = 0; i < GAUGE_STYLE_NUM; i++) {
      hbox = gtk_hbox_new(FALSE, 4);

      snprintf(buf, sizeof(buf), _("_Width %d:"), i + 1);
      w = combo_box_entry_create();
      item_setup(hbox, w, buf, TRUE);
      d->width[i] = w;

      snprintf(buf, sizeof(buf), _("_Length %d:"), i + 1);
      w = create_text_entry(TRUE, TRUE);
      item_setup(hbox, w, buf, TRUE);
      d->length[i] = w;
      
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    }

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);
  }
  GaugeDialogSetupItem(wi, d, d->Id);
}


static void
GaugeDialogClose(GtkWidget *w, void *data)
{
  struct GaugeDialog *d;
  int ret, i;

  d = (struct GaugeDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    GaugeDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromList(d->gauge, d->Obj, d->Id, "gauge"))
    return;

  if (SetObjFieldFromText(d->min, d->Obj, d->Id, "gauge_min"))
    return;

  if (SetObjFieldFromText(d->max, d->Obj, d->Id, "gauge_max"))
    return;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "gauge_style"))
    return;

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1"; 

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromText(GTK_BIN(d->width[i])->child, d->Obj, d->Id, width))
      return;

    length[sizeof(length) - 2] += i;
    if (SetObjFieldFromText(d->length[i], d->Obj, d->Id, length))
      return;
  }

  if (putobj_color(d->color, d->Obj, d->Id, "gauge_"))
    return;

  d->ret = ret;
}


void
GaugeDialog(struct GaugeDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = GaugeDialogSetup;
  data->CloseWindow = GaugeDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
AxisDialogSetupItem(GtkWidget *w, struct AxisDialog *d, int id)
{
  char *valstr;
  int i, j;
  double min, max, inc;
  int div, lastinst;
  char *name;
  struct narray *array;
  int num;
  double *data;
  char buf[30];

  combo_box_clear(d->min);
  combo_box_clear(d->max);
  combo_box_clear(d->inc);

  getobj(d->Obj, "scale_history", d->Id, 0, NULL, &array);
  if (array != NULL) {
    num = arraynum(array) / 3;
    data = (double *) arraydata(array);
    for (j = 0; j < num; j++) {
      snprintf(buf, sizeof(buf), "%.15g", data[0 + j * 3]);
      combo_box_append_text(d->min, buf);

      snprintf(buf, sizeof(buf), "%.15g", data[1 + j * 3]);
      combo_box_append_text(d->max, buf);

      snprintf(buf, sizeof(buf), "%.15g", data[2 + j * 3]);
      combo_box_append_text(d->inc, buf);
    }
  }

  getobj(d->Obj, "min", id, 0, NULL, &min);
  getobj(d->Obj, "max", id, 0, NULL, &max);
  getobj(d->Obj, "inc", id, 0, NULL, &inc);

  if ((min == 0) && (max == 0) && (inc == 0)) {
    combo_box_entry_set_text(d->min, "0");
    combo_box_entry_set_text(d->max, "0");
    combo_box_entry_set_text(d->inc, "0");
  } else {
    snprintf(buf, sizeof(buf), "%.15g", min);
    combo_box_entry_set_text(d->min, buf);

    snprintf(buf, sizeof(buf), "%.15g", max);
    combo_box_entry_set_text(d->max, buf);

    snprintf(buf, sizeof(buf), "%.15g", inc);
    combo_box_entry_set_text(d->inc, buf);
  }

  getobj(d->Obj, "div", id, 0, NULL, &div);
  SetTextFromObjField(d->div, d->Obj, id, "div");
  SetListFromObjField(d->scale, d->Obj, id, "type");

  combo_box_clear(d->ref);
  lastinst = chkobjlastinst(d->Obj);
  for (j = 0; j <= lastinst; j++) {
    getobj(d->Obj, "group", j, 0, NULL, &name);
    if (name == NULL)
      name = "";
    combo_box_append_text(d->ref, name);
  }

  sgetobjfield(d->Obj, id, "reference", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->ref, valstr + i);
  memfree(valstr);
}

static void
AxisDialogGauge(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  GaugeDialog(&DlgGauge, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgGauge);
}


static void
AxisDialogBase(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  AxisBaseDialog(&DlgAxisBase, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgAxisBase);
}

static void
AxisDialogPos(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  AxisPosDialog(&DlgAxisPos, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgAxisPos);
}

static void
AxisDialogFont(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  AxisFontDialog(&DlgAxisFont, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgAxisFont);
}

static void
AxisDialogNum(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  NumDialog(&DlgNum, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgNum);
}

static void
AxisDialogClear(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  combo_box_entry_set_text(d->min, "0");
  combo_box_entry_set_text(d->max, "0");
  combo_box_entry_set_text(d->inc, "0");
}

static void
AxisDialogFile(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;
  int anum, room, type;
  char *buf, s[30];
  char *argv2[3];
  struct objlist *fobj;
  struct narray farray;
  int a, i, j, num, *array;
  struct narray *result;

  d = (struct AxisDialog *) client_data;

  if ((fobj = chkobject("file")) == NULL)
    return;

  if (chkobjlastinst(fobj) == -1)
    return;

  SelectDialog(&DlgSelect, fobj, FileCB, (struct narray *) &farray, NULL);

  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    anum = chkobjlastinst(d->Obj);

    if ((num > 0) && (anum != 0)) {
      int len;
      len = 6 * num + 6;
      buf = memalloc(len);;
      if (buf) {
	j = 0;
	j += snprintf(buf + j, len - j, "file:");

	for (i = 0; i < num; i++) {
	  if (i == num - 1) {
	    j += snprintf(buf + j, len -j, "%d", array[i]);
	  } else {
	    j += snprintf(buf + j, len - j, "%d,", array[i]);
	  }
	}

	room = 0;
	argv2[0] = (char *) buf;
	argv2[1] = (char *) &room;
	argv2[2] = NULL;

	a = combo_box_get_active(d->scale);

	if (a >= 0 && ((getobj(d->Obj, "type", d->Id, 0, NULL, &type) == -1)
		       || (putobj(d->Obj, "type", d->Id, &a) == -1))) {
	  arraydel(&farray);
	  return;
	}

	getobj(d->Obj, "get_auto_scale", d->Id, 2, argv2, &result);
	memfree(buf);

	if (arraynum(result) == 3) {
	  snprintf(s, sizeof(s), "%.15g", *(double *) arraynget(result, 0));
	  combo_box_entry_set_text(d->min, s);

	  snprintf(s, sizeof(s), "%.15g", *(double *) arraynget(result, 1));
	  combo_box_entry_set_text(d->max, s);

	  snprintf(s, sizeof(s), "%.15g", *(double *) arraynget(result, 2));
	  combo_box_entry_set_text(d->inc, s);
	}
	putobj(d->Obj, "type", d->Id, &type);
      }
    }
  }
  arraydel(&farray);
}

static void
AxisDialogRef(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;
  char buf[10];
  int a, oid;

  d = (struct AxisDialog *) client_data;

  a = combo_box_get_active(w);
  if (a < 0)
    return;

  getobj(d->Obj, "oid", a, 0, NULL, &oid);
  snprintf(buf, sizeof(buf), "^%d", oid);
  combo_box_entry_set_text(d->ref, buf);
}

static void
AxisDialogCopy(struct AxisDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB)) != -1)
    AxisDialogSetupItem(d->widget, d, sel);
}

static void
AxisDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *hbox2, *vbox, *frame;
  struct AxisDialog *d;
  char *group;
  char title[25];

  d = (struct AxisDialog *) data;
  getobj(d->Obj, "group", d->Id, 0, NULL, &group);
  if (group == NULL)
    group = "";
  snprintf(title, sizeof(title), _("Axis %d %s"), d->Id, group);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Inc:"), TRUE);
    d->inc = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    item_setup(hbox, w, _("_Scale:"), TRUE);
    d->scale = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);


    vbox = gtk_vbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogClear), d);
    gtk_box_pack_start(GTK_BOX(hbox2), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_with_mnemonic(_("_File"));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogFile), d);
    gtk_box_pack_start(GTK_BOX(hbox2), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(vbox, w, _("_Div:"), FALSE);
    d->div = w;

    w = combo_box_entry_create();
    item_setup(vbox, w, _("_Ref:"), FALSE);
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
    g_signal_connect(w, "changed", G_CALLBACK(AxisDialogRef), d);
    d->ref = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
    gtk_container_add(GTK_CONTAINER(frame), hbox);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Baseline ..."));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogBase), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Gauge ..."));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogGauge), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Numbering ..."));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogNum), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Font ..."));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogFont), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Position ..."));
    g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogPos), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  /*
  if (d->CanDel)
    d->Delete;
  else
    XtUnmanageChild(d->Delete);
  */
  AxisDialogSetupItem(wi, d, d->Id);

}

static void
AxisDialogClose(GtkWidget *w, void *data)
{
  struct AxisDialog *d;
  int ret;
  const char *s;
  char *buf;
  int len;

  d = (struct AxisDialog *) data;

  switch (d->ret) {
  case IDCOPY:
    AxisDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  exeobj(d->Obj, "scale_push", d->Id, 0, NULL);

  if (SetObjFieldFromText(GTK_BIN(d->min)->child,
			  d->Obj, d->Id, "min"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->max)->child,
			  d->Obj, d->Id, "max"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->inc)->child,
			  d->Obj, d->Id, "inc"))
    return;

  if (SetObjFieldFromText(d->div, d->Obj, d->Id, "div"))
    return;

  if (SetObjFieldFromList(d->scale, d->Obj, d->Id, "type"))
    return;

  s = combo_box_entry_get_text(d->ref);
  if (s == NULL || s[0] == '\0') {
    if (sputobjfield(d->Obj, d->Id, "reference", NULL) != 0)
      return;
  } else {
    len = strlen(s) + 6;
    if ((buf = (char *) memalloc(len)) != NULL) {
      snprintf(buf, len, "axis:%s", (s)? s: "");
      if (sputobjfield(d->Obj, d->Id, "reference", buf) != 0) {
	memfree(buf);
	return;
      }
      memfree(buf);
    }
  }
  d->ret = ret;
}

void
AxisDialog(void *data, struct objlist *obj, int id, int candel)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) data;

  d->SetupWindow = AxisDialogSetup;
  d->CloseWindow = AxisDialogClose;
  d->Obj = obj;
  d->Id = id;
  d->CanDel = candel;
}

void
CmAxisNewFrame(void)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg, ret;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if ((obj2 = getobject("axisgrid")) == NULL)
    return;
  idx = newobj(obj);
  idy = newobj(obj);
  idu = newobj(obj);
  idr = newobj(obj);
  idg = -1;
  arrayinit(&group, sizeof(int));
  type = 1;
  x = 3500;
  y = 22000;
  lenx = 14000;
  leny = 14000;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &idu);
  arrayadd(&group, &idr);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idr, 1, argv);
  arraydel(&group);
  SectionDialog(&DlgSection, x, y, lenx, leny, obj, idx, idy, idu, idr, obj2,
		&idg, FALSE);
  ret = DialogExecute(TopLevel, &DlgSection);
  if ((ret == IDDELETE) || (ret == IDCANCEL)) {
    if (idg != -1)
      delobj(obj2, idg);
    delobj(obj, idr);
    delobj(obj, idu);
    delobj(obj, idy);
    delobj(obj, idx);
  } else
    NgraphApp.Changed = TRUE;
  AxisWinUpdate(TRUE);
}

void
CmAxisNewSection(void)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg, ret, oidx, oidy;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];
  char *ref;

  if (Menulock || GlobalLock)
    return;
  if ((obj = getobject("axis")) == NULL)
    return;
  if ((obj2 = getobject("axisgrid")) == NULL)
    return;
  idx = newobj(obj);
  idy = newobj(obj);
  idu = newobj(obj);
  idr = newobj(obj);
  idg = newobj(obj2);
  arrayinit(&group, sizeof(int));
  type = 2;
  x = 3500;
  y = 22000;
  lenx = 14000;
  leny = 14000;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &idu);
  arrayadd(&group, &idr);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idr, 1, argv);
  arraydel(&group);
  if (idg >= 0) {
    getobj(obj, "oid", idx, 0, NULL, &oidx);
    if ((ref = (char *) memalloc(ID_BUF_SIZE)) != NULL) {
      snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
      putobj(obj2, "axis_x", idg, ref);
    }
    getobj(obj, "oid", idy, 0, NULL, &oidy);
    if ((ref = (char *) memalloc(ID_BUF_SIZE)) != NULL) {
      snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidy);
      putobj(obj2, "axis_y", idg, ref);
    }
  }
  SectionDialog(&DlgSection, x, y, lenx, leny, obj, idx, idy, idu, idr, obj2,
		&idg, TRUE);
  ret = DialogExecute(TopLevel, &DlgSection);
  if ((ret == IDDELETE) || (ret == IDCANCEL)) {
    delobj(obj2, idg);
    delobj(obj, idr);
    delobj(obj, idu);
    delobj(obj, idy);
    delobj(obj, idx);
  } else
    NgraphApp.Changed = TRUE;
  AxisWinUpdate(TRUE);
}

void
CmAxisNewCross(void)
{
  struct objlist *obj;
  int idx, idy, ret;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  idx = newobj(obj);
  idy = newobj(obj);
  arrayinit(&group, sizeof(int));
  type = 3;
  x = 3500;
  y = 22000;
  lenx = 14000;
  leny = 14000;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idy, 1, argv);
  arraydel(&group);
  CrossDialog(&DlgCross, x, y, lenx, leny, obj, idx, idy);
  ret = DialogExecute(TopLevel, &DlgCross);
  if ((ret == IDDELETE) || (ret == IDCANCEL)) {
    delobj(obj, idy);
    delobj(obj, idx);
  } else
    NgraphApp.Changed = TRUE;
  AxisWinUpdate(TRUE);
}

void
CmAxisNewSingle(void)
{
  struct objlist *obj;
  int id, ret;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    AxisDialog(&DlgAxis, obj, id, TRUE);
    ret = DialogExecute(TopLevel, &DlgAxis);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else
      NgraphApp.Changed = TRUE;
    AxisWinUpdate(TRUE);
  }
}

void
CmAxisAddMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdAxisNewFrame:
    CmAxisNewFrame();
    break;
  case MenuIdAxisNewSection:
    CmAxisNewSection();
    break;
  case MenuIdAxisNewCross:
    CmAxisNewCross();
    break;
  case MenuIdAxisNewSingle:
    CmAxisNewSingle();
    break;
  }
}

void
CmAxisDel(void)
{
  struct objlist *obj;
  int i;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  CopyDialog(&DlgCopy, obj, -1, AxisCB);
  if (DialogExecute(TopLevel, &DlgCopy) == IDOK) {
    i = DlgCopy.sel;
  } else {
    return;
  }
  AxisDel(i);
  NgraphApp.Changed = TRUE;
  AxisWinUpdate(TRUE);
  FileWinUpdate(TRUE);
}

void
CmAxisUpdate(void)
{
  struct objlist *obj;
  int i, ret;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  CopyDialog(&DlgCopy, obj, -1, AxisCB);
  if (DialogExecute(TopLevel, &DlgCopy) == IDOK) {
    i = DlgCopy.sel;
    if (i < 0)
      return;
  } else
    return;
  AxisDialog(&DlgAxis, obj, i, TRUE);
  if ((ret = DialogExecute(TopLevel, &DlgAxis)) == IDDELETE) {
    AxisDel(i);
  }
  if (ret != IDCANCEL)
    NgraphApp.Changed = TRUE;
  AxisWinUpdate(TRUE);
  FileWinUpdate(TRUE);
}

void
CmAxisZoom(void)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num, room;
  double zoom, min, max, mid, wd;
  char *argv[4];

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  ZoomDialog(&DlgZoom);
  if ((DialogExecute(TopLevel, &DlgZoom) == IDOK) && (DlgZoom.zoom > 0)) {
    zoom = DlgZoom.zoom / 10000.0;
    SelectDialog(&DlgSelect, obj, AxisCB, (struct narray *) &farray, NULL);
    if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
      num = arraynum(&farray);
      array = (int *) arraydata(&farray);
      for (i = 0; i < num; i++) {
	getobj(obj, "min", array[i], 0, NULL, &min);
	getobj(obj, "max", array[i], 0, NULL, &max);
	wd = (max - min) / 2;
	if (wd != 0) {
	  mid = (min + max) / 2;
	  min = mid - wd * zoom;
	  max = mid + wd * zoom;
	  room = 1;
	  argv[0] = (char *) &min;
	  argv[1] = (char *) &max;
	  argv[2] = (char *) &room;
	  argv[3] = NULL;
	  exeobj(obj, "scale", array[i], 3, argv);
	  NgraphApp.Changed = TRUE;
	}
      }
      AxisWinUpdate(TRUE);
    }
    arraydel(&farray);
  }
}

void
CmAxisClear(GtkWidget *w, gpointer p)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, AxisCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = 0; i < num; i++) {
      exeobj(obj, "scale_push", array[i], 0, NULL);
      exeobj(obj, "clear", array[i], 0, NULL);
      NgraphApp.Changed = TRUE;
    }
    AxisWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmAxisMenu(GtkMenuItem *menuitem, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdAxisUpdate:
    CmAxisUpdate();
    break;
  case MenuIdAxisDel:
    CmAxisDel();
    break;
  case MenuIdAxisZoom:
    CmAxisZoom();
    break;
  case MenuIdAxisClear:
    CmAxisClear(NULL, NULL);
    break;
  }
}

void
CmAxisGridNew(void)
{
  struct objlist *obj;
  int id, ret;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    GridDialog(&DlgGrid, obj, id);
    ret = DialogExecute(TopLevel, &DlgGrid);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else
      NgraphApp.Changed = TRUE;
  }
}

void
CmAxisGridDel(void)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int num, *array;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, GridCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, array[i]);
      NgraphApp.Changed = TRUE;
    }
  }
  arraydel(&farray);
}


void
CmAxisGridUpdate(void)
{
  struct narray farray;
  struct objlist *obj;
  int i, j, ret;
  int *array, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, GridCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = 0; i < num; i++) {
      GridDialog(&DlgGrid, obj, array[i]);
      if ((ret = DialogExecute(TopLevel, &DlgGrid)) == IDDELETE) {
	delobj(obj, array[i]);
	for (j = i + 1; j < num; j++)
	  array[j]--;
      }
      if (ret != IDCANCEL)
	NgraphApp.Changed = TRUE;
    }
  }
  arraydel(&farray);
}


void
CmGridMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdAxisGridNew:
    CmAxisGridNew();
    break;
  case MenuIdAxisGridUpdate:
    CmAxisGridUpdate();
    break;
  case MenuIdAxisGridDel:
    CmAxisGridDel();
    break;
  }
}


void
AxisWinUpdate(int clear)
{
  struct SubWin *d;

  d = &(NgraphApp.AxisWin);

  AxisWinExpose(NULL, NULL, NULL);

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID, d->select);
  }
}

static void
axis_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row)
{
  int cx;
  int i, len;
  double min, max, inc;
  char buf[256];

  for (i = 0; i < AXIS_WIN_COL_NUM; i++) {
    if (strcmp(Alist[i].name, "group") == 0) {
      char *name;
      getobj(d->obj, "group", row, 0, NULL, &name);
      if (name != NULL) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, name);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
      }
    } else if (strcmp(Alist[i].name, "min") == 0) {
      getobj(d->obj, "min", row, 0, NULL, &min);
    } else if (strcmp(Alist[i].name, "max") == 0) {
      getobj(d->obj, "max", row, 0, NULL, &max);
      if ((min == 0) && (max == 0)) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i - 1, "---------");
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "---------");
      } else {
	len = snprintf(buf, sizeof(buf), "%+.2e", min);
	list_store_set_string(GTK_WIDGET(d->text), iter, i - 1, buf);

	len = snprintf(buf, sizeof(buf), "%+.2e", max);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
      }
    } else if (strcmp(Alist[i].name, "type") == 0) {
      char *valstr;
      sgetobjfield(d->obj, row, "type", NULL, &valstr, FALSE, FALSE, FALSE);
      list_store_set_string(GTK_WIDGET(d->text), iter, i, _(valstr));
      memfree(valstr);
    } else if (strcmp(Alist[i].name, "inc") == 0) {
      getobj(d->obj, "inc", row, 0, NULL, &inc);
      if (inc == 0) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "---------");
      } else {
	len = snprintf(buf, sizeof(buf), "%+.2e", inc);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
      }
    } else if (strcmp(Alist[i].name, "hidden") == 0) {
      getobj(d->obj, Alist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Alist[i].type, &cx);
    } else {
      getobj(d->obj, Alist[i].name, row, 0, NULL, &cx);
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Alist[i].type, &cx);
    }
  }
}

static gboolean
AxisWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data)
{
  struct SubWin *d;

  if (Menulock || GlobalLock)
    return TRUE;

  d = &(NgraphApp.AxisWin);

  if (d->text == NULL)
    return FALSE;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, axis_list_set_val);
  } else {
    list_sub_window_set(d, axis_list_set_val);
  }

  return FALSE;
}

/*
void
AxisWindowUnmap(GtkWidget *w, gpointer client_data)
{
  struct AxisWin *d;
  Position x, y, x0, y0;
  Dimension w0, h0;

  d = &(NgraphApp.AxisWin);
  if (d->Win != NULL) {
    XtVaGetValues(d->Win, XmNx, &x, XmNy, &y,
		  XmNwidth, &w0, XmNheight, &h0, NULL);
    menulocal.axiswidth = w0;
    menulocal.axisheight = h0;
    XtTranslateCoords(TopLevel, 0, 0, &x0, &y0);
    menulocal.axisx = x - x0;
    menulocal.axisy = y - y0;
    XtDestroyGtkWidget(d->Win);
    d->Win = NULL;
    d->text = NULL;
    XmToggleButtonSetState(XtNameToGtkWidget
			   (TopLevel, "*windowmenu.button_1"), False, False);
  }
}
*/

void
CmAxisWinScaleUndo(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;
  char *argv[1];
  struct objlist *obj;
  struct narray farray;
  int i, num, *array;

  if (Menulock || GlobalLock)
    return;
  d = &(NgraphApp.AxisWin);
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, AxisCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      argv[0] = NULL;
      exeobj(obj, "scale_pop", array[i], 0, argv);
      NgraphApp.Changed = TRUE;
    }
    AxisWinUpdate(TRUE);
  }
  arraydel(&farray);
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct SubWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct SubWin *) user_data;
  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_space:
    list_sub_window_focus(NULL, d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, i;
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  sel = d->select;
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= d->num);
  }
}

void
CmAxisWindow(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  d = &(NgraphApp.AxisWin);
  d ->type = TypeAxisWin;

  if (d->Win) {
    if (GTK_WIDGET_VISIBLE(d->Win)) { 
      gtk_widget_hide(d->Win);
    } else {
      gtk_widget_show_all(d->Win);
    }
  } else {
    GtkWidget *dlg;

    d->update = AxisWinUpdate;
    d->setup_dialog = AxisDialog;
    d->dialog = &DlgAxis;
    d->ev_key = ev_key_down;

    dlg = list_sub_window_create(d, "Axis Window", AXIS_WIN_COL_NUM, Alist, Axiswin_xpm);

    g_signal_connect(dlg, "expose-event", G_CALLBACK(AxisWinExpose), NULL);

    d->obj = chkobject("axis");
    d->num = chkobjlastinst(d->obj);

    d->select = -1;
    sub_win_create_popup_menu(d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
    gtk_widget_show_all(dlg);
  }
}
