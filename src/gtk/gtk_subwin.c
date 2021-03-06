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

static void
file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  struct obj_list_data *d;
  int sel, num;
  char *file, *ext;
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

  if (nGetOpenFileName(parent, _("Open"), ext, NULL, gtk_entry_get_text(w),
		       &file, TRUE, Menulocal.changedirectory) == IDOK && file) {
    if (file) {
      gtk_entry_set_text(w, file);
      modify_string(d, "file", file);
      g_free(file);
    }
  }
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
	gtk_entry_set_text(GTK_ENTRY(editable), buf);
      } else {
	char *valstr;

	if (strcmp(list->name, "file") == 0) {
	  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(editable), GTK_ENTRY_ICON_SECONDARY, "document-open-symbolic");
	  g_signal_connect(editable, "icon-release", G_CALLBACK(file_select), d);
	}
	sgetobjfield(d->obj, sel, list->name, NULL, &valstr, FALSE, FALSE, FALSE);
	if (valstr) {
	  gtk_entry_set_text(GTK_ENTRY(editable), CHK_STR(valstr));
	  g_free(valstr);
	}
      }
    }
    break;
  case G_TYPE_DOUBLE:
  case G_TYPE_INT:
    if (GTK_IS_SPIN_BUTTON(editable)) {
      gtk_entry_set_alignment(GTK_ENTRY(editable), 1.0);
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

static void
combo_edited_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct obj_list_data *d;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;
  gtk_widget_grab_focus(d->text);
}

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
  } else {
    list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(combo_edited_cb), d);
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

static void
swin_update(struct obj_list_data *d)
{
  int sel, ret, num, undo;
  GtkWidget *parent;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  parent = TopLevel;

  d->setup_dialog(d, sel, -1);
  d->select = sel;
  if (d->undo_save) {
    undo = d->undo_save(UNDO_TYPE_EDIT);
  } else {
    undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  }
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
do_popup(GdkEventButton *event, struct obj_list_data *d)
{
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
  gtk_menu_popup_at_pointer(GTK_MENU(d->popup), ((GdkEvent *)event));
}

#if GTK_CHECK_VERSION(3, 99, 0)
static void
ev_button_down(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct obj_list_data *d;
#if GTK_CHECK_VERSION(4, 0, 0)
  static guint32 time = 0;
  guint32 current_time;
  int tdif;
#endif
  guint button;

  if (Menulock || Globallock) return;

#if GTK_CHECK_VERSION(4, 0, 0)
  current_time = gtk_event_controller_get_current_event_time(GTK_EVENT_CONTROLLER(gesture));
  tdif = current_time - time;
  time = current_time;

  /* following check is necessary for editable column. */
  if (tdif > 0 && tdif < DOUBLE_CLICK_PERIOD)
    return;
#endif

  d = user_data;

  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
  switch (button) {
  case 1:
    if (n_press > 1) {
      swin_update(d);
    }
    break;
  }
}

static void
ev_button_up(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct obj_list_data *d;
  guint button;

  if (Menulock || Globallock) return;

  d = user_data;

  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
  switch (button) {
  case 3:
    if (d->popup) {
      do_popup(NULL, d);
    }
    break;
  }
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
      list_sub_window_add_focus(NULL, d);
    } else {
      list_sub_window_focus(NULL, d);
    }
    break;
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
      list_sub_window_add_focus(NULL, d);
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

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), view);
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
  gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
  gtk_label_set_single_line_mode(GTK_LABEL(label), FALSE);

  d->data.text = label;

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), label);
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

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), grid);
  d->data.data = data;
  d->Win = swin;

  return swin;
}

static gboolean
list_focused(GtkWidget *widget, GdkEvent *ev, gpointer user_data)
{
  set_focus_insensitive(&NgraphApp.Viewer);
  return FALSE;
}

#if GTK_CHECK_VERSION(3, 99, 0)
static void
add_event_controller(GtkWidget *widget, struct obj_list_data *data)
{
  GtkGesture *gesture;
  GtkEventController *controller;

  gesture = gtk_gesture_multi_press_new(widget);
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed", G_CALLBACK(ev_button_down), data);
  g_signal_connect(gesture, "released", G_CALLBACK(ev_button_up), data);

  controller = gtk_event_controller_key_new(widget);
  g_signal_connect(controller, "key-pressed", G_CALLBACK(ev_key_down), data);
}
#endif

static struct obj_list_data *
list_widget_create(struct SubWin *d, int lisu_num, n_list_store *list, int can_focus, GtkWidget **w)
{
  struct obj_list_data *data;
  GtkWidget *lstor, *swin;
  GList *col_list, *col;

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
  g_signal_connect(lstor, "focus-in-event", G_CALLBACK(list_focused), NULL);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(lstor), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(lstor), COL_ID);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), lstor);

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
list_sub_window_delete(GtkMenuItem *item, gpointer user_data)
{
  delete((struct obj_list_data *) user_data);
}

void
list_sub_window_copy(GtkMenuItem *item, gpointer user_data)
{
  copy((struct obj_list_data *) user_data);
}

void
list_sub_window_move_top(GtkMenuItem *item, gpointer user_data)
{
  move_top((struct obj_list_data *) user_data);
}

void
list_sub_window_move_last(GtkMenuItem *item, gpointer user_data)
{
  move_last((struct obj_list_data *) user_data);
}

void
list_sub_window_move_up(GtkMenuItem *item, gpointer user_data)
{
  move_up((struct obj_list_data *) user_data);
}

void
list_sub_window_move_down(GtkMenuItem *item, gpointer user_data)
{
  move_down((struct obj_list_data *) user_data);
}

void
list_sub_window_update(GtkMenuItem *item, gpointer user_data)
{
  swin_update((struct obj_list_data *) user_data);
}

void
list_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct obj_list_data *) user_data, FOCUS_MODE_NORMAL);
}

void
list_sub_window_focus_all(GtkMenuItem *item, gpointer user_data)
{
  focus_all((struct obj_list_data *) user_data);
}

void
list_sub_window_add_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct obj_list_data *) user_data, FOCUS_MODE_TOGGLE);
}

static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct obj_list_data *d;

  if (Menulock || Globallock) return TRUE;

  d = (struct obj_list_data *) client_data;
  do_popup(NULL, d);
  return TRUE;
}

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

void
list_sub_window_object_name(GtkMenuItem *w, gpointer client_data)
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
  update = set_object_name(d->obj, sel);
  if (update) {
    set_graph_modified();
  }
}

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
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
      submenu = create_recent_menu(RECENT_TYPE_DATA);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
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
  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(d->text), NULL);
  g_signal_connect(d->text, "popup-menu", G_CALLBACK(ev_popup_menu), d);

  if (cb)
    g_signal_connect(menu, "show", cb, d);

  return menu;
}
