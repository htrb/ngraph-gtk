#ifndef ADDIN_COMMON_HEADER
#define ADDIN_COMMON_HEADER

GtkWidget *create_text_entry(int set_default_action);
GtkWidget *add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int n);
int warning_dialog(GtkWidget *parent, const char *msg, const char *str);
char *get_text_from_entry(GtkWidget *entry);
int fgets_int(FILE *fp);
double fgets_double(FILE *fp);
char *fgets_str(FILE *fp);
GtkWidget *create_title(const char *name, const char *comment);

#endif
