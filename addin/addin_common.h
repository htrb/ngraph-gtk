#ifndef ADDIN_COMMON_HEADER
#define ADDIN_COMMON_HEADER

#if GTK_CHECK_VERSION(3, 2, 0)
#define gtk_hbox_new(h, s) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, s)
#define gtk_vbox_new(h, s) gtk_box_new(GTK_ORIENTATION_VERTICAL, s)
#endif

struct font_prm {
  GtkWidget *font, *pt, *space, *script, *color, *bold, *italic;
};

GtkWidget *create_text_entry(int set_default_action);
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

#endif
