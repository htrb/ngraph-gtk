/*
 * $Id: x11lgnd.c,v 1.68 2010-03-04 08:30:17 hito Exp $
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

#include "dir_defs.h"
#include "strconv.h"
#include "ioutil.h"

#include "object.h"
#include "gra.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "odraw.h"
#include "opath.h"
#include "nstring.h"
#include "mathfn.h"

#include "gtk_listview.h"
#include "gtk_columnview.h"
#include "gtk_entry_completion.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11file.h"
#include "x11view.h"
#include "x11lgnd.h"
#include "x11commn.h"

#define DUMMY_MACRO_FOR_GETTEXT_CHARMAP1 N_("_Physics")
#define DUMMY_MACRO_FOR_GETTEXT_CHARMAP2 N_("_Mathematics")
#define DUMMY_MACRO_FOR_GETTEXT_CHARMAP3 N_("_Greece")

#define ARROW_VIEW_SIZE 160

static void *bind_path_type (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_path_pos (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_color (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_rect_x (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_rect_y (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_mark (GtkWidget *w, struct objlist *obj, const char *field, int id);
static void *bind_text (GtkWidget *w, struct objlist *obj, const char *field, int id);

static int setup_path_type_menu (struct objlist *obj, const char *field, int id, GtkStringList *list);
static int select_path_type_menu (struct objlist *obj, const char *field, int id, GtkStringList *list, int sel);
static int setup_font_menu (struct objlist *obj, const char *field, int id, GtkStringList *list);
static int select_font_menu (struct objlist *obj, const char *field, int id, GtkStringList *list, int sel);

static n_list_store Plist[] = {
  {" ",                G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",                G_TYPE_INT,     FALSE, FALSE, "id"},
  {"type",             G_TYPE_ENUM,    TRUE,  FALSE, "type",   bind_path_type, setup_path_type_menu, select_path_type_menu},
  {N_("marker begin"), G_TYPE_ENUM,    TRUE,  FALSE, "marker_begin"},
  {N_("marker end"),   G_TYPE_ENUM,    TRUE,  FALSE, "marker_end"},
  {N_("color"),        G_TYPE_OBJECT,  TRUE,  FALSE, "color",  bind_color},
  {"x",                G_TYPE_DOUBLE,  FALSE,  TRUE,  "x",      bind_path_pos, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",                G_TYPE_DOUBLE,  FALSE,  TRUE,  "y",      bind_path_pos, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),        G_TYPE_DOUBLE,  TRUE,  TRUE,  "width",  NULL, NULL, NULL,            0, SPIN_ENTRY_MAX,  20,  100},
  {N_("points"),       G_TYPE_INT,     FALSE, FALSE, "points", bind_path_pos},
  {"^#",               G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define PATH_LIST_COL_NUM G_N_ELEMENTS (Plist)


static n_list_store Rlist[] = {
  {" ",              G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",              G_TYPE_INT,     FALSE, FALSE, "id"},
  {N_("color"),      G_TYPE_OBJECT,  TRUE,  FALSE, "color", bind_color},
  {"x",              G_TYPE_DOUBLE,  TRUE,  TRUE,  "x1",    bind_rect_x, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",              G_TYPE_DOUBLE,  TRUE,  TRUE,  "y1",    bind_rect_y, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"width",          G_TYPE_DOUBLE,  TRUE,  TRUE,  "x2",    bind_rect_x, NULL, NULL,                0, SPIN_ENTRY_MAX,  20,  100},
  {N_("height"),     G_TYPE_DOUBLE,  TRUE,  TRUE,  "y2",    bind_rect_y, NULL, NULL,                0, SPIN_ENTRY_MAX,  20,  100},
  {N_("line width"), G_TYPE_DOUBLE,  TRUE,  TRUE,  "width", NULL, NULL, NULL,                       0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",             G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define  RECT_LIST_COL_NUM G_N_ELEMENTS (Rlist)

static n_list_store Alist[] = {
  {" ",            G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",            G_TYPE_INT,     FALSE, FALSE, "id"},
  {"color",        G_TYPE_OBJECT,  TRUE,  FALSE, "color",  bind_color},
  {"x",            G_TYPE_DOUBLE,  TRUE,  TRUE,  "x",      NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE,  TRUE,  "y",      NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"rx",           G_TYPE_DOUBLE,  TRUE,  TRUE,  "rx",     NULL, NULL, NULL, 0, SPIN_ENTRY_MAX, 100, 1000},
  {"ry",           G_TYPE_DOUBLE,  TRUE,  TRUE,  "ry",     NULL, NULL, NULL, 0, SPIN_ENTRY_MAX, 100, 1000},
  {N_("angle1"),   G_TYPE_DOUBLE,  TRUE,  TRUE,  "angle1", NULL, NULL, NULL, 0, 36000, 100, 1500},
  {N_("angle2"),   G_TYPE_DOUBLE,  TRUE,  TRUE,  "angle2", NULL, NULL, NULL, 0, 36000, 100, 1500},
  {N_("pieslice"), G_TYPE_BOOLEAN, TRUE,  FALSE, "pieslice"},
  {N_("width"),    G_TYPE_DOUBLE,  TRUE,  TRUE,  "width",  NULL, NULL, NULL, 0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",           G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define ARC_LIST_COL_NUM G_N_ELEMENTS (Alist)

static n_list_store Mlist[] = {
  {" ",            G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",            G_TYPE_INT,     FALSE, FALSE, "id"},
  {"mark",         G_TYPE_OBJECT,  TRUE,  FALSE, "type", bind_mark},
  {"x",            G_TYPE_DOUBLE,  TRUE,  TRUE,  "x", NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE,  TRUE,  "y", NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("size"),     G_TYPE_DOUBLE,  TRUE,  TRUE,  "size", NULL, NULL, NULL,             0, SPIN_ENTRY_MAX, 100,  200},
  {"width",        G_TYPE_DOUBLE,  TRUE,  TRUE,  "width", NULL, NULL, NULL,            0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",           G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define MARK_LIST_COL_NUM G_N_ELEMENTS (Mlist)

static n_list_store Tlist[] = {
  {" ",             G_TYPE_BOOLEAN, TRUE,  FALSE, "hidden"},
  {"#",             G_TYPE_INT,     FALSE, FALSE, "id"},
  {"text",          G_TYPE_STRING,  TRUE,  TRUE,  "text", bind_text, NULL, NULL, 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
  {N_("font"),      G_TYPE_ENUM,    TRUE,  FALSE, "font", NULL, setup_font_menu, select_font_menu},
  {"x",             G_TYPE_DOUBLE,  TRUE,  FALSE, "x", NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",             G_TYPE_DOUBLE,  TRUE,  FALSE, "y", NULL, NULL, NULL, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("pt"),        G_TYPE_DOUBLE,  TRUE,  FALSE, "pt", NULL, NULL, NULL,               0, SPIN_ENTRY_MAX, 100, 1000},
  {N_("direction"), G_TYPE_DOUBLE,  TRUE,  FALSE, "direction", NULL, NULL, NULL,        0, 36000,          100, 1500},
  {"raw",           G_TYPE_BOOLEAN, TRUE,  FALSE, "raw"},
  {"^#",            G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define TEXT_LIST_COL_NUM G_N_ELEMENTS (Tlist)

static GActionEntry PopupAction[] =
{
  {"%sFocusAllAction",     list_sub_window_focus_all, NULL, NULL, NULL},
  {"%sOrderTopAction",     list_sub_window_move_top, NULL, NULL, NULL},
  {"%sOrderUpAction",      list_sub_window_move_up, NULL, NULL, NULL},
  {"%sOrderDownAction",    list_sub_window_move_down, NULL, NULL, NULL},
  {"%sOrderBottomAction",  list_sub_window_move_last, NULL, NULL, NULL},

  {"%sDuplicateAction",    list_sub_window_copy, NULL, NULL, NULL},
  {"%sDeleteAction",       list_sub_window_delete, NULL, NULL, NULL},
  {"%sFocusAction",        list_sub_window_focus, NULL, NULL, NULL},
  {"%sUpdateAction",       list_sub_window_update, NULL, NULL, NULL},
  {"%sInstanceNameAction", list_sub_window_object_name, NULL, NULL, NULL},
};

#define POPUP_ITEM_NUM ((int) (sizeof(PopupAction) / sizeof(*PopupAction)))
#define POPUP_ITEM_FOCUS_ALL 0
#define POPUP_ITEM_TOP       1
#define POPUP_ITEM_UP        2
#define POPUP_ITEM_DOWN      3
#define POPUP_ITEM_BOTTOM    4

typedef void (* LEGEND_DIALOG_SETUP)(struct LegendDialog *data, struct objlist *obj, int id);


enum LegendType {
  LegendTypePath = 0,
  LegendTypeRect,
  LegendTypeArc,
  LegendTypeMark,
  LegendTypeText,
};

static char *MarkChar[] = {
  "●", "○", "○", "◎", "⦿", "🞈", "◑", "◐", "◓", "◒",
  "■", "⬜", "⬜", "⧈", "▣", "🞑", "◨", "◧", "⬒", "⬓",
  "◆", "◇", "◇", "🞜", "◈", "",  "⬗", "⬖", "⬘", "⬙",
  "▲", "△", "△", "⟁", "",   "",  "◮", "◭", "⧗",  "⧖",
  "▼", "▽", "▽", "",   "",   "",  "⧩", "⧨", "",   "",
  "◀", "◁", "◁", "",   "",   "",  "",   "",   "⧓",  "⋈",
  "▶", "▷", "▷", "",   "",   "",  "",   "",   "⧒",  "⧑",
  "＋", "×",  "∗",  "⚹",  "",   "",  "",   "",   "―", "|",
  "◎", "⨁", "⨂", "⧈", "⊞", "⊠", "🞜", "",   "",   "·",
};

#define MarkCharNum ((int) (sizeof(MarkChar) / sizeof(*MarkChar)))

struct lwidget {
  GtkWidget *w;
  char *f;
};

struct legend_menu_update_data
{
  LEGEND_DIALOG_SETUP setup;
  struct narray *array;
  struct objlist *obj;
  struct LegendDialog *dialog;
  int i;
};

static  void
legend_menu_update_object_free(struct response_callback * cb)
{
  struct legend_menu_update_data *data;
  data = (struct legend_menu_update_data *) cb->data;
  if (data->array) {
    arrayfree(data->array);
  }
  g_free(data);
}

static void
legend_menu_update_object_response2(struct response_callback *cb)
{
  struct legend_menu_update_data *ldata,*ldata2;
  struct narray *array;
  struct objlist *obj;
  LEGEND_DIALOG_SETUP setup;
  struct LegendDialog *dialog;
  int i, num, *data;

  ldata = (struct legend_menu_update_data *) cb->data;
  obj = ldata->obj;
  array = ldata->array;
  setup = ldata->setup;
  dialog = ldata->dialog;
  i = ldata->i;

  num = arraynum(array);
  if (i >= num -1) {
    char *objs[2];
    objs[0] = obj->name;
    objs[1] = NULL;
    LegendWinUpdate(objs, TRUE, TRUE);
    return;
  }

  data = arraydata(array);
  setup(dialog, obj, data[i]);
  if (cb->return_value == IDDELETE) {
    int j;
    delobj(obj, data[i]);
    set_graph_modified();
    for (j = i + 1; j < num; j++) {
      data[j]--;
    }
  }
  ldata2 = g_memdup2(ldata, sizeof(*ldata));
  if (ldata2 == NULL) {
    return;
  }
  ldata->array = NULL;
  ldata2->i += 1;
  response_callback_add(dialog, legend_menu_update_object_response2, legend_menu_update_object_free, ldata2);
  DialogExecute(TopLevel, dialog);
}

static void
legend_menu_update_object_response(struct response_callback *cb)
{
  struct legend_menu_update_data *ldata;
  struct narray *array;
  struct objlist *obj;
  LEGEND_DIALOG_SETUP setup;
  struct LegendDialog *dialog;
  int i;

  ldata = (struct legend_menu_update_data *) cb->data;
  obj = ldata->obj;
  array = ldata->array;
  setup = ldata->setup;
  dialog = ldata->dialog;
  i = ldata->i;
  if (cb->return_value == IDOK) {
    int num;

    num = arraynum(array);
    if (num > 0) {
      struct legend_menu_update_data *ldata2;
      int *data;
      menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
      data = arraydata(array);
      ldata2 = g_memdup2(ldata, sizeof(*ldata));
      if (ldata2 == NULL) {
        return;
      }
      ldata->array = NULL;
      ldata2->i = i + 1;
      setup(dialog, obj, data[i]);
      response_callback_add(dialog, legend_menu_update_object_response2, legend_menu_update_object_free, ldata2);
      DialogExecute(TopLevel, dialog);
    }
  }
}

static struct legend_menu_update_data *
legend_menu_update_data_new(struct objlist *obj, struct LegendDialog *dialog, LEGEND_DIALOG_SETUP setup)
{
  struct legend_menu_update_data *data;
  struct narray *array;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  array = arraynew(sizeof(int));
  data->dialog = dialog;
  data->array = array;
  data->obj = obj;
  data->setup = setup;
  data->i = 0;
  return data;
}

static void
legend_menu_update_object(const char *name, char *(*callback) (struct objlist * obj, int id), struct LegendDialog *dialog, LEGEND_DIALOG_SETUP setup)
{
  struct objlist *obj;
  char title[256];
  struct legend_menu_update_data *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject(name)) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;

  data = legend_menu_update_data_new(obj, dialog, setup);
  if (data == NULL) {
    return;
  }
  snprintf(title, sizeof(title), _("%s property (multi select)"), _(obj->name));
  response_callback_add(&DlgSelect, legend_menu_update_object_response, legend_menu_update_object_free, data);
  SelectDialog(&DlgSelect, obj, title, callback, data->array, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

static void
legend_menu_delete_object_response(struct response_callback *cb)
{
  struct legend_menu_update_data *ldata;
  struct narray *array;
  struct objlist *obj;

  ldata = (struct legend_menu_update_data *) cb->data;
  obj = ldata->obj;
  array = ldata->array;
  if (cb->return_value == IDOK) {
    int num;
    num = arraynum(array);
    if (num > 0) {
      int i, *data;
      char *objs[2];
      menu_save_undo_single(UNDO_TYPE_DELETE, obj->name);
      data = arraydata(array);
      for (i = num - 1; i >= 0; i--) {
	delobj(obj, data[i]);
	set_graph_modified();
      }
      objs[0] = obj->name;
      objs[1] = NULL;
      LegendWinUpdate(objs, TRUE, TRUE);
    }
  }
}

static void
legend_menu_delete_object(const char *name, char *(*callback) (struct objlist * obj, int id))
{
  struct objlist *obj;
  char title[256];
  struct legend_menu_update_data *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject(name)) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  snprintf(title, sizeof(title), _("delete %s (multi select)"), _(obj->name));

  data = legend_menu_update_data_new(obj, NULL, NULL);
  if (data == NULL) {
    return;
  }
  response_callback_add(&DlgSelect, legend_menu_delete_object_response, legend_menu_update_object_free, data);
  SelectDialog(&DlgSelect, obj, title, callback, data->array, NULL);
  DialogExecute(TopLevel, &DlgSelect);
}

static char *
LegendLineCB(struct objlist *obj, int id)
{
  struct narray *array;
  int num, *data, path_type;
  char *s, **enum_path_type;

  getobj(obj, "type", id, 0, NULL, &path_type);
  enum_path_type = (char **) chkobjarglist(obj, "type");
  getobj(obj, "points", id, 0, NULL, &array);
  num = arraynum(array);
  data = arraydata(array);

  if (num < 2) {
    s = g_strdup(FILL_STRING);
  } else {
    s = g_strdup_printf("%s (X:%.2f Y:%.2f)-",
			_(enum_path_type[path_type]),
			data[0] / 100.0,
			data[1] / 100.0);
  }

  return s;
}

static char *
LegendRectCB(struct objlist *obj, int id)
{
  int x1, y1;
  char *s;

  getobj(obj, "x1", id, 0, NULL, &x1);
  getobj(obj, "y1", id, 0, NULL, &y1);
  s = g_strdup_printf("X1:%.2f Y1:%.2f", x1 / 100.0, y1 / 100.0);
  return s;
}

static char *
LegendArcCB(struct objlist *obj, int id)
{
  int x1, y1;
  char *s;

  getobj(obj, "x", id, 0, NULL, &x1);
  getobj(obj, "y", id, 0, NULL, &y1);
  s = g_strdup_printf("X:%.2f Y:%.2f", x1 / 100.0, y1 / 100.0);
  return s;
}

static char *
LegendMarkCB(struct objlist *obj, int id)
{
  int x, y, type;
  char *s;

  getobj(obj, "x", id, 0, NULL, &x);
  getobj(obj, "y", id, 0, NULL, &y);
  getobj(obj, "type", id, 0, NULL, &type);
  if (type >= 0 && type < MarkCharNum) {
    char *mc = MarkChar[type];
    s = g_strdup_printf(_("X:%.2f Y:%.2f %s%stype:%-2d"),
			x / 100.0, y / 100.0,
			mc,
			(mc[0]) ? " " : "",
			type);
  } else {
    s = g_strdup_printf("X:%.2f Y:%.2f", x / 100.0, y / 100.0);
  }
  return s;
}

static char *
LegendTextCB(struct objlist *obj, int id)
{
  char *text, *s;

  getobj(obj, "text", id, 0, NULL, &text);

  if (text) {
    s = g_strdup(text);
  } else {
    s = g_strdup("");
  }
  return s;
}

static void
init_legend_dialog_widget_member(struct LegendDialog *d)
{
  d->path_type = NULL;
  d->stroke = NULL;
  d->close_path = NULL;
  d->style = NULL;
  d->points = NULL;
  d->interpolation = NULL;
  d->width = NULL;
  d->miter = NULL;
  d->join = NULL;
  d->color = NULL;
  d->color2 = NULL;
  d->stroke_color = NULL;
  d->fill_color = NULL;
  d->x = NULL;
  d->y = NULL;
  d->x1 = NULL;
  d->y1 = NULL;
  d->x2 = NULL;
  d->y2 = NULL;
  d->rx = NULL;
  d->ry = NULL;
  d->pieslice = NULL;
  d->size = NULL;
  d->type = NULL;
  d->mark_type_begin = NULL;
  d->mark_type_end = NULL;
  d->angle1 = NULL;
  d->angle2 = NULL;
  d->fill = NULL;
  d->fill_rule = NULL;
  d->marker_begin = NULL;
  d->marker_end = NULL;
  d->arrow_length = NULL;
  d->arrow_width = NULL;
  d->view = NULL;
  d->text = NULL;
  d->pt = NULL;
  d->space = NULL;
  d->script_size = NULL;
  d->direction = NULL;
  d->raw = NULL;
  d->font = NULL;
  d->font_bold = NULL;
  d->font_italic = NULL;
}

static void
get_font(struct LegendDialog *d)
{
  int style, bold, italic, old_style;

  SetObjFieldFromFontList(d->font, d->Obj, d->Id, "font");

  style = 0;
  bold = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->font_bold));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }

  italic = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->font_italic));
  if (italic) {
    style |= GRA_FONT_STYLE_ITALIC;
  }

  getobj(d->Obj, "style", d->Id, 0, NULL, &old_style);
  if (old_style != style) {
    putobj(d->Obj, "style", d->Id, &style);
    set_graph_modified();
  }
}

static void
set_font(struct LegendDialog *d, int id)
{
  int style;
  struct compatible_font_info *compatible;

  compatible = SetFontListFromObj(d->font, d->Obj, d->Id, "font");

  if (compatible) {
    /* for backward compatibility */
    style = compatible->style;
    if (d->text && compatible->symbol) {
      char buf[] = "%F{Sym}";
      const char *str;

      str = gtk_editable_get_text(GTK_EDITABLE(d->text));
      if (str && strncmp(str, buf, sizeof(buf) - 1)) {
        char *tmp;
	tmp = g_strdup_printf("%s%s", buf, str);
	editable_set_init_text(d->text, tmp);
	g_free(tmp);
      }
    }
  } else {
    getobj(d->Obj, "style", d->Id, 0, NULL, &style);
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->font_bold), style & GRA_FONT_STYLE_BOLD);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->font_italic), style & GRA_FONT_STYLE_ITALIC);
}

