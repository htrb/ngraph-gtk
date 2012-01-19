/*
 * $Id: gtk_liststore.h,v 1.11 2009-08-19 06:44:16 hito Exp $
 */

#ifndef _GTK_LISTSTORE_HEADER
#define _GTK_LISTSTORE_HEADER

#define OBJECT_COLUMN_TYPE_STRING 0
#define OBJECT_COLUMN_TYPE_PIXBUF 1
#define OBJECT_COLUMN_TYPE_INT    2

typedef struct _list_store {
  char *title;
  GType type;
  gboolean visible, editable;
  char *name;
  gboolean color;
  int min, max, inc, page;
  PangoEllipsizeMode ellipsize;
  gulong edited_id;
} n_list_store;

GtkWidget *create_object_cbox(void);

GtkWidget *list_store_create(int n, n_list_store *list);

void list_store_set_val(GtkWidget *w, GtkTreeIter *iter, int col, GType type, void *ptr);
void list_store_set_sort_all(GtkWidget *tview);
void list_store_set_sort_column(GtkWidget *tview, int col);
void list_store_set_align(GtkWidget *tview, int col, double align);

int list_store_get_int(GtkWidget *w, GtkTreeIter *iter, int col);
void list_store_set_int(GtkWidget *w, GtkTreeIter *iter, int col, int v);
int list_store_path_get_int(GtkWidget *w, GtkTreePath *path, int col, int *val);
void list_store_set_double(GtkWidget *w, GtkTreeIter *iter, int col, double v);
void tree_store_set_double(GtkWidget *w, GtkTreeIter *iter, int col, double v);
char *list_store_get_string(GtkWidget *w, GtkTreeIter *iter, int col);
void list_store_set_string(GtkWidget *w, GtkTreeIter *iter, int col, const char *v);
gboolean list_store_get_boolean(GtkWidget *w, GtkTreeIter *iter, int col);
void list_store_set_boolean(GtkWidget *w, GtkTreeIter *iter, int col, int v);
void list_store_path_set_string(GtkWidget *w, GtkTreePath *path, int col, const char *v);
char *list_store_path_get_string(GtkWidget *w, GtkTreePath *path, int col);

gboolean list_store_get_iter_first(GtkWidget *w, GtkTreeIter *iter);
gboolean list_store_iter_next(GtkWidget *w, GtkTreeIter *iter);
void list_store_append(GtkWidget *w, GtkTreeIter *iter);
void list_store_clear(GtkWidget *w);
gboolean list_store_get_selected_iter(GtkWidget *w, GtkTreeIter *iter);
int list_store_get_selected_int(GtkWidget *w, int col);
void list_store_select_int(GtkWidget *w, int col, int id);
char *list_store_get_selected_string(GtkWidget *w, int col);
int list_store_get_num(GtkWidget *w);
void list_store_set_selection_mode(GtkWidget *w, GtkSelectionMode mode);
void list_store_select_nth(GtkWidget *w, int n);
void list_store_select_iter(GtkWidget *w, GtkTreeIter *iter);
void list_store_multi_select_nth(GtkWidget *w, int n, int m);
gboolean list_store_get_selected_nth(GtkWidget *w, int *n);
int list_store_get_selected_index(GtkWidget *w);
void list_store_set_pixbuf(GtkWidget *w, GtkTreeIter *iter, int col, GdkPixbuf *v);
GdkPixbuf *list_store_get_pixbuf(GtkWidget *w, GtkTreeIter *iter, int col);
void list_store_select_all(GtkWidget *w);

GtkWidget *tree_store_create(int n, n_list_store *list);
void tree_store_append(GtkWidget *w, GtkTreeIter *iter, GtkTreeIter *parent);
void tree_store_prepend(GtkWidget *w, GtkTreeIter *iter, GtkTreeIter *parent);
gboolean tree_store_get_iter_children(GtkWidget *w, GtkTreeIter *child, GtkTreeIter *iter);
gboolean tree_store_get_selected_nth(GtkWidget *w, int *n, int *m);
void tree_store_select_nth(GtkWidget *w, int n, int m);
void tree_store_clear(GtkWidget *w);
int tree_store_get_child_num(GtkWidget *w, GtkTreeIter *iter);
void tree_store_set_int(GtkWidget *w, GtkTreeIter *iter, int col, int v);
void tree_store_set_string(GtkWidget *w, GtkTreeIter *iter, int col, const char *v);
void tree_store_set_boolean(GtkWidget *w, GtkTreeIter *iter, int col, int v);
void tree_store_set_val(GtkWidget *w, GtkTreeIter *iter, int col, GType type, void *ptr);

void list_store_select_all_cb(GtkButton *w, gpointer client_data);
void list_store_remove_selected_cb(GtkWidget *w, gpointer client_data);
void free_tree_path_cb(gpointer data, gpointer user_data);

void tree_store_selected_toggle_expand(GtkWidget *w);

#define tree_store_get_int		list_store_get_int
#define tree_store_get_boolean		list_store_get_boolean
#define tree_store_get_string		list_store_get_string
#define tree_store_get_iter_first	list_store_get_iter_first
#define tree_store_iter_next		list_store_iter_next
#define tree_store_get_selected_iter	list_store_get_selected_iter
#define tree_store_set_selection_mode	list_store_set_selection_mode
#define tree_store_path_get_string	list_store_path_get_string
#define tree_store_set_sort_all		list_store_set_sort_all
#define tree_store_set_sort_column	list_store_set_sort_column
#define tree_store_select_all_cb	list_store_select_all_cb
#define tree_store_set_align		list_store_set_align
#endif
