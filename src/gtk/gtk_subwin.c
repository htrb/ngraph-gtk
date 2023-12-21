/*
 * $Id: gtk_subwin.c,v 1.68 2010-04-01 06:08:23 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libgen.h>

#include "dir_defs.h"

#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathfn.h"

#include "init.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11view.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "gtk_columnview.h"
#include "gtk_listview.h"
#include "gtk_widget.h"
#include "gtk_combo.h"

#include "gtk_subwin.h"

#define DOUBLE_CLICK_PERIOD 250

static void hidden(struct obj_list_data *d);

void
update_viewer(struct obj_list_data *d)
{
  char const *objects[2];
  objects[0] = d->obj->name;
  objects[1] = NULL;
  ViewerWinUpdate(objects);
}

static void
obj_copy(struct objlist *obj, int dest, int src)
{
  char *field[] = {"name", NULL};

  copy_obj_field(obj, dest, src, field);
}

static void
copy(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);

  if (sel >= 0 && sel <= num) {
    int id, undo;
    undo = menu_save_undo_single(UNDO_TYPE_COPY, d->obj->name);
    id = newobj(d->obj);
    if (id < 0) {
      menu_delete_undo(undo);
      return;
    }
    obj_copy(d->obj, id, sel);
    set_graph_modified();
    d->select = id;
    d->update(d, FALSE, TRUE);
  }
}

static void
delete(struct obj_list_data *d)
{
  int sel, inst_num;
  int updated;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);
  inst_num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > inst_num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_DELETE, d->obj->name);
  if (d->delete) {
    d->delete(d, sel);
  } else {
    delobj(d->obj, sel);
  }

  inst_num = chkobjlastinst(d->obj);
  if (inst_num < 0) {
    d->select = -1;
    updated = TRUE;
  } else if (sel > inst_num) {
    d->select = inst_num;
    updated = FALSE;
  } else {
    d->select = sel;
    updated = FALSE;
  }
  d->update(d, updated, TRUE);
  set_graph_modified();
}

static void
move_top(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movetopobj(d->obj, sel);
  d->select = 0;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
move_last(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movelastobj(d->obj, sel);
  d->select = num;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
move_up(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if ((sel >= 1) && (sel <= num)) {
    menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    d->update(d, FALSE, TRUE);
    set_graph_modified();
  }
}

static void
move_down(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if ((sel >= 0) && (sel < num)) {
    menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    d->update(d, FALSE, TRUE);
    set_graph_modified();
  }
}

struct swin_update_data {
  int undo;
  struct obj_list_data *d;
};

static void
swin_update_response(struct response_callback *cb)
{
  struct swin_update_data *data;
  struct obj_list_data *d;
  int undo, sel;

  data = (struct swin_update_data *) cb->data;
  d = data->d;
  undo = data->undo;
  sel = d->select;
  g_free(data);

  switch (cb->return_value) {
  case IDCANCEL:
    menu_undo_internal(undo);
    break;
  case IDDELETE:
    if (d->delete) {
      d->delete(d, sel);
    } else {
      delobj(d->obj, sel);
    }
    d->select = -1;
    d->update(d, FALSE, DRAW_REDRAW);
    break;
  default:
    d->update(d, FALSE, DRAW_NOTIFY);
  }
}

static void
swin_update(struct obj_list_data *d)
{
  int sel, num, undo;
  GtkWidget *parent;
  struct swin_update_data *data;

  if (Menulock || Globallock)
    return;

  UnFocus();

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  parent = TopLevel;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }

  d->setup_dialog(d, sel, -1);
  d->select = sel;
  if (d->undo_save) {
    undo = d->undo_save(UNDO_TYPE_EDIT);
  } else {
    undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  }
  data->d = d;
  data->undo = undo;
  response_callback_add(d->dialog, swin_update_response, NULL, data);
  DialogExecute(parent, d->dialog);
}

static void
focus(struct obj_list_data *d, enum FOCUS_MODE add)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num))
    Focus(d->obj, sel, add);
}

static void
focus_all(struct obj_list_data *d)
{
  if (Menulock || Globallock)
    return;

  ViewerSelectAllObj(d->obj);
}

static void
hidden(struct obj_list_data *d)
{
  int sel, num;
  int hidden;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }

  UnFocus();

  menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  getobj(d->obj, "hidden", sel, 0, NULL, &hidden);
  hidden = hidden ? FALSE : TRUE;
  putobj(d->obj, "hidden", sel, &hidden);
  d->select = sel;
  d->update(d, FALSE, TRUE);
  set_graph_modified();
}

static void
do_popup(gdouble x, gdouble y, struct obj_list_data *d)
{
  GdkRectangle rect;
  if (d->parent->type == TypeFileWin ||
      d->parent->type == TypeAxisWin ||
      d->parent->type == TypeMergeWin ||
      d->parent->type == TypePathWin ||
      d->parent->type == TypeRectWin ||
      d->parent->type == TypeArcWin ||
      d->parent->type == TypeMarkWin ||
      d->parent->type == TypeTextWin) {
    d->select = columnview_get_active (d->text);
  }
  rect.x = x;
  rect.y = y;
  rect.width = 1;
  rect.height = 1;
  gtk_popover_set_pointing_to(GTK_POPOVER(d->popup), &rect);
  gtk_popover_popup(GTK_POPOVER(d->popup));
}

static void
ev_button_down(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct obj_list_data *d;
  guint button;
  GtkEventSequenceState state = GTK_EVENT_SEQUENCE_NONE;

  if (Menulock || Globallock) return;

  d = user_data;

  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
  switch (button) {
  case 1:
    if (n_press > 1) {
      swin_update(d);
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    break;
  case 3:
    /*
    if (tree_view_select_pos(gesture, x, y)) {
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    */
    if (d->popup) {
      do_popup(x, y, d);
      state = GTK_EVENT_SEQUENCE_CLAIMED;
    }
    break;
  }
  gtk_gesture_set_state(GTK_GESTURE(gesture), state);
}

