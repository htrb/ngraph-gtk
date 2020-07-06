/*
 * $Id: x11parameter.c,v 1.33 2010-03-04 08:30:17 hito Exp $
 *
 * This file is part of "Ngraph for X11".
 *
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "object.h"
#include "shell.h"
#include "oparameter.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11commn.h"
#include "x11view.h"
#include "x11parameter.h"

static void set_parameter(double prm, gpointer user_data);
static void check_min_max(double *min, double *max, double *inc);

static void
ParameterDialogSetupItem(struct ParameterDialog *d, int id)
{
  char *text;
  GtkTextBuffer *buf;

  SetWidgetFromObjField(d->title, d->Obj, id, "title");
  SetWidgetFromObjField(d->type, d->Obj, id, "type");
  SetWidgetFromObjField(d->min, d->Obj, id, "min");
  SetWidgetFromObjField(d->max, d->Obj, id, "max");
  SetWidgetFromObjField(d->step, d->Obj, id, "step");
  SetWidgetFromObjField(d->redraw, d->Obj, id, "redraw");
  SetWidgetFromObjField(d->checked, d->Obj, id, "active");
  SetWidgetFromObjField(d->selected, d->Obj, id, "selected");
  SetWidgetFromObjField(d->value, d->Obj, id, "value");

  getobj(d->Obj, "items", id, 0, NULL, &text);
  if (text == NULL) {
    text = "";
  }
  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(d->items));
  gtk_text_buffer_set_text(buf, text, -1);
}

static char *
ParameterCB(struct objlist *obj, int id)
{
  char *s, *title;

  getobj(obj, "title", id, 0, NULL, &title);
  s = g_strdup(title);
  return s;
}

static void
ParameterDialogCopy(GtkWidget *w, gpointer data)
{
  struct ParameterDialog *d;
  int sel;

  d = (struct ParameterDialog *) data;

  sel = CopyClick(d->widget, d->Obj, d->Id, ParameterCB);
  if (sel != -1) {
    ParameterDialogSetupItem(d, sel);
  }
}

#define TYPE_SPIN_NAME  "spin"
#define TYPE_CHECK_NAME "check"
#define TYPE_COMBO_NAME "combo"
#define TYPE_TRANSITION_NAME "transition"

static void
add_page_spin(struct ParameterDialog *d)
{
  GtkWidget *w, *table;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Min:"), TRUE, i++);
  d->min = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Max:"), TRUE, i++);
  d->max = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Step:"), TRUE, i++);
  d->step = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Value:"), TRUE, i++);
  d->value = w;

  w = gtk_check_button_new_with_mnemonic(_("_Wrap"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->wrap = w;

  gtk_stack_add_named(GTK_STACK(d->stack), table, TYPE_SPIN_NAME);
}

static void
add_page_transition(struct ParameterDialog *d)
{
  GtkWidget *w, *table;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Start:"), TRUE, i++);
  d->start = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Stop:"), TRUE, i++);
  d->stop = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Step:"), TRUE, i++);
  d->transition_step = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(w), OPARAMETER_WAIT_MIN / 100.0, OPARAMETER_WAIT_MAX / 100.0);
  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(w), 0.1, 1);
  add_widget_to_table(table, w, _("_Wait:"), TRUE, i++);
  d->wait = w;

  w = gtk_check_button_new_with_mnemonic(_("_Loop"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->loop = w;

  gtk_stack_add_named(GTK_STACK(d->stack), table, TYPE_TRANSITION_NAME);
}

static void
add_page_check(struct ParameterDialog *d)
{
  GtkWidget *w;
  w = gtk_check_button_new_with_mnemonic(_("_Active"));
  gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_START);
  d->checked = w;
  gtk_stack_add_named(GTK_STACK(d->stack), w, TYPE_CHECK_NAME);
}

static void
add_page_combo(struct ParameterDialog *d)
{
  GtkWidget *w, *table, *swin;
  GtkTextBuffer *buf;
  int i;

  table = gtk_grid_new();

  i = 0;

  buf = gtk_text_buffer_new(NULL);
  w = gtk_text_view_new_with_buffer(buf);
  gtk_widget_set_vexpand(w, TRUE);
  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), w);
  add_widget_to_table(table, swin, _("_Items:"), TRUE, i++);
  d->items = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Selected:"), FALSE, i++);
  d->selected = w;

  gtk_stack_add_named(GTK_STACK(d->stack), table, TYPE_COMBO_NAME);
}

static void
parameter_type_changed(GtkComboBox *combo, gpointer user_data)
{
  struct ParameterDialog *d;
  int type;

  d = user_data;
  type = gtk_combo_box_get_active(combo);
  if (type < 0) {
    return;
  }
  switch (type) {
  case PARAMETER_TYPE_SPIN:
    gtk_widget_show(d->wrap);
    gtk_stack_set_visible_child_name(GTK_STACK(d->stack), TYPE_SPIN_NAME);
    break;
  case PARAMETER_TYPE_SCALE:
    gtk_widget_hide(d->wrap);
    gtk_stack_set_visible_child_name(GTK_STACK(d->stack), TYPE_SPIN_NAME);
    break;
  case PARAMETER_TYPE_SWITCH:
  case PARAMETER_TYPE_CHECK:
    gtk_stack_set_visible_child_name(GTK_STACK(d->stack), TYPE_CHECK_NAME);
    break;
  case PARAMETER_TYPE_COMBO:
    gtk_stack_set_visible_child_name(GTK_STACK(d->stack), TYPE_COMBO_NAME);
    break;
  }
 }

static void
ParameterDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct ParameterDialog *d;
  char title[64];

  d = (struct ParameterDialog *) data;

  snprintf(title, sizeof(title), _("Parameter %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *frame, *table;
    int i;
    table = gtk_grid_new();

    i = 0;
    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table(table, w, _("_Title:"), TRUE, i++);
    d->title = w;

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Type:"), TRUE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(parameter_type_changed), d);
    d->type = w;

    w = gtk_stack_new();
    add_widget_to_table(table, w, NULL, TRUE, i++);
    d->stack = w;

    add_page_spin(d);
    add_page_check(d);
    add_page_combo(d);

    w = gtk_check_button_new_with_mnemonic(_("_Redraw"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->redraw = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 4);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(ParameterDialogCopy), d, "parameter");

    gtk_widget_show_all(GTK_WIDGET(d->vbox));
  }
  ParameterDialogSetupItem(d, d->Id);
}

static void
ParameterDialogClose(GtkWidget *w, void *data)
{
  struct ParameterDialog *d;
  GtkTextBuffer *buf;
  int ret;
  char *text;

  d = (struct ParameterDialog *) data;

  switch(d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromWidget(d->title, d->Obj, d->Id, "title"))
    return;
  if (SetObjFieldFromWidget(d->type, d->Obj, d->Id, "type"))
    return;
  if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "min"))
    return;
  if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "max"))
    return;
  if (SetObjFieldFromWidget(d->step, d->Obj, d->Id, "step"))
    return;
  if (SetObjFieldFromWidget(d->wrap, d->Obj, d->Id, "wrap"))
    return;
  if (SetObjFieldFromWidget(d->checked, d->Obj, d->Id, "active"))
    return;
  if (SetObjFieldFromWidget(d->selected, d->Obj, d->Id, "selected"))
    return;
  if (SetObjFieldFromWidget(d->value, d->Obj, d->Id, "value"))
    return;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(d->items));
  text = get_text_from_buffer(buf);
  if (chk_sputobjfield(d->Obj, d->Id, "items", text)) {
    g_free(text);
    return;
  }
  g_free(text);

  d->ret = ret;
}

void
ParameterDialog(struct obj_list_data *data, int id, int user_data)
{
  struct ParameterDialog *d;

  d = (struct ParameterDialog *) data->dialog;

  d->SetupWindow = ParameterDialogSetup;
  d->CloseWindow = ParameterDialogClose;
  d->Obj = data->obj;
  d->Id = id;
}

void
CmParameterAdd(void *w, gpointer client_data)
{
  int id, undo, ret;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;

  undo = menu_save_undo_single(UNDO_TYPE_CREATE, d->obj->name);
  id = newobj(d->obj);
  if (id < 0) {
    return;
  }
  ParameterDialog(NgraphApp.ParameterWin.data.data, id, -1);
  ret = DialogExecute(TopLevel, &DlgParameter);
  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
  }
}

void
CmParameterDelete(void *w, gpointer client_data)
{
  struct narray farray;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  if (chkobjlastinst(d->obj) == -1)
    return;
  SelectDialog(&DlgSelect, d->obj, _("delete parameter (multi select)"), ParameterCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    int i, num, *array;
    num = arraynum(&farray);
    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_DELETE, d->obj->name);
    }
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(d->obj, array[i]);
      set_graph_modified();
    }
    ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
  }
  arraydel(&farray);
}

void
CmParameterUpdate(void *w, gpointer client_data)
{
  struct narray farray;
  int modified;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  if (chkobjlastinst(d->obj) == -1)
    return;
  SelectDialog(&DlgSelect, d->obj, _("parameter property (multi select)"), ParameterCB, (struct narray *) &farray, NULL);
  modified = FALSE;
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    int i, *array, num;
    num = arraynum(&farray);
    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    }
    array = arraydata(&farray);
    for (i = 0; i < num; i++) {
      int ret;
      ParameterDialog(NgraphApp.ParameterWin.data.data, array[i], -1);
      ret = DialogExecute(TopLevel, &DlgParameter);
      if (ret != IDCANCEL) {
        modified = TRUE;
      }
    }
    if (modified) {
      ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
    }
  }
  arraydel(&farray);
}

static void
parameter_update(GtkButton *btn, gpointer data)
{
  int id, undo, ret;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  id = GPOINTER_TO_INT(data);
  undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  ParameterDialog(NgraphApp.ParameterWin.data.data, id, -1);
  ret = DialogExecute(TopLevel, &DlgParameter);
  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
    set_graph_modified();
  }
}

static void
parameter_up(GtkButton *btn, gpointer data)
{
  int id;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  id = GPOINTER_TO_INT(data);
  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  moveupobj(d->obj, id);
  ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
  set_graph_modified();
}

static void
parameter_down(GtkButton *btn, gpointer data)
{
  int id;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  id = GPOINTER_TO_INT(data);
  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movedownobj(d->obj, id);
  ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
  set_graph_modified();
}

static void
parameter_delete(GtkButton *btn, gpointer data)
{
  int id;
  struct obj_list_data *d;

  if (Menulock || Globallock)
    return;

  d = NgraphApp.ParameterWin.data.data;
  id = GPOINTER_TO_INT(data);
  menu_save_undo_single(UNDO_TYPE_DELETE, d->obj->name);
  delobj(d->obj, id);
  ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, FALSE);
  set_graph_modified();
}

static void
parameter_play(GtkButton *btn, gpointer data)
{
  int id, loop, wait;
  double start, stop, step, prm;
  struct obj_list_data *d;
  GtkWidget *scale;

  if (Menulock || Globallock)
    return;

  scale = g_object_get_data(G_OBJECT(btn), "user-data");
  id = GPOINTER_TO_INT(data);
  d = NgraphApp.ParameterWin.data.data;
  getobj(d->obj, "start", id, 0, NULL, &start);
  getobj(d->obj, "stop", id, 0, NULL, &stop);
  getobj(d->obj, "transition_step", id, 0, NULL, &step);
  getobj(d->obj, "loop", id, 0, NULL, &loop);
  getobj(d->obj, "wait", id, 0, NULL, &wait);
  menu_lock(TRUE);
  for (prm = start; fabs(prm - start) <= fabs(stop - start); prm += step) {
    gtk_range_set_value(GTK_RANGE(scale), prm);
    set_parameter(prm, data);
    reset_event();
    msleep(wait * 10);
    prm = gtk_range_get_value(GTK_RANGE(scale));
  }
  menu_lock(FALSE);
}

static void
add_button(GtkWidget *grid, int row, int col, const char *icon, const char *tooltip, GCallback proc)
{
  GtkWidget *w;
  w = gtk_button_new_from_icon_name(icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_tooltip_text(GTK_WIDGET(w), tooltip);
  gtk_widget_set_vexpand(GTK_WIDGET(w), FALSE);
  gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(grid), w, col, row, 1, 1);
  g_signal_connect(w, "clicked", proc, GINT_TO_POINTER(row));
}

static GtkWidget *
create_combo_box(char *str, int selected)
{
  char **items;
  int i;
  GtkWidget *w;

  w = combo_box_create();
  if (str == NULL) {
    return w;
  }
  items = g_strsplit(str, "\n", -1);
  if (items == NULL) {
    return w;
  }
  for (i = 0; items[i]; i++) {
    combo_box_append_text(w, items[i]);
  }
  g_strfreev(items);
  combo_box_set_active(w, selected);
  return w;
}

static void
set_parameter(double prm, gpointer user_data)
{
  N_VALUE *inst;
  int id;
  char const *objects[] = {"data", NULL};
  struct obj_list_data *d;

  d = NgraphApp.ParameterWin.data.data;
  id = GPOINTER_TO_INT(user_data);
  inst = chkobjinst(d->obj, id);
  if (inst == NULL) {
    return;
  }

  _putobj(d->obj, "parameter", inst, &prm);
  ViewerWinUpdate(objects);
}

static void
scale_changed(GtkRange *range, gpointer user_data)
{
  double value;
  if (Menulock || Globallock)
    return;

  value = gtk_range_get_value(range);
  set_parameter(value, user_data);
}

static void
value_changed(GtkAdjustment *adjustment, gpointer user_data)
{
  double value;
  if (Menulock || Globallock)
    return;

  value = gtk_adjustment_get_value(adjustment);
  set_parameter(value, user_data);
}

static void
toggled(GtkToggleButton *toggle_button, gpointer user_data)
{
  int active;
  if (Menulock || Globallock)
    return;

  active = gtk_toggle_button_get_active(toggle_button);
  set_parameter(active, user_data);
}

static void
switched(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data)
{
  int active;
  if (Menulock || Globallock)
    return;

  active = gtk_switch_get_active(sw);
  set_parameter(active, user_data);
}

static void
combo_changed(GtkComboBox *combo_box, gpointer user_data)
{
  int selected;
  if (Menulock || Globallock)
    return;

  selected = gtk_combo_box_get_active(combo_box);
  set_parameter(selected, user_data);
}

static void
check_min_max(double *min, double *max, double *inc)
{
  double span;
  span = fabs(*max - *min);
  if (*inc == 0) {
    *inc = span / 10;
  }
  if (*min == *max) {
    *max += *inc;
  } else if (*min > *max) {
    double tmp;
    tmp = *min;
    *min = *max;
    *max = tmp;
  }
}

static GtkWidget *
create_scale(double min, double max, double inc, double value)
{
  int invert;
  GtkWidget *scale;

  invert = FALSE;
  if (inc < 0) {
    invert = TRUE;
    inc = -inc;
  }
  if (min == max) {
    max += inc;
  } else if (min > max) {
    double tmp;
    tmp = min;
    min = max;
    max = tmp;
  }
  scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, inc);
  gtk_widget_set_size_request(GTK_WIDGET(scale), 200, -1);
  gtk_range_set_inverted(GTK_RANGE(scale), invert);
  gtk_range_set_value(GTK_RANGE(scale), value);

  return scale;
}

static void
create_widget(struct obj_list_data *d, int id, int n)
{
  int type, checked, col, selected, width, wrap, loop;
  double min, max, step, parameter, start, stop, transition_step;
  GtkWidget *w, *label, *separator, *button;
  char buf[32], *title, *items;
  GtkAdjustment *adj;

  width = 1;
  getobj(d->obj, "title", id, 0, NULL, &title);
  getobj(d->obj, "type", id, 0, NULL, &type);
  getobj(d->obj, "min", id, 0, NULL, &min);
  getobj(d->obj, "max", id, 0, NULL, &max);
  getobj(d->obj, "step", id, 0, NULL, &step);
  getobj(d->obj, "wrap", id, 0, NULL, &wrap);
  getobj(d->obj, "start", id, 0, NULL, &start);
  getobj(d->obj, "stop", id, 0, NULL, &stop);
  getobj(d->obj, "transition_step", id, 0, NULL, &transition_step);
  getobj(d->obj, "loop", id, 0, NULL, &loop);
  getobj(d->obj, "items", id, 0, NULL, &items);
  getobj(d->obj, "parameter", id, 0, NULL, &parameter);
  checked = selected = parameter;
  switch (type) {
  case PARAMETER_TYPE_SPIN:
    w = gtk_spin_button_new_with_range(min, max, step);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), parameter);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(w), wrap);
    gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
    gtk_widget_set_hexpand(GTK_WIDGET(w), FALSE);
    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(w));
    g_signal_connect(adj, "value-changed", G_CALLBACK(value_changed), GINT_TO_POINTER(id));
    break;
  case PARAMETER_TYPE_SCALE:
    w = create_scale(min, max, step, parameter);
    g_signal_connect(w, "value-changed", G_CALLBACK(scale_changed), GINT_TO_POINTER(id));
    break;
  case PARAMETER_TYPE_CHECK:
    if (title) {
      w = gtk_check_button_new_with_mnemonic(title);
      g_signal_connect(w, "toggled", G_CALLBACK(toggled), GINT_TO_POINTER(id));
      title = NULL;
    } else {
      w = gtk_check_button_new();
    }
    width = 2;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), checked);
    break;
  case PARAMETER_TYPE_COMBO:
    w = create_combo_box(items, selected);
    gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
    gtk_widget_set_hexpand(w, FALSE);
    g_signal_connect(w, "changed", G_CALLBACK(combo_changed), GINT_TO_POINTER(id));
    break;
  case PARAMETER_TYPE_SWITCH:
    w = gtk_switch_new();
    gtk_switch_set_state(GTK_SWITCH(w), checked);
    gtk_widget_set_hexpand(GTK_WIDGET(w), FALSE);
    gtk_widget_set_vexpand(GTK_WIDGET(w), FALSE);
    gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
    g_signal_connect(w, "notify::active", G_CALLBACK(switched), GINT_TO_POINTER(id));
    break;
  case PARAMETER_TYPE_TRANSITION:
    w = create_scale(start, stop, step, parameter);
    gtk_scale_set_has_origin(GTK_SCALE(w), FALSE);
    g_signal_connect(w, "value-changed", G_CALLBACK(scale_changed), GINT_TO_POINTER(id));
    break;
  }

  col = 0;
  snprintf(buf, sizeof(buf), "%d", id);
  label = gtk_button_new_with_label(buf);
  g_signal_connect(label, "clicked", G_CALLBACK(parameter_update), GINT_TO_POINTER(id));
  gtk_grid_attach(GTK_GRID(d->text), label, col, id, 1, 1);

  col++;
  separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach(GTK_GRID(d->text), separator, col, id, 1, 1);

  col++;
  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_widget_set_halign(GTK_WIDGET(label), GTK_ALIGN_START);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
    gtk_grid_attach(GTK_GRID (d->text), label, col, id, 1, 1);
  }
  col++;
  gtk_grid_attach(GTK_GRID(d->text), w, col - width + 1, id, width, 1);

  col++;
  if (type == PARAMETER_TYPE_TRANSITION) {
    button = gtk_button_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(GTK_WIDGET(button), _("Start"));
    gtk_widget_set_vexpand(GTK_WIDGET(button), FALSE);
    gtk_widget_set_valign(GTK_WIDGET(button), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(button), FALSE);
    gtk_widget_set_halign(GTK_WIDGET(button), GTK_ALIGN_START);
    g_object_set_data(G_OBJECT(button), "user-data", w);
    g_signal_connect(button, "clicked", G_CALLBACK(parameter_play), GINT_TO_POINTER(id));
    gtk_grid_attach(GTK_GRID(d->text), button, col, id, 1, 1);
  }

  col++;
  separator = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(separator), GTK_SHADOW_NONE);
  gtk_widget_set_hexpand(GTK_WIDGET(separator), TRUE);
  gtk_grid_attach(GTK_GRID(d->text), separator, col, id, 1, 1);

  col++;
  separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach(GTK_GRID(d->text), separator, col, id, 1, 1);

  col++;
  if (id > 0) {
    add_button(d->text, id, col, "go-up-symbolic", _("Up"), G_CALLBACK(parameter_up));
  }

  col++;
  if (id < n) {
    add_button(d->text, id, col, "go-down-symbolic", _("Down"), G_CALLBACK(parameter_down));
  }

  col++;
  add_button(d->text, id, col, "edit-delete-symbolic", _("Delete"), G_CALLBACK(parameter_delete));
}

void
ParameterWinUpdate(struct obj_list_data *d, int clear, int draw)
{
  int num, i;
  GtkWidget *row;

  if (Menulock || Globallock)
    return;

  if (d == NULL) {
    return;
  }

  while (1) {
    row = gtk_grid_get_child_at(GTK_GRID(d->text), 0, 0);
    if (row == NULL) {
      break;
    }
    gtk_grid_remove_row(GTK_GRID(d->text), 0);
  }
  num = chkobjlastinst(d->obj);
  for (i = 0; i <= num; i++) {
    create_widget(d, i, num);
  }
  add_button(d->text, num + 1, 0, "list-add-symbolic", "Add", G_CALLBACK(CmParameterAdd));

  gtk_widget_show_all(GTK_WIDGET(d->text));
}

GtkWidget *
create_parameter_list(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  parameter_sub_window_create(d);

  d->data.data->update = ParameterWinUpdate;
  d->data.data->setup_dialog = ParameterDialog;
  d->data.data->dialog = &DlgParameter;
  d->data.data->obj = chkobject("parameter");
  ParameterWinUpdate(d->data.data, FALSE, FALSE);

  return d->Win;
}
