/* 
 * $Id: gtk_liststore.c,v 1.23 2009/07/14 08:53:40 hito Exp $
 */

#include <stdlib.h>

#include "gtk_common.h"

#include "gtk_liststore.h"

static GtkCellEditable *
start_editing_obj(GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path,
	      GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
{
  GtkTreeModel *model;
  GtkCellRenderer *rend;
  GtkWidget *cbox;

  model = GTK_TREE_MODEL(gtk_tree_store_new(2, G_TYPE_OBJECT, G_TYPE_STRING));
  cbox = gtk_combo_box_new_with_model(model);

  rend = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "pixbuf", 0);

  rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "text", 1);


  g_object_set(cbox, "has-frame", FALSE, NULL);

  return GTK_CELL_EDITABLE(cbox);
}

static GtkTreeViewColumn *
create_column(n_list_store *list, int i, int j)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  switch (list[i].type) {
  case G_TYPE_BOOLEAN:
    renderer = gtk_cell_renderer_toggle_new();
    col = gtk_tree_view_column_new_with_attributes(list[i].title, renderer,
						   "active", i, NULL);
    if (list[i].editable) {
      g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
    }
    break;
  case G_TYPE_INT:
  case G_TYPE_UINT:
  case G_TYPE_LONG:
  case G_TYPE_ULONG:
  case G_TYPE_INT64:
  case G_TYPE_UINT64:
  case G_TYPE_FLOAT:
  case G_TYPE_DOUBLE:
    renderer = gtk_cell_renderer_spin_new();
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						   "text", i, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    g_object_set((GObject *) renderer, "xalign", (gfloat) 1.0, NULL);
    if (list[i].editable) {
      if (list[i].type == G_TYPE_DOUBLE || list[i].type == G_TYPE_FLOAT) {
	g_object_set((GObject *) renderer,
		     "editable", list[i].editable,
		     "adjustment", gtk_adjustment_new(0,
						      list[i].min / 100.0,
						      list[i].max / 100.0,
						      list[i].inc / 100.0,
						      list[i].page / 100.0,
						      0),
		     "digits", 2,
		     NULL);
	g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
      } else {
	g_object_set((GObject *) renderer,
		     "editable", list[i].editable,
		     "adjustment", gtk_adjustment_new(0,
						      list[i].min,
						      list[i].max,
						      list[i].inc,
						      list[i].page,
						      0),
		     "digits", 0,
		     NULL);
	g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);

      }
    }
    break;
  case G_TYPE_OBJECT:
    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set((GObject *) renderer,
		 "mode", (list[i].editable) ?
		 GTK_CELL_RENDERER_MODE_EDITABLE :
		 GTK_CELL_RENDERER_MODE_INERT,
		 "sensitive", list[i].editable,
		 NULL);
    g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
    GTK_CELL_RENDERER_GET_CLASS(renderer)->start_editing = start_editing_obj;
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						   "pixbuf", i, NULL);
    break;
  case G_TYPE_ENUM:
    renderer = gtk_cell_renderer_combo_new();
    model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
    g_object_set((GObject *) renderer,
		 "has-entry", FALSE, 
		 "model", model,
		 "text-column", 0,
		 "editable", list[i].editable, 
		 NULL);
    g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						   "text", i, NULL);
    break;
  case G_TYPE_STRING:
  default:
    renderer = gtk_cell_renderer_text_new();
    g_object_set((GObject *) renderer,
		 "editable", list[i].editable, 
		 "ellipsize", list[i].ellipsize, 
		 NULL);
    g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
    if (list[i].color){
      col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						     "text", i,
						     "foreground", j,
						     NULL);
    } else {
      col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						     "text", i, NULL);
    }
    gtk_tree_view_column_set_resizable(col, TRUE);
    if (list[i].ellipsize != PANGO_ELLIPSIZE_NONE)
      gtk_tree_view_column_set_expand(col, TRUE);
  }
  return col;
}

