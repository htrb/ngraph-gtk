#include "gtk_common.h"

#include <stdlib.h>
#include <math.h>

#include "mathfn.h"
#include "object.h"

#include "gtk_widget.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "dir_defs.h"

GtkWidget *
widget_get_grandparent(GtkWidget *w)
{
  GtkWidget *parent;
  parent = gtk_widget_get_parent(w);
  if (parent == NULL) {
    return NULL;
  }

  return gtk_widget_get_parent(parent);
}

GtkWidget *
button_new_with_icon(const char *icon_name, int toggle)
{
  GtkWidget *img, *button;
  img = gtk_image_new_from_icon_name(icon_name);
  gtk_image_set_icon_size(GTK_IMAGE(img), Menulocal.icon_size);
  if (toggle) {
    button = gtk_toggle_button_new();
  } else {
    button = gtk_button_new();
  }
  gtk_button_set_child(GTK_BUTTON(button), img);
  return button;
}

void
editable_set_init_text(GtkWidget *w, const char *text)
{
  gtk_editable_set_enable_undo(GTK_EDITABLE(w), FALSE);
  gtk_editable_set_text(GTK_EDITABLE(w), text);
  gtk_editable_set_enable_undo(GTK_EDITABLE(w), TRUE);
}

void
spin_button_set_activates_default(GtkWidget *w)
{
  GtkEditable *editable;
  editable = gtk_editable_get_delegate(GTK_EDITABLE(w));
  gtk_text_set_activates_default(GTK_TEXT(editable), TRUE);
}

void
widget_set_parent(GtkWidget *widget, GtkWidget *parent) {
  gtk_widget_set_parent(widget, parent);
  g_signal_connect_swapped(parent, "destroy", G_CALLBACK(gtk_widget_unparent), widget);
}

gboolean
gtk_true(void)
{
  return TRUE;
}

double
scrollbar_get_value(GtkWidget *w)
{
  GtkAdjustment *adj;
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(w));
  return gtk_adjustment_get_value(adj);
}

void
scrollbar_set_value(GtkWidget *w, double val)
{
  GtkAdjustment *adj;
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(w));
  gtk_adjustment_set_value(adj, val);
}

void
scrollbar_set_range(GtkWidget *w, double min, double max)
{
  GtkAdjustment *adj;
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(w));
  gtk_adjustment_set_lower(adj, min);
  gtk_adjustment_set_upper(adj, max);
}

double
scrollbar_get_max(GtkWidget *w)
{
  GtkAdjustment *adj;
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(w));
  return gtk_adjustment_get_upper(adj);
}

void
scrollbar_set_increment(GtkWidget *w, double step, double page)
{
  GtkAdjustment *adj;
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(w));
  gtk_adjustment_set_step_increment(adj, step);
  gtk_adjustment_set_page_increment(adj, step);
}

void
set_button_icon(GtkWidget *w, const char *icon_name)
{
}

void
set_widget_margin(GtkWidget *w, int margin_pos)
{
  if (margin_pos & WIDGET_MARGIN_LEFT) {
    gtk_widget_set_margin_start(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_RIGHT) {
    gtk_widget_set_margin_end(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_BOTTOM) {
    gtk_widget_set_margin_bottom(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_TOP) {
    gtk_widget_set_margin_top(w, 4);
  }
}

void
set_widget_margin_all(GtkWidget *w, int margin)
{
  gtk_widget_set_margin_bottom(w, margin);
  gtk_widget_set_margin_end(w, margin);
  gtk_widget_set_margin_start(w, margin);
  gtk_widget_set_margin_top(w, margin);
}

void
set_scale_mark(GtkWidget *scale, GtkPositionType pos, int start, int inc)
{
  int max, val;
  GtkAdjustment *adj;

  adj = gtk_range_get_adjustment(GTK_RANGE(scale));
  max = gtk_adjustment_get_upper(adj);

  for (val = start; val <=max; val += inc) {
    gtk_scale_add_mark(GTK_SCALE(scale), val, pos, NULL);
  }

  switch (pos) {
  case GTK_POS_BOTTOM:
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
    break;
  case GTK_POS_TOP:
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_BOTTOM);
    break;
  case GTK_POS_LEFT:
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    break;
  case GTK_POS_RIGHT:
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_LEFT);
    break;
  }
}

GtkWidget *
get_mnemonic_label(GtkWidget *w)
{
  GList *list;
  GtkWidget *label;

  if (w == NULL) {
    return NULL;
  }

  list = gtk_widget_list_mnemonic_labels(w);
  if (list == NULL) {
    return NULL;
  }

  label = GTK_WIDGET(list->data);

  g_list_free(list);

  return label;
}

void
set_widget_sensitivity_with_label(GtkWidget *w, gboolean state)
{
  GtkWidget *label;

  if(w == NULL) {
    return;
  }

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_LABEL)) {
    label = w;
    w = gtk_label_get_mnemonic_widget(GTK_LABEL(w));
  } else {
    label = get_mnemonic_label(w);
  }

  if (w) {
    gtk_widget_set_sensitive(w, state);
  }

  if (label) {
    gtk_widget_set_sensitive(label, state);
  }
}

