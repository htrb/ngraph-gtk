#include "gtk_common.h"
#include "gtk_presettings.h"
#include "gtk_combo.h"
#include "gtk_widget.h"
#include "object.h"
#include "gra.h"
#include "ogra2cairo.h"
#include "odraw.h"
#include "x11menu.h"
#include "x11dialg.h"

#define SETTING_PANEL_MARGIN 4
#define LINE_WIDTH_ICON_NUM 7
#define LINE_STYLE_ICON_NUM 7

#define DEFAULT_JOIN_TYPE JOIN_TYPE_MITER
#define DEFAULT_JOIN_STR  "'miter'"

struct presetting_widgets
{
  GtkWidget *stroke, *fill;
  GtkWidget *line_width, *line_style;
  GtkWidget *color1, *color2;
  GtkWidget *path_type;
  GtkWidget *join_type, *join_icon[3];
  GtkWidget *arrow_type, *arrow_none, *arrow_begin, *arrow_end, *arrow_both;
  GtkWidget *font, *bold, *italic, *pt;
  GtkWidget *mark, *mark_size;
  enum JOIN_TYPE join;
  enum ARROW_POSITION_TYPE arrow;
  int lw;
};

static struct presetting_widgets Widgets = {NULL};

static void
JoinTypeAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  const char *state;
  int i;
  state = g_variant_get_string(parameter, NULL);
  for (i = 0; joinchar[i]; i++) {
    if (g_strcmp0(state, joinchar[i]) == 0) {
      Widgets.join = i;
      gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_icon[i]);
      break;
    }
  }
  g_simple_action_set_state(action, parameter);
}

static void
ArrowTypeNoneAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_none);
  Widgets.arrow = ARROW_POSITION_NONE;
  g_simple_action_set_state(action, parameter);
}

static void
ArrowTypeBeginAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_begin);
  Widgets.arrow = ARROW_POSITION_BEGIN;
  g_simple_action_set_state(action, parameter);
}

static void
ArrowTypeEndAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_end);
  Widgets.arrow = ARROW_POSITION_END;
  g_simple_action_set_state(action, parameter);
}

static void
ArrowTypeBothAction_activated(GSimpleAction *action, GVariant *parameter, gpointer app)
{
  gtk_button_set_image(GTK_BUTTON(Widgets.arrow_type), Widgets.arrow_both);
  Widgets.arrow = ARROW_POSITION_BOTH;
  g_simple_action_set_state(action, parameter);
}


static GActionEntry ToolMenuEntries[] = {
  { "JoinTypeAction",  NULL, "s", DEFAULT_JOIN_STR, JoinTypeAction_activated},
  { "JoinTypeAction",  NULL, "s", DEFAULT_JOIN_STR, JoinTypeAction_activated},
  { "JoinTypeAction",  NULL, "s", DEFAULT_JOIN_STR, JoinTypeAction_activated},
  { "ArrowTypeNoneAction",  NULL, NULL, "true",  ArrowTypeNoneAction_activated},
  { "ArrowTypeBeginAction", NULL, NULL, "false", ArrowTypeBeginAction_activated},
  { "ArrowTypeEndAction",   NULL, NULL, "false", ArrowTypeEndAction_activated},
  { "ArrowTypeBothAction",  NULL, NULL, "false", ArrowTypeBothAction_activated},
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
  widgets->join_icon[JOIN_TYPE_BEVEL] = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_bevel.png");
  g_object_ref(widgets->join_icon[JOIN_TYPE_BEVEL]);
  widgets->join_icon[JOIN_TYPE_ROUND] = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_round.png");
  g_object_ref(widgets->join_icon[JOIN_TYPE_ROUND]);
  widgets->join_icon[JOIN_TYPE_MITER] = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/join_miter.png");
  g_object_ref(widgets->join_icon[JOIN_TYPE_MITER]);
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
get_rgba(struct objlist *obj, int id, int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2)
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
set_font_style(struct objlist *obj, int id, const char *field)
{
  int style, bold, italic;
  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.bold));
  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Widgets.italic));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }
  if (italic) {
    style |= GRA_FONT_STYLE_ITALIC;
  }
  putobj(obj, field, id, &style);
}