static GtkWidget *
create_tree_view(int n, n_list_store *list, int tree)
{
  GType *tarray;
  GtkTreeModel *lstore;
  GtkCellRenderer *renderer;
  GtkWidget *tview;
  GtkTreeViewColumn *col;
  GtkTreeSelection *sel;
  int i, j, cnum = 0;

  if (n < 1 || list == NULL)
    return NULL;

  for (i = 0; i < n; i++) {
    if (list[i].color) {
      cnum++;
    }
  }

  tarray = malloc(sizeof(*tarray) * (n + cnum));
  if (tarray == NULL)
    return NULL;

  for (i = 0; i < n; i++) {
    if (list[i].type == G_TYPE_DOUBLE || list[i].type == G_TYPE_ENUM) {
      tarray[i] = G_TYPE_STRING;
    } else {
      tarray[i] = list[i].type;
    }
    list[i].edited_id = 0;
  }

  for (i = 0; i < cnum; i++) {
    tarray[i + n] = G_TYPE_STRING;
    list[i].edited_id = 0;
  }

  if (tree) {
    lstore = GTK_TREE_MODEL(gtk_tree_store_newv(n + cnum, tarray));
  } else {
    lstore = GTK_TREE_MODEL(gtk_list_store_newv(n + cnum, tarray));
  }
  free(tarray);

  tview = gtk_tree_view_new_with_model(lstore);

  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tview), TRUE);
  gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tview), TRUE);
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tview), GTK_TREE_VIEW_GRID_LINES_VERTICAL);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tview));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  j = 0;
  for (i = 0; i < n; i++) {
    col = create_column(list, i, j + n);
    if (list[i].color){
      j++;
    }
    gtk_tree_view_column_set_visible(col, list[i].visible);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);
  }

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tview), n > 1);
  for (i = 0; i < cnum; i++) {
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("color", renderer,
						   "text", i + n,
						   NULL);
    gtk_tree_view_column_set_visible(col, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);
  }

  return tview;
}

void
list_store_set_sort_all(GtkWidget *tview)
{
  GList *list, *ptr;
  int i;
  GtkTreeViewColumn *column;

  list = gtk_tree_view_get_columns(GTK_TREE_VIEW(tview));

  if (list == NULL)
    return;

  for (ptr = list, i = 0; ptr; ptr = ptr->next, i++) {
    column = GTK_TREE_VIEW_COLUMN(ptr->data);
    gtk_tree_view_column_set_sort_column_id(column, i);
    gtk_tree_view_column_set_clickable(column, TRUE);
  }

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tview), TRUE);
  g_list_free(list);
}

void
list_store_set_sort_column(GtkWidget *tview, int col)
{
  GList *list, *ptr;

  list = gtk_tree_view_get_columns(GTK_TREE_VIEW(tview));

  if (list == NULL)
    return;

  ptr = g_list_nth(list, col);

  if (ptr) {
    GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(ptr->data);

    gtk_tree_view_column_set_sort_column_id(column, col);
    gtk_tree_view_column_set_clickable(column, TRUE);
  }

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tview), TRUE);
  g_list_free(list);
}

GtkWidget *
list_store_create(int n, n_list_store *list)
{
  return create_tree_view(n, list, FALSE);
}

GtkWidget *
tree_store_create(int n, n_list_store *list)
{
  return create_tree_view(n, list, TRUE);
}

int 
list_store_get_int(GtkWidget *w, GtkTreeIter *iter, int col)
{
  GtkTreeModel *model;
  int v = 0;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_model_get(model, iter, col, &v, -1);

  return v;
}

void 
list_store_set_int(GtkWidget *w, GtkTreeIter *iter, int col, int v)
{
  GtkListStore *list;

  list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  gtk_list_store_set(list, iter, col, v, -1);
}

