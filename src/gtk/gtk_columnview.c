#include "gtk_common.h"
#include "object.h"
#include "gtk_columnview.h"

G_DEFINE_TYPE(NgraphInst, ngraph_inst, G_TYPE_OBJECT)

static void
ngraph_inst_finalize (GObject *object)
{
  NgraphInst *self = NGRAPH_INST (object);

  g_free (self->name);
  G_OBJECT_CLASS (ngraph_inst_parent_class)->finalize (object);
}

static void
ngraph_inst_class_init (NgraphInstClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ngraph_inst_finalize;
}

static void
ngraph_inst_init (NgraphInst * noop)
{
}

NgraphInst*
ngraph_inst_new (const gchar *name, int id, struct objlist *obj)
{
  NgraphInst *nobj;

  nobj = g_object_new (NGRAPH_TYPE_INST, NULL);
  nobj->name = g_strdup (name);
  nobj->obj = obj;
  nobj->id = id;
  nobj->x = nobj->y = 0.0;

  return nobj;
}

GtkWidget *
columnview_create(gboolean multi)
{
  GtkWidget *columnview;
  GtkSorter *sorter;
  GtkSelectionModel *selection;
  GListModel *model;

  columnview = gtk_column_view_new (NULL);
  gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (columnview), TRUE);

  model = G_LIST_MODEL(g_list_store_new (NGRAPH_TYPE_INST));
  sorter = g_object_ref (gtk_column_view_get_sorter (GTK_COLUMN_VIEW(columnview)));
  model = G_LIST_MODEL(gtk_sort_list_model_new (model, sorter));
  gtk_column_view_set_enable_rubberband (GTK_COLUMN_VIEW (columnview), multi);
  if (multi) {
    selection = GTK_SELECTION_MODEL(gtk_multi_selection_new (G_LIST_MODEL (model)));
  } else {
    selection = GTK_SELECTION_MODEL(gtk_single_selection_new (G_LIST_MODEL (model)));
  }
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
columnview_set_active(GtkWidget *columnview, int active)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  gtk_selection_model_select_item (selection, active, TRUE);
}

int
columnview_get_active(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  return gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (selection));
}

NgraphInst *
columnview_get_active_item(GtkWidget *columnview)
{
  GtkSelectionModel *selection;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  return gtk_single_selection_get_selected_item (GTK_SINGLE_SELECTION (selection));
}

NgraphInst *
list_store_append_ngraph_inst(GListStore *store, const gchar *name, int id, struct objlist *obj)
{
  NgraphInst *item;
  item = ngraph_inst_new(name, id, obj);
  g_list_store_append (store, item);
  g_object_unref(item);
  return item;
}

NgraphInst *
columnview_append_ngraph_inst(GtkWidget *columnview, const gchar *name, int id, struct objlist *obj)
{
  GListStore *store;
  store = columnview_get_list(columnview);
  if (store) {
    return list_store_append_ngraph_inst(store, name, id, obj);
  }
  return NULL;
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
    NgraphInst *ni;
    guint idx;
    if (! gtk_selection_model_is_selected (selection, i)) {
      continue;
    }
    ni = g_list_model_get_item (G_LIST_MODEL (selection), i);
    g_list_store_find (list, ni, &idx);
    g_list_store_remove (list, idx);
  }
}
