// -*- coding: utf-8 -*-
/* 
 * $Id: x11file.c,v 1.112 2009/08/12 02:58:17 hito Exp $
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
#include <fcntl.h>
#include <math.h>

#include "gtk_entry_completion.h"
#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathcode.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"
#include "nconfig.h"
#include "ofile.h"
#include "ofit.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "ox11menu.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11file.h"
#include "x11commn.h"

static n_list_store Flist[] = {
  {"",	        G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden",     FALSE},
  {"#",		G_TYPE_INT,     TRUE, FALSE, "id",         FALSE},
  {N_("file"),	G_TYPE_STRING,  TRUE, TRUE,  "file",       TRUE},
  {"x   ",	G_TYPE_INT,     TRUE, TRUE,  "x",          FALSE,  0, 999, 1, 10},
  {"y   ",	G_TYPE_INT,     TRUE, TRUE,  "y",          FALSE,  0, 999, 1, 10},
  {N_("ax"),	G_TYPE_ENUM,    TRUE, TRUE,  "axis_x",     FALSE},
  {N_("ay"),	G_TYPE_ENUM,    TRUE, TRUE,  "axis_y",     FALSE},
  {N_("type"),	G_TYPE_OBJECT,  TRUE, TRUE,  "type",       FALSE},
  {N_("size"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "mark_size",  FALSE,  0, SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "line_width", FALSE,  0, SPIN_ENTRY_MAX, 10,   100},
  {N_("skip"),	G_TYPE_INT,     TRUE, TRUE,  "head_skip",  FALSE,  0, INT_MAX,         1,    10},
  {N_("step"),	G_TYPE_INT,     TRUE, TRUE,  "read_step",  FALSE,  1, INT_MAX,         1,    10},
  {N_("final"),	G_TYPE_INT,     TRUE, TRUE,  "final_line", FALSE, -1, INT_MAX,         1,    10},
  {N_("num"), 	G_TYPE_INT,     TRUE, FALSE, "data_num",   FALSE},
  {"^#",	G_TYPE_INT,     TRUE, FALSE, "oid",        FALSE},
};

#define FILE_WIN_COL_NUM (sizeof(Flist)/sizeof(*Flist))
#define FILE_WIN_COL_OID (FILE_WIN_COL_NUM - 1)
#define FILE_WIN_COL_HIDDEN 0
#define FILE_WIN_COL_ID     1
#define FILE_WIN_COL_FILE   2
#define FILE_WIN_COL_X      3
#define FILE_WIN_COL_Y      4
#define FILE_WIN_COL_X_AXIS 5
#define FILE_WIN_COL_Y_AXIS 6
#define FILE_WIN_COL_TYPE   7


static void file_delete_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy2_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_fit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_edit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_draw_popup_func(GtkMenuItem *w, gpointer client_data);

static struct subwin_popup_list Popup_list[] = {
  {GTK_STOCK_OPEN,            G_CALLBACK(CmFileOpenB), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Duplicate"),          G_CALLBACK(file_copy_popup_func), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("duplicate _Behind"),   G_CALLBACK(file_copy2_popup_func), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_CLOSE,           G_CALLBACK(file_delete_popup_func), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Draw"),               G_CALLBACK(file_draw_popup_func), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Show"),               G_CALLBACK(list_sub_window_hide), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_CHECK},
  {GTK_STOCK_PROPERTIES,      G_CALLBACK(list_sub_window_update), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_EDIT,            G_CALLBACK(file_edit_popup_func), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Fit"),                G_CALLBACK(file_fit_popup_func), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {GTK_STOCK_GOTO_TOP,        G_CALLBACK(list_sub_window_move_top), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_UP,           G_CALLBACK(list_sub_window_move_up), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_DOWN,         G_CALLBACK(list_sub_window_move_down), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GOTO_BOTTOM,     G_CALLBACK(list_sub_window_move_last), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))
#define POPUP_ITEM_HIDE 7
#define POPUP_ITEM_FIT 10
#define POPUP_ITEM_TOP 12
#define POPUP_ITEM_UP 13
#define POPUP_ITEM_DOWN 14
#define POPUP_ITEM_BOTTOM 15

#define FITSAVE "fit.ngp"
#define CB_BUF_SIZE 128

enum MATH_FNC_TYPE {
  TYPE_MATH_X = 0,
  TYPE_MATH_Y,
  TYPE_FUNC_F,
  TYPE_FUNC_G,
  TYPE_FUNC_H,
};

static char *FieldStr[] = {"math_x", "math_y", "func_f", "func_g", "func_h"};

static gboolean FileWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data);

static void
MathTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct MathTextDialog *d;
  static char *label[] = {N_("_Math X:"), N_("_Math Y:"), "_F(X,Y,Z):", "_G(X,Y,Z):", "_H(X,Y,Z):"};

  d = (struct MathTextDialog *) data;
  d->math = NULL;
  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    d->label = item_setup(hbox, w, _("_Math:"), TRUE);
    d->list = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  switch (d->Mode) {
  case TYPE_MATH_X:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.x_math_list);
    break;
  case TYPE_MATH_Y:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.y_math_list);
    break;
  case TYPE_FUNC_F:
  case TYPE_FUNC_G:
  case TYPE_FUNC_H:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.func_list);
    break;
  }

  gtk_label_set_text_with_mnemonic(GTK_LABEL(d->label), _(label[d->Mode]));
  gtk_entry_set_text(GTK_ENTRY(d->list), d->Text);
  gtk_window_set_default_size(GTK_WINDOW(wi), 400, -1);
}

static void
MathTextDialogClose(GtkWidget *w, void *data)
{
  struct MathTextDialog *d;
  const char *p;

  d = (struct MathTextDialog *) data;

  if (d->ret == IDCANCEL)
    return;

  p = gtk_entry_get_text(GTK_ENTRY(d->list));
  d->math = strdup(p);
  switch (d->Mode) {
  case TYPE_MATH_X:
    entry_completion_append(NgraphApp.x_math_list, p);
    break;
  case TYPE_MATH_Y:
    entry_completion_append(NgraphApp.y_math_list, p);
    break;
  case TYPE_FUNC_F:
  case TYPE_FUNC_G:
  case TYPE_FUNC_H:
    entry_completion_append(NgraphApp.func_list, p);
    break;
  }
  gtk_entry_set_completion(GTK_ENTRY(d->list), NULL);
}

void
MathTextDialog(struct MathTextDialog *data, char *text, int mode)
{
  if (mode < 0 || mode >= MATH_FNC_NUM)
    mode = 0;

  data->SetupWindow = MathTextDialogSetup;
  data->CloseWindow = MathTextDialogClose;
  data->Text = text;
  data->Mode = mode;
}

static void
MathDialogSetupItem(GtkWidget *w, struct MathDialog *d)
{
  int i;
  char *math, *field = NULL;
  GtkTreeIter iter;

  list_store_clear(d->list);

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    math = NULL;
    getobj(d->Obj, field, i, 0, NULL, &math);
    list_store_append(d->list, &iter);
    list_store_set_int(d->list, &iter, 0, i);
    list_store_set_string(d->list, &iter, 1, CHK_STR(math));
  }

  if (d->Mode >= 0 && d->Mode < MATH_FNC_NUM)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->func[d->Mode]), TRUE);
}

static void
MathDialogMode(GtkWidget *w, gpointer client_data)
{
  struct MathDialog *d;
  int i;
  
  if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    return;

  d = (struct MathDialog *) client_data;

  for (i = 0; i < MATH_FNC_NUM; i++) {
    if (w == d->func[i])
      d->Mode = i;
  }
  MathDialogSetupItem(d->widget, d);
}

static void
MathDialogList(GtkButton *w, gpointer client_data)
{
  struct MathDialog *d;
  int a, *ary, r;
  char *field = NULL, *buf, *obuf;
  GtkTreeSelection *gsel;
  GtkTreePath *path;
  GList *list, *data;

  d = (struct MathDialog *) client_data;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  list = gtk_tree_selection_get_selected_rows(gsel, NULL);

  if (list == NULL)
    return;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(d->list), &path, NULL);

  if (path) {
    r = list_store_path_get_int(d->list, path, 0, &a);
    gtk_tree_path_free(path);
  } else {
    data = g_list_last(list);
    r = list_store_path_get_int(d->list, data->data, 0, &a);
  }

  if (r)
    goto END;

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  sgetobjfield(d->Obj, a, field, NULL, &buf, FALSE, FALSE, FALSE);
  if (buf == NULL)
    goto END;

  MathTextDialog(&DlgMathText, buf, d->Mode);

  if (DialogExecute(d->widget, &DlgMathText) == IDOK) {
    for (data = list; data; data = data->next) {
      r = list_store_path_get_int(d->list, data->data, 0, &a);
      if (r)
	continue;

      sgetobjfield(d->Obj, a, field, NULL, &obuf, FALSE, FALSE, FALSE);
      if (obuf == NULL || strcmp(obuf, DlgMathText.math)) {
	sputobjfield(d->Obj, a, field, DlgMathText.math);
	set_graph_modified();
      }
      memfree(obuf);
    }
    free(DlgMathText.math);
  }
  memfree(buf);

  MathDialogSetupItem(d->widget, d);

  for (data = list; data; data = data->next) {
    ary = gtk_tree_path_get_indices(data->data);
    if (ary == NULL)
      continue;

    gtk_tree_selection_select_path(gsel, data->data);
  }

 END:
  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);
}

static gboolean
math_dialog_key_pressed_cb(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct MathDialog *d;
  GtkTreeSelection *gsel;
  int n;

  d = (struct MathDialog *) user_data;

  if (e->keyval != GDK_Return)
    return FALSE;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));

  n = gtk_tree_selection_count_selected_rows(gsel);
  if (n < 1)
    return FALSE;

  MathDialogList(NULL, d);

  return TRUE;
}

static gboolean
math_dialog_butten_pressed_cb(GtkWidget *w, GdkEventButton *e, gpointer user_data)
{
  struct MathDialog *d;

  d = (struct MathDialog *) user_data;

  if (e->type != GDK_2BUTTON_PRESS)
    return FALSE;

  MathDialogList(NULL, d);

  return TRUE;
}

static void
set_btn_sensitivity_delete_cb(GtkTreeModel *tree_model, GtkTreePath *path, gpointer user_data)
{
  int n;
  GtkWidget *w;

  w = GTK_WIDGET(user_data);
  n = gtk_tree_model_iter_n_children(tree_model, NULL);
  gtk_widget_set_sensitive(w, n > 0);
}

static void
set_btn_sensitivity_insert_cb(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
  set_btn_sensitivity_delete_cb(tree_model, path, user_data);
}

static void
set_sensitivity_by_row_num(GtkWidget *tree, GtkWidget *btn)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));;
  g_signal_connect(model, "row-deleted", G_CALLBACK(set_btn_sensitivity_delete_cb), btn);
  g_signal_connect(model, "row-inserted", G_CALLBACK(set_btn_sensitivity_insert_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static gboolean 
set_btn_sensitivity_selection_cb(GtkTreeSelection *sel, gpointer user_data)
{
  int n;
  GtkWidget *w;

  w = GTK_WIDGET(user_data);
  n = gtk_tree_selection_count_selected_rows(sel);
  gtk_widget_set_sensitive(w, n > 0);

  return FALSE;
}

static void
set_sensitivity_by_selection(GtkWidget *tree, GtkWidget *btn)
{
  GtkTreeSelection *sel;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));;
  g_signal_connect(sel, "changed", G_CALLBACK(set_btn_sensitivity_selection_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static void
MathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *vbox, *hbox;
  struct MathDialog *d;
  static n_list_store list[] = {
    {"id",       G_TYPE_INT,    TRUE, FALSE, NULL, FALSE},
    {N_("math"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };
  int i;

  d = (struct MathDialog *) data;

  if (makewidget) {
    char *button_str[] = {
      N_("_X math"),
      N_("_Y math"),
      "_F(X, Y, Z)",
      "_G(X, Y, Z)",
      "_H(X, Y, Z)",
    };

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_all(w);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(math_dialog_key_pressed_cb), d);
    g_signal_connect(w, "button-press-event", G_CALLBACK(math_dialog_butten_pressed_cb), d);
    d->list = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);

    w = NULL;
    for (i = 0; i < MATH_FNC_NUM; i++) {
      w = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(w), _(button_str[i]));
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
      d->func[i] = w;
      g_signal_connect(w, "toggled", G_CALLBACK(MathDialogMode), d);
    }

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    set_sensitivity_by_row_num(d->list, w);

    w = gtk_button_new_from_stock(GTK_STOCK_EDIT);
    g_signal_connect(w, "clicked", G_CALLBACK(MathDialogList), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    set_sensitivity_by_selection(d->list, w);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 4);

    d->show_cancel = FALSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }

  MathDialogSetupItem(wi, d);
}

static void
MathDialogClose(GtkWidget *w, void *data)
{
}

void
MathDialog(struct MathDialog *data, struct objlist *obj)
{
  data->SetupWindow = MathDialogSetup;
  data->CloseWindow = MathDialogClose;
  data->Obj = obj;
  data->Mode = 0;
}

static void
FitLoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  char *s;
  struct FitLoadDialog *d;
  int i;
  GtkWidget *w;

  d = (struct FitLoadDialog *) data;
  if (makewidget) {
    w = combo_box_create();
    d->list = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);
  }
  combo_box_clear(d->list);
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    combo_box_append_text(d->list, CHK_STR(s));
  }
  combo_box_set_active(d->list, 0);
  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(d->list, XmNwidth, 200, NULL);
  }
  */
}