static void
legend_dialog_set_sensitive(struct LegendDialog *d)
{
  int path_type;

  path_type = PATH_TYPE_LINE;
  if (d->path_type && d->interpolation) {
    path_type = combo_box_get_active(d->path_type);

    set_widget_sensitivity_with_label(d->interpolation, path_type == PATH_TYPE_CURVE);
  }

  if (d->stroke && d->stroke_color && d->style && d->width) {
    int stroke;

    stroke = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->stroke));
    set_widget_sensitivity_with_label(d->stroke_color, stroke);
    set_widget_sensitivity_with_label(d->style, stroke);
    set_widget_sensitivity_with_label(d->width, stroke);
  }

  if (d->stroke &&
      d->miter &&
      d->join &&
      d->close_path) {
    int stroke;

    stroke = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->stroke));
    set_widget_sensitivity_with_label(d->miter, stroke);
    set_widget_sensitivity_with_label(d->join, stroke);
    set_widget_sensitivity_with_label(d->close_path, stroke);
  }

  if (d->stroke &&
      d->close_path &&
      d->marker_begin &&
      d->marker_end &&
      d->arrow_length &&
      d->arrow_width) {
    int stroke, marker_type;

    stroke = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->stroke));
    set_widget_sensitivity_with_label(d->miter, stroke);
    set_widget_sensitivity_with_label(d->join, stroke);
    set_widget_sensitivity_with_label(d->marker_begin, stroke);
    gtk_widget_set_sensitive(d->marker_end, stroke);
    set_widget_sensitivity_with_label(d->arrow_length, stroke);
    set_widget_sensitivity_with_label(d->arrow_width, stroke);

    marker_type = combo_box_get_active(d->marker_begin);
    gtk_widget_set_sensitive(d->mark_type_begin, marker_type == MARKER_TYPE_MARK && stroke);

    marker_type = combo_box_get_active(d->marker_end);
    gtk_widget_set_sensitive(d->mark_type_end, marker_type == MARKER_TYPE_MARK && stroke);

    if (d->interpolation) {
      int intp;
      intp = combo_box_get_active(d->interpolation);
      if (path_type == PATH_TYPE_CURVE) {
	set_widget_sensitivity_with_label(d->close_path, stroke &&
					  (intp != INTERPOLATION_TYPE_SPLINE_CLOSE &&
					   intp != INTERPOLATION_TYPE_BSPLINE_CLOSE));
      }
    }
    if (path_type != PATH_TYPE_CURVE) {
      set_widget_sensitivity_with_label(d->close_path, stroke);
    }
  }

  if (d->fill && d->fill_color) {
    int fill;

    fill = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->fill));
    if (d->marker_begin && d->marker_end) {
      int marker_begin, marker_end, stroke;
      stroke = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->stroke));
      marker_begin = combo_box_get_active(d->marker_begin);
      marker_end = combo_box_get_active(d->marker_end);
      set_widget_sensitivity_with_label(d->fill_color,
                                        (stroke &&
                                         (marker_begin == MARKER_TYPE_MARK ||
                                          marker_end == MARKER_TYPE_MARK)) ||
                                        fill);
    } else {
      set_widget_sensitivity_with_label(d->fill_color, fill);
    }

    if (d->fill_rule) {
      set_widget_sensitivity_with_label(d->fill_rule, fill);
    }
  }
}

