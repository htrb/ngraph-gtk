/*
 * $Id: gtk_combo.c,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
 */

#include "gtk_common.h"

#include <string.h>

#include "gtk_widget.h"
#include "gtk_combo.h"

static void
set_model(GtkComboBox *cbox, int renderer)
{
  GtkListStore  *list;

  list = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_combo_box_set_model(cbox, GTK_TREE_MODEL(list));

  if (renderer) {
    GtkCellRenderer *rend_s;
    rend_s = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend_s, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend_s, "text", 0);
  }

  /*
  rend_n = gtk_cell_renderer_text_new();
  g_object_set_data((GObject *) rend_n, "visible", FALSE);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend_n, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend_s, "text", 1);
  */
}

GtkWidget *
combo_box_create(void)
{
  GtkComboBox *cbox;

  cbox = GTK_COMBO_BOX(gtk_combo_box_new());
  set_model(cbox, TRUE);
  return GTK_WIDGET(cbox);
}

GtkWidget *
combo_box_entry_create(void)
{
  GtkComboBox *cbox;

  cbox = GTK_COMBO_BOX(gtk_combo_box_new_with_entry());
  set_model(cbox, FALSE);
  gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(cbox), 0);
  gtk_entry_set_activates_default(GTK_ENTRY(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox))), TRUE);
  return GTK_WIDGET(cbox);
}

void
combo_box_entry_set_text(GtkWidget *cbox, char *str)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  editable_set_init_text(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox)), str);
#else
  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cbox))), str);
#endif
}

const char *
combo_box_entry_get_text(GtkWidget *cbox)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  return gtk_editable_get_text(GTK_EDITABLE(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox))));
#else
  return gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cbox))));
#endif
}

void
combo_box_entry_set_width(GtkWidget *cbox, int width)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_editable_set_width_chars(GTK_EDITABLE(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox))), width);
#else
  gtk_entry_set_width_chars(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(cbox))), width);
#endif
}

void
combo_box_append_text(GtkWidget *cbox, char *str)
{
  GtkListStore  *list;
  GtkTreeIter iter;

  list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  gtk_list_store_append(list, &iter);
  gtk_list_store_set(list, &iter, 0, str, -1);
}

int
combo_box_get_active(GtkWidget *cbox)
{
  return gtk_combo_box_get_active(GTK_COMBO_BOX(cbox));
}

char *
combo_box_get_active_text(GtkWidget *cbox)
{
  GtkTreeIter iter;
  gboolean ret;
  GtkTreeModel *model;
  char *str;

  ret = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cbox), &iter);

  if (! ret)
    return NULL;

  model = gtk_combo_box_get_model(GTK_COMBO_BOX(cbox));
  gtk_tree_model_get(model, &iter, 0, &str, -1);

  return str;
}

void
combo_box_set_active(GtkWidget *cbox, int i)
{
  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), i);
}

void
combo_box_clear(GtkWidget *cbox)
{
  GtkListStore *list;

  list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));
  gtk_list_store_clear(list);
}

int
combo_box_get_num(GtkWidget *cbox)
{
  GtkTreeModel *model;

  model = gtk_combo_box_get_model(GTK_COMBO_BOX(cbox));
  return gtk_tree_model_iter_n_children(model, NULL);
}