static void
FitLoadDialogClose(GtkWidget *w, void *data)
{
  struct FitLoadDialog *d;

  d = (struct FitLoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
  d->sel = combo_box_get_active(d->list);
}

void
FitLoadDialog(struct FitLoadDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitLoadDialogSetup;
  data->CloseWindow = FitLoadDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->sel = -1;
}

static void
FitSaveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct FitSaveDialog *d;
  int i, j;
  char *s;

  d = (struct FitSaveDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

    hbox = gtk_hbox_new(FALSE, 4);
    item_setup(hbox, w, _("_Profile:"), TRUE);
    d->profile = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
  }
  combo_box_clear(d->profile);
  j = 0;
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    combo_box_append_text(d->profile, CHK_STR(s));
  }
  combo_box_entry_set_text(d->profile, "");
}

static void
FitSaveDialogClose(GtkWidget *w, void *data)
{
  struct FitSaveDialog *d;
  const char *s;

  d = (struct FitSaveDialog *) data;

  if (d->ret != IDOK && d->ret != IDDELETE)
    return;

  s = combo_box_entry_get_text(d->profile);
  if (s) {
    char *ptr;

    ptr = nstrdup(s);
    g_strstrip(ptr);
    if (ptr[0] != '\0') {
      d->Profile = ptr;
      return;
    }
    memfree(ptr);
  }

  MessageBox(d->widget, _("Please specify the profile."), NULL, MB_OK);

  d->ret = IDLOOP;
  return;
}

void
FitSaveDialog(struct FitSaveDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitSaveDialogSetup;
  data->CloseWindow = FitSaveDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->Profile = NULL;
}

static void
FitDialogSetupItem(GtkWidget *w, struct FitDialog *d, int id)
{
  int a, i;

  SetWidgetFromObjField(d->type, d->Obj, id, "type");

  getobj(d->Obj, "poly_dimension", id, 0, NULL, &a);
  combo_box_set_active(d->dim, a - 1);

  SetWidgetFromObjField(d->weight, d->Obj, id, "weight_func");

  SetWidgetFromObjField(d->through_point, d->Obj, id, "through_point");

  SetWidgetFromObjField(d->x, d->Obj, id, "point_x");

  SetWidgetFromObjField(d->y, d->Obj, id, "point_y");

  SetWidgetFromObjField(d->min, d->Obj, id, "min");

  SetWidgetFromObjField(d->max, d->Obj, id, "max");

  SetWidgetFromObjField(d->div, d->Obj, id, "div");

  SetWidgetFromObjField(d->interpolation, d->Obj, id, "interpolation");

  SetWidgetFromObjField(d->formula, d->Obj, id, "user_func");

  SetWidgetFromObjField(d->converge, d->Obj, id, "converge");

  SetWidgetFromObjField(d->derivatives, d->Obj, id, "derivative");

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    SetWidgetFromObjField(d->p[i], d->Obj, id, p);
    SetWidgetFromObjField(d->d[i], d->Obj, id, dd);
  }
}

static char *
FitCB(struct objlist *obj, int id)
{
  char *valstr;

  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);

  return valstr;
}

static void
FitDialogCopy(  struct FitDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FitCB)) != -1)
    FitDialogSetupItem(d->widget, d, sel);
}

static int
FitDialogLoadConfig(struct FitDialog *d, int errmes)
{
  int lastid;
  int newid;
  struct objlist *shell;
  struct narray sarray;
  char *argv[2];
  char *file;

  lastid = chkobjlastinst(d->Obj);
  if (lastid == d->Lastid) {
    if ((file = searchscript(FITSAVE)) == NULL) {
      if (errmes)
	MessageBox(d->widget, _("Setting file not found."), FITSAVE, MB_OK);
      return FALSE;
    }
    if ((shell = chkobject("shell")) == NULL)
      return FALSE;
    newid = newobj(shell);
    if (newid < 0) {
      memfree(file);
      return FALSE;
    }
    arrayinit(&sarray, sizeof(char *));
    changefilename(file);
    if (arrayadd(&sarray, &file) == NULL) {
      memfree(file);
      arraydel2(&sarray);
      return FALSE;
    }
    argv[0] = (char *) &sarray;
    argv[1] = NULL;
    exeobj(shell, "shell", newid, 1, argv);
    arraydel2(&sarray);
    delobj(shell, newid);
  }
  return TRUE;
}

static void
FitDialogLoad(struct FitDialog *d)
{
  int lastid, id;

  if (!FitDialogLoadConfig(d, TRUE))
    return;
  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    MessageBox(d->widget, _("No settings."), FITSAVE, MB_OK);
    return;
  }
  FitLoadDialog(&DlgFitLoad, d->Obj, d->Lastid + 1);
  if ((DialogExecute(d->widget, &DlgFitLoad) == IDOK)
      && (DlgFitLoad.sel >= 0)) {
    id = DlgFitLoad.sel + d->Lastid + 1;
    FitDialogSetupItem(d->widget, d, id);
  }
}

static int
copy_settings_to_fitobj(struct FitDialog *d, char *profile)
{
  int i, id, num;
  char *s;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      if (MessageBox(d->widget, _("Overwrite existing setting?"), "Confirm",
		     MB_YESNO) != IDYES) {
	return 1;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    id = newobj(d->Obj);
  } else {
    id = i;
  }

  if (putobj(d->Obj, "profile", id, profile) == -1)
    return 1;

  if (SetObjFieldFromWidget(d->type, d->Obj, id, "type"))
    return 1;

  num = combo_box_get_active(d->dim);
  num++;
  if (num > 0 && putobj(d->Obj, "poly_dimension", id, &num) == -1)
    return 1;

  if (SetObjFieldFromWidget(d->weight, d->Obj, id, "weight_func"))
    return 1;

  if (SetObjFieldFromWidget
      (d->through_point, d->Obj, id, "through_point"))
    return 1;

  if (SetObjFieldFromWidget(d->x, d->Obj, id, "point_x"))
    return 1;

  if (SetObjFieldFromWidget(d->y, d->Obj, id, "point_y"))
    return 1;

  if (SetObjFieldFromWidget(d->min, d->Obj, id, "min"))
    return 1;

  if (SetObjFieldFromWidget(d->max, d->Obj, id, "max"))
    return 1;

  if (SetObjFieldFromWidget(d->div, d->Obj, id, "div"))
    return 1;

  if (SetObjFieldFromWidget(d->interpolation, d->Obj, id,
			    "interpolation"))
    return 1;
  if (SetObjFieldFromWidget(d->formula, d->Obj, id, "user_func"))
    return 1;

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, id, "derivative"))
    return 1;

  if (SetObjFieldFromWidget(d->converge, d->Obj, id, "converge"))
    return 1;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, id, p))
      return 1;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, id, dd))
      return 1;
  }

  return 0;
}

static int
delete_fitobj(struct FitDialog *d, char *profile)
{
  int i;
  char *s;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      if (MessageBox(d->widget, _("Delete existing setting?"), "Confirm", MB_YESNO) != IDYES) {
	return 1;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    MessageBox(d->widget, _("The profile is not exist."), "Confirm", MB_OK);
    return 1;
  }

  delobj(d->Obj, i);

  return 0;
}

static void
FitDialogSave(struct FitDialog *d)
{
  int i, r, len;
  char *s, *ngpfile;
  int error;
  HANDLE hFile;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  r = DialogExecute(d->widget, &DlgFitSave);
  if (r != IDOK && r != IDDELETE)
    return;

  if (DlgFitSave.Profile == NULL)
    return;

  if (DlgFitSave.Profile[0] == '\0') {
    memfree(DlgFitSave.Profile);
    return;
  }

  switch (r) {
  case IDOK:
    if (copy_settings_to_fitobj(d, DlgFitSave.Profile)) {
      memfree(DlgFitSave.Profile);
      return;
    }
    break;
  case IDDELETE:
    if (delete_fitobj(d, DlgFitSave.Profile)) {
      memfree(DlgFitSave.Profile);
      return;
    }
    memfree(DlgFitSave.Profile);
    break;
  }

  ngpfile = getscriptname(FITSAVE);
  if (ngpfile == NULL) {
    return;
  }

  error = FALSE;

  hFile = nopen(ngpfile, O_CREAT | O_TRUNC | O_RDWR, NFMODE_NORMAL_FILE);
  if (hFile < 0) {
    error = TRUE;
  } else {
    for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
      getobj(d->Obj, "save", i, 0, NULL, &s);
      len = strlen(s);

      if (len != nwrite(hFile, s, len))
	error = TRUE;

      if (nwrite(hFile, "\n", 1) != 1)
	error = TRUE;
    }
    nclose(hFile);
  }

  if (error)
    ErrorMessage();

  memfree(ngpfile);
}

static void
FitDialogResult(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;
  double derror, correlation, coe[10];
  char *equation, *math, *code, *inst, buf[1024];
  int i, j, dim, dimension, maxdim, need2pass, *needdata, tbl[10], type, num;
  struct narray needarray;

  d = (struct FitDialog *) client_data;

  if ((inst = chkobjinst(d->Obj, d->Id)) == NULL)
    return;

  if (_getobj(d->Obj, "type", inst, &type))
    return;

  if (_getobj(d->Obj, "poly_dimension", inst, &dimension))
    return;

  if (_getobj(d->Obj, "number", inst, &num))
    return;

  if (_getobj(d->Obj, "error", inst, &derror))
    return;

  if (_getobj(d->Obj, "correlation", inst, &correlation))
    return;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%00";

    p[sizeof(p) - 2] += i;

    if (_getobj(d->Obj, p, inst, coe + i)) {
      return;
    }
  }

  if (_getobj(d->Obj, "equation", inst, &equation))
    return;

  if (_getobj(d->Obj, "user_func", inst, &math))
    return;

  if (equation == NULL) {
    snprintf(buf, sizeof(buf), "Undefined");
  } else if (type != 4) {
    i = 0;

    if (type == 0) {
      dim = dimension + 1;
    } else {
      dim = 2;
    }

    if (type == 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: %%0i*X^i (i=0-%d)\n\n", dim - 1);
    } else if (type == 1) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: exp(%%00)*X^%%01\n\n");
    } else if (type == 2) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: exp(%%01*X+%%00)\n\n");
    } else if (type == 3) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: %%01*Ln(X)+%%00\n\n");
    }

    for (j = 0; j < dim; j++) {
      i += snprintf(buf + i, sizeof(buf) - i, "       %%0%d = %.7e\n", j, coe[j]);
    }
    i += snprintf(buf + i, sizeof(buf) - i, "\n");
    i += snprintf(buf + i, sizeof(buf) - i, "    points = %d\n", num);
    i += snprintf(buf + i, sizeof(buf) - i, "    <DY^2> = %.7e\n", derror);

    if (correlation >= 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = %.7e\n", correlation);
    } else {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = -------------\n");
    }
  } else {
    arrayinit(&needarray, sizeof(int));
    mathcode(math, &code, &needarray, NULL, &maxdim, &need2pass,
	     TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,
	     FALSE, FALSE);
    memfree(code);
    dim = arraynum(&needarray);
    needdata = (int *) arraydata(&needarray);
    for (i = 0; i < dim; i++) {
      tbl[i] = needdata[i];
    }
    arraydel(&needarray);

    i = 0;
    i += snprintf(buf + i, sizeof(buf) - i, "Eq: User defined\n\n");

    for (j = 0; j < dim; j++) {
      i += snprintf(buf + i, sizeof(buf) - i, "       %%0%d = %.7e\n", tbl[j], coe[tbl[j]]);
    }
    i += snprintf(buf + i, sizeof(buf) - i, "\n");
    i += snprintf(buf + i, sizeof(buf) - i, "    points = %d\n", num);
    i += snprintf(buf + i, sizeof(buf) - i, "    <DY^2> = %.7e\n", derror);

    if (correlation >= 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = %.7e\n", correlation);
    } else {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = -------------\n");
    }
  }
  MessageBox(d->widget, buf, _("Fitting Results"), MB_OK);
}

static int
FitDialogApply(GtkWidget *w, struct FitDialog *d)
{
  int i, num, dim;

  if (SetObjFieldFromWidget(d->type, d->Obj, d->Id, "type"))
    return FALSE;

  if (getobj(d->Obj, "poly_dimension", d->Id, 0, NULL, &dim) == -1)
    return FALSE;

  num = combo_box_get_active(d->dim);
  num++;
  if (num > 0 && putobj(d->Obj, "poly_dimension", d->Id, &num) == -1)
    return FALSE;

  if (num != dim)
    set_graph_modified();

  if (SetObjFieldFromWidget(d->weight, d->Obj, d->Id, "weight_func"))
    return FALSE;

  if (SetObjFieldFromWidget(d->through_point, d->Obj, d->Id, "through_point"))
    return FALSE;

  if (SetObjFieldFromWidget(d->x, d->Obj, d->Id, "point_x"))
    return FALSE;

  if (SetObjFieldFromWidget(d->y, d->Obj, d->Id, "point_y"))
    return FALSE;

  if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "min"))
    return FALSE;

  if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "max"))
    return FALSE;

  if (SetObjFieldFromWidget(d->div, d->Obj, d->Id, "div"))
    return FALSE;

  if (SetObjFieldFromWidget(d->interpolation, d->Obj, d->Id, "interpolation"))
    return FALSE;

  if (SetObjFieldFromWidget(d->formula, d->Obj, d->Id, "user_func"))
    return FALSE;

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, d->Id, "derivative"))
    return FALSE;

  if (SetObjFieldFromWidget(d->converge, d->Obj, d->Id, "converge"))
    return FALSE;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, d->Id, p))
      return FALSE;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, d->Id, dd))
      return FALSE;
  }

  return TRUE;
}