static gboolean
ev_key_down(GtkEventController *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct obj_list_data *d;
  GtkWidget *w;
  /*
  static guint32 ev_time = 0;
  guint32 t;

  t = gtk_event_controller_get_current_event_time(controller);
  if (t <= ev_time) {
    return TRUE;
  } else {
    ev_time = t;
  }
  */

  if (Menulock || Globallock)
    return TRUE;

  d = user_data;

  w = gtk_event_controller_get_widget(controller);
  if (d->ev_key && d->ev_key(w, keyval, state, user_data))
    return TRUE;

  switch (keyval) {
  case GDK_KEY_Delete:
    delete(d);
    break;
  case GDK_KEY_Insert:
    copy(d);
    break;
  case GDK_KEY_Home:
    if (state & GDK_SHIFT_MASK)
      move_top(d);
    else
      return FALSE;
    break;
  case GDK_KEY_End:
    if (state & GDK_SHIFT_MASK)
      move_last(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Up:
    if (state & GDK_SHIFT_MASK)
      move_up(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Down:
    if (state & GDK_SHIFT_MASK)
      move_down(d);
    else
      return FALSE;
    break;
  case GDK_KEY_Return:
    swin_update(d);
    break;
  case GDK_KEY_BackSpace:
    hidden(d);
    break;
  case GDK_KEY_space:
    if (state & GDK_CONTROL_MASK)
      return FALSE;

    if (! d->can_focus)
      return FALSE;

    if (state & GDK_SHIFT_MASK) {
      focus(d, FOCUS_MODE_TOGGLE);
    } else {
      list_sub_window_focus(NULL, NULL, d);
    }
    break;
  case GDK_KEY_Menu:
    do_popup(0, 0, d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

static void
swin_realized(struct obj_list_data *ptr)
{
  ptr->update(ptr, TRUE, TRUE);
}

GtkWidget *
text_sub_window_create(struct SubWin *d)
{
  GtkWidget *view, *swin;
  GtkTextBuffer *buf;

  buf = gtk_text_buffer_new(NULL);
  view = gtk_text_view_new_with_buffer(buf);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);

  d->data.text = view;

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
  d->Win = swin;

  return swin;
}

GtkWidget *
label_sub_window_create(struct SubWin *d)
{
  GtkWidget *label, *swin;

  label = gtk_label_new(NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_START);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_label_set_wrap(GTK_LABEL(label), FALSE);
  gtk_label_set_single_line_mode(GTK_LABEL(label), FALSE);

  d->data.text = label;

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), label);
  d->Win = swin;

  return swin;
}

GtkWidget *
parameter_sub_window_create(struct SubWin *d)
{
  GtkWidget *grid, *swin;
  struct obj_list_data *data;

  grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 4);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  data->select = -1;
  data->parent = d;
  data->undo_save = NULL;
  data->can_focus = FALSE;
  data->list = NULL;
  data->list_col_num = 0;
  data->text = grid;

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), grid);
  d->data.data = data;
  d->Win = swin;

  return swin;
}

