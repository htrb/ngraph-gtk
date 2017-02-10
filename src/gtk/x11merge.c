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
  {" ",        G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",        G_TYPE_INT,     TRUE, FALSE, "id"},
  {N_("file"), G_TYPE_STRING,  TRUE, TRUE,  "file"},
  {N_("top"),  G_TYPE_DOUBLE,  TRUE, TRUE,  "top_margin",  - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("left"), G_TYPE_DOUBLE,  TRUE, TRUE,  "left_margin", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("zoom"), G_TYPE_DOUBLE,  TRUE, TRUE,  "zoom",                       0, SPIN_ENTRY_MAX, 100, 1000},
  {"^#",       G_TYPE_INT,     TRUE, FALSE, "oid"},
};

#define MERG_WIN_COL_NUM (sizeof(Mlist)/sizeof(*Mlist))
#define MERG_WIN_COL_OID (MERG_WIN_COL_NUM - 1)
#define MERG_WIN_COL_HIDDEN 0
#define MERG_WIN_COL_ID     1
#define MERG_WIN_COL_FILE   2

static void merge_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);

static struct subwin_popup_list Popup_list[] = {
  {N_("_Add"),         G_CALLBACK(CmMergeOpen), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Duplicate"),   G_CALLBACK(list_sub_window_copy), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Delete"),       G_CALLBACK(list_sub_window_delete), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {"_Focus",           G_CALLBACK(list_sub_window_focus), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Preferences"), G_CALLBACK(list_sub_window_update), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Top"),    G_CALLBACK(list_sub_window_move_top), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Up"),     G_CALLBACK(list_sub_window_move_up), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Down"),   G_CALLBACK(list_sub_window_move_down), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Bottom"), G_CALLBACK(list_sub_window_move_last), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_END},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list) - 1)
#define POPUP_ITEM_TOP 7
#define POPUP_ITEM_UP 8
#define POPUP_ITEM_DOWN 9
#define POPUP_ITEM_BOTTOM 10


static void
MergeDialogSetupItem(struct MergeDialog *d, int file, int id)
{
  if (file) {
    SetWidgetFromObjField(d->file, d->Obj, id, "file");
    gtk_editable_set_position(GTK_EDITABLE(d->file), -1);
  }
  SetWidgetFromObjField(d->topmargin, d->Obj, id, "top_margin");
  SetWidgetFromObjField(d->leftmargin, d->Obj, id, "left_margin");
  SetWidgetFromObjField(d->zoom, d->Obj, id, "zoom");
}

static void
MergeDialogCopy(GtkWidget *w, gpointer data)
{
  struct MergeDialog *d;
  int sel;

  d = (struct MergeDialog *) data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    MergeDialogSetupItem(d, FALSE, sel);
  }
}

static void
MergeDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *frame, *table;
  struct MergeDialog *d;
  char title[64];
  int i;

  d = (struct MergeDialog *) data;

  snprintf(title, sizeof(title), _("Merge %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
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
    add_widget_to_table(table, w, _("_Zoom:"), FALSE, i++);
    d->zoom = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 4);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(MergeDialogCopy), d, "merge");

    gtk_widget_show_all(GTK_WIDGET(d->vbox));
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
  if (SetObjFieldFromWidget(d->zoom, d->Obj, d->Id, "zoom"))
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

void
CmMergeOpen(void *w, gpointer client_data)
{
  struct objlist *obj;
  char *name = NULL;
  int id, ret;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("merge")) == NULL)
    return;

  if (nGetOpenFileName(TopLevel, _("Add Merge file"), "gra", NULL, NULL, &name,
		       TRUE, Menulocal.changedirectory) != IDOK || ! name)
    return;

  id = newobj(obj);
  if (id >= 0) {
    changefilename(name);
    putobj(obj, "file", id, name);
    MergeDialog(NgraphApp.MergeWin.data.data, id, -1);
    ret = DialogExecute(TopLevel, &DlgMerge);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else {
      set_graph_modified();
    }
  } else {
    g_free(name);
  }
  MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE);
}

void
CmMergeClose(void *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int num, *array;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, array[i]);
      set_graph_modified();
    }
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE);
  }
  arraydel(&farray);
}

void
CmMergeUpdate(void *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i, j;
  int *array, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = arraydata(&farray);
    for (i = 0; i < num; i++) {
      MergeDialog(NgraphApp.MergeWin.data.data, array[i], -1);
      if (DialogExecute(TopLevel, &DlgMerge) == IDDELETE) {
	delobj(obj, array[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  array[j]--;
      }
    }
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE);
  }
  arraydel(&farray);
}

void
MergeWinUpdate(struct obj_list_data *d, int clear)
{
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
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
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
  unsigned int i;
  int sel, num;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
  for (i = 1; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      gtk_widget_set_sensitive(d->popup_item[i], sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel < num);
      break;
    default:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= num);
    }
  }
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
    data_dropped(filenames, num, FILE_TYPE_MERGE);
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
MergeWinState(struct SubWin *d, int state)
{
  if (d->Win) {
    sub_window_set_visibility(d, state);
    return;
  }

  if (! state) {
    return;
  }

  list_sub_window_create(d, "Merge Window", MERG_WIN_COL_NUM, Mlist, NGRAPH_MERGEWIN_ICON_FILE, NGRAPH_MERGEWIN_ICON48_FILE);

  d->data.data->update = MergeWinUpdate;
  d->data.data->setup_dialog = MergeDialog;
  d->data.data->dialog = &DlgMerge;
  d->data.data->obj = chkobject("merge");

  sub_win_create_popup_menu(d->data.data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));

  init_dnd(d);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(d->data.data->text), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(d->data.data->text), MERG_WIN_COL_FILE);
  tree_view_set_tooltip_column(GTK_TREE_VIEW(d->data.data->text), MERG_WIN_COL_FILE);
}
