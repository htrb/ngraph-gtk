/*
 * $Id: x11merge.c,v 1.33 2010-03-04 08:30:17 hito Exp $
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

#include "object.h"
#include "nstring.h"
#include "ioutil.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_widget.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11file.h"
#include "x11merge.h"
#include "x11commn.h"
#include "x11view.h"

static n_list_store Mlist[] = {
  {" ",          G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",          G_TYPE_INT,     TRUE, FALSE, "id"},
  {N_("file"),   G_TYPE_STRING,  TRUE, TRUE,  "file"},
  {N_("top"),    G_TYPE_DOUBLE,  TRUE, TRUE,  "top_margin",  - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("left"),   G_TYPE_DOUBLE,  TRUE, TRUE,  "left_margin", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("zoom_x"), G_TYPE_DOUBLE,  TRUE, TRUE,  "zoom_x",                     0, SPIN_ENTRY_MAX, 100, 1000},
  {N_("zoom_y"), G_TYPE_DOUBLE,  TRUE, TRUE,  "zoom_y",                     0, SPIN_ENTRY_MAX, 100, 1000},
  {"^#",         G_TYPE_INT,     TRUE, FALSE, "oid"},
};

#define MERG_WIN_COL_NUM (sizeof(Mlist)/sizeof(*Mlist))
#define MERG_WIN_COL_OID (MERG_WIN_COL_NUM - 1)
#define MERG_WIN_COL_HIDDEN 0
#define MERG_WIN_COL_ID     1
#define MERG_WIN_COL_FILE   2

static void merge_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);

static GActionEntry Popup_list[] = {
  {"mergeFocusAllAction",     list_sub_window_focus_all, NULL, NULL, NULL},
  {"mergeOrderTopAction",     list_sub_window_move_top, NULL, NULL, NULL},
  {"mergeOrderUpAction",      list_sub_window_move_up, NULL, NULL, NULL},
  {"mergeOrderDownAction",    list_sub_window_move_down, NULL, NULL, NULL},
  {"mergeOrderBottomAction",  list_sub_window_move_last, NULL, NULL, NULL},
  {"mergeAddAction",          CmMergeOpen, NULL, NULL, NULL},

  {"mergeDuplicateAction",    list_sub_window_copy, NULL, NULL, NULL},
  {"mergeDeleteAction",       list_sub_window_delete, NULL, NULL, NULL},
  {"mergeFocusAction",        list_sub_window_focus, NULL, NULL, NULL},
  {"mergeUpdateAction",       list_sub_window_update, NULL, NULL, NULL},
  {"mergeInstanceNameAction", list_sub_window_object_name, NULL, NULL, NULL},
};

#define POPUP_ITEM_NUM ((int) (sizeof(Popup_list) / sizeof(*Popup_list)))
#define POPUP_ITEM_FOCUS_ALL 0
#define POPUP_ITEM_TOP       1
#define POPUP_ITEM_UP        2
#define POPUP_ITEM_DOWN      3
#define POPUP_ITEM_BOTTOM    4
#define POPUP_ITEM_ADD       5
#else
static struct subwin_popup_list Popup_list[] = {
  {N_("_Add"),         G_CALLBACK(CmMergeOpen), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Duplicate"),   G_CALLBACK(list_sub_window_copy), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Delete"),       G_CALLBACK(list_sub_window_delete), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {"_Focus",           G_CALLBACK(list_sub_window_focus), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("focus _All"),   G_CALLBACK(list_sub_window_focus_all), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Preferences"), G_CALLBACK(list_sub_window_update), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Instance name"), G_CALLBACK(list_sub_window_object_name), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Top"),    G_CALLBACK(list_sub_window_move_top), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Up"),     G_CALLBACK(list_sub_window_move_up), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Down"),   G_CALLBACK(list_sub_window_move_down), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Bottom"), G_CALLBACK(list_sub_window_move_last), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_END},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list) - 1)
#define POPUP_ITEM_FOCUS_ALL 5
#define POPUP_ITEM_TOP       9
#define POPUP_ITEM_UP       10
#define POPUP_ITEM_DOWN     11
#define POPUP_ITEM_BOTTOM   12
#endif


static void
MergeDialogSetupItem(struct MergeDialog *d, int file, int id)
{
  double zm_x, zm_y;
  if (file) {
    SetWidgetFromObjField(d->file, d->Obj, id, "file");
    gtk_editable_set_position(GTK_EDITABLE(d->file), -1);
  }
  SetWidgetFromObjField(d->topmargin, d->Obj, id, "top_margin");
  SetWidgetFromObjField(d->leftmargin, d->Obj, id, "left_margin");
  SetWidgetFromObjField(d->zoom_x, d->Obj, id, "zoom_x");
  SetWidgetFromObjField(d->zoom_y, d->Obj, id, "zoom_y");
  zm_x = gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->zoom_x));
  zm_y = gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->zoom_y));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->link), zm_x == zm_y);
}

static void
merge_dialog_copy_response(int sel, gpointer user_data)
{
  struct MergeDialog *d;
  d = (struct MergeDialog *) user_data;
  if (sel != -1) {
    MergeDialogSetupItem(d, FALSE, sel);
  }
}

static void
MergeDialogCopy(GtkButton *btn, gpointer user_data)
{
  struct MergeDialog *d;
  d = (struct MergeDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, MergeFileCB, merge_dialog_copy_response, d);
}

static void
zoom_changed(GtkSpinButton *spin_button, gpointer user_data)
{
  struct MergeDialog *d;
  int linked;
  double zoom;
  d = user_data;
  linked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->link));
  if (! linked) {
    return;
  }
  zoom = gtk_spin_button_get_value(spin_button);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(d->zoom_y), zoom);
}

static void
link_toggled(GtkToggleButton *button, gpointer user_data)
{
  struct MergeDialog *d;
  int linked;
  d = user_data;
  linked = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(d->zoom_y, ! linked);
  if (linked) {
    zoom_changed(GTK_SPIN_BUTTON(d->zoom_x), user_data);
  }
}

static void
MergeDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MergeDialog *d;
  char title[64];

  d = (struct MergeDialog *) data;

  snprintf(title, sizeof(title), _("Merge %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *frame, *table;
    int i;
    table = gtk_grid_new();

    i = 0;
    w = create_file_entry(d->Obj);
    add_widget_to_table(table, w, _("_File:"), TRUE, i++);
    d->file = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Top Margin:"), FALSE, i++);
    d->topmargin = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Left Margin:"), FALSE, i++);
    d->leftmargin = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
    add_widget_to_table(table, w, _("zoom _X:"), FALSE, i++);
    g_signal_connect(w, "value-changed", G_CALLBACK(zoom_changed), d);
    d->zoom_x = w;

    d->link = add_toggle_button(table, i++, 1, NGRAPH_LINK_ICON, _("Link"), G_CALLBACK(link_toggled), d);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
    add_widget_to_table(table, w, _("zoom _Y:"), FALSE, i++);
    d->zoom_y = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(d->vbox), frame);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(MergeDialogCopy), d, "merge");
  }
  MergeDialogSetupItem(d, TRUE, d->Id);
}

static void
MergeDialogClose(GtkWidget *w, void *data)
{
  struct MergeDialog *d;
  int ret;

  d = (struct MergeDialog *) data;

  switch(d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromWidget(d->file, d->Obj, d->Id, "file"))
    return;
  if (SetObjFieldFromWidget(d->topmargin, d->Obj, d->Id, "top_margin"))
    return;
  if (SetObjFieldFromWidget(d->leftmargin, d->Obj, d->Id, "left_margin"))
    return;
  if (SetObjFieldFromWidget(d->zoom_x, d->Obj, d->Id, "zoom_x"))
    return;
  if (SetObjFieldFromWidget(d->zoom_y, d->Obj, d->Id, "zoom_y"))
    return;

  d->ret = ret;
}

void
MergeDialog(struct obj_list_data *data, int id, int user_data)
{
  struct MergeDialog *d;

  d = (struct MergeDialog *) data->dialog;

  d->SetupWindow = MergeDialogSetup;
  d->CloseWindow = MergeDialogClose;
  d->Obj = data->obj;
  d->Id = id;
}

static void
merge_open_response(struct response_callback *cb)
{
  if (cb->return_value == IDCANCEL) {
    int undo = GPOINTER_TO_INT(cb->data);
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
  }
  MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, DRAW_NOTIFY);
}

static void
CmMergeOpen_response(char *name, gpointer user_data)
{
  int id, undo;
  struct objlist *obj;

  if (name == NULL) {
    return;
  }

  if ((obj = chkobject("merge")) == NULL) {
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
  id = newobj(obj);
  if (id >= 0) {
    changefilename(name);
    putobj(obj, "file", id, name);
    MergeDialog(NgraphApp.MergeWin.data.data, id, -1);
    response_callback_add(&DlgMerge, merge_open_response, NULL, GINT_TO_POINTER(undo));
    DialogExecute(TopLevel, &DlgMerge);
  } else {
    g_free(name);
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, DRAW_NOTIFY);
  }
}

void
CmMergeOpen(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int chd;

  if (Menulock || Globallock)
    return;

  chd = Menulocal.changedirectory;
  nGetOpenFileName(TopLevel, _("Add Merge file"), "gra", NULL, NULL, chd,
                   CmMergeOpen_response, NULL);
}

static void
merge_close_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  struct narray *farray;
  struct objlist *obj;
  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  obj = d->Obj;
  if (cb->return_value == IDOK) {
    int i, num, *array;
    num = arraynum(farray);
    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_DELETE, obj->name);
    }
    array = arraydata(farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, array[i]);
      set_graph_modified();
    }
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, TRUE);
  }
  arrayfree(farray);
}

void
CmMergeClose(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    return;
  }
  SelectDialog(&DlgSelect, obj, _("close merge file (multi select)"), MergeFileCB, farray, NULL);
  response_callback_add(&DlgSelect, merge_close_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

struct merge_update_data
{
  int i, num, modified;
  struct narray *farray;
};

static void
merge_update_response_response(struct response_callback *cb)
{
  int num, *array;
  struct merge_update_data *data;
  struct narray *farray;

  data = (struct merge_update_data *) cb->data;
  data->i++;
  num = data->num;
  farray = data->farray;

  if (cb->return_value != IDCANCEL) {
    data->modified = TRUE;
  }

  if (data->i >= num) {
    if (data->modified) {
      MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, TRUE);
    }
    arrayfree(farray);
    g_free(data);
    return;
  }

  array = arraydata(farray);
  MergeDialog(NgraphApp.MergeWin.data.data, array[data->i], -1);
  response_callback_add(&DlgMerge, merge_update_response_response, NULL, data);
  DialogExecute(TopLevel, &DlgMerge);
}

static void
merge_update_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  struct narray *farray;
  struct objlist *obj;
  int *array, num;
  struct merge_update_data *data;
  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  obj = d->Obj;
  if (cb->return_value != IDOK) {
    arrayfree(farray);
    return;
  }

  num = arraynum(farray);
  if (num > 0) {
    menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
  }
  array = arraydata(farray);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    arrayfree(farray);
    return;
  }

  data->i = 0;
  data->num = num;
  data->farray = farray;
  data->modified = FALSE;

  array = arraydata(farray);
  MergeDialog(NgraphApp.MergeWin.data.data, array[0], -1);
  response_callback_add(&DlgMerge, merge_update_response_response, NULL, data);
  DialogExecute(TopLevel, &DlgMerge);
}

void
CmMergeUpdate(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    return;
  }
  SelectDialog(&DlgSelect, obj, _("merge file property (multi select)"), MergeFileCB, farray, NULL);
  response_callback_add(&DlgSelect, merge_update_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

void
MergeWinUpdate(struct obj_list_data *d, int clear, int draw)
{
  int redraw;
  if (Menulock || Globallock)
    return;

  if (d == NULL)
    return;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, merge_list_set_val);
  } else {
    list_sub_window_set(d, merge_list_set_val);
  }

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), MERG_WIN_COL_ID, d->select);
  }

  switch (draw) {
  case DRAW_REDRAW:
    getobj(Menulocal.obj, "redraw_flag", 0, 0, NULL, &redraw);
    if (redraw) {
//      NgraphApp.Viewer.allclear = TRUE;
      update_viewer(d);
    } else {
      draw_notify(TRUE);
    }
    break;
  case DRAW_NOTIFY:
    draw_notify(TRUE);
    break;
  }
}

static void
merge_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx;
  unsigned int i = 0;
  char *file, *bfile;

  for (i = 0; i < MERG_WIN_COL_NUM; i++) {
    switch (i) {
    case MERG_WIN_COL_FILE:
      getobj(d->obj, "file", row, 0, NULL, &file);
      bfile = getbasename(file);
      if (bfile) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, CHK_STR(bfile));
	g_free(bfile);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, FILL_STRING);
      }
      break;
    case MERG_WIN_COL_HIDDEN:
      getobj(d->obj, Mlist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Mlist[i].type, &cx);
      break;
    default:
      if (Mlist[i].type == G_TYPE_DOUBLE) {
	getobj(d->obj, Mlist[i].name, row, 0, NULL, &cx);
	list_store_set_double(GTK_WIDGET(d->text), iter, i, cx / 100.0);
      } else {
	getobj(d->obj, Mlist[i].name, row, 0, NULL, &cx);
	list_store_set_val(GTK_WIDGET(d->text), iter, i, Mlist[i].type, &cx);
      }
    }
  }
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, num, last_id, i;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    GAction *action;
    action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), Popup_list[i].name);
    switch (i) {
    case POPUP_ITEM_FOCUS_ALL:
      last_id = chkobjlastinst(d->obj);
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), last_id >= 0);
      break;
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel >= 0 && sel < num);
      break;
    case POPUP_ITEM_ADD:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), TRUE);
      break;
    default:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel >= 0 && sel <= num);
    }
  }
}

GtkWidget *
create_merge_list(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  list_sub_window_create(d, MERG_WIN_COL_NUM, Mlist);

  d->data.data->update = MergeWinUpdate;
  d->data.data->setup_dialog = MergeDialog;
  d->data.data->dialog = &DlgMerge;
  d->data.data->obj = chkobject("merge");

  sub_win_create_popup_menu(d->data.data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));

  init_dnd_file(d, FILE_TYPE_MERGE);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(d->data.data->text), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(d->data.data->text), MERG_WIN_COL_FILE);
  tree_view_set_tooltip_column(GTK_TREE_VIEW(d->data.data->text), MERG_WIN_COL_FILE);

  return d->Win;
}