static void
list_focused(GtkEventControllerFocus *ev, gpointer user_data)
{
  set_focus_insensitive(&NgraphApp.Viewer);
}

static void
add_event_controller(GtkWidget *widget, struct obj_list_data *data)
{
  GtkGesture *gesture;
  GtkEventController *ev;

  gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));

  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
  g_signal_connect(gesture, "pressed", G_CALLBACK(ev_button_down), data);

  ev = add_event_key(widget, G_CALLBACK(ev_key_down), NULL, data);
  gtk_event_controller_set_propagation_phase (ev, GTK_PHASE_CAPTURE);
}

#define INSTANCE_ID_KEY "n_inst_id"

static void
update_obj (struct obj_list_data *d, const char *field, int id, void *val)
{
  menu_save_undo_single (UNDO_TYPE_EDIT, d->obj->name);
  putobj (d->obj, field, id, val);
  d->select = id;
  d->update (d, FALSE, TRUE);
  set_graph_modified ();
}

static void
item_toggled (GObject *self, n_list_store *item)
{
  struct obj_list_data *d;
  struct objlist *obj;
  int state, id;
  const char *field;

  if (item->block_signal) {
    return;
  }

  d = item->data;
  obj = d->obj;
  field = item->name;
  id = GPOINTER_TO_INT (g_object_get_data (self, INSTANCE_ID_KEY));
  if (id < 0) {
    return;
  }
  if (getobj (obj, field, id, 0, NULL, &state) < 0) {
    return;
  }
  state = ! state;
  update_obj (d, field, id, &state);
}

static void
select_enum_item_cb (GtkWidget* self, int position, gpointer user_data)
{
  GtkWidget *popover, *label;
  int id;
  int cur;
  struct obj_list_data *d;
  n_list_store *item;

  item = (n_list_store *) user_data;
  d = item->data;
  popover = gtk_widget_get_ancestor (self, GTK_TYPE_POPOVER);
  label = gtk_widget_get_parent (popover);
  id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), INSTANCE_ID_KEY));
  getobj (d->obj, item->name, id, 0, NULL, &cur);
  gtk_popover_popdown(GTK_POPOVER(popover));
  if (cur == position) {
    return;
  }
  update_obj (d, item->name, id, &position);
}

