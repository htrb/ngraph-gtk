/*
 * $Id: x11axis.c,v 1.81 2010-03-04 08:30:17 hito Exp $
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
#include "nstring.h"
#include "mathfn.h"
#include "gra.h"
#include "axis.h"

#include "gtk_columnview.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"
#include "gtk_presettings.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11graph.h"
#include "x11file.h"
#include "x11view.h"
#include "x11axis.h"
#include "x11commn.h"

static void bind_minmax (struct objlist *obj, int id, const char *field, GtkWidget *w);
static void bind_inc (struct objlist *obj, int id, const char *field, GtkWidget *w);
static void bind_name (struct objlist *obj, int id, const char *field, GtkWidget *w);

static n_list_store Alist[] = {
  {" ",        G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",        G_TYPE_INT,     FALSE, FALSE, "id"},
  {N_("name"), G_TYPE_STRING,  FALSE, FALSE, "group", bind_name},
  {N_("min"),  G_TYPE_STRING,  TRUE,  TRUE,  "min", bind_minmax},
  {N_("max"),  G_TYPE_STRING,  TRUE,  TRUE,  "max", bind_minmax},
  {N_("inc"),  G_TYPE_STRING,  TRUE,  TRUE,  "inc", bind_inc},
  {N_("type"), G_TYPE_PARAM,   TRUE,  FALSE, "type"},
  {"x",        G_TYPE_DOUBLE,  TRUE,  FALSE, "x",     NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",        G_TYPE_DOUBLE,  TRUE,  FALSE, "y",     NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("dir"),  G_TYPE_DOUBLE,  TRUE,  FALSE, "direction",     NULL,        0,          36000, 100, 1500},
  {N_("len"),  G_TYPE_DOUBLE,  TRUE,  FALSE, "length",     NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"^#",       G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define AXIS_WIN_COL_NUM G_N_ELEMENTS (Alist)

static void axiswin_scale_clear(GSimpleAction *action, GVariant *parameter, gpointer app);
static void axis_delete_popup_func(GSimpleAction *action, GVariant *parameter, gpointer app);
static void AxisWinAxisTop(GSimpleAction *action, GVariant *parameter, gpointer app);
static void AxisWinAxisUp(GSimpleAction *action, GVariant *parameter, gpointer app);
static void AxisWinAxisDown(GSimpleAction *action, GVariant *parameter, gpointer app);
static void AxisWinAxisLast(GSimpleAction *action, GVariant *parameter, gpointer app);

static GActionEntry Popup_list[] =
{
  {"axisFocusAllAction",        list_sub_window_focus_all, NULL, NULL, NULL},
  {"axisOrderTopAction",        AxisWinAxisTop, NULL, NULL, NULL},
  {"axisOrderUpAction",         AxisWinAxisUp, NULL, NULL, NULL},
  {"axisOrderDownAction",       AxisWinAxisDown, NULL, NULL, NULL},
  {"axisOrderBottomAction",     AxisWinAxisLast, NULL, NULL, NULL},
  {"axisAddFrameGraphAction",   CmAxisAddFrame, NULL, NULL, NULL},
  {"axisAddSectionGraphAction", CmAxisAddSection, NULL, NULL, NULL},
  {"axisAddCrossGraphAction",   CmAxisAddCross, NULL, NULL, NULL},
  {"axisAddSingleGraphAction",  CmAxisAddSingle, NULL, NULL, NULL},

  {"axisDuplicateAction",       list_sub_window_copy, NULL, NULL, NULL},
  {"axisDeleteAction",          axis_delete_popup_func, NULL, NULL, NULL},
  {"axisFocusAction",           list_sub_window_focus, NULL, NULL, NULL},
  {"axisClearAction",           axiswin_scale_clear, NULL, NULL, NULL},
  {"axisUpdateAction",          list_sub_window_update, NULL, NULL, NULL},
  {"axisInstanceNameAction",    list_sub_window_object_name, NULL, NULL, NULL},
};

#define POPUP_ITEM_NUM ((int) (sizeof(Popup_list) / sizeof(*Popup_list)))

#define POPUP_ITEM_FOCUS_ALL 0
#define POPUP_ITEM_TOP       1
#define POPUP_ITEM_UP        2
#define POPUP_ITEM_DOWN      3
#define POPUP_ITEM_BOTTOM    4
#define POPUP_ITEM_ADD_F     5
#define POPUP_ITEM_ADD_S     6
#define POPUP_ITEM_ADD_C     7
#define POPUP_ITEM_ADD_A     8

#define TITLE_BUF_SIZE 128

static int check_axis_history(struct objlist *obj);

#define TIME_FORMAT_STR N_(						\
  "%a	The abbreviated weekday name.\n"				\
  "%A	The full weekday name.\n"					\
  "%b	The abbreviated month name.\n"					\
  "%B	The full month name.\n"						\
  "%c	Equivalent to %a %b %e %T %Y.\n"				\
  "%C	The century number (year/100).\n"				\
  "%d	The day of the month (01 to 31).\n"				\
  "%e	The day of the month (1 to 31).\n"				\
  "%F	Equivalent to %Y-%m-%d.\n"					\
  "%H	The hour (00 to 23).\n"						\
  "%I	The hour (01 to 12).\n"						\
  "%j	The day of the year (001 to 366).\n"				\
  "%k	The hour (0 to 23).\n"						\
  "%l	The hour (1 to 12).\n"						\
  "%m	The month (01 to 12).\n"					\
  "%M	The minute (00 to 59).\n"					\
  "%n	A newline character.\n"						\
  "%p	Either \"AM\" or \"PM\".\n"					\
  "%P	Either \"am\" or \"pm\".\n"					\
  "%r	Equivalent to %I:%M:%S %p.\n"					\
  "%R	Equivalent to %H:%M.\n"						\
  "%S	The second (00 to 60).\n"					\
  "%T	Equivalent to %H:%M:%S.\n"					\
  "%u	The day of the week, Monday being 1 (1 to 7).\n"		\
  "%w	The day of the week, Sunday being 0 (0 to 6).\n"		\
  "%y	The year without a century (00 to 99).\n"			\
  "%Y	The year including the century.\n"				\
  "%+	Equivalent to %a %b %e %T GMT %Y.\n"				\
  "%%	A literal '%' character.")

void
axis_scale_push(struct objlist *obj, int id)
{
  int n;

  exeobj(obj, "scale_push", id, 0, NULL);

  n = check_axis_history(obj);
  set_axis_undo_button_sensitivity(n > 0);
}

char *
AxisCB(struct objlist *obj, int id)
{
  char *s, *valstr, *name;
  int dir;

  getobj(obj, "direction", id, 0, NULL, &dir);
  getobj(obj, "group", id, 0, NULL, &name);

  s = NULL;
  name = CHK_STR(name);
  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr) {
    s = g_strdup_printf("%-10s %.6s %s:%.2f", name, _(valstr), _("dir"), dir / 100.0);
    g_free(valstr);
  }

  return s;
}

char *
AxisHistoryCB(struct objlist *obj, int id)
{
  int num;
  struct narray *array;

  getobj(obj, "scale_history", id, 0, NULL, &array);

  num = arraynum(array) / 3;
  if (num == 0)
    return NULL;

  return AxisCB(obj, id);
}

static char *
GridCB(struct objlist *obj, int id)
{
  char *s, *s1, *s2;

  getobj(obj, "axis_x", id, 0, NULL, &s1);
  getobj(obj, "axis_y", id, 0, NULL, &s2);
  s = g_strdup_printf("%.8s %.8s", (s1)? s1: FILL_STRING, (s2)? s2: FILL_STRING);

  return s;
}

static void
bg_button_toggled(GtkCheckButton *button, gpointer user_data)
{
  struct GridDialog *d;
  gboolean state;

  d = (struct GridDialog *) user_data;

  state = gtk_check_button_get_active(button);
  set_widget_sensitivity_with_label(d->bcolor, state);
}

static void
GridDialogSetupItem(struct GridDialog *d, int id)
{
  int i;

  axis_combo_box_setup(d->axisx, d->Obj, id, "axis_x");
  axis_combo_box_setup(d->axisy, d->Obj, id, "axis_y");

  SetWidgetFromObjField(d->draw_x, d->Obj, id, "grid_x");
  SetWidgetFromObjField(d->draw_y, d->Obj, id, "grid_y");

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1";

    style[sizeof(style) - 2] += i;
    SetStyleFromObjField(d->style[i], d->Obj, id, style);

    width[sizeof(width) - 2] += i;
    SetWidgetFromObjField(d->width[i], d->Obj, id, width);
  }
  SetWidgetFromObjField(d->background, d->Obj, id, "background");
  bg_button_toggled(GTK_CHECK_BUTTON(d->background), d);

  set_color(d->color, d->Obj, id, NULL);
  set_color(d->bcolor, d->Obj, id, "B");
}

static void
grid_copy_click_response(int sel, gpointer user_data)
{
  struct GridDialog *d;
  d = (struct GridDialog *) user_data;
  if (sel != -1) {
    GridDialogSetupItem(d, sel);
  }
}

static void
grid_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct GridDialog *d;
  d = (struct GridDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, GridCB, grid_copy_click_response, d);
}

static void
gauge_syle_setup(struct GridDialog *d, GtkWidget *table, int n, int j, int instance)
{
  GtkWidget *w;
  char buf[TITLE_BUF_SIZE];

  if (n < 0 || n >= GRID_DIALOG_STYLE_NUM)
    return;

  snprintf(buf, sizeof(buf), _("_Style %d:"), n + 1);
  w = combo_box_entry_create();
  add_widget_to_table_sub(table, w, buf, TRUE, 0, 1, 4, j);
  d->style[n] = w;

  if (instance) {
    snprintf(buf, sizeof(buf), _("_Width %d:"), n + 1);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
    add_widget_to_table_sub(table, w, buf, FALSE, 2, 1, 4, j);
  } else {
    w = NULL;
  }
  d->width[n] = w;
}

static void
GridDialogSetupCommon(GtkWidget *wi, void *data, int makewidget, int instance)
{
  struct GridDialog *d;

  d = (struct GridDialog *) data;
  if (instance) {
    char title[TITLE_BUF_SIZE];
    snprintf(title, sizeof(title), _("Grid %d"), d->Id);
    gtk_window_set_title(GTK_WINDOW(wi), title);
  }

  d = (struct GridDialog *) data;
  if (makewidget) {
    GtkWidget *frame, *w, *hbox, *table;
    int i, j;

    if (instance) {
      gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);
    }
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    if (instance) {
      j = 0;
      w = axis_combo_box_create(AXIS_COMBO_BOX_USE_OID);
      add_widget_to_table(table, w, _("Axis (_X):"), FALSE, j++);
      d->axisx = w;

      w = gtk_check_button_new_with_mnemonic(_("draw _X grid"));
      add_widget_to_table(table, w, NULL, FALSE, j++);
      d->draw_x = w;

      w = axis_combo_box_create(AXIS_COMBO_BOX_USE_OID);
      add_widget_to_table(table, w, _("Axis (_Y):"), FALSE, j++);
      d->axisy = w;

      w = gtk_check_button_new_with_mnemonic(_("draw _Y grid"));
      add_widget_to_table(table, w, NULL, FALSE, j++);
      d->draw_y = w;

      frame = gtk_frame_new(_("Axis"));
      gtk_frame_set_child(GTK_FRAME(frame), table);
      gtk_box_append(GTK_BOX(hbox), frame);
    } else {
      d->axisx = NULL;
      d->axisy = NULL;
      d->draw_x = NULL;
      d->draw_y = NULL;
    }

    table = gtk_grid_new();

    j = 0;
    w = create_color_button(wi);
    add_widget_to_table(table, w, _("_Color:"), FALSE, j++);
    d->color = w;

    w = gtk_check_button_new_with_mnemonic(_("_Background"));
    add_widget_to_table(table, w, NULL, FALSE, j++);
    g_signal_connect(w, "toggled", G_CALLBACK(bg_button_toggled), d);
    d->background = w;

    w = create_color_button(wi);
    add_widget_to_table(table, w, _("_Background Color:"), FALSE, j++);
    d->bcolor = w;

    frame = gtk_frame_new(_("Color"));
    gtk_widget_set_hexpand(frame, TRUE);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    table = gtk_grid_new();

    j = 0;
    for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
      gauge_syle_setup(d, table, i, j++, instance);
    }

    frame = gtk_frame_new(_("Style"));
    gtk_widget_set_vexpand(frame, TRUE);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(d->vbox), frame);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(grid_copy_clicked), d, "axisgrid");
  }

  GridDialogSetupItem(d, d->Id);
}

static void
GridDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GridDialogSetupCommon(wi, data, makewidget, TRUE);
}

static void
GridDialogClose(GtkWidget *w, void *data)
{
  struct GridDialog *d;
  int ret;
  int i;

  d = (struct GridDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjAxisFieldFromWidget(d->axisx, d->Obj, d->Id, "axis_x"))
    return;

  if (SetObjAxisFieldFromWidget(d->axisy, d->Obj, d->Id, "axis_y"))
    return;

  for (i = 0; i < GRID_DIALOG_STYLE_NUM; i++) {
    char width[] = "width1", style[] = "style1";

    style[sizeof(style) - 2] += i;
    if (SetObjFieldFromStyle(d->style[i], d->Obj, d->Id, style))
      return;

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromWidget(d->width[i], d->Obj, d->Id, width))
      return;
  }

  if (SetObjFieldFromWidget(d->draw_x, d->Obj, d->Id, "grid_x"))
    return;

  if (SetObjFieldFromWidget(d->draw_y, d->Obj, d->Id, "grid_y"))
    return;

  if (SetObjFieldFromWidget(d->background, d->Obj, d->Id, "background"))
    return;

  if (putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

  if (putobj_color(d->bcolor, d->Obj, d->Id, "B"))
    return;

  d->ret = ret;
}

void
GridDialog(struct GridDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = GridDialogSetup;
  data->CloseWindow = GridDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
GridDefDialogSetup(GtkWidget *w, void *data, int makewidget)
{
  GridDialogSetupCommon(w, data, makewidget, FALSE);
}

static void
GridDefDialogClose_response(int ret, struct objlist *obj, int id, int modified)
{
  if (ret) {
    exeobj(obj, "save_config", id, 0, NULL);
  }
  delobj(obj, id);
  if (! modified) {
    reset_graph_modified();
  }
}

static void
GridDefDialogClose(GtkWidget *w, void *data)
{
  struct GridDialog *d;
  GridDialogClose(w, data);

  d = (struct GridDialog *) data;
  if (d->ret == IDOK) {
    CheckIniFile(GridDefDialogClose_response, d->Obj, d->Id, d->modified);
  } else {
    GridDefDialogClose_response(FALSE, d->Obj, d->Id, d->modified);
  }
}

static void
GridDefDialog(struct GridDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = GridDefDialogSetup;
  data->CloseWindow = GridDefDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
option_grid_def_response(struct response_callback *cb)
{
  struct GridDialog *d;
  int modified;
  d = (struct GridDialog *) cb->dialog;
  modified = GPOINTER_TO_INT(cb->data);
  if (cb->return_value == IDOK) {
    CheckIniFile(GridDefDialogClose_response, d->Obj, d->Id, modified);
  } else {
    GridDefDialogClose_response(FALSE, d->Obj, d->Id, modified);
  }
}

void
CmOptionGridDef(void *w, gpointer client_data)
{
  struct objlist *obj;
  int id;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("axisgrid")) == NULL)
    return;

  id = newobj(obj);
  if (id >= 0) {
    int modified;

    modified = get_graph_modified();
    GridDefDialog(&DlgGridDef, obj, id);
    DlgGridDef.modified = modified;
    response_callback_add(&DlgGridDef, option_grid_def_response, NULL, GINT_TO_POINTER(modified));
    DialogExecute(TopLevel, &DlgGridDef);
  }
}

static void
set_axis_id(GtkWidget *w, int id)
{
  char buf[256];

  if (id != -1) {
    snprintf(buf, sizeof(buf), "id:%d", id);
  } else {
    buf[0] = '\0';
  }
  gtk_label_set_text(GTK_LABEL(w), buf);
}

static void
SectionDialogSetupItem(GtkWidget *w, struct SectionDialog *d)
{
  spin_entry_set_val(d->x, d->X);
  spin_entry_set_val(d->y, d->Y);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);
  set_axis_id(d->uid, d->IDU);
  set_axis_id(d->rid, d->IDR);
  set_axis_id(d->gid, d->IDG);

  spin_entry_set_val(d->width, d->LenX);
  spin_entry_set_val(d->height, d->LenY);
}

static void
SectionDialogAxisX(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDX >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDX, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisY(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDY >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDY, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisU(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDU >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDU, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
SectionDialogAxisR(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) client_data;
  if (d->IDR >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDR, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static int
axis_save_undo(int type)
{
  char *arg[4];
  arg[0] = "axis";
  arg[1] = "axisgrid";
  arg[2] = "data";
  arg[3] = NULL;
  return menu_save_undo(type, arg);
}

struct section_dialog_grid_data
{
  int create, undo;
  struct SectionDialog *d;
};

static void
section_dialog_grid_response(struct response_callback *cb)
{
  struct section_dialog_grid_data *data;
  struct SectionDialog *d;
  int create, undo;

  data = (struct section_dialog_grid_data *) cb->data;
  create = data->create;
  undo = data->undo;
  d = data->d;

  switch (cb->return_value) {
  case IDCANCEL:
    menu_undo_internal(undo);
    if (create) {
      d->IDG = -1;
    }
    break;
  case IDDELETE:
    if (create) {
      menu_undo_internal(undo);
    } else {
      delobj(d->Obj2, d->IDG);
      set_graph_modified();
    }
    d->IDG = -1;
    break;
  default:
    menu_delete_undo(undo);
    set_graph_modified();
  }
  SectionDialogSetupItem(d->widget, d);
  g_clear_pointer(&cb->data, g_free);
}

static void
SectionDialogGrid(GtkWidget *w, gpointer client_data)
{
  struct SectionDialog *d;
  int oidx, oidy, create = FALSE, undo = -1;

  d = (struct SectionDialog *) client_data;
  if (d->IDG == -1) {
    undo = axis_save_undo(UNDO_TYPE_DUMMY);
    if ((d->IDG = newobj(d->Obj2)) >= 0) {
      char *ref;
      getobj(d->Obj, "oid", d->IDX, 0, NULL, &oidx);
      ref = g_strdup_printf("axis:^%d", oidx);
      if (ref) {
	putobj(d->Obj2, "axis_x", d->IDG, ref);
      }
      getobj(d->Obj, "oid", d->IDY, 0, NULL, &oidy);
      ref = g_strdup_printf("axis:^%d", oidy);
      if (ref) {
	putobj(d->Obj2, "axis_y", d->IDG, ref);
      }
      create = TRUE;
      presetting_set_obj_field(d->Obj2, d->IDG);
    }
  }
  if (d->IDG >= 0) {
    struct section_dialog_grid_data *data;
    GridDialog(&DlgGrid, d->Obj2, d->IDG);
    data = g_malloc0(sizeof(*data));
    data->create = create;
    data->undo = undo;
    data->d = d;
    response_callback_add(&DlgGrid, section_dialog_grid_response, NULL, data);
    DialogExecute(d->widget, &DlgGrid);
  }
}

static void
SectionDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct SectionDialog *d;

  d = (struct SectionDialog *) data;

  if (makewidget) {
    GtkWidget *w, *hbox, *vbox, *table;
    int i;
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    gtk_box_append(GTK_BOX(hbox), table);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Height:"), FALSE, i++);
    d->height = w;

    gtk_box_append(GTK_BOX(hbox), table);
    gtk_box_append(GTK_BOX(d->vbox), hbox);


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_X axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisX), d);
    d->xaxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->xid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);



    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_Y axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisY), d);
    d->yaxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->yid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_U axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisU), d);
    d->uaxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->uid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_R axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogAxisR), d);
    d->raxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->rid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_Grid"));
    g_signal_connect(w, "clicked", G_CALLBACK(SectionDialogGrid), d);
    d->grid = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->gid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
  }

  SectionDialogSetupItem(wi, d);
}

static void
SectionDialogClose(GtkWidget *w, void *data)
{
  struct SectionDialog *d;
  int ret;
  int type;
  struct narray group;

  d = (struct SectionDialog *) data;
  if (d->ret != IDOK)
    return;
  ret = d->ret;

  d->ret = IDLOOP;

  d->X = spin_entry_get_val(d->x);
  d->Y = spin_entry_get_val(d->y);

  d->LenX = spin_entry_get_val(d->width);
  d->LenY = spin_entry_get_val(d->height);

  if ((d->X != d->X0) || (d->Y != d->Y0)
      || (d->LenX0 != d->LenX) || (d->LenY0 != d->LenY)) {
    char *argv[2];
    arrayinit(&group, sizeof(int));
    if (d->Section)
      type = 2;
    else
      type = 1;
    arrayadd(&group, &type);
    arrayadd(&group, &(d->IDX));
    arrayadd(&group, &(d->IDY));
    arrayadd(&group, &(d->IDU));
    arrayadd(&group, &(d->IDR));
    arrayadd(&group, &(d->X));
    arrayadd(&group, &(d->Y));
    arrayadd(&group, &(d->LenX));
    arrayadd(&group, &(d->LenY));
    argv[0] = (char *) &group;
    argv[1] = NULL;
    exeobj(d->Obj, "group_position", d->IDX, 1, argv);
    arraydel(&group);
    set_graph_modified();
  }
  d->ret = ret;
}

void
SectionDialog(struct SectionDialog *data,
	      int x, int y, int lenx, int leny,
	      struct objlist *obj, int idx, int idy, int idu, int idr,
	      struct objlist *obj2, int idg, int section)
{
  data->SetupWindow = SectionDialogSetup;
  data->CloseWindow = SectionDialogClose;
  data->X0 = data->X = x;
  data->Y0 = data->Y = y;
  data->LenX0 = data->LenX = lenx;
  data->LenY0 = data->LenY = leny;
  data->Obj = obj;
  data->Obj2 = obj2;
  data->IDX = idx;
  data->IDY = idy;
  data->IDU = idu;
  data->IDR = idr;
  data->IDG = idg;
  data->Section = section;
  data->MaxX = Menulocal.PaperWidth * (10000.0 / Menulocal.PaperZoom);
  data->MaxY = Menulocal.PaperHeight * (10000.0 / Menulocal.PaperZoom);
}

static void
CrossDialogSetupItem(GtkWidget *w, struct CrossDialog *d)
{
  spin_entry_set_val(d->x, d->X);
  spin_entry_set_val(d->y, d->Y);

  set_axis_id(d->xid, d->IDX);
  set_axis_id(d->yid, d->IDY);

  spin_entry_set_val(d->width, d->LenX);
  spin_entry_set_val(d->height, d->LenY);
}

static void
CrossDialogAxisX(GtkWidget *w, gpointer client_data)
{
  struct CrossDialog *d;

  d = (struct CrossDialog *) client_data;
  if (d->IDX >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDX, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
CrossDialogAxisY(GtkWidget *w, gpointer client_data)
{
  struct CrossDialog *d;

  d = (struct CrossDialog *) client_data;
  if (d->IDY >= 0) {
    AxisDialog(NgraphApp.AxisWin.data.data, d->IDY, -1);
    DialogExecute(d->widget, &DlgAxis);
  }
}

static void
CrossDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct CrossDialog *d;

  d = (struct CrossDialog *) data;
  if (makewidget) {
    GtkWidget *w, *hbox, *vbox, *table;
    int i;
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    gtk_box_append(GTK_BOX(hbox), table);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("Graph _Height:"), FALSE, i++);
    d->height = w;

    gtk_box_append(GTK_BOX(hbox), table);
    gtk_box_append(GTK_BOX(d->vbox), hbox);


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_X axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(CrossDialogAxisX), d);
    d->xaxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->xid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_Y axis"));
    g_signal_connect(w, "clicked", G_CALLBACK(CrossDialogAxisY), d);
    d->yaxis = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_label_new(NULL);
    d->yid = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
  }
  CrossDialogSetupItem(wi, d);
}

static void
CrossDialogClose(GtkWidget *w, void *data)
{
  struct CrossDialog *d;
  int ret;
  int type;
  struct narray group;

  d = (struct CrossDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;

  d->ret = IDLOOP;

  d->X = spin_entry_get_val(d->x);
  d->Y = spin_entry_get_val(d->y);

  d->LenX = spin_entry_get_val(d->width);
  d->LenY = spin_entry_get_val(d->height);

  if ((d->X != d->X0) || (d->Y != d->Y0)
      || (d->LenX != d->LenX0) || (d->LenY != d->LenY0)) {
    char *argv[2];
    arrayinit(&group, sizeof(int));
    type = 3;
    arrayadd(&group, &type);
    arrayadd(&group, &(d->IDX));
    arrayadd(&group, &(d->IDY));
    arrayadd(&group, &(d->X));
    arrayadd(&group, &(d->Y));
    arrayadd(&group, &(d->LenX));
    arrayadd(&group, &(d->LenY));
    argv[0] = (char *) &group;
    argv[1] = NULL;
    exeobj(d->Obj, "group_position", d->IDX, 1, argv);
    arraydel(&group);
    set_graph_modified();
  }
  if ((d->IDX != -1) && (d->IDY != -1)) {
    exeobj(d->Obj, "adjust", d->IDX, 0, NULL);
    exeobj(d->Obj, "adjust", d->IDY, 0, NULL);
  }
  d->ret = ret;
}

void
CrossDialog(struct CrossDialog *data,
	    int x, int y, int lenx, int leny,
	    struct objlist *obj, int idx, int idy)
{
  data->SetupWindow = CrossDialogSetup;
  data->CloseWindow = CrossDialogClose;
  data->X0 = data->X = x;
  data->Y0 = data->Y = y;
  data->LenX0 = data->LenX = lenx;
  data->LenY0 = data->LenY = leny;
  data->Obj = obj;
  data->IDX = idx;
  data->IDY = idy;
  data->MaxX = Menulocal.PaperWidth * (10000.0 / Menulocal.PaperZoom);
  data->MaxY = Menulocal.PaperHeight * (10000.0 / Menulocal.PaperZoom);
}

static void
ZoomDialogSetupItem(GtkWidget *w, struct ZoomDialog *d)
{
  spin_entry_set_val(d->zoom_entry, d->zoom);
}

static void
ZoomDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct ZoomDialog *d;

  d = (struct ZoomDialog *) data;
  if (makewidget) {
    GtkWidget *w, *vbox;
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
    item_setup(vbox, w, _("_Zoom:"), TRUE);
    d->zoom_entry = w;
    gtk_box_append(GTK_BOX(d->vbox), vbox);
  }
  ZoomDialogSetupItem(wi, d);
}

static void
ZoomDialogClose(GtkWidget *w, void *data)
{
  struct ZoomDialog *d;

  d = (struct ZoomDialog *) data;
  if (d->ret != IDOK)
    return;

  d->zoom = spin_entry_get_val(d->zoom_entry);
}

void
ZoomDialog(struct ZoomDialog *data)
{
  data->SetupWindow = ZoomDialogSetup;
  data->CloseWindow = ZoomDialogClose;
}

static void
scale_tab_setup_item(struct AxisDialog *d, int id)
{
  double min, max, inc;
  struct narray *array;
  double *data;
  char buf[30];

  combo_box_clear(d->min);
  combo_box_clear(d->max);
  combo_box_clear(d->inc);

  getobj(d->Obj, "min", id, 0, NULL, &min);
  getobj(d->Obj, "max", id, 0, NULL, &max);
  getobj(d->Obj, "inc", id, 0, NULL, &inc);

  getobj(d->Obj, "scale_history", d->Id, 0, NULL, &array);
  if (array) {
    double pmin, pmax, pinc;
    int num, j;
    pmin = min;
    pmax = max;
    pinc = inc;
    num = arraynum(array) / 3;
    data = arraydata(array);
    for (j = 0; j < num; j++) {
      if (data[0 + j * 3] != pmin) {
	snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, data[0 + j * 3]);
	combo_box_append_text(d->min, buf);
      }
      pmin = data[0 + j * 3];

      if (data[1 + j * 3] != pmax) {
	snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, data[1 + j * 3]);
	combo_box_append_text(d->max, buf);
      }
      pmax = data[1 + j * 3];

      if (data[2 + j * 3] != pinc) {
	snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, data[2 + j * 3]);
	combo_box_append_text(d->inc, buf);
      }
      pinc = data[2 + j * 3];
    }
  }

  if ((min == 0) && (max == 0) && (inc == 0)) {
    combo_box_entry_set_text(d->min, "0");
    combo_box_entry_set_text(d->max, "0");
    combo_box_entry_set_text(d->inc, "0");
  } else {
    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, min);
    combo_box_entry_set_text(d->min, buf);

    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, max);
    combo_box_entry_set_text(d->max, buf);

    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, inc);
    combo_box_entry_set_text(d->inc, buf);
  }

  SetWidgetFromObjField(d->div, d->Obj, id, "div");
  SetWidgetFromObjField(d->scale, d->Obj, id, "type");

  axis_combo_box_setup(d->ref, d->Obj, id, "reference");
  SetWidgetFromObjField(d->margin, d->Obj, id, "auto_scale_margin");
}

static void
AxisDialogClear(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;
  combo_box_entry_set_text(d->min, "0");
  combo_box_entry_set_text(d->max, "0");
  combo_box_entry_set_text(d->inc, "0");
}

static void
scale_by_files(struct AxisDialog *d, struct narray *farray)
{
  int a, anum, num, *array;

  num = arraynum(farray);
  array = arraydata(farray);
  anum = chkobjlastinst(d->Obj);

  if (num > 0 && anum != 0) {
    GString *str;
    int type;
    struct narray *result;

    str = g_string_sized_new(32);
    if (str) {
      char *buf, *argv2[2];
      int i;
      g_string_append(str, "data:");
      for (i = 0; i < num; i++) {
	if (i == num - 1) {
	  g_string_append_printf(str, "%d", array[i]);
	} else {
	  g_string_append_printf(str, "%d,", array[i]);
	}
      }

      buf = g_string_free(str, FALSE);
      argv2[0] = (char *) buf;
      argv2[1] = NULL;

      if (getobj(d->Obj, "type", d->Id, 0, NULL, &type) == -1) {
	arraydel(farray);
	g_free(buf);
	return;
      }

      a = combo_box_get_active(d->scale);
      if (a >= 0 && (putobj(d->Obj, "type", d->Id, &a) == -1)) {
	arraydel(farray);
	g_free(buf);
	return;
      }

      getobj(d->Obj, "get_auto_scale", d->Id, 1, argv2, &result);
      g_free(buf);

      if (arraynum(result) == 3) {
	char s[30];

	snprintf(s, sizeof(s), DOUBLE_STR_FORMAT, arraynget_double(result, 0));
	combo_box_entry_set_text(d->min, s);

	snprintf(s, sizeof(s), DOUBLE_STR_FORMAT, arraynget_double(result, 1));
	combo_box_entry_set_text(d->max, s);

	snprintf(s, sizeof(s), DOUBLE_STR_FORMAT, arraynget_double(result, 2));
	combo_box_entry_set_text(d->inc, s);
      }
      putobj(d->Obj, "type", d->Id, &type);
    }
  }
}

struct axis_dialog_file_data {
  struct narray *farray;
  struct AxisDialog *d;
};

static void
axis_dialog_file_response(struct response_callback *cb)
{
  struct axis_dialog_file_data *data;

  data = (struct axis_dialog_file_data *) cb->data;
  if (cb->return_value == IDOK) {
    scale_by_files(data->d, data->farray);
  }
  arrayfree(data->farray);
  g_clear_pointer(&cb->data, g_free);
}

static void
AxisDialogFile(GtkWidget *w, gpointer client_data)
{
  struct AxisDialog *d;
  struct objlist *fobj;
  struct narray *farray;
  struct axis_dialog_file_data *data;

  d = (struct AxisDialog *) client_data;

  fobj = chkobject("data");
  if (fobj == NULL)
    return;

  if (chkobjlastinst(fobj) == -1)
    return;

  farray = arraynew(sizeof(int));
  if (farray == NULL)
    return;

  SelectDialog(&DlgSelect, fobj, _("autoscale (multi select)"), FileCB, (struct narray *) farray, NULL);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    arrayfree(farray);
    return;
  }
  data->farray = farray;
  data->d = d;
  response_callback_add(&DlgSelect, axis_dialog_file_response, NULL, data);
  DialogExecute(d->widget, &DlgSelect);
}

static void
file_button_show(GtkWidget *widget, gpointer user_data)
{
  static struct objlist *file = NULL;
  int n;

  if (file == NULL) {
    file = chkobject("data");
  }

  if (file == NULL)
    return;

  n = chkobjlastinst(file);

  gtk_widget_set_sensitive(widget, n >= 0);
}

static void
scale_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    scale_tab_setup_item(d, sel);
  }
}

static void
scale_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, scale_tab_copy_click_response, d);
}

static GtkWidget *
scale_tab_create(struct AxisDialog *d)
{
  GtkWidget *parent_box, *w, *frame, *table, *hbox;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Min:"), TRUE, i++);
  d->min = w;

  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Max:"), TRUE, i++);
  d->max = w;

  w = combo_box_entry_create();
  add_widget_to_table(table, w, _("_Inc:"), TRUE, i++);
  d->inc = w;


  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

  w = gtk_button_new_with_mnemonic(_("_Clear"));
  g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogClear), d);
  gtk_box_append(GTK_BOX(hbox), w);

  w = gtk_button_new_with_mnemonic(_("_Data"));
  g_signal_connect(w, "clicked", G_CALLBACK(AxisDialogFile), d);
  g_signal_connect(w, "map", G_CALLBACK(file_button_show), NULL);
  gtk_box_append(GTK_BOX(hbox), w);

  add_widget_to_table(table, hbox, "", FALSE, i++);

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Scale:"), FALSE, i++);
  d->scale = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Div:"), FALSE, i++);
  d->div = w;

  w = axis_combo_box_create(AXIS_COMBO_BOX_USE_OID | AXIS_COMBO_BOX_ADD_NONE);
  add_widget_to_table(table, w, _("_Ref:"), FALSE, i++);
  d->ref = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Auto scale margin:"), FALSE, i++);
  d->margin = w;

  frame = gtk_frame_new(_("Scale"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  gtk_widget_set_vexpand(frame, TRUE);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  parent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(parent_box), frame);

  add_copy_button_to_box(parent_box, G_CALLBACK(scale_tab_copy_clicked), d, "axis");

  return parent_box;
}

static int
baseline_tab_set_value(struct AxisDialog *axis)
{
  struct AxisBase *d;

  d = &axis->base;

  if (SetObjFieldFromStyle(d->style, axis->Obj, axis->Id, "style"))
    return 1;

  if (SetObjFieldFromWidget(d->width, axis->Obj, axis->Id, "width"))
    return 1;

  if (SetObjFieldFromWidget(d->baseline, axis->Obj, axis->Id, "baseline"))
    return 1;

  if (SetObjFieldFromWidget(d->arrow, axis->Obj, axis->Id, "arrow"))
    return 1;

  if (SetObjFieldFromWidget(d->arrowlen, axis->Obj, axis->Id, "arrow_length"))
    return 1;

  if (SetObjFieldFromWidget(d->arrowwid, axis->Obj, axis->Id, "arrow_width"))
    return 1;

  if (SetObjFieldFromWidget(d->wave, axis->Obj, axis->Id, "wave"))
    return 1;

  if (SetObjFieldFromWidget(d->wavelen, axis->Obj, axis->Id, "wave_length"))
    return 1;

  if (SetObjFieldFromWidget(d->wavewid, axis->Obj, axis->Id, "wave_width"))
    return 1;

  if (putobj_color(d->color, axis->Obj, axis->Id, NULL))
    return 1;

  return 0;
}

static void
baseline_tab_setup_item(struct AxisDialog *axis, int id)
{
  struct AxisBase *d;

  d = &axis->base;

  SetStyleFromObjField(d->style, axis->Obj, id, "style");

  SetWidgetFromObjField(d->width, axis->Obj, id, "width");

  SetWidgetFromObjField(d->baseline, axis->Obj, id, "baseline");

  SetWidgetFromObjField(d->arrow, axis->Obj, id, "arrow");

  SetWidgetFromObjField(d->arrowlen, axis->Obj, id, "arrow_length");

  SetWidgetFromObjField(d->arrowwid, axis->Obj, id, "arrow_width");

  SetWidgetFromObjField(d->wave, axis->Obj, id, "wave");

  SetWidgetFromObjField(d->wavelen, axis->Obj, id, "wave_length");

  SetWidgetFromObjField(d->wavewid, axis->Obj, id, "wave_width");

  set_color(d->color, axis->Obj, id, NULL);
}

static void
baseline_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    baseline_tab_setup_item(d, sel);
  }
}

static void
baseline_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, baseline_tab_copy_click_response, d);
}

static GtkWidget *
baseline_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *hbox, *vbox, *frame, *table;
  struct AxisBase *d;
  int i;

  d = &dd->base;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  table = gtk_grid_new();

  i = 0;
  w = gtk_check_button_new_with_mnemonic(_("Draw _Baseline"));
  add_widget_to_table(table, w, NULL, FALSE , i++);
  d->baseline = w;

  w = combo_box_entry_create();
  combo_box_entry_set_width(w, NUM_ENTRY_WIDTH);
  add_widget_to_table(table, w, _("Line _Style:"), TRUE, i++);
  d->style = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Line Width:"), FALSE, i++);
  d->width = w;

  w = create_color_button(wi);
  add_widget_to_table(table, w, _("_Color:"), FALSE, i++);
  d->color = w;

  frame = gtk_frame_new(_("Baseline"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(hbox), frame);

  table = gtk_grid_new();
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Position:"), FALSE, i++);
  d->arrow = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_inc(w, 1000, 10000);
  add_widget_to_table(table, w, _("_Arrow length:"), FALSE, i++);
  d->arrowlen = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_inc(w, 1000, 10000);
  add_widget_to_table(table, w, _("_Arrow width:"), FALSE, i++);
  d->arrowwid = w;

  frame = gtk_frame_new(_("Arrow"));
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Position:"), FALSE, i++);
  d->wave = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Wave length:"), FALSE, i++);
  d->wavelen = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Wave width:"), FALSE, i++);
  d->wavewid = w;

  frame = gtk_frame_new(_("Wave"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_box_append(GTK_BOX(hbox), vbox);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), hbox);

  add_copy_button_to_box(vbox, G_CALLBACK(baseline_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
gauge_tab_set_value(struct AxisDialog *axis)
{
  int i;
  struct AxisGauge *d;

  d = &axis->gauge;

  if (SetObjFieldFromWidget(d->gauge, axis->Obj, axis->Id, "gauge"))
    return 1;

  if (SetObjFieldFromWidget(d->min, axis->Obj, axis->Id, "gauge_min"))
    return 1;

  if (SetObjFieldFromWidget(d->max, axis->Obj, axis->Id, "gauge_max"))
    return 1;

  if (SetObjFieldFromStyle(d->style, axis->Obj, axis->Id, "gauge_style"))
    return 1;

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1";

    width[sizeof(width) - 2] += i;
    if (SetObjFieldFromWidget(d->width[i], axis->Obj, axis->Id, width))
      return 1;

    length[sizeof(length) - 2] += i;
    if (SetObjFieldFromWidget(d->length[i], axis->Obj, axis->Id, length))
      return 1;
  }

  if (putobj_color(d->color, axis->Obj, axis->Id, "gauge_"))
    return 1;

  return 0;
}

static void
gauge_tab_setup_item(struct AxisDialog *axis, int id)
{
  int i;
  struct AxisGauge *d;

  d = &axis->gauge;

  SetWidgetFromObjField(d->gauge, axis->Obj, id, "gauge");

  SetWidgetFromObjField(d->min, axis->Obj, id, "gauge_min");

  SetWidgetFromObjField(d->max, axis->Obj, id, "gauge_max");

  SetStyleFromObjField(d->style, axis->Obj, id, "gauge_style");

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    char width[] = "gauge_width1", length[] = "gauge_length1";

    width[sizeof(width) - 2] += i;
    SetWidgetFromObjField(d->width[i], axis->Obj, id, width);

    length[sizeof(length) - 2] += i;
    SetWidgetFromObjField(d->length[i], axis->Obj, id, length);
  }

  set_color(d->color, axis->Obj, id, "gauge_");
}

static void
gauge_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    gauge_tab_setup_item(d, sel);
  }
}

static void
gauge_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, gauge_tab_copy_click_response, d);
}

static GtkWidget *
gauge_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *parent_box, *w, *vbox, *frame, *table;
  struct AxisGauge *d;
  int i, j;
  char buf[TITLE_BUF_SIZE];

  d = &dd->gauge;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  table = gtk_grid_new();

  j = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Gauge:"), FALSE, j++);
  d->gauge = w;

  w = create_text_entry(FALSE, TRUE);
  gtk_editable_set_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH * 2);
  add_widget_to_table(table, w, _("_Min:"), TRUE, j++);
  d->min = w;

  w = create_text_entry(FALSE, TRUE);
  gtk_editable_set_width_chars(GTK_EDITABLE(w), NUM_ENTRY_WIDTH * 2);
  add_widget_to_table(table, w, _("_Max:"), TRUE, j++);
  d->max = w;

  frame = gtk_frame_new(_("Range"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);

  table = gtk_grid_new();

  j = 0;
  w = combo_box_entry_create();
  add_widget_to_table_sub(table, w, _("_Style:"), TRUE, 0, 3, 4, j++);
  d->style = w;

  w = create_color_button(wi);
  add_widget_to_table_sub(table, w, _("_Color:"), FALSE, 0, 1, 4, j++);
  d->color = w;

  for (i = 0; i < GAUGE_STYLE_NUM; i++) {
    snprintf(buf, sizeof(buf), _("_Width %d:"), i + 1);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
    add_widget_to_table_sub(table, w, buf, TRUE, 0, 1, 4, j);
    d->width[i] = w;

    snprintf(buf, sizeof(buf), _("_Length %d:"), i + 1);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table_sub(table, w, buf, TRUE, 2, 1, 4, j++);
    d->length[i] = w;
  }

  frame = gtk_frame_new(_("Style"));
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);

  parent_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(parent_box), vbox);
  add_copy_button_to_box(parent_box, G_CALLBACK(gauge_tab_copy_clicked), dd, "axis");

  return parent_box;
}

static int
set_num_format(struct AxisDialog *axis, struct AxisNumbering *d)
{
  GString *format;
  int a;
  char *new, *old;

  format = g_string_new("%");
  if (format == NULL) {
    return 1;
  }

  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(d->add_plus))) {
    g_string_append_c(format, '+');
  }

  a = combo_box_get_active(d->fraction);
  if (a == 0) {
    g_string_append_c(format, 'g');
  } else if (a > 0) {
    g_string_append_printf(format, ".%df", a - 1);
  }

  if (getobj(axis->Obj, "num_format", axis->Id, 0, NULL, &old) == -1) {
    return 1;
  }

  new = g_string_free(format, FALSE);
  if (g_strcmp0(old, new)) {
    set_graph_modified();
  }

  if (putobj(axis->Obj, "num_format", axis->Id, new) == -1) {
    return 1;
  }

  if (getobj(axis->Obj, "num_format", axis->Id, 0, NULL, &new) == -1){
    return 1;
  }

  return 0;
}

static int
numbering_tab_set_value(struct AxisDialog *axis)
{
  struct AxisNumbering *d;

  d = &axis->numbering;

  if (SetObjFieldFromWidget(d->num, axis->Obj, axis->Id, "num"))
    return 1;

  if (SetObjFieldFromWidget(d->begin, axis->Obj, axis->Id, "num_begin"))
    return 1;

  if (SetObjFieldFromWidget(d->step, axis->Obj, axis->Id, "num_step"))
    return 1;

  if (SetObjFieldFromWidget(d->numnum, axis->Obj, axis->Id, "num_num"))
    return 1;

  if (SetObjFieldFromWidget(d->head, axis->Obj, axis->Id, "num_head"))
    return 1;

  if (set_num_format(axis, d))
    return 1;

  if (SetObjFieldFromWidget(d->tail, axis->Obj, axis->Id, "num_tail"))
    return 1;

  if (SetObjFieldFromWidget(d->date_format, axis->Obj, axis->Id, "num_date_format"))
    return 1;

  if (SetObjFieldFromWidget(d->align, axis->Obj, axis->Id, "num_align"))
    return 1;

  if (SetObjFieldFromWidget(d->direction, axis->Obj, axis->Id, "num_direction"))
    return 1;

  if (SetObjFieldFromWidget(d->shiftp, axis->Obj, axis->Id, "num_shift_p"))
    return 1;

  if (SetObjFieldFromWidget(d->shiftn, axis->Obj, axis->Id, "num_shift_n"))
    return 1;

  if (SetObjFieldFromWidget(d->log_power, axis->Obj, axis->Id, "num_log_pow"))
    return 1;

  if (SetObjFieldFromWidget(d->no_zero, axis->Obj, axis->Id, "num_no_zero"))
    return 1;

  if (SetObjFieldFromWidget(d->norm, axis->Obj, axis->Id, "num_auto_norm"))
    return 1;

  if (SetObjFieldFromWidget(d->math, axis->Obj, axis->Id, "num_math"))
    return 1;

  return 0;
}

static void
numbering_tab_setup_item(struct AxisDialog *axis, int id)
{
  char *format, *endptr;
  int j, a;
  struct AxisNumbering *d;

  d = &axis->numbering;

  SetWidgetFromObjField(d->num, axis->Obj, id, "num");

  SetWidgetFromObjField(d->begin, axis->Obj, id, "num_begin");

  SetWidgetFromObjField(d->step, axis->Obj, id, "num_step");

  SetWidgetFromObjField(d->numnum, axis->Obj, id, "num_num");

  SetWidgetFromObjField(d->head, axis->Obj, id, "num_head");

  combo_box_clear(d->fraction);
  for (j = 0; j < FwNumStyleNum; j++) {
    combo_box_append_text(d->fraction, _(FwNumStyle[j]));
  }

  getobj(axis->Obj, "num_format", id, 0, NULL, &format);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->add_plus), strchr(format, '+') != NULL);

  if ((strchr(format, 'f') == NULL) || (strchr(format, '.') == NULL)) {
    a = 0;
  } else {
    a = strtol(strchr(format, '.') + 1, &endptr, 10) + 1;
  }

  if (a < 0) {
    a = 0;
  } else if (a > 10) {
    a = 10;
  }

  combo_box_set_active(d->fraction, a);

  SetWidgetFromObjField(d->date_format, axis->Obj, id, "num_date_format");

  SetWidgetFromObjField(d->tail, axis->Obj, id, "num_tail");

  SetWidgetFromObjField(d->align, axis->Obj, id, "num_align");

  SetWidgetFromObjField(d->direction, axis->Obj, id, "num_direction");

  SetWidgetFromObjField(d->shiftp, axis->Obj, id, "num_shift_p");

  SetWidgetFromObjField(d->shiftn, axis->Obj, id, "num_shift_n");

  SetWidgetFromObjField(d->log_power, axis->Obj, id, "num_log_pow");

  SetWidgetFromObjField(d->no_zero, axis->Obj, id, "num_no_zero");

  SetWidgetFromObjField(d->norm, axis->Obj, id, "num_auto_norm");

  SetWidgetFromObjField(d->math, axis->Obj, id, "num_math");
}

static void
numbering_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    numbering_tab_setup_item(d, sel);
  }
}

static void
numbering_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, numbering_tab_copy_click_response, d);
}

static void
num_direction_changed(GtkWidget *w, GParamSpec *pspec, gpointer client_data)
{
  int dir, state;
  struct AxisDialog *d;

  d = (struct AxisDialog *) client_data;

  dir = combo_box_get_active(w);
  if (dir < 0) {
    return;
  }
  state = (dir != AXIS_NUM_POS_OBLIQUE1 && dir != AXIS_NUM_POS_OBLIQUE2);
  set_widget_sensitivity_with_label(d->numbering.align, state);
}

static GtkWidget *
numbering_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *hbox, *vbox, *frame, *table;
  struct AxisNumbering *d;
  int i;

  d = &dd->numbering;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Numbering:"), FALSE, i++);
  d->num = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Begin:"), FALSE, i++);
  d->begin = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Step:"), FALSE, i++);
  d->step = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NUM, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Num:"), FALSE, i++);
  d->numnum = w;

  frame = gtk_frame_new(_("Range"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Align:"), FALSE, i++);
  d->align = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Direction:"), FALSE, i++);
  g_signal_connect(w, "notify::selected", G_CALLBACK(num_direction_changed), dd);
  d->direction = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("shift (_P):"), FALSE, i++);
  d->shiftp = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("shift (_N):"), FALSE, i++);
  d->shiftn = w;

  frame = gtk_frame_new(_("Position"));
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(vbox), frame);
  gtk_box_append(GTK_BOX(hbox), vbox);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Fraction:"), FALSE, i++);
  d->fraction = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Head:"), TRUE, i++);
  d->head = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Tail:"), TRUE, i++);
  d->tail = w;

  w = create_text_entry(TRUE, TRUE);
  gtk_widget_set_tooltip_text(w, _(TIME_FORMAT_STR));
  add_widget_to_table(table, w, _("_Date/time format:"), TRUE, i++);
  d->date_format = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Auto normalization:"), FALSE, i++);
  d->norm = w;

  w = gtk_check_button_new_with_mnemonic(_("_Log power"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->log_power = w;

  w = gtk_check_button_new_with_mnemonic(_("_Add plus"));
  d->add_plus = w;
  add_widget_to_table(table, w, NULL, FALSE, i++);

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Zero:"), FALSE, i++);
  d->no_zero = w;

  w = create_text_entry(FALSE, TRUE);
  add_widget_to_table(table, w, _("numbering _Math:"), TRUE, i++);
  d->math = w;

  frame = gtk_frame_new(_("Format"));
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_RIGHT);

  gtk_box_append(GTK_BOX(hbox), frame);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), hbox);

  add_copy_button_to_box(vbox, G_CALLBACK(numbering_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
font_tab_set_value(struct AxisDialog *axis)
{
  struct AxisFont *d;
  int style, bold, italic, old_style;

  d = &axis->font;

  if (SetObjFieldFromWidget(d->space, axis->Obj, axis->Id, "num_space"))
    return 1;

  if (SetObjFieldFromWidget(d->pt, axis->Obj, axis->Id, "num_pt"))
    return 1;

  if (SetObjFieldFromWidget(d->script, axis->Obj, axis->Id, "num_script_size"))
    return 1;

  SetObjFieldFromFontList(d->font, axis->Obj, axis->Id, "num_font");

  if (putobj_color(d->color, axis->Obj, axis->Id, "num_"))
    return 1;

  style = 0;
  bold = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->font_bold));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }

  italic = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->font_italic));
  if (italic) {
    style |= GRA_FONT_STYLE_ITALIC;
  }

  getobj(axis->Obj, "num_font_style", axis->Id, 0, NULL, &old_style);
  if (old_style != style) {
    putobj(axis->Obj, "num_font_style", axis->Id, &style);
    set_graph_modified();
  }

  return 0;
}

static void
font_tab_setup_item(struct AxisDialog *axis, int id)
{
  struct compatible_font_info *compatible;
  struct AxisFont *d;
  int style;

  d = &axis->font;

  SetWidgetFromObjField(d->space, axis->Obj, id, "num_space");

  SetWidgetFromObjField(d->pt, axis->Obj, id, "num_pt");

  SetWidgetFromObjField(d->script, axis->Obj, id, "num_script_size");

  set_color(d->color, axis->Obj, id, "num_");

  compatible = SetFontListFromObj(d->font, axis->Obj, id, "num_font");

  if (compatible) {
    style = compatible->style;
  } else {
    getobj(axis->Obj, "num_font_style", axis->Id, 0, NULL, &style);
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->font_bold), style & GRA_FONT_STYLE_BOLD);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->font_italic), style & GRA_FONT_STYLE_ITALIC);
}

static void
font_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    font_tab_setup_item(d, sel);
  }
}

static void
font_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, font_tab_copy_click_response, d);
}

static GtkWidget *
font_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *vbox, *table, *frame, *btn_box;
  struct AxisFont *d;
  int i;

  d = &dd->font;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Point:"), FALSE, i++);
  d->pt = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_SPACE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Space:"), FALSE, i++);
  d->space = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Script size:"), FALSE, i++);
  d->script = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Font:"), FALSE, i++);
  d->font = w;

  btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  w = gtk_check_button_new_with_mnemonic(_("_Bold"));
  d->font_bold = w;
  gtk_box_append(GTK_BOX(btn_box), w);

  w = gtk_check_button_new_with_mnemonic(_("_Italic"));
  d->font_italic = w;
  gtk_box_append(GTK_BOX(btn_box), w);

  add_widget_to_table(table, btn_box, "", FALSE, i++);

  w = create_color_button(wi);
  add_widget_to_table(table, w, _("_Color:"), FALSE, i++);
  d->color = w;

  frame = gtk_frame_new(_("Font"));
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), frame);

  add_copy_button_to_box(vbox, G_CALLBACK(font_tab_copy_clicked), dd, "axis");

  return vbox;
}

static int
position_tab_set_value(struct AxisDialog *axis)
{
  struct AxisPos *d;

  d = &axis->position;

  if (SetObjFieldFromWidget(d->x, axis->Obj, axis->Id, "x"))
    return 1;

  if (SetObjFieldFromWidget(d->y, axis->Obj, axis->Id, "y"))
    return 1;

  if (SetObjFieldFromWidget(d->len, axis->Obj, axis->Id, "length"))
    return 1;

  if (SetObjFieldFromWidget(d->direction, axis->Obj, axis->Id, "direction"))
    return 1;

  if (SetObjAxisFieldFromWidget(d->adjust, axis->Obj, axis->Id, "adjust_axis"))
    return 1;

  if (SetObjFieldFromWidget(d->adjustpos, axis->Obj, axis->Id, "adjust_position"))
    return 1;

  return 0;
}

static void
position_tab_setup_item(struct AxisDialog *axis, int id)
{
  struct AxisPos *d;

  d = &axis->position;

  SetWidgetFromObjField(d->x, axis->Obj, id, "x");

  SetWidgetFromObjField(d->y, axis->Obj, id, "y");

  SetWidgetFromObjField(d->len, axis->Obj, id, "length");

  SetWidgetFromObjField(d->direction, axis->Obj, id, "direction");

  axis_combo_box_setup(d->adjust, axis->Obj, id, "adjust_axis");

  SetWidgetFromObjField(d->adjustpos, axis->Obj, id, "adjust_position");
}

static void
position_tab_copy_click_response(int sel, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  if (sel != -1) {
    position_tab_setup_item(d, sel);
  }
}

static void
position_tab_copy_clicked(GtkButton *btn, gpointer user_data)
{
  struct AxisDialog *d;
  d = (struct AxisDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, AxisCB, position_tab_copy_click_response, d);
}

static GtkWidget *
position_tab_create(GtkWidget *wi, struct AxisDialog *dd)
{
  GtkWidget *w, *vbox, *frame, *table;
  struct AxisPos *d;
  int i;

  d = &dd->position;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, "_X:", FALSE, i++);
  d->x = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, "_Y:", FALSE, i++);
  d->y = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Length:"), FALSE, i++);
  d->len = w;

  w = create_direction_entry(table, _("_Direction:"), i++);
  d->direction = w;

  w = axis_combo_box_create(AXIS_COMBO_BOX_USE_OID | AXIS_COMBO_BOX_ADD_NONE);
  add_widget_to_table(table, w, _("_Adjust:"), FALSE, i++);
  d->adjust = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("Adjust _Position:"), FALSE, i++);
  d->adjustpos = w;


  frame = gtk_frame_new(_("Position"));
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), frame);

  add_copy_button_to_box(vbox, G_CALLBACK(position_tab_copy_clicked), dd, "axis");

  return vbox;
}

static void
AxisDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct AxisDialog *d;
  char *group;
  char title[25];

  d = (struct AxisDialog *) data;
  getobj(d->Obj, "group", d->Id, 0, NULL, &group);
  group = CHK_STR(group);
  snprintf(title, sizeof(title), _("Axis %d %s"), d->Id, group);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *notebook, *w, *label;

    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), FALSE);
    w = scale_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Scale"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = baseline_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Baseline"));
    d->base.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = gauge_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Gauge"));
    d->gauge.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = numbering_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Numbering"));
    d->numbering.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = font_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Font"));
    d->font.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    w = position_tab_create(wi, d);
    label = gtk_label_new_with_mnemonic(_("_Position"));
    d->position.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

    gtk_box_append(GTK_BOX(d->vbox), notebook);

    d->tab = GTK_NOTEBOOK(notebook);
  }

  position_tab_setup_item(d, d->Id);
  font_tab_setup_item(d, d->Id);
  gauge_tab_setup_item(d, d->Id);
  numbering_tab_setup_item(d, d->Id);
  baseline_tab_setup_item(d, d->Id);
  scale_tab_setup_item(d, d->Id);
}

static int
scale_tab_set_value(struct AxisDialog *d)
{
  axis_scale_push(d->Obj, d->Id);

  if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "min")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "max")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->inc, d->Obj, d->Id, "inc")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->div, d->Obj, d->Id, "div")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->scale, d->Obj, d->Id, "type")) {
    return 1;
  }

  if (SetObjAxisFieldFromWidget(d->ref, d->Obj, d->Id, "reference")) {
    return 1;
  }

  if (SetObjFieldFromWidget(d->margin, d->Obj, d->Id, "auto_scale_margin")) {
    return 1;
  }

  return 0;
}

static void
AxisDialogClose(GtkWidget *w, void *data)
{
  struct AxisDialog *d;
  int ret;

  d = (struct AxisDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (scale_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, 0);
    return;
  }

  if (font_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->font.tab_id);
    return;
  }

  if (numbering_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->numbering.tab_id);
    return;
  }

  if (baseline_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->base.tab_id);
    return;
  }

  if (gauge_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->gauge.tab_id);
    return;
  }

  if (position_tab_set_value(d)){
    gtk_notebook_set_current_page(d->tab, d->position.tab_id);
    return;
  }

  d->ret = ret;
}

void
AxisDialog(struct obj_list_data *data, int id, int user_data)
{
  struct AxisDialog *d;

  d = (struct AxisDialog *) data->dialog;

  d->SetupWindow = AxisDialogSetup;
  d->CloseWindow = AxisDialogClose;
  d->Obj = data->obj;
  d->Id = id;
}

void
CmAxisAddFrame
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  enum TOOLBOX_MODE mode;
  mode = get_toolbox_mode();
  CmAxisNewFrame(mode == TOOLBOX_MODE_SETTING_PANEL, NULL);
}

void
CmAxisAddSection
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  enum TOOLBOX_MODE mode;
  mode = get_toolbox_mode();
  CmAxisNewSection(mode == TOOLBOX_MODE_SETTING_PANEL, NULL);
}

void
CmAxisAddCross
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  enum TOOLBOX_MODE mode;
  mode = get_toolbox_mode();
  CmAxisNewCross(mode == TOOLBOX_MODE_SETTING_PANEL, NULL);
}

static void
get_initial_axis_position(int *px, int *py)
{
  double x, y;
  int w, h, grid;
  grid = Menulocal.grid;
  w = Menulocal.PaperWidth;
  h = Menulocal.PaperHeight;
  x = (w - Menulocal.default_axis_width) / 2;
  y = (h + Menulocal.default_axis_height) / 2;
  * px = nround(x / grid) * grid - Menulocal.LeftMargin;
  grid = nround(1000.0 / grid) * grid;
  * py = nround(y / grid) * grid - Menulocal.TopMargin;
}

struct axis_new_data
{
  int undo;
  response_cb cb;
};

static void
axis_new_response(struct response_callback *cb)
{
  struct axis_new_data *data;
  int undo;
  response_cb new_cb;
  data = (struct axis_new_data *) cb->data;
  undo = data->undo;
  new_cb = data->cb;
  g_free(data);
  if (cb->return_value == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
  }
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
  if (new_cb) {
    new_cb(cb->return_value, NULL);
  }
}

void
CmAxisNewFrame(int use_presettings, response_cb cb)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg;
  int type, x, y, lenx, leny, undo;
  struct narray group;
  char *argv[2];
  struct axis_new_data *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if ((obj2 = getobject("axisgrid")) == NULL)
    return;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  undo = axis_save_undo(UNDO_TYPE_CREATE);
  idx = newobj(obj);
  idy = newobj(obj);
  idu = newobj(obj);
  idr = newobj(obj);
  idg = -1;
  arrayinit(&group, sizeof(int));
  type = 1;
  get_initial_axis_position(&x, &y);
  lenx = Menulocal.default_axis_width;
  leny = Menulocal.default_axis_height;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &idu);
  arrayadd(&group, &idr);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idr, 1, argv);
  arraydel(&group);
  if (use_presettings) {
    presetting_set_obj_field(obj, idx);
    presetting_set_obj_field(obj, idy);
    presetting_set_obj_field(obj, idu);
    presetting_set_obj_field(obj, idr);
  }
  SectionDialog(&DlgSection, x, y, lenx, leny, obj, idx, idy, idu, idr, obj2,
		idg, FALSE);
  data->undo = undo;
  data->cb = cb;
  response_callback_add(&DlgSection, axis_new_response, NULL, data);
  DialogExecute(TopLevel, &DlgSection);
  SectionDialog(&DlgSection, x, y, lenx, leny, obj, idx, idy, idu, idr, obj2,
		idg, FALSE);
}

void
CmAxisNewSection(int use_presettings, response_cb cb)
{
  struct objlist *obj, *obj2;
  int idx, idy, idu, idr, idg, oidx, oidy, undo;
  int type, x, y, lenx, leny;
  struct narray group;
  char *argv[2];
  struct axis_new_data *data;

  if (Menulock || Globallock)
    return;
  if ((obj = getobject("axis")) == NULL)
    return;
  if ((obj2 = getobject("axisgrid")) == NULL)
    return;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  undo = axis_save_undo(UNDO_TYPE_CREATE);
  idx = newobj(obj);
  idy = newobj(obj);
  idu = newobj(obj);
  idr = newobj(obj);
  idg = newobj(obj2);
  arrayinit(&group, sizeof(int));
  type = 2;
  get_initial_axis_position(&x, &y);
  lenx = Menulocal.default_axis_width;
  leny = Menulocal.default_axis_height;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &idu);
  arrayadd(&group, &idr);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idr, 1, argv);
  arraydel(&group);
  if (idg >= 0) {
    char *ref;
    getobj(obj, "oid", idx, 0, NULL, &oidx);
    ref = g_strdup_printf("axis:^%d", oidx);
    if (ref) {
      putobj(obj2, "axis_x", idg, ref);
    }
    getobj(obj, "oid", idy, 0, NULL, &oidy);
    ref = g_strdup_printf("axis:^%d", oidy);
    if (ref) {
      putobj(obj2, "axis_y", idg, ref);
    }
  }
  if (use_presettings) {
    presetting_set_obj_field(obj, idx);
    presetting_set_obj_field(obj, idy);
    presetting_set_obj_field(obj, idu);
    presetting_set_obj_field(obj, idr);
    presetting_set_obj_field(obj2, idg);
  }
  SectionDialog(&DlgSection, x, y, lenx, leny, obj, idx, idy, idu, idr, obj2,
		idg, TRUE);
  data->undo = undo;
  data->cb = cb;
  response_callback_add(&DlgSection, axis_new_response, NULL, data);
  DialogExecute(TopLevel, &DlgSection);
}

void
CmAxisNewCross(int use_presettings, response_cb cb)
{
  struct objlist *obj;
  int idx, idy;
  int type, x, y, lenx, leny, undo;
  struct narray group;
  char *argv[2];
  struct axis_new_data *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  undo = axis_save_undo(UNDO_TYPE_CREATE);
  idx = newobj(obj);
  idy = newobj(obj);
  arrayinit(&group, sizeof(int));
  type = 3;
  get_initial_axis_position(&x, &y);
  lenx = Menulocal.default_axis_width;
  leny = Menulocal.default_axis_height;
  arrayadd(&group, &type);
  arrayadd(&group, &idx);
  arrayadd(&group, &idy);
  arrayadd(&group, &x);
  arrayadd(&group, &y);
  arrayadd(&group, &lenx);
  arrayadd(&group, &leny);
  argv[0] = (char *) &group;
  argv[1] = NULL;
  exeobj(obj, "default_grouping", idy, 1, argv);
  arraydel(&group);
  if (use_presettings) {
    presetting_set_obj_field(obj, idx);
    presetting_set_obj_field(obj, idy);
  }
  CrossDialog(&DlgCross, x, y, lenx, leny, obj, idx, idy);
  data->undo = undo;
  data->cb = cb;
  response_callback_add(&DlgCross, axis_new_response, NULL, data);
  DialogExecute(TopLevel, &DlgCross);
}

void
CmAxisAddSingle
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct objlist *obj;
  int id, undo;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  undo = axis_save_undo(UNDO_TYPE_CREATE);
  if ((id = newobj(obj)) >= 0) {
    struct axis_new_data *data;
    data = g_malloc0(sizeof(*data));
    if (data == NULL) {
      return;
    }
    presetting_set_obj_field(obj, id);
    AxisDialog(NgraphApp.AxisWin.data.data, id, -1);
    data->undo = undo;
    data->cb = NULL;
    response_callback_add(&DlgAxis, axis_new_response, NULL, data);
    DialogExecute(TopLevel, &DlgAxis);
  }
}

static void
axis_del_response(struct response_callback *cb)
{
  if (cb->return_value == IDOK && DlgCopy.sel >= 0) {
    axis_save_undo(UNDO_TYPE_DELETE);
    AxisDel(DlgCopy.sel);
    set_graph_modified();
    AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  }
}

void
CmAxisDel(void *w, gpointer client_data)
{
  struct objlist *obj;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  CopyDialog(&DlgCopy, obj, -1, _("delete axis (single select)"), AxisCB);

  response_callback_add(&DlgCopy, axis_del_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgCopy);
}

static void
axis_update_response_response(struct response_callback *cb)
{
  int undo;
  undo = GPOINTER_TO_INT(cb->data);
  if (cb->return_value == IDCANCEL) {
    menu_delete_undo(undo);
  } else {
    AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  }
}

static void
axis_update_response(struct response_callback *cb)
{
  int i, undo;
  if (cb->return_value == IDOK) {
    i = DlgCopy.sel;
    if (i < 0) {
      return;
    }
  } else {
    return;
  }
  undo = axis_save_undo(UNDO_TYPE_EDIT);
  AxisDialog(NgraphApp.AxisWin.data.data, i, -1);
  response_callback_add(&DlgAxis, axis_update_response_response, NULL, GINT_TO_POINTER(undo));
  DialogExecute(TopLevel, &DlgAxis);
}

void
CmAxisUpdate(void *w, gpointer client_data)
{
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  CopyDialog(&DlgCopy, obj, -1, _("axis property (single select)"), AxisCB);
  response_callback_add(&DlgCopy, axis_update_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgCopy);
}

static void
axis_zoom(struct objlist *obj, struct narray *farray, double zoom)
{
  int *array, num, i;
  num = arraynum(farray);
  array = arraydata(farray);
  if (num > 0) {
    axis_save_undo(UNDO_TYPE_EDIT);
  }
  for (i = 0; i < num; i++) {
    double min, max, wd;
    getobj(obj, "min", array[i], 0, NULL, &min);
    getobj(obj, "max", array[i], 0, NULL, &max);
    wd = (max - min) / 2;
    if (wd != 0) {
      char *argv[4];
      double mid, room;
      mid = (min + max) / 2;
      min = mid - wd * zoom;
      max = mid + wd * zoom;
      room = 0;
      argv[0] = (char *) &min;
      argv[1] = (char *) &max;
      argv[2] = (char *) &room;
      argv[3] = NULL;
      exeobj(obj, "scale", array[i], 3, argv);
      set_graph_modified();
    }
  }
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
}

struct axis_zoom_data
{
  double zoom;
  struct narray *farray;
  struct objlist *obj;
};

static void
axis_zoom_response_response(struct response_callback *cb)
{
  struct axis_zoom_data *data;

  data = (struct axis_zoom_data *) cb->data;
  if (cb->return_value == IDOK) {
    axis_zoom(data->obj, data->farray, data->zoom);
  }
  arrayfree(data->farray);
  g_clear_pointer(&cb->data, g_free);
}

static void
axis_zoom_response(struct response_callback *cb)
{
  struct axis_zoom_data *data;
  struct narray *farray;
  struct objlist *obj;

  obj = (struct objlist *) cb->data;
  if (cb->return_value != IDOK || DlgZoom.zoom <= 0) {
    return;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }

  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    g_free(data);
    return;
  }

  data->zoom = DlgZoom.zoom / 10000.0;;
  data->farray = farray;
  data->obj = obj;
  SelectDialog(&DlgSelect, obj, _("scale zoom (multi select)"), AxisCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, axis_zoom_response_response, NULL, data);
  DialogExecute(TopLevel, &DlgSelect);
}

void
CmAxisZoom(void *w, gpointer client_data)
{
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  ZoomDialog(&DlgZoom);
  response_callback_add(&DlgZoom, axis_zoom_response, NULL, obj);
  DialogExecute(TopLevel, &DlgZoom);
}

static void
axiswin_scale_clear(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  struct obj_list_data *d;
  struct objlist *obj;
  int sel, num;

  if (Menulock || Globallock)
    return;

  obj = chkobject("axis");
  if (obj == NULL)
    return;

  d = (struct obj_list_data *) user_data;

  sel = columnview_get_active (d->text);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    axis_save_undo(UNDO_TYPE_CLEAR_SCALE);
    d->setup_dialog(d, sel, -1);
    d->select = sel;
    axis_scale_push(obj, sel);
    exeobj(obj, "clear", sel, 0, NULL);
    set_graph_modified();
    d->update(d, FALSE, TRUE);
  }
}

static void
axis_clear_response(struct response_callback *cb)
{
  struct narray *farray;
  struct SelectDialog *d;
  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  if (cb->return_value == IDOK) {
    int *array, num, i;
    num = arraynum(farray);
    array = arraydata(farray);
    if (num > 0) {
      axis_save_undo(UNDO_TYPE_CLEAR_SCALE);
    }
    for (i = 0; i < num; i++) {
      axis_scale_push(d->Obj, array[i]);
      exeobj(d->Obj, "clear", array[i], 0, NULL);
      set_graph_modified();
    }
    AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
  }
  arrayfree(farray);
}

void
CmAxisClear(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axis")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  if (farray == NULL)
    return;
  SelectDialog(&DlgSelect, obj, _("scale clear (multi select)"), AxisCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, axis_clear_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

void
update_viewer_axisgrid(void)
{
  char const *objects[2];
  objects[0] = "axisgrid";
  objects[1] = NULL;
  ViewerWinUpdate(objects);
}

static void
axis_grid_new_response(struct response_callback *cb)
{
  int undo;
  undo = GPOINTER_TO_INT(cb->data);
  if (cb->return_value == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    update_viewer_axisgrid();
  }
}

void
CmAxisGridNew(void *w, gpointer client_data)
{
  struct objlist *obj;
  int id, undo;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  undo = menu_save_undo_single(UNDO_TYPE_CREATE, "axisgrid");
  id = newobj(obj);
  if (id < 0) {
    menu_delete_undo(undo);
    return;
  }
  GridDialog(&DlgGrid, obj, id);
  response_callback_add(&DlgGrid, axis_grid_new_response, NULL, GINT_TO_POINTER(undo));
  DialogExecute(TopLevel, &DlgGrid);
}

static void
axis_grid_del_response(struct response_callback *cb)
{
  struct narray *farray;
  struct SelectDialog *d;
  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  if (cb->return_value == IDOK) {
    int i, num, *array;
    num = arraynum(farray);
    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_DELETE, "axisgrid");
    }
    array = arraydata(farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(d->Obj, array[i]);
      set_graph_modified();
    }
    update_viewer_axisgrid();
  }
  arrayfree(farray);
}

void
CmAxisGridDel(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  if (farray == NULL)
    return;
  SelectDialog(&DlgSelect, obj, _("delete grid (multi select)"), GridCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, axis_grid_del_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

struct axis_grid_update_data {
  int i, num;
  struct narray *farray;
};

static void
axis_grid_update_update_response(struct response_callback *cb)
{
  struct axis_grid_update_data *data;
  struct GridDialog *d;
  struct objlist *obj;
  int *array;
  data = cb->data;
  array = arraydata(data->farray);
  d = (struct GridDialog *) cb->dialog;
  obj = d->Obj;
  if (cb->return_value == IDDELETE) {
    int j;
    delobj(obj, array[data->i]);
    set_graph_modified();
    for (j = data->i + 1; j < data->num; j++) {
      array[j]--;
    }
  }
  data->i++;
  if (data->i >= data->num) {
    update_viewer_axisgrid();
    arrayfree(data->farray);
    g_clear_pointer(&cb->data, g_free);
  } else {
    GridDialog(&DlgGrid, obj, array[data->i]);
    response_callback_add(&DlgGrid, axis_grid_update_update_response, NULL, data);
    DialogExecute(TopLevel, &DlgGrid);
  }
}

static void
axis_grid_update_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  struct objlist *obj;
  struct narray *farray;

  d = (struct SelectDialog *) cb->dialog;
  obj = d->Obj;
  farray = d->sel;
  if (cb->return_value == IDOK) {
    int *array, num;
    struct axis_grid_update_data *data;
    num = arraynum(farray);
    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_EDIT, "axisgrid");
    }
    array = arraydata(farray);
    data = g_malloc0(sizeof(*data));
    data->num = num;
    data->i = 0;
    data->farray = farray;
    GridDialog(&DlgGrid, obj, array[0]);
    response_callback_add(&DlgGrid, axis_grid_update_update_response, NULL, data);
    DialogExecute(TopLevel, &DlgGrid);
  }
}

void
CmAxisGridUpdate(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("axisgrid")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  if (farray == NULL)
    return;
  SelectDialog(&DlgSelect, obj, _("grid property (multi select)"), GridCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, axis_grid_update_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

void
AxisWinUpdate(struct obj_list_data *d, int clear, int draw)
{
  if (Menulock || Globallock)
    return;

  if (d == NULL)
    return;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d);
  } else {
    list_sub_window_set(d);
  }

  if (! clear && d->select >= 0) {
    columnview_set_active(d->text, d->select, TRUE);
  }
  if (draw) {
    char const *objects[4];
//    NgraphApp.Viewer.allclear = TRUE;
    objects[0] = d->obj->name;
    objects[1] = "axisgrid";
    objects[2] = (draw == DRAW_AXIS_ONLY) ? NULL : NgraphApp.FileWin.data.data->obj->name;
    objects[3] = NULL;
    ViewerWinUpdate(objects);
  }
}

static void
AxisDelCB(struct obj_list_data *data, int id)
{
  AxisDel(id);
}

static void
bind_minmax (struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  double min, max, val;
  char buf[256], *math;

  getobj(obj, "min", id, 0, NULL, &min);
  getobj(obj, "max", id, 0, NULL, &max);
  gtk_widget_set_halign (w, GTK_ALIGN_END);
  if ((min == 0) && (max == 0)) {
    gtk_label_set_text (GTK_LABEL (w), FILL_STRING);
    return;
  }

  val = (field[2] == 'n') ? min : max;
  getobj(obj, "num_math", id, 0, NULL, &math);
  if (math) {
    snprintf(buf, sizeof(buf), "<i>%g</i>", val);
    gtk_label_set_markup (GTK_LABEL (w), buf);
  } else {
    snprintf(buf, sizeof(buf), "%g", val);
    gtk_label_set_text (GTK_LABEL (w), buf);
  }
}

static void
bind_inc (struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  double inc;
  char buf[256], *math;

  getobj(obj, "inc", id, 0, NULL, &inc);
  gtk_widget_set_halign (w, GTK_ALIGN_END);
  if (inc == 0) {
    gtk_label_set_text (GTK_LABEL (w), FILL_STRING);
    return;
  }

  getobj(obj, "num_math", id, 0, NULL, &math);
  if (math) {
    snprintf(buf, sizeof(buf), "<i>%g</i>", inc);
    gtk_label_set_markup (GTK_LABEL (w), buf);
  } else {
    snprintf(buf, sizeof(buf), "%g", inc);
    gtk_label_set_text (GTK_LABEL (w), buf);
  }
}

static void
bind_name (struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  char *group, *math;

  getobj(obj, "group", id, 0, NULL, &group);
  if (group == NULL) {
    gtk_label_set_text (GTK_LABEL (w), FILL_STRING);
    return;
  }

  getobj(obj, "num_math", id, 0, NULL, &math);
  if (math) {
    label_set_italic_text (w, group);
  } else {
    gtk_label_set_text (GTK_LABEL (w), group);
  }
}

static int
check_axis_history(struct objlist *obj)
{
  struct narray *array;
  int num, n, i;

  n = chkobjlastinst(obj);
  if (n < 0)
    return 0;

  num = 0;
  for (i = 0; i <= n; i++) {
    getobj(obj, "scale_history", i, 0, NULL, &array);
    num += arraynum(array) / 3;
  }

  return num;
}

int
axis_check_history(void)
{
  struct objlist *obj;
  obj = chkobject("axis");
  if (obj == NULL) {
    return FALSE;
  }
  return check_axis_history(obj);
}

static void
axis_scale_undo(struct objlist *obj, struct narray *farray)
{
  int i, n, num, *array;
  char *argv[1];
  num = arraynum(farray);
  if (num > 0) {
    axis_save_undo(UNDO_TYPE_UNDO_SCALE);
  }
  array = arraydata(farray);
  for (i = num - 1; i >= 0; i--) {
    argv[0] = NULL;
    exeobj(obj, "scale_pop", array[i], 0, argv);
    set_graph_modified();
  }
  n = check_axis_history(obj);
  set_axis_undo_button_sensitivity(n > 0);
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, TRUE);
}

static void
axis_scale_undo_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  d = (struct SelectDialog *) cb->dialog;
  if (cb->return_value == IDOK) {
    axis_scale_undo(d->Obj, d->sel);
  }
  arrayfree(d->sel);
}

void
CmAxisScaleUndo(void *w, gpointer client_data)
{
  struct objlist *obj;
  struct narray *farray;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (check_axis_history(obj) == 0)
    return;

  farray = arraynew(sizeof(int));
  if (farray == NULL)
    return;

  SelectDialog(&DlgSelect, obj, _("scale undo (multi select)"), AxisHistoryCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, axis_scale_undo_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, num, last_id, i;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    GAction *action;
    action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), Popup_list[i].name);
    switch (i) {
    case POPUP_ITEM_FOCUS_ALL:
      last_id = chkobjlastinst(d->obj);
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), last_id >= 0);
      break;
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel >= 0 && sel < num);
      break;
    case POPUP_ITEM_ADD_F:
    case POPUP_ITEM_ADD_S:
    case POPUP_ITEM_ADD_C:
    case POPUP_ITEM_ADD_A:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), TRUE);
      break;
    default:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action),  sel >= 0 && sel <= num);
    }
  }
}

static void
axiswin_delete_axis(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  UnFocus();
  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    axis_save_undo(UNDO_TYPE_DELETE);
    AxisDel(sel);
    AxisWinUpdate(d, TRUE, DRAW_REDRAW);
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
    set_graph_modified();
    d->select = -1;
  }
}

static void
axis_delete_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  axiswin_delete_axis(d);
}

static void
AxisWinAxisTop(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int sel, num;
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  if ((sel  >=  0) && (sel <= num)) {
    axis_save_undo(UNDO_TYPE_ORDER);
    movetopobj(d->obj, sel);
    d->select = 0;
    AxisMove(sel,0);
    AxisWinUpdate(d, FALSE, FALSE);
    FileWinUpdate(NgraphApp.FileWin.data.data, FALSE, FALSE);
    set_graph_modified();
  }
}

static void
AxisWinAxisLast(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int sel, num;
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    axis_save_undo(UNDO_TYPE_ORDER);
    movelastobj(d->obj, sel);
    d->select = num;
    AxisMove(sel, num);
    AxisWinUpdate(d, FALSE, FALSE);
    FileWinUpdate(NgraphApp.FileWin.data.data, FALSE, FALSE);
    set_graph_modified();
  }
}

static void
AxisWinAxisUp(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int sel, num;
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  if ((sel >= 1) && (sel <= num)) {
    axis_save_undo(UNDO_TYPE_ORDER);
    moveupobj(d->obj, sel);
    d->select = sel - 1;
    AxisMove(sel, sel - 1);
    AxisWinUpdate(d, FALSE, FALSE);
    FileWinUpdate(NgraphApp.FileWin.data.data, FALSE, FALSE);
    set_graph_modified();
  }
}

static void
AxisWinAxisDown(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int sel, num;
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;

  if (Menulock || Globallock) return;
  UnFocus();
  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  if (sel >= 0 && sel <= num-1) {
    axis_save_undo(UNDO_TYPE_ORDER);
    movedownobj(d->obj, sel);
    d->select = sel + 1;
    AxisMove(sel, sel + 1);
    AxisWinUpdate(d, FALSE, FALSE);
    FileWinUpdate(NgraphApp.FileWin.data.data, FALSE, FALSE);
    set_graph_modified();
  }
}

static gboolean
axiswin_ev_key_down(GtkWidget *w, guint keyval, GdkModifierType state, gpointer user_data)
{
  struct obj_list_data *d;

  g_return_val_if_fail(w != NULL, FALSE);

  if (Menulock || Globallock)
    return TRUE;

  d = (struct obj_list_data *) user_data;

  switch (keyval) {
  case GDK_KEY_Delete:
    axiswin_delete_axis(d);
    break;
  case GDK_KEY_Home:
    if (state & GDK_SHIFT_MASK) {
      AxisWinAxisTop(NULL, NULL, d);
    } else {
      return FALSE;
    }
    break;
  case GDK_KEY_End:
    if (state & GDK_SHIFT_MASK) {
      AxisWinAxisLast(NULL, NULL, d);
    } else {
      return FALSE;
    }
    break;
  case GDK_KEY_Up:
    if (state & GDK_SHIFT_MASK) {
      AxisWinAxisUp(NULL, NULL, d);
    } else {
      return FALSE;
    }
    break;
  case GDK_KEY_Down:
    if (state & GDK_SHIFT_MASK) {
      AxisWinAxisDown(NULL, NULL, d);
    } else {
      return FALSE;
    }
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

GtkWidget *
create_axis_list(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  list_sub_window_create(d, AXIS_WIN_COL_NUM, Alist);

  d->data.data->update = AxisWinUpdate;
  d->data.data->setup_dialog = AxisDialog;
  d->data.data->dialog = &DlgAxis;
  d->data.data->ev_key = axiswin_ev_key_down;
  d->data.data->delete = AxisDelCB;
  d->data.data->obj = chkobject("axis");

  sub_win_create_popup_menu(d->data.data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
  return d->Win;
}
