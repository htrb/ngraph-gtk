/*
 * $Id: gtk_combo.c,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
 */

#include "gtk_common.h"

#include <string.h>

#include "gtk_widget.h"
#include "gtk_combo.h"
#include "gtk_listview.h"

GtkWidget *
combo_box_create(void)
{
  GtkStringList *list;
  list = gtk_string_list_new (NULL);
  return gtk_drop_down_new(G_LIST_MODEL(list), NULL);
}

#define ENTRY_COMBO_MENU "entry_popover"

static void
select_item_cb(GtkListView *self, guint position, gpointer user_data)
{
  GtkStringList *list;
  GtkWidget *popover, *entry;
  GtkSingleSelection *model;
  const char *str;

  popover = GTK_WIDGET (user_data);
  entry = gtk_widget_get_parent(popover);
  model = GTK_SINGLE_SELECTION (gtk_list_view_get_model(self));
  list = GTK_STRING_LIST (gtk_single_selection_get_model(model));
  str = gtk_string_list_get_string (list, position);
  gtk_editable_set_text(GTK_EDITABLE(entry), str);
  gtk_popover_popdown (GTK_POPOVER (popover));
}

static GtkWidget *
create_popver(GtkEntry *entry)
{
  GtkWidget *popover, *menu;

  menu = listview_create(N_SELECTION_TYPE_SINGLE, NULL, NULL, NULL);
  gtk_list_view_set_single_click_activate (GTK_LIST_VIEW (menu), TRUE);

  popover = gtk_popover_new();
  gtk_popover_set_has_arrow (GTK_POPOVER (popover), FALSE);
  gtk_popover_set_child(GTK_POPOVER (popover), menu);
  gtk_widget_set_parent (popover, GTK_WIDGET(entry));
  gtk_widget_set_halign (popover, GTK_ALIGN_END);
  g_signal_connect(menu, "activate", G_CALLBACK(select_item_cb), popover);

  return popover;
}

static GtkStringList *
get_string_list(GtkWidget *entry)
{
  GtkWidget *list_view;
  GtkStringList *list;
  GtkSingleSelection *model;
  GtkWidget *popover;

  popover = g_object_get_data(G_OBJECT(entry), ENTRY_COMBO_MENU);
  list_view = gtk_popover_get_child (GTK_POPOVER (popover));
  model = GTK_SINGLE_SELECTION (gtk_list_view_get_model(GTK_LIST_VIEW (list_view)));
  list = GTK_STRING_LIST (gtk_single_selection_get_model(model));

  return list;
}

GtkWidget *
combo_box_entry_create(void)
{
  GtkEntry *entry;
  GtkWidget *popover;

  entry = GTK_ENTRY(gtk_entry_new());
  gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY, "pan-down-symbolic");
  gtk_entry_set_icon_activatable(entry, GTK_ENTRY_ICON_SECONDARY, FALSE);
  popover = create_popver(entry);

  g_signal_connect_swapped(entry, "icon-release", G_CALLBACK(gtk_popover_popup), popover);
  g_signal_connect_swapped(entry, "destroy", G_CALLBACK(gtk_widget_unparent), popover);
  g_object_set_data(G_OBJECT(entry), ENTRY_COMBO_MENU, popover);

  return GTK_WIDGET(entry);
}

void
combo_box_entry_set_text(GtkWidget *cbox, char *str)
{
  editable_set_init_text(cbox, str);
}

const char *
combo_box_entry_get_text(GtkWidget *cbox)
{
  return gtk_editable_get_text(GTK_EDITABLE(cbox));
}

void
combo_box_entry_set_width(GtkWidget *cbox, int width)
{
  gtk_editable_set_width_chars(GTK_EDITABLE(cbox), width);
}

void
combo_box_append_text(GtkWidget *cbox, char *str)
{
  GtkStringList *list;
  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_COMBO_BOX)) {
    GtkListStore  *list;
    GtkTreeIter iter;

    list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, 0, str, -1);
    return;
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_ENTRY)) {
    list = get_string_list(cbox);
    gtk_string_list_append (list, str);
    gtk_entry_set_icon_activatable(GTK_ENTRY(cbox), GTK_ENTRY_ICON_SECONDARY, TRUE);
    return;
  }

  list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  gtk_string_list_append (list, str);
}

int
combo_box_get_active(GtkWidget *cbox)
{
  guint active;
  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_COMBO_BOX)) {
    return gtk_combo_box_get_active(GTK_COMBO_BOX(cbox));
  }

  active = gtk_drop_down_get_selected (GTK_DROP_DOWN (cbox));
  if (active == GTK_INVALID_LIST_POSITION) {
    return -1;
  }
  return active;
}

char *
combo_box_get_active_text(GtkWidget *cbox)
{
  GtkStringList *list;
  int i;
  const char *str;

  i = combo_box_get_active(cbox);
  if (i < 0) {
    return NULL;
  }
  list = GTK_STRING_LIST (gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  str = gtk_string_list_get_string (list, i);
  return g_strdup (str);
}

void
combo_box_set_active(GtkWidget *cbox, int i)
{
  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_COMBO_BOX)) {
    gtk_combo_box_set_active(GTK_COMBO_BOX (cbox), i);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_ENTRY)) {
    GtkStringList *list;
    const char *str;
    list = get_string_list(cbox);
    str = gtk_string_list_get_string (list, i);
    if (str) {
      gtk_editable_set_text(GTK_EDITABLE(cbox), str);
    }
  } else {
    gtk_drop_down_set_selected (GTK_DROP_DOWN (cbox), i);
  }
}

void
combo_box_clear(GtkWidget *cbox)
{
  GtkStringList *list;
  int n;

  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_COMBO_BOX)) {
    GtkListStore *list;

    list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));
    gtk_list_store_clear(list);
    return;
  }
  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_ENTRY)) {
    gtk_entry_set_icon_activatable(GTK_ENTRY (cbox), GTK_ENTRY_ICON_SECONDARY, FALSE);
    list = get_string_list(cbox);
  } else {
    list = GTK_STRING_LIST (gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  }

  n = g_list_model_get_n_items (G_LIST_MODEL (list));
  gtk_string_list_splice (list, 0, n, NULL);
}

int
combo_box_get_num(GtkWidget *cbox)
{
  GListModel *model;

  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_COMBO_BOX)) {
    GtkTreeModel *model;

    model = gtk_combo_box_get_model(GTK_COMBO_BOX(cbox));
    return gtk_tree_model_iter_n_children(model, NULL);
  }
  if (G_TYPE_CHECK_INSTANCE_TYPE(cbox, GTK_TYPE_ENTRY)) {
    model = G_LIST_MODEL(get_string_list(cbox));
  } else {
    model = gtk_drop_down_get_model(GTK_DROP_DOWN(cbox));
  }
  return g_list_model_get_n_items(model);
}
