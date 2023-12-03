#ifndef GTK_LISTVIEW_HEADER
#define GTK_LISTVIEW_HEADER

#include "gtk_common.h"
#include "gtk_columnview.h"

GtkWidget *listview_create(enum N_SELECTION_TYPE selection_type, GCallback setup, GCallback bind, gpointer user_data);
GtkStringList *listview_get_string_list(GtkWidget *listview);
void listview_set_header(GtkWidget *listview, char *header);
void listview_select_all(GtkWidget *listview);

#endif