static void
create_enum_menu(GtkWidget *parent, const char **enumlist, n_list_store *item)
{
  GtkWidget *popover, *menu;
  GtkStringList *list;
  int i, cur, id;

  menu = listview_create(N_SELECTION_TYPE_SINGLE, NULL, NULL, NULL);
  gtk_list_view_set_single_click_activate (GTK_LIST_VIEW (menu), TRUE);

  popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER (popover), menu);

  list = listview_get_string_list (menu);
  for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
    gtk_string_list_append (list, _(enumlist[i]));
  }
  id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (parent), INSTANCE_ID_KEY));
  getobj (item->data->obj, item->name, id, 0, NULL, &cur);
  gtk_list_view_scroll_to (GTK_LIST_VIEW (menu), cur, GTK_LIST_SCROLL_SELECT | GTK_LIST_SCROLL_FOCUS, NULL);
  gtk_widget_set_parent(popover, parent);
  gtk_popover_popup (GTK_POPOVER (popover));
  g_signal_connect (menu, "activate", G_CALLBACK (select_enum_item_cb), item);
  g_signal_connect_swapped (popover, "closed", G_CALLBACK (gtk_widget_unparent), popover);
}

static void
enum_cb (GtkEventController *self, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  GtkWidget *parent;
  n_list_store *item;
  struct obj_list_data *d;
  const char **enumlist;

  item = (n_list_store *) user_data;
  d = item->data;
  enumlist = (const char **) chkobjarglist(d->obj, item->name);
  parent = gtk_event_controller_get_widget (self);
  create_enum_menu(parent, enumlist, item);
}

static int
get_integer_from_entry (GtkWidget *entry, const n_list_store *item, int cur)
{
  int val;
  if (item->inc) {
    double dval;
    dval = gtk_spin_button_get_value (GTK_SPIN_BUTTON (entry));
    if (item->type == G_TYPE_DOUBLE) {
      val = dval * 100;
    } else {
      val = dval;
    }
  } else {
    const char *text;
    char *ptr;
    text = gtk_editable_get_text (GTK_EDITABLE (entry));
    val = strtol (text, &ptr, 10);
    if (text[0] == '\0' || (val == 0 && ptr[0] != '\0')) {
      val = cur;
    }
  }
  return val;
}

static void
set_numeric_item_cb (GtkWidget *self, gpointer user_data)
{
  GtkWidget *popover, *label, *hbox, *vbox, *entry;
  int id;
  int cur, val;
  struct obj_list_data *d;
  n_list_store *item;

  item = (n_list_store *) user_data;
  d = item->data;
  hbox = gtk_widget_get_parent (self);
  vbox = gtk_widget_get_parent (hbox);
  entry = gtk_widget_get_first_child (vbox);
  popover = gtk_widget_get_ancestor (vbox, GTK_TYPE_POPOVER);
  label = gtk_widget_get_parent (popover);
  id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), INSTANCE_ID_KEY));
  getobj (d->obj, item->name, id, 0, NULL, &cur);
  gtk_popover_popdown(GTK_POPOVER(popover));
  val = get_integer_from_entry (entry, item, cur);
  if (val == cur) {
    return;
  }
  update_obj (d, item->name, id, &val);
}

static void
create_numeric_input (GtkWidget *parent, n_list_store *item)
{
  GtkWidget *popover, *entry, *button, *vbox, *hbox;
  struct obj_list_data *d;
  int val, id;

  d = item->data;
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (parent), INSTANCE_ID_KEY));
  getobj (d->obj, item->name, id, 0, NULL, &val);
  if (item->inc) {
    if (item-> type == G_TYPE_DOUBLE) {
      entry = gtk_spin_button_new_with_range (item->min / 100.0, item->max / 100.0, item->inc / 100.0);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), val / 100.0);
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (entry), 2);
    } else {
      entry = gtk_spin_button_new_with_range (item->min, item->max, item->inc);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), val);
    }
    gtk_editable_set_alignment(GTK_EDITABLE(entry), 1.0);
    gtk_editable_set_width_chars(GTK_EDITABLE(entry), NUM_ENTRY_WIDTH);
  } else {
    const char *text;
    text = gtk_label_get_text (GTK_LABEL (parent));
    entry = create_text_entry(TRUE, FALSE);
    gtk_editable_set_text (GTK_EDITABLE (entry), text);
  }
  gtk_box_append(GTK_BOX (vbox), entry);
  button = gtk_button_new_with_mnemonic (_("_Apply"));
  gtk_widget_set_hexpand (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (set_numeric_item_cb), item);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand (hbox, TRUE);
  gtk_widget_set_halign (hbox, GTK_ALIGN_END);
  gtk_box_append (GTK_BOX (hbox), button);
  gtk_box_append(GTK_BOX (vbox), hbox);

  popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER (popover), vbox);

  gtk_widget_set_parent(popover, parent);
  gtk_popover_popup (GTK_POPOVER (popover));
  g_signal_connect_swapped (popover, "closed", G_CALLBACK (gtk_widget_unparent), popover);
}