static void
setup_mark_type(struct LegendDialog *d, int id, GtkWidget *type, const char *field)
{
  int a;
  getobj(d->Obj, field, id, 0, NULL, &a);
  button_set_mark_image(type, a);
}

static void
legend_dialog_setup_item(GtkWidget *w, struct LegendDialog *d, int id)
{
  unsigned int i;
  int x1, y1, x2, y2;
  struct lwidget lw[] = {
    {d->stroke, "stroke"},
    {d->path_type, "type"},
    {d->close_path, "close_path"},
    {d->width, "width"},
    {d->join, "join"},
    {d->miter, "miter_limit"},
    {d->interpolation, "interpolation"},
    {d->x, "x"},
    {d->y, "y"},
    {d->x1, "x1"},
    {d->y1, "y1"},
    {d->rx, "rx"},
    {d->ry, "ry"},
    {d->angle1, "angle1"},
    {d->angle2, "angle2"},
    {d->fill, "fill"},
    {d->fill_rule, "fill_rule"},
    {d->raw, "raw"},
    {d->pieslice, "pieslice"},
    {d->size, "size"},
    {d->pt, "pt"},
    {d->direction, "direction"},
    {d->space, "space"},
    {d->script_size, "script_size"},
  };

  SetTextFromObjPoints(d->points, d->Obj, id, "points");
  SetStyleFromObjField(d->style, d->Obj, id, "style");

  for (i = 0; i < sizeof(lw) / sizeof(*lw); i++) {
    SetWidgetFromObjField(lw[i].w, d->Obj, id, lw[i].f);
  }

  if (d->arrow_length) {
    int len, wid;

    getobj(d->Obj, "arrow_length", id, 0, NULL, &len);
    getobj(d->Obj, "arrow_width", id, 0, NULL, &wid);

    d->wid = (wid / 100) * 100;
    d->ang = atan(0.5 * d->wid / len) * 2 * 180 / MPI;

    if (d->ang < 10) {
      d->ang = 10;
    } else if (d->ang > 170) {
      d->ang = 170;
    }

    gtk_range_set_value(GTK_RANGE(d->arrow_width), d->wid / 100);
    gtk_range_set_value(GTK_RANGE(d->arrow_length), d->ang);
  }

  if (d->type) {
    setup_mark_type(d, id, d->type, "type");
  }

  if (d->marker_begin) {
    getobj(d->Obj, "marker_begin", id, 0, NULL, &i);
    combo_box_set_active(d->marker_begin, i);
  }
  if (d->marker_end) {
    getobj(d->Obj, "marker_end", id, 0, NULL, &i);
    combo_box_set_active(d->marker_end, i);
  }

  if (d->mark_type_begin) {
    setup_mark_type(d, id, d->mark_type_begin, "mark_type_begin");
  }

  if (d->mark_type_end) {
    setup_mark_type(d, id, d->mark_type_end, "mark_type_end");
  }

  if (d->x1 && d->y1 && d->x2 && d->y2) {
    getobj(d->Obj, "x1", id, 0, NULL, &x1);
    getobj(d->Obj, "y1", id, 0, NULL, &y1);
    getobj(d->Obj, "x2", id, 0, NULL, &x2);
    getobj(d->Obj, "y2", id, 0, NULL, &y2);

    spin_entry_set_val(d->x2, x2 - x1);

    spin_entry_set_val(d->y2, y2 - y1);
  }

  if (d->text) {
    char *buf;
    sgetobjfield(d->Obj,id,"text",NULL,&buf,FALSE,FALSE,FALSE);
    if (buf) {
      editable_set_init_text(d->text, buf);
      g_free(buf);
    }
  }

  if (d->font && d->font_bold && d->font_italic)
    set_font(d, id);

  if (d->color)
    set_color(d->color, d->alpha, d->Obj, id, NULL);

  if (d->color2)
    set_color2(d->color2, d->alpha2, d->Obj, id);

  if (d->stroke_color)
    set_stroke_color(d->stroke_color, d->stroke_alpha, d->Obj, id);

  if (d->fill_color)
    set_fill_color(d->fill_color, d->fill_alpha, d->Obj, id);

  legend_dialog_set_sensitive(d);
}

static int
set_mark_type(struct LegendDialog *d, const char *field, GtkWidget *mark)
{
  int oval, nval;
  getobj(d->Obj, field, d->Id, 0, NULL, &oval);
  nval = get_mark_type_from_widget (mark);
  if (oval != nval) {
    if (putobj(d->Obj, field, d->Id, &nval) == -1) {
      return 1;
    }
    set_graph_modified();
  }
  return 0;
}