static void
set_font(struct objlist *obj, int id, const char *field)
{
  char *fontalias;
  fontalias = combo_box_get_active_text(Widgets.font);
  if (fontalias) {
    putobj(obj, field, id, fontalias);
  }
}

static void
set_text_obj(struct objlist *obj, int id)
{
  int r, g, b, a, pt;
  set_font_style(obj, id, "style");
  set_font(obj, id, "font");
  set_rgba(Widgets.color1, &r, &g, &b, &a);
  putobj(obj, "R", id, &r);
  putobj(obj, "G", id, &g);
  putobj(obj, "B", id, &b);
  putobj(obj, "A", id, &a);
  pt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Widgets.pt)) * 100;
  putobj(obj, "pt", id, &pt);
}

static void
set_path_type(struct objlist *obj, int id)
{
  int type, interpolation;
  type = combo_box_get_active(Widgets.path_type);
  if (type == 0) {
    putobj(obj, "type", id, &type);
  } else {
    interpolation = type - 1;
    type = 1;
    putobj(obj, "type", id, &type);
    putobj(obj, "interpolation", id, &interpolation);
  }
}

void
presetting_set_obj_field(struct objlist *obj, int id)
{
  const char *name;
  int ival, r1, g1, b1, a1, r2, g2, b2, a2, stroke, fill, width;

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
  width = (2 << combo_box_get_active(Widgets.line_width)) * 10;

  if (strcmp(name, "axis") == 0) {
    putobj(obj, "width", id, &width);
    putobj(obj, "gauge_width1", id, &width);
    putobj(obj, "gauge_width2", id, &width);
    putobj(obj, "gauge_width3", id, &width);
    putobj(obj, "R", id, &r1);
    putobj(obj, "G", id, &g1);
    putobj(obj, "B", id, &b1);
    putobj(obj, "A", id, &a1);
    putobj(obj, "gauge_R", id, &r1);
    putobj(obj, "gauge_G", id, &g1);
    putobj(obj, "gauge_B", id, &b1);
    putobj(obj, "gauge_A", id, &a1);
    putobj(obj, "num_R", id, &r1);
    putobj(obj, "num_G", id, &g1);
    putobj(obj, "num_B", id, &b1);
    putobj(obj, "num_A", id, &a1);
    set_font_style(obj, id, "num_font_style");
    set_font(obj, id, "num_font");
    ival = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Widgets.pt)) * 100;
    putobj(obj, "num_pt", id, &ival);
    ival = combo_box_get_active(Widgets.line_style);
    sputobjfield(obj, id, "style", FwLineStyle[ival].list);
    sputobjfield(obj, id, "gauge_style", FwLineStyle[ival].list);
  } else if (strcmp(name, "axisgrid") == 0) {
    ival = width / 2;
    putobj(obj, "width3", id, &ival);
    ival = width / 4;
    putobj(obj, "width2", id, &ival);
    ival = width / 8;
    putobj(obj, "width1", id, &ival);
  } else if (strcmp(name, "path") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    ival = Widgets.join;
    putobj(obj, "join", id, &ival);
    ival = Widgets.arrow;
    putobj(obj, "arrow", id, &ival);
    putobj(obj, "width", id, &width);
    get_rgba(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
    ival = combo_box_get_active(Widgets.line_style);
    sputobjfield(obj, id, "style", FwLineStyle[ival].list);
    set_path_type(obj, id);
  } else if (strcmp(name, "rectangle") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    putobj(obj, "width", id, &width);
    get_rgba(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
    ival = combo_box_get_active(Widgets.line_style);
    sputobjfield(obj, id, "style", FwLineStyle[ival].list);
  } else if (strcmp(name, "arc") == 0) {
    putobj(obj, "stroke", id, &stroke);
    putobj(obj, "fill", id, &fill);
    ival = Widgets.join;
    putobj(obj, "join", id, &ival);
    putobj(obj, "width", id, &width);
    get_rgba(obj, id, r1, g1, b1, a1, r2, g2, b2, a2);
    ival = combo_box_get_active(Widgets.line_style);
    sputobjfield(obj, id, "style", FwLineStyle[ival].list);
  } else if (strcmp(name, "mark") == 0) {
    putobj(obj, "width", id, &width);
    putobj(obj, "R", id, &r1);
    putobj(obj, "G", id, &g1);
    putobj(obj, "B", id, &b1);
    putobj(obj, "A", id, &a1);
    putobj(obj, "R2", id, &r2);
    putobj(obj, "G2", id, &g2);
    putobj(obj, "B2", id, &b2);
    putobj(obj, "A2", id, &a2);
    ival = gtk_spin_button_get_value(GTK_SPIN_BUTTON(Widgets.mark_size)) * 100;
    putobj(obj, "size", id, &ival);
    ival = combo_box_get_active(Widgets.mark);
    putobj(obj, "type", id, &ival);
    ival = combo_box_get_active(Widgets.line_style);
    sputobjfield(obj, id, "style", FwLineStyle[ival].list);
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
  combo_box_set_active(cbox, 0);
}

static GtkWidget *
create_toggle_button(GtkWidget *box, GtkWidget *img, const char *tooltip, int state)
{
  GtkWidget *w;
  w = gtk_toggle_button_new();
  gtk_container_add(GTK_CONTAINER(w), img);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), state);
  gtk_widget_set_tooltip_text(w, tooltip);
  return w;
}