static void
numeric_cb (GtkEventController *self, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  GtkWidget *parent;
  n_list_store *item;

  item = (n_list_store *) user_data;
  parent = gtk_event_controller_get_widget (self);
  create_numeric_input(parent, item);
}

static void
setup_editing_item (GtkWidget *w, n_list_store *item, GCallback func)
{
  GtkEventController *ev;
  ev = GTK_EVENT_CONTROLLER (gtk_gesture_long_press_new ());
  gtk_widget_add_controller (w, ev);
  g_signal_connect (ev, "pressed", G_CALLBACK (func), item);
}

static void
setup_column (GtkListItemFactory *factory, GtkListItem *list_item, n_list_store *item)
{
  GtkWidget *w;

  item->block_signal = TRUE;
  switch (item->type) {
  case G_TYPE_BOOLEAN:
    w = gtk_check_button_new ();
    g_signal_connect (w, "toggled", G_CALLBACK (item_toggled), item);
    break;
  case G_TYPE_OBJECT:
    w = gtk_picture_new ();
    gtk_picture_set_content_fit (GTK_PICTURE (w), GTK_CONTENT_FIT_CONTAIN);
    if (strcmp (item->name, "type") == 0 && item->editable) {
      setup_editing_item (w, item, G_CALLBACK (enum_cb));
    }
    break;
  case G_TYPE_INT:
  case G_TYPE_DOUBLE:
    w = gtk_label_new (NULL);
    gtk_widget_set_halign (w, GTK_ALIGN_END);
    if (item->editable) {
      setup_editing_item (w, item, G_CALLBACK (numeric_cb));
    }
    break;
  case G_TYPE_ENUM:
    w = gtk_label_new (NULL);
    gtk_widget_set_halign (w, GTK_ALIGN_START);
    if (item->editable) {
      setup_editing_item (w, item, G_CALLBACK (enum_cb));
    }
    break;
  default:
    w = gtk_label_new (NULL);
    gtk_widget_set_halign (w, GTK_ALIGN_START);
    gtk_label_set_ellipsize (GTK_LABEL (w), item->ellipsize);
  }
  gtk_list_item_set_child (list_item, w);
}

