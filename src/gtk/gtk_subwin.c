/*
 * $Id: gtk_subwin.c,v 1.68 2010-04-01 06:08:23 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>

#include "dir_defs.h"

#include "object.h"
#include "nstring.h"
#include "mathfn.h"

#include "init.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11view.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "gtk_liststore.h"
#include "gtk_widget.h"
#include "gtk_combo.h"

#include "gtk_subwin.h"

#define DOUBLE_CLICK_PERIOD 250

static void hidden(struct obj_list_data *d);
static void modify_numeric(struct obj_list_data *d, char *field, int val);
static void modify_string(struct obj_list_data *d, char *field, char *str);
static void toggle_boolean(struct obj_list_data *d, char *field, int sel);

void
set_cell_attribute_source(struct SubWin *d, const char *attr, int target_column, int source_column)
{
  GList *list;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  col = gtk_tree_view_get_column(GTK_TREE_VIEW(d->data.data->text), target_column);
  list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  if (list == NULL) {
    return;
  }

  if (list->data == NULL) {
    return;
  }

  renderer = list->data;
  gtk_tree_view_column_add_attribute(col, renderer, attr, source_column);
  g_list_free(list);
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
file_select_response(char *file, gpointer user_data)
{
  struct obj_list_data *d;
  d = (struct obj_list_data *) user_data;
  if (file) {
    modify_string(d, "file", file);
    g_free(file);
  }
}
#endif

static void
#if GTK_CHECK_VERSION(4, 0, 0)
file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, gpointer user_data)
#else
file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
#endif
{
  struct obj_list_data *d;
  int sel, num, chd;
  char *file, *ext;
  const char *str;
  GtkWidget *parent;

  d = user_data;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  ext = NULL;
  if (chkobjfield(d->obj, "ext") == 0) {
    getobj(d->obj, "ext", sel, 0, NULL, &ext);
  }

  parent = TopLevel;

#if GTK_CHECK_VERSION(4, 0, 0)
  str = gtk_editable_get_text(GTK_EDITABLE(w));
  chd = Menulocal.changedirectory;
  nGetOpenFileName(parent, _("Open"), ext, NULL, str, chd, file_select_response, d);
#else
  str = gtk_entry_get_text(w);
  chd = Menulocal.changedirectory;
  file = nGetOpenFileName(parent, _("Open"), ext, NULL, str, TRUE, chd);
  if (file) {
    gtk_entry_set_text(w, file);
    modify_string(d, "file", file);
    g_free(file);
  }
#endif
}

static void
select_enum(GtkComboBox *w, gpointer user_data)
{
  int j, val, sel;
  struct obj_list_data *d;
  n_list_store *list;

  d = (struct obj_list_data *) user_data;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if (sel < 0) {
    return;
  }

  list = g_object_get_data(G_OBJECT(w), "user-data");

  getobj(d->obj, list->name, sel, 0, NULL, &val);

  j = combo_box_get_active(GTK_WIDGET(w));
  if (j < 0 || j == val)
    return;

  menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  if (putobj(d->obj, list->name, sel, &j) >= 0) {
    d->select = sel;
  }
}

static void
start_editing_enum(GtkCellEditable *editable, struct obj_list_data *d, n_list_store *list)
{
  GtkComboBox *cbox;
  int sel, type;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", list);

  SetWidgetFromObjField(GTK_WIDGET(cbox), d->obj, sel, list->name);

  getobj(d->obj, list->name, sel, 0, NULL, &type);
  combo_box_set_active(GTK_WIDGET(cbox), type);

  d->select = -1;
  g_signal_connect(cbox, "changed", G_CALLBACK(select_enum), d);
}

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  n_list_store *list;
  struct obj_list_data *d;

  menu_lock(TRUE);
  UnFocus();

  d = user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  list = (n_list_store *) g_object_get_data(G_OBJECT(renderer), "user-data");
  if (list == NULL)
    return;

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);


  switch (list->type) {
  case G_TYPE_ENUM:
    start_editing_enum(editable, d, list);
    break;
  case G_TYPE_STRING:
    if (GTK_IS_ENTRY(editable)) {
      int sel;

      sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
      if (chkobjfieldtype(d->obj, list->name) == NDOUBLE) {
	char buf[64];
	double val;

	getobj(d->obj, list->name, sel, 0, NULL, &val);
	snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, val);
#if GTK_CHECK_VERSION(4, 0, 0)
	gtk_editable_set_text(GTK_EDITABLE(editable), buf);
#else
	gtk_entry_set_text(GTK_ENTRY(editable), buf);
#endif
      } else {
	char *valstr;

	if (strcmp(list->name, "file") == 0) {
	  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(editable), GTK_ENTRY_ICON_SECONDARY, "document-open-symbolic");
	  g_signal_connect(editable, "icon-release", G_CALLBACK(file_select), d);
	}
	sgetobjfield(d->obj, sel, list->name, NULL, &valstr, FALSE, FALSE, FALSE);
	if (valstr) {
#if GTK_CHECK_VERSION(4, 0, 0)
	  gtk_editable_set_text(GTK_EDITABLE(editable), CHK_STR(valstr));
#else
	  gtk_entry_set_text(GTK_ENTRY(editable), CHK_STR(valstr));
#endif
	  g_free(valstr);
	}
      }
    }
    break;
  case G_TYPE_DOUBLE:
  case G_TYPE_INT:
    if (GTK_IS_SPIN_BUTTON(editable)) {
#if GTK_CHECK_VERSION(4, 0, 0)
      gtk_editable_set_alignment(GTK_EDITABLE(editable), 1.0);
#else
      gtk_entry_set_alignment(GTK_ENTRY(editable), 1.0);
#endif
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(editable), FALSE);
      if (list->max == 36000) {
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(editable), TRUE);
      }
    }
    break;
  }
}

static void
cancel_editing(GtkCellRenderer *renderer, gpointer user_data)
{
  menu_lock(FALSE);
  if (Menulock || Globallock) {
    return;
  }

  if (user_data) {
    struct obj_list_data *d;
    d = (struct obj_list_data *) user_data;
    gtk_widget_grab_focus(d->text);
  }
}

static void
toggle_cb(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer user_data)
{
  struct obj_list_data *d;
  n_list_store *list;
  long int sel;
  GtkTreeModel *model;
  GtkTreeIter iter;

  d = user_data;

  list = (n_list_store *) g_object_get_data(G_OBJECT(cell_renderer), "user-data");
  if (list == NULL) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->text));
  if (model == NULL) {
    return;
  }

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, COL_ID, &sel, -1);

  toggle_boolean(d, list->name, sel);
}

static void
enum_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct obj_list_data *d;

  menu_lock(FALSE);
  if (Menulock || Globallock) {
    return;
  }

  d = (struct obj_list_data *) user_data;

  gtk_widget_grab_focus(d->text);

  if (str == NULL || d->select < 0)
    return;

  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
numeric_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  struct obj_list_data *d;
  n_list_store *list;
  double val;
  int ecode;

  menu_lock(FALSE);
  if (Menulock || Globallock) {
    return;
  }

  d = user_data;

  if (str == NULL)
    return;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  list = (n_list_store *) g_object_get_data(G_OBJECT(cell_renderer), "user-data");
  if (list == NULL)
    return;

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  if (list->type == G_TYPE_DOUBLE || list->type == G_TYPE_FLOAT)
    val *= 100;

  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE)) {
    modify_numeric(d, list->name, nround(val));
  }
}

static void
string_cb(GtkCellRenderer *renderer, gchar *path, gchar *str, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct obj_list_data *d;
  n_list_store *list;

  menu_lock(FALSE);
  if (Menulock || Globallock) {
    return;
  }

  d = user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  list = (n_list_store *) g_object_get_data(G_OBJECT(renderer), "user-data");
  if (list == NULL)
    return;

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);

  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE)) {
    modify_string(d, list->name, str);
  }
}

static void
set_cell_renderer_cb(struct obj_list_data *d, int n, n_list_store *list, GtkWidget *w)
{
  int i;
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkTreeView *view;
  GList *glist;

  view = GTK_TREE_VIEW(w);

  for (i = 0; i < n; i++) {
    if (! list[i].editable)
      continue;

    col = gtk_tree_view_get_column(view, i);
    glist = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
    rend = GTK_CELL_RENDERER(glist->data);
    g_list_free(glist);

    switch (list[i].type) {
    case G_TYPE_BOOLEAN:
      g_signal_connect(rend, "toggled", G_CALLBACK(toggle_cb), d);
      break;
    case G_TYPE_DOUBLE:
    case G_TYPE_INT:
      list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(numeric_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    case G_TYPE_ENUM:
      list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(enum_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), d);
      break;
    case G_TYPE_STRING:
      list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(string_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    }
  }
}

static GtkCellRenderer *
get_cell_renderer_from_tree_view(GtkWidget *view, int i)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GList *glist;

  col = gtk_tree_view_get_column(GTK_TREE_VIEW(view), i);
  glist = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  rend = GTK_CELL_RENDERER(glist->data);
  g_list_free(glist);

  return rend;
}

void
set_editable_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback end)
{
  GtkCellRenderer *rend;

  if (list == NULL || end == NULL || i < 0)
    return;

  if (! list[i].editable)
    return;

  rend = get_cell_renderer_from_tree_view(d->text, i);

  if (list[i].edited_id) {
    g_signal_handler_disconnect(rend, list[i].edited_id);
  }
  list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(end), d);
}

#if ! GTK_CHECK_VERSION(4, 0, 0)
static void
combo_edited_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct obj_list_data *d;

  menu_lock(FALSE);
  if (Menulock || Globallock) {
    return;
  }

  d = (struct obj_list_data *) user_data;
  gtk_widget_grab_focus(d->text);
}
#endif

void
set_combo_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback start, GCallback end)
{
  GtkCellRenderer *rend;

  if (list == NULL || i < 0)
    return;

  if (! list[i].editable || (list[i].type != G_TYPE_ENUM && list[i].type != G_TYPE_PARAM))
    return;

  rend = get_cell_renderer_from_tree_view(d->text, i);

  if (list[i].edited_id) {
    g_signal_handler_disconnect(rend, list[i].edited_id);
  }

  if (end) {
    list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(end), d);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  } else {
    list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(combo_edited_cb), d);
#endif
  }

  if (start)
    g_signal_connect(rend, "editing-started", G_CALLBACK(start), d);

  g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), d);
}

void
set_obj_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback start)
{
  GtkCellRenderer *rend;

  if (list == NULL || i < 0)
    return;

  if (! list[i].editable || list[i].type != G_TYPE_OBJECT)
    return;

  rend = get_cell_renderer_from_tree_view(d->text, i);

  if (start)
    g_signal_connect(rend, "editing-started", G_CALLBACK(start), d);

  g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), d);
}

void
update_viewer(struct obj_list_data *d)
{
  char const *objects[2];
  objects[0] = d->obj->name;
  objects[1] = NULL;
  ViewerWinUpdate(objects);
}

static void
obj_copy(struct objlist *obj, int dest, int src)
{
  char *field[] = {"name", NULL};

  copy_obj_field(obj, dest, src, field);
}

static void
copy(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);

  if (sel >= 0 && sel <= num) {
    int id, undo;
    undo = menu_save_undo_single(UNDO_TYPE_COPY, d->obj->name);
    id = newobj(d->obj);
    if (id < 0) {
      menu_delete_undo(undo);
      return;
    }
    obj_copy(d->obj, id, sel);
    set_graph_modified();
    d->select = id;
    d->update(d, FALSE, TRUE);
  }
}

static void
delete(struct obj_list_data *d)
{
  int sel, inst_num;
  int updated;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  inst_num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > inst_num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_DELETE, d->obj->name);
  if (d->delete) {
    d->delete(d, sel);
  } else {
    delobj(d->obj, sel);
  }

  inst_num = chkobjlastinst(d->obj);
  if (inst_num < 0) {
    d->select = -1;
    updated = TRUE;
  } else if (sel > inst_num) {
    d->select = inst_num;
    updated = FALSE;
  } else {
    d->select = sel;
    updated = FALSE;
  }
  d->update(d, updated, TRUE);
  set_graph_modified();
}

static void
move_top(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movetopobj(d->obj, sel);
  d->select = 0;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
move_last(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movelastobj(d->obj, sel);
  d->select = num;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
move_up(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if ((sel >= 1) && (sel <= num)) {
    menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    d->update(d, FALSE, TRUE);
    set_graph_modified();
  }
}

static void
move_down(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if ((sel >= 0) && (sel < num)) {
    menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    d->update(d, FALSE, TRUE);
    set_graph_modified();
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
struct swin_update_data {
  int undo;
  struct obj_list_data *d;
};

static int
swin_update_response(struct response_callback *cb)
{
  struct swin_update_data *data;
  struct obj_list_data *d;
  int undo, sel;

  data = (struct swin_update_data *) cb->data;
  d = data->d;
  undo = data->undo;
  sel = d->select;
  g_free(data);

  switch (cb->return_value) {
  case IDCANCEL:
    menu_undo_internal(undo);
    break;
  case IDDELETE:
    if (d->delete) {
      d->delete(d, sel);
    } else {
      delobj(d->obj, sel);
    }
    d->select = -1;
    d->update(d, FALSE, DRAW_REDRAW);
    break;
  default:
    d->update(d, FALSE, DRAW_NOTIFY);
  }
  return IDOK;
}
#endif

static void
swin_update(struct obj_list_data *d)
{
  int sel, ret, num, undo;
  GtkWidget *parent;
#if GTK_CHECK_VERSION(4, 0, 0)
  struct swin_update_data *data;
#endif

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  parent = TopLevel;

#if GTK_CHECK_VERSION(4, 0, 0)
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
#endif

  d->setup_dialog(d, sel, -1);
  d->select = sel;
  if (d->undo_save) {
    undo = d->undo_save(UNDO_TYPE_EDIT);
  } else {
    undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  /* must be implemented */
  data->d = d;
  data->undo = undo;
  response_callback_add(d->dialog, swin_update_response, NULL, data);
  DialogExecute(parent, d->dialog);