static void
legend_dialog_close(GtkWidget *w, void *data)
{
  struct LegendDialog *d = (struct LegendDialog *) data;
  unsigned int i;
  int ret, x2, y2, oval;
  struct lwidget lw[] = {
    {d->stroke, "stroke"},
    {d->path_type, "type"},
    {d->close_path, "close_path"},
    {d->width, "width"},
    {d->join, "join"},
    {d->miter, "miter_limit"},
    {d->interpolation, "interpolation"},
    {d->x, "x"},
    {d->y, "y"},
    {d->x1, "x1"},
    {d->y1, "y1"},
    {d->rx, "rx"},
    {d->ry, "ry"},
    {d->angle1, "angle1"},
    {d->angle2, "angle2"},
    {d->fill, "fill"},
    {d->fill_rule, "fill_rule"},
    {d->raw, "raw"},
    {d->marker_begin, "marker_begin"},
    {d->marker_end, "marker_end"},
    {d->pieslice, "pieslice"},
    {d->size, "size"},
    {d->pt, "pt"},
    {d->direction, "direction"},
    {d->space, "space"},
    {d->script_size, "script_size"},
  };

  switch(d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  for (i = 0; i < sizeof(lw) / sizeof(*lw); i++) {
    if (SetObjFieldFromWidget(lw[i].w, d->Obj, d->Id, lw[i].f))
      return;
  }

  if (SetObjPointsFromText(d->points, d->Obj, d->Id, "points"))
    return;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "style"))
    return;

  if (d->x1 && d->y1 && d->x2 && d->y2) {
    int x1, y1;
    x1 = spin_entry_get_val(d->x1);
    x2 = spin_entry_get_val(d->x2);
    y1 = spin_entry_get_val(d->y1);
    y2 = spin_entry_get_val(d->y2);

    x2 += x1;
    y2 += y1;

    getobj(d->Obj, "x2", d->Id, 0, NULL, &oval);
    if (oval != x2 && putobj(d->Obj, "x2", d->Id, &x2) == -1)
      return;

    getobj(d->Obj, "y2", d->Id, 0, NULL, &oval);
    if (oval != y2 && putobj(d->Obj, "y2", d->Id, &y2) == -1)
      return;
  }

  if (d->arrow_length) {
    int wid, ang, len;

    wid = gtk_range_get_value(GTK_RANGE(d->arrow_width));
    ang = gtk_range_get_value(GTK_RANGE(d->arrow_length));

    d->wid = wid * 100;
    d->ang = ang;

    len = d->wid * 0.5 / tan(d->ang * 0.5 * MPI / 180);

    getobj(d->Obj, "arrow_length", d->Id, 0, NULL, &oval);
    if (oval != len) {
      if (putobj(d->Obj, "arrow_length", d->Id, &len) == -1) {
	return;
      }
      set_graph_modified();
    }

    getobj(d->Obj, "arrow_width", d->Id, 0, NULL, &oval);
    if (oval != d->wid) {
      if(putobj(d->Obj, "arrow_width", d->Id, &(d->wid)) == -1) {
	return;
      }
      set_graph_modified();
    }
  }

  if (d->type) {
    set_mark_type(d, "type", d->type);
  }

  if (d->mark_type_begin) {
    set_mark_type(d, "mark_type_begin", d->mark_type_begin);
  }

  if (d->mark_type_end) {
    set_mark_type(d, "mark_type_end", d->mark_type_end);
  }

  if (d->font && d->font_bold && d->font_italic) {
    get_font(d);
  }

  if (d->text) {
    const char *str;
    char *ptr;

    str = gtk_editable_get_text(GTK_EDITABLE(d->text));
    if (str == NULL)
      return;

    entry_completion_append(NgraphApp.legend_text_list, str);

    ptr = g_strdup(str);

    if (ptr) {
      char *org_str;
      sgetobjfield(d->Obj, d->Id, "text", NULL, &org_str, FALSE, FALSE, FALSE);
      if (org_str == NULL || strcmp(ptr, org_str)) {
	if (sputobjfield(d->Obj, d->Id, "text", ptr) != 0) {
	  g_free(org_str);
	  g_free(ptr);
	  return;
	}
	set_graph_modified();
      }
      g_free(org_str);
      g_free(ptr);
    }
  }

  if (d->stroke_color && d->stroke_alpha && putobj_stroke_color(d->stroke_color, d->stroke_alpha, d->Obj, d->Id))
    return;

  if (d->fill_color && d->fill_alpha && putobj_fill_color(d->fill_color, d->fill_alpha, d->Obj, d->Id))
    return;

  if (d->color && d->alpha && putobj_color(d->color, d->alpha, d->Obj, d->Id, NULL))
    return;

  if (d->color2 && d->alpha2 && putobj_color2(d->color2, d->alpha2, d->Obj, d->Id))
    return;

  d->ret = ret;
}

static void
legend_copy_clicked_response(int sel, gpointer user_data)
{
  struct LegendDialog *d;
  d = (struct LegendDialog *) user_data;
  if (sel != -1) {
    legend_dialog_setup_item(d->widget, d, sel);
  }
}

static void
legend_copy_clicked(struct LegendDialog *d)
{
  CopyClick(d->widget, d->Obj, d->Id, d->prop_cb, legend_copy_clicked_response, d);
}

static void
width_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  d->width = w;
  add_widget_to_table(table, w, _("_Line width:"), FALSE, i++);
}

#define POINTS_DIMENSION 2

static void
insert_column(GtkWidget *columnview)
{
  GListStore *list;
  GtkSelectionModel *sel;
  int i, n;

  sel = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  list = columnview_get_list (columnview);

  n = g_list_model_get_n_items (G_LIST_MODEL (sel));
  for (i = 0; i < n; i++) {
    if (gtk_selection_model_is_selected (sel, i)) {
      NPoint *item;
      item = n_point_new (0, 0);
      g_list_store_insert (list, i, item);
      g_object_unref (item);
      return;
    }
  }
  list_store_append_n_point(list, 0, 0);
}

static void
point_changed (GtkEditableLabel *label, GParamSpec *spec, gpointer user_data)
{
  GtkListItem *item;
  const char *col, *text;
  NPoint *point;
  int val;
  gboolean editing;
  GValue value = G_VALUE_INIT;

  editing = gtk_editable_label_get_editing (label);
  if (editing) {
    return;
  }

  item = GTK_LIST_ITEM (user_data);
  text = gtk_editable_get_text (GTK_EDITABLE (label));
  col = gtk_list_item_get_accessible_label (item);
  if (text == NULL) {
    return;
  }
  val = strtod (text, NULL) * 100;
  point = gtk_list_item_get_item (item);
  g_value_init(&value, G_TYPE_INT);
  g_value_set_int(&value, val);
  g_object_set_property (G_OBJECT (point), col, &value);
  g_value_unset (&value);
}

static gboolean
transform_text (GBinding* binding, const GValue* from_value, GValue* to_value, gpointer user_data)
{
  char buf[64];
  int val;

  val = g_value_get_int (from_value);
  snprintf (buf, sizeof (buf), "%.2f", val / 100.0);
  g_value_set_string (to_value, buf);
  return TRUE;
}

static void
setup_point_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, const char *col)
{
  GtkWidget *label = gtk_editable_label_new ("");
  gtk_editable_set_alignment (GTK_EDITABLE (label), 1.0);
  gtk_list_item_set_accessible_label (list_item, col);
  gtk_list_item_set_child (list_item, label);
  g_signal_connect (label, "notify::editing", G_CALLBACK (point_changed), list_item);
}

static void
bind_point_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer col)
{
  GtkWidget *label = gtk_list_item_get_child (list_item);
  NPoint *item = N_POINT(gtk_list_item_get_item (list_item));
  g_object_bind_property_full (G_OBJECT (item), col, label, "text", G_BINDING_SYNC_CREATE, transform_text, NULL, col, NULL);
}

static void
remove_selected_points(GtkWidget *columnview)
{
  GListStore *list;
  int n;

  list = columnview_get_list (columnview);
  n = columnview_get_active (columnview);
  if (n >= 0) {
    g_list_store_remove (list, n);
  }
}

static void
point_row_up (GtkWidget *columnview)
{
  GListStore *list;
  NPoint *item;
  int i;
  list = columnview_get_list (columnview);
  i = columnview_get_active (columnview);
  if (i < 1) {
    return;
  }
  item = g_list_model_get_item (G_LIST_MODEL (list), i);
  g_list_store_remove (list, i);
  g_list_store_insert (list, i - 1, item);
  columnview_set_active (columnview, i - 1, TRUE);
  g_object_unref (item);
}

static void
point_row_down (GtkWidget *columnview)
{
  GListStore *list;
  NPoint *item;
  int i, n;
  list = columnview_get_list (columnview);
  n = g_list_model_get_n_items (G_LIST_MODEL (list));
  i = columnview_get_active (columnview);
  if (i < 0 || i >= n - 1) {
    return;
  }
  item = g_list_model_get_item (G_LIST_MODEL (list), i);
  g_list_store_remove (list, i);
  g_list_store_insert (list, i + 1, item);
  columnview_set_active (columnview, i + 1, TRUE);
  g_object_unref (item);
}

static GtkWidget *
points_setup(struct LegendDialog *d)
{
  GtkWidget *label, *swin, *vbox, *hbox, *columnview, *btn;
  char *title[] = {"x", "y"};
  int i;

  columnview = columnview_create (N_TYPE_POINT, N_SELECTION_TYPE_SINGLE);
  for (i = 0; i < POINTS_DIMENSION; i++) {
    columnview_create_column(columnview, title[i], G_CALLBACK (setup_point_column), G_CALLBACK (bind_point_column), NULL, title[i], TRUE);
  }

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  label = gtk_label_new_with_mnemonic(_("_Points:"));
  set_widget_margin(label, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_TOP | WIDGET_MARGIN_BOTTOM);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), columnview);
  gtk_box_append(GTK_BOX(vbox), label);

  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(swin), TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  set_widget_margin(swin, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), columnview);
  gtk_box_append(GTK_BOX(vbox), swin);

  hbox = gtk_grid_new();

  btn = gtk_button_new_with_mnemonic(_("_Add"));
  g_signal_connect_swapped(btn, "clicked", G_CALLBACK(insert_column), columnview);
  gtk_grid_attach(GTK_GRID(hbox), btn, 0, 0, 1, 1);

  btn = gtk_button_new_with_mnemonic(_("_Delete"));
  g_signal_connect_swapped(btn, "clicked", G_CALLBACK(remove_selected_points), columnview);
  gtk_grid_attach(GTK_GRID(hbox), btn, 1, 0, 1, 1);
  set_widget_margin(hbox, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);


  btn = gtk_button_new_with_mnemonic(_("_Up"));
  g_signal_connect_swapped(btn, "clicked", G_CALLBACK(point_row_up), columnview);
  gtk_grid_attach(GTK_GRID(hbox), btn, 0, 1, 1, 1);

  btn = gtk_button_new_with_mnemonic(_("_Down"));
  g_signal_connect_swapped(btn, "clicked", G_CALLBACK(point_row_down), columnview);
  gtk_grid_attach(GTK_GRID(hbox), btn, 1, 1, 1, 1);

  gtk_box_append(GTK_BOX(vbox), hbox);

  d->points = columnview;

  return vbox;
}