void
set_widget_visibility_with_label(GtkWidget *w, gboolean state)
{
  GtkWidget *label;

  if(w == NULL) {
    return;
  }

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_LABEL)) {
    label = w;
    w = gtk_label_get_mnemonic_widget(GTK_LABEL(w));
  } else {
    label = get_mnemonic_label(w);
  }

  if (w) {
    gtk_widget_set_visible(w, state);
  }

  if (label) {
    gtk_widget_set_visible(label, state);
  }
}

GtkWidget *
add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, const char *title, int expand, int col, int width, int col_max, int n)
{
  GtkWidget *label;

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    set_widget_margin_all(label, 4);
    gtk_grid_attach(GTK_GRID(table), label, col, n, 1, 1);
    col++;
  }

  if (w) {
    if (expand) {
      gtk_widget_set_hexpand(w, TRUE);
      gtk_widget_set_halign(w, GTK_ALIGN_FILL);
    } else {
      gtk_widget_set_halign(w, GTK_ALIGN_START);
    }
    set_widget_margin_all(w, 4);
    gtk_grid_attach(GTK_GRID(table), w, col, n, width, 1);
  }

  return label;
}

GtkWidget *
add_widget_to_table(GtkWidget *table, GtkWidget *w, const char *title, int expand, int n)
{
  return add_widget_to_table_sub(table, w, title, expand, 0, (title) ? 1 : 2, 2, n);
}

GtkWidget *
add_copy_button_to_box(GtkWidget *parent_box, GCallback cb, gpointer d, char *obj_name)
{
  GtkWidget *hbox, *w;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  w = gtk_button_new_with_mnemonic(_("_Copy Settings"));
  g_signal_connect(w, "map", G_CALLBACK(set_sensitivity_by_check_instance), obj_name);
  g_signal_connect(w, "clicked", cb, d);
  gtk_box_append(GTK_BOX(hbox), w);
  gtk_box_append(GTK_BOX(parent_box), hbox);

  return hbox;
}

GtkWidget *
item_setup(GtkWidget *box, GtkWidget *w, char *title, gboolean expand)
{
  GtkWidget *hbox, *label;

  hbox = gtk_grid_new();
  label = gtk_label_new_with_mnemonic(title);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  set_widget_margin_all(label, 2);
  set_widget_margin_all(w, 2);
  if (expand) {
    gtk_widget_set_hexpand(w, TRUE);
    gtk_widget_set_halign(w, GTK_ALIGN_FILL);
  } else {
    gtk_widget_set_halign(w, GTK_ALIGN_START);
  }
  gtk_grid_attach(GTK_GRID(hbox), label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(hbox), w, 1, 0, 1, 1);
  gtk_box_append(GTK_BOX(box), hbox);

  return label;
}

void
entry_set_filename(GtkWidget *w, const char *filename)
{
  editable_set_init_text(w, filename);
}

char *
entry_get_filename(GtkWidget *w)
{
  const char *utf8filename;

  utf8filename = gtk_editable_get_text(GTK_EDITABLE(w));
  if (utf8filename == NULL) {
    return NULL;
  }

  return g_strdup(utf8filename);
}

GtkWidget *
get_parent_window(GtkWidget *w)
{
  GtkWidget *ptr;

  ptr = w;
  while (ptr && ! G_TYPE_CHECK_INSTANCE_TYPE(ptr, GTK_TYPE_WINDOW)) {
    ptr = gtk_widget_get_parent(ptr);
  }
  return (ptr) ? ptr : TopLevel;
}

static void
entry_icon_file_select_response(char *file, gpointer user_data)
{
  GtkWidget *w;
  w = GTK_WIDGET(user_data);
  if (file) {
    entry_set_filename(w, file);
    g_free(file);
  }
}

