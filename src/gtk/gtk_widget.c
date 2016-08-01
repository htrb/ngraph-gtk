#include <stdlib.h>
#include <math.h>

#include "mathfn.h"
#include "object.h"
#include "otext.h"

#include "gtk_common.h"
#include "gtk_widget.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "dir_defs.h"

GtkWidget *
create_image_from_file(const char *file)
{
  GtkWidget *img;
#ifdef WINDOWS
    char *str;
    str = g_strdup_printf("%s%s", PIXMAPDIR, file);
    img = gtk_image_new_from_file(str);
    g_free(str);
#else
    img = gtk_image_new_from_file(file);
#endif
    return img;
}

void
set_button_icon(GtkWidget *w, const char *icon_name)
{
#if ! GTK_CHECK_VERSION(3, 12, 0)
  GtkWidget *icon;
  icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON(w), icon);
#endif
}

void
set_widget_margin(GtkWidget *w, int margin_pos)
{
#if GTK_CHECK_VERSION(3, 0, 0)
#if GTK_CHECK_VERSION(3, 12, 0)
  if (margin_pos & WIDGET_MARGIN_LEFT) {
    gtk_widget_set_margin_start(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_RIGHT) {
    gtk_widget_set_margin_end(w, 4);
  }
#else
  if (margin_pos & WIDGET_MARGIN_LEFT) {
    gtk_widget_set_margin_left(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_RIGHT) {
    gtk_widget_set_margin_right(w, 4);
  }
#endif

  if (margin_pos & WIDGET_MARGIN_BOTTOM) {
    gtk_widget_set_margin_bottom(w, 4);
  }

  if (margin_pos & WIDGET_MARGIN_TOP) {
    gtk_widget_set_margin_top(w, 4);
  }
#endif
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
add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int col_max, int n)
{
  GtkWidget *label;
#if ! GTK_CHECK_VERSION(3, 4, 0)
  GtkWidget *align;
  int x, y;

  g_object_get(table, "n-columns", &x, "n-rows", &y, NULL);

  x = (x > col_max) ? x : col_max;
  y = (y > n + 1) ? y : n + 1;
  gtk_table_resize(GTK_TABLE(table), y, x);
#endif

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
#if GTK_CHECK_VERSION(3, 4, 0)
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), label, col, n, 1, 1);
#else
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, n, n + 1, GTK_FILL, 0, 4, 4);
#endif
    col++;
  }

  if (w) {
#if GTK_CHECK_VERSION(3, 4, 0)
    if (expand) {
      gtk_widget_set_hexpand(w, TRUE);
      gtk_widget_set_halign(w, GTK_ALIGN_FILL);
    } else {
      gtk_widget_set_halign(w, GTK_ALIGN_START);
    }
    g_object_set(w, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), w, col, n, width, 1);
#else
    align = gtk_alignment_new(0, 0.5, (expand) ? 1 : 0, 0);
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, col, col + width, n, n + 1, ((expand) ? GTK_EXPAND : 0) | GTK_FILL, 0, 4, 4);
#endif
  }

  return label;
}

GtkWidget *
add_widget_to_table(GtkWidget *table, GtkWidget *w, char *title, int expand, int n)
{
  return add_widget_to_table_sub(table, w, title, expand, 0, (title) ? 1 : 2, 2, n);
}

GtkWidget *
add_copy_button_to_box(GtkWidget *parent_box, GCallback cb, gpointer d, char *obj_name)
{
  GtkWidget *hbox, *w;

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif
  w = gtk_button_new_with_mnemonic(_("_Copy Settings"));
  g_signal_connect(w, "map", G_CALLBACK(set_sensitivity_by_check_instance), obj_name);
  g_signal_connect(w, "clicked", cb, d);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(parent_box), hbox, FALSE, FALSE, 4);

  return hbox;
}

