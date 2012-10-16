/*
 * $Id: gtk_subwin.c,v 1.68 2010-04-01 06:08:23 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "mathfn.h"

#include "main.h"
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

#if USE_ENTRY_ICON
static void
file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  struct obj_list_data *d;
  int sel, num;
  char *file, *ext;

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

  if (nGetOpenFileName(d->parent->Win, _("Open"), ext, NULL, gtk_entry_get_text(w),
		       &file, TRUE, Menulocal.changedirectory) == IDOK && file) {
    if (file) {
      gtk_entry_set_text(w, file);
      modify_string(d, "file", file);
      g_free(file);
    }
  }
}
#endif

#if GTK_CHECK_VERSION(3, 0, 0) && ! GTK_CHECK_VERSION(3, 2, 0)
static gboolean
cell_focus_out(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  menu_lock(FALSE);
  return FALSE;
}
#endif

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
	snprintf(buf, sizeof(buf), "%.15g", val);
	gtk_entry_set_text(GTK_ENTRY(editable), buf);
      } else {
	char *valstr;

#if USE_ENTRY_ICON
	if (strcmp(list->name, "file") == 0) {
	  gtk_entry_set_icon_from_stock(GTK_ENTRY(editable), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
	  g_signal_connect(editable, "icon-release", G_CALLBACK(file_select), d);
	}
#endif
	sgetobjfield(d->obj, sel, list->name, NULL, &valstr, FALSE, FALSE, FALSE);
	gtk_entry_set_text(GTK_ENTRY(editable), CHK_STR(valstr));
	g_free(valstr);
      }
#if GTK_CHECK_VERSION(3, 0, 0) && ! GTK_CHECK_VERSION(3, 2, 0)
      /* this code may need to avoid a bug of GTK+ 3.0 */
      g_signal_connect(editable, "focus-out-event", G_CALLBACK(cell_focus_out), d);
#endif
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

  if (str == NULL || d->select < 0)
    return;

  d->update(d, FALSE);
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
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    case G_TYPE_STRING:
      list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(string_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    }
  }
}

void
set_editable_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback end)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkTreeView *view;
  GList *glist;

  view = GTK_TREE_VIEW(d->text);

  if (list == NULL || end == NULL || i < 0)
    return;

  if (! list[i].editable)
    return;

  col = gtk_tree_view_get_column(view, i);
  glist = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  rend = GTK_CELL_RENDERER(glist->data);
  g_list_free(glist);

  if (list[i].edited_id) {
    g_signal_handler_disconnect(rend, list[i].edited_id);
  }
  list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(end), d);
}

void
set_combo_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback start, GCallback end)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkTreeView *view;
  GList *glist;

  view = GTK_TREE_VIEW(d->text);

  if (list == NULL || i < 0)
    return;

  if (! list[i].editable || (list[i].type != G_TYPE_ENUM && list[i].type != G_TYPE_PARAM))
    return;

  col = gtk_tree_view_get_column(view, i);
  glist = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  rend = GTK_CELL_RENDERER(glist->data);
  g_list_free(glist);

  if (list[i].edited_id) {
    g_signal_handler_disconnect(rend, list[i].edited_id);
  }

  if (end) {
    list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(end), d);
  } else {
    list[i].edited_id = g_signal_connect(rend, "edited", G_CALLBACK(string_cb), d);
  }

  if (start)
    g_signal_connect(rend, "editing-started", G_CALLBACK(start), d);

  g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
}

void
set_obj_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback start)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkTreeView *view;
  GList *glist;

  view = GTK_TREE_VIEW(d->text);

  if (list == NULL || i < 0)
    return;

  if (! list[i].editable || list[i].type != G_TYPE_OBJECT)
    return;

  col = gtk_tree_view_get_column(view, i);
  glist = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  rend = GTK_CELL_RENDERER(glist->data);
  g_list_free(glist);

  if (start)
    g_signal_connect(rend, "editing-started", G_CALLBACK(start), d);

  g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
}