static void
entry_icon_file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, gpointer user_data)
{
  struct objlist *obj;
  char *ext;
  const char *str;
  int chd;

  obj = (struct objlist *) user_data;
  if (obj == NULL)
    return;

  ext = NULL;
  if (chkobjfield(obj, "ext") == 0 && chkobjlastinst(obj) >= 0) {
    getobj(obj, "ext", 0, 0, NULL, &ext);
  }

  chd = Menulocal.changedirectory;
  str = gtk_editable_get_text(GTK_EDITABLE(w));
  nGetOpenFileName(get_parent_window(GTK_WIDGET(w)), obj->name, ext, NULL, str, chd, entry_icon_file_select_response, w);
}

GtkWidget *
create_file_entry_with_cb(GCallback cb, gpointer data)
{
  GtkWidget *w;

  w = create_text_entry(TRUE, TRUE);

  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, "document-open-symbolic");
  g_signal_connect(w, "icon-release", cb, data);

  return w;
}

GtkWidget *
create_file_entry(struct objlist *obj)
{
  return create_file_entry_with_cb(G_CALLBACK(entry_icon_file_select), obj);
}

static void
direction_icon_released(GtkSpinButton *entry, GtkEntryIconPosition pos, GdkEvent *event, gpointer user_data)
{
  int angle, val;

  angle = gtk_spin_button_get_value(entry);
  val = angle % 360;
  val += (val < 0) ? 360 : 0;

  switch (pos) {
  case GTK_ENTRY_ICON_SECONDARY:
    if (angle == 360) {
      val = 0;
    } else {
      val -= val % 90;
      val += 90;
    }
    break;
  case GTK_ENTRY_ICON_PRIMARY:
    if (angle == 0) {
      val = 360;
    } else {
      int rest;
      rest = val % 90;
      if (rest == 0) {
	val -= 90;
      } else {
	val -= rest;
      }
    }
    break;
  }

  val += (val < 0) ? 360 : 0;

  gtk_spin_button_set_value(entry, val);
}

static void
direction_down(GtkWidget *button, GtkSpinButton *user_data)
{
  direction_icon_released(user_data, GTK_ENTRY_ICON_PRIMARY, NULL, NULL);
}

static void
direction_up(GtkWidget *button, GtkSpinButton *user_data)
{
  direction_icon_released(user_data, GTK_ENTRY_ICON_SECONDARY, NULL, NULL);
}

GtkWidget *
create_direction_entry(GtkWidget *table, const char *title, int row)
{
  GtkWidget *w, *ubtn, *dbtn, *box;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, FALSE, TRUE);
  gtk_editable_set_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH);
  gtk_editable_set_max_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH);

  ubtn = button_new_with_icon("go-up-symbolic", FALSE);
  g_signal_connect(ubtn, "clicked", G_CALLBACK(direction_up), w);

  dbtn = button_new_with_icon("go-down-symbolic", FALSE);
  g_signal_connect(dbtn, "clicked", G_CALLBACK(direction_down), w);

  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(box), w);
  gtk_box_append(GTK_BOX(box), dbtn);
  gtk_box_append(GTK_BOX(box), ubtn);

  add_widget_to_table(table, box, title, FALSE, row);

  return w;
}

GtkWidget *
create_text_entry(int set_default_size, int set_default_action)
{
  GtkWidget *w;

  w = gtk_entry_new();
  if (set_default_size) {
    gtk_editable_set_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH);
  }

  if (set_default_action) {
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
  }

  return w;
}

GtkWidget *
create_number_entry(int set_default_size, int set_default_action)
{
  GtkWidget *w;
  w = create_text_entry(set_default_size, set_default_action);
  gtk_entry_set_input_purpose(GTK_ENTRY(w), GTK_INPUT_PURPOSE_NUMBER);
  return w;
}

static double
int2val(enum SPIN_BUTTON_TYPE type, int ival)
{
  gdouble val;

  switch (type) {
  case SPIN_BUTTON_TYPE_WIDTH:
  case SPIN_BUTTON_TYPE_LENGTH:
  case SPIN_BUTTON_TYPE_POSITION:
  case SPIN_BUTTON_TYPE_ANGLE:
  case SPIN_BUTTON_TYPE_SPACE_POINT:
  case SPIN_BUTTON_TYPE_POINT:
  case SPIN_BUTTON_TYPE_PERCENT:
    val = ival * 0.01;
    break;
  case SPIN_BUTTON_TYPE_INT:
  case SPIN_BUTTON_TYPE_UINT:
  case SPIN_BUTTON_TYPE_NUM:
  case SPIN_BUTTON_TYPE_CUSTOM:
  default:
    val = ival;
  }

  return val;
}