GtkWidget *
item_setup(GtkWidget *box, GtkWidget *w, char *title, gboolean expand)
{
  GtkWidget *hbox, *label;

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_grid_new();
  label = gtk_label_new_with_mnemonic(title);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  g_object_set(label, "margin", GINT_TO_POINTER(2), NULL);
  g_object_set(w, "margin", GINT_TO_POINTER(2), NULL);
  if (expand) {
    gtk_widget_set_hexpand(w, TRUE);
    gtk_widget_set_halign(w, GTK_ALIGN_FILL);
  } else {
    gtk_widget_set_halign(w, GTK_ALIGN_START);
  }
  gtk_grid_attach(GTK_GRID(hbox), label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(hbox), w, 1, 0, 1, 1);
#else
  hbox = gtk_hbox_new(FALSE, 4);
  label = gtk_label_new_with_mnemonic(title);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), w, expand, expand, 2);
#endif
  gtk_box_pack_start(GTK_BOX(box), hbox, expand, expand, 4);

  return label;
}

int
entry_set_filename(GtkWidget *w, char *filename)
{
  gtk_entry_set_text(GTK_ENTRY(w), filename);

  return 0;
}

char *
entry_get_filename(GtkWidget *w)
{
  const char *utf8filename;

  utf8filename = gtk_entry_get_text(GTK_ENTRY(w));
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
entry_icon_file_select(GtkEntry *w, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
  struct objlist *obj;
  char *file, *ext;

  obj = (struct objlist *) user_data;
  if (obj == NULL)
    return;

  ext = NULL;
  if (chkobjfield(obj, "ext") == 0 && chkobjlastinst(obj) >= 0) {
    getobj(obj, "ext", 0, 0, NULL, &ext);
  }

  if (nGetOpenFileName(get_parent_window(GTK_WIDGET(w)), obj->name, ext, NULL,
		       gtk_entry_get_text(w),
		       &file, TRUE, Menulocal.changedirectory) == IDOK && file) {
    entry_set_filename(GTK_WIDGET(w), file);
    g_free(file);
  }
}

GtkWidget *
create_file_entry_with_cb(GCallback cb, gpointer data)
{
  GtkWidget *w;

  w = create_text_entry(TRUE, TRUE);

  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, "document-open");
  g_signal_connect(w, "icon-release", cb, data);

  return w;
}

GtkWidget *
create_file_entry(struct objlist *obj)
{
  return create_file_entry_with_cb(G_CALLBACK(entry_icon_file_select), obj);
}

static void
direction_icon_released(GtkEntry *entry, GtkEntryIconPosition pos, GdkEvent *event, gpointer user_data)
{
  int angle, val, rest;

  angle = gtk_spin_button_get_value(GTK_SPIN_BUTTON(entry));
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

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), val);
}

GtkWidget *
create_direction_entry(void)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, FALSE, TRUE);
  gtk_entry_set_width_chars(GTK_ENTRY(w), NUM_ENTRY_WIDTH);
#if GTK_CHECK_VERSION(3, 12, 0)
  gtk_entry_set_max_width_chars(GTK_ENTRY(w), NUM_ENTRY_WIDTH);
#endif
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, "go-up");
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_PRIMARY, "go-down");
  g_signal_connect(w, "icon-release", G_CALLBACK(direction_icon_released), NULL);

  return w;
}



