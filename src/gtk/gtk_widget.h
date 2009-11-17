#ifndef GTK_WIDGET_HEADER
#define GTK_WIDGET_HEADER

#define SPIN_ENTRY_MAX 100000
#define NUM_ENTRY_WIDTH 80

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

char *filename_from_utf8(const char *str);
char *filename_to_utf8(const char *str);

GtkWidget *create_spin_entry_type(enum SPIN_BUTTON_TYPE type, int set_default_size, int set_default_action);
GtkWidget *create_spin_entry(int min, int max, int inc, int set_default_size, int set_default_action);
void spin_entry_set_val(GtkWidget *entry, int val);
int spin_entry_get_val(GtkWidget *entry);
void spin_entry_set_range(GtkWidget *w, int min, int max);
void spin_entry_set_inc(GtkWidget *w, int inc, int page);
char *entry_get_filename(GtkWidget *w);
int entry_set_filename(GtkWidget *w, char *filename);

GtkWidget *create_color_button(GtkWidget *win);
GtkWidget *create_text_entry(int set_default_size, int set_default_action);
GtkWidget *create_file_entry(struct objlist *obj);
GtkWidget *create_direction_entry(void);
GtkWidget *item_setup(GtkWidget *box, GtkWidget *w, char *title, gboolean expand);
GtkWidget *get_parent_window(GtkWidget *w);

#endif