static void
get_geometry(struct SubWin *d, int *x, int *y, int *w, int *h)
{
  switch (d->type) {
  case TypeFileWin:
    *w = Menulocal.filewidth;
    *h = Menulocal.fileheight;
    *x = Menulocal.filex;
    *y = Menulocal.filey;
    break;
  case TypeAxisWin:
    *w = Menulocal.axiswidth;
    *h = Menulocal.axisheight;
    *x = Menulocal.axisx;
    *y = Menulocal.axisy;
    break;
  case TypeLegendWin:
    *w = Menulocal.legendwidth;
    *h = Menulocal.legendheight;
    *x = Menulocal.legendx;
    *y = Menulocal.legendy;
    break;
  case TypeMergeWin:
    *w = Menulocal.mergewidth;
    *h = Menulocal.mergeheight;
    *x = Menulocal.mergex;
    *y = Menulocal.mergey;
    break;
  case TypeInfoWin:
    *w = Menulocal.dialogwidth;
    *h = Menulocal.dialogheight;
    *x = Menulocal.dialogx;
    *y = Menulocal.dialogy;
    break;
  case TypeCoordWin:
    *w = Menulocal.coordwidth;
    *h = Menulocal.coordheight;
    *x = Menulocal.coordx;
    *y = Menulocal.coordy;
    break;
  default:
    *w = DEFAULT_GEOMETRY;
    *h = DEFAULT_GEOMETRY;
    *x = DEFAULT_GEOMETRY;
    *y = DEFAULT_GEOMETRY;
    break;
  }
}

static void
set_geometry(struct SubWin *d, int x, int y, int w, int h)
{
  switch (d->type) {
  case TypeFileWin:
    Menulocal.filewidth = w;
    Menulocal.fileheight = h;
    Menulocal.filex = x;
    Menulocal.filey = y;
    break;
  case TypeAxisWin:
    Menulocal.axiswidth = w;
    Menulocal.axisheight = h;
    Menulocal.axisx = x;
    Menulocal.axisy = y;
    break;
  case TypeLegendWin:
    Menulocal.legendwidth = w;
    Menulocal.legendheight = h;
    Menulocal.legendx = x;
    Menulocal.legendy = y;
    break;
  case TypeMergeWin:
    Menulocal.mergewidth = w;
    Menulocal.mergeheight = h;
    Menulocal.mergex = x;
    Menulocal.mergey = y;
    break;
  case TypeInfoWin:
    Menulocal.dialogwidth = w;
    Menulocal.dialogheight = h;
    Menulocal.dialogx = x;
    Menulocal.dialogy = y;
    break;
  case TypeCoordWin:
    Menulocal.coordwidth = w;
    Menulocal.coordheight = h;
    Menulocal.coordx = x;
    Menulocal.coordy = y;
    break;
  }
}

static void
set_visibility(struct SubWin *d, int s)
{
  switch (d->type) {
  case TypeFileWin:
    Menulocal.fileopen = s;
    break;
  case TypeAxisWin:
    Menulocal.axisopen = s;
    break;
  case TypeLegendWin:
    Menulocal.legendopen = s;
    break;
  case TypeMergeWin:
    Menulocal.mergeopen = s;
    break;
  case TypeInfoWin:
    Menulocal.dialogopen = s;
    break;
  case TypeCoordWin:
    Menulocal.coordopen = s;
    break;
  }
}

#define DEFAULT_WIDTH 240
#define DEFAULT_HEIGHT 320

void
sub_window_set_geometry(struct SubWin *d, int resize)
{
  int w, h, x, y, x0, y0;

  if (d->Win == NULL) return;

  get_geometry(d, &x, &y, &w, &h);

  if (w == DEFAULT_GEOMETRY) {
    w = DEFAULT_WIDTH;
  }

  if (h == DEFAULT_GEOMETRY) {
    h = DEFAULT_HEIGHT;
  }

  if ((x != DEFAULT_GEOMETRY) && (y != DEFAULT_GEOMETRY)) {
    gtk_window_get_position(GTK_WINDOW(TopLevel), &x0, &y0);
    x += x0;
    y += y0;

    if (x < 0) {
      x = 0;
    }
    if (y < 0) {
      y = 0;
    }
    gtk_window_move(GTK_WINDOW(d->Win), x, y);
  }

  gtk_window_set_default_size(GTK_WINDOW(d->Win), w, h);

  if (resize)
    gtk_window_resize(GTK_WINDOW(d->Win), w, h);
}

static int
get_window_visibility(struct SubWin *d)
{
  GdkWindow *win;
  GdkWindowState state;

  if (d->Win == NULL) {
    return 0;
  }

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return 0;
  }

  state = gdk_window_get_state(win);

  return ! (state & GDK_WINDOW_STATE_WITHDRAWN);
}

void
sub_window_save_geometry(struct SubWin *d)
{
  gint x, y, x0, y0, w, h;

  if (! get_window_visibility(d)) {
    return;
  }

  gtk_window_get_position(GTK_WINDOW(TopLevel), &x0, &y0);
  get_window_geometry(d->Win, &x, &y, &w, &h);
  set_geometry(d, x - x0, y - y0, w, h);
}