static int
val2int(enum SPIN_BUTTON_TYPE type, double val)
{
  int ival;

  switch (type) {
  case SPIN_BUTTON_TYPE_WIDTH:
  case SPIN_BUTTON_TYPE_LENGTH:
  case SPIN_BUTTON_TYPE_POSITION:
  case SPIN_BUTTON_TYPE_ANGLE:
  case SPIN_BUTTON_TYPE_SPACE_POINT:
  case SPIN_BUTTON_TYPE_POINT:
  case SPIN_BUTTON_TYPE_PERCENT:
    ival = nround(val * 100);
    break;
  case SPIN_BUTTON_TYPE_INT:
  case SPIN_BUTTON_TYPE_UINT:
  case SPIN_BUTTON_TYPE_NUM:
  case SPIN_BUTTON_TYPE_CUSTOM:
  default:
    ival = val;
  }
  return ival;
}

void
spin_entry_set_range(GtkWidget *w, int min, int max)
{
  enum SPIN_BUTTON_TYPE type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));

  gtk_spin_button_set_range(GTK_SPIN_BUTTON(w),
			    int2val(type, min),
			    int2val(type, max));

}

void
spin_entry_set_inc(GtkWidget *w, int inc, int page)
{
  enum SPIN_BUTTON_TYPE type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));

  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(w),
				 int2val(type, inc),
				 int2val(type, page));
}

void
spin_entry_set_val(GtkWidget *entry, int ival)
{
  gdouble min, max, val;
  enum SPIN_BUTTON_TYPE type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "user-data"));

  val = int2val(type, ival);

  gtk_spin_button_get_range(GTK_SPIN_BUTTON(entry), &min, &max);

  if (val < min) {
    val = min;
  } else if (val > max) {
    val = max;
  }

  gtk_editable_set_enable_undo(GTK_EDITABLE(entry), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), val);
  gtk_editable_set_enable_undo(GTK_EDITABLE(entry), TRUE);
}

int
spin_entry_get_val(GtkWidget *entry)
{
  gdouble val;
  enum SPIN_BUTTON_TYPE type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(entry), "user-data"));
  val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(entry));

  return val2int(type, val);
}

static int
spin_change_value_cb(GtkSpinButton *spinbutton, GtkScrollType arg1, gpointer user_data)
{
  const char *str;
  double oval, val;
  int ecode;

  str = gtk_editable_get_text(GTK_EDITABLE(spinbutton));
  if (str == NULL)
    return 0;

  oval = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton));

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), oval);
    return 0;
  }
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), val);

  return 0;
}

static GtkWidget *
_create_spin_entry(enum SPIN_BUTTON_TYPE type, double min, double max,
		   double inc, double page, gboolean numeric,
		   gboolean wrap, int set_default_size, int set_default_action)
{
  GtkWidget *w;

  w = gtk_spin_button_new_with_range(min, max, inc);
  gtk_editable_set_alignment(GTK_EDITABLE(w), 1.0);

  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(w), inc, page);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), wrap);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), (numeric) ? 0 : 2);

  if (set_default_size) {
    gtk_editable_set_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH);
    gtk_editable_set_max_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH);
  }

  if (set_default_action) {
    spin_button_set_activates_default(w);
  }

  g_object_set_data(G_OBJECT(w), "user-data", GINT_TO_POINTER(type));

  g_signal_connect(w, "input", G_CALLBACK(spin_change_value_cb), NULL);

  return w;
}

