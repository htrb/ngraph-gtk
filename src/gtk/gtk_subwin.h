/*
 * $Id: gtk_subwin.h,v 1.14 2009-07-22 14:53:31 hito Exp $
 */

#ifndef _GTK_SUBWIN_HEADER
#define _GTK_SUBWIN_HEADER

#include "common.h"

#include "gtk_common.h"

#include "object.h"
#include "ogra2cairo.h"
#include "x11menu.h"
#include "ox11menu.h"

#define COL_ID 1

typedef void (* list_sub_window_set_val_func) (struct obj_list_data *d, GtkTreeIter *iter, int i);

struct subwin_popup_list {
  char *title;
  GCallback func;
  struct subwin_popup_list *submenu;
  enum pop_up_menu_item_type type;
};

GtkWidget *label_sub_window_create(struct SubWin *d);
GtkWidget *text_sub_window_create(struct SubWin *d);
GtkWidget *list_sub_window_create(struct SubWin *d, int lisu_num, n_list_store *list);
GtkWidget *parameter_sub_window_create(struct SubWin *d);
GtkWidget *tree_sub_window_create(struct SubWin *d, int page_num, int *lisu_num, n_list_store **list, GtkWidget **icons);
void sub_window_set_geometry(struct SubWin *d, int resize);
void sub_window_save_geometry(struct SubWin *d);
void sub_window_set_visibility(struct SubWin *d, int state);
void sub_window_save_visibility(struct SubWin *d);
//GtkWidget *sub_window_get_nth_content(struct LegendWin *d, int n);

void set_editable_cell_renderer_cb(struct obj_list_data *d, int i, n_list_store *list, GCallback end);
void set_combo_cell_renderer_cb(struct obj_list_data *d, int col, n_list_store *list, GCallback start, GCallback end);
void set_obj_cell_renderer_cb(struct obj_list_data *d, int col, n_list_store *list, GCallback start);
void set_cell_attribute_source(struct SubWin *d, const char *attr, int target_column, int source_column);

gboolean list_sub_window_must_rebuild(struct obj_list_data *d);
void list_sub_window_build(struct obj_list_data *d);

void list_sub_window_object_name(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_delete(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_copy(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_move_top(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_move_last(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_move_up(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_move_down(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_update(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_focus(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void list_sub_window_focus_all(GSimpleAction *action, GVariant *parameter, gpointer user_data);
void update_viewer(struct obj_list_data *d);

void sub_win_create_popup_menu(struct obj_list_data *d, int n, GActionEntry *list, GCallback cb);
#endif