void
sub_window_save_visibility(struct SubWin *d)
{
  int state;

  state = get_window_visibility(d);
  set_visibility(d, state);
}

static void
sub_window_hide(struct SubWin *d)
{
  if (d->Win) {
    sub_window_save_geometry(d);
    gtk_widget_hide(d->Win);
  }
}

static void
sub_window_show(struct SubWin *d)
{
  if (d->Win) {
    gtk_window_present(GTK_WINDOW(d->Win));
  }
}

static void
sub_window_show_all(struct SubWin *d)
{
  if (d->Win) {
    gtk_widget_show_all(d->Win);
  }
}

void
sub_window_set_visibility(struct SubWin *d, int state)
{
  if (d->Win == NULL) {
    return;
  }

  if (state) {
    if (! gtk_widget_get_realized(d->Win)) {
      struct obj_list_data *ptr;
      sub_window_show_all(d);
      sub_window_set_geometry(d, TRUE);
      switch (d->type) {
      case TypeFileWin:
      case TypeAxisWin:
      case TypeMergeWin:
      case TypeLegendWin:
	for (ptr = d->data.data; ptr; ptr = ptr->next) {
	  ptr->update(ptr, TRUE);
	}
	break;
      case TypeCoordWin:
      case TypeInfoWin:
	break;
      }
    }
    sub_window_show(d);
  } else {
    sub_window_hide(d);
  }
}

static void
cb_show(GtkWidget *widget, gpointer user_data)
{
  struct SubWin *d;

  d = user_data;

  if (d->Win == NULL) return;

  sub_window_set_geometry(d, FALSE);
}

static gboolean
cb_del(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;
  window_action_set_active(d->type, FALSE);

  return TRUE;
}

static void
cb_destroy(GtkWidget *w, gpointer user_data)
{
  struct SubWin *d;
  struct obj_list_data *ptr, *next;

  d = user_data;

  d->Win = NULL;

  switch (d->type) {
  case TypeFileWin:
  case TypeAxisWin:
  case TypeMergeWin:
  case TypeLegendWin:
    for (ptr = d->data.data; ptr; ptr = next) {
      if (ptr->popup) {
	gtk_widget_destroy(ptr->popup);
      }
      if (ptr->popup_item) {
	g_free(ptr->popup_item);
      }
      next = ptr->next;
      g_free(ptr);
    }
    d->data.data = NULL;
    break;
  default:
    d->data.text = NULL;
    break;
  }
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
  int sel, id, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);

  if (sel >= 0 && sel <= num) {
    id = newobj(d->obj);
    if (id >= 0) {
      obj_copy(d->obj, id, sel);
      set_graph_modified();
      d->select = id;
      d->update(d, FALSE);
    }
  }
}

static void
delete(struct obj_list_data *d)
{
  int sel, num;
  int update;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  if (d->delete) {
    d->delete(d, sel);
  } else {
    delobj(d->obj, sel);
  }

  num = chkobjlastinst(d->obj);
  if (num < 0) {
    d->select = -1;
    update = TRUE;
  } else if (sel > num) {
    d->select = num;
    update = FALSE;
  } else {
    d->select = sel;
    update = FALSE;
  }
  d->update(d, update);
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

  movetopobj(d->obj, sel);
  d->select = 0;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
move_last(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  movelastobj(d->obj, sel);
  d->select = num;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
move_up(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if ((sel >= 1) && (sel <= num)) {
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    d->update(d, FALSE);
    set_graph_modified();
  }
}

static void
move_down(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if ((sel >= 0) && (sel < num)) {
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    d->update(d, FALSE);
    set_graph_modified();
  }
}

static void
update(struct obj_list_data *d)
{
  int sel, ret, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  d->setup_dialog(d, sel, -1);
  d->select = sel;
  if ((ret = DialogExecute(d->parent->Win, d->dialog)) == IDDELETE) {
    if (d->delete) {
      d->delete(d, sel);
    } else {
      delobj(d->obj, sel);
    }
    d->select = -1;
    set_graph_modified();
  }
  d->update(d, FALSE);
}

static void
focus(struct obj_list_data *d, int add)
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
toggle_boolean(struct obj_list_data *d, char *field, int sel)
{
  int v1, num;

  if (Menulock || Globallock)
    return;

  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  getobj(d->obj, field, sel, 0, NULL, &v1);
  v1 = ! v1;
  if (putobj(d->obj, field, sel, &v1) < 0) {
    return;
  }

  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
modify_numeric(struct obj_list_data *d, char *field, int val)
{
  int sel, v1, v2, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  getobj(d->obj, field, sel, 0, NULL, &v1);

  if (putobj(d->obj, field, sel, &val) < 0) {
    return;
  }

  getobj(d->obj, field, sel, 0, NULL, &v2);
  if (v1 != v2) {
    d->select = sel;
    d->update(d, FALSE);
    set_graph_modified();
  }
}

static void
modify_string(struct obj_list_data *d, char *field, char *str)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  if (chk_sputobjfield(d->obj, sel, field, str))
    return;

  d->select = sel;
  d->update(d, FALSE);
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

  getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
  hidden = hidden ? FALSE : TRUE;
  putobj(d->obj, "hidden", sel, &hidden);
  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
set_hidden_state(struct obj_list_data *d, int hide)
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

  getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
  if (hidden != hide) {
    putobj(d->obj, "hidden", sel, &hide);
    d->select = sel;
    d->update(d, FALSE);
    set_graph_modified();
  }
}

static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct SubWin *d;

  d = user_data;

  gtk_window_get_position(GTK_WINDOW(d->Win), x, y);
}

static void
do_popup(GdkEventButton *event, struct obj_list_data *d)
{
  int button, event_time;
  GtkMenuPositionFunc func = NULL;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
    func = popup_menu_position;
  }

  if (d->parent->type == TypeFileWin ||
      d->parent->type == TypeAxisWin ||
      d->parent->type == TypeMergeWin ||
      d->parent->type == TypeLegendWin) {
    d->select = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  }

  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d,
		 button, event_time);
}

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

  if (d->ev_button && d->ev_button(w, event, user_data))
    return TRUE;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      update(d);
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

  if (d->ev_key && d->ev_key(w, event, user_data))
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

    update(d);
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