static void
FitDialogDraw(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;
  if (!FitDialogApply(d->widget, d))
    return;
  FitDialogSetupItem(d->widget, d, d->Id);
  Draw(FALSE);
}

static void
set_fitdialog_sensitivity(struct FitDialog *d, int type, int through)
{
  gtk_widget_set_sensitive(d->dim_box, type == 0);
  gtk_widget_set_sensitive(d->usr_def_frame, FALSE);
  gtk_widget_set_sensitive(d->through_box, through);
  gtk_widget_set_sensitive(d->through_point, TRUE);
}

static void
FitDialogSetSensitivity(GtkWidget *widget, gpointer user_data)
{
  struct FitDialog *d;
  int type, through, deriv, intp, dim;
  char buf[1024];

  d = (struct FitDialog *) user_data;

  type = combo_box_get_active(d->type);
  through = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->through_point));
  intp =  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->interpolation));

  switch (type) {
  case FIT_TYPE_POLY:
    dim = combo_box_get_active(d->dim);

    snprintf(buf, sizeof(buf), "Equation: <i>∑ a<sub>i</sub>·</i>X<sup><i>i</i></sup> (<i>i=0-%d</i>)", dim + 1);
    gtk_label_set_markup(GTK_LABEL(d->func_label), buf);
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_POW:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: <i>a·</i>X<i><sup>b</sup></i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_EXP:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: <i>e</i><sup><i>(a·</i>X<i>+b)</i></sup>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_LOG:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: <i>a·Ln(</i>X<i>)+b</i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_USER:
    gtk_label_set_text(GTK_LABEL(d->func_label), "");
    deriv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->derivatives));

    gtk_widget_set_sensitive(d->dim_box, FALSE);
    gtk_widget_set_sensitive(d->through_point, FALSE);
    gtk_widget_set_sensitive(d->through_box, FALSE);
    gtk_widget_set_sensitive(d->usr_def_frame, TRUE);
    gtk_widget_set_sensitive(d->usr_deriv_box, deriv);
    break;
  }
  gtk_widget_set_sensitive(d->div_box, ! intp);
}

static void
FitDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *hbox2, *vbox, *vbox2, *vbox3, *frame;
  struct FitDialog *d;
  char title[20], **enumlist, mes[10];
  int i;

  d = (struct FitDialog *) data;
  snprintf(title, sizeof(title), _("Fit %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "fit");

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Load"), IDLOAD);
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_SAVE, IDSAVE);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Type:"), FALSE);
    d->type = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    item_setup(hbox2, w, _("_Dim:"), FALSE);
    d->dim = w;
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 4);
    d->dim_box = hbox2;

    w = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->func_label = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, _("_Weight:"), TRUE);
    d->weight = w;
    d->weight_box = hbox2;

    gtk_box_pack_start(GTK_BOX(hbox), hbox2, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Through"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->through_point = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_X:", TRUE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_Y:", TRUE);
    d->y = w;

    d->through_box = hbox2;

    gtk_box_pack_start(GTK_BOX(hbox), hbox2, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry(1, 65535, 1, TRUE, TRUE);
    item_setup(hbox2, w, _("_Div:"), TRUE);
    d->div = w;
    d->div_box = hbox2;
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 4);


    w = gtk_check_button_new_with_mnemonic(_("_Interpolation"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->interpolation = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Formula:"), TRUE);
    d->formula = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Converge (%):"), TRUE);
    d->converge = w;

    w = gtk_check_button_new_with_mnemonic(_("_Derivatives"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->derivatives = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    
    vbox2 = gtk_vbox_new(FALSE, 4);
    vbox3 = gtk_vbox_new(FALSE, 4);
    for (i = 0; i < FIT_PARM_NUM; i++) {
      char p[] = "_P0", dd[] = "_D0";
    
      p[sizeof(p) - 2] += i;
      dd[sizeof(dd) - 2] += i;

      hbox = gtk_hbox_new(FALSE, 4);
      w = create_text_entry(TRUE, TRUE);
      item_setup(hbox, w, p, TRUE);
      d->p[i] = w;
      gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 4);

      hbox = gtk_hbox_new(FALSE, 4);
      w = create_text_entry(TRUE, TRUE);
      item_setup(hbox, w, dd, TRUE);
      d->d[i] = w;
      gtk_box_pack_start(GTK_BOX(vbox3), hbox, TRUE, TRUE, 4);
    }
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox3, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    d->usr_deriv_box = vbox3;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);
    d->usr_def_frame = frame;

    enumlist = (char **) chkobjarglist(d->Obj, "type");
    for (i = 0; enumlist[i]; i++) {
      combo_box_append_text(d->type, _(enumlist[i]));
    }

    for (i = 0; i < FIT_PARM_NUM - 1; i++) {
      snprintf(mes, sizeof(mes), "%d", i + 1);
      combo_box_append_text(d->dim, mes);
    }

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Result"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogResult), d);

    w = gtk_button_new_with_mnemonic(_("_Draw"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogDraw), d);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    g_signal_connect(d->dim, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->type, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->through_point, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->derivatives, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->interpolation, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
  }


  FitDialogSetupItem(wi, d, d->Id);
}

static void
FitDialogClose(GtkWidget *w, void *data)
{
  struct FitDialog *d;
  int ret;
  int i, lastid;

  d = (struct FitDialog *) data;
  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FitDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDLOAD:
    FitDialogLoad(d);
    d->ret = IDLOOP;
    return;
  case IDSAVE:
    FitDialogSave(d);
    d->ret = IDLOOP;
    return;
  case IDDELETE:
    break;
  case IDCANCEL:
    break;
  default:
    d->ret = IDLOOP;
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;
  if (ret == IDOK && ! FitDialogApply(w, d)) {
    return;
  }
  d->ret = ret;
  lastid = chkobjlastinst(d->Obj);
  for (i = lastid; i > d->Lastid; i--) {
    delobj(d->Obj, i);
  }
}

void
FitDialog(struct FitDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FitDialogSetup;
  data->CloseWindow = FitDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->Lastid = chkobjlastinst(obj);
}

static void
FileMoveDialogSetupItem(GtkWidget *w, struct FileMoveDialog *d, int id)
{
  unsigned int j, movenum;
  int line;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  char buf[64];

  list_store_clear(d->list);

  exeobj(d->Obj, "move_data_adjust", id, 0, NULL);
  getobj(d->Obj, "move_data", id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", id, 0, NULL, &movey);

  movenum = arraynum(move);
  
  if (arraynum(movex) < movenum) {
    movenum = arraynum(movex);
  }

  if (arraynum(movey) < movenum) {
    movenum = arraynum(movey);
  }

  if (movenum > 0) {
    for (j = 0; j < movenum; j++) {
      line = *(int *) arraynget(move, j);
      x = *(double *) arraynget(movex, j);
      y = *(double *) arraynget(movey, j);

      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, line);

      snprintf(buf, sizeof(buf), "%+.15e", x);
      list_store_set_string(d->list, &iter, 1, buf);

      snprintf(buf, sizeof(buf), "%+.15e", y);
      list_store_set_string(d->list, &iter, 2, buf);
    }
  }
}

static void
FileMoveDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileMoveDialog *d;
  int a;
  double x, y;
  const char *buf;
  char *endptr, buf2[64];
  GtkTreeIter iter;

  d = (struct FileMoveDialog *) client_data;

  a = spin_entry_get_val(d->line);

  buf = gtk_entry_get_text(GTK_ENTRY(d->x));
  if (buf[0] == '\0') return;

  x = strtod(buf, &endptr);
  if (x != x || x == HUGE_VAL || x == - HUGE_VAL || endptr[0] != '\0')
    return;

  buf = gtk_entry_get_text(GTK_ENTRY(d->y));
  if (buf[0] == '\0') return;

  y = strtod(buf, &endptr);
  if (y != y || y == HUGE_VAL || y == - HUGE_VAL || endptr[0] != '\0')
    return;

  list_store_append(d->list, &iter);
  list_store_set_int(d->list, &iter, 0, a);

  snprintf(buf2, sizeof(buf2), "%+.15e", x);
  list_store_set_string(d->list, &iter, 1, buf2);

  snprintf(buf2, sizeof(buf2), "%+.15e", y);
  list_store_set_string(d->list, &iter, 2, buf2);

  gtk_entry_set_text(GTK_ENTRY(d->x), "");
  gtk_entry_set_text(GTK_ENTRY(d->y), "");
  d->changed = TRUE;
}


static gboolean
move_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileMoveDialog *d;

  d = (struct FileMoveDialog *) user_data;
  if (e->keyval != GDK_Return)
    return FALSE;

  FileMoveDialogAdd(NULL, d);

  return TRUE;
}

static void
FileMoveDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileMoveDialog *d;
  d = (struct FileMoveDialog *) client_data;

  list_store_remove_selected_cb(w, d->list);
  d->changed = TRUE;
}

static void
FileMoveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  struct FileMoveDialog *d;
  n_list_store list[] = {
    {N_("Line No."), G_TYPE_INT,    TRUE, FALSE, NULL, FALSE},
    {"X",            G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {"Y",            G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct FileMoveDialog *) data;

  if (makewidget) {
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_column(w, 0);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);


    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, _("_Line:"), TRUE);
    d->line = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, "_X:", TRUE);
    d->x = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, "_Y:", TRUE);
    d->y = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogAdd), d);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogRemove), d);
    set_sensitivity_by_selection(d->list, w);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    set_sensitivity_by_row_num(d->list, w);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), 640, 400);
  }

  FileMoveDialogSetupItem(wi, d, d->Id);
  /*
  if (makewidget)
    d->widget = NULL;
  */
}

static void
FileMoveDialogCopy(struct FileMoveDialog *d)
{
  int sel;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    FileMoveDialogSetupItem(d->widget, d, sel);
    d->changed = TRUE;
  }
}

static void
FileMoveDialogClose(GtkWidget *w, void *data)
{
  struct FileMoveDialog *d;
  unsigned int j, movenum;
  int ret, line, a;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  gboolean state;
  char *ptr, *endptr;

  d = (struct FileMoveDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileMoveDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  if (d->changed == FALSE) {
    return;
  }

  set_graph_modified();
  ret = d->ret;
  d->ret = IDLOOP;
  exeobj(d->Obj, "move_data_adjust", d->Id, 0, NULL);
  getobj(d->Obj, "move_data", d->Id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", d->Id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", d->Id, 0, NULL, &movey);
  if (move) {
    putobj(d->Obj, "move_data", d->Id, NULL);
    move = NULL;
  }
  if (movex) {
    putobj(d->Obj, "move_data_x", d->Id, NULL);
    movex = NULL;
  }
  if (movey) {
    putobj(d->Obj, "move_data_y", d->Id, NULL);
    movey = NULL;
  }

  state = list_store_get_iter_first(d->list, &iter);
  while (state) {
    a = list_store_get_int(d->list, &iter, 0); 

    ptr = list_store_get_string(d->list, &iter, 1); 
    x = strtod(ptr, &endptr);
    free(ptr);

    ptr = list_store_get_string(d->list, &iter, 2); 
    y = strtod(ptr, &endptr);
    free(ptr);

    if (move == NULL)
      move = arraynew(sizeof(int));

    if (movex == NULL)
      movex = arraynew(sizeof(double));

    if (movey == NULL)
      movey = arraynew(sizeof(double));

    movenum = arraynum(move);
    if (arraynum(movex) < movenum)
      movenum = arraynum(movex);

    if (arraynum(movey) < movenum)
      movenum = arraynum(movey);

    for (j = 0; j < movenum; j++) {
      line = *(int *) arraynget(move, j);

      if (line == a)
	break;
    }

    if (j == movenum) {
      arrayadd(move, &a);
      arrayadd(movex, &x);
      arrayadd(movey, &y);
    }

    state = list_store_iter_next(d->list, &iter);
  }

  putobj(d->Obj, "move_data", d->Id, move);
  putobj(d->Obj, "move_data_x", d->Id, movex);
  putobj(d->Obj, "move_data_y", d->Id, movey);
  d->ret = ret;
}

void
FileMoveDialog(struct FileMoveDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMoveDialogSetup;
  data->CloseWindow = FileMoveDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->changed = FALSE;
}

static void
FileMaskDialogSetupItem(GtkWidget *w, struct FileMaskDialog *d, int id)
{
  int line, j, masknum;
  struct narray *mask;
  GtkTreeIter iter;

  list_store_clear(d->list);
  getobj(d->Obj, "mask", id, 0, NULL, &mask);
  if ((masknum = arraynum(mask)) > 0) {
    for (j = 0; j < masknum; j++) {
      line = *(int *) arraynget(mask, j);
      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, line);
    }
  }
}

