/* 
 * $Id: gtk_subwin.h,v 1.14 2009-07-22 14:53:31 hito Exp $
 */

#ifndef _GTK_SUBWIN_HEADER
#define _GTK_SUBWIN_HEADER

#include "gtk_liststore.h"

#include "ngraph.h"
#include "object.h"
#include "ogra2cairo.h"
#include "x11menu.h"
#include "ox11menu.h"

#define COL_ID 1

typedef void (* list_sub_window_set_val_func) (struct SubWin *d, GtkTreeIter *iter, int i);

struct subwin_popup_list {
  char *title;
  GCallback func;
  int use_stock;
  GtkAccelGroup *accel_group;
  enum pop_up_menu_item_type type;
};

GtkWidget *label_sub_window_create(struct SubWin *d, char *title, const char **xpm, const char **xpm2);
GtkWidget *text_sub_window_create(struct SubWin *d, char *title, const char **xpm, const char **xpm2);
GtkWidget *list_sub_window_create(struct SubWin *d, char *title, int lisu_num, n_list_store *list, const char **xpm, const char **xpm2);
GtkWidget *tree_sub_window_create(struct LegendWin *d, char *title, int lisu_num, n_list_store *list, const char **xpm, const char **xpm2);
void sub_window_minimize(void *d);
void sub_window_restore_state(void *d);
void sub_window_set_geometry(struct SubWin *d, int resize);
void sub_window_save_geometry(struct SubWin *d);
void sub_window_show_all(struct SubWin *d);
void sub_window_set_visibility(struct SubWin *d, int state);

void set_editable_cell_renderer_cb(struct SubWin *d, int i, n_list_store *list, GCallback end);
void set_combo_cell_renderer_cb(struct SubWin *d, int col, n_list_store *list, GCallback start, GCallback end);
void set_obj_cell_renderer_cb(struct SubWin *d, int col, n_list_store *list, GCallback start);

gboolean list_sub_window_must_rebuild(struct SubWin *d);
void list_sub_window_build(struct SubWin *d, list_sub_window_set_val_func func);
void list_sub_window_set(struct SubWin *d, list_sub_window_set_val_func func);

void list_sub_window_delete(GtkMenuItem *item, gpointer user_data);
void list_sub_window_copy(GtkMenuItem *item, gpointer user_data);
void list_sub_window_move_top(GtkMenuItem *item, gpointer user_data);
void list_sub_window_move_last(GtkMenuItem *item, gpointer user_data);
void list_sub_window_move_up(GtkMenuItem *item, gpointer user_data);
void list_sub_window_move_down(GtkMenuItem *item, gpointer user_data);
void list_sub_window_update(GtkMenuItem *item, gpointer user_data);
void list_sub_window_hide(GtkMenuItem *item, gpointer user_data);
void list_sub_window_focus(GtkMenuItem *item, gpointer user_data);
void list_sub_window_add_focus(GtkMenuItem *item, gpointer user_data);

void tree_sub_window_delete(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_copy(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_move_top(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_move_last(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_move_up(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_move_down(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_update(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_hide(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_focus(GtkMenuItem *item, gpointer user_data);
void tree_sub_window_add_focus(GtkMenuItem *item, gpointer user_data);

GtkWidget *sub_win_create_popup_menu(struct SubWin *d, int n, struct subwin_popup_list *list, GCallback cb);

#endif
