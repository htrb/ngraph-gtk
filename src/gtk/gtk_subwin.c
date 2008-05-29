/* 
 * $Id: gtk_subwin.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>

#include "ngraph.h"
#include "object.h"
#include "nstring.h"

#include "main.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11view.h"
#include "x11gui.h"
#include "gtk_liststore.h"

#include "gtk_subwin.h"

#define COL_ID 1

static void
get_geometry(struct SubWin *d, int *x, int *y, int *w, int *h)
{
  switch (d->type) {
  case TypeFileWin:
    *w = Menulocal.filewidth;
    *h = Menulocal.fileheight;
    *x = Menulocal.filex;
    *y = Menulocal.filey;
    break;
  case TypeAxisWin:
    *w = Menulocal.axiswidth;
    *h = Menulocal.axisheight;
    *x = Menulocal.axisx;
    *y = Menulocal.axisy;
    break;
  case TypeLegendWin:
    *w = Menulocal.legendwidth;
    *h = Menulocal.legendheight;
    *x = Menulocal.legendx;
    *y = Menulocal.legendy;
    break;
  case TypeMergeWin:
    *w = Menulocal.mergewidth;
    *h = Menulocal.mergeheight;
    *x = Menulocal.mergex;
    *y = Menulocal.mergey;
    break;
  case TypeInfoWin:
    *w = Menulocal.dialogwidth;
    *h = Menulocal.dialogheight;
    *x = Menulocal.dialogx;
    *y = Menulocal.dialogy;
    break;
  case TypeCoordWin:
    *w = Menulocal.coordwidth;
    *h = Menulocal.coordheight;
    *x = Menulocal.coordx;
    *y = Menulocal.coordy;
    break;
  }
}

static void
set_geometry(struct SubWin *d, int x, int y, int w, int h)
{
  switch (d->type) {
  case TypeFileWin:
    Menulocal.filewidth = w;
    Menulocal.fileheight = h;
    Menulocal.filex = x;
    Menulocal.filey = y;
    break;
  case TypeAxisWin:
    Menulocal.axiswidth = w;
    Menulocal.axisheight = h;
    Menulocal.axisx = x;
    Menulocal.axisy = y;
    break;
  case TypeLegendWin:
    Menulocal.legendwidth = w;
    Menulocal.legendheight = h;
    Menulocal.legendx = x;
    Menulocal.legendy = y;
    break;
  case TypeMergeWin:
    Menulocal.mergewidth = w;
    Menulocal.mergeheight = h;
    Menulocal.mergex = x;
    Menulocal.mergey = y;
    break;
  case TypeInfoWin:
    Menulocal.dialogwidth = w;
    Menulocal.dialogheight = h;
    Menulocal.dialogx = x;
    Menulocal.dialogy = y;
    break;
  case TypeCoordWin:
    Menulocal.coordwidth = w;
    Menulocal.coordheight = h;
    Menulocal.coordx = x;
    Menulocal.coordy = y;
    break;
  }
}

#define DEFAULT_WIDTH 240
#define DEFAULT_HEIGHT 320

static void
cb_show(GtkWidget *widget, gpointer user_data)
{
  struct SubWin *d;
  int w, h, x, y, x0, y0;

  d = (struct SubWin *) user_data;

  if (d->Win == NULL) return;

  get_geometry(d, &x, &y, &w, &h);

  if (w == CW_USEDEFAULT) {
    w = DEFAULT_WIDTH;
  }

  if (h == CW_USEDEFAULT) {
    h = DEFAULT_HEIGHT;
  }

  if ((x != CW_USEDEFAULT) && (y != CW_USEDEFAULT)) {
    gtk_window_get_position(GTK_WINDOW(TopLevel), &x0, &y0);
    x += x0;
    y += y0;

    if (x < 0) {
      x = 0;
    }
    if (y < 0) {
      y = 0;
    }
    gtk_window_move(GTK_WINDOW(d->Win), x, y);
  }

  gtk_window_set_default_size(GTK_WINDOW(d->Win), w, h);
}

static void
cb_hide(GtkWidget *widget, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  if (d->Win) {
    gint x, y, x0, y0, w0, h0;

    gtk_window_get_position(GTK_WINDOW(TopLevel), &x0, &y0);
    gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
    gtk_window_get_size(GTK_WINDOW(d->Win), &w0, &h0);

    set_geometry(d, x - x0, y - y0, w0, h0);

  }
}

static gboolean
cb_del(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  gtk_widget_hide_all(w);

  return TRUE;
}

static void
cb_destroy(GtkWidget *w, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  d->Win = NULL;

  if (d->popup)
    gtk_widget_destroy(d->popup);

  d->popup = NULL;

  if (d->popup_item)
    free(d->popup_item);

  d->popup_item = NULL;
}

static void
obj_copy(struct objlist *obj, int dest, int src)
{
  int j, perm, type;
  char *field;

  for (j = 0; j < chkobjfieldnum(obj); j++) {
    field = chkobjfieldname(obj, j);
    perm = chkobjperm(obj, field);
    type = chkobjfieldtype(obj, field);
    if (strcmp2(field, "name") && (perm & NREAD) && (perm & NWRITE) && (type < NVFUNC))
      copyobj(obj, field, dest, src);
  }
}

static void 
copy(struct SubWin *d)
{
  int sel, id;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if (sel >= 0 && sel <= d->num) {
    id = newobj(d->obj);
    if (id >= 0) {
      obj_copy(d->obj, id, sel);
      d->num++;
      NgraphApp.Changed = TRUE;
      d->select = id;
      d->update(FALSE);
    }
  }
}

static void 
delete(struct SubWin *d)
{
  int sel;
  int update;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if (sel >= 0 && sel <= d->num) {
    delobj(d->obj, sel);
    d->num--;
    update = FALSE;
    if (d->num < 0) {
      d->select = -1;
      update = TRUE;
    } else if (sel > d->num) {
      d->select = d->num;
    } else {
      d->select = sel;
    }
    d->update(update);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_top(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel <= d->num)) {
    movetopobj(d->obj, sel);
    d->select = 0;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_last(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel <= d->num)) {
    movelastobj(d->obj, sel);
    d->select = d->num;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_up(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 1) && (sel <= d->num)) {
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
move_down(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  if ((sel >= 0) && (sel < d->num)) {
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
update(struct SubWin *d)
{
  int sel, ret;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
 
  if ((sel >= 0) && (sel <= d->num)) {
    d->setup_dialog(d->dialog, d->obj, sel, -1);
    d->select = sel;
    if ((ret = DialogExecute(d->Win, d->dialog)) == IDDELETE) {
      delobj(d->obj, sel);
      d->select = -1;
    }
    if (ret != IDCANCEL)
      NgraphApp.Changed = TRUE;
    d->update(FALSE);
  }
}

static void
focus(struct SubWin *d)
{
  int sel;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if ((sel >= 0) && (sel <= d->num))
    Focus(d->obj, sel);
}

static void
hidden(struct SubWin *d)
{
  int sel;
  int hidden;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
    hidden = hidden ? FALSE : TRUE;
    putobj(d->obj, "hidden", sel, &hidden);
    d->select = sel;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  gtk_window_get_position(GTK_WINDOW(d->Win), x, y);
}

static void
do_popup(GdkEventButton *event, struct SubWin *d)
{
  int button, event_time;
  GtkMenuPositionFunc func = NULL;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
    func = popup_menu_position;
  }

  if (d->type == TypeFileWin ||
      d->type == TypeAxisWin ||
      d->type == TypeMergeWin) {
    d->select = list_store_get_selected_int(GTK_WIDGET(d->text), COL_ID);
  }

  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d,
		 button, event_time);
}

static gboolean
ev_button_down(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct SubWin *d;

  if (Menulock || GlobalLock) return TRUE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  d = (struct SubWin *) user_data;

  if (d->ev_button && d->ev_button(w, event, user_data))
    return TRUE;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      update(d);
      return TRUE;
    }
    break;
  case 2:
    break;
  case 3:
    if (d->popup) {
      do_popup(event, d);
      return TRUE;
    }
    break;
  }

  return FALSE;
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct SubWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct SubWin *) user_data;
  e = (GdkEventKey *)event;

  if (d->ev_key && d->ev_key(w, event, user_data))
    return TRUE;

  switch (e->keyval) {
  case GDK_Delete:
    delete(d);
    break;
  case GDK_Insert:
    copy(d);
    break;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK)
      move_top(d);
    else
      return FALSE;
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK)
      move_last(d);
    else
      return FALSE;
    break;
  case GDK_Up:
    if (e->state & GDK_SHIFT_MASK)
      move_up(d);
    else
      return FALSE;
    break;
  case GDK_Down:
    if (e->state & GDK_SHIFT_MASK)
      move_down(d);
    else
      return FALSE;
    break;
  case GDK_Return:
    update(d);
    break;
  case GDK_BackSpace:
    hidden(d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

static void
tree_copy(struct LegendWin *d)
{
  int n, m, sel, id;

  if (Menulock || GlobalLock) return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    id = newobj(d->obj[n]);
    if (id >= 0) {
      obj_copy(d->obj[n], id, m);
      d->select = id;
      d->legend_type = n;
      d->legend[n]++;
      d->update(FALSE);
      NgraphApp.Changed = TRUE;
    }
  }
}

static void 
tree_delete(struct LegendWin *d)
{
  int n, m;
  gboolean sel, update;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    delobj(d->obj[n], m);
    d->legend[n]--;
    update = FALSE;
    if (d->legend[n] < 0) {
      d->select = -1;
      d->legend_type = -1;
      update = TRUE;
    } else if (m > d->legend[n]) {
      d->legend_type = n;
      d->select = d->legend[n];
    } else {
      d->legend_type = n;
      d->select = m;
    }
    d->update(update);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_top(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    movetopobj(d->obj[n], m);
    d->select = 0;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_last(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    movelastobj(d->obj[n], m);
    d->select = d->legend[n];
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_up(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 1 && m <= d->legend[n]) {
    moveupobj(d->obj[n], m);
    d->select = m - 1;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_move_down(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m < d->legend[n]) {
    movedownobj(d->obj[n], m);
    d->select = m + 1;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static void
tree_update(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    d->setup_dialog(d->dialog, d->obj[n], n, m);
  }
}

static void
tree_focus(struct LegendWin *d)
{
  int n, m;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    Focus(d->obj[n], m);
  }
}

static void
tree_hidden(struct LegendWin *d)
{
  int n, m, hidden;
  gboolean sel;

  if (Menulock || GlobalLock)
    return;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);

  if (sel && n >=0 && m >= 0 && m <= d->legend[n]) {
    getobj(d->obj[n], "hidden", m, 0, NULL, &hidden);
    hidden = hidden ? FALSE : TRUE;
    putobj(d->obj[n], "hidden", m, &hidden);
    d->select = m;
    d->legend_type = n;
    d->update(FALSE);
    NgraphApp.Changed = TRUE;
  }
}

static gboolean
ev_button_down_tree(GtkWidget *w, GdkEventButton *event,  gpointer user_data)
{
  struct LegendWin *d;

  if (Menulock || GlobalLock) return TRUE;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  d = (struct LegendWin *) user_data;

  if (d->ev_button && d->ev_button(w, event, user_data))
    return TRUE;

  switch (event->button) {
  case 1:
    if (event->type == GDK_2BUTTON_PRESS) {
      tree_update(d);
      return TRUE;
    }
    break;
  case 2:
    break;
  case 3:
    if (d->popup) {
      do_popup(event, (struct SubWin *)d);
      return TRUE;
    }
    break;
  }

  return FALSE;
}

static gboolean
ev_key_down_tree(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct LegendWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct LegendWin *) user_data;
  e = (GdkEventKey *)event;

  if (d->ev_key && d->ev_key(w, event, user_data))
    return TRUE;

  switch (e->keyval) {
  case GDK_Delete:
    tree_delete(d);
    break;
  case GDK_Insert:
    tree_copy(d);
    break;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_top(d);
    else
      return FALSE;
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK)
      tree_move_last(d);
    else
      return FALSE;
    break;
  case GDK_Page_Up:
    tree_move_up(d);
    break;
  case GDK_Page_Down:
    tree_move_down(d);
    break;
  case GDK_Return:
    tree_update(d);
    break;
  case GDK_BackSpace:
    tree_hidden(d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

void
sub_window_minimize(struct SubWin *d)
{
  if (d->Win == NULL)
    return;

  d->window_state = gdk_window_get_state(d->Win->window);
  gtk_widget_hide_all(d->Win);
}

void
sub_window_restore_state(struct SubWin *d)
{
  if (d->Win == NULL)
    return;

  if (d->window_state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
    return;

  gtk_widget_show_all(d->Win);
}

static GtkWidget *
sub_window_create(struct SubWin *d, char *title, GtkWidget *text, char **xpm)
{
  GtkWidget *dlg, *swin;
  GdkPixbuf *icon;
  GtkWindowGroup *group;

  dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  d->Win = dlg;

  if (xpm) {
    icon = create_pixbuf_from_xpm(TopLevel, xpm);
    gtk_window_set_icon(GTK_WINDOW(dlg), icon);
  }
  if (title) {
    gtk_window_set_title(GTK_WINDOW(dlg), title);
  }

  group = gtk_window_get_group(GTK_WINDOW(TopLevel));
  gtk_window_group_add_window(group, GTK_WINDOW(dlg));
  //  gtk_widget_set_parent_window(GTK_WIDGET(dlg), TopLevel);
  //  gtk_window_set_destroy_with_parent(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_UTILITY);
  //  gtk_window_set_type_hint(GTK_WINDOW(dlg), GDK_WINDOW_TYPE_HINT_DIALOG);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(swin), text);
  gtk_container_add(GTK_CONTAINER(dlg), swin);

  d->swin = swin;

  g_signal_connect(dlg, "show", G_CALLBACK(cb_show), d);
  //  g_signal_connect(dlg, "hide", G_CALLBACK(cb_hide), d);
  g_signal_connect(dlg, "delete-event", G_CALLBACK(cb_del), d);
  g_signal_connect(dlg, "destroy", G_CALLBACK(cb_destroy), d);

  cb_show(dlg, d);
  return dlg;
}

GtkWidget *
text_sub_window_create(struct SubWin *d, char *title, char **xpm)
{
  GtkWidget *view;

  view = gtk_text_view_new_with_buffer(NULL);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  d->text = G_OBJECT(view);

  return sub_window_create(d, title, view, xpm);
}

GtkWidget *
list_sub_window_create(struct SubWin *d, char *title, int lisu_num, n_list_store *list, char **xpm)
{
  GtkWidget *lstor;

  lstor = list_store_create(lisu_num, list);
  d->text = G_OBJECT(lstor);

  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down), d);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down), d);

  return sub_window_create(d, title, lstor, xpm);
}

GtkWidget *
tree_sub_window_create(struct LegendWin *d, char *title, int lisu_num, n_list_store *list, char **xpm)
{
  GtkWidget *lstor;

  lstor = tree_store_create(lisu_num, list);
  d->text = G_OBJECT(lstor);

  g_signal_connect(lstor, "button-press-event", G_CALLBACK(ev_button_down_tree), d);
  g_signal_connect(lstor, "key-press-event", G_CALLBACK(ev_key_down_tree), d);

  return sub_window_create((struct SubWin *)d, title, lstor, xpm);
}

gboolean
list_sub_window_must_rebuild(struct SubWin *d)
{
  int n;

  d->num = chkobjlastinst(d->obj);
  n = list_store_get_num(GTK_WIDGET(d->text));

  return (n != d->num + 1);
}

void 
list_sub_window_build(struct SubWin *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i;

  d->num = chkobjlastinst(d->obj);
  list_store_clear(GTK_WIDGET(d->text));
  for (i = 0; i <= d->num; i++) {
    list_store_append(GTK_WIDGET(d->text), &iter);
    func(d, &iter, i);
  }
}

void 
list_sub_window_set(struct SubWin *d, list_sub_window_set_val_func func)
{
  GtkTreeIter iter;
  int i;
  gboolean state;

  state = list_store_get_iter_first(GTK_WIDGET(d->text), &iter);

  if (! state)
    return;

  for (i = 0; i <= d->num; i++) {
    func(d, &iter, i);
    if (! list_store_iter_next(GTK_WIDGET(d->text), &iter)) {
      break;
    }
  }
}


void 
list_sub_window_delete(GtkMenuItem *item, gpointer user_data)
{
  delete((struct SubWin *) user_data);
}

void 
list_sub_window_copy(GtkMenuItem *item, gpointer user_data)
{
  copy((struct SubWin *) user_data);
}

void 
list_sub_window_move_top(GtkMenuItem *item, gpointer user_data)
{
  move_top((struct SubWin *) user_data);
}

void 
list_sub_window_move_last(GtkMenuItem *item, gpointer user_data)
{
  move_last((struct SubWin *) user_data);
}

void 
list_sub_window_move_up(GtkMenuItem *item, gpointer user_data)
{
  move_up((struct SubWin *) user_data);
}

void 
list_sub_window_move_down(GtkMenuItem *item, gpointer user_data)
{
  move_down((struct SubWin *) user_data);
}

void 
list_sub_window_update(GtkMenuItem *item, gpointer user_data)
{
  update((struct SubWin *) user_data);
}

void 
list_sub_window_hide(GtkMenuItem *item, gpointer user_data)
{
  hidden((struct SubWin *) user_data);
}

void 
list_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  focus((struct SubWin *) user_data);
}

void 
tree_sub_window_delete(GtkMenuItem *item, gpointer user_data)
{
  tree_delete((struct LegendWin *) user_data);
}

void 
tree_sub_window_copy(GtkMenuItem *item, gpointer user_data)
{
  tree_copy((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_top(GtkMenuItem *item, gpointer user_data)
{
  tree_move_top((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_last(GtkMenuItem *item, gpointer user_data)
{
  tree_move_last((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_up(GtkMenuItem *item, gpointer user_data)
{
  tree_move_up((struct LegendWin *) user_data);
}

void 
tree_sub_window_move_down(GtkMenuItem *item, gpointer user_data)
{
  tree_move_down((struct LegendWin *) user_data);
}

void 
tree_sub_window_update(GtkMenuItem *item, gpointer user_data)
{
  tree_update((struct LegendWin *) user_data);
}

void 
tree_sub_window_hide(GtkMenuItem *item, gpointer user_data)
{
  tree_hidden((struct LegendWin *) user_data);
}

void 
tree_sub_window_focus(GtkMenuItem *item, gpointer user_data)
{
  tree_focus((struct LegendWin *) user_data);
}

static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  do_popup(NULL, d);
  return TRUE;
}

GtkWidget *
sub_win_create_popup_menu(struct SubWin *d, int n, struct subwin_popup_list *list, GCallback cb)
{
  GtkWidget *menu, *item;
  int i = 0;

  if (d->popup_item)
    free(d->popup_item);

  d->popup_item = malloc(sizeof(GtkWidget *) * n);

  menu = gtk_menu_new();

  for (i = 0; i < n; i++) {
    if (list[i].use_stock) {
      item = gtk_image_menu_item_new_from_stock(list[i].title, list[i].accel_group);
    } else {
      item = gtk_menu_item_new_with_mnemonic(_(list[i].title));
    }
    d->popup_item[i] = item;
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", list[i].func, d);
  }

  d->popup = menu;
  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget(GTK_MENU(menu), GTK_WIDGET(d->text), NULL);
  g_signal_connect(d->text, "popup-menu", G_CALLBACK(ev_popup_menu), d);

  if (cb)
    g_signal_connect(menu, "show", cb, d);

  return menu;
}