#else
  ret = DialogExecute(parent, d->dialog);
  switch (ret) {
  case IDCANCEL:
    menu_undo_internal(undo);
    break;
  case IDDELETE:
    if (d->delete) {
      d->delete(d, sel);
    } else {
      delobj(d->obj, sel);
    }
    d->select = -1;
    d->update(d, FALSE, DRAW_REDRAW);
    break;
  default:
    d->update(d, FALSE, DRAW_NOTIFY);
  }
#endif
}

static void
focus(struct obj_list_data *d, enum FOCUS_MODE add)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num))
    Focus(d->obj, sel, add);
}


static void
focus_all(struct obj_list_data *d)
{
  if (Menulock || Globallock)
    return;

  ViewerSelectAllObj(d->obj);
}

static void
toggle_boolean(struct obj_list_data *d, char *field, int sel)
{
  int v1, num;

  if (Menulock || Globallock)
    return;

  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  getobj(d->obj, field, sel, 0, NULL, &v1);
  v1 = ! v1;
  if (putobj(d->obj, field, sel, &v1) < 0) {
    return;
  }

  d->select = sel;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
modify_numeric(struct obj_list_data *d, char *field, int val)
{
  int sel, v1, v2, num, undo;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  getobj(d->obj, field, sel, 0, NULL, &v1);
  if (putobj(d->obj, field, sel, &val) < 0) {
    return;
  }

  getobj(d->obj, field, sel, 0, NULL, &v2);
  if (v1 != v2) {
    d->select = sel;
    d->update(d, FALSE, TRUE);
    set_graph_modified();
  } else {
    menu_delete_undo(undo);
  }
}

static void
modify_string(struct obj_list_data *d, char *field, char *str)
{
  int sel, num, undo;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  if (chk_sputobjfield(d->obj, sel, field, str)) {
    menu_delete_undo(undo);
    return;
  }

  d->select = sel;
  d->update(d, FALSE, TRUE);
}

static void
hidden(struct obj_list_data *d)
{
  int sel, num;
  int hidden;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
  hidden = hidden ? FALSE : TRUE;
  putobj(d->obj, "hidden", sel, &hidden);
  d->select = sel;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
do_popup(gdouble x, gdouble y, struct obj_list_data *d)
#else
do_popup(GdkEventButton *event, struct obj_list_data *d)
#endif
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GdkRectangle rect;
#endif
  if (d->parent->type == TypeFileWin ||
      d->parent->type == TypeAxisWin ||
      d->parent->type == TypeMergeWin ||
      d->parent->type == TypePathWin ||
      d->parent->type == TypeRectWin ||
      d->parent->type == TypeArcWin ||
      d->parent->type == TypeMarkWin ||
      d->parent->type == TypeTextWin) {
    d->select = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  rect.x = x;
  rect.y = y;
  rect.width = 1;
  rect.height = 1;
  gtk_popover_set_pointing_to(GTK_POPOVER(d->popup), &rect);
  gtk_popover_popup(GTK_POPOVER(d->popup));
#else
  gtk_menu_popup_at_pointer(GTK_MENU(d->popup), ((GdkEvent *)event));
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
static int
tree_view_select_pos(GtkGestureClick *gesture, gdouble x, gdouble y)
{
  GtkWidget *tree_view;
  GtkTreePath *path;
  GtkTreeSelection *selection;
  int bx, by;
  tree_view = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
  gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(tree_view), x, y, &bx, &by);
  if (! gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), bx, by, &path, NULL, NULL, NULL)) {
    return FALSE;
  }
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  gtk_tree_selection_select_path(selection, path);
  return TRUE;
}

