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

  GtkStringList *list;
  list = gtk_string_list_new (NULL);
  return gtk_drop_down_new(G_LIST_MODEL(list), NULL);
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
  editable_set_init_text(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox)), str);
}

const char *
combo_box_entry_get_text(GtkWidget *cbox)
{
  return gtk_editable_get_text(GTK_EDITABLE(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox))));
}

void
combo_box_entry_set_width(GtkWidget *cbox, int width)
{
  gtk_editable_set_width_chars(GTK_EDITABLE(gtk_combo_box_get_child(GTK_COMBO_BOX(cbox))), width);
}

void
combo_box_append_text(GtkWidget *cbox, char *str)
{
  GtkStringList *list;

  list = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  gtk_string_list_append (list, str);
}

int
combo_box_get_active(GtkWidget *cbox)
{
  return gtk_drop_down_get_selected (GTK_DROP_DOWN (cbox));
}

char *
combo_box_get_active_text(GtkWidget *cbox)
{
  GtkStringList *list;
  int i;
  const char *str;

  i = combo_box_get_active(cbox);
  list = GTK_STRING_LIST (gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  str = gtk_string_list_get_string (list, i);
  return g_strdup (str);
}

void
combo_box_set_active(GtkWidget *cbox, int i)
{
  gtk_drop_down_set_selected (GTK_DROP_DOWN (cbox), i);
}

void
combo_box_clear(GtkWidget *cbox)
{
  GtkStringList *list;
  int n;

  list = GTK_STRING_LIST (gtk_drop_down_get_model(GTK_DROP_DOWN (cbox)));
  n = g_list_model_get_n_items (G_LIST_MODEL (list));
  gtk_string_list_splice (list, 0, n, NULL);
}

int
combo_box_get_num(GtkWidget *cbox)
{
  GListModel *model;

  model = gtk_drop_down_get_model(GTK_DROP_DOWN(cbox));
  return g_list_model_get_n_items(model);
}
