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

struct parameter_data
{
  int playing;
  GtkWidget *scale, *repeat;
  struct objlist *obj;
  int id;
  N_VALUE *inst;
  struct obj_list_data *obj_list_data;
};

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
  SetWidgetFromObjField(d->start, d->Obj, id, "start");
  SetWidgetFromObjField(d->stop, d->Obj, id, "stop");
  SetWidgetFromObjField(d->transition_step, d->Obj, id, "step");
  SetWidgetFromObjField(d->wait, d->Obj, id, "wait");
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
  if (title == NULL) {
    title = "";
  }
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
  case PARAMETER_TYPE_TRANSITION:
    gtk_stack_set_visible_child_name(GTK_STACK(d->stack), TYPE_TRANSITION_NAME);
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
    add_page_transition(d);

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
  int ret, type;
  char *text;
  double min, max, value, step;

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
  if (SetObjFieldFromWidget(d->wrap, d->Obj, d->Id, "wrap"))
    return;
  if (SetObjFieldFromWidget(d->start, d->Obj, d->Id, "start"))
    return;
  if (SetObjFieldFromWidget(d->stop, d->Obj, d->Id, "stop"))
    return;
  if (SetObjFieldFromWidget(d->wait, d->Obj, d->Id, "wait"))
    return;
  if (SetObjFieldFromWidget(d->checked, d->Obj, d->Id, "active"))
    return;
  if (SetObjFieldFromWidget(d->redraw, d->Obj, d->Id, "redraw"))
    return;
  if (SetObjFieldFromWidget(d->selected, d->Obj, d->Id, "selected"))
    return;
  if (SetObjFieldFromWidget(d->value, d->Obj, d->Id, "value"))
    return;

  type = gtk_combo_box_get_active(GTK_COMBO_BOX(d->type));
  if (type == PARAMETER_TYPE_TRANSITION) {
    if (SetObjFieldFromWidget(d->transition_step, d->Obj, d->Id, "step")) {
      return;
    }
  } else {
    if (SetObjFieldFromWidget(d->step, d->Obj, d->Id, "step")) {
      return;
    }
  }

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(d->items));
  text = get_text_from_buffer(buf);
  if (chk_sputobjfield(d->Obj, d->Id, "items", text)) {
    g_free(text);
    return;
  }
  g_free(text);

  getobj(d->Obj, "min", d->Id, 0, NULL, &min);
  getobj(d->Obj, "max", d->Id, 0, NULL, &max);
  getobj(d->Obj, "step", d->Id, 0, NULL, &step);
  getobj(d->Obj, "value", d->Id, 0, NULL, &value);
  check_min_max(&min, &max, &step);
  if (value < min) {
    value = min;
  } else if (value > max) {
    value = max;
  }
  putobj(d->Obj, "min", d->Id, &min);
  putobj(d->Obj, "max", d->Id, &max);
  putobj(d->Obj, "step", d->Id, &step);
  putobj(d->Obj, "value", d->Id, &value);

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
  ParameterDialog(d, id, -1);
  ret = DialogExecute(TopLevel, &DlgParameter);
  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
  }
  ParameterWinUpdate(d, FALSE, FALSE);
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
    ParameterWinUpdate(d, FALSE, FALSE);
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
      ParameterDialog(d, array[i], -1);
      ret = DialogExecute(TopLevel, &DlgParameter);
      if (ret != IDCANCEL) {
        modified = TRUE;
      }
    }
    if (modified) {
      ParameterWinUpdate(d, FALSE, FALSE);
    }
  }
  arraydel(&farray);
}

static void
parameter_update(GtkButton *btn, gpointer data)
{
  int undo, ret;
  struct parameter_data *d;

  if (Menulock || Globallock)
    return;

  d = data;
  undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
  ParameterDialog(d->obj_list_data, d->id, -1);
  ret = DialogExecute(TopLevel, &DlgParameter);
  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
  }
  ParameterWinUpdate(d->obj_list_data, FALSE, FALSE);
}

static void
parameter_up(GtkButton *btn, gpointer data)
{
  struct parameter_data *d;

  if (Menulock || Globallock)
    return;

  d = data;
  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  moveupobj(d->obj, d->id);
  ParameterWinUpdate(d->obj_list_data, FALSE, FALSE);
  set_graph_modified();
}

