#include "gtk_common.h"
#include "gtk_presettings.h"

struct presetting_widgets
{
  GtkWidget *stroke, *stroke_icon, *fill, *fill_icon;
  GtkWidget *line_width;
  GtkWidget *fill_rule, *path_type;
  GtkWidget *join_type, *join_bevel, *join_round, *join_miter;
  GtkWidget *arrow_type, *arrow_none, *arrow_begin, *arrow_end, *arrow_both;
};

static struct presetting_widgets Widgets = {NULL};

static void
JoinTypeMiterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_miter);
}

static void
JoinTypeRoundAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_round);
}

static void
JoinTypeBevelAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_bevel);
}

static void
ArrowTypeNoneAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_none);
}

static void
ArrowTypeBeginAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_begin);
}

static void
ArrowTypeEndAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_end);
}

static void
ArrowTypeBothAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_both);
}

static GActionEntry ToolMenuEntries[] =
{
  { "JoinTypeMiterAction", JoinTypeMiterAction_activated, NULL, NULL, NULL },
  { "JoinTypeRoundAction", JoinTypeRoundAction_activated, NULL, NULL, NULL },
  { "JoinTypeBevelAction", JoinTypeBevelAction_activated, NULL, NULL, NULL },
  { "ArrowTypeNoneAction",  ArrowTypeNoneAction_activated, NULL, NULL, NULL },
  { "ArrowTypeBeginAction", ArrowTypeBeginAction_activated, NULL, NULL, NULL },
  { "ArrowTypeEndAction",   ArrowTypeEndAction_activated, NULL, NULL, NULL },
  { "ArrowTypeBothAction",  ArrowTypeBothAction_activated, NULL, NULL, NULL },
};

static void
create_images(struct presetting_widgets *widgets)
{
  widgets->fill_icon = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/fill.png");
  g_object_ref(widgets->fill_icon);
  widgets->stroke_icon = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/stroke.png");
  g_object_ref(widgets->stroke_icon);
  widgets->arrow_both = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/arrow_both.png");
  g_object_ref(widgets->arrow_both);
  widgets->arrow_begin = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/arrow_begin.png");
  g_object_ref(widgets->arrow_begin);
  widgets->arrow_end = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/arrow_end.png");
  g_object_ref(widgets->arrow_end);
  widgets->arrow_none = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/arrow_none.png");
  g_object_ref(widgets->arrow_none);
  widgets->join_bevel = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_bevel.png");
  g_object_ref(widgets->join_bevel);
  widgets->join_round = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_round.png");
  g_object_ref(widgets->join_round);
  widgets->join_miter = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_miter.png");
  g_object_ref(widgets->join_miter);
}

void
add_setting_panel(GtkWidget *vbox, GtkApplication *app)
{
  GtkWidget *w, *box, *label;
  GtkBuilder *builder;
  GMenuModel *menu;

  g_action_map_add_action_entries(G_ACTION_MAP(app), ToolMenuEntries, G_N_ELEMENTS(ToolMenuEntries), app);
  create_images(&Widgets);

  builder = gtk_builder_new_from_resource(RESOURCE_PATH "/gtk/menus-tool.ui");
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(w), Widgets.stroke_icon);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.stroke = w;

  w = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(w), Widgets.fill_icon);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.fill = w;

  label = gtk_label_new_with_mnemonic(_("_Width:"));
  w = gtk_spin_button_new(NULL, 1, 0);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.line_width = w;

  w = gtk_spin_button_new(NULL, 1, 0);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  
  w = gtk_menu_button_new();
  gtk_button_set_image(GTK_BUTTON(w), Widgets.arrow_none);
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "arrow-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.arrow_type = w;

  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "fill-rule-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.fill_rule = w;

  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "path-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.path_type = w;

  w = gtk_menu_button_new();
  gtk_button_set_image(GTK_BUTTON(w), Widgets.join_miter);
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "join-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.join_type = w;

  gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

  g_object_unref(builder);
}