void
presetting_set_visibility(enum PointerType type)
{
  switch (type) {
  case PointB:
  case AxisB:
  case LegendB:
  case DataB:
  case EvalB:
  case TrimB:
  case ZoomB:
    break;
  case PathB:
    gtk_widget_set_visible(Widgets.stroke,     TRUE);
    gtk_widget_set_visible(Widgets.fill,       TRUE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     TRUE);
    gtk_widget_set_visible(Widgets.path_type,  TRUE);
    gtk_widget_set_visible(Widgets.join_type,  TRUE);
    gtk_widget_set_visible(Widgets.arrow_type, TRUE);
    gtk_widget_set_visible(Widgets.font,       FALSE);
    gtk_widget_set_visible(Widgets.bold,       FALSE);
    gtk_widget_set_visible(Widgets.italic,     FALSE);
    gtk_widget_set_visible(Widgets.pt,         FALSE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  case RectB:
    gtk_widget_set_visible(Widgets.stroke,     TRUE);
    gtk_widget_set_visible(Widgets.fill,       TRUE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     TRUE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  FALSE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       FALSE);
    gtk_widget_set_visible(Widgets.bold,       FALSE);
    gtk_widget_set_visible(Widgets.italic,     FALSE);
    gtk_widget_set_visible(Widgets.pt,         FALSE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  case ArcB:
    gtk_widget_set_visible(Widgets.stroke,     TRUE);
    gtk_widget_set_visible(Widgets.fill,       TRUE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     TRUE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  TRUE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       FALSE);
    gtk_widget_set_visible(Widgets.bold,       FALSE);
    gtk_widget_set_visible(Widgets.italic,     FALSE);
    gtk_widget_set_visible(Widgets.pt,         FALSE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  case MarkB:
    gtk_widget_set_visible(Widgets.stroke,     FALSE);
    gtk_widget_set_visible(Widgets.fill,       FALSE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     TRUE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  FALSE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       FALSE);
    gtk_widget_set_visible(Widgets.bold,       FALSE);
    gtk_widget_set_visible(Widgets.italic,     FALSE);
    gtk_widget_set_visible(Widgets.pt,         FALSE);
    gtk_widget_set_visible(Widgets.mark_size,  TRUE);
    gtk_widget_set_visible(Widgets.mark,       TRUE);
    break;
  case TextB:
    set_font_family(Widgets.font);
    gtk_widget_set_visible(Widgets.stroke,     FALSE);
    gtk_widget_set_visible(Widgets.fill,       FALSE);
    gtk_widget_set_visible(Widgets.line_width, FALSE);
    gtk_widget_set_visible(Widgets.line_style, FALSE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     FALSE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  FALSE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       TRUE);
    gtk_widget_set_visible(Widgets.bold,       TRUE);
    gtk_widget_set_visible(Widgets.italic,     TRUE);
    gtk_widget_set_visible(Widgets.pt,         TRUE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  case GaussB:
    gtk_widget_set_visible(Widgets.stroke,     FALSE);
    gtk_widget_set_visible(Widgets.fill,       FALSE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     FALSE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  TRUE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       FALSE);
    gtk_widget_set_visible(Widgets.bold,       FALSE);
    gtk_widget_set_visible(Widgets.italic,     FALSE);
    gtk_widget_set_visible(Widgets.pt,         FALSE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  case FrameB:
  case SectionB:
  case CrossB:
  case SingleB:
    gtk_widget_set_visible(Widgets.stroke,     FALSE);
    gtk_widget_set_visible(Widgets.fill,       FALSE);
    gtk_widget_set_visible(Widgets.line_width, TRUE);
    gtk_widget_set_visible(Widgets.line_style, TRUE);
    gtk_widget_set_visible(Widgets.color1,     TRUE);
    gtk_widget_set_visible(Widgets.color2,     FALSE);
    gtk_widget_set_visible(Widgets.path_type,  FALSE);
    gtk_widget_set_visible(Widgets.join_type,  FALSE);
    gtk_widget_set_visible(Widgets.arrow_type, FALSE);
    gtk_widget_set_visible(Widgets.font,       TRUE);
    gtk_widget_set_visible(Widgets.bold,       TRUE);
    gtk_widget_set_visible(Widgets.italic,     TRUE);
    gtk_widget_set_visible(Widgets.pt,         TRUE);
    gtk_widget_set_visible(Widgets.mark_size,  FALSE);
    gtk_widget_set_visible(Widgets.mark,       FALSE);
    break;
  }
}

GtkWidget *
create_mark_combo_box(void)
{
  GtkWidget *cbox;
  GtkListStore *list;
  GtkTreeIter iter;
  int j;
  GtkCellRenderer *rend;

  list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_OBJECT);
  cbox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
  gtk_widget_set_tooltip_text(cbox, _("Mark type"));
  rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "text", 0);
  rend = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "pixbuf", 1);
  for (j = 0; j < MARK_TYPE_NUM; j++) {
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(NgraphApp.markpix[j],
					 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE);
    if (pixbuf) {
      char buf[64];
      gtk_list_store_append(list, &iter);
      snprintf(buf, sizeof(buf), "%02d", j);
      gtk_list_store_set(list, &iter,
			 0, buf,
			 1, pixbuf,
			 -1);
      g_object_unref(pixbuf);
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
  return cbox;
}

GtkWidget *
create_line_width_combo_box(void)
{
  GtkWidget *cbox;
  GtkListStore *list;
  GtkTreeIter iter;
  int j;
  GtkCellRenderer *rend;

  list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_OBJECT);
  cbox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
  rend = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "text", 0);
  rend = gtk_cell_renderer_pixbuf_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cbox), rend, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cbox), rend, "pixbuf", 1);
  for (j = 0; j < LINE_WIDTH_ICON_NUM; j++) {
    GdkPixbuf *pixbuf;
    GtkWidget *image;
    char buf[64], *img_file;
    img_file = g_strdup_printf("%s/pixmaps/linewidth_%03d.png", RESOURCE_PATH, 2 << j);
    image = gtk_image_new_from_resource(img_file);
    g_free(img_file);
    pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    if (pixbuf) {
      gtk_list_store_append(list, &iter);
      snprintf(buf, sizeof(buf), "%4.1f", (2 << j) / 10.0);
      gtk_list_store_set(list, &iter,
			 0, buf,
			 1, pixbuf,
			 -1);
    }
    gtk_widget_destroy(image);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 1);
  return cbox;
}