void 
tree_store_set_int(GtkWidget *w, GtkTreeIter *iter, int col, int v)
{
  GtkTreeStore *list;

  list = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  gtk_tree_store_set(list, iter, col, v, -1);
}


int
list_store_path_get_int(GtkWidget *w, GtkTreePath *path, int col, int *val)
{
  GtkTreeModel *model;
  gboolean found;
  GtkTreeIter iter;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  found = gtk_tree_model_get_iter(model, &iter, path);

  if (! found)
    return 1;

  gtk_tree_model_get(model, &iter, col, val, -1);

  return 0;
}

void 
list_store_set_double(GtkWidget *w, GtkTreeIter *iter, int col, double v)
{
  char buf[128];

  snprintf(buf, sizeof(buf), "%.2f", v);
  list_store_set_string(w, iter, col, buf);
}

void 
tree_store_set_double(GtkWidget *w, GtkTreeIter *iter, int col, double v)
{
  char buf[128];

  snprintf(buf, sizeof(buf), "%.2f", v);
  tree_store_set_string(w, iter, col, buf);
}

gboolean 
list_store_get_boolean(GtkWidget *w, GtkTreeIter *iter, int col)
{
  GtkTreeModel *model;
  gboolean v;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_model_get(model, iter, col, &v, -1);

  return v;
}

void 
list_store_set_boolean(GtkWidget *w, GtkTreeIter *iter, int col, gboolean v)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_list_store_set(GTK_LIST_STORE(model), iter, col, v, -1);
}

void 
tree_store_set_boolean(GtkWidget *w, GtkTreeIter *iter, int col, gboolean v)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_store_set(GTK_TREE_STORE(model), iter, col, v, -1);
}

char *
list_store_get_string(GtkWidget *w, GtkTreeIter *iter, int col)
{
  GtkTreeModel *model;
  char *v;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_model_get(model, iter, col, &v, -1);

  return v;
}

char * 
list_store_path_get_string(GtkWidget *w, GtkTreePath *path, int col)
{
  GtkTreeModel *model;
  gboolean found;
  GtkTreeIter iter;
  char *v;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  found = gtk_tree_model_get_iter(model, &iter, path);

  if (! found)
    return NULL;

  gtk_tree_model_get(model, &iter, col, &v, -1);

  return v;
}

void 
list_store_set_string(GtkWidget *w, GtkTreeIter *iter, int col, const char *v)
{
  GtkListStore *list;

  list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  gtk_list_store_set(list, iter, col, v, -1);
}

void 
list_store_path_set_string(GtkWidget *w, GtkTreePath *path, int col, const char *v)
{
  GtkListStore *list;
  gboolean found;
  GtkTreeIter iter;

  list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  found = gtk_tree_model_get_iter(GTK_TREE_MODEL(list), &iter, path);

  if (! found)
    return;

  gtk_list_store_set(list, &iter, col, v, -1);
}

void 
tree_store_set_string(GtkWidget *w, GtkTreeIter *iter, int col, const char *v)
{
  GtkTreeStore *list;

  list = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  gtk_tree_store_set(list, iter, col, v, -1);
}

void 
list_store_set_pixbuf(GtkWidget *w, GtkTreeIter *iter, int col, GdkPixbuf *v)
{
  GtkListStore *list;

  list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(w)));
  gtk_list_store_set(list, iter, col, v, -1);
}

GdkPixbuf *
list_store_get_pixbuf(GtkWidget *w, GtkTreeIter *iter, int col)
{
  GtkTreeModel *model;
  GObject *v;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_model_get(model, iter, col, &v, -1);

  return  GDK_PIXBUF(v);
}

void 
list_store_set_val(GtkWidget *w, GtkTreeIter *iter, int col, GType type, void *ptr)
{
  int v;
  gboolean b;
  char *s;

  switch (type) {
  case G_TYPE_BOOLEAN:
    b = *((int *)ptr);
    list_store_set_boolean(w, iter, col, b);
    break;
  case G_TYPE_INT:
    v = *((int *)ptr);
    list_store_set_int(w, iter, col, v);
    break;
  case G_TYPE_STRING:
    s = (char *)ptr;
    list_store_set_string(w, iter, col, s);
    break;
  }
}