GtkWidget *
create_text_entry(int set_default_size, int set_default_action)
{
  GtkWidget *w;

  w = gtk_entry_new();
  if (set_default_size) {
    gtk_entry_set_width_chars(GTK_ENTRY(w), NUM_ENTRY_WIDTH);
  }

  if (set_default_action) {
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
  }

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

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), val);
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

  str = gtk_entry_get_text(GTK_ENTRY(spinbutton));
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
  gtk_entry_set_alignment(GTK_ENTRY(w), 1.0);

  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(w), inc, page);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), wrap);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(w), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), (numeric) ? 0 : 2);

  if (set_default_size) {
    gtk_entry_set_width_chars(GTK_ENTRY(w), NUM_ENTRY_WIDTH);
#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_entry_set_max_width_chars(GTK_ENTRY(w), NUM_ENTRY_WIDTH);
#endif
  }

  if (set_default_action) {
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
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
    inc = 2;
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

#if GTK_CHECK_VERSION(3, 4, 0)
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
#else
static gboolean
show_color_sel(GtkWidget *w, GdkEventButton *e, gpointer user_data)
{
  GtkWidget *dlg;
  GtkColorSelection *sel;
  GdkColor col;
  GtkColorButton *button;
  gboolean r;
  guint16 alpha;


  button = GTK_COLOR_BUTTON(w);

  gtk_color_button_get_color(button, &col);
  alpha = (Menulocal.use_opacity) ? gtk_color_button_get_alpha(button) : 0xffff;

  dlg = gtk_color_selection_dialog_new(_("Pick a Color"));
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(user_data));
  sel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dlg)));

  gtk_color_selection_set_has_palette(sel, TRUE);
  gtk_color_selection_set_has_opacity_control(sel, Menulocal.use_opacity);
  gtk_color_selection_set_current_color(sel, &col);
  gtk_color_selection_set_current_alpha(sel, alpha);

  r = ndialog_run(dlg);
  gtk_color_selection_get_current_color(sel, &col);
  alpha = gtk_color_selection_get_current_alpha(sel);
  gtk_widget_destroy(dlg);

  if (r == GTK_RESPONSE_OK) {
    char buf[64];
    snprintf(buf, sizeof(buf),
	     "#%02X%02X%02X",
	     col.red >> 8,
	     col.green >> 8,
	     col.blue >> 8);
    gtk_widget_set_tooltip_text(w, buf);

    gtk_color_button_set_color(button, &col);
    gtk_color_button_set_alpha(button, alpha);
  }

  return TRUE;
}

static gboolean
color_button_key_event(GtkWidget *w, GdkEventKey *e, gpointer u)
{
  switch (e->keyval) {
  case GDK_KEY_space:
  case GDK_KEY_Return:
    if (e->type == GDK_KEY_RELEASE) {
      show_color_sel(w, NULL, u);
    }
    return TRUE;
  }

  return FALSE;
}
#endif

GtkWidget *
create_color_button(GtkWidget *win)
{
  GtkWidget *w;

  w = gtk_color_button_new();
#if GTK_CHECK_VERSION(3, 4, 0)
  g_signal_connect(w, "color-set", G_CALLBACK(show_color_sel), win);
#else
  gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(w), Menulocal.use_opacity);
  g_signal_connect(w, "button-release-event", G_CALLBACK(show_color_sel), win);
  g_signal_connect(w, "key-press-event", G_CALLBACK(color_button_key_event), win);
  g_signal_connect(w, "key-release-event", G_CALLBACK(color_button_key_event), win);
#endif

  return w;
}

static void
set_adjustment(GtkAdjustment *adj, gdouble inc)
{
  gdouble val, min, max;

  if (adj == NULL || inc == 0) {
    return;
  }

  min = gtk_adjustment_get_lower(adj);
  max = gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj);
  val = gtk_adjustment_get_value(adj);
  val += inc;

  if (max < min) {
    return;
  }

  if (val < min) {
    val = min;
  } else if (val > max) {
    val = max;
  }

  gtk_adjustment_set_value(adj, val);
}

