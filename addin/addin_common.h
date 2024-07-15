#ifndef ADDIN_COMMON_HEADER
#define ADDIN_COMMON_HEADER

struct font_prm {
  GtkWidget *font, *pt, *space, *script, *color, *bold, *italic;
};

GtkWidget *create_text_entry(int set_default_action);
GtkWidget *create_spin_button(double min, double max, double inc, double init, int digit);
GtkWidget *add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int n);
int warning_dialog(GtkWidget *parent, const char *msg, const char *str);
char *get_text_from_entry(GtkWidget *entry);
int fgets_int(FILE *fp);
double fgets_double(FILE *fp);
char *fgets_str(FILE *fp);
GtkWidget *create_title(const char *name, const char *comment);
const char *get_selected_font(struct font_prm *prm);
GtkWidget *create_font_frame(struct font_prm *prm);
void get_font_parameter(struct font_prm *prm, int *pt, int *spc, int *script, int *style, int *r, int *g, int *b);
GtkWidget *dialog_new(const char *title, GCallback cancel_cb, GCallback ok_cb, gpointer user_data);
GtkWidget *columnview_create(GType item_type);
GtkColumnViewColumn *create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, gpointer user_data);

#endif
