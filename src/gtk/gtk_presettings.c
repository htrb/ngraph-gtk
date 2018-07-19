#include "gtk_common.h"

void
add_setting_panel(GtkWidget *vbox)
{
  GtkWidget *w, *box, *label, *img;
  GtkBuilder *builder;
  GMenuModel *menu;

  builder = gtk_builder_new_from_resource(RESOURCE_PATH "/gtk/menus-tool.ui");
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = gtk_toggle_button_new();
  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/stroke.png");
  gtk_container_add(GTK_CONTAINER(w), img);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_toggle_button_new();
  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/fill.png");
  gtk_container_add(GTK_CONTAINER(w), img);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic(_("_Width:"));
  w = gtk_spin_button_new(NULL, 1, 0);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_spin_button_new(NULL, 1, 0);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  
  w = gtk_menu_button_new();
  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/arrow_both.png");
  gtk_container_add(GTK_CONTAINER(w), img);
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "arrow-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "fill-rule-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "path-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_menu_button_new();
  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_bevel.png");
  gtk_container_add(GTK_CONTAINER(w), img);
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "join-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

  g_object_unref(builder);
}
