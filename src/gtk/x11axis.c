/* 
 * $Id: x11axis.c,v 1.81 2010-03-04 08:30:17 hito Exp $
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
#include <math.h>

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "mathfn.h"
#include "gra.h"
#include "axis.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

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
  {"",         G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden",    FALSE},
  {"#",        G_TYPE_INT,     TRUE, FALSE, "id",        FALSE},
  {N_("name"), G_TYPE_STRING,  TRUE, FALSE, "group",     FALSE},
  {N_("min"),  G_TYPE_STRING,  TRUE, TRUE,  "min",       FALSE},
  {N_("max"),  G_TYPE_STRING,  TRUE, TRUE,  "max",       FALSE},
  {N_("inc"),  G_TYPE_STRING,  TRUE, TRUE,  "inc",       FALSE},
  {N_("type"), G_TYPE_ENUM,    TRUE, TRUE,  "type",      FALSE},
  {"x",        G_TYPE_DOUBLE,  TRUE, TRUE,  "x",         FALSE, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",        G_TYPE_DOUBLE,  TRUE, TRUE,  "y",         FALSE, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("dir"),  G_TYPE_DOUBLE,  TRUE, TRUE,  "direction", FALSE,                0,          36000, 100, 1500},
  {N_("len"),  G_TYPE_DOUBLE,  TRUE, TRUE,  "length",    FALSE, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"^#",       G_TYPE_INT,     TRUE, FALSE, "oid",       FALSE},
};

#define AXIS_WIN_COL_NUM (sizeof(Alist)/sizeof(*Alist))
#define AXIS_WIN_COL_OID (AXIS_WIN_COL_NUM - 1)
#define AXIS_WIN_COL_HIDDEN 0
#define AXIS_WIN_COL_ID     1
#define AXIS_WIN_COL_NAME   2
#define AXIS_WIN_COL_MIN    3
#define AXIS_WIN_COL_MAX    4
#define AXIS_WIN_COL_INC    5
#define AXIS_WIN_COL_TYPE   6
#define AXIS_WIN_COL_X      7
#define AXIS_WIN_COL_Y      8

static void axiswin_scale_clear(GtkMenuItem *item, gpointer user_data);
static void axis_delete_popup_func(GtkMenuItem *w, gpointer client_data);
static void AxisWinAxisTop(GtkWidget *w, gpointer client_data);
static void AxisWinAxisUp(GtkWidget *w, gpointer client_data);
static void AxisWinAxisDown(GtkWidget *w, gpointer client_data);
static void AxisWinAxisLast(GtkWidget *w, gpointer client_data);

static struct subwin_popup_list Popup_list[] = {
  {N_("_Duplicate"),      G_CALLBACK(list_sub_window_copy), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_DELETE,      G_CALLBACK(axis_delete_popup_func), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Focus"),          G_CALLBACK(list_sub_window_focus), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Show"),           G_CALLBACK(list_sub_window_hide), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_CHECK},
  {GTK_STOCK_CLEAR,       G_CALLBACK(axiswin_scale_clear), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_PROPERTIES,  G_CALLBACK(list_sub_window_update), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(AxisWinAxisTop), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(AxisWinAxisUp), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(AxisWinAxisDown), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(AxisWinAxisLast), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))

#define POPUP_ITEM_HIDE 4
#define POPUP_ITEM_TOP 8
#define POPUP_ITEM_UP 9
#define POPUP_ITEM_DOWN 10
#define POPUP_ITEM_BOTTOM 11

#define CB_BUF_SIZE 128
#define ID_BUF_SIZE 16
#define TITLE_BUF_SIZE 128 

static gboolean AxisWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data);
static void axis_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row);
static int check_axis_history(struct objlist *obj);

void
axis_scale_push(struct objlist *obj, int id)
{
  int n;

  exeobj(obj, "scale_push", id, 0, NULL);

  n = check_axis_history(obj);
  set_axis_undo_button_sensitivity(n > 0);
}

char *
AxisCB(struct objlist *obj, int id)
{
  char *s, *valstr, *name;
  int dir;

  getobj(obj, "direction", id, 0, NULL, &dir);
  getobj(obj, "group", id, 0, NULL, &name);
  name = CHK_STR(name);
  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);
  s = g_strdup_printf("%-10s %.6s %s:%.2f", name, _(valstr), _("dir"), dir / 100.0);
  g_free(valstr);

  return s;
}

char *
AxisHistoryCB(struct objlist *obj, int id)
{
  char *s, *valstr, *name;
  int dir, num;
  struct narray *array;

  getobj(obj, "scale_history", id, 0, NULL, &array);

  num = arraynum(array) / 3;
  if (num == 0)
    return NULL;

  getobj(obj, "group", id, 0, NULL, &name);
  getobj(obj, "direction", id, 0, NULL, &dir);

  name = CHK_STR(name);
  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);
  s = g_strdup_printf("%-10s %.6s %s:%.2f", name, _(valstr), _("dir"), dir / 100.0);
  g_free(valstr);

  return s;
}

static char *
GridCB(struct objlist *obj, int id)
{
  char *s, *s1, *s2;

  getobj(obj, "axis_x", id, 0, NULL, &s1);
  getobj(obj, "axis_y", id, 0, NULL, &s2);
  s = g_strdup_printf("%.8s %.8s", (s1)? s1: "-----", (s2)? s2: "-----");

  return s;
}

static void
bg_button_toggled(GtkToggleButton *button, gpointer user_data)
{
  struct GridDialog *d;
  gboolean state;

  d = (struct GridDialog *) user_data;

  state = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(d->bcolor, state);
  gtk_widget_set_sensitive(d->bclabel, state);
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
    name = CHK_STR(name);

    combo_box_append_text(d->axisx, name);
    combo_box_append_text(d->axisy, name);
  }

  sgetobjfield(d->Obj, id, "axis_x", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':') {
    i++;
  }
  combo_box_entry_set_text(d->axisx, valstr + i);
  g_free(valstr);

  sgetobjfield(d->Obj, id, "axis_y", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':') {
    i++;
  }
  combo_box_entry_set_text(d->axisy, valstr + i);
  g_free(valstr);

  SetWidgetFromObjField(d->draw_x, d->Obj, id, "grid_x");
  SetWidgetFromObjField(d->draw_y, d->Obj, id, "grid_y");

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1"; 

    style[sizeof(style) - 2] += i;
    SetStyleFromObjField(d->style[i], d->Obj, id, style);

    width[sizeof(width) - 2] += i;
    SetWidgetFromObjField(d->width[i], d->Obj, id, width);
  }
  SetWidgetFromObjField(d->background, d->Obj, id, "background");
  bg_button_toggled(GTK_TOGGLE_BUTTON(d->background), d);

  set_color(d->color, d->Obj, id, NULL);
  set_color(d->bcolor, d->Obj, id, "B");
}

static void
grid_copy_clicked(GtkButton *btn, gpointer user_data)
{
  int sel;
  struct GridDialog *d;

  d = (struct GridDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, GridCB);
  if (sel != -1) {
    GridDialogSetupItem(d->widget, d, sel);
  }
}

static void
GridDialogAxis(GtkWidget *w, gpointer client_data)
{
  char buf[10];
  int a, oid;
  struct objlist *aobj;

  aobj = getobject("axis");
  a = combo_box_get_active(w);
  if (a < 0)
    return;
  getobj(aobj, "oid", a, 0, NULL, &oid);
  snprintf(buf, sizeof(buf), "^%d", oid);
  combo_box_entry_set_text(w, buf);
}

static void
gauge_syle_setup(struct GridDialog *d, GtkWidget *table, int n, int j)
{
  GtkWidget *w;
  char buf[TITLE_BUF_SIZE]; 

  if (n < 0 || n >= GRID_DIALOG_STYLE_NUM)
    return;

  snprintf(buf, sizeof(buf), _("_Style %d:"), n + 1);
  w = combo_box_entry_create();
  add_widget_to_table_sub(table, w, buf, TRUE, 0, 1, 4, j);
  d->style[n] = w;

  snprintf(buf, sizeof(buf), _("_Width %d:"), n + 1);
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table_sub(table, w, buf, FALSE, 2, 1, 4, j);
  d->width[n] = w;
}

static void
GridDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *frame, *w, *hbox, *table;
  struct GridDialog *d;
  char title[TITLE_BUF_SIZE];
  int i, j;

  d = (struct GridDialog *) data;
  snprintf(title, sizeof(title), _("Grid %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  d = (struct GridDialog *) data;
  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

    j = 0;
    w = combo_box_entry_create();
    add_widget_to_table(table, w, _("Axis (_X):"), FALSE, j++);
    g_signal_connect(w, "changed", G_CALLBACK(GridDialogAxis), NULL);
    d->axisx = w;

    w = gtk_check_button_new_with_mnemonic(_("draw _X grid"));
    add_widget_to_table(table, w, NULL, FALSE, j++);
    d->draw_x = w;

    w = combo_box_entry_create();
    add_widget_to_table(table, w, _("Axis (_Y):"), FALSE, j++);
    g_signal_connect(w, "changed", G_CALLBACK(GridDialogAxis), NULL);
    d->axisy = w;

    w = gtk_check_button_new_with_mnemonic(_("draw _Y grid"));
    add_widget_to_table(table, w, NULL, FALSE, j++);
    d->draw_y = w;

    frame = gtk_frame_new(_("Axis"));
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);


    table = gtk_table_new(1, 2, FALSE);

    j = 0;
    w = create_color_button(wi);
    add_widget_to_table(table, w, _("_Color:"), FALSE, j++);
    d->color = w;

    w = gtk_check_button_new_with_mnemonic(_("_Background"));
    add_widget_to_table(table, w, NULL, FALSE, j++);
    g_signal_connect(w, "toggled", G_CALLBACK(bg_button_toggled), d);
    d->background = w;

    w = create_color_button(wi);
    d->bclabel = add_widget_to_table(table, w, _("_Background Color:"), FALSE, j++);
    d->bcolor = w;

    frame = gtk_frame_new(_("Color"));
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    table = gtk_table_new(1, 4, FALSE);
    j = 0;
    for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
      gauge_syle_setup(d, table, i, j++);
    }

    frame = gtk_frame_new(_("Style"));
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(grid_copy_clicked), d, "axisgrid");
  }

  GridDialogSetupItem(wi, d, d->Id);
}

static void
GridDialogClose(GtkWidget *w, void *data)
{
  struct GridDialog *d;
  int ret;
  int i;

  d = (struct GridDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjAxisFieldFromWidget(d->axisx, d->Obj, d->Id, "axis_x"))
    return;

  if (SetObjAxisFieldFromWidget(d->axisy, d->Obj, d->Id, "axis_y"))
    return;

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1"; 

    style[sizeof(style) - 2] += i;
    if (SetObjFieldFromStyle(d->style[i], d->Obj, d->Id, style))
      return;

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromWidget(d->width[i], d->Obj, d->Id, width))
      return;
  }

  if (SetObjFieldFromWidget(d->draw_x, d->Obj, d->Id, "grid_x"))
    return;

  if (SetObjFieldFromWidget(d->draw_y, d->Obj, d->Id, "grid_y"))
    return;

  if (SetObjFieldFromWidget(d->background, d->Obj, d->Id, "background"))
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
  spin_entry_set_val(d->x, d->X);
  spin_entry_set_val(d->y, d->Y);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);
  set_axis_id(d->uid, d->IDU);
  set_axis_id(d->rid, d->IDR);
  set_axis_id(d->gid, *(d->IDG));

  spin_entry_set_val(d->width, d->LenX);
  spin_entry_set_val(d->height, d->LenY);
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
  int ret, oidx, oidy, create = FALSE;

  d = (struct SectionDialog *) client_data;
  if (*(d->IDG) == -1) {
    if ((*(d->IDG) = newobj(d->Obj2)) >= 0) {
      if ((ref = (char *) g_malloc(ID_BUF_SIZE)) != NULL) {
	getobj(d->Obj, "oid", d->IDX, 0, NULL, &oidx);
	snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
	putobj(d->Obj2, "axis_x", *(d->IDG), ref);
      }
      if ((ref = (char *) g_malloc(ID_BUF_SIZE)) != NULL) {
	getobj(d->Obj, "oid", d->IDY, 0, NULL, &oidy);
	snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidy);
	putobj(d->Obj2, "axis_y", *(d->IDG), ref);
      }
      create = TRUE;
    }
  }
  if (*(d->IDG) >= 0) {
    GridDialog(&DlgGrid, d->Obj2, *(d->IDG));
    ret = DialogExecute(d->widget, &DlgGrid);
    switch (ret) {
    case IDCANCEL:
      if (! create)
	break;
    case IDDELETE:
      delobj(d->Obj2, *(d->IDG));
      *(d->IDG) = -1;
      if (create)
	break;
    default:
      set_graph_modified();
    }
  }
  SectionDialogSetupItem(d->widget, d);
}


static void
SectionDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *table;
  struct SectionDialog *d;
  int i;

  d = (struct SectionDialog *) data;

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);


    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Height:"), FALSE, i++);
    d->height = w;

    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);
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
  int type;
  struct narray group;
  char *argv[2];

  d = (struct SectionDialog *) data;
  if (d->ret != IDOK)
    return;
  ret = d->ret;

  d->ret = IDLOOP;

  d->X = spin_entry_get_val(d->x);
  d->Y = spin_entry_get_val(d->y);

  d->LenX = spin_entry_get_val(d->width);
  d->LenY = spin_entry_get_val(d->height);

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
  spin_entry_set_val(d->x, d->X);
  spin_entry_set_val(d->y, d->Y);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);

  spin_entry_set_val(d->width, d->LenX);
  spin_entry_set_val(d->height, d->LenY);
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
  GtkWidget *w, *hbox, *vbox, *table;
  struct CrossDialog *d;
  int i;

  d = (struct CrossDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);


    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Height:"), FALSE, i++);
    d->height = w;

    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);

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
  int type;
  struct narray group;
  char *argv[2];

  d = (struct CrossDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;

  d->ret = IDLOOP;

  d->X = spin_entry_get_val(d->x);
  d->Y = spin_entry_get_val(d->y);

  d->LenX = spin_entry_get_val(d->width);
  d->LenY = spin_entry_get_val(d->height);

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
  spin_entry_set_val(d->zoom_entry, d->zoom);
}

static void
ZoomDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *vbox;
  struct ZoomDialog *d;

  d = (struct ZoomDialog *) data;
  if (makewidget) {
    vbox = gtk_vbox_new(FALSE, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
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

  d = (struct ZoomDialog *) data;
  if (d->ret != IDOK)
    return;

  d->zoom = spin_entry_get_val(d->zoom_entry);
}

void
ZoomDialog(struct ZoomDialog *data)
{
  data->SetupWindow = ZoomDialogSetup;
  data->CloseWindow = ZoomDialogClose;
}

static void
scale_tab_setup_item(struct AxisDialog *d, int id)
{
  char *valstr;
  int i, j;
  double min, max, inc, pmin, pmax, pinc;
  int lastinst;
  char *name;
  struct narray *array;
  int num;
  double *data;
  char buf[30];

  combo_box_clear(d->min);
  combo_box_clear(d->max);
  combo_box_clear(d->inc);

  getobj(d->Obj, "min", id, 0, NULL, &min);
  getobj(d->Obj, "max", id, 0, NULL, &max);
  getobj(d->Obj, "inc", id, 0, NULL, &inc);

  getobj(d->Obj, "scale_history", d->Id, 0, NULL, &array);
  if (array) {
    pmin = min;
    pmax = max;
    pinc = inc;
    num = arraynum(array) / 3;
    data = arraydata(array);
    for (j = 0; j < num; j++) {
      if (data[0 + j * 3] != pmin) {
	snprintf(buf, sizeof(buf), "%.15g", data[0 + j * 3]);
	combo_box_append_text(d->min, buf);
      }
      pmin = data[0 + j * 3];

      if (data[1 + j * 3] != pmax) {
	snprintf(buf, sizeof(buf), "%.15g", data[1 + j * 3]);
	combo_box_append_text(d->max, buf);
      }
      pmax = data[1 + j * 3];

      if (data[2 + j * 3] != pinc) {
	snprintf(buf, sizeof(buf), "%.15g", data[2 + j * 3]);
	combo_box_append_text(d->inc, buf);
      }
      pinc = data[2 + j * 3];
    }
  }

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

  SetWidgetFromObjField(d->div, d->Obj, id, "div");
  SetWidgetFromObjField(d->scale, d->Obj, id, "type");

  combo_box_clear(d->ref);
  lastinst = chkobjlastinst(d->Obj);
  combo_box_append_text(d->ref, _("none"));
  for (j = 0; j <= lastinst; j++) {
    getobj(d->Obj, "group", j, 0, NULL, &name);
    name =CHK_STR(name);
    combo_box_append_text(d->ref, name);
  }

  sgetobjfield(d->Obj, id, "reference", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':') {
    i++;
  }
  combo_box_entry_set_text(d->ref, valstr + i);
  g_free(valstr);

  SetWidgetFromObjField(d->margin, d->Obj, id, "auto_scale_margin");
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
  struct objlist *fobj;
  struct narray farray;

  d = (struct AxisDialog *) client_data;

  fobj = chkobject("file");
  if (fobj == NULL)
    return;

  if (chkobjlastinst(fobj) == -1)
    return;

  SelectDialog(&DlgSelect, fobj, FileCB, (struct narray *) &farray, NULL);

  if (DialogExecute(d->widget, &DlgSelect) == IDOK) {
    int a, i, anum, num, *array;

    num = arraynum(&farray);
    array = arraydata(&farray);
    anum = chkobjlastinst(d->Obj);

    if (num > 0 && anum != 0) {
      char *buf, *argv2[2];
      GString *str;
      int type;
      struct narray *result;

      str = g_string_sized_new(32);
      if (str) {
	g_string_append(str, "file:");
	for (i = 0; i < num; i++) {
	  if (i == num - 1) {
	    g_string_append_printf(str, "%d", array[i]);
	  } else {
	    g_string_append_printf(str, "%d,", array[i]);
	  }
	}

	buf = g_string_free(str, FALSE);
	argv2[0] = (char *) buf;
	argv2[1] = NULL;

	if (getobj(d->Obj, "type", d->Id, 0, NULL, &type) == -1) {
	  arraydel(&farray);
	  g_free(buf);
	  return;
	}

	a = combo_box_get_active(d->scale);
	if (a >= 0 && (putobj(d->Obj, "type", d->Id, &a) == -1)) {
	  arraydel(&farray);
	  g_free(buf);
	  return;
	}

	getobj(d->Obj, "get_auto_scale", d->Id, 1, argv2, &result);
	g_free(buf);

	if (arraynum(result) == 3) {
	  char s[30];

	  snprintf(s, sizeof(s), "%.15g", arraynget_double(result, 0));
	  combo_box_entry_set_text(d->min, s);

	  snprintf(s, sizeof(s), "%.15g", arraynget_double(result, 1));
	  combo_box_entry_set_text(d->max, s);

	  snprintf(s, sizeof(s), "%.15g", arraynget_double(result, 2));
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
  if (a < 0) {
    return;
  }

  if (a == 0) {
    combo_box_entry_set_text(w, "");
    return;
  }

  getobj(d->Obj, "oid", a - 1, 0, NULL, &oid);
  snprintf(buf, sizeof(buf), "^%d", oid);
  combo_box_entry_set_text(w, buf);
}

static void
file_button_show(GtkWidget *widget, gpointer user_data)
{
  static struct objlist *file = NULL;
  int n;

  if (file == NULL) {
    file = chkobject("file");
  }

  if (file == NULL)
    return;

  n = chkobjlastinst(file);

  gtk_widget_set_sensitive(widget, n >= 0);
}

static void
scale_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    scale_tab_setup_item(d, sel);
  }
}

static GtkWidget *
scale_tab_create(struct AxisDialog *d)
{
  GtkWidget *parent_box, *w, *frame, *table, *hbox;
  int i;

  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Min:"), TRUE, i++);
  d->min = w;

  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Max:"), TRUE, i++);
  d->max = w;

  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Inc:"), TRUE, i++);
  d->inc = w;


  hbox = gtk_hbox_new(FALSE, 12);

  w = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogClear), d);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);

  w = gtk_button_new_with_mnemonic(_("_File"));
  g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogFile), d);
  g_signal_connect(w, "show", G_CALLBACK(file_button_show), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  add_widget_to_table(table, hbox, "", FALSE, i++);
  
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Scale:"), FALSE, i++);
  d->scale = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Div:"), FALSE, i++);
  d->div = w;

  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Ref:"), FALSE, i++);
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  g_signal_connect(w, "changed", G_CALLBACK(AxisDialogRef), d);
  d->ref = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Auto scale margin:"), FALSE, i++);
  d->margin = w;

  frame = gtk_frame_new(_("Scale"));
  gtk_container_add(GTK_CONTAINER(frame), table);

  parent_box = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(parent_box), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(parent_box, G_CALLBACK(scale_tab_copy_clicked), d, "axis");

  return parent_box;
}

static int
baseline_tab_set_value(struct AxisDialog *axis)
{
  struct AxisBase *d;

  d = &axis->base;

  if (SetObjFieldFromStyle(d->style, axis->Obj, axis->Id, "style"))
    return 1;

  if (SetObjFieldFromWidget(d->width, axis->Obj, axis->Id, "width"))
    return 1;

  if (SetObjFieldFromWidget(d->baseline, axis->Obj, axis->Id, "baseline"))
    return 1;

  if (SetObjFieldFromWidget(d->arrow, axis->Obj, axis->Id, "arrow"))
    return 1;

  if (SetObjFieldFromWidget(d->arrowlen, axis->Obj, axis->Id, "arrow_length"))
    return 1;

  if (SetObjFieldFromWidget(d->arrowwid, axis->Obj, axis->Id, "arrow_width"))
    return 1;

  if (SetObjFieldFromWidget(d->wave, axis->Obj, axis->Id, "wave"))
    return 1;

  if (SetObjFieldFromWidget(d->wavelen, axis->Obj, axis->Id, "wave_length"))
    return 1;

  if (SetObjFieldFromWidget(d->wavewid, axis->Obj, axis->Id, "wave_width"))
    return 1;

  if (putobj_color(d->color, axis->Obj, axis->Id, NULL))
    return 1;

  return 0;
}

static void
baseline_tab_setup_item(struct AxisDialog *axis, int id)
{
  struct AxisBase *d;

  d = &axis->base;

  SetStyleFromObjField(d->style, axis->Obj, id, "style");

  SetWidgetFromObjField(d->width, axis->Obj, id, "width");

  SetWidgetFromObjField(d->baseline, axis->Obj, id, "baseline");

  SetWidgetFromObjField(d->arrow, axis->Obj, id, "arrow");

  SetWidgetFromObjField(d->arrowlen, axis->Obj, id, "arrow_length");

  SetWidgetFromObjField(d->arrowwid, axis->Obj, id, "arrow_width");

  SetWidgetFromObjField(d->wave, axis->Obj, id, "wave");

  SetWidgetFromObjField(d->wavelen, axis->Obj, id, "wave_length");

  SetWidgetFromObjField(d->wavewid, axis->Obj, id, "wave_width");

  set_color(d->color, axis->Obj, id, NULL);
}

static void
baseline_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    baseline_tab_setup_item(d, sel);
  }
}

static GtkWidget *
baseline_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *hbox, *vbox, *frame, *table;
  struct AxisBase *d;
  int i;

  d = &dd->base;

  hbox = gtk_hbox_new(FALSE, 4);

  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = gtk_check_button_new_with_mnemonic(_("Draw _Baseline"));
  add_widget_to_table(table, w, NULL, FALSE , i++);
  d->baseline = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  add_widget_to_table(table, w, _("Line _Style:"), TRUE, i++);
  d->style = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Line Width:"), FALSE, i++);
  d->width = w;

  w = create_color_button(wi);
  add_widget_to_table(table, w, _("_Color:"), FALSE, i++);
  d->color = w;

  frame = gtk_frame_new(_("Baseline"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);


  table = gtk_table_new(1, 2, FALSE);
  vbox = gtk_vbox_new(FALSE, 4);

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Position:"), FALSE, i++);
  d->arrow = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_inc(w, 1000, 10000);
  add_widget_to_table(table, w, _("_Arrow length:"), FALSE, i++);
  d->arrowlen = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_inc(w, 1000, 10000);
  add_widget_to_table(table, w, _("_Arrow width:"), FALSE, i++);
  d->arrowwid = w;

  frame = gtk_frame_new(_("Arrow"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);


  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Position:"), FALSE, i++);
  d->wave = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Wave length:"), FALSE, i++);
  d->wavelen = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH, -1);
  add_widget_to_table(table, w, _("_Wave width:"), FALSE, i++);
  d->wavewid = w;

  frame = gtk_frame_new(_("Wave"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(baseline_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
gauge_tab_set_value(struct AxisDialog *axis)
{
  int i;
  struct AxisGauge *d;

  d = &axis->gauge;

  if (SetObjFieldFromWidget(d->gauge, axis->Obj, axis->Id, "gauge"))
    return 1;

  if (SetObjFieldFromWidget(d->min, axis->Obj, axis->Id, "gauge_min"))
    return 1;

  if (SetObjFieldFromWidget(d->max, axis->Obj, axis->Id, "gauge_max"))
    return 1;

  if (SetObjFieldFromStyle(d->style, axis->Obj, axis->Id, "gauge_style"))
    return 1;

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1"; 

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromWidget(d->width[i], axis->Obj, axis->Id, width))
      return 1;

    length[sizeof(length) - 2] += i;
    if (SetObjFieldFromWidget(d->length[i], axis->Obj, axis->Id, length))
      return 1;
  }

  if (putobj_color(d->color, axis->Obj, axis->Id, "gauge_"))
    return 1;

  return 0;
}

static void
gauge_tab_setup_item(struct AxisDialog *axis, int id)
{
  int i;
  struct AxisGauge *d;

  d = &axis->gauge;

  SetWidgetFromObjField(d->gauge, axis->Obj, id, "gauge");

  SetWidgetFromObjField(d->min, axis->Obj, id, "gauge_min");

  SetWidgetFromObjField(d->max, axis->Obj, id, "gauge_max");

  SetStyleFromObjField(d->style, axis->Obj, id, "gauge_style");

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1"; 

    width[sizeof(width) - 2] += i;
    SetWidgetFromObjField(d->width[i], axis->Obj, id, width);

    length[sizeof(length) - 2] += i;
    SetWidgetFromObjField(d->length[i], axis->Obj, id, length);
  }

  set_color(d->color, axis->Obj, id, "gauge_");
}

static void
gauge_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    gauge_tab_setup_item(d, sel);
  }
}

static GtkWidget *
gauge_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *parent_box, *w, *vbox, *frame, *table;
  struct AxisGauge *d;
  int i, j;
  char buf[TITLE_BUF_SIZE];

  d = &dd->gauge;

  vbox = gtk_vbox_new(FALSE, 4);

  table = gtk_table_new(1, 2, FALSE);

  j = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Gauge:"), FALSE, j++);
  d->gauge = w;

  w = create_text_entry(FALSE, TRUE);
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 3, -1);
  add_widget_to_table(table, w, _("_Min:"), TRUE, j++);
  d->min = w;

  w = create_text_entry(FALSE, TRUE);
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 3, -1);
  add_widget_to_table(table, w, _("_Max:"), TRUE, j++);
  d->max = w;

  frame = gtk_frame_new(_("Range"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);


  table = gtk_table_new(1, 4, FALSE);

  j = 0;
  w = combo_box_entry_create();
  add_widget_to_table_sub(table, w, _("_Style:"), TRUE, 0, 3, 4, j++);
  d->style = w;

  w = create_color_button(wi);
  add_widget_to_table_sub(table, w, _("_Color:"), FALSE, 0, 1, 4, j++);
  d->color = w;

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    snprintf(buf, sizeof(buf), _("_Width %d:"), i + 1);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
    add_widget_to_table_sub(table, w, buf, TRUE, 0, 1, 4, j);
    d->width[i] = w;

    snprintf(buf, sizeof(buf), _("_Length %d:"), i + 1);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table_sub(table, w, buf, TRUE, 2, 1, 4, j++);
    d->length[i] = w;
  }

  frame = gtk_frame_new(_("Style"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

  parent_box = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(parent_box), vbox, TRUE, TRUE, 4);
  add_copy_button_to_box(parent_box, G_CALLBACK(gauge_tab_copy_clicked), dd, "axis");

  return parent_box;
}

static int
set_num_format(struct AxisDialog *axis, struct AxisNumbering *d)
{ 
  GString *format;
  int a;
  char *new, *old;

  format = g_string_new("%");
  if (format == NULL) {
    return 1;
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->add_plus))) {
    g_string_append_c(format, '+');
  }

  a = combo_box_get_active(d->fraction);
  if (a == 0) {
    g_string_append_c(format, 'g');
  } else if (a > 0) {
    g_string_append_printf(format, ".%df", a - 1);
  }

  if (getobj(axis->Obj, "num_format", axis->Id, 0, NULL, &old) == -1) {
    return 1;
  }

  new = g_string_free(format, FALSE);
  if (g_strcmp0(old, new)) {
    set_graph_modified();
  }

  if (putobj(axis->Obj, "num_format", axis->Id, new) == -1) {
    return 1;
  }
 
  if (getobj(axis->Obj, "num_format", axis->Id, 0, NULL, &new) == -1){
    return 1;
  }

  return 0;
}

static int
numbering_tab_set_value(struct AxisDialog *axis)
{
  struct AxisNumbering *d;

  d = &axis->numbering;

  if (SetObjFieldFromWidget(d->num, axis->Obj, axis->Id, "num"))
    return 1;

  if (SetObjFieldFromWidget(d->begin, axis->Obj, axis->Id, "num_begin"))
    return 1;

  if (SetObjFieldFromWidget(d->step, axis->Obj, axis->Id, "num_step"))
    return 1;

  if (SetObjFieldFromWidget(d->numnum, axis->Obj, axis->Id, "num_num"))
    return 1;

  if (SetObjFieldFromWidget(d->head, axis->Obj, axis->Id, "num_head"))
    return 1;

  if (set_num_format(axis, d))
    return 1;

  if (SetObjFieldFromWidget(d->tail, axis->Obj, axis->Id, "num_tail"))
    return 1;

  if (SetObjFieldFromWidget(d->date_format, axis->Obj, axis->Id, "num_date_format"))
    return 1;

  if (SetObjFieldFromWidget(d->align, axis->Obj, axis->Id, "num_align"))
    return 1;

  if (SetObjFieldFromWidget(d->direction, axis->Obj, axis->Id, "num_direction"))
    return 1;

  if (SetObjFieldFromWidget(d->shiftp, axis->Obj, axis->Id, "num_shift_p"))
    return 1;

  if (SetObjFieldFromWidget(d->shiftn, axis->Obj, axis->Id, "num_shift_n"))
    return 1;

  if (SetObjFieldFromWidget(d->log_power, axis->Obj, axis->Id, "num_log_pow"))
    return 1;

  if (SetObjFieldFromWidget(d->no_zero, axis->Obj, axis->Id, "num_no_zero"))
    return 1;

  if (SetObjFieldFromWidget(d->norm, axis->Obj, axis->Id, "num_auto_norm"))
    return 1;

  return 0;
}

static void
numbering_tab_setup_item(struct AxisDialog *axis, int id)
{
  char *format, *endptr;
  int j, a;
  struct AxisNumbering *d;

  d = &axis->numbering;

  SetWidgetFromObjField(d->num, axis->Obj, id, "num");

  SetWidgetFromObjField(d->begin, axis->Obj, id, "num_begin");

  SetWidgetFromObjField(d->step, axis->Obj, id, "num_step");

  SetWidgetFromObjField(d->numnum, axis->Obj, id, "num_num");

  SetWidgetFromObjField(d->head, axis->Obj, id, "num_head");

  combo_box_clear(d->fraction);
  for (j = 0; j < FwNumStyleNum; j++) {
    combo_box_append_text(d->fraction, _(FwNumStyle[j]));
  }

  getobj(axis->Obj, "num_format", id, 0, NULL, &format);
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

  SetWidgetFromObjField(d->date_format, axis->Obj, id, "num_date_format");

  SetWidgetFromObjField(d->tail, axis->Obj, id, "num_tail");

  SetWidgetFromObjField(d->align, axis->Obj, id, "num_align");

  SetWidgetFromObjField(d->direction, axis->Obj, id, "num_direction");

  SetWidgetFromObjField(d->shiftp, axis->Obj, id, "num_shift_p");

  SetWidgetFromObjField(d->shiftn, axis->Obj, id, "num_shift_n");

  SetWidgetFromObjField(d->log_power, axis->Obj, id, "num_log_pow");

  SetWidgetFromObjField(d->no_zero, axis->Obj, id, "num_no_zero");

  SetWidgetFromObjField(d->norm, axis->Obj, id, "num_auto_norm");
}

static void
numbering_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    numbering_tab_setup_item(d, sel);
  }
}

static void
num_direction_changed(GtkWidget *w, gpointer client_data)
{
  int dir, state;
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;

  dir = combo_box_get_active(w);
  state = (dir != AXIS_NUM_POS_OBLIQUE1 && dir != AXIS_NUM_POS_OBLIQUE2);
  gtk_widget_set_sensitive(d->numbering.align, state);
  gtk_widget_set_sensitive(d->numbering.align_label, state);
}

static GtkWidget *
numbering_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *hbox, *vbox, *frame, *table;
  struct AxisNumbering *d;
  int i;

  d = &dd->numbering;

  hbox = gtk_hbox_new(FALSE, 4);

  vbox = gtk_vbox_new(FALSE, 0);


  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Numbering:"), FALSE, i++);
  d->num = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Begin:"), FALSE, i++);
  d->begin = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Step:"), FALSE, i++);
  d->step = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NUM, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Num:"), FALSE, i++);
  d->numnum = w;

  frame = gtk_frame_new(_("Range"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);


  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = combo_box_create();
  d->align_label = add_widget_to_table(table, w, _("_Align:"), FALSE, i++);
  d->align = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Direction:"), FALSE, i++);
  g_signal_connect(w, "changed", G_CALLBACK(num_direction_changed), dd);
  d->direction = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("shift (_P):"), FALSE, i++);
  d->shiftp = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("shift (_N):"), FALSE, i++);
  d->shiftn = w;

  frame = gtk_frame_new(_("Position"));
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);


  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Fraction:"), FALSE, i++);
  d->fraction = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Head:"), TRUE, i++);
  d->head = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Tail:"), TRUE, i++);
  d->tail = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Date/time format:"), TRUE, i++);
  d->date_format = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Auto normalization:"), FALSE, i++);
  d->norm = w;

  w = gtk_check_button_new_with_mnemonic(_("_Log power"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->log_power = w;

  w = gtk_check_button_new_with_mnemonic(_("_Add plus"));
  d->add_plus = w;
  add_widget_to_table(table, w, NULL, FALSE, i++);

  w = gtk_check_button_new_with_mnemonic(_("no _Zero"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->no_zero = w;

  frame = gtk_frame_new(_("Format"));
  gtk_container_add(GTK_CONTAINER(frame), table);

  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
 
  add_copy_button_to_box(vbox, G_CALLBACK(numbering_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
font_tab_set_value(struct AxisDialog *axis)
{
  struct AxisFont *d;
  int style, bold, italic, old_style;

  d = &axis->font;

  if (SetObjFieldFromWidget(d->space, axis->Obj, axis->Id, "num_space"))
    return 1;

  if (SetObjFieldFromWidget(d->pt, axis->Obj, axis->Id, "num_pt"))
    return 1;

  if (SetObjFieldFromWidget(d->script, axis->Obj, axis->Id, "num_script_size"))
    return 1;

  SetObjFieldFromFontList(d->font, axis->Obj, axis->Id, "num_font");

  if (putobj_color(d->color, axis->Obj, axis->Id, "num_"))
    return 1;

  style = 0;
  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->font_bold));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }

  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->font_italic));
  if (italic) {
    style |= GRA_FONT_STYLE_ITALIC;
  }

  getobj(axis->Obj, "num_font_style", axis->Id, 0, NULL, &old_style);
  if (old_style != style) {
    putobj(axis->Obj, "num_font_style", axis->Id, &style);
    set_graph_modified();
  }

  return 0;
}

static void
font_tab_setup_item(struct AxisDialog *axis, int id)
{
  struct compatible_font_info *compatible;
  struct AxisFont *d;
  int style;

  d = &axis->font;

  SetWidgetFromObjField(d->space, axis->Obj, id, "num_space");

  SetWidgetFromObjField(d->pt, axis->Obj, id, "num_pt");

  SetWidgetFromObjField(d->script, axis->Obj, id, "num_script_size");

  set_color(d->color, axis->Obj, id, "num_");

  compatible = SetFontListFromObj(d->font, axis->Obj, id, "num_font");

  if (compatible) {
    style = compatible->style;
  } else {
    getobj(axis->Obj, "num_font_style", axis->Id, 0, NULL, &style);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->font_bold), style & GRA_FONT_STYLE_BOLD);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->font_italic), style & GRA_FONT_STYLE_ITALIC);
}

static void
font_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    font_tab_setup_item(d, sel);
  }
}

static GtkWidget *
font_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *vbox, *table, *frame, *btn_box;
  struct AxisFont *d;
  int i;

  d = &dd->font;

  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Point:"), FALSE, i++);
  d->pt = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_SPACE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Space:"), FALSE, i++);
  d->space = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Script size:"), FALSE, i++);
  d->script = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Font:"), FALSE, i++);
  d->font = w;

  btn_box = gtk_hbutton_box_new();
  gtk_box_set_spacing(GTK_BOX(btn_box), 10);
  w = gtk_check_button_new_with_label("gtk-bold");
  gtk_button_set_use_stock(GTK_BUTTON(w), TRUE);
  d->font_bold = w;
  gtk_box_pack_start(GTK_BOX(btn_box), w, FALSE, FALSE, 0);

  w = gtk_check_button_new_with_label("gtk-italic");
  gtk_button_set_use_stock(GTK_BUTTON(w), TRUE);
  d->font_italic = w;
  gtk_box_pack_start(GTK_BOX(btn_box), w, FALSE, FALSE, 0);

  add_widget_to_table(table, btn_box, "", FALSE, i++);

  w = create_color_button(wi);
  add_widget_to_table(table, w, _("_Color:"), FALSE, i++);
  d->color = w;

  frame = gtk_frame_new(_("Font"));
  gtk_container_add(GTK_CONTAINER(frame), table);

  vbox = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(font_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
position_tab_set_value(struct AxisDialog *axis)
{
  struct AxisPos *d;

  d = &axis->position;

  if (SetObjFieldFromWidget(d->x, axis->Obj, axis->Id, "x"))
    return 1;

  if (SetObjFieldFromWidget(d->y, axis->Obj, axis->Id, "y"))
    return 1;

  if (SetObjFieldFromWidget(d->len, axis->Obj, axis->Id, "length"))
    return 1;

  if (SetObjFieldFromWidget(d->direction, axis->Obj, axis->Id, "direction"))
    return 1;

  if (SetObjAxisFieldFromWidget(d->adjust, axis->Obj, axis->Id, "adjust_axis"))
    return 1;

  if (SetObjFieldFromWidget(d->adjustpos, axis->Obj, axis->Id, "adjust_position"))
    return 1;

  return 0;
}

static void
position_tab_setup_item(struct AxisDialog *axis, int id)
{
  char *valstr;
  int i, j;
  int lastinst;
  char *name;
  struct AxisPos *d;

  d = &axis->position;

  SetWidgetFromObjField(d->x, axis->Obj, id, "x");

  SetWidgetFromObjField(d->y, axis->Obj, id, "y");

  SetWidgetFromObjField(d->len, axis->Obj, id, "length");

  SetWidgetFromObjField(d->direction, axis->Obj, id, "direction");

  lastinst = chkobjlastinst(axis->Obj);
  combo_box_clear(d->adjust);
  combo_box_append_text(d->adjust, _("none"));
  for (j = 0; j <= lastinst; j++) {
    getobj(axis->Obj, "group", j, 0, NULL, &name);
    name = CHK_STR(name);
    combo_box_append_text(d->adjust, name);
  }

  sgetobjfield(axis->Obj, id, "adjust_axis", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;

  combo_box_entry_set_text(d->adjust, valstr + i);
  g_free(valstr);

  SetWidgetFromObjField(d->adjustpos, axis->Obj, id, "adjust_position");
}

static void
position_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  int sel;

  d = (struct AxisDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, AxisCB);
  if (sel != -1) {
    position_tab_setup_item(d, sel);
  }
}

static GtkWidget *
position_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *vbox, *frame, *table;
  struct AxisPos *d;
  int i;

  d = &dd->position;

  table = gtk_table_new(1, 2, FALSE);

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, "_X:", FALSE, i++);
  d->x = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, "_Y:", FALSE, i++);
  d->y = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Length:"), FALSE, i++);
  d->len = w;

  w = create_direction_entry();
  add_widget_to_table(table, w, _("_Direction:"), FALSE, i++);
  d->direction = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 2, -1);
  g_signal_connect(w, "changed", G_CALLBACK(AxisDialogRef), dd);
  add_widget_to_table(table, w, _("_Adjust:"), FALSE, i++);
  d->adjust = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("Adjust _Position:"), FALSE, i++);
  d->adjustpos = w;


  frame = gtk_frame_new(_("Position"));
  gtk_container_add(GTK_CONTAINER(frame), table);

  vbox = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(position_tab_copy_clicked), dd, "axis");

  return vbox;
}

static void
axis_dialog_show_tab(GtkWidget *w, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  gtk_notebook_set_current_page(d->tab, d->tab_active);
}

static void
AxisDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct AxisDialog *d;
  char *group;
  char title[25];

  d = (struct AxisDialog *) data;
  getobj(d->Obj, "group", d->Id, 0, NULL, &group);
  group = CHK_STR(group);
  snprintf(title, sizeof(title), _("Axis %d %s"), d->Id, group);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *notebook, *w, *label;

    d->del_btn = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), FALSE);
    w = scale_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Scale"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = baseline_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Baseline"));
    d->base.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = gauge_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Gauge"));
    d->gauge.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = numbering_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Numbering"));
    d->numbering.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = font_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Font"));
    d->font.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = position_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Position"));
    d->position.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);


    gtk_box_pack_start(GTK_BOX(d->vbox), notebook, TRUE, TRUE, 4);

    d->tab = GTK_NOTEBOOK(notebook);
    d->tab_active = 0;
    g_signal_connect(notebook, "show", G_CALLBACK(axis_dialog_show_tab), d);
  }

  gtk_widget_set_sensitive(d->del_btn, d->CanDel);

  position_tab_setup_item(d, d->Id);
  font_tab_setup_item(d, d->Id);
  gauge_tab_setup_item(d, d->Id);
  numbering_tab_setup_item(d, d->Id);
  baseline_tab_setup_item(d, d->Id);
  scale_tab_setup_item(d, d->Id);
}

static int
scale_tab_set_value(struct AxisDialog *d)
{
  axis_scale_push(d->Obj, d->Id);

  if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "min")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "max")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->inc, d->Obj, d->Id, "inc")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->div, d->Obj, d->Id, "div")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->scale, d->Obj, d->Id, "type")) {
    return 1;
  }

  if (SetObjAxisFieldFromWidget(d->ref, d->Obj, d->Id, "reference")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->margin, d->Obj, d->Id, "auto_scale_margin")) {
    return 1;
  }

  return 0;
}

static void
AxisDialogClose(GtkWidget *w, void *data)
{
  struct AxisDialog *d;
  int ret;

  d = (struct AxisDialog *) data;

  d->tab_active = gtk_notebook_get_current_page(d->tab);

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (scale_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, 0);
    return;
  }

  if (font_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->font.tab_id);
    return;
  }

  if (numbering_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->numbering.tab_id);
    return;
  }

  if (baseline_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->base.tab_id);
    return;
  }

  if (gauge_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->gauge.tab_id);
    return;
  }

  if (position_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->position.tab_id);
    return;
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
CmAxisNewFrame(GtkAction *w, gpointer client_data)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg, ret;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];

  if (Menulock || Globallock)
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
    if (idg != -1) {
      delobj(obj2, idg);
    }
    delobj(obj, idr);
    delobj(obj, idu);
    delobj(obj, idy);
    delobj(obj, idx);
  } else {
    set_graph_modified();
  }
  AxisWinUpdate(TRUE);
}

void
CmAxisNewSection(GtkAction *w, gpointer client_data)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg, ret, oidx, oidy;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];
  char *ref;

  if (Menulock || Globallock)
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
    if ((ref = (char *) g_malloc(ID_BUF_SIZE)) != NULL) {
      snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
      putobj(obj2, "axis_x", idg, ref);
    }
    getobj(obj, "oid", idy, 0, NULL, &oidy);
    if ((ref = (char *) g_malloc(ID_BUF_SIZE)) != NULL) {
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
  } else {
    set_graph_modified();
  }
  AxisWinUpdate(TRUE);
}

void
CmAxisNewCross(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int idx, idy, ret;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];

  if (Menulock || Globallock)
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
    set_graph_modified();
  AxisWinUpdate(TRUE);
}

void
CmAxisNewSingle(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int id, ret;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    AxisDialog(&DlgAxis, obj, id, TRUE);
    ret = DialogExecute(TopLevel, &DlgAxis);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else
      set_graph_modified();
    AxisWinUpdate(TRUE);
  }
}

void
CmAxisDel(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  CopyDialog(&DlgCopy, obj, -1, AxisCB);

  if (DialogExecute(TopLevel, &DlgCopy) == IDOK && DlgCopy.sel >= 0) {
    AxisDel(DlgCopy.sel);
    set_graph_modified();
    AxisWinUpdate(TRUE);
    FileWinUpdate(TRUE);
  }
}

void
CmAxisUpdate(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int i, ret;

  if (Menulock || Globallock)
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
  } else {
    return;
  }
  AxisDialog(&DlgAxis, obj, i, TRUE);
  if ((ret = DialogExecute(TopLevel, &DlgAxis)) == IDDELETE) {
    AxisDel(i);
  }
  AxisWinUpdate(TRUE);
  FileWinUpdate(TRUE);
}

void
CmAxisZoom(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num, room;
  double zoom, min, max, mid, wd;
  char *argv[4];

  if (Menulock || Globallock)
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
      array = arraydata(&farray);
      for (i = 0; i < num; i++) {
	getobj(obj, "min", array[i], 0, NULL, &min);
	getobj(obj, "max", array[i], 0, NULL, &max);
	wd = (max - min) / 2;
	if (wd != 0) {
	  mid = (min + max) / 2;
	  min = mid - wd * zoom;
	  max = mid + wd * zoom;
	  room = 0;
	  argv[0] = (char *) &min;
	  argv[1] = (char *) &max;
	  argv[2] = (char *) &room;
	  argv[3] = NULL;
	  exeobj(obj, "scale", array[i], 3, argv);
	  set_graph_modified();
	}
      }
      AxisWinUpdate(TRUE);
    }
    arraydel(&farray);
  }
}

static void 
axiswin_scale_clear(GtkMenuItem *item, gpointer user_data)
{
  struct SubWin *d;
  struct objlist *obj;
  int sel;

  if (Menulock || Globallock)
    return;

  obj = chkobject("axis");
  if (obj == NULL)
    return;

  d = (struct SubWin *) user_data;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    d->setup_dialog(d->dialog, d->obj, sel, -1);
    d->select = sel;
    axis_scale_push(obj, sel);
    exeobj(obj, "clear", sel, 0, NULL);
    set_graph_modified();
    d->update(FALSE);
  }
}

void
CmAxisClear(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, AxisCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = 0; i < num; i++) {
      axis_scale_push(obj, array[i]);
      exeobj(obj, "clear", array[i], 0, NULL);
      set_graph_modified();
    }
    AxisWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmAxisGridNew(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int id, ret;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    GridDialog(&DlgGrid, obj, id);
    ret = DialogExecute(TopLevel, &DlgGrid);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else
      set_graph_modified();
  }
}

void
CmAxisGridDel(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int num, *array;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, GridCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, array[i]);
      set_graph_modified();
    }
  }
  arraydel(&farray);
}

void
CmAxisGridUpdate(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i, j, ret;
  int *array, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, GridCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = 0; i < num; i++) {
      GridDialog(&DlgGrid, obj, array[i]);
      if ((ret = DialogExecute(TopLevel, &DlgGrid)) == IDDELETE) {
	delobj(obj, array[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  array[j]--;
      }
    }
  }
  arraydel(&farray);
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
  unsigned int i;
  double min, max, inc;
  char buf[256], *valstr;

  for (i = 0; i < AXIS_WIN_COL_NUM; i++) {
    switch (i) {
    case AXIS_WIN_COL_NAME:
      getobj(d->obj, "group", row, 0, NULL, &valstr);
      if (valstr) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, valstr);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, ".....");
      }
      break;
    case AXIS_WIN_COL_MIN:
      getobj(d->obj, "min", row, 0, NULL, &min);
      break;
    case AXIS_WIN_COL_MAX:
      getobj(d->obj, "max", row, 0, NULL, &max);
      if ((min == 0) && (max == 0)) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i - 1, "---------");
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "---------");
      } else {
	snprintf(buf, sizeof(buf), "%g", min);
	list_store_set_string(GTK_WIDGET(d->text), iter, i - 1, buf);

	snprintf(buf, sizeof(buf), "%g", max);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
      }
      break;
    case AXIS_WIN_COL_TYPE:
      sgetobjfield(d->obj, row, "type", NULL, &valstr, FALSE, FALSE, FALSE);
      list_store_set_string(GTK_WIDGET(d->text), iter, i, _(valstr));
      g_free(valstr);
      break;
    case AXIS_WIN_COL_INC:
      getobj(d->obj, "inc", row, 0, NULL, &inc);
      if (inc == 0) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "---------");
      } else {
	snprintf(buf, sizeof(buf), "%g", inc);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
      }
      break;
    case AXIS_WIN_COL_HIDDEN:
      getobj(d->obj, Alist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Alist[i].type, &cx);
      break;
    default:
      if (Alist[i].type == G_TYPE_DOUBLE) {
	getobj(d->obj, Alist[i].name, row, 0, NULL, &cx);
	list_store_set_double(GTK_WIDGET(d->text), iter, i, cx / 100.0);
      } else {
	getobj(d->obj, Alist[i].name, row, 0, NULL, &cx);
	list_store_set_val(GTK_WIDGET(d->text), iter, i, Alist[i].type, &cx);
      }
    }
  }
}

static gboolean
AxisWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data)
{
  struct SubWin *d;

  if (Menulock || Globallock)
    return FALSE;

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

static int 
check_axis_history(struct objlist *obj)
{
  struct narray *array;
  int num, n, i;

  n = chkobjlastinst(obj);
  if (n < 0)
    return 0;

  num = 0;
  for (i = 0; i <= n; i++) {
    getobj(obj, "scale_history", i, 0, NULL, &array);
    num += arraynum(array) / 3;
  }

  return num;
}

void
CmAxisScaleUndo(GtkAction *w, gpointer client_data)
{
  char *argv[1];
  struct objlist *obj;
  struct narray farray;
  int i, n, num, *array;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (check_axis_history(obj) == 0)
    return;

  SelectDialog(&DlgSelect, obj, AxisHistoryCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      argv[0] = NULL;
      exeobj(obj, "scale_pop", array[i], 0, argv);
      set_graph_modified();
    }
    n = check_axis_history(obj);
    set_axis_undo_button_sensitivity(n > 0);
    AxisWinUpdate(TRUE);
  }
  arraydel(&farray);
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  unsigned int i;
  int sel;
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  sel = d->select;
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      gtk_widget_set_sensitive(d->popup_item[i], sel > 0 && sel <= d->num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel < d->num);
      break;
    case POPUP_ITEM_HIDE:
      if (sel >= 0 && sel <= d->num) {
	int hidden;
	getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(d->popup_item[i]), ! hidden);
      }
    default:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= d->num);
    }
  }
}

static void
select_type(GtkComboBox *w, gpointer user_data)
{
  int j, type, sel;
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  getobj(d->obj, "type", sel, 0, NULL, &type);

  j = combo_box_get_active(GTK_WIDGET(w));
  if (j < 0 || j == type)
    return;

  if (putobj(d->obj, "type", sel, &j) >= 0)
    d->select = sel;
}

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct SubWin *d;
  GtkComboBox *cbox;
  int sel, type;

  menu_lock(TRUE);

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);
  sel = list_store_get_selected_int(GTK_WIDGET(view), AXIS_WIN_COL_ID);

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  SetWidgetFromObjField(GTK_WIDGET(cbox), d->obj, sel, "type");

  getobj(d->obj, "type", sel, 0, NULL, &type);
  combo_box_set_active(GTK_WIDGET(cbox), type);

  d->select = -1;
  g_signal_connect(cbox, "changed", G_CALLBACK(select_type), d);
}

static void
edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct SubWin *d;

  menu_lock(FALSE);

  d = (struct SubWin *) user_data;

  if (str == NULL || d->select < 0)
    return;

  d->update(FALSE);
  set_graph_modified();
}

enum CHANGE_DIR {
  CHANGE_DIR_X,
  CHANGE_DIR_Y,
};

static void
pos_edited_common(struct SubWin *d, int id, char *str, enum CHANGE_DIR dir)
{
  int x, y, pos1, pos2, man, ecode;
  double val;
  char *argv[3];

  if (str == NULL || id < 0)
    return;

  switch (dir) {
  case CHANGE_DIR_X:
    getobj(d->obj, "x", id, 0, NULL, &pos1);
    break;
  case CHANGE_DIR_Y:
    getobj(d->obj, "y", id, 0, NULL, &pos1);
    break;
  }

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  pos2 = nround(val * 100);

  if (pos1 == pos2)
    return;

  switch (dir) {
  case CHANGE_DIR_X:
    x = (pos2 - pos1);
    y = 0;
    break;
  case CHANGE_DIR_Y:
    x = 0;
    y = (pos2 - pos1);
    break;
  }
 
  argv[0] = (char *) &x;
  argv[1] = (char *) &y;
  argv[2] = NULL;

  getobj(d->obj, "group_manager", id, 0, NULL, &man);
  if (man >= 0) {
    exeobj(d->obj, "move", man, 2, argv);

    set_graph_modified();
    AxisWinUpdate(TRUE);
  }
}

static void
pos_x_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct SubWin *d;
  int sel;

  menu_lock(FALSE);

  d = (struct SubWin *) user_data;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  pos_edited_common(d, sel, str, CHANGE_DIR_X);
}

static void
pos_y_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct SubWin *d;
  int sel;

  menu_lock(FALSE);

  d = (struct SubWin *) user_data;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  pos_edited_common(d, sel, str, CHANGE_DIR_Y);
}

static void
axis_prm_edited_common(struct SubWin *d, char *field, gchar *str)
{
  int sel;

  menu_lock(FALSE);

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if (sel < 0 || sel > d->num)
    return;

  axis_scale_push(d->obj, sel);

  if (chk_sputobjfield(d->obj, sel, field, str))
    return;

  d->select = sel;
  d->update(FALSE);
}

static void
min_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  axis_prm_edited_common((struct SubWin *) user_data, "min", str);
}

static void
max_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  axis_prm_edited_common((struct SubWin *) user_data, "max", str);
}

static void
inc_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  axis_prm_edited_common((struct SubWin *) user_data, "inc", str);
}

static void 
axiswin_delete_axis(struct SubWin *d)
{
  int sel;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    AxisDel(sel);
    AxisWinUpdate(TRUE);
    FileWinUpdate(TRUE);
    set_graph_modified();
    d->select = -1;
  }
}

static void
axis_delete_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  axiswin_delete_axis(d);
}

static void
AxisWinAxisTop(GtkWidget *w, gpointer client_data)
{
  int sel;
  struct SubWin *d;

  d = (struct SubWin *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if ((sel  >=  0) && (sel <= d->num)) {
    movetopobj(d->obj, sel);
    d->select = 0;
    AxisMove(sel,0);
    AxisWinUpdate(FALSE);
    FileWinUpdate(FALSE);
    set_graph_modified();
  }
}

static void
AxisWinAxisLast(GtkWidget *w, gpointer client_data)
{
  int sel;
  struct SubWin *d;

  d = (struct SubWin *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    movelastobj(d->obj, sel);
    d->select = d->num;
    AxisMove(sel, d->num);
    AxisWinUpdate(FALSE);
    FileWinUpdate(FALSE);
    set_graph_modified();
  }
}

static void 
AxisWinAxisUp(GtkWidget *w, gpointer client_data)
{
  int sel;
  struct SubWin *d;

  d = (struct SubWin *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if ((sel >= 1) && (sel <= d->num)) {
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    AxisMove(sel, sel - 1);
    AxisWinUpdate(FALSE);
    FileWinUpdate(FALSE);
    set_graph_modified();
  }
}

static void 
AxisWinAxisDown(GtkWidget *w, gpointer client_data)
{
  int sel;
  struct SubWin *d;

  d = (struct SubWin *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), AXIS_WIN_COL_ID);

  if (sel >= 0 && sel <= d->num-1) {
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    AxisMove(sel, sel + 1);
    AxisWinUpdate(FALSE);
    FileWinUpdate(FALSE);
    set_graph_modified();
  }
}

static gboolean
axiswin_ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct SubWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || Globallock)
    return TRUE;

  d = (struct SubWin *) user_data;
  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_Delete:
    axiswin_delete_axis(d);
    break;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK)
      AxisWinAxisTop(w, d);
    else
      return FALSE;
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK)
      AxisWinAxisLast(w, d);
    else
      return FALSE;
    break;
  case GDK_Up:
    if (e->state & GDK_SHIFT_MASK)
      AxisWinAxisUp(w, d);
    else
      return FALSE;
    break;
  case GDK_Down:
    if (e->state & GDK_SHIFT_MASK)
      AxisWinAxisDown(w, d);
    else
      return FALSE;
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void
CmAxisWindow(GtkToggleAction *action, gpointer client_data)
{
  struct SubWin *d;
  int state;
  GtkWidget *dlg;

  d = &(NgraphApp.AxisWin);

  if (action) {
    state = gtk_toggle_action_get_active(action);
  } else {
    state = TRUE;
  }

  if (d->Win) {
    sub_window_set_visibility(d, state);
    return;
  }

  if (! state) {
    return;
  }

  d->update = AxisWinUpdate;
  d->setup_dialog = AxisDialog;
  d->dialog = &DlgAxis;
  d->ev_key = axiswin_ev_key_down;
  d->delete = AxisDel;

  dlg = list_sub_window_create(d, "Axis Window", AXIS_WIN_COL_NUM, Alist, Axiswin_xpm, Axiswin48_xpm);

  g_signal_connect(dlg, "expose-event", G_CALLBACK(AxisWinExpose), NULL);

  d->obj = chkobject("axis");
  d->num = chkobjlastinst(d->obj);

  sub_win_create_popup_menu(d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
  set_combo_cell_renderer_cb(d, AXIS_WIN_COL_TYPE, Alist, G_CALLBACK(start_editing), G_CALLBACK(edited));
  set_editable_cell_renderer_cb(d, AXIS_WIN_COL_X, Alist, G_CALLBACK(pos_x_edited));
  set_editable_cell_renderer_cb(d, AXIS_WIN_COL_Y, Alist, G_CALLBACK(pos_y_edited));
  set_editable_cell_renderer_cb(d, AXIS_WIN_COL_MIN, Alist, G_CALLBACK(min_edited));
  set_editable_cell_renderer_cb(d, AXIS_WIN_COL_MAX, Alist, G_CALLBACK(max_edited));
  set_editable_cell_renderer_cb(d, AXIS_WIN_COL_INC, Alist, G_CALLBACK(inc_edited));

  list_store_set_align(GTK_WIDGET(d->text), AXIS_WIN_COL_MIN, 1.0);
  list_store_set_align(GTK_WIDGET(d->text), AXIS_WIN_COL_MAX, 1.0);
  list_store_set_align(GTK_WIDGET(d->text), AXIS_WIN_COL_INC, 1.0);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(d->text), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(d->text), AXIS_WIN_COL_NAME);
  gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(d->text), AXIS_WIN_COL_NAME);

  sub_window_show_all(d);
  sub_window_set_geometry(d, TRUE);
}
