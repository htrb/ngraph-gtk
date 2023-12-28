#include "gtk_common.h"
#include "object.h"
#include "gtk_columnview.h"

GtkSelectionModel *
selection_model_create(enum N_SELECTION_TYPE selection_type, GListModel *model)
{
  GtkSelectionModel *selection;
  switch (selection_type) {
  case N_SELECTION_TYPE_MULTI:
    selection = GTK_SELECTION_MODEL(gtk_multi_selection_new (model));
    break;
  case N_SELECTION_TYPE_SINGLE:
    selection = GTK_SELECTION_MODEL(gtk_single_selection_new (model));
    break;
  case N_SELECTION_TYPE_NONE:
    selection = GTK_SELECTION_MODEL(gtk_no_selection_new (model));
    break;
  }
  return selection;
}


/* NInst Object */
G_DEFINE_TYPE(NInst, n_inst, G_TYPE_OBJECT)

enum {
  INST_PROP_NOBJ = 1,
  INST_PROP_ID,
  INST_PROP_NAME,
  INST_PROP_NUM_PROPERTIES
};

static void
n_inst_get_property (GObject    *object,
		     guint       property_id,
		     GValue     *value,
		     GParamSpec *pspec)
{
  NInst *self = N_INST (object);

  switch (property_id) {
  case INST_PROP_NOBJ:
    g_value_set_pointer (value, self->obj);
    break;
  case INST_PROP_ID:
    g_value_set_int (value, self->id);
    break;
  case INST_PROP_NAME:
    g_value_set_string (value, self->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
n_inst_finalize (GObject *object)
{
  NInst *self = N_INST (object);

  g_clear_pointer (&self->name, g_free);
  G_OBJECT_CLASS (n_inst_parent_class)->finalize (object);
}

static void
n_inst_class_init (NInstClass * klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_inst_finalize;
  object_class->get_property = n_inst_get_property;
  pspec = g_param_spec_pointer ("obj", NULL, NULL, G_PARAM_READABLE);
  g_object_class_install_property (object_class, INST_PROP_NOBJ, pspec);
  pspec = g_param_spec_int ("id", NULL, NULL, -1, G_MAXINT, 0, G_PARAM_READABLE);
  g_object_class_install_property (object_class, INST_PROP_ID, pspec);
  pspec = g_param_spec_string ("name", NULL, NULL, NULL, G_PARAM_READABLE);
  g_object_class_install_property (object_class, INST_PROP_NAME, pspec);
}

static void
n_inst_init (NInst * noop)
{
}

NInst*
n_inst_new (const gchar *name, int id, struct objlist *obj)
{
  NInst *nobj;

  nobj = g_object_new (N_TYPE_INST, NULL);
  nobj->name = g_strdup (name);
  nobj->obj = obj;
  nobj->id = id;

  return nobj;
}

void
n_inst_update (NInst *inst)
{
  g_object_notify (G_OBJECT (inst), "obj");
}

/* NData Object */
G_DEFINE_TYPE(NData, n_data, G_TYPE_OBJECT)

static void
n_data_finalize (GObject *object)
{
  G_OBJECT_CLASS (n_data_parent_class)->finalize (object);
}

static void
n_data_class_init (NDataClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_data_finalize;
}

static void
n_data_init (NData * noop)
{
}

NData*
n_data_new (int id, int line, double x, double y)
{
  NData *nobj;

  nobj = g_object_new (N_TYPE_DATA, NULL);
  nobj->id = id;
  nobj->line = line;
  nobj->x = x;
  nobj->y = y;
  nobj->data = 0;

  return nobj;
}

/* NPoint Object */
G_DEFINE_TYPE(NPoint, n_point, G_TYPE_OBJECT)

static void
n_point_finalize (GObject *object)
{
  G_OBJECT_CLASS (n_point_parent_class)->finalize (object);
}

static void
n_point_class_init (NPointClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_point_finalize;
}

static void
n_point_init (NPoint * noop)
{
}

NPoint*
n_point_new (int x, int y)
{
  NPoint *nobj;

  nobj = g_object_new (N_TYPE_POINT, NULL);
  nobj->x = x;
  nobj->y = y;

  return nobj;
}

/* NArray Object */
G_DEFINE_TYPE(NArray, n_array, G_TYPE_OBJECT)

static void
n_array_finalize (GObject *object)
{
  NArray *self = N_ARRAY (object);

  g_clear_pointer (&self->array, arrayfree2);
  G_OBJECT_CLASS (n_array_parent_class)->finalize (object);
}

static void
n_array_class_init (NArrayClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_array_finalize;
}

static void
n_array_init (NArray * noop)
{
}

NArray*
n_array_new (int line)
{
  NArray *nobj;

  nobj = g_object_new (N_TYPE_ARRAY, NULL);
  nobj->array = arraynew(sizeof(char *));
  nobj->line = line;

  return nobj;
}

/* NText Object */
G_DEFINE_TYPE(NText, n_text, G_TYPE_OBJECT)

static void
n_text_finalize (GObject *object)
{
  NText *self = N_TEXT (object);

  g_clear_pointer (&self->text, g_strfreev);
  G_OBJECT_CLASS (n_text_parent_class)->finalize (object);
}

static void
n_text_class_init (NTextClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_text_finalize;
}

static void
n_text_init (NText * noop)
{
}

NText*
n_text_new (gchar **text, guint attribute)
{
  NText *nobj;

  nobj = g_object_new (N_TYPE_TEXT, NULL);
  nobj->text = g_strdupv (text);
  nobj->size = (text) ? g_strv_length (text) : 0;
  nobj->attribute = attribute;
  return nobj;
}


/* GtkColumnView */

GtkWidget *
columnview_create(GType item_type, enum N_SELECTION_TYPE selection_type)
{
  GtkWidget *columnview;
  GtkSorter *sorter;
  GtkSelectionModel *selection;
  GListModel *model;

  columnview = gtk_column_view_new (NULL);
  gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (columnview), TRUE);

  model = G_LIST_MODEL(g_list_store_new (item_type));
  sorter = g_object_ref (gtk_column_view_get_sorter (GTK_COLUMN_VIEW(columnview)));
  model = G_LIST_MODEL(gtk_sort_list_model_new (model, sorter));
  gtk_column_view_set_enable_rubberband (GTK_COLUMN_VIEW (columnview), selection_type == N_SELECTION_TYPE_MULTI);
  gtk_column_view_set_reorderable (GTK_COLUMN_VIEW (columnview), FALSE);
  selection = selection_model_create(selection_type, model);
  gtk_column_view_set_model (GTK_COLUMN_VIEW (columnview), selection);
  return columnview;
}

GtkWidget *
columnview_tree_create(GListModel *root, GtkTreeListModelCreateModelFunc create_func, gpointer user_data)
{
  GtkWidget *columnview;
  GtkSelectionModel *selection;
  GtkTreeListModel *model;

  columnview = gtk_column_view_new (NULL);
  gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (columnview), TRUE);

  model = gtk_tree_list_model_new (G_LIST_MODEL (root), FALSE, FALSE, create_func, user_data, NULL);
  gtk_column_view_set_enable_rubberband (GTK_COLUMN_VIEW (columnview), TRUE);
  gtk_column_view_set_reorderable (GTK_COLUMN_VIEW (columnview), FALSE);

  selection = GTK_SELECTION_MODEL (gtk_multi_selection_new (G_LIST_MODEL (model)));
  gtk_column_view_set_model (GTK_COLUMN_VIEW (columnview), selection);
  return columnview;
}

GtkColumnViewColumn *
columnview_create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, GCallback sort, gpointer user_data, gboolean expand)
{
  GtkColumnViewColumn *column;
  GtkListItemFactory *factory;

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup), user_data);
  g_signal_connect (factory, "bind", G_CALLBACK (bind), user_data);

  column = gtk_column_view_column_new (header, factory);
  gtk_column_view_column_set_expand(column, expand);
  gtk_column_view_append_column (GTK_COLUMN_VIEW (columnview), column);

  if (sort) {
    GtkExpression *expression;
    GtkSorter *sorter;
    expression = gtk_cclosure_expression_new (G_TYPE_STRING, NULL, 0, NULL, G_CALLBACK (sort), user_data, NULL);
    sorter = GTK_SORTER(gtk_string_sorter_new(expression));
    gtk_column_view_column_set_sorter (column, sorter);
  }
  return column;
}

