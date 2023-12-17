/*
 * $Id: gtk_liststore.h,v 1.11 2009-08-19 06:44:16 hito Exp $
 */

#ifndef _GTK_LISTSTORE_HEADER
#define _GTK_LISTSTORE_HEADER

#include "gtk_common.h"
#include "object.h"

enum TOGGLE_TYPE {
  TOGGLE_NONE,
  TOGGLE_CHECK,
  TOGGLE_RADIO,
};

enum OBJECT_COLUMN_TYPE {
  OBJECT_COLUMN_TYPE_TOGGLE,
  OBJECT_COLUMN_TYPE_STRING,
  OBJECT_COLUMN_TYPE_PIXBUF,
  OBJECT_COLUMN_TYPE_INT,
  OBJECT_COLUMN_TYPE_ENUM,
  OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE,
  OBJECT_COLUMN_TYPE_TOGGLE_IS_RADIO,
  OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE,
  OBJECT_COLUMN_TYPE_NUM,
};


void tree_view_set_tooltip_column(GtkTreeView *tree_view, gint column);
void tree_view_set_no_expand_column(GtkWidget *tview, const int *columns, int n);

void init_object_combo_box(GtkWidget *cbox);

void add_separator_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent);
void add_line_style_item_to_cbox(GtkTreeStore *list, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id);
void add_bool_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id, const char *title);
void add_mark_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id);
void add_enum_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id, GtkTreeIter *selected);
void add_font_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iterp, GtkTreeIter *parent, int column_id, struct objlist *obj, const char *field, int id, GtkTreeIter *active);
void add_text_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id, int enum_id, const char *title, enum TOGGLE_TYPE type, int active);
void add_font_style_combo_item_to_cbox(GtkTreeStore *list, GtkTreeIter *iter, GtkTreeIter *parent, int column_id_bold, int column_id_italic, struct objlist *obj, const char *field, int id);

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

void tree_store_selected_toggle_expand(GtkWidget *w);

int tree_view_get_selected_row_int_from_path(GtkWidget *view, gchar *path, GtkTreeIter *iter, int col);

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