static void
style_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = combo_box_entry_create();
  combo_box_entry_set_width(w, NUM_ENTRY_WIDTH);
  d->style = w;
  add_widget_to_table(table, w, _("Line _Style:"), TRUE, i);
}

static void
miter_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  d->miter = w;
  add_widget_to_table(table, w, _("_Miter:"), FALSE, i++);
}

static void
join_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = combo_box_create();
  d->join = w;
  add_widget_to_table(table, w, _("_Join:"), FALSE, i++);
}

static void
color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color = w;
  add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
color2_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color2 = w;
  add_widget_to_table(table, w, _("_Color2:"), FALSE, i);
}

static void
fill_color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->fill_color = w;
  add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
stroke_color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->stroke_color = w;
  add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
alpha_setup_common (const char *title, GtkWidget *table, GtkWidget **aw, int i)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_ALPHA, FALSE, TRUE);
  *aw = w;
  add_widget_to_table(table, w, _(title), FALSE, i);
}

static void
alpha_setup (struct LegendDialog *d, GtkWidget *table, int i)
{
  alpha_setup_common ("_Alpha:", table, &d->alpha, i);
}

static void
alpha2_setup (struct LegendDialog *d, GtkWidget *table, int i)
{
  alpha_setup_common ("_Alpha2:", table, &d->alpha2, i);
}

static void
stroke_alpha_setup (struct LegendDialog *d, GtkWidget *table, int i)
{
  alpha_setup_common ("_Alpha:", table, &d->stroke_alpha, i);
}

static void
fill_alpha_setup (struct LegendDialog *d, GtkWidget *table, int i)
{
  alpha_setup_common ("_Alpha:", table, &d->fill_alpha, i);
}

static void
draw_arrow_pixmap(GtkWidget *win, struct LegendDialog *d)
{
  int lw, len, x, w;
  cairo_t *cr;

  if (d->arrow_pixmap == NULL) {
    d->arrow_pixmap = cairo_image_surface_create(CAIRO_FORMAT_RGB24, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
  }

  cr = cairo_create(d->arrow_pixmap);

  cairo_set_source_rgb(cr, Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  cairo_paint(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);

  lw = ARROW_VIEW_SIZE / 20;
  len = d->wid * 0.5 / tan(d->ang * 0.5 * MPI / 180);
  x = nround(lw * (len / 10000.0));
  w = nround(lw * (d->wid / 20000.0));

  cairo_rectangle(cr,
		  x, (ARROW_VIEW_SIZE - lw) / 2,
		  ARROW_VIEW_SIZE - x, lw);
  cairo_move_to(cr, 0, ARROW_VIEW_SIZE / 2);
  cairo_line_to(cr, x, ARROW_VIEW_SIZE / 2 - w);
  cairo_line_to(cr, x, ARROW_VIEW_SIZE / 2 + w);
  cairo_fill(cr);
}

static void
LegendArrowDialogPaint(GtkWidget *w, cairo_t *cr, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;

  if (d->arrow_pixmap == NULL) {
    draw_arrow_pixmap(w, d);
  }

  if (d->arrow_pixmap == NULL) {
    return;
  }

  cairo_set_source_surface(cr, d->arrow_pixmap, 0, 0);
  cairo_paint(cr);
}

static void
LegendArrowDialogScale(GtkWidget *w, struct LegendDialog *d)
{
  draw_arrow_pixmap(w, d);
  gtk_widget_queue_draw(d->view);
}

static void
LegendArrowDialogScaleW(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;
  d->wid = gtk_range_get_value(GTK_RANGE(w)) * 100;
  LegendArrowDialogScale(w, d);
}

static void
LegendArrowDialogScaleL(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;
  d->ang = gtk_range_get_value(GTK_RANGE(w));
  LegendArrowDialogScale(w, d);
}

static gchar*
format_value_percent(GtkScale *scale, gdouble value, gpointer user_data)
{
  return g_strdup_printf ("%.0f%%", value);
}

static gchar*
format_value_degree(GtkScale *scale, gdouble value, gpointer user_data)
{
  return g_strdup_printf ("%.0f°", value);
}

static void
setup_mark_item (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *image;

  image = gtk_image_new ();
  gtk_image_set_icon_size(GTK_IMAGE(image), Menulocal.icon_size);
  gtk_list_item_set_child (list_item, image);
}

static void
bind_mark_item (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *image;
  GtkStringObject *strobj;
  const char *icon;

  image = gtk_list_item_get_child (list_item);
  strobj = gtk_list_item_get_item (list_item);
  icon = gtk_string_object_get_string (strobj);
  gtk_image_set_from_icon_name (GTK_IMAGE (image), icon);
}

static GtkWidget *
create_marker_type_combo_box(const char *postfix, const char *tooltip)
{
  GtkWidget *cbox;
  GtkStringList *list;
  GtkListItemFactory *factory;
  int j;
  cbox = combo_box_create();
  gtk_drop_down_set_show_arrow (GTK_DROP_DOWN (cbox), FALSE);
  list = GTK_STRING_LIST (gtk_drop_down_get_model (GTK_DROP_DOWN (cbox)));
  factory = gtk_signal_list_item_factory_new ();
  gtk_drop_down_set_factory (GTK_DROP_DOWN (cbox), factory);
  g_signal_connect (factory, "setup", G_CALLBACK (setup_mark_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_mark_item), NULL);
  for (j = 0; j < MARKER_TYPE_NUM; j++) {
    char img_file[256];
    snprintf(img_file, sizeof(img_file), "%s_%s-symbolic", marker_type_char[j], postfix);
    gtk_string_list_append (list, img_file);
  }
  combo_box_set_active (cbox, 1);
  gtk_widget_set_tooltip_text(cbox, tooltip);
  return cbox;
}

static void
create_maker_setting_widgets(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w, *hbox3, *label;
  hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  w = gtk_button_new();
  g_signal_connect(w, "clicked", G_CALLBACK(mark_popover_popup), NULL);
  gtk_box_append(GTK_BOX(hbox3), w);
  mark_popover_new (w, NULL);
  d->mark_type_begin = w;

  w = create_marker_type_combo_box("begin", _("Marker begin"));
  gtk_box_append(GTK_BOX(hbox3), w);
  d->marker_begin = w;

  w = create_marker_type_combo_box("end", _("Marker end"));
  gtk_box_append(GTK_BOX(hbox3), w);
  d->marker_end = w;

  w = gtk_button_new();
  gtk_box_append(GTK_BOX(hbox3), w);
  g_signal_connect(w, "clicked", G_CALLBACK(mark_popover_popup), NULL);
  mark_popover_new (w, NULL);
  d->mark_type_end = w;

  g_signal_connect_swapped(d->marker_begin, "notify::selected-item", G_CALLBACK(legend_dialog_set_sensitive), d);
  g_signal_connect_swapped(d->marker_end,   "notify::selected-item", G_CALLBACK(legend_dialog_set_sensitive), d);

  label = gtk_label_new_with_mnemonic(_("_Marker:"));
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), d->marker_begin);
  gtk_grid_attach(GTK_GRID(table), label, 0, i, 1, 1);
  gtk_grid_attach(GTK_GRID(table), hbox3, 1, i, 1, 1);
}

static void
draw_function(GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height, gpointer user_data)
{
  LegendArrowDialogPaint(GTK_WIDGET(drawing_area), cr, user_data);
}

static void
create_arrow_setting_widgets(struct LegendDialog *d, GtkWidget *hbox)
{
  GtkWidget *w, *vbox;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 170, 1);
  set_scale_mark(w, GTK_POS_BOTTOM, 15, 15);

  g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleL), d);
  gtk_scale_set_draw_value(GTK_SCALE(w), TRUE);
  gtk_scale_set_format_value_func(GTK_SCALE(w), format_value_degree, NULL, NULL);
  gtk_box_append(GTK_BOX(vbox), w);
  d->arrow_length = w;

  w = gtk_drawing_area_new();
  gtk_widget_set_size_request(w, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(w), draw_function, d, NULL);
  d->view = w;

  w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 2000, 1);
  set_scale_mark(w, GTK_POS_TOP, 200, 200);
  g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleW), d);
  gtk_scale_set_draw_value(GTK_SCALE(w), TRUE);
  gtk_scale_set_format_value_func(GTK_SCALE(w), format_value_percent, NULL, NULL);
  gtk_box_append(GTK_BOX(vbox), w);
  d->arrow_width = w;

  set_widget_margin(vbox, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);
  gtk_box_append(GTK_BOX(hbox), vbox);
}

static void
LegendArrowDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend line %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *hbox2, *vbox2, *frame, *table;
    int i;
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = points_setup(d);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), w);
    gtk_box_append(GTK_BOX(hbox), frame);

    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Type:"), FALSE, i++);
    g_signal_connect_swapped(w, "notify::selected", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->path_type = w;

    w = combo_box_create();
    d->interpolation = w;
    add_widget_to_table(table, w, _("_Interpolation:"), FALSE, i++);
    g_signal_connect_swapped(w, "notify::selected", G_CALLBACK(legend_dialog_set_sensitive), d);

    gtk_box_append(GTK_BOX(vbox2), table);

    table = gtk_grid_new();

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Close path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->close_path = w;

    create_maker_setting_widgets(d, table, i++);

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    miter_setup(d, table, i++);
    join_setup(d, table, i++);

    stroke_color_setup(d, table, i++);
    stroke_alpha_setup(d, table, i++);

    gtk_box_append(GTK_BOX(hbox2), table);

    create_arrow_setting_widgets(d, hbox2);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), hbox2);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox2), frame);

    table = gtk_grid_new();

    i = 0;
    w = combo_box_create();
    d->fill_rule = w;
    add_widget_to_table(table, w, _("fill _Rule:"), FALSE, i++);

    fill_color_setup(d, table, i++);
    fill_alpha_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox2), frame);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), vbox2);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "path");

    d->prop_cb = LegendLineCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
}

