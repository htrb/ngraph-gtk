#include "gtk_common.h"
#include "gtk_presettings.h"
#include "gtk_combo.h"
#include "gtk_widget.h"
#include "object.h"
#include "gra.h"
#include "ogra2cairo.h"
#include "odraw.h"

struct presetting_widgets
{
  GtkWidget *stroke, *stroke_icon, *fill, *fill_icon;
  GtkWidget *line_width;
  GtkWidget *color1, *color2;
  GtkWidget *fill_rule, *path_type;
  GtkWidget *join_type, *join_bevel, *join_round, *join_miter;
  GtkWidget *arrow_type, *arrow_none, *arrow_begin, *arrow_end, *arrow_both;
  GtkWidget *lw002, *lw004, *lw008, *lw016, *lw032, *lw064, *lw128;
  GtkWidget *font, *bold, *italic, *pt;
  enum JOIN_TYPE join;
  enum ARROW_POSITION_TYPE arrow;
  int lw;
};

static struct presetting_widgets Widgets = {NULL};

static void
JoinTypeMiterAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_miter);
  Widgets.join = JOIN_TYPE_MITER;
}

static void
JoinTypeRoundAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_round);
  Widgets.join = JOIN_TYPE_ROUND;
}

static void
JoinTypeBevelAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_bevel);
  Widgets.join = JOIN_TYPE_BEVEL;
}

static void
ArrowTypeNoneAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_none);
  Widgets.arrow = ARROW_POSITION_NONE;
}

static void
ArrowTypeBeginAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_begin);
  Widgets.arrow = ARROW_POSITION_BEGIN;
}

static void
ArrowTypeEndAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_end);
  Widgets.arrow = ARROW_POSITION_END;
}

static void
ArrowTypeBothAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_both);
  Widgets.arrow = ARROW_POSITION_BOTH;
}

static void
LineWidth002Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw002);
  Widgets.lw = 20;
}

static void
LineWidth004Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw004);
  Widgets.lw = 40;
}

static void
LineWidth008Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw008);
  Widgets.lw = 80;
}

static void
LineWidth016Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw016);
  Widgets.lw = 160;
}

static void
LineWidth032Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw032);
  Widgets.lw = 320;
}

static void
LineWidth064Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw064);
  Widgets.lw = 640;
}


static void
LineWidth128Action_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.line_width), Widgets.lw128);
  Widgets.lw = 1280;
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
  { "LineWidth002Action",  LineWidth002Action_activated, NULL, NULL, NULL },
  { "LineWidth004Action",  LineWidth004Action_activated, NULL, NULL, NULL },
  { "LineWidth008Action",  LineWidth008Action_activated, NULL, NULL, NULL },
  { "LineWidth016Action",  LineWidth016Action_activated, NULL, NULL, NULL },
  { "LineWidth032Action",  LineWidth032Action_activated, NULL, NULL, NULL },
  { "LineWidth064Action",  LineWidth064Action_activated, NULL, NULL, NULL },
  { "LineWidth128Action",  LineWidth128Action_activated, NULL, NULL, NULL },
};

static void
create_images(struct presetting_widgets *widgets)
{
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
  widgets->lw002 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_002.png");
  g_object_ref(widgets->lw002);
  widgets->lw004 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_004.png");
  g_object_ref(widgets->lw004);
  widgets->lw008 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_008.png");
  g_object_ref(widgets->lw008);
  widgets->lw016 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_016.png");
  g_object_ref(widgets->lw016);
  widgets->lw032 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_032.png");
  g_object_ref(widgets->lw032);
  widgets->lw064 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_064.png");
  g_object_ref(widgets->lw064);
  widgets->lw128 = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/linewidth_128.png");
  g_object_ref(widgets->lw128);
}

static void
set_rgba(GtkWidget *cbutton, int *r, int *g, int *b, int *a)
{
  GdkRGBA color;
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(cbutton), &color);
  *r = color.red * 255;
  *g = color.green * 255;
  *b = color.blue * 255;
  *a = color.alpha * 255;
}

static void
set_color(struct objlist *obj, int id, int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2)
{
    putobj(obj, "stroke_R", id, &r1);
    putobj(obj, "stroke_G", id, &g1);
    putobj(obj, "stroke_B", id, &b1);
    putobj(obj, "stroke_A", id, &a1);
    putobj(obj, "fill_R", id, &r2);
    putobj(obj, "fill_G", id, &g2);
    putobj(obj, "fill_B", id, &b2);
    putobj(obj, "fill_A", id, &a2);
}

static void
set_text_obj(struct objlist *obj, int id)
{
  int style, r, g, b, a, bold, italic, pt;
  char *fontalias;
  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.bold));
  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.italic));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }
  if (italic) {
    style |= GRA_FONT_STYLE_ITALIC;
  }
  putobj(obj, "style", id, &style);
  fontalias = combo_box_get_active_text(Widgets.font);
  if (fontalias) {
    putobj(obj, "font", id, fontalias);
  }
  set_rgba(Widgets.color1, &r, &g, &b, &a);
  putobj(obj, "R", id, &r);
  putobj(obj, "G", id, &g);
  putobj(obj, "B", id, &b);
  putobj(obj, "A", id, &a);
  pt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Widgets.pt)) * 100;
  putobj(obj, "pt", id, &pt);
}