static gboolean
ev_sub_win_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;
  struct SubWin *d;

  d = user_data;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;


  switch (e->keyval) {
  case GDK_KEY_w:
    if (e->state & GDK_CONTROL_MASK) {
      window_action_set_active(d->type, FALSE);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

#ifdef WINDOWS
#include <gdk/gdkwin32.h>

static void
hide_minimize_menu_item(GtkWidget *widget, gpointer user_data)
{
  HWND handle;
  HMENU menu;

  handle = GDK_WINDOW_HWND(gtk_widget_get_window(widget));
  menu = GetSystemMenu(handle, FALSE);
  RemoveMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
}
#endif

static GtkWidget *
sub_window_create(struct SubWin *d, char *title, GtkWidget *swin, const char **xpm, const char **xpm2)
{
  GtkWidget *dlg;
  GdkPixbuf *icon;
  GtkWindowGroup *group;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (AccelGroup)
    gtk_window_add_accel_group(GTK_WINDOW(dlg), AccelGroup);

  d->Win = dlg;

  if (xpm) {
    icon = gdk_pixbuf_new_from_xpm_data(xpm);
    if (xpm2) {
      GList *tmp, *list = NULL;

      list = g_list_append(list, icon);

      icon = gdk_pixbuf_new_from_xpm_data(xpm2);
      list = g_list_append(list, icon);

      gtk_window_set_icon_list(GTK_WINDOW(dlg), list);

      tmp = list;
      while (tmp) {
	g_object_unref(tmp->data);
	tmp = tmp->next;
      }
      g_list_free(list);
    } else {
      gtk_window_set_icon(GTK_WINDOW(dlg), icon);
    }
  }
  if (title) {
    gtk_window_set_title(GTK_WINDOW(dlg), title);
  }

  group = gtk_window_get_group(GTK_WINDOW(TopLevel));
  gtk_window_group_add_window(group, GTK_WINDOW(dlg));
  //  gtk_widget_set_parent_window(GTK_WIDGET(dlg), TopLevel);
  //  gtk_window_set_destroy_with_parent(GTK_WINDOW(dlg), TRUE);
  //  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(TopLevel));
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(dlg), FALSE);
  gtk_window_set_urgency_hint(GTK_WINDOW(dlg), FALSE);

  gtk_container_add(GTK_CONTAINER(dlg), swin);

#ifdef WINDOWS
  g_signal_connect(dlg, "realize", G_CALLBACK(hide_minimize_menu_item), NULL);
#endif
  g_signal_connect(dlg, "show", G_CALLBACK(cb_show), d);
  g_signal_connect(dlg, "delete-event", G_CALLBACK(cb_del), d);
  g_signal_connect(dlg, "destroy", G_CALLBACK(cb_destroy), d);
  g_signal_connect(dlg, "key-press-event", G_CALLBACK(ev_sub_win_key_down), d);

  gtk_widget_show_all(swin);

  return dlg;
}

GtkWidget *
text_sub_window_create(struct SubWin *d, char *title, const char **xpm, const char **xpm2)
{
  GtkWidget *view, *swin;

  view = gtk_text_view_new_with_buffer(NULL);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  d->data.text = view;

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), view);

  return sub_window_create(d, title, swin, xpm, xpm2);
}