void
LegendArrowDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendArrowDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
LegendRectDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend rectangle %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *vbox, *frame, *table;
    int i;
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x1 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y1 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Width:"), FALSE, i++);
    d->x2 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Height:"), FALSE, i++);
    d->y2 = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    table = gtk_grid_new();

    i = 0;

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    stroke_color_setup(d, table, i++);
    stroke_alpha_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox), frame);

    table = gtk_grid_new();

    i = 0;
    fill_color_setup(d, table, i++);
    fill_alpha_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox), frame);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "rectangle");

    d->prop_cb = LegendRectCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
}

void
LegendRectDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendRectDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
LegendArcDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend arc %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *hbox2, *vbox, *table, *frame;
    int i;
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table(table, w, "_RX:", FALSE, i++);
    d->rx = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table(table, w, "_RY:", FALSE, i++);
    d->ry = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);

    table = gtk_grid_new();

    i = 0;
    w = create_direction_entry(table, _("_Angle1:"), i++);
    d->angle1 = w;

    w = create_direction_entry(table, _("_Angle2:"), i++);
    d->angle2 = w;

    w = gtk_check_button_new_with_mnemonic(_("_Pieslice"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->pieslice = w;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(vbox), table);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    table = gtk_grid_new();

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Close path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->close_path = w;

    create_maker_setting_widgets(d, table, i++);

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    miter_setup(d, table, i++);
    join_setup(d, table, i++);
    stroke_color_setup(d, table, i++);
    stroke_alpha_setup(d, table, i++);

    gtk_box_append(GTK_BOX(hbox2), table);

    create_arrow_setting_widgets(d, hbox2);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), hbox2);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox), frame);

    table = gtk_grid_new();

    i = 0;
    fill_color_setup(d, table, i++);
    fill_alpha_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect_swapped(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT | WIDGET_MARGIN_BOTTOM);
    gtk_box_append(GTK_BOX(vbox), frame);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "arc");

    d->prop_cb = LegendArcCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
}

void
LegendArcDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendArcDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
LegendMarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend mark %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *frame, *table;
    int i;
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);

    table = gtk_grid_new();

    i = 0;
    style_setup(d, table, i++);
    width_setup(d, table, i++);

    w = gtk_button_new();
    add_widget_to_table(table, w, _("_Mark:"), FALSE, i++);
    g_signal_connect(w, "clicked", G_CALLBACK(mark_popover_popup), NULL);
    mark_popover_new (w, NULL);
    d->type = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Size:"), FALSE, i++);
    d->size = w;

    color_setup(d, table, i++);
    alpha_setup(d, table, i++);
    color2_setup(d, table, i++);
    alpha2_setup(d, table, i++);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "mark");

    d->prop_cb = LegendArcCB;
  }

  legend_dialog_setup_item(wi, d, d->Id);

  if (makewidget)
    d->widget = NULL;
}

void
LegendMarkDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendMarkDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
legend_dialog_setup_sub(struct LegendDialog *d, GtkWidget *table, int i, int instance)
{
  GtkWidget *w;

  if (instance) {
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Pt:"), FALSE, i++);
    d->pt = w;
  } else {
    d->pt = NULL;
  }

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_SPACE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Space:"), FALSE, i++);
  d->space = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_range(w, SCRIPT_SIZE_MIN, SCRIPT_SIZE_MAX);
  add_widget_to_table(table, w, _("_Script:"), FALSE, i++);
  d->script_size = w;

  if (instance) {
    GtkWidget *btn_box;
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

    color_setup(d, table, i++);
    alpha_setup(d, table, i++);
  } else {
    d->font = NULL;
    d->font_bold = NULL;
    d->font_italic = NULL;
    d->color = NULL;
  }

  w = create_direction_entry(table, _("_Direction:"), i++);
  d->direction = w;

  w = gtk_check_button_new_with_mnemonic(_("_Raw"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->raw = w;
}

static void
insert_selcted_char(GtkGridView *icon_view, guint i, gpointer user_data)
{
  GtkSelectionModel *model;
  GtkStringList *list;
  const char *str;
  GtkEditable *entry;
  int pos;

  model = gtk_grid_view_get_model(icon_view);
  list = GTK_STRING_LIST (gtk_single_selection_get_model (GTK_SINGLE_SELECTION (model)));
  str = gtk_string_list_get_string (list, i);

  entry = GTK_EDITABLE(user_data);
  pos = gtk_editable_get_position(entry);
  gtk_editable_insert_text(entry, str, -1, &pos);
  gtk_editable_set_position(entry, pos + 1);
  gtk_widget_grab_focus(GTK_WIDGET(user_data));
  gtk_editable_select_region(entry, pos, pos);
}

static void
setup_character_map (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;

  label = gtk_label_new (NULL);
  gtk_list_item_set_child (list_item, label);
}

static void
bind_char_cb (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;
  gpointer item;
  const char *string;
  char *markup;

  label = gtk_list_item_get_child (list_item);
  item = gtk_list_item_get_item (list_item);
  string = gtk_string_object_get_string (GTK_STRING_OBJECT (item));
  markup = g_markup_printf_escaped ("<big>%s</big>", string);
  gtk_label_set_markup (GTK_LABEL(label), markup);
  g_free (markup);
}

static GtkWidget *
create_character_view(GtkWidget *entry, gchar *data)
{
  GtkWidget *icon_view, *swin;
  gchar *ptr;
  GtkStringList *list;
  GtkSelectionModel *model;
  GtkListItemFactory *factory;
  list = gtk_string_list_new (NULL);
  model = GTK_SELECTION_MODEL(gtk_single_selection_new (G_LIST_MODEL (list)));
  factory = gtk_signal_list_item_factory_new();
  icon_view = gtk_grid_view_new (model, factory);
  gtk_grid_view_set_max_columns (GTK_GRID_VIEW (icon_view), 100);
  gtk_grid_view_set_single_click_activate (GTK_GRID_VIEW (icon_view), TRUE);
  g_signal_connect (factory, "setup", G_CALLBACK (setup_character_map), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_char_cb), NULL);
  g_signal_connect (icon_view, "activate", G_CALLBACK(insert_selcted_char), entry);

  for (ptr = data; *ptr; ptr = g_utf8_next_char(ptr)) {
    gunichar ch;
    gchar str[8];
    int l;

    ch = g_utf8_get_char(ptr);
    l = g_unichar_to_utf8(ch, str);
    str[l] = '\0';
    gtk_string_list_append (list, str);
  }

  swin = gtk_scrolled_window_new();

  gtk_widget_set_size_request(GTK_WIDGET(swin), -1, 100);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), icon_view);
  gtk_widget_set_vexpand (swin, TRUE);

  return swin;
}

static GtkWidget *
create_character_panel(GtkWidget *entry)
{
  GtkWidget *tab, *label, *child;
  struct character_map_list *list;

  tab = gtk_notebook_new();

  for (list = Menulocal.char_map; list; list = list->next) {
    char *data, *title;

    data = list->data;
    title = list->title;
    if (data && data[0]) {
      label = gtk_label_new_with_mnemonic(_(title));
      child = create_character_view(entry, data);
      gtk_notebook_append_page(GTK_NOTEBOOK(tab), child, label);
    }
  }

  return tab;
}

static void
LegendTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend text %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *frame, *table;
    int i;
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);

    table = gtk_grid_new();

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Text:"), TRUE, i++);
    d->text = w;

    legend_dialog_setup_sub(d, table, i++, TRUE);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(hbox), frame);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    w = create_character_panel(d->text);
    gtk_box_append(GTK_BOX(d->vbox), w);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "text");

    d->prop_cb = LegendTextCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
  entry_completion_set_entry(NgraphApp.legend_text_list, d->text);
  d->focus = d->text;
}

void
LegendTextDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendTextDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
LegendTextDefDialogSetup(GtkWidget *w, void *data, int makewidget)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) data;
  if (makewidget) {
    GtkWidget *table, *frame;
    init_legend_dialog_widget_member(d);

    table = gtk_grid_new();
    legend_dialog_setup_sub(d, table, 0, FALSE);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), table);
    gtk_box_append(GTK_BOX(d->vbox), frame);

    add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(legend_copy_clicked), d, "text");

    d->prop_cb = LegendTextCB;
  }
  legend_dialog_setup_item(w, d, d->Id);
}

void
LegendTextDefDialog(struct LegendDialog *data,
		    struct objlist *obj, int id)
{
  data->SetupWindow = LegendTextDefDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

void
CmLineDel(void)
{
  legend_menu_delete_object("path", LegendLineCB);
}

void
CmLineUpdate(void)
{
  legend_menu_update_object("path", LegendLineCB, &DlgLegendArrow, LegendArrowDialog);
}

void
CmRectDel(void)
{
  legend_menu_delete_object("rectangle", LegendRectCB);
}

void
CmRectUpdate(void)
{
  legend_menu_update_object("rectangle", LegendRectCB, &DlgLegendRect, LegendRectDialog);
}

void
CmArcDel(void)
{
  legend_menu_delete_object("arc", LegendArcCB);
}

void
CmArcUpdate(void)
{
  legend_menu_update_object("arc", LegendArcCB, &DlgLegendArc, LegendArcDialog);
}

void
CmMarkDel(void)
{
  legend_menu_delete_object("mark", LegendMarkCB);
}

void
CmMarkUpdate(void)
{
  legend_menu_update_object("mark", LegendMarkCB, &DlgLegendMark, LegendMarkDialog);
}

void
CmTextDel(void)
{
  legend_menu_delete_object("text", LegendTextCB);
}

void
CmTextUpdate(void)
{
  legend_menu_update_object("text", LegendTextCB, &DlgLegendText, LegendTextDialog);
}

static void
option_text_def_dialog_response_response(int ret, struct objlist *obj, int id, int modified)
{
  char *objs[2];
  if (ret) {
    exeobj(obj, "save_config", id, 0, NULL);
  }
  delobj(obj, id);
  objs[0] = obj->name;
  objs[1] = NULL;
  UpdateAll2(objs, TRUE);
  if (! modified) {
    reset_graph_modified();
  }
}

static void
option_text_def_dialog_response(struct response_callback *cb)
{
  int modified;
  modified = GPOINTER_TO_INT(cb->data);
  if (cb->return_value == IDOK) {
    CheckIniFile(option_text_def_dialog_response_response, DlgLegendTextDef.Obj, DlgLegendTextDef.Id, modified);
  } else {
    option_text_def_dialog_response_response(FALSE, DlgLegendTextDef.Obj, DlgLegendTextDef.Id, modified);
  }
}

void
CmOptionTextDef(void)
{
  struct objlist *obj;
  int id;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("text")) == NULL)
    return;

  id = newobj(obj);
  if (id >= 0) {
    int modified;

    modified = get_graph_modified();
    LegendTextDefDialog(&DlgLegendTextDef, obj, id);
    response_callback_add(&DlgLegendTextDef, option_text_def_dialog_response, NULL, GINT_TO_POINTER(modified));
    DialogExecute(TopLevel, &DlgLegendTextDef);
  }
}