GtkWidget *
create_line_style_combo_box(void)
{
  GtkWidget *cbox;
  int j;

  cbox = gtk_combo_box_text_new();
  for (j = 0; j < LINE_STYLE_ICON_NUM; j++) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cbox), NULL, _(FwLineStyle[j].name));
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
  return cbox;
}

GtkWidget *
create_path_type_combo_box(void)
{
  GtkWidget *cbox;
  int j;
  cbox = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cbox), NULL, _("line"));
  for (j = 0; intpchar[j]; j++) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(cbox), NULL, _(intpchar[j]));
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), 0);
  return cbox;
}

GtkWidget *
create_menu_button(GtkBuilder *builder, const char *menu_name, const char *tooltip)
{
  GtkWidget *w;
  GMenuModel *menu;
  w = gtk_menu_button_new();
  menu = G_MENU_MODEL(gtk_builder_get_object(builder, menu_name));
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(w), menu);
  gtk_widget_set_tooltip_text(w, tooltip);
  return w;
}

GtkWidget *
presetting_create_panel(GtkApplication *app)
{
  GtkWidget *w, *box, *img;
  GtkBuilder *builder;
  GdkRGBA color;

  g_action_map_add_action_entries(G_ACTION_MAP(app), ToolMenuEntries, G_N_ELEMENTS(ToolMenuEntries), app);
  create_images(&Widgets);

  builder = gtk_builder_new_from_resource(RESOURCE_PATH "/gtk/menus-tool.ui");
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(box, SETTING_PANEL_MARGIN);
  gtk_widget_set_margin_end(box, SETTING_PANEL_MARGIN);
  gtk_widget_set_margin_top(box, SETTING_PANEL_MARGIN);
  gtk_widget_set_margin_bottom(box, SETTING_PANEL_MARGIN);

  w = combo_box_create();
  set_font_family(w);
  gtk_widget_set_tooltip_text(w, _("Font name"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.font = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, FALSE, FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), DEFAULT_FONT_PT / 100.0);
  gtk_entry_set_width_chars(GTK_ENTRY(w), 5);
  gtk_widget_set_tooltip_text(w, _("Font size"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.pt = w;

  img = gtk_image_new_from_icon_name("format-text-bold-symbolic", GTK_ICON_SIZE_BUTTON);
  Widgets.bold = create_toggle_button(box, img, _("Bold"), FALSE);

  img = gtk_image_new_from_icon_name("format-text-italic-symbolic", GTK_ICON_SIZE_BUTTON);
  Widgets.italic = create_toggle_button(box, img,  _("Italic"), FALSE);

  w = create_path_type_combo_box();
  gtk_widget_set_margin_end(w, SETTING_PANEL_MARGIN * 4);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.path_type = w;

  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/stroke.png");
  w = create_toggle_button(box, img,  _("Stroke"), TRUE);
  Widgets.stroke = w;

  w = create_mark_combo_box();
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.mark = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), DEFAULT_MARK_SIZE / 100.0);
  gtk_entry_set_width_chars(GTK_ENTRY(w), 5);
  gtk_widget_set_tooltip_text(w, _("Mark size"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.mark_size = w;

  w = create_line_width_combo_box();
  gtk_widget_set_tooltip_text(w, _("Line Width"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.line_width = w;

  w = create_line_style_combo_box();
  gtk_widget_set_tooltip_text(w, _("Line Style"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.line_style = w;

  w = create_color_button(NULL);
  color.red = color.green = color.blue = 0;
  color.alpha = 1;
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(w), &color);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.color1 = w;

  w = create_menu_button(builder, "arrow-type-menu", _("Arrow"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.arrow_type = w;
  //  ArrowTypeNoneAction_activated(NULL, NULL, NULL);

  w = create_menu_button(builder, "join-type-menu", _("Join"));
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.join_type = w;
  gtk_button_set_image(GTK_BUTTON(Widgets.join_type), Widgets.join_icon[DEFAULT_JOIN_TYPE]);

  img = gtk_image_new_from_resource(RESOURCE_PATH "/pixmaps/fill.png");
  w = create_toggle_button(box, img, _("Fill"), FALSE);
  gtk_widget_set_margin_start(w, SETTING_PANEL_MARGIN * 4);
  Widgets.fill = w;

  w = create_color_button(NULL);
  color.red = color.green = color.blue = 1;
  color.alpha = 1;
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(w), &color);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);
  Widgets.color2 = w;

  g_object_unref(builder);
  return box;
}