GtkWidget *
create_spin_entry_type(enum SPIN_BUTTON_TYPE type,
		       int set_default_size, int set_default_action)
{
  double min, max, inc, page = 10;
  gboolean wrap = FALSE, numeric = FALSE;

  switch (type) {
  case SPIN_BUTTON_TYPE_WIDTH:
    min = 0;
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 0.1;
    page = 1;
    break;
  case SPIN_BUTTON_TYPE_LENGTH:
    min = 0;
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 1;
    break;
  case SPIN_BUTTON_TYPE_POSITION:
    min = int2val(type, - SPIN_ENTRY_MAX);
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 1;
    break;
  case SPIN_BUTTON_TYPE_ANGLE:
    min = 0;
    max = 360;
    inc = 1;
    page = 15;
    wrap = TRUE;
    break;
  case SPIN_BUTTON_TYPE_SPACE_POINT:
    min = 0;
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 1;
    break;
  case SPIN_BUTTON_TYPE_POINT:
    min = int2val(type, TEXT_SIZE_MIN);
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 1;
    break;
  case SPIN_BUTTON_TYPE_PERCENT:
    min = 0;
    max = int2val(type, SPIN_ENTRY_MAX);
    inc = 1;
    break;
  case SPIN_BUTTON_TYPE_INT:
    min = INT_MIN;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
    break;
  case SPIN_BUTTON_TYPE_UINT:
    min = 0;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
    break;
  case SPIN_BUTTON_TYPE_NUM:
    min = -1;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
    break;
  case SPIN_BUTTON_TYPE_NATURAL:
    min = 1;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
    break;
  case SPIN_BUTTON_TYPE_CUSTOM:
    min = INT_MIN;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
    break;
  default:
    type = SPIN_BUTTON_TYPE_CUSTOM;
    min = INT_MIN;
    max = INT_MAX;
    inc = 1;
    numeric = TRUE;
  }

  return _create_spin_entry(type, min, max, inc, page, numeric, wrap,
			    set_default_size, set_default_action);
}

GtkWidget *
create_spin_entry(int min, int max, int inc,
		  int set_default_size, int set_default_action)
{
  return _create_spin_entry(SPIN_BUTTON_TYPE_CUSTOM, min, max, inc, inc * 10,
			    TRUE, FALSE, set_default_size, set_default_action);
}

static void
set_custom_palette(GtkWidget *dlg)
{
  int n;
  struct narray *palette;
  GdkRGBA *colors;

  palette = &(Menulocal.custom_palette);
  n = arraynum(palette);
  if (n < 1) {
    return;
  }
  colors = arraydata(palette);
  gtk_color_chooser_add_palette(GTK_COLOR_CHOOSER(dlg), GTK_ORIENTATION_HORIZONTAL, 9, n, colors);
}

static void
show_color_sel(GtkWidget *w, gpointer user_data)
{
  GdkRGBA col;
  char buf[64];

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &col);
  snprintf(buf, sizeof(buf),
	   "#%02X%02X%02X",
	   nround(col.red * 255),
	   nround(col.green * 255),
	   nround(col.blue * 255));
  gtk_widget_set_tooltip_text(w, buf);
}

void
add_default_color(struct narray *palette)
{
  const char * const default_colors[9*5] = {
    "#99c1f1", "#8ff0a4", "#f9f06b", "#ffbe6f", "#f66151", "#dc8add", "#cdab8f", "#ffffff", "#77767b",
    "#62a0ea", "#57e389", "#f8e45c", "#ffa348", "#ed333b", "#c061cb", "#b5835a", "#f6f5f4", "#5e5c64",
    "#3584e4", "#33d17a", "#f6d32d", "#ff7800", "#e01b24", "#9141ac", "#986a44", "#deddda", "#3d3846",
    "#1c71d8", "#2ec27e", "#f5c211", "#e66100", "#c01c28", "#813d9c", "#865e3c", "#c0bfbc", "#241f31",
    "#1a5fb4", "#26a269", "#e5a50a", "#c64600", "#a51d2d", "#613583", "#63452c", "#9a9996", "#000000",
    /* Blue     Green      Yellow     Orange     Red        Purple     Brown      Light      Dark */
  };
  GdkRGBA color;
  int i, n;
  n = sizeof(default_colors) / sizeof(*default_colors);
  for (i = 0; i < n; i++) {
    gdk_rgba_parse(&color, default_colors[i]);
    arrayadd(palette, &color);
  }
}

static void
set_default_palette(GtkWidget *cc)
{
  struct narray palette;
  gint n;

  arrayinit(&palette, sizeof(GdkRGBA));
  add_default_color(&palette);
  n = arraynum(&palette);
  gtk_color_chooser_add_palette(GTK_COLOR_CHOOSER(cc), GTK_ORIENTATION_HORIZONTAL, 9, n, arraydata(&palette));
  arraydel(&palette);
}

#define CUSTOM_PALETTE_KEY "custom_palette"

static void
show_color_dialog(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  GtkColorChooser *btn;
  btn = GTK_COLOR_CHOOSER(user_data);
  gtk_color_chooser_add_palette(GTK_COLOR_CHOOSER(btn), GTK_ORIENTATION_HORIZONTAL, 0, 0, NULL);
  if (Menulocal.use_custom_palette) {
    set_custom_palette(GTK_WIDGET(btn));
  } else {
    set_default_palette(GTK_WIDGET(btn));
  }
}