static void
ev_button_down(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct obj_list_data *d;
  guint button;
  GtkEventSequenceState state = GTK_EVENT_SEQUENCE_NONE;

  if (Menulock || Globallock) return;

  d = user_data;

  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
  switch (button) {
  case 1:
    if (n_press > 1) {
      swin_update(d);
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    break;
  case 3:
    if (tree_view_select_pos(gesture, x, y)) {
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    if (d->popup) {
      do_popup(x, y, d);
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    break;
  }
  gtk_gesture_set_state(GTK_GESTURE(gesture), state);
}

static gboolean
ev_key_down(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct obj_list_data *d;
  GtkWidget *w;

  if (Menulock || Globallock)
    return TRUE;

  d = user_data;

  w = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
  if (d->ev_key && d->ev_key(w, keyval, state, user_data))
    return TRUE;

  switch (keyval) {
  case GDK_KEY_Delete:
    delete(d);
    break;
  case GDK_KEY_Insert:
    copy(d);
    break;
  case GDK_KEY_Home:
    if (state & GDK_SHIFT_MASK)
      move_top(d);
    else
      return FALSE;
    break;
  case GDK_KEY_End:
    if (state & GDK_SHIFT_MASK)
      move_last(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Up:
    if (state & GDK_SHIFT_MASK)
      move_up(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Down:
    if (state & GDK_SHIFT_MASK)
      move_down(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Return:
    if (state & GDK_SHIFT_MASK) {
      //      e->state &= ~ GDK_SHIFT_MASK;
      return FALSE;
    }

    swin_update(d);
    break;
  case GDK_KEY_BackSpace:
    hidden(d);
    break;
  case GDK_KEY_space:
    if (state & GDK_CONTROL_MASK)
      return FALSE;

    if (! d->can_focus)
      return FALSE;

    if (state & GDK_SHIFT_MASK) {
      focus(d, FOCUS_MODE_TOGGLE);
    } else {
#if GTK_CHECK_VERSION(4, 0, 0)
      list_sub_window_focus(NULL, NULL, d);
#else
      list_sub_window_focus(NULL, d);
#endif
    }
    break;
#if GTK_CHECK_VERSION(4, 0, 0)
  case GDK_KEY_Menu:
    do_popup(0, 0, d);
    break;
#endif
  default:
    return FALSE;
  }
  return TRUE;
}
#else
static gboolean
ev_button_down(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct obj_list_data *d;
  static guint32 time = 0;
  int tdif;

  if (Menulock || Globallock) return FALSE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  tdif = event->time - time;
  time = event->time;

  /* following check is necessary for editable column. */
  if (tdif > 0 && tdif < DOUBLE_CLICK_PERIOD)
    return TRUE;

  d = user_data;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      swin_update(d);
      return TRUE;
    }
    break;
  }

  return FALSE;
}

static gboolean
ev_button_up(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct obj_list_data *d;

  if (Menulock || Globallock) return FALSE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  d = user_data;

  switch (event->button) {
  case 3:
    if (d->popup) {
      do_popup(event, d);
      return TRUE;
    }
    break;
  }

  return FALSE;
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct obj_list_data *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || Globallock)
    return TRUE;

  d = user_data;
  e = (GdkEventKey *)event;

  if (d->ev_key && d->ev_key(w, e->keyval, e->state, user_data))
    return TRUE;

  switch (e->keyval) {
  case GDK_KEY_Delete:
    delete(d);
    break;
  case GDK_KEY_Insert:
    copy(d);
    break;
  case GDK_KEY_Home:
    if (e->state & GDK_SHIFT_MASK)
      move_top(d);
    else
      return FALSE;
    break;
  case GDK_KEY_End:
    if (e->state & GDK_SHIFT_MASK)
      move_last(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Up:
    if (e->state & GDK_SHIFT_MASK)
      move_up(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Down:
    if (e->state & GDK_SHIFT_MASK)
      move_down(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Return:
    if (e->state & GDK_SHIFT_MASK) {
      e->state &= ~ GDK_SHIFT_MASK;
      return FALSE;
    }

    swin_update(d);
    break;
  case GDK_KEY_BackSpace:
    hidden(d);
    break;
  case GDK_KEY_space:
    if (e->state & GDK_CONTROL_MASK)
      return FALSE;

    if (! d->can_focus)
      return FALSE;

    if (e->state & GDK_SHIFT_MASK) {
      focus(d, FOCUS_MODE_TOGGLE);
    } else {
      list_sub_window_focus(NULL, d);
    }
    break;
  default:
    return FALSE;
  }
  return TRUE;
}
#endif

static void
swin_realized(GtkWidget *widget, gpointer user_data)
{
  struct obj_list_data *ptr;

  ptr = (struct obj_list_data *) user_data;

  ptr->update(ptr, TRUE, TRUE);
}

GtkWidget *
text_sub_window_create(struct SubWin *d)
{
  GtkWidget *view, *swin;
  GtkTextBuffer *buf;

  buf = gtk_text_buffer_new(NULL);
  view = gtk_text_view_new_with_buffer(buf);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  d->data.text = view;

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
#else
  gtk_container_add(GTK_CONTAINER(swin), view);
#endif
  d->Win = swin;

  return swin;
}

GtkWidget *
label_sub_window_create(struct SubWin *d)
{
  GtkWidget *label, *swin;

  label = gtk_label_new(NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_START);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_label_set_wrap(GTK_LABEL(label), FALSE);
#else
  gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
#endif
  gtk_label_set_single_line_mode(GTK_LABEL(label), FALSE);

  d->data.text = label;

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), label);
#else
  gtk_container_add(GTK_CONTAINER(swin), label);
#endif
  d->Win = swin;

  return swin;
}

GtkWidget *
parameter_sub_window_create(struct SubWin *d)
{
  GtkWidget *grid, *swin;
  struct obj_list_data *data;

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 4);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  data->select = -1;
  data->parent = d;
  data->undo_save = NULL;
  data->can_focus = FALSE;
  data->list = NULL;
  data->list_col_num = 0;
  data->text = grid;

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), grid);
#else
  gtk_container_add(GTK_CONTAINER(swin), grid);
#endif
  d->data.data = data;
  d->Win = swin;

  return swin;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
list_focused(GtkEventControllerFocus *ev, gpointer user_data)
{
  set_focus_insensitive(&NgraphApp.Viewer);
}
#else
static gboolean
list_focused(GtkWidget *widget, GdkEvent *ev, gpointer user_data)
{
  set_focus_insensitive(&NgraphApp.Viewer);
  return FALSE;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
add_event_controller(GtkWidget *widget, struct obj_list_data *data)
{
  GtkGesture *gesture;

  gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));

  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed", G_CALLBACK(ev_button_down), data);

  add_event_key(widget, G_CALLBACK(ev_key_down), NULL, data);
}
#endif

static struct obj_list_data *
list_widget_create(struct SubWin *d, int lisu_num, n_list_store *list, int can_focus, GtkWidget **w)
{
  struct obj_list_data *data;
  GtkWidget *lstor, *swin;
  GList *col_list, *col;
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkEventController *ev;
#endif

  data = g_malloc0(sizeof(*data));
  data->select = -1;
  data->parent = d;
  data->undo_save = NULL;
  data->can_focus = can_focus;
  data->list = list;
  data->list_col_num = lisu_num;
  lstor = list_store_create(lisu_num, list);
  data->text = lstor;

  set_cell_renderer_cb(data, lisu_num, list, lstor);

#if GTK_CHECK_VERSION(3, 99, 0)
  add_event_controller(lstor, data);
#else
  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down), data);
  g_signal_connect(lstor, "button-release-event", G_CALLBACK(ev_button_up), data);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down), data);