static void
FileMaskDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileMaskDialog *d;
  int a;
  GtkTreeIter iter;

  d = (struct FileMaskDialog *) client_data;

  a = spin_entry_get_val(d->line);
  list_store_append(d->list, &iter);
  list_store_set_int(d->list, &iter, 0, a);
  d->changed = TRUE;
}

static gboolean
mask_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileMaskDialog *d;

  d = (struct FileMaskDialog *) user_data;
  if (e->keyval != GDK_Return)
    return FALSE;

  FileMaskDialogAdd(NULL, d);

  return TRUE;
}

static void
FileMaskDialogCopy(struct FileMaskDialog *d)
{
  int sel;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    FileMaskDialogSetupItem(d->widget, d, sel);
    d->changed = TRUE;
  }
}

static void
FileMaskDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileMaskDialog *d;
  d = (struct FileMaskDialog *) client_data;

  list_store_remove_selected_cb(w, d->list);
  d->changed = TRUE;
}

static void
FileMaskDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *hbox, *vbox;
  struct FileMaskDialog *d;
  n_list_store list[] = {
    {_("Line No."), G_TYPE_INT, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct FileMaskDialog *) data;

  if (makewidget) {
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(mask_dialog_key_pressed), d);

    item_setup(vbox, w, _("_Line:"), FALSE);
    d->line = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_column(w, 0);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogAdd), d);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogRemove), d);
    set_sensitivity_by_selection(d->list, w);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    set_sensitivity_by_row_num(d->list, w);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), 300, 400);
  }

  FileMaskDialogSetupItem(wi, d, d->Id);
}

static void
FileMaskDialogClose(GtkWidget *w, void *data)
{
  struct FileMaskDialog *d;
  int ret, a;
  struct narray *mask;
  GtkTreeIter iter;
  gboolean state;

  d = (struct FileMaskDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileMaskDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  if (d->changed == FALSE) {
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if (mask) {
    putobj(d->Obj, "mask", d->Id, NULL);
    mask = NULL;
  }
  
  state = list_store_get_iter_first(d->list, &iter);
  while (state) {
    a = list_store_get_int(d->list, &iter, 0); 
    if (mask == NULL)
      mask = arraynew(sizeof(int));

    arrayadd(mask, &a);
    state = list_store_iter_next(d->list, &iter);
  }
  putobj(d->Obj, "mask", d->Id, mask);
  d->ret = ret;
  set_graph_modified();
}

void
FileMaskDialog(struct FileMaskDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMaskDialogSetup;
  data->CloseWindow = FileMaskDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->changed = FALSE;
}

static void
FileLoadDialogSetupItem(GtkWidget *w, struct FileLoadDialog *d, int id)
{
  char *ifs, *s;
  unsigned int i, l;

  SetWidgetFromObjField(d->headskip, d->Obj, id, "head_skip");
  SetWidgetFromObjField(d->readstep, d->Obj, id, "read_step");
  SetWidgetFromObjField(d->finalline, d->Obj, id, "final_line");
  SetWidgetFromObjField(d->remark, d->Obj, id, "remark");
  sgetobjfield(d->Obj, id, "ifs", NULL, &ifs, FALSE, FALSE, FALSE);
  s = nstrnew();

  l = strlen(ifs);
  for (i = 0; i < l; i++) {
    if (ifs[i] == '\t') {
      s = nstrccat(s, '\\');
      s = nstrccat(s, 't');
    } else if (ifs[i] == '\\') {
      s = nstrccat(s, '\\');
      s = nstrccat(s, '\\');
    } else
      s = nstrccat(s, ifs[i]);
  }
  gtk_entry_set_text(GTK_ENTRY(d->ifs), s);
  memfree(s);
  memfree(ifs);
  SetWidgetFromObjField(d->csv, d->Obj, id, "csv");
}

static void
FileLoadDialogCopy(struct FileLoadDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FileCB)) != -1)
    FileLoadDialogSetupItem(d->widget, d, sel);
}

static void
FileLoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox;
  struct FileLoadDialog *d;

  d = (struct FileLoadDialog *) data;

  if (makewidget) {
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
    item_setup(hbox, w, _("_Head skip:"), TRUE);
    d->headskip = w;

    w = create_spin_entry(1, INT_MAX, 1, TRUE, TRUE);
    item_setup(hbox, w, _("_Read step:"), TRUE);
    d->readstep = w;


    w = create_spin_entry_type(SPIN_BUTTON_TYPE_NUM, TRUE, TRUE);
    item_setup(hbox, w, _("_Final line:"), TRUE);
    d->finalline = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Remark:"), TRUE);
    d->remark = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Ifs:"), TRUE);
    d->ifs = w;

    w = gtk_check_button_new_with_mnemonic(_("_CSV"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->csv = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }

  FileLoadDialogSetupItem(wi, d, d->Id);
}

static void
FileLoadDialogClose(GtkWidget *w, void *data)
{
  struct FileLoadDialog *d;
  int ret;
  const char *ifs;
  char *s, *obuf;
  unsigned int i, l;

  d = (struct FileLoadDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileLoadDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromWidget(d->headskip, d->Obj, d->Id, "head_skip"))
    return;

  if (SetObjFieldFromWidget(d->readstep, d->Obj, d->Id, "read_step"))
    return;

  if (SetObjFieldFromWidget(d->finalline, d->Obj, d->Id, "final_line"))
    return;

  if (SetObjFieldFromWidget(d->remark, d->Obj, d->Id, "remark"))
    return;

  ifs = gtk_entry_get_text(GTK_ENTRY(d->ifs));
  s = nstrnew();

  l = strlen(ifs);
  for (i = 0; i < l; i++) {
    if ((ifs[i] == '\\') && (ifs[i + 1] == 't')) {
      s = nstrccat(s, 0x09);
      i++;
    } else if (ifs[i] == '\\') {
      s = nstrccat(s, '\\');
      i++;
    } else {
      s = nstrccat(s, ifs[i]);
    }
  }

  sgetobjfield(d->Obj, d->Id, "ifs", NULL, &obuf, FALSE, FALSE, FALSE);
  if (obuf == NULL || strcmp(s, obuf)) {
    if (sputobjfield(d->Obj, d->Id, "ifs", s) != 0) {
      memfree(obuf);
      memfree(s);
      return;
    }
    set_graph_modified();
  }

  memfree(obuf);
  memfree(s);

  if (SetObjFieldFromWidget(d->csv, d->Obj, d->Id, "csv"))
    return;

  d->ret = ret;
}

void
FileLoadDialog(struct FileLoadDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileLoadDialogSetup;
  data->CloseWindow = FileLoadDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
FileMathDialogSetupItem(GtkWidget *w, struct FileMathDialog *d, int id)
{
  SetWidgetFromObjField(d->xsmooth, d->Obj, id, "smooth_x");
  SetWidgetFromObjField(d->ysmooth, d->Obj, id, "smooth_y");
  SetWidgetFromObjField(d->xmath, d->Obj, id, "math_x");
  SetWidgetFromObjField(d->ymath, d->Obj, id, "math_y");
  SetWidgetFromObjField(d->fmath, d->Obj, id, "func_f");
  SetWidgetFromObjField(d->gmath, d->Obj, id, "func_g");
  SetWidgetFromObjField(d->hmath, d->Obj, id, "func_h");
}

static void
FileMathDialogCopy(struct FileMathDialog *d)
{
  int sel;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    FileMathDialogSetupItem(d->widget, d, sel);
  }
}

static gboolean
func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data)
{
  struct FileMathDialog *d;

  d = (struct FileMathDialog *) user_data;
  gtk_entry_set_completion(GTK_ENTRY(d->fmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->gmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->hmath), NULL);

  gtk_entry_set_completion(GTK_ENTRY(w), NgraphApp.func_list);

  return FALSE;
}


static void
FileMathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox;
  struct FileMathDialog *d;

  d = (struct FileMathDialog *) data;
  if (makewidget) {
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, TRUE, TRUE);
    item_setup(hbox, w, _("_X smooth:"), FALSE);
    d->xsmooth = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_X math:"), TRUE);
    d->xmath = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, TRUE, TRUE);
    item_setup(hbox, w, _("_Y smooth:"), FALSE);
    d->ysmooth = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Y math:"), TRUE);
    d->ymath = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_F(X,Y,Z):", TRUE);
    d->fmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_G(X,Y,Z):", TRUE);
    d->gmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_H(X,Y,Z):", TRUE);
    d->hmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }

  FileMathDialogSetupItem(wi, d, d->Id);

  gtk_entry_set_completion(GTK_ENTRY(d->xmath), NgraphApp.x_math_list);
  gtk_entry_set_completion(GTK_ENTRY(d->ymath), NgraphApp.y_math_list);
}

static void
FileMathDialogClose(GtkWidget *w, void *data)
{
  struct FileMathDialog *d;
  int ret;
  const char *s;

  d = (struct FileMathDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileMathDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  s = gtk_entry_get_text(GTK_ENTRY(d->ymath));
  entry_completion_append(NgraphApp.y_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->xmath));
  entry_completion_append(NgraphApp.x_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->fmath));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->gmath));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->hmath));
  entry_completion_append(NgraphApp.func_list, s);

  if (SetObjFieldFromWidget(d->xsmooth, d->Obj, d->Id, "smooth_x"))
    return;

  if (SetObjFieldFromWidget(d->xmath, d->Obj, d->Id, "math_x"))
    return;

  if (SetObjFieldFromWidget(d->ysmooth, d->Obj, d->Id, "smooth_y"))
    return;

  if (SetObjFieldFromWidget(d->ymath, d->Obj, d->Id, "math_y"))
    return;

  if (SetObjFieldFromWidget(d->fmath, d->Obj, d->Id, "func_f"))
    return;

  if (SetObjFieldFromWidget(d->gmath, d->Obj, d->Id, "func_g"))
    return;

  if (SetObjFieldFromWidget(d->hmath, d->Obj, d->Id, "func_h"))
    return;

  gtk_entry_set_completion(GTK_ENTRY(d->ymath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->xmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->fmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->gmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->hmath), NULL);

  d->ret = ret;
}

void
FileMathDialog(struct FileMathDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMathDialogSetup;
  data->CloseWindow = FileMathDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
MarkDialogCB(GtkWidget *w, gpointer client_data)
{
  int i;
  struct MarkDialog *d;

  d = (struct MarkDialog *) client_data;

  if (! d->cb_respond)
    return;

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    if (w == d->toggle[i])
      break;
  }

  d->Type = i;
  d->ret = IDOK;
  gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
}

static void
MarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox, *img;
  struct MarkDialog *d;
  int type;
#define COL 10

  d = (struct MarkDialog *) data;

  if (makewidget) {
    hbox = NULL;
    vbox = gtk_vbox_new(FALSE, 4);

    for (type = 0; type < MARK_TYPE_NUM; type++) {
      w = gtk_toggle_button_new();
      if (NgraphApp.markpix[type]) {
	img = gtk_image_new_from_pixmap(NgraphApp.markpix[type], NULL);
	gtk_button_set_image(GTK_BUTTON(w), img);
      }
      g_signal_connect(w, "clicked", G_CALLBACK(MarkDialogCB), d);
      d->toggle[type] = w;
      if (type % COL == 0) {
	if (hbox) {
	  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	}
	hbox = gtk_hbox_new(FALSE, 4);
      }
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    }
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    //    d->show_buttons = FALSE;
  }

  d->cb_respond = FALSE;
  for (type = 0; type < MARK_TYPE_NUM; type++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[type]), FALSE);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[d->Type]), TRUE);
  d->focus = d->toggle[d->Type];
  d->cb_respond = TRUE;
}

static void
MarkDialogClose(GtkWidget *w, void *data)
{
}

void
MarkDialog(struct MarkDialog *data, int type)
{
  data->SetupWindow = MarkDialogSetup;
  data->CloseWindow = MarkDialogClose;
  data->Type = type;
}