static void
LegendWinPathUpdate(struct obj_list_data *data, int id, int user_data)
{
  LegendArrowDialog(&DlgLegendArrow, data->obj, id);
}

static void
LegendWinRectUpdate(struct obj_list_data *data, int id, int user_data)
{
  LegendRectDialog(&DlgLegendRect, data->obj, id);
}

static void
LegendWinArcUpdate(struct obj_list_data *data, int id, int user_data)
{
  LegendArcDialog(&DlgLegendArc, data->obj, id);
}

static void
LegendWinMarkUpdate(struct obj_list_data *data, int id, int user_data)
{
  LegendMarkDialog(&DlgLegendMark, data->obj, id);
}

static void
LegendWinTextUpdate(struct obj_list_data *data, int id, int user_data)
{
  LegendTextDialog(&DlgLegendText, data->obj, id);
}

static void
ObjListUpdate(struct obj_list_data *d, int clear, int draw)
{
  if (Menulock || Globallock)
    return;

  if (d->text == NULL) {
    return;
  }

  list_sub_window_build(d);

  if (! clear && d->select >= 0) {
    columnview_set_active(d->text, d->select, TRUE);
  }
  if (draw) {
//    NgraphApp.Viewer.allclear = TRUE;
    update_viewer(d);
  }
}


void
LegendWinUpdate(char **objects, int clear, int draw)
{
  struct objlist *obj;
  char **ptr;
  struct SubWin *win[] = {
    &NgraphApp.PathWin,
    &NgraphApp.RectWin,
    &NgraphApp.ArcWin,
    &NgraphApp.MarkWin,
    &NgraphApp.TextWin,
  };
  int i, n;
  if (Menulock || Globallock)
    return;

  n = G_N_ELEMENTS(win);
  for (i = 0; i < n; i++) {
    struct obj_list_data *d;
    d = win[i]->data.data;
    if (d == NULL) {
      return;
    }
    if (objects) {
      for (ptr = objects; *ptr; ptr++) {
	obj = getobject(*ptr);
	if (obj == d->obj) {
	  d->update(d, clear, draw);
	  break;
	}
      }
    } else {
      d->update(d, clear, draw);
    }
  }
}

static void
get_points(struct objlist *obj, int id, int *x, int *y, int *n)
{
  int *points, i;
  struct narray *array;

  getobj(obj, "points", id, 0, NULL, &array);
  points = arraydata(array);
  i = arraynum(array);
  *n = i / 2;
  if (i < 2) {
    *x = 0;
    *y = 0;
  } else {
    *x = points[0];
    *y = points[1];
  }
}

static GdkPixbuf *
draw_color_pixbuf(struct objlist *obj, int id, enum OBJ_FIELD_COLOR_TYPE type, int width)
{
  int ggc, fr, fg, fb, stroke, fill, close_path, height = 20, lockstate, found, output, n;
  cairo_surface_t *pix;
  GdkPixbuf *pixbuf;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;
  struct narray *line_style;

  lockstate = Globallock;
  Globallock = TRUE;

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return NULL;
  }

  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return NULL;
  }

  ggc = _GRAopen("gra2gdk", "_output",
		 robj, inst, output, -1, -1, -1, NULL, local);
  if (ggc < 0) {
    _GRAclose(ggc);
    g_object_unref(G_OBJECT(pix));
    return NULL;
  }

  GRAview(ggc, 0, 0, width, height, 0);

  if (chkobjfieldtype(obj, "style") == NINT) {
    line_style = NULL;
  } else {
    getobj(obj, "style", id, 0, NULL, &line_style);
  }
  n = arraynum(line_style);
  if (n > 0) {
    int i, *style, *ptr;
    style = g_malloc(sizeof(*style) * n);
    if (style == NULL) {
      _GRAclose(ggc);
      g_object_unref(G_OBJECT(pix));
      return NULL;
    }
    ptr = arraydata(line_style);
    for (i = 0; i < n; i++) {
      style[i] = ptr[i] / 30;
    }
    GRAlinestyle(ggc, n, style, 4, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
    g_free(style);
  } else {
    GRAlinestyle(ggc, 0, NULL, 4, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
  }

  if (type == OBJ_FIELD_COLOR_TYPE_STROKE) {
    getobj(obj, "fill", id, 0, NULL, &fill);
    if (fill) {
      getobj(obj, "fill_R", id, 0, NULL, &fr);
      getobj(obj, "fill_G", id, 0, NULL, &fg);
      getobj(obj, "fill_B", id, 0, NULL, &fb);
      GRAcolor(ggc, fr, fg, fb, 255);
      GRArectangle(ggc, 2, 2, width - 2, height - 2, 1);
    }

    getobj(obj, "stroke", id, 0, NULL, &stroke);
    if (stroke) {
      int pos[8];
      if (chkobjfield(obj, "close_path") == 0) {
	getobj(obj, "close_path", id, 0, NULL, &close_path);
      } else {
	close_path = TRUE;
      }
      getobj(obj, "stroke_R", id, 0, NULL, &fr);
      getobj(obj, "stroke_G", id, 0, NULL, &fg);
      getobj(obj, "stroke_B", id, 0, NULL, &fb);
      GRAcolor(ggc, fr, fg, fb, 255);
      pos[0] = 2;
      pos[1] = height - 2;
      pos[2] = 2;
      pos[3] = 2;
      pos[4] = width - 2;
      pos[5] = 2;
      pos[6] = width - 2;
      pos[7] = height - 2;
      if (close_path) {
	GRAdrawpoly(ggc, 4, pos, GRA_FILL_MODE_NONE);
      } else {
	GRAlines(ggc, 4, pos);
      }
    }
  } else {
    getobj(obj, "R", id, 0, NULL, &fr);
    getobj(obj, "G", id, 0, NULL, &fg);
    getobj(obj, "B", id, 0, NULL, &fb);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, 0, 0, width, height, 1);
  }

  _GRAclose(ggc);
  gra2cairo_draw_path(local);

  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);

  Globallock = lockstate;

  return pixbuf;
}

static void *
bind_color (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  GdkPixbuf *pixbuf;
  pixbuf = draw_color_pixbuf(obj, id, OBJ_FIELD_COLOR_TYPE_STROKE, 40);
  if (pixbuf) {
    GdkTexture *texture;
    texture = gdk_texture_new_for_pixbuf (pixbuf);
    g_object_unref (pixbuf);
    return texture;
  }
  return NULL;
}

static int
select_path_type_menu (struct objlist *obj, const char *field, int id, GtkStringList *list, int sel)
{
  int type, interpolation;

  getobj(obj, "type", id, 0, NULL, &type);
  getobj(obj, "interpolation", id, 0, NULL, &interpolation);

  if (type == 0 && sel == 0) {
    return 1;
  }

  if (type == 1 && interpolation == sel - 1) {
    return 1;
  }

  menu_save_undo_single (UNDO_TYPE_EDIT, obj->name);
  if (sel == 0) {
    putobj (obj, "type", id, &sel);
    return 0;
  }

  type = 1;
  interpolation = sel -1;
  putobj (obj, "type", id, &type);
  putobj (obj, "interpolation", id, &interpolation);

  return 0;
}

static int
setup_path_type_menu (struct objlist *obj, const char *field, int id, GtkStringList *list)
{
  int type, interpolation;
  const char **enumlist;
  int i;

  enumlist = (const char **) chkobjarglist(obj, "type");
  gtk_string_list_append (list, _(enumlist[0]));

  enumlist = (const char **) chkobjarglist(obj, "interpolation");
  for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
    gtk_string_list_append (list, _(enumlist[i]));
  }

  getobj(obj, "type", id, 0, NULL, &type);
  if (type == 0) {
    return 0;
  }

  getobj(obj, "interpolation", id, 0, NULL, &interpolation);
  return interpolation + 1;
}

static void *
bind_path_type (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  char **enumlist;
  int type, interpolation;

  getobj(obj, "type", id, 0, NULL, &type);
  getobj(obj, "interpolation", id, 0, NULL, &interpolation);

  if (type == 0) {
    enumlist = (char **) chkobjarglist(obj, "type");
    return g_strdup (_(enumlist[0]));
  } else {
    enumlist = (char **) chkobjarglist(obj, "interpolation");
    return g_strdup (_(enumlist[interpolation]));
  }
}

