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

GtkWidget *
get_widget(GtkWidget *w)
{
  GtkWidget *widget;

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_LABEL)) {
    widget = gtk_label_get_mnemonic_widget(GTK_LABEL(w));
  } else {
    widget = w;
  }

  return widget;
}

GtkWidget *
add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int col_max, int n)
{
  GtkWidget *align, *label;
  int x, y;

  g_object_get(table, "n-columns", &x, "n-rows", &y, NULL);

  x = (x > col_max) ? x : col_max;
  y = (y > n + 1) ? y : n + 1;
  gtk_table_resize(GTK_TABLE(table), y, x);

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, n, n + 1, GTK_FILL, 0, 4, 4);
    col++;
  }

  if (w) {
    align = gtk_alignment_new(0, 0.5, (expand) ? 1 : 0, 0);
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, col, col + width, n, n + 1, ((expand) ? GTK_EXPAND : 0) | GTK_FILL, 0, 4, 4);
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

  hbox = gtk_hbox_new(FALSE, 4);
  w = gtk_button_new_with_mnemonic(_("_Copy Settings"));
  g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), obj_name);
  g_signal_connect(w, "clicked", cb, d);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(parent_box), hbox, FALSE, FALSE, 4);

  return hbox;
}

GtkWidget *
item_setup(GtkWidget *box, GtkWidget *w, char *title, gboolean expand)
{
  GtkWidget *hbox, *label;

  hbox = gtk_hbox_new(FALSE, 4);
  label = gtk_label_new_with_mnemonic(title);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), w, expand, expand, 2);
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


#if USE_ENTRY_ICON
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
#else
GCallback entry_icon_file_select = NULL;
#endif

GtkWidget *
create_file_entry_with_cb(GCallback cb, gpointer data)
{
  GtkWidget *w;

  w = create_text_entry(TRUE, TRUE);

#if USE_ENTRY_ICON
  gtk_entry_set_icon_from_stock(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);
  g_signal_connect(w, "icon-release", cb, data);
#endif

  return w;
}

GtkWidget *
create_file_entry(struct objlist *obj)
{
  return create_file_entry_with_cb(G_CALLBACK(entry_icon_file_select), obj);
}

#if USE_ENTRY_ICON
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
#endif

GtkWidget *
create_direction_entry(void)
{
  GtkWidget *w;

#if USE_ENTRY_ICON
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, FALSE, TRUE);
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  gtk_entry_set_icon_from_stock(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_GO_UP);
  gtk_entry_set_icon_from_stock(GTK_ENTRY(w), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_GO_DOWN);
  g_signal_connect(w, "icon-release", G_CALLBACK(direction_icon_released), NULL);
#else
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, TRUE, TRUE);
#endif

  return w;
}



GtkWidget *
create_text_entry(int set_default_size, int set_default_action)
{
  GtkWidget *w;

  w = gtk_entry_new();
  if (set_default_size)
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH, -1);

  if (set_default_action)
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);

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
spin_entry_set_val(GtkWidget *w, int ival)
{
  gdouble min, max, val;
  enum SPIN_BUTTON_TYPE type;
  GtkWidget *entry;

  entry = get_widget(w);

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
spin_entry_get_val(GtkWidget *w)
{
  gdouble val;
  enum SPIN_BUTTON_TYPE type;
  GtkWidget *entry;

  entry = get_widget(w);

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
 
  if (set_default_size)
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH, -1);

  if (set_default_action)
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);

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
  sel = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG_GET_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dlg)));

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
  case GDK_space:
  case GDK_Return:
    if (e->type == GDK_KEY_RELEASE) {
      show_color_sel(w, NULL, u);
    }
    return TRUE;
  }

  return FALSE;
}

GtkWidget *
create_color_button(GtkWidget *win)
{
  GtkWidget *w;

  w = gtk_color_button_new();
  gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(w), Menulocal.use_opacity);
  g_signal_connect(w, "button-release-event", G_CALLBACK(show_color_sel), win);
  g_signal_connect(w, "key-press-event", G_CALLBACK(color_button_key_event), win);
  g_signal_connect(w, "key-release-event", G_CALLBACK(color_button_key_event), win);

  return w;
}