static void
parameter_down(GtkButton *btn, gpointer data)
{
  struct parameter_data *d;

  if (Menulock || Globallock)
    return;

  d = data;
  menu_save_undo_single(UNDO_TYPE_ORDER, d->obj->name);
  movedownobj(d->obj, d->id);
  ParameterWinUpdate(d->obj_list_data, FALSE, FALSE);
  set_graph_modified();
}

static void
parameter_delete(GtkButton *btn, gpointer data)
{
  struct parameter_data *d;

  if (Menulock || Globallock)
    return;

  d = data;
  menu_save_undo_single(UNDO_TYPE_DELETE, d->obj->name);
  delobj(d->obj, d->id);
  ParameterWinUpdate(d->obj_list_data, FALSE, FALSE);
  set_graph_modified();
}

static void
set_pause_icon(GtkButton *btn)
{
  GtkWidget *icon;
  icon = gtk_image_new_from_icon_name("media-playback-pause-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_tooltip_text(GTK_WIDGET(btn), _("Pause"));
  gtk_button_set_image(btn, icon);
}

static void
set_play_icon(GtkButton *btn)
{
  GtkWidget *icon;
  icon = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_tooltip_text(GTK_WIDGET(btn), _("Play"));
  gtk_button_set_image(btn, icon);
}

static void
parameter_play(GtkButton *btn, gpointer user_data)
{
  int wait;
  double start, stop, step, prm;
  GtkWidget *scale;
  struct parameter_data *data;

  data = user_data;

  if (data->playing) {
    data->playing = FALSE;
  }

  if (Menulock || Globallock)
    return;

  scale = data->scale;
  getobj(data->obj, "start", data->id, 0, NULL, &start);
  getobj(data->obj, "stop", data->id, 0, NULL, &stop);
  getobj(data->obj, "step", data->id, 0, NULL, &step);
  getobj(data->obj, "wait", data->id, 0, NULL, &wait);
  if (start == stop) {
    return;
  }
  if (step == 0) {
    return;
  }
  if (start > stop) {
    step = -step;
  }
  menu_lock(TRUE);

  set_pause_icon(btn);
  prm = gtk_range_get_value(GTK_RANGE(scale));
  if (prm >= stop) {
    prm = start;
  }
  data->playing = TRUE;
  while (1) {
    while (fabs(prm - start) <= fabs(stop - start)) {
      gtk_range_set_value(GTK_RANGE(scale), prm);
      set_parameter(prm, data);
      reset_event();
      msleep(wait * 10);
      prm = gtk_range_get_value(GTK_RANGE(scale));
      if (! data->playing) {
	goto EndPlaying;
      }
      prm += step;
    }
    if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->repeat))) {
      break;
    }
    prm = start;
  }
 EndPlaying:
  if (prm > stop) {
    prm = stop;
  }
  data->playing = FALSE;
  set_play_icon(btn);
  set_parameter(prm, data);
  menu_lock(FALSE);
}

static void
parameter_skip_backward(GtkButton *btn, gpointer user_data)
{
  double start;
  struct parameter_data *data;

  data = user_data;

  if ((! data->playing) && (Menulock || Globallock)) {
    return;
  }

  getobj(data->obj, "start", data->id, 0, NULL, &start);
  gtk_range_set_value(GTK_RANGE(data->scale), start);
}

static void
parameter_skip_forward(GtkButton *btn, gpointer user_data)
{
  double stop;
  struct parameter_data *data;

  data = user_data;

  if ((! data->playing) && (Menulock || Globallock)) {
    return;
  }

  getobj(data->obj, "stop", data->id, 0, NULL, &stop);
  gtk_range_set_value(GTK_RANGE(data->scale), stop);
}

static void
add_button_common(GtkWidget *w, GtkWidget *grid, int row, int col, const char *tooltip, GCallback proc, gpointer data)
{
  gtk_widget_set_tooltip_text(GTK_WIDGET(w), tooltip);
  gtk_widget_set_vexpand(GTK_WIDGET(w), FALSE);
  gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(GTK_WIDGET(w), FALSE);
  gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), w, col, row, 1, 1);
  if (proc) {
    g_signal_connect(w, "clicked", proc, data);
  }
}