#endif

  /* to handle key-press-event correctly in single window mode */
#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_event_controller_focus_new();
  g_signal_connect(ev, "enter", G_CALLBACK(list_focused), NULL);
  gtk_widget_add_controller(lstor, ev);
#else
  g_signal_connect(lstor, "focus-in-event", G_CALLBACK(list_focused), NULL);
#endif

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(lstor), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(lstor), COL_ID);

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), lstor);
#else
  gtk_container_add(GTK_CONTAINER(swin), lstor);
#endif

  col_list = gtk_tree_view_get_columns(GTK_TREE_VIEW(lstor));
  for (col = g_list_next(col_list); col; col = g_list_next(col)) {
    GList *rend_list;
    rend_list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col->data));
    gtk_tree_view_column_add_attribute(GTK_TREE_VIEW_COLUMN(col->data),
				       GTK_CELL_RENDERER(rend_list->data),
				       "sensitive", 0);
    g_list_free(rend_list);
  }
  g_list_free(col_list);

  g_signal_connect(swin, "realize", G_CALLBACK(swin_realized), data);

  *w = swin;

  return data;
}

GtkWidget *
list_sub_window_create(struct SubWin *d, int lisu_num, n_list_store *list)
{
  GtkWidget *swin;
  struct obj_list_data *data;

  data = list_widget_create(d, lisu_num, list, d->type != TypeFileWin, &swin);
  d->data.data = data;
  d->Win = swin;

  return swin;
}