static void
FileDialogSetupItemCommon(GtkWidget *w, struct FileDialog *d, int id)
{
  int i, j, a, lastinst;
  struct objlist *aobj;
  char *name, *valstr;
  GtkWidget *img;

  combo_box_clear(d->xaxis);
  combo_box_clear(d->yaxis);

  aobj = getobject("axis");

  lastinst = chkobjlastinst(aobj);
  for (j = 0; j <= lastinst; j++) {
    getobj(aobj, "group", j, 0, NULL, &name);
    name = CHK_STR(name);
    combo_box_append_text(d->xaxis, name);
    combo_box_append_text(d->yaxis, name);
  }

  SetWidgetFromObjField(d->xcol, d->Obj, id, "x");

  sgetobjfield(d->Obj, id, "axis_x", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->xaxis, valstr + i);;
  memfree(valstr);

  SetWidgetFromObjField(d->ycol, d->Obj, id, "y");

  sgetobjfield(d->Obj, id, "axis_y", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->yaxis, valstr + i);;
  memfree(valstr);

  SetWidgetFromObjField(d->type, d->Obj, id, "type");

  SetWidgetFromObjField(d->curve, d->Obj, id, "interpolation");

  getobj(d->Obj, "mark_type", id, 0, NULL, &a);
  if (a < 0 || a >= MARK_TYPE_NUM)
    a = 0;

  if (NgraphApp.markpix[a]) {
    img = gtk_image_new_from_pixmap(NgraphApp.markpix[a], NULL);
    gtk_button_set_image(GTK_BUTTON(d->mark_btn), img);
  }

  MarkDialog(&(d->mark), a);

  SetWidgetFromObjField(d->size, d->Obj, id, "mark_size");

  SetWidgetFromObjField(d->width, d->Obj, id, "line_width");

  SetStyleFromObjField(d->style, d->Obj, id, "line_style");

  SetWidgetFromObjField(d->join, d->Obj, id, "line_join");

  SetWidgetFromObjField(d->miter, d->Obj, id, "line_miter_limit");

  SetWidgetFromObjField(d->clip, d->Obj, id, "data_clip");

  set_color(d->col1, d->Obj, id, NULL);
  set_color2(d->col2, d->Obj, id);
}

static void
FileDialogSetupItem(GtkWidget *w, struct FileDialog *d, int file, int id)
{
  int i;
  char *valstr;

  FileDialogSetupItemCommon(w, d, id);

  if (file) {
    SetWidgetFromObjField(d->file, d->Obj, id, "file");
  }

  if (id == d->Id) {
    sgetobjfield(d->Obj, id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
    for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
    if (valstr[i] == ':')
      i++;
    gtk_label_set_text(GTK_LABEL(d->fitid), valstr + i);
    memfree(valstr);
  }
  
  gtk_widget_set_sensitive(d->apply_all, d->multi_open);
}

static void
FileDialogAxis(GtkWidget *w, gpointer client_data)
{
  char buf[10];
  int a;

  a = combo_box_get_active(w);

  if (a < 0)
    return;

  snprintf(buf, sizeof(buf), "%d", a);
  combo_box_entry_set_text(w, buf);
}

static void
FileDialogMark(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  GtkWidget *img;

  d = (struct FileDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  img = gtk_image_new_from_pixmap(NgraphApp.markpix[d->mark.Type], NULL);
  gtk_button_set_image(GTK_BUTTON(w), img);
}

static void
FileDialogFit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  struct objlist *fitobj, *obj;
  char *fit, *inst;
  int i, idnum, fitid = 0, fitoid, ret, create = FALSE;
  struct narray iarray;
  char *valstr;

  d = (struct FileDialog *) client_data;

  if ((fitobj = chkobject("fit")) == NULL)
    return;

  if (getobj(d->Obj, "fit", d->Id, 0, NULL, &fit) == -1)
    return;

  if (fit) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &obj, &iarray, FALSE, NULL))
      return;

    idnum = arraynum(&iarray);
    if ((obj != fitobj) || (idnum < 1)) {
      if (putobj(d->Obj, "fit", d->Id, NULL) == -1) {
	arraydel(&iarray);
	return;
      }
    } else {
      fitid = *(int *) arraylast(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL) {
    fitid = newobj(fitobj);
    inst = getobjinst(fitobj, fitid);

    _getobj(fitobj, "oid", inst, &fitoid);

    if ((fit = mkobjlist(fitobj, NULL, fitoid, NULL, TRUE)) == NULL)
      return;

    if (putobj(d->Obj, "fit", d->Id, fit) == -1) {
      memfree(fit);
      return;
    }
    create = TRUE;
  }

  FitDialog(&DlgFit, fitobj, fitid);
  ret = DialogExecute(d->widget, &DlgFit);

  switch (ret) {
  case IDCANCEL:
    if (! create)
      break;
  case IDDELETE:
    delobj(fitobj, fitid);
    putobj(d->Obj, "fit", d->Id, NULL);
    if (! create)
      set_graph_modified();
    break;
  case IDOK:
    combo_box_set_active(d->type, PLOT_TYPE_FIT);
    if (create)
      set_graph_modified();
    break;
  }

  sgetobjfield(d->Obj, d->Id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  gtk_label_set_text(GTK_LABEL(d->fitid), valstr);
  memfree(valstr);
}

static void
FileDialogCopy(struct FileDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FileCB)) != -1)
    FileDialogSetupItem(d->widget, d, FALSE, sel);
}

void 
copy_file_obj_field(struct objlist *obj, int id, int sel, int copy_filename)
{
  char *field[] = {"name", "fit", NULL, NULL};

  if (! copy_filename) {
    int i;
    i = sizeof(field) / sizeof(*field) - 2;
    field[i] = "file";
  }

  copy_obj_field(obj, id, sel, field);

  FitCopy(obj, id, sel);
  set_graph_modified();
}

static void
FileDialogCopyAll(struct FileDialog *d)
{
  int sel;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel == -1) 
    return;

  copy_file_obj_field(d->Obj, d->Id, sel, FALSE);
  FileDialogSetupItem(d->widget, d, FALSE, d->Id);
}

static void
FileDialogOption(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  exeobj(d->Obj, "load_settings", d->Id, 0, NULL);
  FileDialogSetupItem(d->widget, d, FALSE, d->Id);
}

static void
FileDialogMath(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMathDialog(&DlgFileMath, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMath);
}

static void
FileDialogLoad(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileLoadDialog(&DlgFileLoad, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileLoad);
}

static void
FileDialogMask(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMaskDialog(&DlgFileMask, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMask);
}

static void
FileDialogMove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMoveDialog(&DlgFileMove, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMove);
}

static void
FileDialogEdit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  const char *tmp;
  char *name;
  char *argv[3];
  pid_t pid;

  d = (struct FileDialog *) client_data;

  if (Menulocal.editor == NULL)
    return;

  argv[0] = Menulocal.editor;
  argv[2] = NULL;

  tmp = gtk_entry_get_text(GTK_ENTRY(d->file));

  if (tmp == NULL)
    return;

  name = strdup(tmp);
  if (name == NULL)
    return;
  if ((pid = fork()) >= 0) {
    argv[1] = name;
    if (pid == 0) {
      execvp(argv[0], argv);
      exit(1);
    }
  }
  free(name);
}

static void
FileDialogType(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int type;

  d = (struct FileDialog *) client_data;

  gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->col1), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->style), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->width), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), TRUE);
  gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), TRUE);
  
  type = combo_box_get_active(w);

  switch (type) {
  case PLOT_TYPE_MARK:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    break;
  case PLOT_TYPE_CURVE:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    break;
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_ARROW:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_RECTANGLE_FILL:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    break;
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->miter), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->join), FALSE);
    break;
  case PLOT_TYPE_FIT:
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->mark_btn), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->curve), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->col2), FALSE);
    gtk_widget_set_sensitive(gtk_widget_get_parent(d->size), FALSE);
    break;
  }
}

static void
FileDialogSetupCommon(GtkWidget *wi, struct FileDialog *d)
{
  GtkWidget *w, *hbox, *vbox, *vbox2, *frame;

  vbox = gtk_vbox_new(FALSE, 4);

  vbox2 = gtk_vbox_new(FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);

  w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, TRUE, TRUE);
  item_setup(hbox, w, _("_X column:"), TRUE);
  d->xcol = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

  item_setup(hbox, w, _("_X axis:"), TRUE);
  d->xaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);


  hbox = gtk_hbox_new(FALSE, 4);

  w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, TRUE, TRUE);
  item_setup(hbox, w, _("_Y column:"), TRUE);
  d->ycol = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

  item_setup(hbox, w, _("_Y axis:"), TRUE);
  d->yaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  d->comment_box = hbox;

  vbox2 = gtk_vbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox2), frame, TRUE, TRUE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);

  hbox = gtk_hbox_new(FALSE, 4);

  w = combo_box_create();
  item_setup(hbox, w, _("_Type:"), FALSE);
  d->type = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogType), d);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);


  vbox2 = gtk_vbox_new(FALSE, 4);
  w = gtk_button_new();
  item_setup(vbox2, w, _("_Mark:"), FALSE);
  d->mark_btn = w;
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMark), d);

  w = combo_box_create();
  item_setup(vbox2, w, _("_Curve:"), FALSE);
  d->curve = w;

  d->fit_box = vbox2;

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);


  vbox2 = gtk_vbox_new(FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  w = create_color_button(wi);
  item_setup(hbox, w, _("_Color 1:"), FALSE);
  d->col1 = w;

  w = create_color_button(wi);
  item_setup(hbox, w, _("_Color 2:"), FALSE);
  d->col2 = w;

  w = gtk_check_button_new_with_mnemonic(_("_Clip"));
  d->clip = w;
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);


  vbox2 = gtk_vbox_new(FALSE, 4);

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  item_setup(vbox2, w, _("Line _Style:"), TRUE);
  d->style = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  item_setup(vbox2, w, _("_Line Width:"), TRUE);
  d->width = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  item_setup(vbox2, w, _("_Size:"), TRUE);
  d->size = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  item_setup(vbox2, w, _("_Miter:"), TRUE);
  d->miter = w;

  w = combo_box_create();
  item_setup(vbox2, w, _("_Join:"), TRUE);
  d->join = w;

  gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 4);


  vbox2 = gtk_vbox_new(FALSE, 4);

  w = gtk_button_new_with_mnemonic(_("_Math"));
  gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 4);
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMath), d);

  w = gtk_button_new_with_mnemonic(_("_Load"));
  gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 4);
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogLoad), d);

  d->button_box = vbox2;

  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 4);
}

static int
count_line_number_str(const char *str)
{
  int i, n;

  if (str == NULL)
    return 0;

  n = 0;
  for (i = 0; str[i] != '\0' && str[i] != '\n'; i++) {
    if (str[i] != ' ') {
      n = i + 2;
      break;
    }
  }

  return n;
}

static void
set_line_number_tag(GtkTextBuffer *buf, GtkTextTag *tag, int n)
{
  GtkTextIter start, end;

  if (tag == NULL || n == 0)
    return;

  gtk_text_buffer_get_iter_at_offset(buf, &start, 0);
  do {
    end = start;
    if (! gtk_text_iter_forward_chars(&end, n))
      break;
    gtk_text_buffer_apply_tag (buf, tag, &start, &end);

    if (! gtk_text_iter_forward_line(&start))
      break;
  } while (1);
}

static GtkTextTag *
create_text_tag(GtkWidget *view, GtkTextBuffer *buf)
{
  GtkTextTag *tag;
  GdkColor fg, bg;

  bg.red = 0xCC00;
  bg.green = 0xCC00;
  bg.blue = 0xCC00;

  fg.red = 0x00;
  fg.green = 0x00;
  fg.blue = 0x00;

  tag =  gtk_text_buffer_create_tag(buf,
				    "line_number",
				    // "weight", PANGO_WEIGHT_BOLD,
				    "foreground-gdk", &fg,
				    "background-gdk", &bg,
				    NULL);
  return tag;
}


static void
FileDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *swin, *view;
  struct FileDialog *d;
  int line, rcode;
  char title[20], *argv[2], *s;

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    d->apply_all = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Apply all"), IDFAPPLY);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_CLOSE, IDDELETE);

    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    w = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Copy all"), IDCOPYALL);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "file");

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_file_entry(d->Obj);
    item_setup(hbox, w, _("_File:"), TRUE);
    d->file = w;

    w = gtk_button_new_with_mnemonic(_("_Load settings"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->load_settings = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogOption), d);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
    FileDialogSetupCommon(wi, d);

    swin = gtk_scrolled_window_new(NULL, NULL);
    view = gtk_text_view_new_with_buffer(NULL);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    d->comment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    d->comment_num_tag = create_text_tag(view, d->comment);
    d->comment_view = view;
    gtk_container_add(GTK_CONTAINER(swin), view);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->comment_box), w, TRUE, TRUE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_with_mnemonic(_("_Fit"));
    d->fit = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);

    w = gtk_label_new("");
    d->fitid = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->fit_box), hbox, FALSE, FALSE, 4);


    w = gtk_button_new_with_mnemonic(_("_Mask"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMask), d);

    w = gtk_button_new_with_mnemonic(_("_Move"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMove), d);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogEdit), d);
  }

  line = Menulocal.data_head_lines;
  argv[0] = (char *) &line;
  argv[1] = NULL;
  rcode = getobj(d->Obj, "head_lines", d->Id, 1, argv, &s);
  if (s) {
    gboolean valid;
    const gchar *ptr;

    if (Menulocal.file_preview_font) {
      PangoFontDescription *desc;

      desc = pango_font_description_from_string(Menulocal.file_preview_font);
      gtk_widget_modify_font(d->comment_view, NULL);
      gtk_widget_modify_font(d->comment_view, desc);
      pango_font_description_free(desc);
    }
    valid = g_utf8_validate(s, -1, &ptr);
    if (valid) {
      int n;
      gtk_text_buffer_set_text(d->comment, s, -1);
      n = count_line_number_str(s);
      set_line_number_tag(d->comment, d->comment_num_tag, n);
    } else {
      gtk_text_buffer_set_text(d->comment, _("This file contain invalid UTF-8 strings."), -1);
    }
  }
  FileDialogSetupItem(wi, d, TRUE, d->Id);

  /*
  if (makewidget)
    d->widget = NULL;
  */
}

