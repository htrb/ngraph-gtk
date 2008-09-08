/* 
 * $Id: gtk_subwin.c,v 1.29 2008/09/08 04:41:40 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <math.h>

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "mathfn.h"

#include "main.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11view.h"
#include "x11gui.h"
#include "gtk_liststore.h"

#include "gtk_subwin.h"

#define COL_ID 1
#define DOUBLE_CLICK_PERIOD 250

static int SaveWindowState = FALSE;

static void hidden(struct SubWin *d);
static void tree_hidden(struct LegendWin *d);
static void modify_numeric(struct SubWin *d, char *field, int val);
static void modify_string(struct SubWin *d, char *field, char *str);

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  n_list_store *list;
  struct SubWin *d;

  Menulock = TRUE;

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);

  list = (n_list_store *) gtk_object_get_user_data(GTK_OBJECT(renderer));

  switch (list->type) {
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

	sgetobjfield(d->obj, sel, list->name, NULL, &valstr, FALSE, FALSE, FALSE);
	gtk_entry_set_text(GTK_ENTRY(editable), valstr);
	memfree(valstr);
      }
    }
    break;
  case G_TYPE_DOUBLE:
  case G_TYPE_INT:
    if (GTK_IS_SPIN_BUTTON(editable)) {
      if (list->max == 36000)
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(editable), TRUE);

      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(editable), TRUE);
    }
    break;
  }
}

static void
cancel_editing(GtkCellRenderer *renderer, gpointer user_data)
{
  Menulock = FALSE;
}

static void
toggle_cb(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);

  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE)) {
    hidden(d);
  } else {
#if 0
    tree_hidden((struct LegendWin *) user_data);
#else
    int hide, n, m, dep, *ary;
    struct LegendWin *ld;
    GtkTreePath *gpth;

    ld = (struct LegendWin *) user_data;

    gpth = gtk_tree_path_new_from_string(path);
    if (gpth == NULL)
      return;

    dep = gtk_tree_path_get_depth(gpth);
    ary = gtk_tree_path_get_indices(gpth);

    if (dep != 2) {
      gtk_tree_path_free(gpth);
      return;
    }

    n = ary[0];
    m = ary[1];
    gtk_tree_path_free(gpth);

    if (m >= 0 && m <= ld->legend[n]) {
      hide = gtk_cell_renderer_toggle_get_active(cell_renderer);
      putobj(ld->obj[n], "hidden", m, &hide);
      hide = ! hide;
      gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, hide, -1);
    }
#endif
  }
}

static void
numeric_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  struct SubWin *d;
  n_list_store *list;
  double val;
  char *ptr;

  Menulock = FALSE;

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  list = (n_list_store *) gtk_object_get_user_data(GTK_OBJECT(cell_renderer));

  val = strtod(str, &ptr);
  if (val != val || val == HUGE_VAL || val == - HUGE_VAL || ptr[0] != '\0') {
    return;
  }

  if (list->type == G_TYPE_DOUBLE || list->type == G_TYPE_FLOAT)
    val *= 100;

  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE)) {
    modify_numeric(d, list->name, nround(val));
  }
}

static void
string_cb(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct SubWin *d;
  n_list_store *list;

  Menulock = FALSE;

  d = (struct SubWin *) user_data;

  view = GTK_TREE_VIEW(d->text);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path))
    return;

  list_store_select_iter(GTK_WIDGET(view), &iter);
  list = (n_list_store *) gtk_object_get_user_data(GTK_OBJECT(cell_renderer));

  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE)) {
    modify_string(d, list->name, str);
  }
}

static void
set_cell_renderer_cb(struct SubWin *d, int n, n_list_store *list, GtkWidget *w)
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
    glist = gtk_tree_view_column_get_cell_renderers(col);
    rend = GTK_CELL_RENDERER(glist->data);
    g_list_free(glist);

    switch (list[i].type) {
    case G_TYPE_BOOLEAN:
      g_signal_connect(rend, "toggled", G_CALLBACK(toggle_cb), d);
      break;
    case G_TYPE_DOUBLE:
    case G_TYPE_INT:
      g_signal_connect(rend, "edited", G_CALLBACK(numeric_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    case G_TYPE_STRING:
      g_signal_connect(rend, "edited", G_CALLBACK(string_cb), d);
      g_signal_connect(rend, "editing-started", G_CALLBACK(start_editing), d);
      g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
      break;
    }
  }
}

void
set_combo_cell_renderer_cb(struct SubWin *d, int i, n_list_store *list, GCallback start, GCallback end)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  GtkTreeView *view;
  GList *glist;

  view = GTK_TREE_VIEW(d->text);

  if (list == NULL || i < 0)
    return;

  if (! list[i].editable || list[i].type != G_TYPE_ENUM)
    return;

  col = gtk_tree_view_get_column(view, i);
  glist = gtk_tree_view_column_get_cell_renderers(col);
  rend = GTK_CELL_RENDERER(glist->data);
  g_list_free(glist);

  if (end) {
    g_signal_connect(rend, "edited", G_CALLBACK(end), d);
  } else {
    g_signal_connect(rend, "edited", G_CALLBACK(string_cb), d);
  }

  if (start)
    g_signal_connect(rend, "editing-started", G_CALLBACK(start), d);

  g_signal_connect(rend, "editing-canceled", G_CALLBACK(cancel_editing), NULL);
}

void
set_obj_cell_renderer_cb(struct SubWin *d, int i, n_list_store *list, GCallback start)
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
  glist = gtk_tree_view_column_get_cell_renderers(col);
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
    *w = CW_USEDEFAULT;
    *h = CW_USEDEFAULT;
    *x = CW_USEDEFAULT;
    *y = CW_USEDEFAULT;
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

#define DEFAULT_WIDTH 240
#define DEFAULT_HEIGHT 320

static void
cb_show(GtkWidget *widget, gpointer user_data)
{
  struct SubWin *d;
  int w, h, x, y, x0, y0;

  d = (struct SubWin *) user_data;

  if (d->Win == NULL) return;

  get_geometry(d, &x, &y, &w, &h);

  if (w == CW_USEDEFAULT) {
    w = DEFAULT_WIDTH;
  }

  if (h == CW_USEDEFAULT) {
    h = DEFAULT_HEIGHT;
  }

  if ((x != CW_USEDEFAULT) && (y != CW_USEDEFAULT)) {
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
}

static void
cb_hide(GtkWidget *widget, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  if (d->Win) {
    gint x, y, x0, y0, w0, h0;

    gtk_window_get_position(GTK_WINDOW(TopLevel), &x0, &y0);
    gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
    gtk_window_get_size(GTK_WINDOW(d->Win), &w0, &h0);

    set_geometry(d, x - x0, y - y0, w0, h0);

  }
}

static gboolean
cb_del(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  gtk_widget_hide_all(w);

  return TRUE;
}

static void
cb_destroy(GtkWidget *w, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  d->Win = NULL;

  if (d->popup)
    gtk_widget_destroy(d->popup);

  d->popup = NULL;

  if (d->popup_item)
    free(d->popup_item);

  d->popup_item = NULL;
}

static void
obj_copy(struct objlist *obj, int dest, int src)
{
  int j, perm, type;
  char *field;

  for (j = 0; j < chkobjfieldnum(obj); j++) {
    field = chkobjfieldname(obj, j);
    perm = chkobjperm(obj, field);
    type = chkobjfieldtype(obj, field);
    if (strcmp2(field, "name") && (perm & NREAD) && (perm & NWRITE) && (type < NVFUNC))
      copyobj(obj, field, dest, src);
  }
}

static void 
copy(struct SubWin *d)
{
  int sel, id;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if (sel >= 0 && sel <= d->num) {
    id = newobj(d->obj);
    if (id >= 0) {
      obj_copy(d->obj, id, sel);
      d->num++;
      NgraphApp.Changed = TRUE;
      d->select = id;
      d->update(FALSE);
    }
  }
}

static void 
delete(struct SubWin *d)
{
  int sel;
  int update;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if (sel >= 0 && sel <= d->num) {
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
    d->update(update);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_top(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel <= d->num)) {
    movetopobj(d->obj, sel);
    d->select = 0;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_last(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel <= d->num)) {
    movelastobj(d->obj, sel);
    d->select = d->num;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_up(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 1) && (sel <= d->num)) {
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_down(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel < d->num)) {
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
update(struct SubWin *d)
{
  int sel, ret;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
 
  if ((sel >= 0) && (sel <= d->num)) {
    d->setup_dialog(d->dialog, d->obj, sel, -1);
    d->select = sel;
    if ((ret = DialogExecute(d->Win, d->dialog)) == IDDELETE) {
      delobj(d->obj, sel);
      d->select = -1;
    }
    if (ret != IDCANCEL)
      NgraphApp.Changed = TRUE;
    d->update(FALSE);
  }
}

static void
focus(struct SubWin *d, int add)
{
  int sel;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if ((sel >= 0) && (sel <= d->num))
    Focus(d->obj, sel, add);
}

static void
modify_numeric(struct SubWin *d, char *field, int val)
{
  int sel, org;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if (sel < 0 || sel > d->num)
    return;

  getobj(d->obj, field, sel, 0, NULL, &org);
  if (org == val)
    return;

  if (putobj(d->obj, field, sel, &val) >= 0) {
    d->select = sel;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
modify_string(struct SubWin *d, char *field, char *str)
{
  int sel;
  char *valstr = NULL, *valstr2 = NULL;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if (sel < 0 || sel > d->num)
    return;

  sgetobjfield(d->obj, sel, field, NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr == NULL)
    return;

  if (sputobjfield(d->obj, sel, field, str)) {
    goto End;
  }

  sgetobjfield(d->obj, sel, field, NULL, &valstr2, FALSE, FALSE, FALSE);
  if (valstr2 == NULL) {
    goto End;
  }

  if (strcmp(valstr, valstr2) == 0) {
    goto End;
  }

  d->select = sel;
  d->update(FALSE);
  NgraphApp.Changed = TRUE;

 End:
  memfree(valstr);
  memfree(valstr2);
}

static void
hidden(struct SubWin *d)
{
  int sel;
  int hidden;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
    hidden = hidden ? FALSE : TRUE;
    putobj(d->obj, "hidden", sel, &hidden);
    d->select = sel;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  gtk_window_get_position(GTK_WINDOW(d->Win), x, y);
}

static void
do_popup(GdkEventButton *event, struct SubWin *d)
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

  if (d->type == TypeFileWin ||
      d->type == TypeAxisWin ||
      d->type == TypeMergeWin) {
    d->select = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  }

  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d,
		 button, event_time);
}

static gboolean
ev_button_down(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct SubWin *d;
  static guint32 time = 0;
  int tdif;

  if (Menulock || GlobalLock) return FALSE;

  tdif = event->time - time;
  time = event->time;

  if (tdif > 0 && tdif < DOUBLE_CLICK_PERIOD)
    return TRUE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  d = (struct SubWin *) user_data;

  if (d->ev_button && d->ev_button(w, event, user_data))
    return TRUE;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      update(d);
      return TRUE;
    }
    break;
  case 2:
    break;
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
  struct SubWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct SubWin *) user_data;
  e = (GdkEventKey *)event;

  if (d->ev_key && d->ev_key(w, event, user_data))
    return TRUE;

  switch (e->keyval) {
  case GDK_Delete:
    delete(d);
    break;
  case GDK_Insert:
    copy(d);
    break;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK)
      move_top(d);
    else
      return FALSE;
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK)
      move_last(d);
    else
      return FALSE;
    break;
  case GDK_Up:
    if (e->state & GDK_SHIFT_MASK)
      move_up(d);
    else
      return FALSE;
    break;
  case GDK_Down:
    if (e->state & GDK_SHIFT_MASK)
      move_down(d);
    else
      return FALSE;
    break;
  case GDK_Return:
    if (e->state & GDK_SHIFT_MASK) {
      e->state &= ~ GDK_SHIFT_MASK;
      return FALSE;
    }

    update(d);
    break;
  case GDK_BackSpace:
    hidden(d);
    break;
  case GDK_space:
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

static void
tree_copy(struct LegendWin *d)
{
  int n, m, sel, id;

  if (Menulock || GlobalLock) return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    id = newobj(d->obj[n]);
    if (id >= 0) {
      obj_copy(d->obj[n], id, m);
      d->select = id;
      d->legend_type = n;
      d->legend[n]++;
      d->update(FALSE);
      NgraphApp.Changed = TRUE;
    }
  }
}

static void 
tree_delete(struct LegendWin *d)
{
  int n, m;
  gboolean sel, update;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    delobj(d->obj[n], m);
    d->legend[n]--;
    update = FALSE;
    if (d->legend[n] < 0) {
      d->select = -1;
      d->legend_type = -1;
      update = TRUE;
    } else if (m > d->legend[n]) {
      d->legend_type = n;
      d->select = d->legend[n];
    } else {
      d->legend_type = n;
      d->select = m;
    }
    d->update(update);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_top(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    movetopobj(d->obj[n], m);
    d->select = 0;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_last(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    movelastobj(d->obj[n], m);
    d->select = d->legend[n];
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_up(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 1 && m <= d->legend[n]) {
    moveupobj(d->obj[n], m);
    d->select = m - 1;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_down(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m < d->legend[n]) {
    movedownobj(d->obj[n], m);
    d->select = m + 1;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_update(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    d->setup_dialog(d->dialog, d->obj[n], n, m);
  }
}

static void
tree_focus(struct LegendWin *d, int add)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (m < 0) {
    tree_store_selected_toggle_expand(GTK_WIDGET(d->text));
  } else if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    Focus(d->obj[n], m, add);
  }
}

static void
tree_hidden(struct LegendWin *d)
{
  int n, m, hidden;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    getobj(d->obj[n], "hidden", m, 0, NULL, &hidden);
    hidden = hidden ? FALSE : TRUE;
    putobj(d->obj[n], "hidden", m, &hidden);
    d->select = m;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static gboolean
ev_button_down_tree(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct LegendWin *d;
  static guint32 time = 0;
  int tdif;

  if (Menulock || GlobalLock) return FALSE;

  tdif = event->time - time;
  time = event->time;

  if (tdif > 0 && tdif < DOUBLE_CLICK_PERIOD)
    return TRUE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  d = (struct LegendWin *) user_data;

  if (d->ev_button && d->ev_button(w, event, user_data))
    return TRUE;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      tree_update(d);
      return TRUE;
    }
    break;
  case 2:
    break;
  case 3:
    if (d->popup) {
      do_popup(event, (struct SubWin *)d);
      return TRUE;
    }
    break;
  }

  return FALSE;
}

static gboolean
ev_key_down_tree(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct LegendWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct LegendWin *) user_data;
  e = (GdkEventKey *)event;

  if (d->ev_key && d->ev_key(w, event, user_data))
    return TRUE;

  switch (e->keyval) {
  case GDK_Delete:
    tree_delete(d);
    break;
  case GDK_Insert:
    tree_copy(d);
    break;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_top(d);
    else
      return FALSE;
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_last(d);
    else
      return FALSE;
    break;
  case GDK_Up:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_up(d);
    else
      return FALSE;
    break;
  case GDK_Down:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_down(d);
    else
      return FALSE;
    break;
  case GDK_Return:
    if (e->state & GDK_SHIFT_MASK) {
      e->state &= ~ GDK_SHIFT_MASK;
      return FALSE;
    }

    tree_update(d);
    break;
  case GDK_BackSpace:
    tree_hidden(d);
    break;
  case GDK_space:
    tree_focus(d, e->state & GDK_SHIFT_MASK);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void
sub_window_minimize(void *ptr)
{
  struct SubWin *d;
  GdkWindowState window_state;

  d = (struct SubWin *) ptr;

  if (d->Win == NULL || d->Win->window == NULL)
    return;

  window_state = gdk_window_get_state(d->Win->window);
  d->window_state = window_state;

  if (! (window_state & GDK_WINDOW_STATE_ICONIFIED)) {
    gtk_widget_hide_all(d->Win);
    //    gtk_window_iconify(GTK_WINDOW(d->Win));
    SaveWindowState = TRUE;
  }
}

void
sub_window_restore_state(void *ptr)
{
  struct SubWin *d;

  d = (struct SubWin *) ptr;

  if (d->Win == NULL)
    return;

  if (! SaveWindowState)
    return;

  if (d->window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
    return;

  //  gtk_window_deiconify(GTK_WINDOW(d->Win));
  gtk_widget_show_all(d->Win);
}

static gboolean
ev_sub_win_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;


  switch (e->keyval) {
  case GDK_w:
    if (e->state & GDK_CONTROL_MASK) {
      gtk_widget_hide_all(w);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

static GtkWidget *
sub_window_create(struct SubWin *d, char *title, GtkWidget *text, char **xpm)
{
  GtkWidget *dlg, *swin;
  GdkPixbuf *icon;
  GtkWindowGroup *group;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (AccelGroup)
    gtk_window_add_accel_group(GTK_WINDOW(dlg), AccelGroup);

  d->Win = dlg;

  if (xpm) {
    icon = create_pixbuf_from_xpm(TopLevel, xpm);
    gtk_window_set_icon(GTK_WINDOW(dlg), icon);
  }
  if (title) {
    gtk_window_set_title(GTK_WINDOW(dlg), title);
  }

  group = gtk_window_get_group(GTK_WINDOW(TopLevel));
  gtk_window_group_add_window(group, GTK_WINDOW(dlg));
  //  gtk_widget_set_parent_window(GTK_WIDGET(dlg), TopLevel);
  //  gtk_window_set_destroy_with_parent(GTK_WINDOW(dlg), TRUE);
  //  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_urgency_hint(GTK_WINDOW(dlg), FALSE);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(swin), text);
  gtk_container_add(GTK_CONTAINER(dlg), swin);

  d->swin = swin;

  g_signal_connect(dlg, "show", G_CALLBACK(cb_show), d);
  //  g_signal_connect(dlg, "hide", G_CALLBACK(cb_hide), d);
  g_signal_connect(dlg, "delete-event", G_CALLBACK(cb_del), d);
  g_signal_connect(dlg, "destroy", G_CALLBACK(cb_destroy), d);
  g_signal_connect(dlg, "key-press-event", G_CALLBACK(ev_sub_win_key_down), NULL);

  cb_show(dlg, d);
  return dlg;
}

GtkWidget *
text_sub_window_create(struct SubWin *d, char *title, char **xpm)
{
  GtkWidget *view;

  view = gtk_text_view_new_with_buffer(NULL);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  d->text = G_OBJECT(view);

  return sub_window_create(d, title, view, xpm);
}

GtkWidget *
list_sub_window_create(struct SubWin *d, char *title, int lisu_num, n_list_store *list, char **xpm)
{
  GtkWidget *lstor;

  lstor = list_store_create(lisu_num, list);
  d->text = G_OBJECT(lstor);

  set_cell_renderer_cb(d, lisu_num, list, lstor);

  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down), d);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down), d);

  return sub_window_create(d, title, lstor, xpm);
}

GtkWidget *
tree_sub_window_create(struct LegendWin *d, char *title, int lisu_num, n_list_store *list, char **xpm)
{
  GtkWidget *lstor;

  lstor = tree_store_create(lisu_num, list);
  d->text = G_OBJECT(lstor);

  set_cell_renderer_cb((struct SubWin *)d, lisu_num, list, lstor);

  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down_tree), d);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down_tree), d);

  return sub_window_create((struct SubWin *)d, title, lstor, xpm);
}

gboolean
list_sub_window_must_rebuild(struct SubWin *d)
{
  int n;

  d->num = chkobjlastinst(d->obj);
  n = list_store_get_num(GTK_WIDGET(d->text));

  return (n != d->num + 1);
}

void 
list_sub_window_build(struct SubWin *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i;

  d->num = chkobjlastinst(d->obj);
  list_store_clear(GTK_WIDGET(d->text));
  for (i = 0; i <= d->num; i++) {
    list_store_append(GTK_WIDGET(d->text), &iter);
    func(d, &iter, i);
  }
}

void 
list_sub_window_set(struct SubWin *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i;
  gboolean state;

  state = list_store_get_iter_first(GTK_WIDGET(d->text), &iter);

  if (! state)
    return;

  for (i = 0; i <= d->num; i++) {
    func(d, &iter, i);
    if (! list_store_iter_next(GTK_WIDGET(d->text), &iter)) {
      break;
    }
  }
}


void 
list_sub_window_delete(GtkMenuItem *item, gpointer user_data)
{
  delete((struct SubWin *) user_data);
}

void 
list_sub_window_copy(GtkMenuItem *item, gpointer user_data)
{
  copy((struct SubWin *) user_data);
}

void 
list_sub_window_move_top(GtkMenuItem *item, gpointer user_data)
{
  move_top((struct SubWin *) user_data);
}

void 
list_sub_window_move_last(GtkMenuItem *item, gpointer user_data)
{
  move_last((struct SubWin *) user_data);
}

void 
list_sub_window_move_up(GtkMenuItem *item, gpointer user_data)
{
  move_up((struct SubWin *) user_data);
}

void 
list_sub_window_move_down(GtkMenuItem *item, gpointer user_data)
{
  move_down((struct SubWin *) user_data);
}

void 
list_sub_window_update(GtkMenuItem *item, gpointer user_data)
{
  update((struct SubWin *) user_data);
}

void 
list_sub_window_hide(GtkMenuItem *item, gpointer user_data)
{
  hidden((struct SubWin *) user_data);
}

void 
list_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct SubWin *) user_data, FALSE);
}

void 
list_sub_window_add_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct SubWin *) user_data, TRUE);
}

void 
tree_sub_window_delete(GtkMenuItem *item, gpointer user_data)
{
  tree_delete((struct LegendWin *) user_data);
}

void 
tree_sub_window_copy(GtkMenuItem *item, gpointer user_data)
{
  tree_copy((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_top(GtkMenuItem *item, gpointer user_data)
{
  tree_move_top((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_last(GtkMenuItem *item, gpointer user_data)
{
  tree_move_last((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_up(GtkMenuItem *item, gpointer user_data)
{
  tree_move_up((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_down(GtkMenuItem *item, gpointer user_data)
{
  tree_move_down((struct LegendWin *) user_data);
}

void 
tree_sub_window_update(GtkMenuItem *item, gpointer user_data)
{
  tree_update((struct LegendWin *) user_data);
}

void 
tree_sub_window_hide(GtkMenuItem *item, gpointer user_data)
{
  tree_hidden((struct LegendWin *) user_data);
}

void 
tree_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  tree_focus((struct LegendWin *) user_data, FALSE);
}

void 
tree_sub_window_add_focus(GtkMenuItem *item, gpointer user_data)
{
  tree_focus((struct LegendWin *) user_data, TRUE);
}

static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  if (Menulock || GlobalLock) return TRUE;

  d = (struct SubWin *) client_data;
  do_popup(NULL, d);
  return TRUE;
}

GtkWidget *
sub_win_create_popup_menu(struct SubWin *d, int n, struct subwin_popup_list *list, GCallback cb)
{
  GtkWidget *menu, *item;
  int i = 0;

  if (d->popup_item)
    free(d->popup_item);

  d->popup_item = malloc(sizeof(GtkWidget *) * n);

  menu = gtk_menu_new();

  for (i = 0; i < n; i++) {
    if (list[i].use_stock) {
      item = gtk_image_menu_item_new_from_stock(list[i].title, list[i].accel_group);
    } else {
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
    }
    d->popup_item[i] = item;
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", list[i].func, d);
  }

  d->popup = menu;
  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(d->text), NULL);
  g_signal_connect(d->text, "popup-menu", G_CALLBACK(ev_popup_menu), d);

  if (cb)
    g_signal_connect(menu, "show", cb, d);

  return menu;
}