gboolean
list_sub_window_must_rebuild(struct obj_list_data *d)
{
  int n, num;

  num = chkobjlastinst(d->obj);
  n = list_store_get_num(GTK_WIDGET(d->text));

  return (n != num + 1);
}

void
list_sub_window_build(struct obj_list_data *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i, num;

  num = chkobjlastinst(d->obj);
  list_store_clear(d->text);
  for (i = 0; i <= num; i++) {
    list_store_append(GTK_WIDGET(d->text), &iter);
    func(d, &iter, i);
  }
}

void
list_sub_window_set(struct obj_list_data *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i, num;
  gboolean state;

  state = list_store_get_iter_first(GTK_WIDGET(d->text), &iter);
  if (! state)
    return;

  num = chkobjlastinst(d->obj);
  for (i = 0; i <= num; i++) {
    func(d, &iter, i);
    if (! list_store_iter_next(GTK_WIDGET(d->text), &iter)) {
      break;
    }
  }
}


void
list_sub_window_delete
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  delete((struct obj_list_data *) user_data);
}

void
list_sub_window_copy
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  copy((struct obj_list_data *) user_data);
}

void
list_sub_window_move_top
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  move_top((struct obj_list_data *) user_data);
}

void
list_sub_window_move_last
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  move_last((struct obj_list_data *) user_data);
}