GtkWidget *
label_sub_window_create(struct SubWin *d, char *title, const char **xpm, const char **xpm2)
{
  GtkWidget *label, *swin;

  label = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_START);
#else
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
#endif
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
  gtk_label_set_single_line_mode(GTK_LABEL(label), FALSE);

  d->data.text = label;

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), label);

  return sub_window_create(d, title, swin, xpm, xpm2);
}

static struct obj_list_data *
list_widget_create(struct SubWin *d, int lisu_num, n_list_store *list, int can_focus, GtkWidget **w)
{
  struct obj_list_data *data;
  GtkWidget *lstor, *swin;

  data = g_malloc0(sizeof(*data));
  data->select = -1;
  data->parent = d;
  data->can_focus = can_focus;
  data->list = list;
  data->list_col_num = lisu_num;
  data->next = NULL;
  lstor = list_store_create(lisu_num, list);
  data->text = lstor;

  set_cell_renderer_cb(data, lisu_num, list, lstor);

  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down), data);
  g_signal_connect(lstor, "button-release-event", G_CALLBACK(ev_button_up), data);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down), data);

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(lstor), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(lstor), COL_ID);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), lstor);

  *w = swin;

  return data;
}

GtkWidget *
list_sub_window_create(struct SubWin *d, char *title, int lisu_num, n_list_store *list, const char **xpm, const char **xpm2)
{
  GtkWidget *swin;
  struct obj_list_data *data;

  data = list_widget_create(d, lisu_num, list, d->type != TypeFileWin, &swin);
  d->data.data = data;

  return sub_window_create(d, title, swin, xpm, xpm2);
}

GtkWidget *
tree_sub_window_create(struct SubWin *d, char *title, int page_num, int *lisu_num, n_list_store **list, GtkWidget **icons, const char **xpm, const char **xpm2)
{
  GtkWidget *tab, *swin;
  int i;
  struct obj_list_data *data, *prev;

  tab = gtk_notebook_new();

  prev = NULL;
  for (i = 0; i < page_num; i++) {
    data = list_widget_create(d, lisu_num[i], list[i], TRUE, &swin);
    if (prev) {
      prev->next = data;
    } else {
      d->data.data = data;
    }
    gtk_notebook_append_page(GTK_NOTEBOOK(tab), swin, icons[i]);
    prev = data;
  }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(tab), 0);

  return sub_window_create((struct SubWin *)d, title, tab, xpm, xpm2);
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
  update((struct obj_list_data *) user_data);
}

void
list_sub_window_hide(GtkMenuItem *item, gpointer user_data)
{
  int hide;

  hide = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
  set_hidden_state((struct obj_list_data *)user_data, ! hide);
}

void
list_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct obj_list_data *) user_data, FALSE);
}

void
list_sub_window_add_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct obj_list_data *) user_data, TRUE);
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

GtkWidget *
sub_win_create_popup_menu(struct obj_list_data *d, int n, struct subwin_popup_list *list, GCallback cb)
{
  GtkWidget *menu, *item;
  int i = 0;

  if (d->popup_item)
    g_free(d->popup_item);

  d->popup_item = g_malloc(sizeof(GtkWidget *) * n);

  menu = gtk_menu_new();

  for (i = 0; i < n; i++) {
    switch (list[i].type) {
    case POP_UP_MENU_ITEM_TYPE_NORMAL:
      if (list[i].use_stock) {
	item = gtk_image_menu_item_new_from_stock(list[i].title, list[i].accel_group);
      } else {
	item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
      }
      g_signal_connect(item, "activate", list[i].func, d);
      break;
    case POP_UP_MENU_ITEM_TYPE_CHECK:
      item = gtk_check_menu_item_new_with_mnemonic(_(list[i].title));
      g_signal_connect(item, "toggled", list[i].func, d);
      break;
    case POP_UP_MENU_ITEM_TYPE_SEPARATOR:
    default:
      item = gtk_separator_menu_item_new();
      break;
    }
    d->popup_item[i] = item;
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  d->popup = menu;
  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(d->text), NULL);
  g_signal_connect(d->text, "popup-menu", G_CALLBACK(ev_popup_menu), d);

  if (cb)
    g_signal_connect(menu, "show", cb, d);

  return menu;
}