void
columnview_set_numeric_sorter(GtkColumnViewColumn *column, GType type, GCallback sort, gpointer user_data)
{
  GtkExpression *expression;
  GtkSorter *sorter;
  expression = gtk_cclosure_expression_new (type, NULL, 0, NULL, G_CALLBACK (sort), user_data, NULL);
  sorter = GTK_SORTER(gtk_numeric_sorter_new(expression));
  gtk_column_view_column_set_sorter (column, sorter);
}


GListStore *
columnview_get_list(GtkWidget *columnview)
{
  GtkSelectionModel *selection;
  GListModel *sort_model, *model;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  if (G_TYPE_CHECK_INSTANCE_TYPE(selection, GTK_TYPE_SINGLE_SELECTION)) {
    sort_model = gtk_single_selection_get_model (GTK_SINGLE_SELECTION (selection));
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(selection, GTK_TYPE_MULTI_SELECTION)) {
    sort_model = gtk_multi_selection_get_model (GTK_MULTI_SELECTION (selection));
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(selection, GTK_TYPE_NO_SELECTION)) {
    sort_model = gtk_no_selection_get_model (GTK_NO_SELECTION (selection));
  } else {
    return NULL;
  }
  model = gtk_sort_list_model_get_model (GTK_SORT_LIST_MODEL (sort_model));
  return G_LIST_STORE (model);
}