static GtkWidget *
add_button(GtkWidget *grid, int row, int col, const char *icon, const char *tooltip, GCallback proc, gpointer data)
{
  GtkWidget *w;
  w = gtk_button_new_from_icon_name(icon, GTK_ICON_SIZE_BUTTON);
  add_button_common(w, grid, row, col, tooltip, proc, data);
  return w;
}

static GtkWidget *
add_toggle_button(GtkWidget *grid, int row, int col, const char *icon_name, const char *tooltip, GCallback proc, gpointer data)
{
  GtkWidget *w, *icon;
  w = gtk_toggle_button_new();
  icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON(w), icon);
  add_button_common(w, grid, row, col, tooltip, proc, data);
  return w;
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

struct redraw_info
{
  int redraw_flag, redraw_num;
};

static int
get_redraw_info(struct redraw_info *info)
{
  if (getobj(Menulocal.obj, "redraw_num", 0, 0, NULL, &info->redraw_num) == -1) {
    return 1;
  }
  if (getobj(Menulocal.obj, "redraw_flag", 0, 0, NULL, &info->redraw_flag) == -1) {
    return 1;
  }
  return 0;
}

static int
put_redraw_info(struct redraw_info *info)
{
  if (putobj(Menulocal.obj, "redraw_num", 0, &info->redraw_num) == -1) {
    return 1;
  }
  if (putobj(Menulocal.obj, "redraw_flag", 0, &info->redraw_flag) == -1) {
    return 1;
  }
  return 0;
}

static void
set_parameter(double prm, gpointer user_data)
{
  int redraw;
  char const *objects[] = {"data", NULL};
  struct parameter_data *d;

  d = user_data;
  if (d->inst == NULL) {
    return;
  }
  _putobj(d->obj, "parameter", d->inst, &prm);
  _getobj(d->obj, "redraw", d->inst, &redraw);
  if (redraw) {
    struct redraw_info save_info, info;
    if (get_redraw_info(&save_info)) {
      return;
    }
    info.redraw_num = 0;
    info.redraw_flag = TRUE;
    if (put_redraw_info(&info)) {
      return;
    }
    ViewerWinUpdate(objects);
    if (put_redraw_info(&save_info)) {
      return;
    }
  }
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
  if (Menulock || Globallock) {
    gtk_toggle_button_set_inconsistent(toggle_button, TRUE);
    return;
  }

  gtk_toggle_button_set_inconsistent(toggle_button, FALSE);
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
create_spin_button(double min, double max, double inc, int wrap, double value)
{
  GtkWidget *spin_button;

  check_min_max(&min, &max, &inc);
  spin_button = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), value);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button), wrap);
  gtk_widget_set_halign(GTK_WIDGET(spin_button), GTK_ALIGN_START);
  gtk_widget_set_hexpand(GTK_WIDGET(spin_button), FALSE);

  return spin_button;
}

static GtkWidget *
create_scale(double min, double max, double inc, double value)
{
  GtkWidget *scale;

  check_min_max(&min, &max, &inc);
  scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, inc);
  gtk_widget_set_size_request(GTK_WIDGET(scale), 200, -1);
  gtk_range_set_value(GTK_RANGE(scale), value);

  return scale;
}

static void
delete_parameter_data(gpointer data)
{
  g_free(data);
}