static void
bind_column_main (GtkWidget *w, struct objlist *obj, const char *field, int id, n_list_store *item)
{
  enum ngraph_object_field_type type;
  int ival;
  double dval;
  char *str, **enumlist, buf[256];

  getobj (obj, "hidden", id, 0, NULL, &ival);
  if (strcmp (field, "hidden")) {
    gtk_widget_set_sensitive (w, ! ival);
  }
  if (item->bind_func) {
    item->bind_func(w, obj, field, id);
    return;
  }
  str = NULL;
  type = chkobjfieldtype(obj, field);
  switch (type){
  case NBOOL:
    getobj (obj, field, id, 0, NULL, &ival);
    if (strcmp (field, "hidden") == 0) {
      ival = ! ival;
    }
    gtk_check_button_set_active(GTK_CHECK_BUTTON (w), ival);
    return;
  case NENUM:
    getobj (obj, field, id, 0, NULL, &ival);
    enumlist = (char **) chkobjarglist(obj, field);
    str = _(enumlist[ival]);
    break;
  case NINT:
    getobj (obj, field, id, 0, NULL, &ival);
    if (item->type == G_TYPE_INT) {
      snprintf (buf, sizeof (buf), "%d", ival);
    } else {
      snprintf (buf, sizeof (buf), "%.2f", ival / 100.0);
    }
    str = buf;
    break;
  case NDOUBLE:
    getobj (obj, field, id, 0, NULL, &dval);
    snprintf (buf, sizeof (buf), "%.15g", dval);
    str = buf;
    break;
  case NSTR:
    getobj (obj, field, id, 0, NULL, &str);
    break;
  default:
    return;
  }
  gtk_label_set_text(GTK_LABEL (w), str);
}

static void
bind_column (GtkListItemFactory *factory, GtkListItem *list_item, n_list_store *item)
{
  GtkWidget *w;
  NInst *inst;
  w = gtk_list_item_get_child (list_item);
  inst = gtk_list_item_get_item (list_item);
  g_object_set_data (G_OBJECT (w), INSTANCE_ID_KEY, GINT_TO_POINTER (inst->id));
  item->block_signal = TRUE;
  bind_column_main (w, inst->obj, item->name, inst->id, item);
  item->block_signal = FALSE;
}

static struct obj_list_data *
list_widget_create(struct SubWin *d, int lisu_num, n_list_store *list, int can_focus, GtkWidget **w)
{
  struct obj_list_data *data;
  GtkWidget *lstor, *swin;
  GtkEventController *ev;
  int i;

  data = g_malloc0(sizeof(*data));
  data->select = -1;
  data->parent = d;
  data->undo_save = NULL;
  data->can_focus = can_focus;
  data->list = list;
  data->list_col_num = lisu_num;
  lstor = columnview_create(N_TYPE_INST, N_SELECTION_TYPE_SINGLE);
  data->text = lstor;

  for (i = 0; i < lisu_num; i++) {
    columnview_create_column (lstor, _(list[i].title), G_CALLBACK (setup_column), G_CALLBACK (bind_column), NULL, list + i, list[i].expand);
    list[i].data = data;
  }

  add_event_controller(lstor, data);
  g_signal_connect_swapped(lstor, "activate", G_CALLBACK (swin_update), data);

  /* to handle key-press-event correctly in single window mode */
  ev = gtk_event_controller_focus_new();
  g_signal_connect(ev, "enter", G_CALLBACK(list_focused), NULL);
  gtk_widget_add_controller(lstor, ev);

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), lstor);

  g_signal_connect_swapped(swin, "realize", G_CALLBACK(swin_realized), data);

  *w = swin;

  return data;
}

GtkWidget *
list_sub_window_create(struct SubWin *d, int lisu_num, n_list_store *list)
{
  GtkWidget *swin;
  struct obj_list_data *data;

  data = list_widget_create(d, lisu_num, list, d->type != TypeFileWin, &swin);
  d->data.data = data;
  d->Win = swin;

  return swin;
}

gboolean
list_sub_window_must_rebuild(struct obj_list_data *d)
{
  int n, num;

  num = chkobjlastinst(d->obj);
  n = columnview_get_n_items(d->text);

  return (n != num + 1);
}

void
list_sub_window_build(struct obj_list_data *d)
{
  int i, num;

  num = chkobjlastinst(d->obj);
  columnview_clear(d->text);
  for (i = 0; i <= num; i++) {
    columnview_append_n_inst(d->text, chkobjectname (d->obj), i, d->obj);
  }
}

void
list_sub_window_set(struct obj_list_data *d)
{
  int active;
  active = columnview_get_active (d->text);
  list_sub_window_build(d);
  if (active >= 0) {
    columnview_set_active (d->text, active, TRUE);
  }
}