static gboolean
text_view_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
  GtkRange *scl;
  GtkAdjustment *x_adj, *y_adj;
  gdouble x, y;

  switch (event->direction) {
  case GDK_SCROLL_UP:
    scl = g_object_get_data(G_OBJECT(widget), "vscroll");
    y_adj = gtk_range_get_adjustment(scl);
    y = - gtk_adjustment_get_step_increment(y_adj);
    x_adj = NULL;
    x = 0;
    break;
  case GDK_SCROLL_DOWN:
    scl = g_object_get_data(G_OBJECT(widget), "vscroll");
    y_adj = gtk_range_get_adjustment(scl);
    y = gtk_adjustment_get_step_increment(y_adj);
    x_adj = NULL;
    x = 0;
    break;
  case GDK_SCROLL_LEFT:
    scl = g_object_get_data(G_OBJECT(widget), "hscroll");
    x_adj = gtk_range_get_adjustment(scl);
    x = - gtk_adjustment_get_step_increment(x_adj);
    y_adj = NULL;
    y = 0;
    break;
  case GDK_SCROLL_RIGHT:
    scl = g_object_get_data(G_OBJECT(widget), "hscroll");
    x_adj = gtk_range_get_adjustment(scl);
    x = gtk_adjustment_get_step_increment(x_adj);
    y_adj = NULL;
    y = 0;
    break;
#if GTK_CHECK_VERSION(3, 4, 0)
  case GDK_SCROLL_SMOOTH:
    if (gdk_event_get_scroll_deltas((GdkEvent *) event, &x, &y)) {
      scl = g_object_get_data(G_OBJECT(widget), "hscroll");
      x_adj = gtk_range_get_adjustment(scl);
      x *= gtk_adjustment_get_step_increment(x_adj);

      scl = g_object_get_data(G_OBJECT(widget), "vscroll");
      y_adj = gtk_range_get_adjustment(scl);
      y *= gtk_adjustment_get_step_increment(y_adj);
      break;
    }
    return FALSE;
#endif
  default:
    return FALSE;
  }

  set_adjustment(x_adj, x);
  set_adjustment(y_adj, y);

  return FALSE;
}


static void
set_scroll_visibility(GtkWidget *scroll)
{
  GtkAdjustment *adj;
  gdouble min, max, page;

  if (scroll == NULL) {
    return;
  }

  adj = gtk_range_get_adjustment(GTK_RANGE(scroll));
  min = gtk_adjustment_get_lower(adj);
  max = gtk_adjustment_get_upper(adj);
  page = gtk_adjustment_get_page_size(adj);

  gtk_widget_set_visible(scroll, max - min > page);
}

static void
text_view_size_allocate(GtkWidget*widget, GdkRectangle *allocation, gpointer user_data)
{
  GtkWidget *scl;

  scl = g_object_get_data(G_OBJECT(widget), "hscroll");
  set_scroll_visibility(scl);

  scl = g_object_get_data(G_OBJECT(widget), "vscroll");
  set_scroll_visibility(scl);
}

#if GTK_CHECK_VERSION(3, 16, 0)
void
set_widget_font(GtkWidget *w, const char *font)
{
  GtkCssProvider *css_provider;
  GError *error;
  char *css_str;

  css_str = g_strdup_printf("* {font: %s;}", font);
  if (css_str == NULL) {
    return;
  }

  css_provider = gtk_css_provider_new();
  if (css_provider == NULL) {
    return;
  }

  error = NULL;
  gtk_css_provider_load_from_data(css_provider, css_str, -1, &error);
  g_free(css_str);
  if (error == NULL) {
    gtk_style_context_add_provider(gtk_widget_get_style_context(w),
				   GTK_STYLE_PROVIDER(css_provider),
				   GTK_STYLE_PROVIDER_PRIORITY_USER);
  }
}

#else  /* GTK_CHECK_VERSION(3, 16, 0) */

#define LINE_NUMBER_R 0xCC00
#define LINE_NUMBER_G 0xCC00
#define LINE_NUMBER_B 0xCC00