GtkWidget *
create_color_button(GtkWidget *win)
{
  GtkGesture *gesture;
  GtkWidget *w;

  w = gtk_color_button_new();
  g_object_set_data(G_OBJECT(w), CUSTOM_PALETTE_KEY, GINT_TO_POINTER(0));
  g_signal_connect(w, "color-set", G_CALLBACK(show_color_sel), win);
  gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(w, GTK_EVENT_CONTROLLER(gesture));

  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed", G_CALLBACK(show_color_dialog), w);

  return w;
}

void
set_widget_font(GtkWidget *w, const char *font)
{
  GtkCssProvider *css_provider;
  char *css_str;
  PangoFontDescription *desc;
  const char *family, *style_str, *unit;
  PangoStyle style;
  PangoWeight weight;
  int weight_val, size;

  desc = pango_font_description_from_string(font);
  if (desc == NULL) {
    return;
  }

  family = pango_font_description_get_family(desc);
  style = pango_font_description_get_style(desc);
  size = pango_font_description_get_size(desc);
  weight = pango_font_description_get_weight(desc);
  switch (style) {
  case PANGO_STYLE_NORMAL:
    style_str = "normal";
    break;
  case PANGO_STYLE_OBLIQUE:
    style_str = "oblique";
    break;
 case PANGO_STYLE_ITALIC:
    style_str = "italic";
    break;
  default:
    style_str = "normal";
    break;
  }

  switch (weight) {
  case PANGO_WEIGHT_THIN:
    weight_val = 100;
    break;
  case PANGO_WEIGHT_ULTRALIGHT:
    weight_val = 200;
    break;
  case PANGO_WEIGHT_LIGHT:
    weight_val = 300;
    break;
  case PANGO_WEIGHT_SEMILIGHT:
    weight_val = 350;
    break;
  case PANGO_WEIGHT_BOOK:
    weight_val = 380;
    break;
  case PANGO_WEIGHT_NORMAL:
    weight_val = 400;
    break;
  case PANGO_WEIGHT_MEDIUM:
    weight_val = 500;
    break;
  case PANGO_WEIGHT_SEMIBOLD:
    weight_val = 600;
    break;
  case PANGO_WEIGHT_BOLD:
    weight_val = 700;
    break;
  case PANGO_WEIGHT_ULTRABOLD:
    weight_val = 800;
    break;
  case PANGO_WEIGHT_HEAVY:
    weight_val = 900;
    break;
  case PANGO_WEIGHT_ULTRAHEAVY:
    weight_val = 1000;
    break;
  default:
    weight_val = 400;
    break;
  }

  if (pango_font_description_get_size_is_absolute(desc)) {
    unit = "px";
  } else {
    unit = "pt";
  }

  css_str = g_strdup_printf("* {\n"
			    "   font-style: %s;\n"
			    "   font-weight: %d;\n"
			    "   font-size: %d%s;\n"
			    "   font-family: \"%s\";\n"
			    "}",
			    style_str,
			    weight_val,
			    size / PANGO_SCALE,
			    unit,
			    family ? family : "");
  pango_font_description_free(desc);
  if (css_str == NULL) {
    return;
  }

  css_provider = gtk_css_provider_new();
  if (css_provider == NULL) {
    return;
  }

  gtk_css_provider_load_from_data(css_provider, css_str, -1);
  g_free(css_str);
  gtk_style_context_add_provider(gtk_widget_get_style_context(w),
				 GTK_STYLE_PROVIDER(css_provider),
				 GTK_STYLE_PROVIDER_PRIORITY_USER);
}

GtkWidget *
create_text_view_with_line_number(GtkWidget **v)
{
  GtkWidget *source_view, *swin;
  GtkSourceBuffer *buffer;

  source_view = gtk_source_view_new();
  gtk_widget_set_hexpand(source_view, TRUE);
  gtk_widget_set_vexpand(source_view, TRUE);
  buffer = gtk_source_buffer_new(NULL);

  gtk_text_view_set_buffer(GTK_TEXT_VIEW(source_view), GTK_TEXT_BUFFER(buffer));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(source_view), FALSE);

  gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), FALSE);
  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(source_view), TRUE);

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), source_view);
  *v = source_view;
  return swin;
}

void
text_view_with_line_number_set_text(GtkWidget *view, const gchar *str)
{
  GtkTextBuffer *buf;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  gtk_text_buffer_set_text(buf, str, -1);
}

void
text_view_with_line_number_set_font(GtkWidget *view, const gchar *font)
{
  set_widget_font(view, font);
}