void 
tree_store_set_val(GtkWidget *w, GtkTreeIter *iter, int col, GType type, void *ptr)
{
  int v;
  gboolean b;
  char *s;

  switch (type) {
  case G_TYPE_BOOLEAN:
    b = *((int *)ptr);
    tree_store_set_boolean(w, iter, col, b);
    break;
  case G_TYPE_INT:
    v = *((int *)ptr);
    tree_store_set_int(w, iter, col, v);
    break;
  case G_TYPE_STRING:
    s = (char *)ptr;
    tree_store_set_string(w, iter, col, s);
    break;
  }
}

void 
list_store_clear(GtkWidget *w)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_list_store_clear(GTK_LIST_STORE(model));
}

void 
tree_store_clear(GtkWidget *w)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_store_clear(GTK_TREE_STORE(model));
}

void
list_store_append(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_list_store_append(GTK_LIST_STORE(model), iter);
}

void
tree_store_append(GtkWidget *w, GtkTreeIter *iter, GtkTreeIter *parent)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_store_append(GTK_TREE_STORE(model), iter, parent);
}

gboolean
list_store_get_iter_first(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  return gtk_tree_model_get_iter_first(model, iter);
}

gboolean
list_store_iter_next(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  return gtk_tree_model_iter_next(GTK_TREE_MODEL(model), iter);
}

gboolean
tree_store_get_iter_children(GtkWidget *w, GtkTreeIter *child, GtkTreeIter *iter)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  return gtk_tree_model_iter_children(model, child, iter);
}

gboolean
list_store_get_selected_iter(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeSelection *sel;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  return gtk_tree_selection_get_selected(sel, NULL, iter);
}

int 
list_store_get_selected_int(GtkWidget *w, int col)
{
  GtkTreeIter iter;

  if (! list_store_get_selected_iter(w, &iter))
    return -1;
 
  return list_store_get_int(w, &iter, col);
}

char *
list_store_get_selected_string(GtkWidget *w, int col)
{
  GtkTreeIter iter;

  if (! list_store_get_selected_iter(w, &iter))
    return NULL;
 
  return list_store_get_string(w, &iter, col);
}

gboolean 
tree_store_get_selected_nth(GtkWidget *w, int *n, int *m)
{
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath* path;
  int depth, *ary;
  gboolean state;
  
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  state = gtk_tree_selection_get_selected(sel, &model, &iter);

  if (! state)
    return FALSE;

  path = gtk_tree_model_get_path(model, &iter);

  depth = gtk_tree_path_get_depth(path);
  ary = gtk_tree_path_get_indices(path);

  *n = ary[0];
  if (depth < 2) {
    *m = -1;
  } else {
    *m = ary[1];
  }

  gtk_tree_path_free(path);
  return TRUE;
}

int
list_store_get_selected_index(GtkWidget *w)
{
  int m, n;
  gboolean state;

  state = tree_store_get_selected_nth(w, &n, &m);
  if (state)
    return n;

  return -1;
}

gboolean 
list_store_get_selected_nth(GtkWidget *w, int *n)
{
  int m;

  return tree_store_get_selected_nth(w, n, &m);
}

void 
list_store_select_all(GtkWidget *w)
{
  GtkTreeSelection *sel;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  gtk_tree_selection_select_all(sel);
}

void 
list_store_select_iter(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeModel *model;
  GtkTreeSelection *sel;
  GtkTreePath *path;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));

  gtk_tree_selection_select_iter(sel, iter);

  path = gtk_tree_model_get_path(model, iter);

  gtk_tree_view_set_cursor(GTK_TREE_VIEW(w), path, NULL, FALSE);

  gtk_tree_path_free(path);
}