static int
FileDialogCloseCommon(GtkWidget *w, struct FileDialog *d)
{
  if (SetObjFieldFromWidget(d->xcol, d->Obj, d->Id, "x"))
    return TRUE;

  if (SetObjAxisFieldFromWidget(d->xaxis, d->Obj, d->Id, "axis_x"))
    return TRUE;

  if (SetObjFieldFromWidget(d->ycol, d->Obj, d->Id, "y"))
    return TRUE;

  if (SetObjAxisFieldFromWidget(d->yaxis, d->Obj, d->Id, "axis_y"))
    return TRUE;

  if (SetObjFieldFromWidget(d->type, d->Obj, d->Id, "type"))
    return TRUE;

  if (SetObjFieldFromWidget(d->curve, d->Obj, d->Id, "interpolation"))
    return TRUE;

  if (putobj(d->Obj, "mark_type", d->Id, &(d->mark.Type)) == -1)
    return TRUE;

  if (SetObjFieldFromWidget(d->size, d->Obj, d->Id, "mark_size"))
    return TRUE;

  if (SetObjFieldFromWidget(d->width, d->Obj, d->Id, "line_width"))
    return TRUE;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "line_style"))
    return TRUE;

  if (SetObjFieldFromWidget(d->join, d->Obj, d->Id, "line_join"))
    return TRUE;

  if (SetObjFieldFromWidget(d->miter, d->Obj, d->Id, "line_miter_limit"))
    return TRUE;

  if (SetObjFieldFromWidget(d->clip, d->Obj, d->Id, "data_clip"))
    return TRUE;

  if (putobj_color(d->col1, d->Obj, d->Id, NULL))
    return TRUE;

  if (putobj_color2(d->col2, d->Obj, d->Id))
    return TRUE;

  return FALSE;
}

static void
FileDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
  case IDFAPPLY:
    break;
  case IDCOPY:
    FileDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDCOPYALL:
    FileDialogCopyAll(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromWidget(d->file, d->Obj, d->Id, "file"))
    return;

  if (FileDialogCloseCommon(w, d))
    return;

  d->ret = ret;
}

void
FileDialog(void *data, struct objlist *obj, int id, int multi)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  d->SetupWindow = FileDialogSetup;
  d->CloseWindow = FileDialogClose;
  d->Obj = obj;
  d->Id = id;
  d->multi_open = multi > 0;
}

static void
FileDialogDefSetupItem(GtkWidget *w, struct FileDialog *d, int id)
{
  FileDialogSetupItemCommon(w, d, id);
}

static void
FileDefDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  if (makewidget) {
    FileDialogSetupCommon(wi, d);
  }
  FileDialogDefSetupItem(wi, d, d->Id);
}

static void
FileDefDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (FileDialogCloseCommon(w, d))
    return;

  d->ret = ret;
}

void
FileDefDialog(struct FileDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileDefDialogSetup;
  data->CloseWindow = FileDefDialogClose;
  data->Obj = obj;
  data->Id = id;
}

void
CmFileHistory(GtkWidget *w, gpointer client_data)
{
  int i, fil, num;
  char **data;
  struct narray *datafilelist;
  int ret;
  char *name;
  int id;
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (w == NgraphApp.fhistory[i]) break;
  }
  fil = i;
  datafilelist = Menulocal.datafilelist;
  num = arraynum(datafilelist);
  data = (char **) arraydata(datafilelist);
  if ((fil < 0) || (fil >= num) || (data[fil] == NULL))
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    name = nstrdup(data[fil]);
    if (name) {
      putobj(obj, "file", id, name);
      FileDialog(&DlgFile, obj, id, FALSE);
      ret = DialogExecute(TopLevel, &DlgFile);
      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	FitDel(obj, id);
	delobj(obj, id);
      } else {
	set_graph_modified();
      }
    }
  }
  AddDataFileList(data[fil]);
  FileWinUpdate(TRUE);
}

void
CmFileNew(void)
{
  char *name, *file;
  int id, ret;
  struct objlist *obj;


  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if (nGetOpenFileName(TopLevel, _("Data new"), NULL, NULL,
		       NULL, &file, FALSE,
		       Menulocal.changedirectory) == IDOK)
  {
    if ((id = newobj(obj)) >= 0) {
      name = nstrdup(file);
      if (name == NULL) {
	free(file);
	return;
      }
      changefilename(name);
      AddDataFileList(name);
      putobj(obj, "file", id, name);
      FileDialog(&DlgFile, obj, id, FALSE);
      ret = DialogExecute(TopLevel, &DlgFile);
      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	FitDel(obj, id);
	delobj(obj, id);
      } else
	set_graph_modified();
      FileWinUpdate(TRUE);
    }
    free(file);
  }
}


void
CmFileOpen(void)
{
  int id, ret;
  char *name, *tmp;;
  char **file = NULL, **ptr;
  struct objlist *obj;
  struct narray farray;

  if (Menulock || GlobalLock)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  ret = nGetOpenFileNameMulti(TopLevel, _("Data open"), NULL,
			      &(Menulocal.fileopendir), NULL,
			      &file, Menulocal.changedirectory);

  arrayinit(&farray, sizeof(int));
  if (ret == IDOK && file) {
    for (ptr = file; *ptr; ptr++) {
      name = *ptr;
      id = newobj(obj);
      if (id >= 0) {
	arrayadd(&farray, &id);
	tmp = nstrdup(name);
	changefilename(tmp);
	AddDataFileList(tmp);
	putobj(obj, "file", id, tmp);
      }
      free(name);
    }
    free(file);
  }

  if (update_file_obj_multi(obj, &farray, TRUE)) {
    FileWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmFileClose(void)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      FitDel(obj, array[i]);
      delobj(obj, array[i]);
      set_graph_modified();
    }
    FileWinUpdate(TRUE);
  }
  arraydel(&farray);
}

int
update_file_obj_multi(struct objlist *obj, struct narray *farray, int new_file)
{
  int i, j, num, *array, id0;

  num = arraynum(farray);
  if (num < 1)
    return 0;

  array = (int *) arraydata(farray);
  id0 = -1;

  for (i = 0; i < num; i++) {
    if (id0 != -1) {
      copy_file_obj_field(obj, array[i], array[id0], FALSE);
    } else {
      int ret;

      FileDialog(&DlgFile, obj, array[i], i < num - 1);
      ret = DialogExecute(TopLevel, &DlgFile);
      if (ret == IDCANCEL && new_file) {
	ret = IDDELETE;
      }
      if (ret == IDDELETE) {
	FitDel(obj, array[i]);
	delobj(obj, array[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  array[j]--;
      } else if (ret == IDFAPPLY) {
	id0 = i;
      }
    }
  }

  return 1;
}

void
CmFileUpdate(void)
{
  struct objlist *obj;
  int ret;
  struct narray farray;
  int last;

  if (Menulock || GlobalLock)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  last = chkobjlastinst(obj);
  if (last == -1) {
    return;
  } else if (last == 0) {
    arrayinit(&farray, sizeof(int));
    arrayadd(&farray, &last);
    ret = IDOK;
  } else {
    SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
    ret = DialogExecute(TopLevel, &DlgSelect);
  }

  if (ret == IDOK && update_file_obj_multi(obj, &farray, FALSE)) {
    FileWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmFileEdit(void)
{
  struct objlist *obj;
  int i;
  char *name;
  char *argv[3];
  pid_t pid;
  int last;

  if (Menulock || GlobalLock)
    return;

  if (Menulocal.editor == NULL)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  last = chkobjlastinst(obj);
  if (last == -1) {
    return;
  } else if (last == 0) {
    i = 0;
  } else {
    CopyDialog(&DlgCopy, obj, -1, FileCB);
    if (DialogExecute(TopLevel, &DlgCopy) == IDOK) {
      i = DlgCopy.sel;
    } else {
      return;
    }
  }

  if (i < 0)
    return;

  if (getobj(obj, "file", i, 0, NULL, &name) == -1)
    return;

  if (name == NULL)
    return;

  argv[0] = Menulocal.editor;
  argv[1] = name;
  argv[2] = NULL;
  pid = fork();
  if (pid == 0) {
    execvp(argv[0], argv);
    exit(1);
  }
}

void
CmFileOpenB(GtkWidget *w, gpointer p)
{
  CmFileOpen();
}

void
CmFileMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdFileNew:
    CmFileNew();
    break;
  case MenuIdFileOpen:
    CmFileOpen();
    break;
  case MenuIdFileUpdate:
    CmFileUpdate();
    break;
  case MenuIdFileClose:
    CmFileClose();
    break;
  case MenuIdFileEdit:
    CmFileEdit();
    break;
  case MenuIdFileMath:
    CmFileWinMath(NULL, NULL);
    break;
  }
}

void
CmOptionFileDef(void)
{
  struct objlist *obj;
  int id;

  if (Menulock || GlobalLock)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  id = newobj(obj);
  if (id >= 0) {
    int modified;

    modified = get_graph_modified();
    FileDefDialog(&DlgFileDef, obj, id);
    if (DialogExecute(TopLevel, &DlgFileDef) == IDOK) {
      if (CheckIniFile()) {
	exeobj(obj, "save_config", id, 0, NULL);
      }
    }
    delobj(obj, id);
    UpdateAll2();
    if (! modified)
      reset_graph_modified();
  }
}

static void
FileWinFileEdit(struct SubWin *d)
{
  int sel;
  char *argv[3], *name;
  pid_t pid;

  if (Menulock || GlobalLock)
    return;

  if (Menulocal.editor == NULL)
    return;

  sel = d->select;

  if (sel < 0 || sel > d->num)
    return;

  if (getobj(d->obj, "file", sel, 0, NULL, &name) == -1)
    return;

  if (name == NULL)
    return;

  argv[0] = Menulocal.editor;
  argv[1] = name;
  argv[2] = NULL;
  pid = fork();
  if (pid == 0) {
    execvp(argv[0], argv);
    exit(1);
  }
}

static void
file_edit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileEdit(d);
}

static void
FileWinFileDelete(struct SubWin *d)
{
  int sel, update;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    FitDel(d->obj, sel);
    delobj(d->obj, sel);
    d->num--;
    update = FALSE;
    if (d->num < 0) {
      d->select = -1;
      update = TRUE;
    } else if (sel > d->num) {
      d->select = d->num;
    } else {
      d->select = sel;
    }
    FileWinUpdate(update);
    set_graph_modified();
  }
}

static void
file_delete_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileDelete(d);
}

static int
file_obj_copy(struct SubWin *d)
{
  int sel, id;

  if (Menulock || GlobalLock)
    return -1;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);

  if ((sel < 0) || (sel > d->num))
    return -1;

  id = newobj(d->obj);

  if (id < 0)
    return -1;

  copy_file_obj_field(d->obj, id, sel, TRUE);
  d->num++;

  return id;
}

static void
FileWinFileCopy(struct SubWin *d)
{
  d->select = file_obj_copy(d);
  FileWinUpdate(FALSE);
}

static void
file_copy_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileCopy(d);
}

static void
FileWinFileCopy2(struct SubWin *d)
{
  int id, sel, j;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  id = file_obj_copy(d);

  if (id < 0) {
    d->select = sel;
    FileWinUpdate(TRUE);
    return;
  }

  for (j = d->num; j > sel + 1; j--) {
    moveupobj(d->obj, j);
  }

  d->select = sel + 1;
  FileWinUpdate(FALSE);
}

static void
file_copy2_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileCopy2(d);
}

static void
FileWinFileUpdate(struct SubWin *d)
{
  int sel, ret;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
 
  if ((sel >= 0) && (sel <= d->num)) {
    d->setup_dialog(d->dialog, d->obj, sel, FALSE);
    d->select = sel;
    if ((ret = DialogExecute(d->Win, d->dialog)) == IDDELETE) {
      FitDel(d->obj, sel);
      delobj(d->obj, sel);
      d->select = -1;
      set_graph_modified();
    }
    d->update(FALSE);
  }
}

static void
FileWinFileDraw(struct SubWin *d)
{
  int i, sel, hidden, h;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_index(GTK_WIDGET(d->text));

  if ((sel >= 0) && (sel <= d->num)) {
    for (i = 0; i <= d->num; i++) {
      hidden = (i != sel);
      getobj(d->obj, "hidden", i, 0, NULL, &h);
      putobj(d->obj, "hidden", i, &hidden);
      if (h != hidden) {
	set_graph_modified();
      }
    }
    d->select = sel;
  } else {
    hidden = FALSE;
    for (i = 0; i <= d->num; i++) {
      getobj(d->obj, "hidden", i, 0, NULL, &h);
      putobj(d->obj, "hidden", i, &hidden);
      if (h != hidden) {
	set_graph_modified();
      }
    }
    d->select = -1;
  }
  CmViewerDrawB(NULL, NULL);
  FileWinUpdate(FALSE);
}

static void
file_draw_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileDraw(d);
}

void
FileWinUpdate(int clear)
{
  struct SubWin *d;

  d = &(NgraphApp.FileWin);

  FileWinExpose(NULL, NULL, NULL);

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID, d->select);
  }
}

