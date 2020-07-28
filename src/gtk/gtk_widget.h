#ifndef GTK_WIDGET_HEADER
#define GTK_WIDGET_HEADER

#include "object.h"

#define SPIN_ENTRY_MAX 1000000
#define NUM_ENTRY_WIDTH 12

enum SPIN_BUTTON_TYPE {
  SPIN_BUTTON_TYPE_WIDTH,
  SPIN_BUTTON_TYPE_LENGTH,
  SPIN_BUTTON_TYPE_POSITION,
  SPIN_BUTTON_TYPE_ANGLE,
  SPIN_BUTTON_TYPE_POINT,
  SPIN_BUTTON_TYPE_SPACE_POINT,
  SPIN_BUTTON_TYPE_PERCENT,
  SPIN_BUTTON_TYPE_INT,
  SPIN_BUTTON_TYPE_UINT,
  SPIN_BUTTON_TYPE_NUM,
  SPIN_BUTTON_TYPE_NATURAL,
  SPIN_BUTTON_TYPE_CUSTOM,
};

enum OBJ_FIELD_COLOR_TYPE {
  OBJ_FIELD_COLOR_TYPE_0,
  OBJ_FIELD_COLOR_TYPE_1,
  OBJ_FIELD_COLOR_TYPE_2,
  OBJ_FIELD_COLOR_TYPE_FILL,
  OBJ_FIELD_COLOR_TYPE_STROKE,
  OBJ_FIELD_COLOR_TYPE_AXIS_BASE,
  OBJ_FIELD_COLOR_TYPE_AXIS_GAUGE,
  OBJ_FIELD_COLOR_TYPE_AXIS_NUM,
};

enum SELECT_OBJ_COLOR_RESULT {
  SELECT_OBJ_COLOR_DIFFERENT,
  SELECT_OBJ_COLOR_SAME,
  SELECT_OBJ_COLOR_ERROR,
  SELECT_OBJ_COLOR_CANCEL,
};

enum WIDGET_MARGIN {
  WIDGET_MARGIN_LEFT = 1,
  WIDGET_MARGIN_RIGHT = 2,
  WIDGET_MARGIN_TOP = 4,
  WIDGET_MARGIN_BOTTOM = 8,
};

GtkWidget *create_spin_entry_type(enum SPIN_BUTTON_TYPE type, int set_default_size, int set_default_action);
GtkWidget *create_spin_entry(int min, int max, int inc, int set_default_size, int set_default_action);

void set_button_icon(GtkWidget *w, const char *icon_name);

void spin_entry_set_val(GtkWidget *entry, int val);
int spin_entry_get_val(GtkWidget *entry);
void spin_entry_set_range(GtkWidget *w, int min, int max);
void spin_entry_set_inc(GtkWidget *w, int inc, int page);
char *entry_get_filename(GtkWidget *w);
int entry_set_filename(GtkWidget *w, char *filename);

GtkWidget *create_color_button(GtkWidget *win);
GtkWidget *create_text_entry(int set_default_size, int set_default_action);
GtkWidget *create_number_entry(int set_default_size, int set_default_action);
GtkWidget *create_file_entry(struct objlist *obj);
GtkWidget *create_file_entry_with_cb(GCallback cb, gpointer data);
GtkWidget *create_direction_entry(void);
GtkWidget *item_setup(GtkWidget *box, GtkWidget *w, char *title, gboolean expand);
GtkWidget *get_parent_window(GtkWidget *w);
GtkWidget *add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int col_max, int n);
GtkWidget *add_widget_to_table(GtkWidget *table, GtkWidget *w, char *title, int expand, int n);
GtkWidget *add_copy_button_to_box(GtkWidget *parent_box, GCallback cb, gpointer d, char *obj_name);
GtkWidget *get_mnemonic_label(GtkWidget *w);
GtkWidget *create_text_view_with_line_number(GtkWidget **v);
void text_view_with_line_number_set_text(GtkWidget *view, const gchar *str);
void text_view_with_line_number_set_font(GtkWidget *view, const gchar *font);
void set_widget_sensitivity_with_label(GtkWidget *w, gboolean state);
void set_widget_visibility_with_label(GtkWidget *w, gboolean state);
void combo_box_create_mark(GtkWidget *cbox, GtkTreeIter *parent, int col_id, int type);
enum SELECT_OBJ_COLOR_RESULT select_obj_color(struct objlist *obj, int id, enum OBJ_FIELD_COLOR_TYPE type);
void set_widget_margin(GtkWidget *w, int margin_pos);
void set_scale_mark(GtkWidget *scale, GtkPositionType pos, int start, int inc);
#if GTK_CHECK_VERSION(3, 16, 0)
void set_widget_font(GtkWidget *w, const char *font);
#endif
void add_default_color(struct narray *palette);
void add_default_gray(struct narray *palette);
gchar *get_text_from_buffer(GtkTextBuffer *buffer);
GtkWidget *add_button(GtkWidget *grid, int row, int col, const char *icon, const char *tooltip, GCallback proc, gpointer data);
GtkWidget *add_toggle_button(GtkWidget *grid, int row, int col, const char *icon_name, const char *tooltip, GCallback proc, gpointer data);

#endif