void
presetting_set_obj_field(struct objlist *obj, int id)
{
  const char *name;
  int ival, r1, g1, b1, a1, r2, g2, b2, a2, stroke, fill;

  if (obj == NULL) {
    return;
  }
  name = chkobjectname(obj);
  if (name == NULL) {
    return;
  }

  set_rgba(Widgets.color1, &r1, &g1, &b1, &a1);
  set_rgba(Widgets.color2, &r2, &g2, &b2, &a2);
  stroke = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.stroke));
  fill = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.fill));

  if (strcmp(name, "axis") == 0) {
  } else if (strcmp(name, "path") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    ival = Widgets.join;
    putobj(obj, "join", id, &ival);
    ival = Widgets.arrow;
    putobj(obj, "arrow", id, &ival);
    ival = Widgets.lw;
    putobj(obj, "width", id, &ival);
    set_color(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
  } else if (strcmp(name, "rectangle") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    ival = Widgets.lw;
    putobj(obj, "width", id, &ival);
    set_color(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
  } else if (strcmp(name, "arc") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    ival = Widgets.join;
    putobj(obj, "join", id, &ival);
    ival = Widgets.lw;
    putobj(obj, "width", id, &ival);
    set_color(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
  } else if (strcmp(name, "mark") == 0) {
    ival = Widgets.lw;
    putobj(obj, "width", id, &ival);
    putobj(obj, "R", id, &r1);
    putobj(obj, "G", id, &g1);
    putobj(obj, "B", id, &b1);
    putobj(obj, "A", id, &a1);
    putobj(obj, "R2", id, &r2);
    putobj(obj, "G2", id, &g2);
    putobj(obj, "B2", id, &b2);
    putobj(obj, "A2", id, &a2);
  } else if (strcmp(name, "text") == 0) {
    set_text_obj(obj, id);
  }
}

static void
set_font_family(GtkWidget *cbox)
{
  struct fontmap *fcur;
  combo_box_clear(cbox);
  fcur = Gra2cairoConf->fontmap_list_root;
  while (fcur) {
    combo_box_append_text(cbox, fcur->fontalias);
    fcur = fcur->next;
  }
  combo_box_set_active(cbox, 1);
}

static GtkWidget *
create_toggle_button(GtkWidget *box, GtkWidget *img, int state)
{
  GtkWidget *w;
  w = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(w), img);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), state);
  return w;
}

GtkWidget *
add_setting_panel(GtkApplication *app)
{
  GtkWidget *w, *box, *img;
  GtkBuilder *builder;
  GMenuModel *menu;
  GdkRGBA color;

  g_action_map_add_action_entries(G_ACTION_MAP(app), ToolMenuEntries, G_N_ELEMENTS(ToolMenuEntries), app);
  create_images(&Widgets);

  builder = gtk_builder_new_from_resource(RESOURCE_PATH "/gtk/menus-tool.ui");
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = combo_box_create();
  set_font_family(w);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.font = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, FALSE, FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), DEFAULT_FONT_PT / 100.0);
  gtk_entry_set_width_chars(GTK_ENTRY(w), 5);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.pt = w;

  img = gtk_image_new_from_icon_name("format-text-bold-symbolic", GTK_ICON_SIZE_BUTTON);
  Widgets.bold = create_toggle_button(box, img, FALSE);

  img = gtk_image_new_from_icon_name("format-text-italic-symbolic", GTK_ICON_SIZE_BUTTON);
  Widgets.italic = create_toggle_button(box, img, FALSE);

  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/stroke.png");
  Widgets.stroke = create_toggle_button(box, img, TRUE);

  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/fill.png");
  Widgets.fill = create_toggle_button(box, img, FALSE);

  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "linewidth-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_widget_set_tooltip_text(w, _("Line Width"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.line_width = w;
  LineWidth004Action_activated(NULL, NULL, NULL);

  w = create_color_button(NULL);
  color.red = color.green = color.blue = 0;
  color.alpha = 1;
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(w), &color);
  gtk_widget_set_tooltip_text(w, _("Stroke color"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.color1 = w;

  w = create_color_button(NULL);
  color.red = color.green = color.blue = 1;
  color.alpha = 1;
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(w), &color);
  gtk_widget_set_tooltip_text(w, _("Fill color"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.color2 = w;
  
  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "arrow-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_widget_set_tooltip_text(w, _("Arrow"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.arrow_type = w;
  ArrowTypeNoneAction_activated(NULL, NULL, NULL);

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
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, "join-type-menu"));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_widget_set_tooltip_text(w, _("Join"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.join_type = w;
  JoinTypeMiterAction_activated(NULL, NULL, NULL);

  g_object_unref(builder);
  return box;
}