static void
FileWinFit(struct SubWin *d)
{
  struct objlist *fitobj, *obj2;
  char *fit;
  int sel, idnum, fitid = 0, ret;
  struct narray iarray;

  if (Menulock || GlobalLock)
    return;

  sel = d->select;

  if (sel < 0 || sel > d->num)
    return;

  if ((fitobj = chkobject("fit")) == NULL)
    return;

  if (getobj(d->obj, "fit", sel, 0, NULL, &fit) == -1)
    return;

  if (fit) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &obj2, &iarray, FALSE, NULL)) {
      arraydel(&iarray);
      return;
    }
    idnum = arraynum(&iarray);

    if ((obj2 != fitobj) || (idnum < 1)) {
      if (putobj(d->obj, "fit", sel, NULL) == -1) {
	arraydel(&iarray);
	return;
      }
    } else {
      fitid = *(int *) arraylast(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL)
    return;

  FitDialog(&DlgFit, fitobj, fitid);

  ret = DialogExecute(d->Win, &DlgFit);
  if (ret == IDDELETE) {
    delobj(fitobj, fitid);
    putobj(d->obj, "fit", sel, NULL);
  }
}

static void
file_fit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFit(d);
}


static GdkPixbuf *
draw_type_pixbuf(struct objlist *obj, int i)
{
  int j, k, ggc, fr, fg, fb, fr2, fg2, fb2,
    type, width = 40, height = 20, poly[14], marktype,
    intp, spcond, spnum, lockstate, found, output;
  double spx[7], spy[7], spz[7], spc[6][7], spc2[6];
  GdkPixmap *pix;
  GdkPixbuf *pixbuf;
  struct objlist *gobj, *robj;
  char *inst, *name;
  struct gra2cairo_local *local;

  lockstate = GlobalLock;
  GlobalLock = TRUE;

  found = find_gra2gdk_inst(&name, &gobj, &inst, &robj, &output, &local);
  if (! found) {
    return NULL;
  }

  pix = gra2gdk_create_pixmap(gobj, inst, local, TopLevel->window,
			      width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return NULL;
  }


  getobj(obj, "type", i, 0, NULL, &type);
  getobj(obj, "R", i, 0, NULL, &fr);
  getobj(obj, "G", i, 0, NULL, &fg);
  getobj(obj, "B", i, 0, NULL, &fb);

  getobj(obj, "R2", i, 0, NULL, &fr2);
  getobj(obj, "G2", i, 0, NULL, &fg2);
  getobj(obj, "B2", i, 0, NULL, &fb2);

  ggc = _GRAopen("gra2gdk", "_output",
		 robj, inst, output, -1,
		 -1, -1, NULL, local);
  if (ggc < 0) {
    _GRAclose(ggc);
    return NULL;
  }
  GRAview(ggc, 0, 0, width, height, 0);

  switch (type) {
  case PLOT_TYPE_MARK:
    getobj(obj, "mark_type", i, 0, NULL, &marktype);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    GRAmark(ggc, marktype, height / 2, height / 2, height - 2,
	    fr, fg, fb, fr2, fg2, fb2);
    break;
  case PLOT_TYPE_LINE:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    GRAline(ggc, 1, height / 2, height - 1, height / 2);
    break;
  case PLOT_TYPE_POLYGON:
    poly[0] = 1;
    poly[1] = height / 2;

    poly[2] = height / 4;
    poly[3] = 1;

    poly[4] = height * 3 / 4;
    poly[5] = height - 1;

    poly[6] = height - 1;
    poly[7] = height / 2;

    poly[8] = height * 3 / 4;
    poly[9] = 1;

    poly[10] = height / 4;
    poly[11] = height - 1;

    poly[12] = 1;
    poly[13] = height / 2;
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    GRAdrawpoly(ggc, 7, poly, 0);
    break;
  case PLOT_TYPE_CURVE:
    spx[0] = 1;
    spx[1] = height / 3 + 1;
    spx[2] = height * 2 / 3;
    spx[3] = height - 1;
    spx[4] = height * 2 / 3;
    spx[5] = height / 3 + 1;
    spx[6] = 1;

    spy[0] = height / 2;
    spy[1] = 1;
    spy[2] = height - 1;
    spy[3] = height / 2;
    spy[4] = 1;
    spy[5] = height - 1;
    spy[6] = height / 2;
    for (j = 0; j < 7; j++)
      spz[j] = j;
    getobj(obj, "interpolation", i, 0, NULL, &intp);
    if ((intp == 0) || (intp == 2)) {
      spcond = SPLCND2NDDIF;
      spnum = 4;
    } else {
      spcond = SPLCNDPERIODIC;
      spnum = 7;
    }
    spline(spz, spx, spc[0], spc[1], spc[2], spnum, spcond, spcond, 0, 0);
    spline(spz, spy, spc[3], spc[4], spc[5], spnum, spcond, spcond, 0, 0);
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (intp >= 2) {
      GRAmoveto(ggc, height, height * 3 / 4);
      GRAtextstyle(ggc, "Times", 52, 0, 0);
      GRAouttext(ggc, "B");
    }
    GRAcurvefirst(ggc, 0, NULL, NULL, NULL, splinedif, splineint, NULL, spx[0], spy[0]);
    for (j = 0; j < spnum - 1; j++) {
      for (k = 0; k < 6; k++) {
	spc2[k] = spc[k][j];
      }
      if (!GRAcurve(ggc, spc2, spx[j], spy[j])) break;
    }
    break;
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    GRAcolor(ggc, fr, fg, fb);
    if ((type == PLOT_TYPE_DIAGONAL) || (type == PLOT_TYPE_ARROW))
      GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    else
      GRAlinestyle(ggc, 0, NULL, 1, 2, 0, 1000);
    spx[0] = 1;
    spy[0] = height - 1;

    spx[1] = height - 1;
    spy[1] = 1;
    if ((type == PLOT_TYPE_DIAGONAL) || (type == PLOT_TYPE_ARROW))
      GRAline(ggc, spx[0], spy[0], spx[1], spy[1]);
    if (type == PLOT_TYPE_ARROW) {
      poly[0] = height - 6;
      poly[1] = 1;

      poly[2] = spx[1];
      poly[3] = spy[1];

      poly[4] = height - 1;
      poly[5] = 6;
      GRAdrawpoly(ggc, 3, poly, 1);
    }
    if ((type == PLOT_TYPE_RECTANGLE_FILL) || (type == PLOT_TYPE_RECTANGLE_SOLID_FILL)) {
      if (type == PLOT_TYPE_RECTANGLE_FILL)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, spx[0], spy[0], spx[1], spy[1], 1);
      if (type == PLOT_TYPE_RECTANGLE_FILL)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == PLOT_TYPE_RECTANGLE) || (type == PLOT_TYPE_RECTANGLE_FILL)) {
      GRAline(ggc, spx[0], spy[0], spx[0], spy[1]);
      GRAline(ggc, spx[0], spy[1], spx[1], spy[1]);
      GRAline(ggc, spx[1], spy[1], spx[1], spy[0]);
      GRAline(ggc, spx[1], spy[0], spx[0], spy[0]);
    }
    break;
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (type == PLOT_TYPE_ERRORBAR_X) {
      GRAline(ggc, 1, height / 2, height - 1, height / 2);
      GRAline(ggc, 1, height / 4, 1, height * 3 / 4);
      GRAline(ggc, height - 1, height / 4, height - 1, height * 3 / 4);
    } else {
      GRAline(ggc, height / 2, 1, height / 2, height - 1);
      GRAline(ggc, height / 4, 1, height * 3 / 4, 1);
      GRAline(ggc, height / 4, height -1, height * 3 / 4, height - 1);
    }
    break;
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (type == PLOT_TYPE_STAIRCASE_X) {
      GRAmoveto(ggc, 1, height - 1);
      GRAlineto(ggc, height / 4, height - 1);
      GRAlineto(ggc, height / 4, height / 2);
      GRAlineto(ggc, height * 3 / 4, height / 2);
      GRAlineto(ggc, height * 3 / 4, 1);
      GRAlineto(ggc, height - 1, 1);
    } else {
      GRAmoveto(ggc, 1, height - 1);
      GRAlineto(ggc, 1, height / 2 + 1);
      GRAlineto(ggc, height / 2, height / 2 + 1);
      GRAlineto(ggc, height / 2, height / 4);
      GRAlineto(ggc, height - 1, height / 4);
      GRAlineto(ggc, height - 1, 1);
    }
    break;
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 2, 0, 1000);
    if ((type == PLOT_TYPE_BAR_FILL_X) || (type == PLOT_TYPE_BAR_SOLID_FILL_X)) {
      if (type == PLOT_TYPE_BAR_FILL_X)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, 1, height / 4, height - 1, height * 3 / 4, 1);
      if (type == PLOT_TYPE_BAR_FILL_X)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == PLOT_TYPE_BAR_FILL_Y) || (type == PLOT_TYPE_BAR_SOLID_FILL_Y)) {
      if (type == PLOT_TYPE_BAR_FILL_Y)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, height / 3, 1, height * 3 / 4, height - 1, 1);
      if (type == PLOT_TYPE_BAR_FILL_Y)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == PLOT_TYPE_BAR_X) || (type == PLOT_TYPE_BAR_FILL_X)) {
      GRAline(ggc, 1,          height / 4,     height - 1, height /4);
      GRAline(ggc, height - 1, height / 4,     height - 1, height * 3 / 4);
      GRAline(ggc, height - 1, height * 3 / 4, 1,          height * 3 / 4);
      GRAline(ggc, 1,          height * 3 / 4, 1,          height / 4);
    }
    if ((type == PLOT_TYPE_BAR_Y) || (type == PLOT_TYPE_BAR_FILL_Y)) {
      GRAline(ggc, height / 4,     1,          height * 3 / 4, 1);
      GRAline(ggc, height * 3 / 4, 1,          height * 3 / 4, height - 1);
      GRAline(ggc, height * 3/ 4,  height - 1, height / 4,     height - 1);
      GRAline(ggc, height / 4,     height - 1, height / 4,     1);
    }
    break;
  case PLOT_TYPE_FIT:
    GRAcolor(ggc, fr, fg, fb);
    GRAmoveto(ggc, 1, height * 3 / 4);
    GRAtextstyle(ggc, "Times", 52, 0, 0);
    GRAouttext(ggc, "fit");
    break;
  }

  _GRAclose(ggc);
  if (local->linetonum && local->cairo) {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }

  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, width, height);

  g_object_unref(G_OBJECT(pix));

  GlobalLock = lockstate;

  return pixbuf;
}

static char *
get_axis_obj_str(struct objlist *obj, int id, char *field)
{
  char *tmp, *valstr;
  int j;

  sgetobjfield(obj, id, field, NULL, &valstr, FALSE, FALSE, FALSE);
  for (j = 0; (valstr[j] != '\0') && (valstr[j] != ':'); j++);
  if (valstr[j] == ':') j++;
  tmp = strdup(valstr + j);
  memfree(valstr);

  return tmp;
}

static void
file_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row)
{
  int cx, len;
  unsigned int i;
  char buf[256], *color;
  struct narray *mask, *move;
  char *file, *bfile, *axis;
  GdkPixbuf *pixbuf = NULL;

  for (i = 0; i < FILE_WIN_COL_NUM; i++) {
    switch (i) {
    case FILE_WIN_COL_FILE:
      getobj(d->obj, "mask", row, 0, NULL, &mask);
      getobj(d->obj, "move_data", row, 0, NULL, &move);
      getobj(d->obj, "file", row, 0, NULL, &file);
      if ((arraynum(mask) != 0) || (arraynum(move) != 0)) {
	color = "blue";
      } else {
	color = "black";
      }
      bfile = getbasename(file);
      if (bfile) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, bfile);
	memfree(bfile);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
      }
      list_store_set_string(GTK_WIDGET(d->text), iter, FILE_WIN_COL_NUM, color);
      break;
    case FILE_WIN_COL_TYPE:
      pixbuf = draw_type_pixbuf(d->obj, row);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case FILE_WIN_COL_X_AXIS:
    case FILE_WIN_COL_Y_AXIS:
      axis = get_axis_obj_str(d->obj, row, Flist[i].name);
      if (axis) {
	len = snprintf(buf, sizeof(buf), "%3s", axis);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
	free(axis);
      }
      break;
    case FILE_WIN_COL_HIDDEN:
      getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
      break;
    default:
      if (Flist[i].type == G_TYPE_DOUBLE) {
	getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
	list_store_set_double(GTK_WIDGET(d->text), iter, i, cx / 100.0);
      } else {
	getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
	list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
      }
    }
  }
}

static gboolean
FileWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data)
{
  struct SubWin *d;

  if (Menulock || GlobalLock)
    return FALSE;

  d = &(NgraphApp.FileWin);

  if (GTK_WIDGET(d->text) == NULL)
    return FALSE;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, file_list_set_val);
  } else {
    list_sub_window_set(d, file_list_set_val);
  }

  return FALSE;
}

void
CmFileWinMath(GtkWidget *w, gpointer p)
{
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;

  obj = chkobject("file");

  if (chkobjlastinst(obj) < 0)
    return;

  MathDialog(&DlgMath, obj);
  DialogExecute(TopLevel, &DlgMath);
}

