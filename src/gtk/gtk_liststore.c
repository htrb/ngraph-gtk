/*
 * $Id: gtk_liststore.c,v 1.27 2010-02-03 01:18:12 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>

#include "gra.h"

#include "gtk_liststore.h"

#include "x11dialg.h"


static gboolean
tree_view_set_tooltip_query_cb(GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_tip,
			       GtkTooltip *tooltip,
			       gpointer    data)
{
  char *str;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeModel *model;
  GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
  int column;

  if (!gtk_tree_view_get_tooltip_context(GTK_TREE_VIEW(widget),
					 &x, &y,
					 keyboard_tip,
					 &model, &path, &iter)) {
    return FALSE;
  }

  column = GPOINTER_TO_INT(data);
  if (column < 0) {
    return FALSE;
  }

  str = NULL;
  gtk_tree_model_get(model, &iter, column, &str, -1);
  if (str == NULL) {
    return FALSE;
  }

  gtk_tooltip_set_text(tooltip, str);
  gtk_tree_view_set_tooltip_row(tree_view, tooltip, path);

  g_free(str);
  gtk_tree_path_free(path);

  return TRUE;
}

void
tree_view_set_tooltip_column(GtkTreeView *tree_view, gint column)
{
  g_signal_connect(tree_view, "query-tooltip",
		   G_CALLBACK(tree_view_set_tooltip_query_cb), GINT_TO_POINTER(column));
  gtk_widget_set_has_tooltip(GTK_WIDGET(tree_view), TRUE);
}

static gboolean
combo_box_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  char *str;
  int r;

  gtk_tree_model_get(model, iter, OBJECT_COLUMN_TYPE_STRING, &str, -1);
  r = ! str;

  if (str) {
    g_free(str);
  }

  return r;
}

void
init_object_combo_box(GtkWidget *cbox)
{
  GtkCellRenderer *rend;
  GtkTreeViewRowSeparatorFunc func;

  func = gtk_combo_box_get_row_separator_func(GTK_COMBO_BOX(cbox));
  if (func == NULL) {
    gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(cbox), combo_box_separator_func, NULL, NULL);
  }

  gtk_cell_layout_clear(GTK_CELL_LAYOUT(cbox));

  rend = gtk_cell_renderer_toggle_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cbox), rend,
				 "active", OBJECT_COLUMN_TYPE_TOGGLE,
				 "visible", OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE,
				 "radio", OBJECT_COLUMN_TYPE_TOGGLE_IS_RADIO,
				 NULL);

  rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "text", OBJECT_COLUMN_TYPE_STRING);

  rend = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "pixbuf", OBJECT_COLUMN_TYPE_PIXBUF);
}

static GtkTreeModel *
create_object_tree_model(void)
{
  return GTK_TREE_MODEL(gtk_tree_store_new(OBJECT_COLUMN_TYPE_NUM, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN));
}

static GtkWidget *
create_object_cbox(void)
{
  GtkTreeModel *model;
  GtkWidget *cbox;

  model = create_object_tree_model();
  cbox = gtk_combo_box_new_with_model(model);

  init_object_combo_box(cbox);
  g_object_set(cbox, "has-frame", FALSE, NULL);

  return cbox;
}

void
add_separator_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent)
{
  add_text_combo_item_to_cbox(list, iter, parent, -1, -1, NULL, TOGGLE_NONE, FALSE);
}

void
add_font_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id)
{
  struct fontmap *fcur;
  char *font;

  getobj(obj, field, id, 0, NULL, &font);

  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur) {
    int match;
    match = ! g_strcmp0(font, fcur->fontalias);
    add_text_combo_item_to_cbox(list, iter, parent, column_id, -1, fcur->fontalias, TOGGLE_RADIO, match);
    fcur = fcur->next;
  }
}

void
add_font_style_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id_bold, int column_id_italic, struct objlist *obj, const char *field, int id)
{
  int style;

  getobj(obj, field, id, 0, NULL, &style);

  add_text_combo_item_to_cbox(list, iter, parent, column_id_bold, -1, _("Bold"), TOGGLE_CHECK, style & GRA_FONT_STYLE_BOLD);
  add_text_combo_item_to_cbox(list, iter, parent, column_id_italic, -1, _("Italic"), TOGGLE_CHECK, style & GRA_FONT_STYLE_ITALIC);
}

void
add_text_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, int enum_id, const char *title, enum TOGGLE_TYPE type, int active)
{
  GtkTreeIter locl_iter;

  if (iter == NULL) {
    iter = &locl_iter;
  }

  gtk_tree_store_append(list, iter, parent);
  gtk_tree_store_set(list, iter,
		     OBJECT_COLUMN_TYPE_STRING, title,
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, column_id,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, type != TOGGLE_NONE,
		     OBJECT_COLUMN_TYPE_TOGGLE_IS_RADIO, type == TOGGLE_RADIO,
		     OBJECT_COLUMN_TYPE_TOGGLE, active,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_ENUM, enum_id,
		     -1);
}

void
add_mark_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id)
{
  int j, type;
  GtkTreeIter locl_iter;

  if (iter == NULL) {
    iter = &locl_iter;
  }

  type = -1;
  getobj(obj, field, id, 0, NULL, &type);


  for (j = 0; j < MARK_TYPE_NUM; j++) {
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(NgraphApp.markpix[j],
					 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE);
    if (pixbuf) {
      char buf[64];

      gtk_tree_store_append(list, iter, parent);
      snprintf(buf, sizeof(buf), "%02d ", j);
      gtk_tree_store_set(list, iter,
			 OBJECT_COLUMN_TYPE_STRING, buf,
			 OBJECT_COLUMN_TYPE_PIXBUF, pixbuf,
			 OBJECT_COLUMN_TYPE_INT, column_id,
			 OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
			 OBJECT_COLUMN_TYPE_TOGGLE_IS_RADIO, TRUE,
			 OBJECT_COLUMN_TYPE_TOGGLE, j == type,
			 OBJECT_COLUMN_TYPE_ENUM, j,
			 -1);
      g_object_unref(pixbuf);
    }
  }
}

void
add_enum_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id)
{
  char **enum_array;
  int state, i;

  getobj(obj, field, id, 0, NULL, &state);
  enum_array = (char **) chkobjarglist(obj, field);
  if (enum_array == NULL) {
    return;
  }

  for (i = 0; enum_array[i] && enum_array[i][0]; i++) {
    add_text_combo_item_to_cbox(list, iter, parent, column_id, i, _(enum_array[i]), TOGGLE_RADIO, i == state);
  }
}

void
add_bool_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id, const char *title)
{
  GtkTreeIter locl_iter;
  int state;

  if (iter == NULL) {
    iter = &locl_iter;
  }

  getobj(obj, field, id, 0, NULL, &state);

  add_text_combo_item_to_cbox(list, iter, parent, column_id, -1, title, TOGGLE_CHECK, state);
}

void
add_line_style_item_to_cbox(GtkTreeStore *list, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id)
{
  GtkTreeIter iter;
  int i;
  char *str;

  sgetobjfield(obj, id, field, NULL, &str, FALSE, FALSE, FALSE);
  if (str == NULL) {
    return;
  }

  add_text_combo_item_to_cbox(list, &iter, parent, -1, -1, _("Line style"), TOGGLE_NONE, FALSE);
  for (i = 0; FwLineStyle[i].name; i++) {
    int active;
    active = ! g_strcmp0(str, FwLineStyle[i].list);
    add_text_combo_item_to_cbox(list, NULL, &iter, column_id, i, _(FwLineStyle[i].name), TOGGLE_RADIO, active);
  }
  g_free(str);
}

GtkCellEditable *
start_editing_obj(GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path,
		  const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags)
{
  return GTK_CELL_EDITABLE(create_object_cbox());
}

static GtkTreeViewColumn *
create_column(n_list_store *list, int i)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeModel *model;

  switch (list[i].type) {
  case G_TYPE_BOOLEAN:
    renderer = gtk_cell_renderer_toggle_new();
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						   "active", i, NULL);
    if (list[i].editable) {
      g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
      g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
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
    if (list[i].editable) {
      gtk_tree_view_column_set_expand(col, TRUE);
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
  case G_TYPE_PARAM:
    renderer = gtk_cell_renderer_combo_new();
    model = create_object_tree_model();
    g_object_set((GObject *) renderer,
		 "has-entry", FALSE,
		 "model", model,
		 "text-column", OBJECT_COLUMN_TYPE_STRING,
		 "editable", list[i].editable,
		 NULL);
    g_object_set_data(G_OBJECT(renderer), "user-data", &list[i]);
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						   "text", i, NULL);
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
    col = gtk_tree_view_column_new_with_attributes(_(list[i].title), renderer,
						     "text", i, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    if (list[i].ellipsize != PANGO_ELLIPSIZE_NONE || list[i].editable) {
      gtk_tree_view_column_set_expand(col, TRUE);
    }
  }
  return col;
}

void
tree_view_set_no_expand_column(GtkWidget *tview, const int *columns, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    GtkTreeViewColumn *col;
    col = gtk_tree_view_get_column(GTK_TREE_VIEW(tview), columns[i]);
    gtk_tree_view_column_set_expand(col, FALSE);
  }
}

void
list_store_set_align(GtkWidget *tview, int n, double align)
{
  GtkTreeViewColumn *col;
  GList *list;

  col = gtk_tree_view_get_column(GTK_TREE_VIEW(tview), n);
  if (col == NULL) {
    return;
  }

  list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
  if (list == NULL || list->data == NULL) {
    return;
  }

  g_object_set((GObject *) list->data, "xalign", (gfloat) align, NULL);

  g_list_free(list);
}

static GtkWidget *
create_tree_view(int n, n_list_store *list, int tree)
{
  GType *tarray;
  GtkTreeModel *lstore;
  GtkWidget *tview;
  GtkTreeViewColumn *col;
  GtkTreeSelection *sel;
  int i;

  if (n < 1 || list == NULL)
    return NULL;

  tarray = g_malloc(sizeof(*tarray) * n);
  if (tarray == NULL)
    return NULL;

  for (i = 0; i < n; i++) {
    if (list[i].type == G_TYPE_DOUBLE || list[i].type == G_TYPE_ENUM || list[i].type == G_TYPE_PARAM) {
      tarray[i] = G_TYPE_STRING;
    } else {
      tarray[i] = list[i].type;
    }
    list[i].edited_id = 0;
  }

  if (tree) {
    lstore = GTK_TREE_MODEL(gtk_tree_store_newv(n, tarray));
  } else {
    lstore = GTK_TREE_MODEL(gtk_list_store_newv(n, tarray));
  }
  g_free(tarray);

  tview = gtk_tree_view_new_with_model(lstore);

#if ! GTK_CHECK_VERSION(3, 14, 0)
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tview), TRUE);
#endif
  gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tview), TRUE);
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tview), GTK_TREE_VIEW_GRID_LINES_VERTICAL);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tview));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  for (i = 0; i < n; i++) {
    if (list[i].visible) {
      col = create_column(list, i);
      gtk_tree_view_column_set_visible(col, list[i].visible);
      gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);
    }
  }

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tview), n > 1);

  return tview;
}

void
list_store_set_sort_all(GtkWidget *tview)
{
  GList *list, *ptr;
  int i;

  list = gtk_tree_view_get_columns(GTK_TREE_VIEW(tview));

  if (list == NULL)
    return;

  for (ptr = list, i = 0; ptr; ptr = ptr->next, i++) {
    GtkTreeViewColumn *column;
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

void
tree_store_prepend(GtkWidget *w, GtkTreeIter *iter, GtkTreeIter *parent)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  gtk_tree_store_prepend(GTK_TREE_STORE(model), iter, parent);
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

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(w));
  state = gtk_tree_model_get_iter_first(model, &iter);
  while (state) {
    int val;
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

  gtk_tree_view_set_cursor(GTK_TREE_VIEW(w), path, NULL, FALSE); /* this line is commented out before (I forgot the reason). */

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
  GtkTreePath *first_path = NULL;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(client_data));
  selected = gtk_tree_selection_get_selected_rows(sel, &model);

  if (selected == NULL)
    return;

  for (data = g_list_last(selected); data; data = data->prev) {
    found = gtk_tree_model_get_iter(model, &iter, data->data);
    if (found) {
      gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }

    first_path = data->data;
  }

  if (first_path) {
    if (! gtk_tree_model_get_iter(model, &iter, first_path)) {
      gtk_tree_path_prev(first_path);
    }
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(client_data), first_path, NULL, FALSE);
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


int
tree_view_get_selected_row_int_from_path(GtkWidget *view, gchar *path, GtkTreeIter *iter, int col)
{
  GtkTreeModel *model;
  int sel;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  if (! gtk_tree_model_get_iter_from_string(model, iter, path)) {
    return -1;
  }


  list_store_select_iter(GTK_WIDGET(view), iter);
  gtk_tree_model_get(model, iter, col, &sel, -1);

  return sel;
}