static struct parameter_data *
create_parameter_data(struct obj_list_data *d, int id)
{
  struct parameter_data *data;

  data = g_malloc(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  data->playing = FALSE;
  data->scale = NULL;
  data->repeat = NULL;
  data->obj = d->obj;
  data->id = id;
  data->obj_list_data = d;
  data->inst = chkobjinst(d->obj, id);
  return data;
}

static int
create_play_buttons(GtkWidget *grid, int id, int col, GtkWidget *scale, struct parameter_data *data)
{
  GtkWidget *button;

  add_button(grid, id, col, "media-skip-backward-symbolic", _("To start"), G_CALLBACK(parameter_skip_backward), data);

  col++;
  add_button(grid, id, col, "media-playback-start-symbolic", _("Play"), G_CALLBACK(parameter_play), data);

  col++;
  add_button(grid, id, col, "media-skip-forward-symbolic", _("To stop"), G_CALLBACK(parameter_skip_forward), data);

  col++;
  button = add_toggle_button(grid, id, col, "media-playlist-repeat-symbolic", _("Repeat"), NULL, NULL);
  data->repeat = button;
  data->scale = scale;

  return col;
}

static void
create_widget(struct obj_list_data *d, int id, int n)
{
  int type, checked, col, selected, width, wrap;
  double min, max, step, parameter, start, stop;
  GtkWidget *w, *label, *separator;
  char buf[32], *title, *items;
  GtkAdjustment *adj;
  struct parameter_data *data;

  width = 1;
  getobj(d->obj, "title", id, 0, NULL, &title);
  getobj(d->obj, "type", id, 0, NULL, &type);
  getobj(d->obj, "min", id, 0, NULL, &min);
  getobj(d->obj, "max", id, 0, NULL, &max);
  getobj(d->obj, "step", id, 0, NULL, &step);
  getobj(d->obj, "wrap", id, 0, NULL, &wrap);
  getobj(d->obj, "start", id, 0, NULL, &start);
  getobj(d->obj, "stop", id, 0, NULL, &stop);
  getobj(d->obj, "items", id, 0, NULL, &items);
  getobj(d->obj, "parameter", id, 0, NULL, &parameter);
  checked = selected = parameter;

  data = create_parameter_data(d, id);
  if (data == NULL) {
    return;
  }
  switch (type) {
  case PARAMETER_TYPE_SPIN:
    w = create_spin_button(min, max, step, wrap, parameter);
    adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(w));
    g_signal_connect(adj, "value-changed", G_CALLBACK(value_changed), data);
    break;
  case PARAMETER_TYPE_SCALE:
    w = create_scale(min, max, step, parameter);
    g_signal_connect(w, "value-changed", G_CALLBACK(scale_changed), data);
    break;
  case PARAMETER_TYPE_CHECK:
    if (title) {
      w = gtk_check_button_new_with_mnemonic(title);
      title = NULL;
    } else {
      w = gtk_check_button_new();
    }
    width = 2;
    g_signal_connect(w, "toggled", G_CALLBACK(toggled), data);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), checked);
    break;
  case PARAMETER_TYPE_COMBO:
    w = create_combo_box(items, selected);
    gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
    gtk_widget_set_hexpand(w, FALSE);
    g_signal_connect(w, "changed", G_CALLBACK(combo_changed), data);
    break;
  case PARAMETER_TYPE_SWITCH:
    w = gtk_switch_new();
    gtk_switch_set_state(GTK_SWITCH(w), checked);
    gtk_widget_set_hexpand(GTK_WIDGET(w), FALSE);
    gtk_widget_set_vexpand(GTK_WIDGET(w), FALSE);
    gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
    gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
    g_signal_connect(w, "notify::active", G_CALLBACK(switched), data);
    break;
  case PARAMETER_TYPE_TRANSITION:
    w = create_scale(start, stop, step, parameter);
    gtk_scale_set_has_origin(GTK_SCALE(w), FALSE);
    g_signal_connect(w, "value-changed", G_CALLBACK(scale_changed), data);
    break;
  default:
    g_free(data);
    return;
  }

  col = 0;
  snprintf(buf, sizeof(buf), "%d", id);
  label = gtk_button_new_with_label(buf);
  g_signal_connect(label, "clicked", G_CALLBACK(parameter_update), data);
  g_object_set_data_full(G_OBJECT(label), "user-data", data, delete_parameter_data);
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
    col = create_play_buttons(d->text, id, col, w, data);
    if (col < 0) {
      return;
    }
  } else {
    col += 3;
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
    add_button(d->text, id, col, "go-up-symbolic", _("Up"), G_CALLBACK(parameter_up), data);
  }

  col++;
  if (id < n) {
    add_button(d->text, id, col, "go-down-symbolic", _("Down"), G_CALLBACK(parameter_down), data);
  }

  col++;
  add_button(d->text, id, col, "edit-delete-symbolic", _("Delete"), G_CALLBACK(parameter_delete), data);
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
  add_button(d->text, num + 1, 0, "list-add-symbolic", _("Add"), G_CALLBACK(CmParameterAdd), NULL);

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