void
columnview_set_active(GtkWidget *columnview, int active, gboolean scroll)
{
  if (scroll) {
    gtk_column_view_scroll_to (GTK_COLUMN_VIEW (columnview), active, NULL, GTK_LIST_SCROLL_SELECT | GTK_LIST_SCROLL_FOCUS, NULL);
  } else {
    GtkSelectionModel *selection;

    selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
    gtk_selection_model_select_item (selection, active, TRUE);
  }
}

int
columnview_get_active(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  return selection_model_get_selected(selection);
}

GObject *
columnview_get_active_item(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  return gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
}

NInst *
list_store_append_n_inst(GListStore *store, const gchar *name, int id, struct objlist *obj)
{
  NInst *item;
  item = n_inst_new(name, id, obj);
  g_list_store_append (store, item);
  g_object_unref(item);
  return item;
}

NInst *
columnview_append_n_inst(GtkWidget *columnview, const gchar *name, int id, struct objlist *obj)
{
  GListStore *store;
  store = columnview_get_list(columnview);
  if (store) {
    return list_store_append_n_inst(store, name, id, obj);
  }
  return NULL;
}

NData *
list_store_append_n_data(GListStore *store, int id, int line, double x, double y)
{
  NData *item;
  item = n_data_new(id, line, x, y);
  g_list_store_append (store, item);
  g_object_unref(item);
  return item;
}

NPoint *
list_store_append_n_point(GListStore *store, int x, int y)
{
  NPoint *item;
  item = n_point_new(x, y);
  g_list_store_append (store, item);
  g_object_unref(item);
  return item;
}

NText *
list_store_append_n_text(GListStore *store, gchar **text, guint attribute)
{
  NText *item;
  item = n_text_new(text, attribute);
  g_list_store_append (store, item);
  g_object_unref(item);
  return item;
}

void
columnview_clear(GtkWidget *columnview)
{
  GListStore *store;
  store = columnview_get_list(columnview);
  g_list_store_remove_all (store);
}

void
columnview_select_all(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  gtk_selection_model_select_all (selection);
}

void
columnview_unselect_all(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  gtk_selection_model_unselect_all (selection);
}

void
columnview_select(GtkWidget *columnview, int i)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  gtk_selection_model_unselect_all (selection);
  gtk_selection_model_select_item (selection, i, FALSE);
}

void
columnview_remove_selected(GtkWidget *columnview)
{
  GtkSelectionModel *selection;
  GListStore *list;
  int i, n;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  list = columnview_get_list (columnview);
  n = g_list_model_get_n_items (G_LIST_MODEL (list));
  for (i = n - 1; i >= 0; i--) {
    GObject *ni;
    guint idx;
    if (! gtk_selection_model_is_selected (selection, i)) {
      continue;
    }
    ni = g_list_model_get_item (G_LIST_MODEL (selection), i);
    g_list_store_find (list, ni, &idx);
    g_list_store_remove (list, idx);
    g_object_unref (ni);
  }
  /* to set sensitivity of the delete button */
  gtk_selection_model_select_all (selection);
  gtk_selection_model_unselect_all (selection);
}

int
selection_model_get_selected(GtkSelectionModel *model)
{
  int i, n;
  if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_SINGLE_SELECTION)) {
    guint sel;
    sel = gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (model));
    if (sel == GTK_INVALID_LIST_POSITION) {
      return -1;
    }
    return sel;
  }

  n = g_list_model_get_n_items (G_LIST_MODEL (model));
  for (i = 0; i < n; i++) {
    if (gtk_selection_model_is_selected (model, i)) {
      return i;
    }
  }
  return -1;
}

int
columnview_get_n_items(GtkWidget *view)
{
  GtkSelectionModel *model;
  model = gtk_column_view_get_model (GTK_COLUMN_VIEW (view));
  return g_list_model_get_n_items (G_LIST_MODEL (model));
}