void
list_sub_window_move_up
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  move_up((struct obj_list_data *) user_data);
}

void
list_sub_window_move_down
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  move_down((struct obj_list_data *) user_data);
}

void
list_sub_window_update
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  swin_update((struct obj_list_data *) user_data);
}

void
list_sub_window_focus
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  focus((struct obj_list_data *) user_data, FOCUS_MODE_NORMAL);
}

void
list_sub_window_focus_all
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer user_data)
#else
(GtkMenuItem *item, gpointer user_data)
#endif
{
  focus_all((struct obj_list_data *) user_data);
}

#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct obj_list_data *d;

  if (Menulock || Globallock) return TRUE;

  d = (struct obj_list_data *) client_data;
  do_popup(NULL, d);
  return TRUE;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct set_object_name_data {
  struct objlist *obj;
  int id;
  char *name;
};

static void
set_object_name_response(int res, const char *str, gpointer user_data)
{
  struct set_object_name_data *data;
  struct objlist *obj;
  int id, undo;
  char *name, *new_name;

  data = (struct set_object_name_data *) user_data;
  obj = data->obj;
  id = data->id;
  name = data->name;
  g_free(data);

  if (res != IDOK) {
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
  if (str == NULL) {
    putobj(obj, "name", id, NULL);
    if (name) {
      set_graph_modified();
    } else {
      menu_delete_undo(undo);
    }
    return;
  }

  new_name = g_strdup(str);
  if (new_name == NULL) {
    menu_delete_undo(undo);
    return;
  }

  g_strstrip(new_name);
  if (new_name[0] == '\0') {
    g_free(new_name);
    new_name = NULL;
  }

  if (g_strcmp0(name, new_name) == 0) {
    g_free(new_name);
    menu_delete_undo(undo);
    return;
  }

  if (putobj(obj, "name", id, new_name) < 0) {
    g_free(new_name);
    menu_delete_undo(undo);
    return;
  }
  set_graph_modified();
  return;
}

static void
set_object_name(struct objlist *obj, int id)
{
  char *name, buf[256];
  int r;
  struct set_object_name_data *data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  getobj(obj, "name", id, 0, NULL, &name);
  data->obj = obj;
  data->id = id;
  data->name = name;
  snprintf(buf, sizeof(buf), "%s:%d:name", chkobjectname(obj), id);
  input_dialog(TopLevel, _("Instance name"), buf, name, _("_Apply"), NULL, NULL, set_object_name_response, data);
}
#else
static int
set_object_name(struct objlist *obj, int id)
{
  char *name, *new_name, buf[256];
  int r;
  getobj(obj, "name", id, 0, NULL, &name);
  new_name = NULL;
  snprintf(buf, sizeof(buf), "%s:%d:name", chkobjectname(obj), id);
  r = DialogInput(TopLevel, _("Instance name"), buf, name, NULL, NULL, &new_name, NULL, NULL);
  if (r != IDOK) {
    return 0;
  }
  if (g_strcmp0(name, new_name) == 0) {
    return 0;
  }
  if (new_name == NULL) {
    putobj(obj, "name", id, new_name);
    return 1;
  }
  g_strstrip(new_name);
  if (new_name[0] == '\0') {
    g_free(new_name);
    new_name = NULL;
  }
  if (putobj(obj, "name", id, new_name) < 0) {
    g_free(new_name);
    return 0;
  }
  return 1;
}
#endif

void
list_sub_window_object_name
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *item, gpointer client_data)
#endif
{
  struct obj_list_data *d;

  d = (struct obj_list_data*) client_data;
  int sel, update, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  set_object_name(d->obj, sel);
#else
  update = set_object_name(d->obj, sel);
  if (update) {
    set_graph_modified();
  }
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
void
sub_win_create_popup_menu(struct obj_list_data *d, int n, GActionEntry *list, GCallback cb)
{
  GtkApplication *app;
  GtkWidget *popup;
  GMenu *menu;
  char menu_id[256];
  const char *name;

  app = n_get_gtk_application();
  if (app == NULL) {
    return;
  }
  g_action_map_add_action_entries(G_ACTION_MAP(app), list, n, d);

  name = chkobjectname(d->obj);
  snprintf(menu_id, sizeof(menu_id), "%s-popup-menu", name);
  menu = gtk_application_get_menu_by_id(GtkApp, menu_id);

#if USE_NESTED_SUBMENUS
  popup = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(menu), POPOVERMEU_FLAG);
#else  /* USE_NESTED_SUBMENUS */
  popup = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
#endif	/* USE_NESTED_SUBMENUS */
  gtk_popover_set_has_arrow(GTK_POPOVER(popup), FALSE);
  widget_set_parent(popup, d->parent->Win);
  if (cb) {
    g_signal_connect(popup, "show", cb, d);
  }
  d->popup = popup;
}
#else
static GtkWidget *
create_popup_menu_sub(struct obj_list_data *d, int top, struct subwin_popup_list *list)
{
  GtkWidget *menu, *item, *submenu;
  int i;
  menu = gtk_menu_new();

  for (i = 0; list[i].type != POP_UP_MENU_ITEM_TYPE_END; i++) {
    switch (list[i].type) {
    case POP_UP_MENU_ITEM_TYPE_NORMAL:
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
      g_signal_connect(item, "activate", list[i].func, d);
      break;
    case POP_UP_MENU_ITEM_TYPE_CHECK:
      item = gtk_check_menu_item_new_with_mnemonic(_(list[i].title));
      g_signal_connect(item, "toggled", list[i].func, d);
      break;
    case POP_UP_MENU_ITEM_TYPE_MENU:
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
      submenu = create_popup_menu_sub(d, FALSE, list[i].submenu);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
      break;
    case POP_UP_MENU_ITEM_TYPE_RECENT_DATA:
#if ! GTK_CHECK_VERSION(4, 0, 0)
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
      submenu = create_recent_menu(RECENT_TYPE_DATA);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
#endif
      break;
    case POP_UP_MENU_ITEM_TYPE_SEPARATOR:
    default:
      item = gtk_separator_menu_item_new();
      break;
    }
    if (top) {
      d->popup_item[i] = item;
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  return menu;
}

GtkWidget *
sub_win_create_popup_menu(struct obj_list_data *d, int n, struct subwin_popup_list *list, GCallback cb)
{
  GtkWidget *menu;

  if (d->popup_item)
    g_free(d->popup_item);

  d->popup_item = g_malloc(sizeof(GtkWidget *) * n);

  menu = create_popup_menu_sub(d, TRUE, list);

  d->popup = menu;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show_all(menu);
#endif
  gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(d->text), NULL);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_signal_connect(d->text, "popup-menu", G_CALLBACK(ev_popup_menu), d);
#endif

  if (cb)
    g_signal_connect(menu, "show", cb, d);

  return menu;
}
#endif