struct select_obj_color_data {
  struct objlist *obj;
  int id, type, r, g, b, a;
  response_cb cb;
  gpointer data;
};

static void
select_obj_color_response(GtkWindow *dlg, int response, gpointer user_data)
{
  int r, g, b, a, rr ,gg, bb, aa, modified, undo;
  GdkRGBA color;
  struct select_obj_color_data *data;
  struct objlist *obj;
  int id, type;
  response_cb cb;
  gpointer ud;

  data = (struct select_obj_color_data *) user_data;
  obj = data->obj;
  id = data->id;
  type = data->type;
  cb = data->cb;
  ud = data->data;
  r = data->r;
  g = data->g;
  b = data->b;
  a = data->a;
  g_free(data);

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dlg), &color);
  gtk_window_destroy(dlg);

  if (response != GTK_RESPONSE_OK) {
    if (cb) {
      cb(SELECT_OBJ_COLOR_CANCEL, ud);
    }
    return;
  }

  rr = nround(color.red * 255);
  gg = nround(color.green * 255);
  bb = nround(color.blue * 255);
  aa = nround(color.alpha * 255);

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
  switch (type) {
  case OBJ_FIELD_COLOR_TYPE_STROKE:
    putobj(obj, "stroke_R", id, &rr);
    putobj(obj, "stroke_G", id, &gg);
    putobj(obj, "stroke_B", id, &bb);
    putobj(obj, "stroke_A", id, &aa);
    break;
  case OBJ_FIELD_COLOR_TYPE_FILL:
    putobj(obj, "fill_R", id, &rr);
    putobj(obj, "fill_G", id, &gg);
    putobj(obj, "fill_B", id, &bb);
    putobj(obj, "fill_A", id, &aa);
    break;
  case OBJ_FIELD_COLOR_TYPE_0:
  case OBJ_FIELD_COLOR_TYPE_1:
  case OBJ_FIELD_COLOR_TYPE_AXIS_BASE:
    putobj(obj, "R", id, &rr);
    putobj(obj, "G", id, &gg);
    putobj(obj, "B", id, &bb);
    putobj(obj, "A", id, &aa);
    break;
  case OBJ_FIELD_COLOR_TYPE_2:
    putobj(obj, "R2", id, &rr);
    putobj(obj, "G2", id, &gg);
    putobj(obj, "B2", id, &bb);
    putobj(obj, "A2", id, &aa);
    break;
  case OBJ_FIELD_COLOR_TYPE_AXIS_GAUGE:
    putobj(obj, "gauge_R", id, &rr);
    putobj(obj, "gauge_G", id, &gg);
    putobj(obj, "gauge_B", id, &bb);
    putobj(obj, "gauge_A", id, &aa);
    break;
  case OBJ_FIELD_COLOR_TYPE_AXIS_NUM:
    putobj(obj, "num_R", id, &rr);
    putobj(obj, "num_G", id, &gg);
    putobj(obj, "num_B", id, &bb);
    putobj(obj, "num_A", id, &aa);
    break;
  default:
    menu_delete_undo(undo);
    if (cb) {
      cb(SELECT_OBJ_COLOR_ERROR, ud);
    }
    return;
  }

  if (rr == r && gg == g && bb == b && aa == a) {
    menu_delete_undo(undo);
    modified = SELECT_OBJ_COLOR_SAME;
  } else {
    modified = SELECT_OBJ_COLOR_DIFFERENT;
  }
  if (cb) {
    cb(modified, ud);
  }
}