static void
set_linumber_color(GtkWidget *w)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkRGBA col;

  col.red   = LINE_NUMBER_R * 1.0 / 0xFFFF;
  col.green = LINE_NUMBER_G * 1.0 / 0xFFFF;
  col.blue  = LINE_NUMBER_B * 1.0 / 0xFFFF;
  col.alpha = 1.0;

  gtk_widget_override_background_color(w, GTK_STATE_FLAG_NORMAL, &col);
  gtk_widget_override_background_color(w, GTK_STATE_FLAG_ACTIVE, &col);
  gtk_widget_override_background_color(w, GTK_STATE_FLAG_PRELIGHT, &col);
  gtk_widget_override_background_color(w, GTK_STATE_FLAG_SELECTED, &col);
  gtk_widget_override_background_color(w, GTK_STATE_FLAG_INSENSITIVE, &col);

  col.red   = 0;
  col.green = 0;
  col.blue  = 0;
  col.alpha = 1.0;

  gtk_widget_override_color(w, GTK_STATE_FLAG_NORMAL, &col);
  gtk_widget_override_color(w, GTK_STATE_FLAG_ACTIVE, &col);
  gtk_widget_override_color(w, GTK_STATE_FLAG_PRELIGHT, &col);
  gtk_widget_override_color(w, GTK_STATE_FLAG_SELECTED, &col);
  gtk_widget_override_color(w, GTK_STATE_FLAG_INSENSITIVE, &col);
#else
  GdkColor col;

  col.red = LINE_NUMBER_R;
  col.green = LINE_NUMBER_G;
  col.blue = LINE_NUMBER_B;

  gtk_widget_modify_base(w, GTK_STATE_NORMAL, &col);
  gtk_widget_modify_base(w, GTK_STATE_ACTIVE, &col);
  gtk_widget_modify_base(w, GTK_STATE_PRELIGHT, &col);
  gtk_widget_modify_base(w, GTK_STATE_SELECTED, &col);
  gtk_widget_modify_base(w, GTK_STATE_INSENSITIVE, &col);

  col.red = 0;
  col.green = 0;
  col.blue = 0;

  gtk_widget_modify_text(w, GTK_STATE_NORMAL, &col);
  gtk_widget_modify_text(w, GTK_STATE_ACTIVE, &col);
  gtk_widget_modify_text(w, GTK_STATE_PRELIGHT, &col);
  gtk_widget_modify_text(w, GTK_STATE_SELECTED, &col);
  gtk_widget_modify_text(w, GTK_STATE_INSENSITIVE, &col);
#endif

  gtk_widget_set_sensitive(w, FALSE);
}

#endif	/* ! GTK_CHECK_VERSION(3, 16, 0) */

#if GTK_CHECK_VERSION(3, 0, 0)
static void (* get_preferred_width_org) (GtkWidget *w, gint *min, gint *natulal);
static void (* get_preferred_height_org) (GtkWidget *w, gint *min, gint *natulal);

static void
get_preferred_width(GtkWidget *w, gint *min, gint *natulal)
{
  get_preferred_width_org(w, min, natulal);
  if (*min > 240) {
    *min = 0;
  }
}

static void
get_preferred_height(GtkWidget *w, gint *min, gint *natulal)
{
  get_preferred_height_org(w, min, natulal);
  *min = 0;
}
#endif

