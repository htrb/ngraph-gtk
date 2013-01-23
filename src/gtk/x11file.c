// -*- coding: utf-8 -*-
/*
 * $Id: x11file.c,v 1.136 2010-03-04 08:30:17 hito Exp $
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
#include "shell.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"
#include "nconfig.h"
#include "ofile.h"
#include "ofit.h"

#include "math_equation.h"

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
  {" ",	        G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",		G_TYPE_INT,     TRUE, FALSE, "id"},
  {N_("file"),	G_TYPE_STRING,  TRUE, TRUE,  "file"},
  {"x   ",	G_TYPE_INT,     TRUE, TRUE,  "x",  0, 999, 1, 10},
  {"y   ",	G_TYPE_INT,     TRUE, TRUE,  "y",  0, 999, 1, 10},
  {N_("ax"),	G_TYPE_PARAM,   TRUE, TRUE,  "axis_x"},
  {N_("ay"),	G_TYPE_PARAM,   TRUE, TRUE,  "axis_y"},
  {N_("type"),	G_TYPE_OBJECT,  TRUE, TRUE,  "type"},
  {N_("size"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "mark_size",  0,       SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "line_width", 0,       SPIN_ENTRY_MAX, 10,   100},
  {N_("skip"),	G_TYPE_INT,     TRUE, TRUE,  "head_skip",  0,       INT_MAX,         1,    10},
  {N_("step"),	G_TYPE_INT,     TRUE, TRUE,  "read_step",  1,       INT_MAX,         1,    10},
  {N_("final"),	G_TYPE_INT,     TRUE, TRUE,  "final_line", INT_MIN, INT_MAX,    1,    10},
  {N_("num"), 	G_TYPE_INT,     TRUE, FALSE, "data_num"},
  {"^#",	G_TYPE_INT,     TRUE, FALSE, "oid"},
  {"masked",	G_TYPE_INT,     FALSE, FALSE, "masked"},
};

enum {
  FILE_WIN_COL_HIDDEN,
  FILE_WIN_COL_ID,
  FILE_WIN_COL_FILE,
  FILE_WIN_COL_X,
  FILE_WIN_COL_Y,
  FILE_WIN_COL_X_AXIS,
  FILE_WIN_COL_Y_AXIS,
  FILE_WIN_COL_TYPE,
  FILE_WIN_COL_SIZE,
  FILE_WIN_COL_WIDTH,
  FILE_WIN_COL_SKIP,
  FILE_WIN_COL_STEP,
  FILE_WIN_COL_FINAL,
  FILE_WIN_COL_DNUM,
  FILE_WIN_COL_OID,
  FILE_WIN_COL_MASKED,
  FILE_WIN_COL_NUM,
};

static void file_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
static void file_delete_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy2_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_fit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_edit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_draw_popup_func(GtkMenuItem *w, gpointer client_data);
static void FileDialogType(GtkWidget *w, gpointer client_data);
static void create_type_combo_box(GtkWidget *cbox, struct objlist *obj, GtkTreeIter *parent);
static gboolean func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data);

static struct subwin_popup_list Popup_list[] = {
  {GTK_STOCK_ADD,             G_CALLBACK(CmFileOpen), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
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

enum MATH_FNC_TYPE {
  TYPE_MATH_X = 0,
  TYPE_MATH_Y,
  TYPE_FUNC_F,
  TYPE_FUNC_G,
  TYPE_FUNC_H,
};

static char *FieldStr[] = {"math_x", "math_y", "func_f", "func_g", "func_h"};

static void
MathTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct MathTextDialog *d;
  static char *label[] = {N_("_Math X:"), N_("_Math Y:"), "_F(X,Y,Z):", "_G(X,Y,Z):", "_H(X,Y,Z):"};

  d = (struct MathTextDialog *) data;
  if (makewidget) {
#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif
    w = create_text_entry(TRUE, TRUE);
    d->label = item_setup(hbox, w, _("_Math:"), TRUE);
    d->list = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }

  switch (d->Mode) {
  case TYPE_MATH_X:
    entry_completion_set_entry(NgraphApp.x_math_list, d->list);
    break;
  case TYPE_MATH_Y:
    entry_completion_set_entry(NgraphApp.y_math_list, d->list);
    break;
  case TYPE_FUNC_F:
  case TYPE_FUNC_G:
  case TYPE_FUNC_H:
    entry_completion_set_entry(NgraphApp.func_list, d->list);
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
  char *obuf, *ptr;
  int r, id;
  GList *id_ptr;

  d = (struct MathTextDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCANCEL:
    return;
  default:
    d->ret = IDLOOP;
    return;
  }

  p = gtk_entry_get_text(GTK_ENTRY(d->list));
  ptr = g_strdup(p);
  if (ptr == NULL) {
    return;
  }

  for (id_ptr = d->id_list; id_ptr; id_ptr = id_ptr->next) {
    r = list_store_path_get_int(d->tree, id_ptr->data, 0, &id);
    if (r)
      continue;

    sgetobjfield(d->Obj, id, FieldStr[d->Mode], NULL, &obuf, FALSE, FALSE, FALSE);
    if (obuf == NULL || strcmp(obuf, ptr)) {
      if (sputobjfield(d->Obj, id, FieldStr[d->Mode], ptr)) {
	g_free(ptr);
	d->ret = IDLOOP;
	return;
      }
      set_graph_modified();
    }
    g_free(obuf);
  }
  g_free(ptr);

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
}

void
MathTextDialog(struct MathTextDialog *data, char *text, int mode, struct objlist *obj, GList *list, GtkWidget *tree)
{
  if (mode < 0 || mode >= MATH_FNC_NUM)
    mode = 0;

  data->SetupWindow = MathTextDialogSetup;
  data->CloseWindow = MathTextDialogClose;
  data->tree = tree;
  data->Text = text;
  data->Mode = mode;
  data->Obj = obj;
  data->id_list = list;
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
  char *field = NULL, *buf;
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

  MathTextDialog(&DlgMathText, buf, d->Mode, d->Obj, list, d->list);

  DialogExecute(d->widget, &DlgMathText);

  g_free(buf);

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

  if (e->keyval != GDK_KEY_Return)
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

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
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

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect(sel, "changed", G_CALLBACK(set_btn_sensitivity_selection_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static void
MathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *vbox, *hbox;
  struct MathDialog *d;
  static n_list_store list[] = {
    {"id",       G_TYPE_INT,    TRUE, FALSE, NULL},
    {N_("math"), G_TYPE_STRING, TRUE, FALSE, NULL},
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

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);
#endif
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_all(w);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(math_dialog_key_pressed_cb), d);
    g_signal_connect(w, "button-press-event", G_CALLBACK(math_dialog_butten_pressed_cb), d);
    d->list = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
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

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

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
    d->ok_button = GTK_STOCK_CLOSE;

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
  int i;
  char *s;

  d = (struct FitSaveDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   NULL);

    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif
    item_setup(hbox, w, _("_Profile:"), TRUE);
    d->profile = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
  }
  combo_box_clear(d->profile);
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

    ptr = g_strdup(s);
    g_strstrip(ptr);
    if (ptr[0] != '\0') {
      d->Profile = ptr;
      return;
    }
    g_free(ptr);
  }

  message_box(d->widget, _("Please specify the profile."), NULL, RESPONS_OK);

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

  SetWidgetFromObjField(d->converge, d->Obj, id, "converge");

  SetWidgetFromObjField(d->derivatives, d->Obj, id, "derivative");

  SetWidgetFromObjField(d->formula, d->Obj, id, "user_func");

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
  char *valstr, *profile;

  getobj(obj, "profile", id, 0, NULL, &profile);

  if (profile == NULL) {
    char *tmp;

    sgetobjfield(obj, id, "type", NULL, &tmp, FALSE, FALSE, FALSE);
    valstr = g_strdup(_(tmp));
    g_free(tmp);
  } else {
    valstr = NULL;
  }

  return valstr;
}

static void
FitDialogCopy(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  int sel;

  d = (struct FitDialog *) user_data;
  sel = CopyClick(d->widget, d->Obj, d->Id, FitCB);
  if (sel != -1)
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
	message_box(d->widget, _("Setting file not found."), FITSAVE, RESPONS_OK);
      return FALSE;
    }
    if ((shell = chkobject("shell")) == NULL)
      return FALSE;
    newid = newobj(shell);
    if (newid < 0) {
      g_free(file);
      return FALSE;
    }
    arrayinit(&sarray, sizeof(char *));
    changefilename(file);
    if (arrayadd(&sarray, &file) == NULL) {
      g_free(file);
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
FitDialogLoad(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  int lastid, id;

  d = (struct FitDialog *) user_data;

  if (!FitDialogLoadConfig(d, TRUE))
    return;

  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    message_box(d->widget, _("No settings."), FITSAVE, RESPONS_OK);
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
      if (message_box(d->widget, _("Overwrite existing profile?"), "Confirm",
		     RESPONS_YESNO) != IDYES) {
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
  int i, r;
  char *s, *ptr;

  if (profile == NULL)
    return 1;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      ptr = g_strdup_printf(_("Delete the profile '%s'?"), profile);
      r = message_box(d->widget, ptr, "Confirm", RESPONS_YESNO);
      g_free(ptr);
      if (r != IDYES) {
	return 1;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    ptr = g_strdup_printf(_("The profile '%s' is not exist."), profile);
    message_box(d->widget, ptr, "Confirm", RESPONS_OK);
    g_free(ptr);
    return 1;
  }

  delobj(d->Obj, i);

  return 0;
}

static void
FitDialogSave(GtkWidget *w, gpointer client_data)
{
  int i, r, len;
  char *s, *ngpfile, *ptr;
  int error;
  int hFile;
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  r = DialogExecute(d->widget, &DlgFitSave);
  if (r != IDOK && r != IDDELETE)
    return;

  if (DlgFitSave.Profile == NULL)
    return;

  if (DlgFitSave.Profile[0] == '\0') {
    g_free(DlgFitSave.Profile);
    return;
  }

  switch (r) {
  case IDOK:
    if (copy_settings_to_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return;
    }
    break;
  case IDDELETE:
    if (delete_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return;
    }
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

  if (error) {
    ErrorMessage();
  } else {
    switch (r) {
    case IDOK:
      ptr = g_strdup_printf(_("The profile '%s' is saved."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      break;
    case IDDELETE:
      ptr = g_strdup_printf(_("The profile '%s' is deleted."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      g_free(DlgFitSave.Profile);
      break;
    }
  }

  g_free(ngpfile);
}

static int
check_fit_func(GtkEditable *w, gpointer client_data)
{
  struct FitDialog *d;
  MathEquation *code;
  MathEquationParametar *prm;
  const char *math;
  int dim, i, n, deriv;

  d = (struct FitDialog *) client_data;

  code = math_equation_basic_new();
  if (code == NULL)
    return FALSE;

  if (math_equation_add_parameter(code, 0, 1, 2, MATH_EQUATION_PARAMETAR_USE_ID)) {
    math_equation_free(code);
    return FALSE;
  }

  math = gtk_entry_get_text(GTK_ENTRY(d->formula));
  if (math_equation_parse(code, math)) {
    math_equation_free(code);
    return FALSE;
  }

  prm = math_equation_get_parameter(code, 0, NULL);
  dim = prm->id_num;
  deriv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->derivatives));

  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->p[i], FALSE);
    set_widget_sensitivity_with_label(d->d[i], FALSE);
  }

  for (i = 0; i < dim; i++) {
    n = prm->id[i];

    if (n < FIT_PARM_NUM) {
      set_widget_sensitivity_with_label(d->p[n], TRUE);
      if (deriv) {
	set_widget_sensitivity_with_label(d->d[n], TRUE);
      }
    }
  }

  math_equation_free(code);

  return TRUE;
}

static void
FitDialogResult(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;
  double derror, correlation, coe[FIT_PARM_NUM];
  char *equation, *math, buf[1024];
  N_VALUE *inst;
  int i, j, dim, dimension, type, num;

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
    int tbl[FIT_PARM_NUM];
    MathEquation *code;
    MathEquationParametar *prm;

    code = math_equation_basic_new();
    if (code == NULL)
      return;

    if (math_equation_add_parameter(code, 0, 1, 2, MATH_EQUATION_PARAMETAR_USE_ID)) {
      math_equation_free(code);
      return;
    }

    if (math_equation_parse(code, math)) {
      math_equation_free(code);
      return;
    }
    prm = math_equation_get_parameter(code, 0, NULL);
    dim = prm->id_num;
    for (i = 0; i < dim; i++) {
      tbl[i] = prm->id[i];
    }
    math_equation_free(code);
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
  message_box(d->widget, buf, _("Fitting Results"), RESPONS_OK);
}

static int
FitDialogApply(GtkWidget *w, struct FitDialog *d)
{
  int i, num, dim;
  const gchar *s;

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

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, d->Id, "derivative"))
    return FALSE;

  if (SetObjFieldFromWidget(d->converge, d->Obj, d->Id, "converge"))
    return FALSE;

  if (SetObjFieldFromWidget(d->formula, d->Obj, d->Id, "user_func"))
    return FALSE;

 s = gtk_entry_get_text(GTK_ENTRY(d->formula));
  entry_completion_append(NgraphApp.fit_list, s);

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, d->Id, p))
      return FALSE;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, d->Id, dd))
      return FALSE;

    s = gtk_entry_get_text(GTK_ENTRY(d->d[i]));
    entry_completion_append(NgraphApp.fit_list, s);
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
set_user_fit_sensitivity(struct FitDialog *d, int active)
{
  int i;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->d[i], active);
  }
}

static void
set_fitdialog_sensitivity(struct FitDialog *d, int type, int through)
{
  int i;

  set_user_fit_sensitivity(d, FALSE);
  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->p[i], FALSE);
  }

  set_widget_sensitivity_with_label(d->dim, type == 0);
  gtk_widget_set_sensitive(d->usr_def_frame, FALSE);
  gtk_widget_set_sensitive(d->usr_def_prm_tbl, FALSE);
  gtk_widget_set_sensitive(d->through_box, through);
  gtk_widget_set_sensitive(d->through_point, TRUE);
}

static void
FitDialogSetSensitivity(GtkWidget *widget, gpointer user_data)
{
  struct FitDialog *d;
  int type, through, deriv, dim;
  char buf[1024];

  d = (struct FitDialog *) user_data;

  type = combo_box_get_active(d->type);
  through = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->through_point));

  switch (type) {
  case FIT_TYPE_POLY:
    dim = combo_box_get_active(d->dim);

    if (dim == 0) {
      gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·</i>X<i>+b</i>");
    } else {
      snprintf(buf, sizeof(buf), "Equation: Y=<i>∑ a<sub>i</sub>·</i>X<sup><i>i</i></sup> (<i>i=0-%d</i>)", dim + 1);
      gtk_label_set_markup(GTK_LABEL(d->func_label), buf);
    }
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_POW:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·</i>X<i><sup>b</sup></i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_EXP:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>e</i><sup><i>(a·</i>X<i>+b)</i></sup>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_LOG:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·Ln(</i>X<i>)+b</i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_USER:
    gtk_label_set_text(GTK_LABEL(d->func_label), "");
    deriv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->derivatives));

    set_widget_sensitivity_with_label(d->dim, FALSE);
    gtk_widget_set_sensitive(d->through_point, FALSE);
    gtk_widget_set_sensitive(d->through_box, FALSE);
    gtk_widget_set_sensitive(d->usr_def_frame, TRUE);
    gtk_widget_set_sensitive(d->usr_def_prm_tbl, TRUE);
    set_user_fit_sensitivity(d, deriv);
    check_fit_func(NULL, d);
    break;
  }
}

static GtkWidget *
create_user_fit_frame(struct FitDialog *d)
{
  GtkWidget *table, *w, *vbox;
  int i, j;

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 3, FALSE);
#endif

  j = 0;
  w = create_text_entry(FALSE, TRUE);
  add_widget_to_table_sub(table, w, _("_Formula:"), TRUE, 0, 2, 3, j++);
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.fit_list);
  g_signal_connect(w, "changed", G_CALLBACK(check_fit_func), d);
  d->formula = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table_sub(table, w, _("_Converge (%):"), TRUE, 0, 1, 3, j);
  d->converge = w;

  w = gtk_check_button_new_with_mnemonic(_("_Derivatives"));
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, 3, j++);
  d->derivatives = w;

  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 4, FALSE);
#endif

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%0_0:", dd[] = "dF/d(%0_0):";

    p[sizeof(p) - 3] += i;
    dd[sizeof(dd) - 4] += i;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, p, TRUE, 0, 1, 4, j);
    d->p[i] = w;

    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.fit_list);
    add_widget_to_table_sub(table, w, dd, TRUE, 2, 1, 4, j++);
    d->d[i] = w;
  }

  w = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(w), table);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(w), GTK_SHADOW_NONE);
  gtk_widget_set_size_request(GTK_WIDGET(w), -1, 200);
  gtk_container_set_border_width(GTK_CONTAINER(w), 2);

  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 0);
  d->usr_def_prm_tbl = table;

  w = gtk_frame_new(_("User definition"));
  gtk_container_add(GTK_CONTAINER(w), vbox);

  return w;
}

static void
FitDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *hbox2, *vbox, *frame, *table;
  struct FitDialog *d;
  char title[20], **enumlist, mes[10];
  int i;

  d = (struct FitDialog *) data;
  snprintf(title, sizeof(title), _("Fit %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 5, FALSE);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);
#endif

    w = combo_box_create();
    add_widget_to_table_sub(table, w, _("_Type:"), FALSE, 0, 1, 5, 0);
    d->type = w;
    enumlist = (char **) chkobjarglist(d->Obj, "type");
    for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
      combo_box_append_text(d->type, _(enumlist[i]));
    }

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    add_widget_to_table_sub(table, w, _("_Dim:"), FALSE, 2, 1, 5, 0);
    d->dim = w;
    for (i = 0; i < FIT_PARM_NUM - 1; i++) {
      snprintf(mes, sizeof(mes), "%d", i + 1);
      combo_box_append_text(d->dim, mes);
    }

    w = gtk_label_new("");
#if ! GTK_CHECK_VERSION(3, 4, 0)
    gtk_misc_set_alignment(GTK_MISC(w), 0, 1);
#endif
    add_widget_to_table_sub(table, w, NULL, TRUE, 4, 1, 5, 0);
#if GTK_CHECK_VERSION(3, 4, 0)
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    gtk_widget_set_valign(w, GTK_ALIGN_END);
#endif
    d->func_label = w;


    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, _("_Weight:"), TRUE, 0, 4, 5, 1);
    d->weight = w;

    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 4);


#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

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

    gtk_box_pack_start(GTK_BOX(hbox), hbox2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    frame = gtk_frame_new(_("Action"));
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;

    w = create_spin_entry(1, 65535, 1, TRUE, TRUE);
    item_setup(hbox, w, _("_Div:"), FALSE);
    d->div = w;

    w = gtk_check_button_new_with_mnemonic(_("_Interpolation"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->interpolation = w;

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    frame = gtk_frame_new(_("Draw X range"));
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    frame = create_user_fit_frame(d);
    d->usr_def_frame = frame;
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 4);


    hbox = add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(FitDialogCopy), d, "fit");

    w = gtk_button_new_with_mnemonic(_("_Load"));
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogLoad), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogSave), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);


    w = gtk_button_new_with_mnemonic(_("_Draw"));
    gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogDraw), d);

    w = gtk_button_new_with_mnemonic(_("_Result"));
    gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogResult), d);


    g_signal_connect(d->dim, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->type, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->through_point, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->derivatives, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
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
move_tab_setup_item(struct FileDialog *d, int id)
{
  unsigned int j, movenum;
  int line;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  char buf[64];

  list_store_clear(d->move.list);

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
      line = arraynget_int(move, j);
      x = arraynget_double(movex, j);
      y = arraynget_double(movey, j);

      list_store_append(d->move.list, &iter);
      list_store_set_int(d->move.list, &iter, 0, line);

      snprintf(buf, sizeof(buf), "%+.15e", x);
      list_store_set_string(d->move.list, &iter, 1, buf);

      snprintf(buf, sizeof(buf), "%+.15e", y);
      list_store_set_string(d->move.list, &iter, 2, buf);
    }
  }
}

static void
FileMoveDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  double x, y;
  const char *buf;
  char *endptr, buf2[64];
  GtkTreeIter iter;

  d = (struct FileDialog *) client_data;

  a = spin_entry_get_val(d->move.line);

  buf = gtk_entry_get_text(GTK_ENTRY(d->move.x));
  if (buf[0] == '\0') return;

  x = strtod(buf, &endptr);
  if (x != x || x == HUGE_VAL || x == - HUGE_VAL || endptr[0] != '\0')
    return;

  buf = gtk_entry_get_text(GTK_ENTRY(d->move.y));
  if (buf[0] == '\0') return;

  y = strtod(buf, &endptr);
  if (y != y || y == HUGE_VAL || y == - HUGE_VAL || endptr[0] != '\0')
    return;

  list_store_append(d->move.list, &iter);
  list_store_set_int(d->move.list, &iter, 0, a);

  snprintf(buf2, sizeof(buf2), "%+.15e", x);
  list_store_set_string(d->move.list, &iter, 1, buf2);

  snprintf(buf2, sizeof(buf2), "%+.15e", y);
  list_store_set_string(d->move.list, &iter, 2, buf2);

  gtk_entry_set_text(GTK_ENTRY(d->move.x), "");
  gtk_entry_set_text(GTK_ENTRY(d->move.y), "");
  d->move.changed = TRUE;
}


static gboolean
move_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) user_data;
  if (e->keyval != GDK_KEY_Return)
    return FALSE;

  FileMoveDialogAdd(NULL, d);

  return TRUE;
}

static void
FileMoveDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  list_store_remove_selected_cb(w, d->move.list);
  d->move.changed = TRUE;
}

static void
move_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    move_tab_setup_item(d, sel);
    d->move.changed = TRUE;
  }
}

static GtkWidget *
move_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *hbox, *swin, *table, *vbox;
  n_list_store list[] = {
    {N_("Line No."), G_TYPE_INT,    TRUE, FALSE, NULL},
    {"X",            G_TYPE_STRING, TRUE, FALSE, NULL},
    {"Y",            G_TYPE_STRING, TRUE, FALSE, NULL},
  };
  int i;

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = list_store_create(sizeof(list) / sizeof(*list), list);
  list_store_set_sort_column(w, 0);
  list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
  d->move.list = w;
  gtk_container_add(GTK_CONTAINER(swin), w);

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
  g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
  add_widget_to_table(table, w, _("_Line:"), FALSE, i++);
  d->move.line = w;

  w = create_text_entry(TRUE, FALSE);
  g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
  add_widget_to_table(table, w, "_X:", FALSE, i++);
  d->move.x = w;

  w = create_text_entry(TRUE, FALSE);
  g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
  add_widget_to_table(table, w, "_Y:", FALSE, i++);
  d->move.y = w;

  w = gtk_button_new_from_stock(GTK_STOCK_ADD);
  add_widget_to_table(table, w, "", FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogAdd), d);

  w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogRemove), d);
  set_sensitivity_by_selection(d->move.list, w);

  w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->move.list);
  set_sensitivity_by_row_num(d->move.list, w);

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);

  w = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(w), hbox);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(move_tab_copy), d, "file");

  return vbox;
}

static int
move_tab_set_value(struct FileDialog *d)
{
  unsigned int j, movenum;
  int line, a;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  gboolean state;
  char *ptr, *endptr;

  if (d->move.changed == FALSE) {
    return 0;
  }

  set_graph_modified();
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

  state = list_store_get_iter_first(d->move.list, &iter);
  while (state) {
    a = list_store_get_int(d->move.list, &iter, 0);

    ptr = list_store_get_string(d->move.list, &iter, 1);
    x = strtod(ptr, &endptr);
    g_free(ptr);

    ptr = list_store_get_string(d->move.list, &iter, 2);
    y = strtod(ptr, &endptr);
    g_free(ptr);

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
      line = arraynget_int(move, j);

      if (line == a)
	break;
    }

    if (j == movenum) {
      arrayadd(move, &a);
      arrayadd(movex, &x);
      arrayadd(movey, &y);
    }

    state = list_store_iter_next(d->move.list, &iter);
  }

  putobj(d->Obj, "move_data", d->Id, move);
  putobj(d->Obj, "move_data_x", d->Id, movex);
  putobj(d->Obj, "move_data_y", d->Id, movey);

  return 0;
}

static void
mask_tab_setup_item(struct FileDialog *d, int id)
{
  int line, j, masknum;
  struct narray *mask;
  GtkTreeIter iter;

  list_store_clear(d->mask.list);
  getobj(d->Obj, "mask", id, 0, NULL, &mask);
  if ((masknum = arraynum(mask)) > 0) {
    for (j = 0; j < masknum; j++) {
      line = arraynget_int(mask, j);
      list_store_append(d->mask.list, &iter);
      list_store_set_int(d->mask.list, &iter, 0, line);
    }
  }
}

static void
FileMaskDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  GtkTreeIter iter;

  d = (struct FileDialog *) client_data;

  a = spin_entry_get_val(d->mask.line);
  list_store_append(d->mask.list, &iter);
  list_store_set_int(d->mask.list, &iter, 0, a);
  d->mask.changed = TRUE;
}

static gboolean
mask_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) user_data;
  if (e->keyval != GDK_KEY_Return)
    return FALSE;

  FileMaskDialogAdd(NULL, d);

  return TRUE;
}

static void
mask_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    mask_tab_setup_item(d, sel);
    d->mask.changed = TRUE;
  }
}

static void
FileMaskDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  list_store_remove_selected_cb(w, d->mask.list);
  d->mask.changed = TRUE;
}

static GtkWidget *
mask_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *swin, *hbox, *table, *vbox, *frame;
  n_list_store list[] = {
    {_("Line No."), G_TYPE_INT, TRUE, FALSE, NULL},
  };
  int i;

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
  g_signal_connect(w, "key-press-event", G_CALLBACK(mask_dialog_key_pressed), d);
  add_widget_to_table(table, w, _("_Line:"), FALSE, i++);
  d->mask.line = w;

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = list_store_create(sizeof(list) / sizeof(*list), list);
  list_store_set_sort_column(w, 0);
  list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
  d->mask.list = w;
  gtk_container_add(GTK_CONTAINER(swin), w);

  w = gtk_button_new_from_stock(GTK_STOCK_ADD);
  add_widget_to_table(table, w, "", FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogAdd), d);

  w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogRemove), d);
  set_sensitivity_by_selection(d->mask.list, w);

  w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->mask.list);
  set_sensitivity_by_row_num(d->mask.list, w);

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(mask_tab_copy), d, "file");

  return vbox;
}

static int
mask_tab_set_value(struct FileDialog *d)
{
  int a;
  struct narray *mask;
  GtkTreeIter iter;
  gboolean state;

  if (d->mask.changed == FALSE) {
    return 0;
  }

  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if (mask) {
    putobj(d->Obj, "mask", d->Id, NULL);
    mask = NULL;
  }

  state = list_store_get_iter_first(d->mask.list, &iter);
  while (state) {
    a = list_store_get_int(d->mask.list, &iter, 0);
    if (mask == NULL)
      mask = arraynew(sizeof(int));

    arrayadd(mask, &a);
    state = list_store_iter_next(d->mask.list, &iter);
  }
  putobj(d->Obj, "mask", d->Id, mask);
  set_graph_modified();

  return 0;
}

static void
load_tab_setup_item(struct FileDialog *d, int id)
{
  char *ifs, *s;
  unsigned int i, j, l;

  SetWidgetFromObjField(d->load.headskip, d->Obj, id, "head_skip");
  SetWidgetFromObjField(d->load.readstep, d->Obj, id, "read_step");
  SetWidgetFromObjField(d->load.finalline, d->Obj, id, "final_line");
  SetWidgetFromObjField(d->load.remark, d->Obj, id, "remark");
  sgetobjfield(d->Obj, id, "ifs", NULL, &ifs, FALSE, FALSE, FALSE);

  l = strlen(ifs);
  s = g_malloc(l * 2 + 1);
  j = 0;
  for (i = 0; i < l; i++) {
    if (ifs[i] == '\t') {
      s[j++] = '\\';
      s[j++] = 't';
    } else if (ifs[i] == '\\') {
      s[j++] = '\\';
      s[j++] = '\\';
    } else {
      s[j++] = ifs[i];
    }
  }
  s[j] = '\0';
  gtk_entry_set_text(GTK_ENTRY(d->load.ifs), s);
  g_free(s);
  g_free(ifs);
  SetWidgetFromObjField(d->load.csv, d->Obj, id, "csv");
}

static void
load_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    load_tab_setup_item(d, sel);
  }
}

static GtkWidget *
load_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *table, *frame, *vbox;
  int i;

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Head skip:"), FALSE, i++);
  d->load.headskip = w;

  w = create_spin_entry(1, INT_MAX, 1, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Read step:"), FALSE, i++);
  d->load.readstep = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_INT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Final line:"), FALSE, i++);
  d->load.finalline = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Remark:"), TRUE, i++);
  d->load.remark = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_IFS:"), TRUE, i++);
  d->load.ifs = w;

  w = gtk_check_button_new_with_mnemonic(_("_CSV"));
  add_widget_to_table(table, w, NULL, TRUE, i++);
  d->load.csv = w;

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), table);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(load_tab_copy), d, "file");

  return vbox;
}

static int
load_tab_set_value(struct FileDialog *d)
{
  const char *ifs;
  char *obuf;
  unsigned int i, l;
  GString *s;

  if (SetObjFieldFromWidget(d->load.headskip, d->Obj, d->Id, "head_skip"))
    return 1;

  if (SetObjFieldFromWidget(d->load.readstep, d->Obj, d->Id, "read_step"))
    return 1;

  if (SetObjFieldFromWidget(d->load.finalline, d->Obj, d->Id, "final_line"))
    return 1;

  if (SetObjFieldFromWidget(d->load.remark, d->Obj, d->Id, "remark"))
    return 1;

  ifs = gtk_entry_get_text(GTK_ENTRY(d->load.ifs));
  s = g_string_new("");

  l = strlen(ifs);
  for (i = 0; i < l; i++) {
    if ((ifs[i] == '\\') && (ifs[i + 1] == 't')) {
      g_string_append_c(s, 0x09);
      i++;
    } else if (ifs[i] == '\\') {
      g_string_append_c(s, '\\');
      i++;
    } else {
      g_string_append_c(s, ifs[i]);
    }
  }

  sgetobjfield(d->Obj, d->Id, "ifs", NULL, &obuf, FALSE, FALSE, FALSE);
  if (obuf == NULL || strcmp(s->str, obuf)) {
    if (sputobjfield(d->Obj, d->Id, "ifs", s->str) != 0) {
      g_free(obuf);
      g_string_free(s, TRUE);
      return 1;
    }
    set_graph_modified();
  }

  g_free(obuf);
  g_string_free(s, TRUE);

  if (SetObjFieldFromWidget(d->load.csv, d->Obj, d->Id, "csv"))
    return 1;

  return 0;
}

static void
math_tab_setup_item(struct FileDialog *d, int id)
{
  SetWidgetFromObjField(d->math.xsmooth, d->Obj, id, "smooth_x");
  SetWidgetFromObjField(d->math.ysmooth, d->Obj, id, "smooth_y");
  SetWidgetFromObjField(d->math.x, d->Obj, id, "math_x");
  SetWidgetFromObjField(d->math.y, d->Obj, id, "math_y");
  SetWidgetFromObjField(d->math.f, d->Obj, id, "func_f");
  SetWidgetFromObjField(d->math.g, d->Obj, id, "func_g");
  SetWidgetFromObjField(d->math.h, d->Obj, id, "func_h");

  entry_completion_set_entry(NgraphApp.x_math_list, d->math.x);
  entry_completion_set_entry(NgraphApp.y_math_list, d->math.y);
}

static void
math_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    math_tab_setup_item(d, sel);
  }
}

static gboolean
func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data)
{
  GtkEntryCompletion *compl;

  compl = GTK_ENTRY_COMPLETION(user_data);
  entry_completion_set_entry(compl, w);

  return FALSE;
}

static GtkWidget *
math_tab_create(struct FileDialog *d)
{
  GtkWidget *table, *w, *vbox, *frame;
  int i;

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, TRUE, TRUE);
  add_widget_to_table(table, w, _("_X smooth:"), FALSE, i++);
  d->math.xsmooth = w;

  w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Y smooth:"), FALSE, i++);
  d->math.ysmooth = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_X math:"), TRUE, i++);
  d->math.x = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Y math:"), TRUE, i++);
  d->math.y = w;

  w = create_text_entry(TRUE, TRUE);
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
  add_widget_to_table(table, w, "_F(X,Y,Z):", TRUE, i++);
  d->math.f = w;

  w = create_text_entry(TRUE, TRUE);
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
  add_widget_to_table(table, w, "_G(X,Y,Z):", TRUE, i++);
  d->math.g = w;

  w = create_text_entry(TRUE, TRUE);
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
  add_widget_to_table(table, w, "_H(X,Y,Z):", TRUE, i++);
  d->math.h = w;

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), table);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(math_tab_copy), d, "file");

  return vbox;
}

static int
math_tab_set_value(void *data)
{
  struct FileDialog *d;
  const char *s;

  d = (struct FileDialog *) data;

  s = gtk_entry_get_text(GTK_ENTRY(d->math.y));
  entry_completion_append(NgraphApp.y_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->math.x));
  entry_completion_append(NgraphApp.x_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->math.f));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->math.g));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->math.h));
  entry_completion_append(NgraphApp.func_list, s);

  if (SetObjFieldFromWidget(d->math.xsmooth, d->Obj, d->Id, "smooth_x"))
    return 1;

  if (SetObjFieldFromWidget(d->math.x, d->Obj, d->Id, "math_x"))
    return 1;

  if (SetObjFieldFromWidget(d->math.ysmooth, d->Obj, d->Id, "smooth_y"))
    return 1;

  if (SetObjFieldFromWidget(d->math.y, d->Obj, d->Id, "math_y"))
    return 1;

  if (SetObjFieldFromWidget(d->math.f, d->Obj, d->Id, "func_f"))
    return 1;

  if (SetObjFieldFromWidget(d->math.g, d->Obj, d->Id, "func_g"))
    return 1;

  if (SetObjFieldFromWidget(d->math.h, d->Obj, d->Id, "func_h"))
    return 1;

  return 0;
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

void
button_set_mark_image(GtkWidget *w, int type)
{
  GtkWidget *img;
  char buf[64];

  if (type < 0 || type >= MARK_TYPE_NUM) {
    type = 0;
  }

  if (NgraphApp.markpix[type]) {
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(NgraphApp.markpix[type],
					 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE);
    img = gtk_image_new_from_pixbuf(pixbuf);
#else
    img = gtk_image_new_from_pixmap(NgraphApp.markpix[type], NULL);
#endif
    if (img) {
      gtk_button_set_image(GTK_BUTTON(w), img);
    }
    snprintf(buf, sizeof(buf), "%02d", type);
    gtk_widget_set_tooltip_text(w, buf);
  }
 }

static void
MarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkWidget *w, *grid;
#else
  GtkWidget *w, *hbox,*vbox;
#endif
  struct MarkDialog *d;
  int type;
#define COL 10

  d = (struct MarkDialog *) data;

  if (makewidget) {
#if GTK_CHECK_VERSION(3, 0, 0)
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_right(grid, 4);
    gtk_widget_set_margin_left(grid, 4);
    for (type = 0; type < MARK_TYPE_NUM; type++) {
      w = gtk_toggle_button_new();
      button_set_mark_image(w, type);
      g_signal_connect(w, "clicked", G_CALLBACK(MarkDialogCB), d);
      d->toggle[type] = w;
      gtk_grid_attach(GTK_GRID(grid), w, type % COL, type / COL, 1, 1);
    }
    gtk_box_pack_start(GTK_BOX(d->vbox), grid, FALSE, FALSE, 4);
#else
    hbox = NULL;
    vbox = gtk_vbox_new(FALSE, 4);

    for (type = 0; type < MARK_TYPE_NUM; type++) {
      w = gtk_toggle_button_new();
      button_set_mark_image(w, type);
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
#endif
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
  if (type < 0 || type >= MARK_TYPE_NUM) {
    type = 0;
  }
  data->SetupWindow = MarkDialogSetup;
  data->CloseWindow = MarkDialogClose;
  data->Type = type;
}

static void
file_setup_item(struct FileDialog *d, int id)
{
  int i, j, lastinst;
  struct objlist *aobj;
  char *name, *valstr;

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
  combo_box_entry_set_text(d->xaxis, valstr + i);
  g_free(valstr);

  SetWidgetFromObjField(d->ycol, d->Obj, id, "y");

  sgetobjfield(d->Obj, id, "axis_y", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->yaxis, valstr + i);
  g_free(valstr);
}

static void
set_fit_button_label(GtkWidget *btn, const char *str)
{
  char buf[128];

  if (str && str[0] != '\0') {
    snprintf(buf, sizeof(buf), "Fit:%s", str);
  } else {
    snprintf(buf, sizeof(buf), _("Create"));
  }
  gtk_button_set_label(GTK_BUTTON(btn), buf);
}

static void
plot_tab_setup_item(struct FileDialog *d, int id)
{
  int a;

  SetWidgetFromObjField(d->type, d->Obj, id, "type");

  SetWidgetFromObjField(d->curve, d->Obj, id, "interpolation");

  getobj(d->Obj, "mark_type", id, 0, NULL, &a);
  button_set_mark_image(d->mark_btn, a);
  MarkDialog(&(d->mark), a);

  SetWidgetFromObjField(d->size, d->Obj, id, "mark_size");

  SetWidgetFromObjField(d->width, d->Obj, id, "line_width");

  SetStyleFromObjField(d->style, d->Obj, id, "line_style");

  SetWidgetFromObjField(d->join, d->Obj, id, "line_join");

  SetWidgetFromObjField(d->miter, d->Obj, id, "line_miter_limit");

  SetWidgetFromObjField(d->clip, d->Obj, id, "data_clip");

  set_color(d->col1, d->Obj, id, NULL);
  set_color2(d->col2, d->Obj, id);

  FileDialogType(d->type, d);
}

static void
FileDialogSetupItem(GtkWidget *w, struct FileDialog *d, int file, int id)
{
  char *valstr;
  int i;

  plot_tab_setup_item(d, d->Id);
  math_tab_setup_item(d, d->Id);
  load_tab_setup_item(d, d->Id);
  mask_tab_setup_item(d, d->Id);
  move_tab_setup_item(d, d->Id);
  file_setup_item(d, d->Id);

  if (file) {
    SetWidgetFromObjField(d->file, d->Obj, id, "file");
    gtk_editable_set_position(GTK_EDITABLE(d->file), -1);
  }

  if (id == d->Id) {
    sgetobjfield(d->Obj, id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
    for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
    if (valstr[i] == ':') {
      i++;
    }
    set_fit_button_label(d->fit, valstr + i);
    g_free(valstr);
  } else {
    set_fit_button_label(d->fit, NULL);
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

  d = (struct FileDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  button_set_mark_image(w, d->mark.Type);
}

static int
execute_fit_dialog(GtkWidget *w, struct objlist *fileobj, int fileid, struct objlist *fitobj, int fitid)
{
  int save_type, type, ret;

  type = PLOT_TYPE_FIT;
  getobj(fileobj, "type", fileid, 0, NULL, &save_type);
  putobj(fileobj, "type", fileid, &type);

  FitDialog(&DlgFit, fitobj, fitid);
  ret = DialogExecute(w, &DlgFit);

  putobj(fileobj, "type", fileid, &save_type);

  return ret;
}

static void
FileDialogFit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  struct objlist *fitobj, *obj;
  char *fit;
  N_VALUE *inst;
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
      fitid = arraylast_int(&iarray);
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
      g_free(fit);
      return;
    }
    create = TRUE;
  }

  ret = execute_fit_dialog(d->widget, d->Obj, d->Id, fitobj, fitid);

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
  set_fit_button_label(d->fit, valstr + i);
  g_free(valstr);
}

static void
plot_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    plot_tab_setup_item(d, sel);
  }
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
FileDialogOption(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  exeobj(d->Obj, "load_settings", d->Id, 0, NULL);
  FileDialogSetupItem(d->widget, d, FALSE, d->Id);
}

static void
edit_file(const char *file)
{
  char *cmd, *localize_name;

  if (file == NULL)
    return;

  localize_name = get_localized_filename(file);
  if (localize_name == NULL)
    return;

  cmd = g_strdup_printf("\"%s\" \"%s\"", Menulocal.editor, localize_name);
  g_free(localize_name);

  system_bg(cmd);

  g_free(cmd);
}

static void
FileDialogEdit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  const char *file;

  d = (struct FileDialog *) client_data;

  if (Menulocal.editor == NULL)
    return;

  file = gtk_entry_get_text(GTK_ENTRY(d->file));
  if (file == NULL)
    return;

  edit_file(file);
}

static void
FileDialogType(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int type;

  d = (struct FileDialog *) client_data;

  type = combo_box_get_active(w);

  set_widget_sensitivity_with_label(d->mark_btn, TRUE);
  set_widget_sensitivity_with_label(d->curve, TRUE);
  set_widget_sensitivity_with_label(d->col2, TRUE);
  set_widget_sensitivity_with_label(d->size, TRUE);
  set_widget_sensitivity_with_label(d->miter, TRUE);
  set_widget_sensitivity_with_label(d->join, TRUE);
  set_widget_sensitivity_with_label(d->fit, TRUE);

  switch (type) {
  case PLOT_TYPE_MARK:
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_CURVE:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_POLYGON_SOLID_FILL:
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_ARROW:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_RECTANGLE_FILL:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_FIT:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    break;
  }
}

static void
file_settings_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    file_setup_item(d, sel);
  }
}

static GtkWidget *
plot_tab_create(GtkWidget *parent, struct FileDialog *d)
{
  GtkWidget *table, *hbox, *w, *vbox;
  int i;

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Type:"), FALSE, i++);
  d->type = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogType), d);

  w = gtk_button_new();
  add_widget_to_table(table, w, _("_Mark:"), FALSE, i++);
  d->mark_btn = w;
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMark), d);

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Curve:"), FALSE, i++);
  d->curve = w;

  d->fit_table = table;
  d->fit_row = i;

  i++;
  w = create_color_button(parent);
  add_widget_to_table(table, w, _("_Color 1:"), FALSE, i++);
  d->col1 = w;

  w = create_color_button(parent);
  add_widget_to_table(table, w, _("_Color 2:"), FALSE, i++);
  d->col2 = w;

  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);


#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif

  i = 0;
  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  add_widget_to_table(table, w, _("Line _Style:"), TRUE, i++);
  d->style = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Line Width:"), FALSE, i++);
  d->width = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Size:"), FALSE, i++);
  d->size = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Miter:"), FALSE, i++);
  d->miter = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Join:"), FALSE, i++);
  d->join = w;

  w = gtk_check_button_new_with_mnemonic(_("_Clip"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->clip = w;

  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);

  w = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(w), hbox);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);

  add_copy_button_to_box(vbox, G_CALLBACK(plot_tab_copy), d, "file");

  return vbox;
}

static void
FileDialogSetupCommon(GtkWidget *wi, struct FileDialog *d)
{
  GtkWidget *w, *hbox, *vbox2, *frame, *notebook, *label;


#if GTK_CHECK_VERSION(3, 0, 0)
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  vbox2 = gtk_vbox_new(FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
#endif

  w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, TRUE, TRUE);
  item_setup(hbox, w, _("_X column:"), TRUE);
  d->xcol = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH, -1);
  item_setup(hbox, w, _("_X axis:"), TRUE);
  d->xaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);


#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif

  w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, TRUE, TRUE);
  item_setup(hbox, w, _("_Y column:"), TRUE);
  d->ycol = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH, -1);
  item_setup(hbox, w, _("_Y axis:"), TRUE);
  d->yaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);

  add_copy_button_to_box(vbox2, G_CALLBACK(file_settings_copy), d, "file");

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif
  d->comment_box = hbox;
  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);


  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

  notebook = gtk_notebook_new();

  d->tab = GTK_NOTEBOOK(notebook);
  gtk_notebook_set_scrollable(d->tab, FALSE);
  gtk_notebook_set_tab_pos(d->tab, GTK_POS_TOP);


  w = plot_tab_create(wi, d);
  label = gtk_label_new_with_mnemonic(_("_Plot"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

  w = math_tab_create(d);
  label = gtk_label_new_with_mnemonic(_("_Math"));
  d->math.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

  w = load_tab_create(d);
  label = gtk_label_new_with_mnemonic(_("_Load"));
  d->load.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);


  gtk_box_pack_start(GTK_BOX(d->vbox), notebook, TRUE, TRUE, 4);
}

static void
set_headlines(struct FileDialog *d, char *s)
{
  gboolean valid;
  const gchar *ptr;

  if (s == NULL) {
    return;
  }

  if (Menulocal.file_preview_font) {
    text_view_with_line_number_set_font(d->comment_view, Menulocal.file_preview_font);
  }
  valid = g_utf8_validate(s, -1, &ptr);

  if (valid) {
    text_view_with_line_number_set_text(d->comment_view, s);
  } else {
    char *ptr;

    ptr = g_locale_to_utf8(s, -1, NULL, NULL, NULL);
    if (ptr) {
      text_view_with_line_number_set_text(d->comment_view, ptr);
      g_free(ptr);
    } else {
      text_view_with_line_number_set_text(d->comment_view, _("This file contain invalid UTF-8 strings."));
    }
  }
}

static void
FileDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *view, *label;
  struct FileDialog *d;
  int line;
  char title[20], *argv[2], *s;

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    d->apply_all = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Apply all"), IDFAPPLY);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

    w = create_file_entry(d->Obj);
    item_setup(GTK_WIDGET(hbox), w, _("_File:"), TRUE);
    d->file = w;

    w = gtk_button_new_with_mnemonic(_("_Load settings"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->load_settings = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogOption), d);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogEdit), d);


    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);


    FileDialogSetupCommon(wi, d);

    w = mask_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Mask"));
    d->mask.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = move_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Move"));
    d->move.tab_id = gtk_notebook_append_page(d->tab, w, label);

    view = create_text_view_with_line_number(&d->comment_view);
    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), view);
    gtk_box_pack_start(GTK_BOX(d->comment_box), w, TRUE, TRUE, 0);

    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
  }

  line = Menulocal.data_head_lines;
  argv[0] = (char *) &line;
  argv[1] = NULL;
  getobj(d->Obj, "head_lines", d->Id, 1, argv, &s);
  set_headlines(d, s);
  FileDialogSetupItem(wi, d, TRUE, d->Id);
}

static int
plot_tab_set_value(struct FileDialog *d)
{
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

  return 0;
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

  if (plot_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, 0);
    return TRUE;
  }

  if (math_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->math.tab_id);
    return TRUE;
  }

  if (load_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->load.tab_id);
    return TRUE;
  }

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
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromWidget(d->file, d->Obj, d->Id, "file"))
    return;

  if (FileDialogCloseCommon(w, d))
    return;

  if (mask_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->mask.tab_id);
    return;
  }

  if (move_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->move.tab_id);
    return;
  }

  d->ret = ret;
}

void
FileDialog(struct obj_list_data *data, int id, int multi)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data->dialog;

  d->SetupWindow = FileDialogSetup;
  d->CloseWindow = FileDialogClose;
  d->Obj = data->obj;
  d->Id = id;
  d->multi_open = multi > 0;
}

static void
FileDialogDefSetupItem(GtkWidget *w, struct FileDialog *d, int id)
{
  plot_tab_setup_item(d, d->Id);
  math_tab_setup_item(d, d->Id);
  load_tab_setup_item(d, d->Id);
  file_setup_item(d, d->Id);
}

static void
FileDefDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  if (makewidget) {
    FileDialogSetupCommon(wi, d);
    gtk_notebook_set_tab_pos(d->tab, GTK_POS_TOP);
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

static void
delete_file_obj(struct obj_list_data *data, int id)
{
  FitDel(data->obj, id);
  delobj(data->obj, id);
}

void
CmFileHistory(GtkRecentChooser *w, gpointer client_data)
{
  int ret;
  char *name, *fname;
  int id;
  struct objlist *obj;
  char *uri;

  if (Menulock || Globallock) {
    return;
  }

  uri = gtk_recent_chooser_get_current_uri(w);
  if (uri == NULL) {
    return;
  }

  name = g_filename_from_uri(uri, NULL, NULL);
  g_free(uri);
  if (name == NULL) {
    return;
  }

  obj = chkobject("file");
  if (obj == NULL) {
    return;
  }

  id = newobj(obj);
  if (id < 0) {
    return;
  }

  fname = g_strdup(name);
  if (fname == NULL) {
    return;
  }

  putobj(obj, "file", id, name);
  FileDialog(NgraphApp.FileWin.data.data, id, FALSE);
  ret = DialogExecute(TopLevel, &DlgFile);
  if ((ret == IDDELETE) || (ret == IDCANCEL)) {
    delete_file_obj(NgraphApp.FileWin.data.data, id);
  } else {
    set_graph_modified();
    AddDataFileList(fname);
  }
  g_free(fname);
  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
}

void
CmFileNew(GtkAction *w, gpointer client_data)
{
  char *file;
  int id, ret;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  if (nGetOpenFileName(TopLevel, _("Data new"), NULL, NULL,
		       NULL, &file, FALSE,
		       Menulocal.changedirectory) != IDOK) {
    return;
  }

  id = newobj(obj);
  if (id < 0) {
    g_free(file);
    return;
  }

  changefilename(file);
  putobj(obj, "file", id, file);
  FileDialog(NgraphApp.FileWin.data.data, id, FALSE);
  ret = DialogExecute(TopLevel, &DlgFile);

  if (ret == IDDELETE || ret == IDCANCEL) {
    delete_file_obj(NgraphApp.FileWin.data.data, id);
  } else {
    set_graph_modified();
    AddDataFileList(file);
  }

  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
}


void
CmFileOpen(GtkAction *w, gpointer client_data)
{
  int id, ret, n;
  char *name;
  char **file = NULL, **ptr;
  struct objlist *obj;
  struct narray farray;

  if (Menulock || Globallock)
    return;

  obj = chkobject("file");
  if (obj == NULL)
    return;

  ret = nGetOpenFileNameMulti(TopLevel, _("Add Data file"), NULL,
			      &(Menulocal.fileopendir), NULL,
			      &file, Menulocal.changedirectory);

  n = chkobjlastinst(obj);

  arrayinit(&farray, sizeof(int));
  if (ret == IDOK && file) {
    for (ptr = file; *ptr; ptr++) {
      name = *ptr;
      id = newobj(obj);
      if (id >= 0) {
	arrayadd(&farray, &id);
	changefilename(name);
	putobj(obj, "file", id, name);
      }
    }
    g_free(file);
  }

  if (update_file_obj_multi(obj, &farray, TRUE)) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }

  if (n != chkobjlastinst(obj)) {
    set_graph_modified();
  }

  arraydel(&farray);
}

void
CmFileClose(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delete_file_obj(NgraphApp.FileWin.data.data, array[i]);
      set_graph_modified();
    }
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
  arraydel(&farray);
}

int
update_file_obj_multi(struct objlist *obj, struct narray *farray, int new_file)
{
  int i, j, num, *array, id0;
  char *name;

  num = arraynum(farray);
  if (num < 1) {
    return 0;
  }

  array = arraydata(farray);
  id0 = -1;

  for (i = 0; i < num; i++) {
    name = NULL;
    if (id0 != -1) {
      copy_file_obj_field(obj, array[i], array[id0], FALSE);
      if (new_file) {
	getobj(obj, "file", array[i], 0, NULL, &name);
	AddDataFileList(name);
      }
    } else {
      int ret;

      FileDialog(NgraphApp.FileWin.data.data, array[i], i < num - 1);
      ret = DialogExecute(TopLevel, &DlgFile);
      if (ret == IDCANCEL && new_file) {
	ret = IDDELETE;
      }
      if (ret == IDDELETE) {
	delete_file_obj(NgraphApp.FileWin.data.data, array[i]);
	if (! new_file) {
	  set_graph_modified();
	}
	for (j = i + 1; j < num; j++) {
	  array[j]--;
	}
      } else {
	if (new_file) {
	  getobj(obj, "file", array[i], 0, NULL, &name);
	  AddDataFileList(name);
	}

	if (ret == IDFAPPLY) {
	  id0 = i;
	}
      }
    }
  }

  return 1;
}

void
CmFileUpdate(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int ret;
  struct narray farray;
  int last;

  if (Menulock || Globallock)
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
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  }
  arraydel(&farray);
}

void
CmFileEdit(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int i;
  char *name;
  int last;

  if (Menulock || Globallock)
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

  edit_file(name);
}

void
CmOptionFileDef(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  int id;

  if (Menulock || Globallock)
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
FileWinFileEdit(struct obj_list_data *d)
{
  int sel, num;
  char *name;

  if (Menulock || Globallock)
    return;

  if (Menulocal.editor == NULL)
    return;

  sel = d->select;
  num = chkobjlastinst(d->obj);

  if (sel < 0 || sel > num)
    return;

  if (getobj(d->obj, "file", sel, 0, NULL, &name) == -1)
    return;

  edit_file(name);
}

static void
file_edit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileEdit(d);
}

static void
FileWinFileDelete(struct obj_list_data *d)
{
  int sel, update, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    delete_file_obj(d, sel);
    num = chkobjlastinst(d->obj);
    update = FALSE;
    if (num < 0) {
      d->select = -1;
      update = TRUE;
    } else if (sel > num) {
      d->select = num;
    } else {
      d->select = sel;
    }
    FileWinUpdate(d, update);
    set_graph_modified();
  }
}

static void
file_delete_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data*) client_data;
  FileWinFileDelete(d);
}

static int
file_obj_copy(struct obj_list_data *d)
{
  int sel, id, num;

  if (Menulock || Globallock)
    return -1;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  num = chkobjlastinst(d->obj);

  if ((sel < 0) || (sel > num))
    return -1;

  id = newobj(d->obj);

  if (id < 0)
    return -1;

  copy_file_obj_field(d->obj, id, sel, TRUE);

  return id;
}

static void
FileWinFileCopy(struct obj_list_data *d)
{
  d->select = file_obj_copy(d);
  FileWinUpdate(d, FALSE);
}

static void
file_copy_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileCopy(d);
}

static void
FileWinFileCopy2(struct obj_list_data *d)
{
  int id, sel, j, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  id = file_obj_copy(d);
  num = chkobjlastinst(d->obj);

  if (id < 0) {
    d->select = sel;
    FileWinUpdate(d, TRUE);
    return;
  }

  for (j = num; j > sel + 1; j--) {
    moveupobj(d->obj, j);
  }

  d->select = sel + 1;
  FileWinUpdate(d, FALSE);
}

static void
file_copy2_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileCopy2(d);
}

static void
FileWinFileUpdate(struct obj_list_data *d)
{
  int sel, ret, num;
  GtkWidget *parent;

  if (Menulock || Globallock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    d->setup_dialog(d, sel, FALSE);
    d->select = sel;

    parent = (Menulocal.single_window_mode) ? TopLevel : d->parent->Win;
    ret = DialogExecute(parent, d->dialog);
    if (ret == IDDELETE) {
      delete_file_obj(d, sel);
      d->select = -1;
      set_graph_modified();
    }
    d->update(d, FALSE);
  }
}

static void
FileWinFileDraw(struct obj_list_data *d)
{
  int i, sel, hidden, h, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_index(GTK_WIDGET(d->text));
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    for (i = 0; i <= num; i++) {
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
    for (i = 0; i <= num; i++) {
      getobj(d->obj, "hidden", i, 0, NULL, &h);
      putobj(d->obj, "hidden", i, &hidden);
      if (h != hidden) {
	set_graph_modified();
      }
    }
    d->select = -1;
  }
  CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
  FileWinUpdate(d, FALSE);
}

static void
file_draw_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileDraw(d);
}

void
FileWinUpdate(struct obj_list_data *d, int clear)
{
  if (Menulock || Globallock)
    return;

  if (d == NULL)
    return;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, file_list_set_val);
  } else {
    list_sub_window_set(d, file_list_set_val);
  }

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID, d->select);
  }
}

static void
FileWinFit(struct obj_list_data *d)
{
  struct objlist *fitobj, *obj2;
  char *fit;
  int sel, idnum, fitid = 0, ret, num;
  struct narray iarray;
  GtkWidget *parent;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  num = chkobjlastinst(d->obj);

  if (sel < 0 || sel > num)
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
      fitid = arraylast_int(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL)
    return;

  parent = (Menulocal.single_window_mode) ? TopLevel : d->parent->Win;
  ret = execute_fit_dialog(parent, d->obj, sel, fitobj, fitid);

  if (ret == IDDELETE) {
    delobj(fitobj, fitid);
    putobj(d->obj, "fit", sel, NULL);
  }
}

static void
file_fit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFit(d);
}


static GdkPixbuf *
draw_type_pixbuf(struct objlist *obj, int i)
{
  int j, k, ggc, fr, fg, fb, fr2, fg2, fb2,
    type, width = 40, height = 20, poly[14], marktype,
    intp, spcond, spnum, lockstate, found, output;
  double spx[7], spy[7], spz[7], spc[6][7], spc2[6];
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *pix;
#else
  GdkPixmap *pix;
#endif
  GdkPixbuf *pixbuf;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;

  lockstate = Globallock;
  Globallock = TRUE;

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return NULL;
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#else
  pix = gra2gdk_create_pixmap(local, gtk_widget_get_window(TopLevel),
			      width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#endif
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
		 robj, inst, output, -1, -1, -1, NULL, local);
  if (ggc < 0) {
    _GRAclose(ggc);
    g_object_unref(G_OBJECT(pix));
    return NULL;
  }
  GRAview(ggc, 0, 0, width, height, 0);
  GRAcolor(ggc, fr, fg, fb, 255);
  GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);

  switch (type) {
  case PLOT_TYPE_MARK:
    getobj(obj, "mark_type", i, 0, NULL, &marktype);
    GRAmark(ggc, marktype, height / 2, height / 2, height - 2,
	    fr, fg, fb, 255, fr2, fg2, fb2, 255);
    break;
  case PLOT_TYPE_LINE:
    GRAline(ggc, 1, height / 2, height - 1, height / 2);
    break;
  case PLOT_TYPE_POLYGON:
  case PLOT_TYPE_POLYGON_SOLID_FILL:
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
    GRAdrawpoly(ggc, 7, poly, (type == PLOT_TYPE_POLYGON) ? 0: 2);
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
    if (intp >= 2) {
      GRAmoveto(ggc, height, height * 3 / 4);
      GRAtextstyle(ggc, "Serif", GRA_FONT_STYLE_NORMAL, 52, 0, 0);
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
    GRAline(ggc, 1, height - 1, height - 1, 1);
    break;
  case PLOT_TYPE_ARROW:
    spx[0] = 1;
    spy[0] = height - 1;

    spx[1] = height - 1;
    spy[1] = 1;

    GRAline(ggc, 1, height - 1, height - 1, 1);
    poly[0] = height - 6;
    poly[1] = 1;
	
    poly[2] = height - 1;
    poly[3] = 1;

    poly[4] = height - 1;
    poly[5] = 6;
    GRAdrawpoly(ggc, 3, poly, 1);
    break;
  case PLOT_TYPE_RECTANGLE:
    GRArectangle(ggc, 1, height - 1, height - 1, 1, 0);
    break;
  case PLOT_TYPE_RECTANGLE_FILL:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, 1, height - 1, height - 1, 1, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, 1, height - 1, height - 1, 1, 0);
    break;
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    GRArectangle(ggc, 1, height - 1, height - 1, 1, 1);
    break;
  case PLOT_TYPE_ERRORBAR_X:
    GRAline(ggc, 1, height / 2, height - 1, height / 2);
    GRAline(ggc, 1, height / 4, 1, height * 3 / 4);
    GRAline(ggc, height - 1, height / 4, height - 1, height * 3 / 4);
    break;
  case PLOT_TYPE_ERRORBAR_Y:
    GRAline(ggc, height / 2, 1, height / 2, height - 1);
    GRAline(ggc, height / 4, 1, height * 3 / 4, 1);
    GRAline(ggc, height / 4, height -1, height * 3 / 4, height - 1);
    break;
  case PLOT_TYPE_STAIRCASE_X:
    GRAmoveto(ggc, 1, height - 1);
    GRAlineto(ggc, height / 4, height - 1);
    GRAlineto(ggc, height / 4, height / 2);
    GRAlineto(ggc, height * 3 / 4, height / 2);
    GRAlineto(ggc, height * 3 / 4, 1);
    GRAlineto(ggc, height - 1, 1);
    break;
  case PLOT_TYPE_STAIRCASE_Y:
    GRAmoveto(ggc, 1, height - 1);
    GRAlineto(ggc, 1, height / 2 + 1);
    GRAlineto(ggc, height / 2, height / 2 + 1);
    GRAlineto(ggc, height / 2, height / 4);
    GRAlineto(ggc, height - 1, height / 4);
    GRAlineto(ggc, height - 1, 1);
    break;
  case PLOT_TYPE_BAR_X:
    GRArectangle(ggc, 1, height / 4, height - 1, height * 3 / 4, 0);
    break;
  case PLOT_TYPE_BAR_Y:
    GRArectangle(ggc, height / 4, 1, height * 3/ 4, height - 1, 0);
    break;
  case PLOT_TYPE_BAR_FILL_X:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, 1, height / 4, height - 1, height * 3 / 4, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, 1, height / 4, height - 1, height * 3 / 4, 0);
    break;
  case PLOT_TYPE_BAR_FILL_Y:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, height / 4, 1, height * 3/ 4, height - 1, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, height / 4, 1, height * 3/ 4, height - 1, 0);
    break;
  case PLOT_TYPE_BAR_SOLID_FILL_X:
    GRArectangle(ggc, 1, height / 4, height - 1, height * 3 / 4, 1);
    break;
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    GRArectangle(ggc, height / 3, 1, height * 3 / 4, height - 1, 1);
    break;
  case PLOT_TYPE_FIT:
    GRAmoveto(ggc, 1, height * 3 / 4);
    GRAtextstyle(ggc, "Serif", GRA_FONT_STYLE_NORMAL, 52, 0, 0);
    GRAouttext(ggc, "fit");
    break;
  }

  _GRAclose(ggc);
  if (local->linetonum && local->cairo) {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);
#else
  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, width, height);
  g_object_unref(G_OBJECT(pix));
#endif

  Globallock = lockstate;

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
  tmp = g_strdup(valstr + j);
  g_free(valstr);

  return tmp;
}

static void
file_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, style;
  unsigned int i;
  char buf[256];
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
	style = PANGO_STYLE_ITALIC;
      } else {
	style = PANGO_STYLE_NORMAL;
      }
      bfile = getbasename(file);
      if (bfile) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, CHK_STR(bfile));
	g_free(bfile);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
      }
      list_store_set_int(GTK_WIDGET(d->text), iter, FILE_WIN_COL_MASKED, style);
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
	snprintf(buf, sizeof(buf), "%3s", axis);
	list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
	g_free(axis);
      }
      break;
    case FILE_WIN_COL_HIDDEN:
      getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
      break;
    case FILE_WIN_COL_MASKED:
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

void
CmFileMath(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;

  if (Menulock || Globallock)
    return;

  obj = chkobject("file");

  if (chkobjlastinst(obj) < 0)
    return;

  MathDialog(&DlgMath, obj);
  DialogExecute(TopLevel, &DlgMath);
}

static int
GetDrawFiles(struct narray *farray)
{
  struct objlist *fobj;
  int lastinst;
  struct narray ifarray;
  int i, a;

  if (farray == NULL)
    return 1;

  fobj = chkobject("file");
  if (fobj == NULL)
    return 1;

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0)
    return 1;

  arrayinit(&ifarray, sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      arrayadd(&ifarray, &i);
  }
  SelectDialog(&DlgSelect, fobj, FileCB, farray, &ifarray);
  if (DialogExecute(TopLevel, &DlgSelect) != IDOK) {
    arraydel(&ifarray);
    arraydel(farray);
    return 1;
  }
  arraydel(&ifarray);

  return 0;
}

void
CmFileSaveData(GtkAction *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i, num, onum, type, div, curve = FALSE, *array, append;
  char *file, buf[1024];
  char *argv[4];

  if (Menulock || Globallock)
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

  array = arraydata(&farray);
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
		       &file, FALSE, Menulocal.changedirectory) != IDOK) {
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

    snprintf(buf, sizeof(buf), "%d/%d", i, num);
    set_progress(1, buf, 1.0 * (i + 1) / num);

    append = (i == 0) ? FALSE : TRUE;
    argv[2] = (char *) &append;
    if (exeobj(obj, "output_file", array[i], 3, argv))
      break;
  }
  ProgressDialogFinalize();
  ResetStatusBar();
  gdk_window_invalidate_rect(NgraphApp.Viewer.gdk_win, NULL, FALSE);

  arraydel(&farray);
  g_free(file);
}

static gboolean
filewin_ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct obj_list_data *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || Globallock)
    return TRUE;

  d = (struct obj_list_data *) user_data;
  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_KEY_Delete:
    FileWinFileDelete(d);
    UnFocus();
    break;
  case GDK_KEY_Return:
    if (e->state & GDK_SHIFT_MASK) {
      return FALSE;
    }

    FileWinFileUpdate(d);
    UnFocus();
    break;
  case GDK_KEY_Insert:
    if (e->state & GDK_SHIFT_MASK) {
      FileWinFileCopy2(d);
    } else {
      FileWinFileCopy(d);
    }
    UnFocus();
    break;
  case GDK_KEY_space:
    if (e->state & GDK_CONTROL_MASK)
      return FALSE;

    FileWinFileDraw(d);
    UnFocus();
    break;
  case GDK_KEY_f:
    if (e->state & GDK_CONTROL_MASK) {
      FileWinFit(d);
      UnFocus();
    }
    break;
  default:
    return FALSE;
  }
  return TRUE;
}


static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, num;
  unsigned int i;
  struct obj_list_data *d;
  char *fit;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
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
      gtk_widget_set_sensitive(d->popup_item[i], sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel < num);
      break;
    case POPUP_ITEM_HIDE:
      if (sel >= 0 && sel <= num) {
	int hidden;
	getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(d->popup_item[i]), ! hidden);
      }
    default:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= num);
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
create_type_color_combo_box(GtkWidget *cbox, struct objlist *obj, int type)
{
  int count;
  GtkTreeStore *list;
  GtkTreeIter parent;

  count = combo_box_get_num(cbox);
  if (count > 0)
    return;

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  gtk_tree_store_append(list, &parent, NULL);
  gtk_tree_store_set(list, &parent,
		     OBJECT_COLUMN_TYPE_STRING, _("Type"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_TYPE, 
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);
  create_type_combo_box(cbox, obj, &parent);

  gtk_tree_store_append(list, &parent, NULL);
  gtk_tree_store_set(list, &parent,
		     OBJECT_COLUMN_TYPE_STRING, _("Color 1"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_COLOR_1,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);

  switch (type) {
  case PLOT_TYPE_MARK:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    gtk_tree_store_append(list, &parent, NULL);
    gtk_tree_store_set(list, &parent,
		       OBJECT_COLUMN_TYPE_STRING, _("Color 2"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_COLOR_2,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);
    break;
  }
}

static void
create_type_combo_box(GtkWidget *cbox, struct objlist *obj, GtkTreeIter *parent)
{
  char **enumlist, **curvelist;
  unsigned int i;
  int j;
  GtkTreeStore *list;

  enumlist = (char **) chkobjarglist(obj, "type");
  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
    GtkTreeIter iter, child;

    gtk_tree_store_append(list, &iter, parent);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_STRING, _(enumlist[i]),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_TYPE,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);

    if (strcmp(enumlist[i], "mark") == 0) {
      combo_box_create_mark(cbox, &iter, FILE_COMBO_ITEM_MARK);
    } else if (strcmp(enumlist[i], "curve") == 0) {
      curvelist = (char **) chkobjarglist(obj, "interpolation");
      for (j = 0; curvelist[j] && curvelist[j][0]; j++) {
	gtk_tree_store_append(list, &child, &iter);
	gtk_tree_store_set(list, &child,
			   OBJECT_COLUMN_TYPE_STRING, _(curvelist[j]),
			   OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			   OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_INTP,
			   OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
			   OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			   -1);
      }
    }
  }
}

static void
select_type(GtkComboBox *w, gpointer user_data)
{
  int sel, col_type, type, mark_type, curve_type, b, c, *ary, found, depth;
  struct objlist *obj;
  struct obj_list_data *d;
  GtkTreeStore *list;
  GtkTreeIter iter;
  GtkTreePath *path;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  obj = getobject("file");
  getobj(obj, "type", sel, 0, NULL, &type);

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_INT, &col_type, -1);
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);
  ary = gtk_tree_path_get_indices(path);
  depth = gtk_tree_path_get_depth(path);
  b = c = -1;

  switch (depth) {
  case 3:
    c = ary[2];
  case 2:
    b = ary[1];
  case 1:
    break;
  default:
    return;
  }

  gtk_tree_path_free(path);

  switch (col_type) {
  case FILE_COMBO_ITEM_COLOR_1:
    if (select_obj_color(obj, sel, OBJ_FIELD_COLOR_TYPE_1)) {
      return;
    }
    break;
  case FILE_COMBO_ITEM_COLOR_2:
    if (select_obj_color(obj, sel, OBJ_FIELD_COLOR_TYPE_2)) {
      return;
    }
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
  d->update(d, FALSE);
  set_graph_modified();
}

static void
start_editing_type(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  GtkTreeIter iter;
  struct obj_list_data *d;
  GtkComboBox *cbox;
  int sel, type;
  struct objlist *obj;

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path_str, &iter, FILE_WIN_COL_ID);
  if (sel < 0) {
    return;
  }

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  obj = getobject("file");
  if (obj == NULL)
    return;

  getobj(obj, "type", sel, 0, NULL, &type);
  create_type_color_combo_box(GTK_WIDGET(cbox), obj, type);

  g_signal_connect(cbox, "editing-done", G_CALLBACK(select_type), d);
  gtk_widget_show(GTK_WIDGET(cbox));


  return;
}

static void
select_axis(GtkComboBox *w, gpointer user_data, char *axis)
{
  char buf[64];
  int j, sel;
  struct obj_list_data *d;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  d = (struct obj_list_data *) user_data;

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

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data, int axis)
{
  GtkTreeIter iter;
  struct obj_list_data *d;
  GtkComboBox *cbox;
  int lastinst, j, sel, id = 0, is_oid;
  struct objlist *aobj;
  char *name, *ptr;

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path, &iter, FILE_WIN_COL_ID);
  if (sel < 0) {
    return;
  }

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
    g_free(name);
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
  struct obj_list_data *d;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  if (str == NULL || d->select < 0)
    return;

  d->update(d, FALSE);
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

  widget = d->data.data->text;

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, target, sizeof(target) / sizeof(*target), GDK_ACTION_COPY);
  g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_drop_cb), NULL);
}

void
CmFileWindow(GtkToggleAction *action, gpointer client_data)
{
  struct SubWin *d;
  GList *list;
  GtkTreeViewColumn *col;
  int state;

  d = &(NgraphApp.FileWin);

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

  list_sub_window_create(d, "Data Window", FILE_WIN_COL_NUM, Flist, Filewin_xpm, Filewin48_xpm);

  d->data.data->update = FileWinUpdate;
  d->data.data->setup_dialog = FileDialog;
  d->data.data->dialog = &DlgFile;
  d->data.data->ev_key = filewin_ev_key_down;
  d->data.data->delete = delete_file_obj;
  d->data.data->obj = chkobject("file");

  sub_win_create_popup_menu(d->data.data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
  set_combo_cell_renderer_cb(d->data.data, FILE_WIN_COL_X_AXIS, Flist, G_CALLBACK(start_editing_x), G_CALLBACK(edited_axis));
  set_combo_cell_renderer_cb(d->data.data, FILE_WIN_COL_Y_AXIS, Flist, G_CALLBACK(start_editing_y), G_CALLBACK(edited_axis));
  set_obj_cell_renderer_cb(d->data.data, FILE_WIN_COL_TYPE, Flist, G_CALLBACK(start_editing_type));

  init_dnd(d);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(d->data.data->text), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(d->data.data->text), FILE_WIN_COL_FILE);
  gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(d->data.data->text), FILE_WIN_COL_FILE);

  col = gtk_tree_view_get_column(GTK_TREE_VIEW(d->data.data->text), FILE_WIN_COL_FILE);
  list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  if (list){ 
    if (list->data) {
      GtkCellRenderer *renderer;
      renderer = list->data;
      gtk_tree_view_column_add_attribute(col, renderer, "style", FILE_WIN_COL_MASKED);
    }
    g_list_free(list);
  }
}