static void *
bind_path_pos (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  int n, x0, y0;
  char str[256];

  get_points(obj, id, &x0, &y0, &n);
  switch (field[0]) {
  case 'x':
    snprintf (str, sizeof (str), "%.2f", x0 / 100.0);
    break;
  case 'y':
    snprintf (str, sizeof (str), "%.2f", y0 / 100.0);
    break;
  case 'p':
    snprintf (str, sizeof (str), "%d", n);
    break;
  default:
    return NULL;
  }
  return g_strdup (str);
}

static void *
bind_rect_x (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  int x1, x2;
  char str[256];

  getobj(obj, "x1", id, 0, NULL, &x1);
  getobj(obj, "x2", id, 0, NULL, &x2);
  if (field[1] == '1') {
    snprintf (str, sizeof (str), "%.2f",  ((x1 < x2) ? x1 : x2) / 100.0);
  } else {
    snprintf (str, sizeof (str), "%.2f",  abs(x1 - x2) / 100.0);
  }
  return g_strdup (str);
}

static void *
bind_rect_y (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  int y1, y2;
  char str[256];

  getobj(obj, "y1", id, 0, NULL, &y1);
  getobj(obj, "y2", id, 0, NULL, &y2);
  if (field[1] == '1') {
    snprintf (str, sizeof (str), "%.2f",  ((y1 < y2) ? y1 : y2) / 100.0);
  } else {
    snprintf (str, sizeof (str), "%.2f",  abs(y1 - y2) / 100.0);
  }
  return g_strdup (str);
}

static GdkPixbuf *
draw_mark_pixbuf(struct objlist *obj, int i)
{
  int ggc, fr, fg, fb, fr2, fg2, fb2,
    width = 20, height = 20, marktype,
    lockstate, found, output;
  cairo_surface_t *pix;
  GdkPixbuf *pixbuf;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;

  lockstate = Globallock;
  Globallock = TRUE;

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return NULL;
  }

  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return NULL;
  }


  getobj(obj, "R", i, 0, NULL, &fr);
  getobj(obj, "G", i, 0, NULL, &fg);
  getobj(obj, "B", i, 0, NULL, &fb);

  getobj(obj, "R2", i, 0, NULL, &fr2);
  getobj(obj, "G2", i, 0, NULL, &fg2);
  getobj(obj, "B2", i, 0, NULL, &fb2);

  ggc = _GRAopen("gra2gdk", "_output",
		 robj, inst, output, -1, -1, -1, NULL, local);
  if (ggc < 0) {
    _GRAclose(ggc);
    g_object_unref(G_OBJECT(pix));
    return NULL;
  }
  GRAview(ggc, 0, 0, width, height, 0);
  GRAcolor(ggc, fr, fg, fb, 255);
  GRAlinestyle(ggc, 0, NULL, 1, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);

  getobj(obj, "type", i, 0, NULL, &marktype);
  GRAmark(ggc, marktype, height / 2, height / 2, height - 2,
	  fr, fg, fb, 255, fr2, fg2, fb2, 255);

  _GRAclose(ggc);
  gra2cairo_draw_path(local);

  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);

  Globallock = lockstate;

  return pixbuf;
}

static void *
bind_mark (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  GdkPixbuf *pixbuf;
  pixbuf = draw_mark_pixbuf(obj, id);
  if (pixbuf) {
    GdkTexture *texture;
    texture = gdk_texture_new_for_pixbuf (pixbuf);
    g_object_unref (pixbuf);
    return texture;
  }
  return NULL;
}

static int
select_font_menu (struct objlist *obj, const char *field, int id, GtkStringList *list, int sel)
{
  const char *cur, *font;
  char *str;
  if (sel < 0) {
    return 1;
  }
  font = gtk_string_list_get_string (list, sel);
  if (font == NULL) {
    return 1;
  }
  getobj (obj, field, id, 0, NULL, &cur);
  if (g_strcmp0 (font, cur) == 0) {
    return 1;
  }
  str = g_strdup (font);
  menu_save_undo_single (UNDO_TYPE_EDIT, obj->name);
  putobj (obj, field, id, str);
  return 0;
}

static int
setup_font_menu (struct objlist *obj, const char *field, int id, GtkStringList *list)
{
  struct fontmap *fcur;
  char *font;
  int j, selfont;

  getobj(obj, field, id, 0, NULL, &font);
  fcur = Gra2cairoConf->fontmap_list_root;
  j = selfont = 0;
  while (fcur) {
    gtk_string_list_append (list, fcur->fontalias);
    if (font && strcmp(font, fcur->fontalias) == 0) {
      selfont = j;
    }
    j++;
    fcur = fcur->next;
  }
  return selfont;
}

static void *
bind_text (GtkWidget *w, struct objlist *obj, const char *field, int id)
{
  int style, r, g ,b;
  struct fontmap *fmap;
  char *text, *str, *font, *alias;

  getobj(obj, field, id, 0, NULL, &str);
  if (str == NULL) {
    gtk_label_set_text(GTK_LABEL (w), NULL);
    return NULL;
  }

  getobj(obj, "font", id, 0, NULL, &alias);
  fmap = gra2cairo_get_fontmap(alias);
  if (fmap && fmap->fontname) {
    font = fmap->fontname;
  } else {
    font = "Sans";
  }

  getobj(obj, "style", id, 0, NULL, &style);
  getobj(obj, "R", id, 0, NULL, &r);
  getobj(obj, "G", id, 0, NULL, &g);
  getobj(obj, "B", id, 0, NULL, &b);
  text = g_markup_printf_escaped ("<span"
				  " face=\"%s\""
				  " style=\"%s\""
				  " weight=\"%s\""
				  " fgcolor=\"#%02x%02x%02x\""
				  " bgcolor=\"#%02x%02x%02x\">"
				  "%s"
				  "</span>",
				  font,
				  (style & GRA_FONT_STYLE_ITALIC) ? "italic" : "normal",
				  (style & GRA_FONT_STYLE_BOLD) ? "bold" : "normal",
				  r, g, b,
				  ((int) (Menulocal.bg_r * 255)) & 0xff,
				  ((int) (Menulocal.bg_g * 255)) & 0xff,
				  ((int) (Menulocal.bg_b * 255)) & 0xff,
				  str);
  gtk_label_set_use_markup (GTK_LABEL (w), TRUE);
  return text;
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  unsigned int i;
  int m, last_id;
  struct obj_list_data *d;
  const char *name;

  d = (struct obj_list_data *) user_data;

  if (d->text == NULL) {
    return;
  }

  name = chkobjectname(d->obj);
  m = columnview_get_active(d->text);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    char action_name[256];
    GAction *action;
    snprintf(action_name, sizeof(action_name), PopupAction[i].name, name);
    action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), action_name);
    switch (i) {
    case POPUP_ITEM_FOCUS_ALL:
      last_id = chkobjlastinst(d->obj);
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), last_id >= 0);
      break;
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), m > 0);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      last_id = -1;
      if (m >= 0) {
	last_id = chkobjlastinst(d->obj);
      }
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), m >= 0 && m < last_id);
      break;
    default:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), m >= 0);
    }
  }
}

static void
create_legend_popup_menu(struct obj_list_data *d)
{
  GActionEntry *entry;
  int i;
  const char *name;

  entry = g_malloc(POPUP_ITEM_NUM * sizeof(*PopupAction));
  if (entry == NULL) {
    return;
  }

  name = chkobjectname(d->obj);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    char *action;
    entry[i] = PopupAction[i];
    action = g_strdup_printf(PopupAction[i].name, name);
    entry[i].name = action;
  }

  sub_win_create_popup_menu(d, POPUP_ITEM_NUM, entry, G_CALLBACK(popup_show_cb));

  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    g_free((char *) entry[i].name);
  }
  g_free(entry);
}

GtkWidget *
create_path_list(struct SubWin *d)
{
  struct obj_list_data *data;

  list_sub_window_create(d, PATH_LIST_COL_NUM, Plist);
  data = d->data.data;
  data->update = ObjListUpdate;
  data->dialog = &DlgLegendArrow;
  data->setup_dialog = LegendWinPathUpdate;
  data->ev_key = NULL;
  data->obj = chkobject("path");

  create_legend_popup_menu(data);

  return d->Win;
}

GtkWidget *
create_rect_list(struct SubWin *d)
{
  struct obj_list_data *data;

  list_sub_window_create(d, RECT_LIST_COL_NUM, Rlist);
  data = d->data.data;
  data->update = ObjListUpdate;
  data->dialog = &DlgLegendRect;
  data->setup_dialog = LegendWinRectUpdate;
  data->ev_key = NULL;
  data->obj = chkobject("rectangle");

  create_legend_popup_menu(data);

  return d->Win;
}

GtkWidget *
create_arc_list(struct SubWin *d)
{
  struct obj_list_data *data;

  list_sub_window_create(d, ARC_LIST_COL_NUM, Alist);
  data = d->data.data;
  data->update = ObjListUpdate;
  data->dialog = &DlgLegendArc;
  data->setup_dialog = LegendWinArcUpdate;
  data->ev_key = NULL;
  data->obj = chkobject("arc");

  create_legend_popup_menu(data);

  return d->Win;
}

GtkWidget *
create_mark_list(struct SubWin *d)
{
  struct obj_list_data *data;

  list_sub_window_create(d, MARK_LIST_COL_NUM, Mlist);
  data = d->data.data;
  data->update = ObjListUpdate;
  data->dialog = &DlgLegendMark;
  data->setup_dialog = LegendWinMarkUpdate;
  data->ev_key = NULL;
  data->obj = chkobject("mark");

  create_legend_popup_menu(data);

  return d->Win;
}

GtkWidget *
create_text_list(struct SubWin *d)
{
  struct obj_list_data *data;

  list_sub_window_create(d, TEXT_LIST_COL_NUM, Tlist);
  data = d->data.data;
  data->update = ObjListUpdate;
  data->dialog = &DlgLegendText;
  data->setup_dialog = LegendWinTextUpdate;
  data->ev_key = NULL;
  data->obj = chkobject("text");

  create_legend_popup_menu(data);

  return d->Win;
}