void
select_obj_color(struct objlist *obj, int id, enum OBJ_FIELD_COLOR_TYPE type, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg;
  int r, g, b, a;
  GdkRGBA color;
  char *title;
  struct select_obj_color_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(SELECT_OBJ_COLOR_ERROR, user_data);
    }
    return;
  }
  data->obj = obj;
  data->id = id;
  data->type = type;
  data->cb = cb;
  data->data = user_data;

  switch (type) {
  case OBJ_FIELD_COLOR_TYPE_STROKE:
    title = _("Stroke Color");
    getobj(obj, "stroke_R", id, 0, NULL, &r);
    getobj(obj, "stroke_G", id, 0, NULL, &g);
    getobj(obj, "stroke_B", id, 0, NULL, &b);
    getobj(obj, "stroke_A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_FILL:
    title = _("Fill Color");
    getobj(obj, "fill_R", id, 0, NULL, &r);
    getobj(obj, "fill_G", id, 0, NULL, &g);
    getobj(obj, "fill_B", id, 0, NULL, &b);
    getobj(obj, "fill_A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_0:
    title = _("Color");
    getobj(obj, "R", id, 0, NULL, &r);
    getobj(obj, "G", id, 0, NULL, &g);
    getobj(obj, "B", id, 0, NULL, &b);
    getobj(obj, "A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_1:
    title = _("Color 1");
    getobj(obj, "R", id, 0, NULL, &r);
    getobj(obj, "G", id, 0, NULL, &g);
    getobj(obj, "B", id, 0, NULL, &b);
    getobj(obj, "A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_2:
    title = _("Color 2");
    getobj(obj, "R2", id, 0, NULL, &r);
    getobj(obj, "G2", id, 0, NULL, &g);
    getobj(obj, "B2", id, 0, NULL, &b);
    getobj(obj, "A2", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_AXIS_BASE:
    title = _("Axis baseline color");
    getobj(obj, "R", id, 0, NULL, &r);
    getobj(obj, "G", id, 0, NULL, &g);
    getobj(obj, "B", id, 0, NULL, &b);
    getobj(obj, "A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_AXIS_GAUGE:
    title = _("Axis gauge color");
    getobj(obj, "gauge_R", id, 0, NULL, &r);
    getobj(obj, "gauge_G", id, 0, NULL, &g);
    getobj(obj, "gauge_B", id, 0, NULL, &b);
    getobj(obj, "gauge_A", id, 0, NULL, &a);
    break;
  case OBJ_FIELD_COLOR_TYPE_AXIS_NUM:
    title = _("Axis numbering color");
    getobj(obj, "num_R", id, 0, NULL, &r);
    getobj(obj, "num_G", id, 0, NULL, &g);
    getobj(obj, "num_B", id, 0, NULL, &b);
    getobj(obj, "num_A", id, 0, NULL, &a);
    break;
  default:
    g_free(data);
    if (cb) {
      cb(SELECT_OBJ_COLOR_ERROR, user_data);
    }
    return;
  }

  if (! Menulocal.use_opacity) {
    a = 255;
  }

  color.red = r / 255.0;
  color.green = g / 255.0;
  color.blue = b / 255.0;
  color.alpha = a / 255.0;

  dlg = gtk_color_chooser_dialog_new(title, GTK_WINDOW(TopLevel));
  if (Menulocal.use_custom_palette) {
    set_custom_palette(dlg);
  }
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dlg), &color);
  gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(dlg), Menulocal.use_opacity);
  data->r = r;
  data->g = g;
  data->b = b;
  data->a = a;
  g_signal_connect(dlg, "response", G_CALLBACK(select_obj_color_response), data);
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_widget_show(GTK_WIDGET(dlg));
  return;
}

gchar *
get_text_from_buffer(GtkTextBuffer *buffer)
{
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void
add_button_common(GtkWidget *w, GtkWidget *grid, int row, int col, const char *tooltip, GCallback proc, gpointer data)
{
  gtk_widget_set_tooltip_text(GTK_WIDGET(w), tooltip);
  gtk_widget_set_vexpand(GTK_WIDGET(w), FALSE);
  gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(GTK_WIDGET(w), FALSE);
  gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), w, col, row, 1, 1);
  if (proc) {
    g_signal_connect(w, "clicked", proc, data);
  }
}

GtkWidget *
add_button(GtkWidget *grid, int row, int col, const char *icon, const char *tooltip, GCallback proc, gpointer data)
{
  GtkWidget *w;
  w = button_new_with_icon(icon, FALSE);
  add_button_common(w, grid, row, col, tooltip, proc, data);
  return w;
}

GtkWidget *
add_toggle_button(GtkWidget *grid, int row, int col, const char *icon_name, const char *tooltip, GCallback proc, gpointer data)
{
  GtkWidget *w;
  w = button_new_with_icon(icon_name, TRUE);
  add_button_common(w, grid, row, col, tooltip, NULL, NULL);
  if (proc) {
    g_signal_connect(w, "toggled", proc, data);
  }
  return w;
}

void
add_event_key(GtkWidget *widget, GCallback press_proc, GCallback release_proc, gpointer user_data)
{
  GtkEventController *ev;

  ev = gtk_event_controller_key_new();
  gtk_widget_add_controller(widget, ev);
  if (press_proc) {
    g_signal_connect(ev, "key-pressed", press_proc, user_data);
  }
  if (release_proc) {
    g_signal_connect(ev, "key-released", release_proc, user_data);
  }
}