void 
list_store_select_int(GtkWidget *w, int col, int id)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean state;
  int val;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  state = gtk_tree_model_get_iter_first(model, &iter);
  while (state) {
    val = list_store_get_int(w, &iter, col);
    if (val == id) {
      list_store_select_iter(w, &iter);
      break;
    }
    state = gtk_tree_model_iter_next(model, &iter);
  }
}

static void
select_path_str(GtkWidget *w, char *str)
{
  GtkTreePath *path;
  GtkTreeSelection *sel;
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));

  path = gtk_tree_path_new_from_string(str);
  if (! path)
    return;

  gtk_tree_selection_select_path(sel, path);

  gtk_tree_view_set_cursor(GTK_TREE_VIEW(w), path, NULL, FALSE); /* this line is comented out before (I forgot the reason). */

  gtk_tree_path_free(path);
}

static void
select_range_path_str(GtkWidget *w, char *from, char *to)
{
  GtkTreePath *path1, *path2;
  GtkTreeSelection *sel;
  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));

  path1 = gtk_tree_path_new_from_string(from);
  if (! path1)
    return;

  path2 = gtk_tree_path_new_from_string(to);
  if (! path2) {
    gtk_tree_path_free(path1);
    return;
  }

  gtk_tree_selection_select_range(sel, path1, path2);

  gtk_tree_path_free(path2);
  gtk_tree_path_free(path1);
}

void 
list_store_select_nth(GtkWidget *w, int n)
{
  char buf[1024];

  snprintf(buf, sizeof(buf), "%d", n);
  select_path_str(w, buf);
}

void
tree_store_select_nth(GtkWidget *w, int n, int m)
{
  char buf[1024];

  snprintf(buf, sizeof(buf), "%d:%d", n, m);
  select_path_str(w, buf);
}

void
list_store_multi_select_nth(GtkWidget *w, int n, int m)
{
  char buf1[1024], buf2[1024];

  snprintf(buf1, sizeof(buf1), "%d", n);
  snprintf(buf2, sizeof(buf2), "%d", m);
  select_range_path_str(w, buf1, buf2);
}

int
list_store_get_num(GtkWidget *w)
{
  return tree_store_get_child_num(w, NULL);
}

int
tree_store_get_child_num(GtkWidget *w, GtkTreeIter *iter)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  return gtk_tree_model_iter_n_children(model, iter);
}

void 
list_store_set_selection_mode(GtkWidget *w, GtkSelectionMode mode)
{
  GtkTreeSelection *gsel;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  gtk_tree_selection_set_mode(gsel, mode);
}

void
list_store_select_all_cb(GtkButton *w, gpointer client_data)
{
  list_store_select_all(GTK_WIDGET(client_data));
}

void
list_store_remove_selected_cb(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *sel;
  GList *selected, *data;
  GtkTreeModel *model;
  gboolean found;
  GtkTreeIter iter;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(client_data));
  selected = gtk_tree_selection_get_selected_rows(sel, &model);

  if (selected == NULL)
    return;

  for (data = g_list_last(selected); data; data = data->prev) {
    found = gtk_tree_model_get_iter(model, &iter, data->data);
    if (found)
      gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  }

  g_list_foreach(selected, free_tree_path_cb, NULL);
  g_list_free(selected);
}

void
free_tree_path_cb(gpointer data, gpointer user_data)
{
  gtk_tree_path_free(data);
}


void
tree_store_selected_toggle_expand(GtkWidget *w)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;

  if (! list_store_get_selected_iter(w, &iter))
    return;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  path = gtk_tree_model_get_path(model, &iter);

  if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(w), path)) {
    gtk_tree_view_collapse_row(GTK_TREE_VIEW(w), path);
  } else {
    gtk_tree_view_expand_row(GTK_TREE_VIEW(w), path, FALSE);
  }
}