GtkWidget *
create_text_view_with_line_number(GtkWidget **v)
{
  GtkWidget *view, *ln, *swin, *hs, *vs;
  GtkAdjustment *hadj, *vadj;
  GtkTextBuffer *buf;

  buf = gtk_text_buffer_new(NULL);
  view = gtk_text_view_new_with_buffer(buf);

  buf = gtk_text_buffer_new(NULL);
  ln = gtk_text_view_new_with_buffer(buf);
#if GTK_CHECK_VERSION(3, 12, 0)
  gtk_widget_set_margin_end(ln, 4);
#endif

#if GTK_CHECK_VERSION(3, 16, 0)
  gtk_widget_set_name(ln, LINE_NUMBER_WIDGET_NAME);
#else
  set_linumber_color(ln);
#endif

  g_object_set_data(G_OBJECT(view), "line_number", ln);

  gtk_widget_set_size_request(GTK_WIDGET(view), 240, 120);
  gtk_widget_set_size_request(GTK_WIDGET(ln),   -1,  120);

  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  gtk_text_view_set_editable(GTK_TEXT_VIEW(ln), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(ln), FALSE);

#if GTK_CHECK_VERSION(3, 4, 0)
  swin = gtk_grid_new();
#else
  swin = gtk_table_new(3, 2, FALSE);
#endif
#if GTK_CHECK_VERSION(3, 2, 0)
  hs = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
  vs = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
#else
  hs = gtk_hscrollbar_new(NULL);
  vs = gtk_vscrollbar_new(NULL);
#endif

  g_object_set_data(G_OBJECT(view), "hscroll", hs);
  g_object_set_data(G_OBJECT(view), "vscroll", vs);

  hadj = gtk_range_get_adjustment(GTK_RANGE(hs));
  vadj = gtk_range_get_adjustment(GTK_RANGE(vs));
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_scrollable_set_hadjustment(GTK_SCROLLABLE(view), hadj);
  gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(view), vadj);
  gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(ln), vadj);

  /* fix-me: is there any other way to set minimum size of GtkTextView? */
  get_preferred_width_org = GTK_WIDGET_GET_CLASS(view)->get_preferred_width;
  get_preferred_height_org = GTK_WIDGET_GET_CLASS(view)->get_preferred_height;
  GTK_WIDGET_GET_CLASS(view)->get_preferred_width = get_preferred_width;
  GTK_WIDGET_GET_CLASS(view)->get_preferred_height = get_preferred_height;
#else
  gtk_widget_set_scroll_adjustments(view, hadj, vadj);
  gtk_widget_set_scroll_adjustments(ln,   NULL, vadj);
#endif

  g_signal_connect(view, "scroll-event", G_CALLBACK(text_view_scroll_event), NULL);
  g_signal_connect(view, "size-allocate", G_CALLBACK(text_view_size_allocate), NULL);

#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_vexpand(ln, TRUE);
  gtk_grid_attach(GTK_GRID(swin), ln,   0, 0, 1, 1);

  gtk_widget_set_hexpand(view, TRUE);
  gtk_widget_set_vexpand(view, TRUE);
  gtk_grid_attach(GTK_GRID(swin), view, 1, 0, 1, 1);

  gtk_widget_set_vexpand(hs, TRUE);
  gtk_grid_attach(GTK_GRID(swin), hs,   0, 1, 2, 1);

  gtk_widget_set_vexpand(vs, TRUE);
  gtk_grid_attach(GTK_GRID(swin), vs,   2, 0, 1, 1);
#else
  gtk_table_attach(GTK_TABLE(swin), ln,   0, 1, 0, 1,
		   0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(swin), view, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(swin), hs,   0, 2, 1, 2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(swin), vs,   2, 3, 0, 1,
		   GTK_FILL, GTK_FILL, 0, 0);
#endif

  if (v) {
    *v = view;
  }

  return swin;
}

void
text_view_with_line_number_set_text(GtkWidget *view, const gchar *str)
{
  GtkWidget *ln;
  GtkTextBuffer *buf, *ln_buf;
  int p, i, n;
  GString *s;
  gchar *ptr;
  GtkTextIter start, end;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  gtk_text_buffer_set_text(buf, str, -1);

  ln = g_object_get_data(G_OBJECT(view), "line_number");
  if (ln == NULL) {
    return;
  }

  s = g_string_sized_new(256);
  if (s == NULL) {
    return;
  }

  n = gtk_text_buffer_get_line_count(buf);
  ln_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(ln));
  p = ceil(log10(n + 1));
  for (i = 0; i < n; i++) {
    g_string_append_printf(s, "%*d \n", p, i + 1);
  }
  ptr = g_string_free(s, FALSE);
  gtk_text_buffer_set_text(ln_buf, ptr, -1);
  g_free(ptr);

  gtk_text_buffer_get_iter_at_offset(ln_buf, &start, 0);
  gtk_text_buffer_get_iter_at_offset(ln_buf, &end, -1);
}

