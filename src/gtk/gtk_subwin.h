/* 
 * $Id: gtk_subwin.h,v 1.4 2008/06/30 13:03:22 hito Exp $
 */

#ifndef _GTK_SUBWIN_HEADER
#define _GTK_SUBWIN_HEADER

#include "gtk_liststore.h"

#include "ngraph.h"
#include "object.h"
#include "ogra2cairo.h"
#include "x11menu.h"
#include "ox11menu.h"

typedef void (* list_sub_window_set_val_func) (struct SubWin *d, GtkTreeIter *iter, int i);

struct subwin_popup_list {
  char *title;
  GCallback func;
  int use_stock;
  GtkAccelGroup *accel_group;
};

GtkWidget *text_sub_window_create(struct SubWin *d, char *title, char **xpm);
GtkWidget *list_sub_window_create(struct SubWin *d, char *title, int lisu_num, n_list_store *list, char **xpm);
GtkWidget *tree_sub_window_create(struct LegendWin *d, char *title, int lisu_num, n_list_store *list, char **xpm);
void sub_window_minimize(void *d);
void sub_window_restore_state(void *d);

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
