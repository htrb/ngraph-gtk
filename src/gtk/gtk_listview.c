#include "gtk_columnview.h"
#include "gtk_listview.h"

static void
setup_listitem_cb (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;

  label = gtk_label_new (NULL);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_list_item_set_child (list_item, label);
}

static void
bind_listitem_cb (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;
  gpointer item;
  const char *string;

  label = gtk_list_item_get_child (list_item);
  item = gtk_list_item_get_item (list_item);
  string = gtk_string_object_get_string (GTK_STRING_OBJECT (item));
  gtk_label_set_text(GTK_LABEL(label), string);
}

GtkWidget *
listview_create(enum N_SELECTION_TYPE selection_type, GCallback setup, GCallback bind, gpointer user_data)
{
  GtkWidget *listview;
  GtkStringList *list;
  GtkListItemFactory *factory;
  GtkSelectionModel *selection;

  list = gtk_string_list_new (NULL);

  factory = gtk_signal_list_item_factory_new();
  if (setup == NULL) {
    setup = G_CALLBACK (setup_listitem_cb);
  }
  if (bind == NULL) {
    bind = G_CALLBACK (bind_listitem_cb);
  }
  g_signal_connect (factory, "setup", setup, user_data);
  g_signal_connect (factory, "bind", bind, user_data);

  selection = selection_model_create(selection_type, G_LIST_MODEL(list));
  listview = gtk_list_view_new (selection, factory);
  gtk_list_view_set_enable_rubberband (GTK_LIST_VIEW (listview), selection_type == N_SELECTION_TYPE_MULTI);
  return listview;
}

GtkStringList *
listview_get_string_list(GtkWidget *listview)
{
  GtkSelectionModel *sel;
  GListModel *list = NULL;
  sel = gtk_list_view_get_model (GTK_LIST_VIEW (listview));
  if (G_TYPE_CHECK_INSTANCE_TYPE(sel, GTK_TYPE_SINGLE_SELECTION)) {
    list = gtk_single_selection_get_model (GTK_SINGLE_SELECTION (sel));
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(sel, GTK_TYPE_MULTI_SELECTION)) {
    list = gtk_multi_selection_get_model (GTK_MULTI_SELECTION (sel));
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(sel, GTK_TYPE_NO_SELECTION)) {
    list = gtk_no_selection_get_model (GTK_NO_SELECTION (sel));
  }
  return GTK_STRING_LIST (list);
}

static void
setup_header_cb (GtkListItemFactory *factory, GtkListHeader *list_item, gpointer user_data)
{
  GtkWidget *label;
  const char *string = (const char *) user_data;

  label = gtk_label_new (string);
  gtk_list_header_set_child (list_item, label);
}

void
listview_set_header(GtkWidget *listview, char *header)
{
  GtkListItemFactory *factory;

  factory = gtk_signal_list_item_factory_new();
  gtk_list_view_set_header_factory (GTK_LIST_VIEW (listview), factory);
  g_signal_connect (factory, "setup", G_CALLBACK (setup_header_cb), header);
}

void
listview_select_all(GtkWidget *listview)
{
  GtkSelectionModel *selection;

  selection = gtk_list_view_get_model (GTK_LIST_VIEW (listview));
  gtk_selection_model_select_all (selection);
}