void
text_view_with_line_number_set_font(GtkWidget *view, const gchar *font)
{
#if ! GTK_CHECK_VERSION(3, 16, 0)
  PangoFontDescription *desc;
#endif
  GtkWidget *ln;

#if GTK_CHECK_VERSION(3, 16, 0)
  set_widget_font(view, font);
#else  /* GTK_CHECK_VERSION(3, 16, 0) */
  desc = pango_font_description_from_string(font);
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_widget_override_font(view, NULL);
  gtk_widget_override_font(view, desc);
#else
  gtk_widget_modify_font(view, NULL);
  gtk_widget_modify_font(view, desc);
#endif
#endif	/* GTK_CHECK_VERSION(3, 16, 0) */

  ln = g_object_get_data(G_OBJECT(view), "line_number");
  if (ln == NULL) {
#if ! GTK_CHECK_VERSION(3, 16, 0)
    pango_font_description_free(desc);
#endif
    return;
  }

#if GTK_CHECK_VERSION(3, 16, 0)
  set_widget_font(ln, font);
#else  /* GTK_CHECK_VERSION(3, 16, 0) */
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_widget_override_font(ln, NULL);
  gtk_widget_override_font(ln, desc);
#else
  gtk_widget_modify_font(ln, NULL);
  gtk_widget_modify_font(ln, desc);
#endif

  pango_font_description_free(desc);
#endif	/* GTK_CHECK_VERSION(3, 16, 0) */
}

enum SELECT_OBJ_COLOR_RESULT
select_obj_color(struct objlist *obj, int id, enum OBJ_FIELD_COLOR_TYPE type)
{
  GtkWidget *dlg;
  int r, g, b, a, rr ,gg, bb, aa, response;
#if GTK_CHECK_VERSION(3, 4, 0)
  GdkRGBA color;
#else
  GtkWidget *sel;
  GdkColor color;
#endif
  char *title;

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
    return SELECT_OBJ_COLOR_ERROR;
  }

  if (! Menulocal.use_opacity) {
    a = 255;
  }

#if GTK_CHECK_VERSION(3, 4, 0)
  color.red = r / 255.0;
  color.green = g / 255.0;
  color.blue = b / 255.0;
  color.alpha = a / 255.0;

  dlg = gtk_color_chooser_dialog_new(title, GTK_WINDOW(NgraphApp.FileWin.Win));
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dlg), &color);
  gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(dlg), Menulocal.use_opacity);

  response = ndialog_run(dlg);
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dlg), &color);

  gtk_widget_destroy(dlg);

  if (response != GTK_RESPONSE_OK) {
    return SELECT_OBJ_COLOR_CANCEL;
  }

  rr = nround(color.red * 255);
  gg = nround(color.green * 255);
  bb = nround(color.blue * 255);
  aa = nround(color.alpha * 255);
#else
  color.red = (r & 0xffU) * 257;
  color.green = (g & 0xffU) * 257;
  color.blue = (b & 0xffU) * 257;

  dlg = gtk_color_selection_dialog_new(title);
  sel = gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dlg));

  gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(sel), TRUE);
  gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(sel), Menulocal.use_opacity);

  gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(sel), &color);
  gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(sel), (a & 0xffU) * 257);

  response = ndialog_run(dlg);
  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(sel), &color);
  aa = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(sel));
  gtk_widget_destroy(dlg);

  if (response != GTK_RESPONSE_OK) {
    return SELECT_OBJ_COLOR_CANCEL;
  }

  rr = (color.red >> 8);
  gg = (color.green >> 8);
  bb = (color.blue >> 8);
  aa >>= 8;
#endif

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
    return SELECT_OBJ_COLOR_ERROR;
  }

  return (rr == r && gg == g && bb == b && aa == a) ? SELECT_OBJ_COLOR_SAME : SELECT_OBJ_COLOR_DIFFERENT;
}