void
list_sub_window_delete(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  delete((struct obj_list_data *) user_data);
}

void
list_sub_window_copy(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  copy((struct obj_list_data *) user_data);
}

void
list_sub_window_move_top(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  move_top((struct obj_list_data *) user_data);
}

void
list_sub_window_move_last(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  move_last((struct obj_list_data *) user_data);
}

void
list_sub_window_move_up(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  move_up((struct obj_list_data *) user_data);
}

void
list_sub_window_move_down(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  move_down((struct obj_list_data *) user_data);
}

void
list_sub_window_update(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  swin_update((struct obj_list_data *) user_data);
}

void
list_sub_window_focus(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  focus((struct obj_list_data *) user_data, FOCUS_MODE_NORMAL);
}

void
list_sub_window_focus_all(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  focus_all((struct obj_list_data *) user_data);
}

struct set_object_name_data {
  struct objlist *obj;
  int id;
  char *name;
};

static void
set_object_name_response(int res, const char *str, gpointer user_data)
{
  struct set_object_name_data *data;
  struct objlist *obj;
  int id, undo;
  char *name, *new_name;

  data = (struct set_object_name_data *) user_data;
  obj = data->obj;
  id = data->id;
  name = data->name;
  g_free(data);

  if (res != IDOK) {
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
  if (str == NULL) {
    putobj(obj, "name", id, NULL);
    if (name) {
      set_graph_modified();
    } else {
      menu_delete_undo(undo);
    }
    return;
  }

  new_name = g_strdup(str);
  if (new_name == NULL) {
    menu_delete_undo(undo);
    return;
  }

  g_strstrip(new_name);
  if (new_name[0] == '\0') {
    g_free(new_name);
    new_name = NULL;
  }

  if (g_strcmp0(name, new_name) == 0) {
    g_free(new_name);
    menu_delete_undo(undo);
    return;
  }

  if (putobj(obj, "name", id, new_name) < 0) {
    g_free(new_name);
    menu_delete_undo(undo);
    return;
  }
  set_graph_modified();
  return;
}

static void
set_object_name(struct objlist *obj, int id)
{
  char *name, buf[256];
  struct set_object_name_data *data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  getobj(obj, "name", id, 0, NULL, &name);
  data->obj = obj;
  data->id = id;
  data->name = name;
  snprintf(buf, sizeof(buf), "%s:%d:name", chkobjectname(obj), id);
  input_dialog(TopLevel, _("Instance name"), buf, name, _("_Apply"), NULL, NULL, set_object_name_response, data);
}

void
list_sub_window_object_name(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data*) client_data;
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);
  if (sel < 0 || sel > num) {
    return;
  }
  set_object_name(d->obj, sel);
}

void
sub_win_create_popup_menu(struct obj_list_data *d, int n, GActionEntry *list, GCallback cb)
{
  GtkApplication *app;
  GtkWidget *popup;
  GMenu *menu;
  char menu_id[256];
  const char *name;

  app = n_get_gtk_application();
  if (app == NULL) {
    return;
  }
  g_action_map_add_action_entries(G_ACTION_MAP(app), list, n, d);

  name = chkobjectname(d->obj);
  snprintf(menu_id, sizeof(menu_id), "%s-popup-menu", name);
  menu = gtk_application_get_menu_by_id(GtkApp, menu_id);

#if USE_NESTED_SUBMENUS
  popup = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(menu), POPOVERMEU_FLAG);
#else  /* USE_NESTED_SUBMENUS */
  popup = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
#endif	/* USE_NESTED_SUBMENUS */
  gtk_popover_set_has_arrow(GTK_POPOVER(popup), FALSE);
  widget_set_parent(popup, d->text);
  if (cb) {
    g_signal_connect(popup, "show", cb, d);
  }
  d->popup = popup;
}