static gboolean
filewin_ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
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
  case GDK_Delete:
    FileWinFileDelete(d);
    break;
  case GDK_Return:
    if (e->state & GDK_SHIFT_MASK) {
      return FALSE;
    }

    FileWinFileUpdate(d);
    break;
  case GDK_Insert:
    if (e->state & GDK_SHIFT_MASK)
      FileWinFileCopy2(d);
    else
      FileWinFileCopy(d);
    break;
  case GDK_space:
    if (e->state & GDK_CONTROL_MASK)
      return FALSE;

    FileWinFileDraw(d);
    break;
  case GDK_F:
    FileWinFit(d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}


static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel;
  unsigned int i;
  struct SubWin *d;
  char *fit;

  d = (struct SubWin *) user_data;

  sel = d->select;
  for (i = 1; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_FIT:
      fit = NULL;
      if (sel >= 0) {
	getobj(d->obj, "fit", sel, 0, NULL, &fit);
      }
      gtk_widget_set_sensitive(d->popup_item[i], fit != NULL);
      break;
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

enum FILE_COMBO_ITEM {
  FILE_COMBO_ITEM_COLOR_1,
  FILE_COMBO_ITEM_COLOR_2,
  FILE_COMBO_ITEM_TYPE,
  FILE_COMBO_ITEM_MARK,
  FILE_COMBO_ITEM_INTP,
};


static void
create_type_combo_box(GtkWidget *cbox, struct objlist *obj, int type)
{
  char **enumlist, **curvelist;
  unsigned int i;
  int j, count;
  GtkTreeStore *list;
  GtkTreeIter parent;

  count = combo_box_get_num(cbox);
  if (count > 0)
    return;

  enumlist = (char **) chkobjarglist(obj, "type");
  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  gtk_tree_store_append(list, &parent, NULL);
  gtk_tree_store_set(list, &parent, 0, NULL, 1, _("Color 1"), 2, FILE_COMBO_ITEM_COLOR_1, -1);

  switch (type) {
  case PLOT_TYPE_MARK:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    gtk_tree_store_append(list, &parent, NULL);
    gtk_tree_store_set(list, &parent, 0, NULL, 1, _("Color 2"), 2, FILE_COMBO_ITEM_COLOR_2, -1);
    break;
  }

  gtk_tree_store_append(list, &parent, NULL);
  gtk_tree_store_set(list, &parent, 0, NULL, 1, _("Type"), 2, FILE_COMBO_ITEM_TYPE, -1);

  for (i = 0; enumlist[i]; i++) {
    GtkTreeIter iter, child;

    gtk_tree_store_append(list, &iter, &parent);
    gtk_tree_store_set(list, &iter, 0, NULL, 1, _(enumlist[i]), 2, FILE_COMBO_ITEM_TYPE, -1);

    if (strcmp(enumlist[i], "mark") == 0) {
      for (j = 0; j < MARK_TYPE_NUM; j++) {
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_get_from_drawable(NULL, NgraphApp.markpix[j], NULL, 0, 0, 0, 0, -1, -1);

	gtk_tree_store_append(list, &child, &iter);
	gtk_tree_store_set(list, &child, 0, pixbuf, 1, NULL, 2, FILE_COMBO_ITEM_MARK, -1);
      }
    } else if (strcmp(enumlist[i], "curve") == 0) {
      curvelist = (char **) chkobjarglist(obj, "interpolation");
      for (j = 0; curvelist[j]; j++) {
	gtk_tree_store_append(list, &child, &iter);
	gtk_tree_store_set(list, &child, 0, NULL, 1, _(curvelist[j]), 2, FILE_COMBO_ITEM_INTP, -1);
      }
    }
  }
}

static int
select_color(struct objlist *obj, int id, enum  FILE_COMBO_ITEM type)
{
  GtkWidget *dlg, *sel;
  int r, g, b, response;
  GdkColor color;
  char *title;

  switch (type) {
  case FILE_COMBO_ITEM_COLOR_1:
    title = _("Color 1");
    getobj(obj, "R", id, 0, NULL, &r);
    getobj(obj, "G", id, 0, NULL, &g);
    getobj(obj, "B", id, 0, NULL, &b);
    break;
  case FILE_COMBO_ITEM_COLOR_2:
    title = _("Color 2");
    getobj(obj, "R2", id, 0, NULL, &r);
    getobj(obj, "G2", id, 0, NULL, &g);
    getobj(obj, "B2", id, 0, NULL, &b);
    break;
  default:
    return 1;
  }

  color.red = (r & 0xff) * 257;
  color.green = (g & 0xff) * 257;
  color.blue = (b & 0xff) * 257;

  dlg = gtk_color_selection_dialog_new(title);

#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14))
  sel = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dlg));
#else
  sel = GTK_COLOR_SELECTION_DIALOG(dlg)->colorsel;
#endif

  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(sel), &color);
  gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(sel), TRUE);

  response = gtk_dialog_run(GTK_DIALOG(dlg));
  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(sel), &color);
  gtk_widget_destroy(dlg);

  if (response != GTK_RESPONSE_OK)
    return 1;

  r = (color.red >> 8);
  g = (color.green >> 8);
  b = (color.blue >> 8);

  switch (type) {
  case FILE_COMBO_ITEM_COLOR_1:
    putobj(obj, "R", id, &r);
    putobj(obj, "G", id, &g);
    putobj(obj, "B", id, &b);
    break;
  case FILE_COMBO_ITEM_COLOR_2:
    putobj(obj, "R2", id, &r);
    putobj(obj, "G2", id, &g);
    putobj(obj, "B2", id, &b);
  default:
    return 1;
  }

  return 0;
}

static void
select_type(GtkComboBox *w, gpointer user_data)
{
  int sel, col_type, type, mark_type, curve_type, a, b, c, *ary, found, depth;
  struct objlist *obj;
  struct SubWin *d;
  GtkTreeStore *list;
  GtkTreeIter iter;
  GtkTreePath *path;

  menu_lock(FALSE);

  d = (struct SubWin *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  obj = getobject("file");
  getobj(obj, "type", sel, 0, NULL, &type);

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, 2, &col_type, -1);
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);
  ary = gtk_tree_path_get_indices(path);
  depth = gtk_tree_path_get_depth(path);
  a = b = c = -1;

  switch (depth) {
  case 3:
    c = ary[2];
  case 2:
    b = ary[1];
  case 1:
    a = ary[0];
    break;
  default:
    return;
  }

  gtk_tree_path_free(path);

  switch (col_type) {
  case FILE_COMBO_ITEM_COLOR_1:
  case FILE_COMBO_ITEM_COLOR_2:
    if (select_color(obj, sel, a))
      return;
    break;
  case FILE_COMBO_ITEM_TYPE:
    if (b < 0 || b == type) {
      return;
    }
    putobj(obj, "type", sel, &b);
    break;
  case FILE_COMBO_ITEM_MARK:
    getobj(obj, "mark_type", sel, 0, NULL, &mark_type);

    if (c < 0)
      c = mark_type;

    if (b == type && c == mark_type)
      return;

    putobj(obj, "mark_type", sel, &c);
    putobj(obj, "type", sel, &b);

    break;
  case FILE_COMBO_ITEM_INTP:
    getobj(obj, "interpolation", sel, 0, NULL, &curve_type);

    if (c < 0)
      c = curve_type;

    if (b == type && c == curve_type)
      return;

    putobj(obj, "interpolation", sel, &c);
    putobj(obj, "type", sel, &b);

    break;
  default:
    return;
  }


  d->select = sel;
  d->update(FALSE);
  set_graph_modified();
}

static void
start_editing_type(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  n_list_store *list;
  struct SubWin *d;
  GtkComboBox *cbox;
  int sel, type, child = -1;
  struct objlist *obj;

  menu_lock(TRUE);

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path_str))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);

  list = (n_list_store *) g_object_get_data(G_OBJECT(renderer), "user-data");
  sel = list_store_get_selected_int(GTK_WIDGET(view), FILE_WIN_COL_ID);

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  obj = getobject("file");
  if (obj == NULL)
    return;

  getobj(obj, "type", sel, 0, NULL, &type);

  create_type_combo_box(GTK_WIDGET(cbox), obj, type);

  switch (type) {
  case PLOT_TYPE_MARK:
    getobj(obj, "mark_type", sel, 0, NULL, &child);
    break;
  case PLOT_TYPE_CURVE:
    getobj(obj, "interpolation", sel, 0, NULL, &child);
    break;
  }

  gtk_widget_show(GTK_WIDGET(cbox));

  g_signal_connect(cbox, "editing-done", G_CALLBACK(select_type), d);

  return;
}

static void
select_axis(GtkComboBox *w, gpointer user_data, char *axis)
{
  char buf[64];
  int j, sel;
  struct SubWin *d;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  d = (struct SubWin *) user_data;

  j = combo_box_get_active(GTK_WIDGET(w));

  if (j < 0)
    return;

  snprintf(buf, sizeof(buf), "axis:%d", j);
  if (sputobjfield(d->obj, sel, axis, buf) == 0) {
    d->select = sel;
  }
}

static void
select_axis_x(GtkComboBox *w, gpointer user_data)
{
  select_axis(w, user_data, "axis_x");
}

static void
select_axis_y(GtkComboBox *w, gpointer user_data)
{
  select_axis(w, user_data, "axis_y");
}

#define AXIS_X 0
#define AXIS_Y 1

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data, int axis)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct SubWin *d;
  GtkComboBox *cbox;
  int lastinst, j, sel, id = 0, is_oid;
  struct objlist *aobj;
  char *name, *ptr;

  menu_lock(TRUE);

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);

  sel = list_store_get_selected_int(GTK_WIDGET(view), FILE_WIN_COL_ID);

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  combo_box_clear(GTK_WIDGET(cbox));
  aobj = getobject("axis");
  lastinst = chkobjlastinst(aobj);
  for (j = 0; j <= lastinst; j++) {
    getobj(aobj, "group", j, 0, NULL, &name);
    name = CHK_STR(name);
    combo_box_append_text(GTK_WIDGET(cbox), name);
  }

  name = get_axis_obj_str(d->obj, sel, (axis == AXIS_X) ? "axis_x" : "axis_y");
  if (name) {
    is_oid = (name[0] == '^');
    id = strtol(name + is_oid, &ptr, 10);
    if (*ptr == '\0') {
      if (is_oid)
	id = chkobjoid(aobj, id);

      combo_box_set_active(GTK_WIDGET(cbox), id);
    }
    free(name);
  }

  d->select = -1;
  g_signal_connect(cbox, "changed", G_CALLBACK((axis == AXIS_X) ? select_axis_x : select_axis_y), d);
}

static void
start_editing_x(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  start_editing(renderer, editable, path, user_data, AXIS_X);
}

static void
start_editing_y(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  start_editing(renderer, editable, path, user_data, AXIS_Y);
}

static void
edited_axis(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data, char *axis)
{
  struct SubWin *d;

  menu_lock(FALSE);

  d = (struct SubWin *) user_data;

  if (str == NULL || d->select < 0)
    return;

  d->update(FALSE);
  set_graph_modified();
}

static void 
drag_drop_cb(GtkWidget *w, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  gchar **filenames;
  int num;

  switch (info) {
  case DROP_TYPE_FILE:
    filenames = gtk_selection_data_get_uris(data);
    num = g_strv_length(filenames);
    data_dropped(filenames, num, FILE_TYPE_DATA);
    g_strfreev(filenames);
    gtk_drag_finish(context, TRUE, FALSE, time);
    break;
  }
}

static void
init_dnd(struct SubWin *d)
{
  GtkWidget *widget;
  GtkTargetEntry target[] = {
    {"text/uri-list", 0, DROP_TYPE_FILE},
  };

  widget = d->Win;

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, target, sizeof(target) / sizeof(*target), GDK_ACTION_COPY);
  g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_drop_cb), NULL);
}

void
CmFileWindow(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  d = &(NgraphApp.FileWin);
  d ->type = TypeFileWin;

  if (d->Win) {
    sub_window_toggle_visibility(d);
  } else {
    GtkWidget *dlg;

    d->update = FileWinUpdate;
    d->setup_dialog = FileDialog;
    d->dialog = &DlgFile;
    d->ev_key = filewin_ev_key_down;

    dlg = list_sub_window_create(d, "Data Window", FILE_WIN_COL_NUM, Flist, Filewin_xpm, Filewin48_xpm);

    g_signal_connect(dlg, "expose-event", G_CALLBACK(FileWinExpose), NULL);

    d->obj = chkobject("file");
    d->num = chkobjlastinst(d->obj);

    sub_win_create_popup_menu(d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
    set_combo_cell_renderer_cb(d, FILE_WIN_COL_X_AXIS, Flist, G_CALLBACK(start_editing_x), G_CALLBACK(edited_axis));
    set_combo_cell_renderer_cb(d, FILE_WIN_COL_Y_AXIS, Flist, G_CALLBACK(start_editing_y), G_CALLBACK(edited_axis));
    set_obj_cell_renderer_cb(d, FILE_WIN_COL_TYPE, Flist, G_CALLBACK(start_editing_type));

    init_dnd(d);

    sub_window_show_all(d);
    sub_window_set_geometry(d, TRUE);
  }
}
