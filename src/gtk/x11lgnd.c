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

#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "odraw.h"
#include "otext.h"
#include "opath.h"
#include "nstring.h"
#include "mathfn.h"

#include "gtk_liststore.h"
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

#define ARROW_VIEW_SIZE 160
#define CB_BUF_SIZE 128
#define LEGENDNUM 5

static n_list_store Plist[] = {
  {" ",            G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",            G_TYPE_INT,     TRUE, FALSE, "id"},
  {"type",         G_TYPE_PARAM,   TRUE, TRUE,  "type"},
  {"arrow",        G_TYPE_ENUM,    TRUE, TRUE,  "arrow"},
  {N_("color"),    G_TYPE_OBJECT,  TRUE, TRUE,  "color"},
  {"x",            G_TYPE_DOUBLE,  TRUE, TRUE,  "x", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE, TRUE,  "y", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),    G_TYPE_DOUBLE,  TRUE, TRUE,  "width",                0, SPIN_ENTRY_MAX,  20,  100},
  {N_("points"),   G_TYPE_INT,     TRUE, FALSE, "points"},
  {"^#",           G_TYPE_INT,     TRUE, FALSE, "oid"},
};

enum PATH_LIST_COL {
  PATH_LIST_COL_HIDDEN = 0,
  PATH_LIST_COL_ID,
  PATH_LIST_COL_TYPE,
  PATH_LIST_COL_ARROW,
  PATH_LIST_COL_COLOR,
  PATH_LIST_COL_X,
  PATH_LIST_COL_Y,
  PATH_LIST_COL_WIDTH,
  PATH_LIST_COL_POINTS,
  PATH_LIST_COL_OID,
  PATH_LIST_COL_NUM,
};

static n_list_store Rlist[] = {
  {" ",              G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",              G_TYPE_INT,     TRUE, FALSE, "id"},
  {N_("color"),      G_TYPE_OBJECT,  TRUE, TRUE,  "color"},
  {"x",              G_TYPE_DOUBLE,  TRUE, TRUE,  "x1", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",              G_TYPE_DOUBLE,  TRUE, TRUE,  "y1", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"width",          G_TYPE_DOUBLE,  TRUE, TRUE,  "width",             0, SPIN_ENTRY_MAX,  20,  100},
  {N_("height"),     G_TYPE_DOUBLE,  TRUE, TRUE,  "height",            0, SPIN_ENTRY_MAX,  20,  100},
  {N_("line width"), G_TYPE_DOUBLE,  TRUE, TRUE,  "width",             0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",             G_TYPE_INT,     TRUE, FALSE, "oid"},
};

enum RECT_LIST_COL {
  RECT_LIST_COL_HIDDEN = 0,
  RECT_LIST_COL_ID,
  RECT_LIST_COL_COLOR,
  RECT_LIST_COL_X,
  RECT_LIST_COL_Y,
  RECT_LIST_COL_WIDTH,
  RECT_LIST_COL_HEIGHT,
  RECT_LIST_COL_LWIDTH,
  RECT_LIST_COL_OID,
  RECT_LIST_COL_NUM,
};

static n_list_store Alist[] = {
  {" ",            G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",            G_TYPE_INT,     TRUE, FALSE, "id"},
  {"color",        G_TYPE_OBJECT,  TRUE, TRUE,  "color"},
  {"x",            G_TYPE_DOUBLE,  TRUE, TRUE,  "x", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE, TRUE,  "y", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"rx",           G_TYPE_DOUBLE,  TRUE, TRUE,  "rx", 0, SPIN_ENTRY_MAX, 100, 1000},
  {"ry",           G_TYPE_DOUBLE,  TRUE, TRUE,  "ry", 0, SPIN_ENTRY_MAX, 100, 1000},
  {N_("angle1"),   G_TYPE_DOUBLE,  TRUE, TRUE,  "angle1", 0, 36000, 100, 1500},
  {N_("angle2"),   G_TYPE_DOUBLE,  TRUE, TRUE,  "angle2", 0, 36000, 100, 1500},
  {N_("pieslice"), G_TYPE_BOOLEAN, TRUE, TRUE,  "pieslice"},
  {N_("width"),    G_TYPE_DOUBLE,  TRUE, TRUE,  "width", 0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",           G_TYPE_INT,     TRUE, FALSE, "oid"},
};

enum ARC_LIST_COL {
  ARC_LIST_COL_HIDDEN = 0,
  ARC_LIST_COL_ID,
  ARC_LIST_COL_COLOR,
  ARC_LIST_COL_X,
  ARC_LIST_COL_Y,
  ARC_LIST_COL_RX,
  ARC_LIST_COL_RY,
  ARC_LIST_COL_ANGLE1,
  ARC_LIST_COL_ANGLE2,
  ARC_LIST_COL_PIESLICE,
  ARC_LIST_COL_WIDTH,
  ARC_LIST_COL_OID,
  ARC_LIST_COL_NUM,
};

static n_list_store Mlist[] = {
  {" ",            G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",            G_TYPE_INT,     TRUE, FALSE, "id"},
  {"mark",         G_TYPE_OBJECT,  TRUE, TRUE,  "type"},
  {"x",            G_TYPE_DOUBLE,  TRUE, TRUE,  "x", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE, TRUE,  "y", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("size"),     G_TYPE_DOUBLE,  TRUE, TRUE,  "size",             0, SPIN_ENTRY_MAX,  20,  100},
  {"width",        G_TYPE_DOUBLE,  TRUE, TRUE,  "width",            0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",           G_TYPE_INT,     TRUE, FALSE, "oid"},
};

enum MARK_LIST_COL {
  MARK_LIST_COL_HIDDEN = 0,
  MARK_LIST_COL_ID,
  MARK_LIST_COL_MARK,
  MARK_LIST_COL_X,
  MARK_LIST_COL_Y,
  MARK_LIST_COL_SIZE,
  MARK_LIST_COL_WIDTH,
  MARK_LIST_COL_OID,
  MARK_LIST_COL_NUM,
};

static n_list_store Tlist[] = {
  {" ",             G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",             G_TYPE_INT,     TRUE, FALSE, "id"},
  {"text",          G_TYPE_STRING,  TRUE, TRUE,  "text", 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
  {N_("font"),      G_TYPE_PARAM,   TRUE, TRUE,  "font"},
  {"color",         G_TYPE_OBJECT,  TRUE, TRUE,  "color"},
  {"x",             G_TYPE_DOUBLE,  TRUE, TRUE,  "x", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",             G_TYPE_DOUBLE,  TRUE, TRUE,  "y", - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("pt"),        G_TYPE_DOUBLE,  TRUE, TRUE,  "pt",                0, SPIN_ENTRY_MAX,  20,  100},
  {N_("direction"), G_TYPE_DOUBLE,  TRUE, TRUE,  "direction",         0, 36000,          100, 1500},
  {"raw",           G_TYPE_BOOLEAN, TRUE, TRUE,  "raw"},
  {"^#",            G_TYPE_INT,     TRUE, FALSE, "oid"},
  {"style",         G_TYPE_INT,     FALSE, FALSE, "style"},
  {"weight",        G_TYPE_INT,     FALSE, FALSE, "weight"},
#ifdef TEXT_LIST_USE_FONT_FAMILY
  {"font_family",   G_TYPE_STRING,  FALSE, FALSE, "font_family"},
#endif
};

enum TEXT_LIST_COL {
  TEXT_LIST_COL_HIDDEN = 0,
  TEXT_LIST_COL_ID,
  TEXT_LIST_COL_TEXT,
  TEXT_LIST_COL_FONT,
  TEXT_LIST_COL_COLOR,
  TEXT_LIST_COL_X,
  TEXT_LIST_COL_Y,
  TEXT_LIST_COL_PT,
  TEXT_LIST_COL_DIR,
  TEXT_LIST_COL_RAW,
  TEXT_LIST_COL_OID,
  TEXT_LIST_COL_STYLE,
  TEXT_LIST_COL_WEIGHT,
#ifdef TEXT_LIST_USE_FONT_FAMILY
  TEXT_LIST_COL_FONT_FAMILY,
#endif
  TEXT_LIST_COL_NUM,
};

static n_list_store *Llist[] = {Plist, Rlist, Alist, Mlist, Tlist};
static int Llist_num[] = {PATH_LIST_COL_NUM, RECT_LIST_COL_NUM, ARC_LIST_COL_NUM, MARK_LIST_COL_NUM, TEXT_LIST_COL_NUM};

static struct subwin_popup_list Popup_list[] = {
  {N_("_Duplicate"),      G_CALLBACK(list_sub_window_copy), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  //  {N_("duplicate _Behind"),   G_CALLBACK(list_sub_window_copy), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_DELETE,      G_CALLBACK(list_sub_window_delete), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Focus"),          G_CALLBACK(list_sub_window_focus), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Show"),           G_CALLBACK(list_sub_window_hide), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_CHECK},
  {GTK_STOCK_PROPERTIES,  G_CALLBACK(list_sub_window_update), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(list_sub_window_move_top), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(list_sub_window_move_up), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(list_sub_window_move_down), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(list_sub_window_move_last), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))
#define POPUP_ITEM_HIDE 4
#define POPUP_ITEM_TOP 7
#define POPUP_ITEM_UP 8
#define POPUP_ITEM_DOWN 9
#define POPUP_ITEM_BOTTOM 10

static void LegendDialogCopy(struct LegendDialog *d);
static void path_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
static void rect_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
static void arc_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
static void mark_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
static void text_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);

static char *legendlist[LEGENDNUM] = {
  N_("path"),
  N_("rectangle"),
  N_("arc"),
  N_("mark"),
  N_("text"),
};

static struct LegendDialog *Ldlg[] = {
  &DlgLegendArrow,
  &DlgLegendRect,
  &DlgLegendArc,
  &DlgLegendMark,
  &DlgLegendText,
};

enum LegendType {
  LegendTypePath = 0,
  LegendTypeRect,
  LegendTypeArc,
  LegendTypeMark,
  LegendTypeText,
};

static char *MarkChar[] = {
  "●", "○", "○", "◎", "⦿", "",  "◑",  "◐",  "◓",  "◒",
  "■", "⬜", "⬜", "⧈", "▣",  "",  "◨",  "◧",  "⬒", "⬓",
  "◆", "◇", "◇", "",   "◈",  "",  "⬗", "⬖", "⬘", "⬙",
  "▲", "△", "△", "",   "",   "",  "◮",  "◭",  "⧗",  "⧖",
  "▼", "▽", "▽", "",   "",   "",  "⧩", "⧨", "",   "",
  "◀", "◁", "◁", "",   "",   "",  "",   "",   "⧓",  "⋈",
  "▶", "▷", "▷", "",   "",   "",  "",   "",   "⧒",  "⧑",
  "＋", "×",  "∗",  "⚹",  "",   "",  "",   "",   "―", "|",
  "◎", "⨁", "⨂", "⧈", "⊞",  "⊠", "",   "",   "",   "·",
};

#define MarkCharNum ((int) (sizeof(MarkChar) / sizeof(*MarkChar)))

struct lwidget {
  GtkWidget *w;
  char *f;
};

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
    s = g_strdup("------");
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
  d->angle1 = NULL;
  d->angle2 = NULL;
  d->fill = NULL;
  d->fill_rule = NULL;
  d->arrow = NULL;
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
  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->font_bold));
  if (bold) {
    style |= GRA_FONT_STYLE_BOLD;
  }

  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->font_italic));
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
      char buf[] = "%F{Sym}", *tmp;
      const char *str;

      str = gtk_entry_get_text(GTK_ENTRY(d->text));
      if (str && strncmp(str, buf, sizeof(buf) - 1)) {
	tmp = g_strdup_printf("%s%s", buf, str);
	gtk_entry_set_text(GTK_ENTRY(d->text), tmp);
	g_free(tmp);
      }
    }
  } else {
    getobj(d->Obj, "style", d->Id, 0, NULL, &style);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->font_bold), style & GRA_FONT_STYLE_BOLD);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->font_italic), style & GRA_FONT_STYLE_ITALIC);
}

static void
legend_dialog_set_sensitive(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;
  int path_type;

  d = (struct LegendDialog *) client_data;

  path_type = PATH_TYPE_LINE;
  if (d->path_type && d->interpolation) {
    path_type = combo_box_get_active(d->path_type);

    set_widget_sensitivity_with_label(d->interpolation, path_type == PATH_TYPE_CURVE);
  }

  if (d->stroke && d->stroke_color && d->style && d->width) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    set_widget_sensitivity_with_label(d->stroke_color, a);
    set_widget_sensitivity_with_label(d->style, a);
    set_widget_sensitivity_with_label(d->width, a);
  }

  if (d->stroke &&
      d->miter &&
      d->join &&
      d->close_path) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    set_widget_sensitivity_with_label(d->miter, a);
    set_widget_sensitivity_with_label(d->join, a);
    set_widget_sensitivity_with_label(d->close_path, a);
  }

  if (d->stroke &&
      d->interpolation &&
      d->close_path &&
      d->arrow &&
      d->arrow_length &&
      d->arrow_width) {
    int a, ca;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    ca = combo_box_get_active(d->interpolation);

    set_widget_sensitivity_with_label(d->miter, a);
    set_widget_sensitivity_with_label(d->join, a);
    set_widget_sensitivity_with_label(d->arrow, a);
    set_widget_sensitivity_with_label(d->arrow_length, a);
    set_widget_sensitivity_with_label(d->arrow_width, a);

    if (path_type == PATH_TYPE_CURVE) {
      set_widget_sensitivity_with_label(d->close_path, a &&
			       (ca != INTERPOLATION_TYPE_SPLINE_CLOSE &&
				ca != INTERPOLATION_TYPE_BSPLINE_CLOSE));
    } else {
      set_widget_sensitivity_with_label(d->close_path, a);
    }
  }

  if (d->fill && d->fill_color) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->fill));
    set_widget_sensitivity_with_label(d->fill_color, a);

    if (d->fill_rule) {
      set_widget_sensitivity_with_label(d->fill_rule, a);
    }
  }
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
    {d->arrow, "arrow"},
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

  if (d->type) {
    int a;

    getobj(d->Obj, "type", id, 0, NULL, &a);
    button_set_mark_image(d->type, a);
    MarkDialog(&d->mark, a);
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
    gtk_entry_set_text(GTK_ENTRY(d->text), buf);
    g_free(buf);
  }

  if (d->font && d->font_bold && d->font_italic)
    set_font(d, id);

  if (d->color)
    set_color(d->color, d->Obj, id, NULL);

  if (d->color2)
    set_color2(d->color2, d->Obj, id);

  if (d->stroke_color)
    set_stroke_color(d->stroke_color, d->Obj, id);

  if (d->fill_color)
    set_fill_color(d->fill_color, d->Obj, id);

  legend_dialog_set_sensitive(NULL, d);
}

static void
legend_dialog_close(GtkWidget *w, void *data)
{
  struct LegendDialog *d = (struct LegendDialog *) data;
  unsigned int i;
  int ret, x1, y1, x2, y2, oval;
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
    {d->arrow, "arrow"},
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
  case IDCOPY:
    LegendDialogCopy(d);
    d->ret = IDLOOP;
    return;
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
    getobj(d->Obj, "type", d->Id, 0, NULL, &oval);
    if (oval != d->mark.Type) {
      if (putobj(d->Obj, "type", d->Id, &(d->mark.Type)) == -1) {
	return;
      }
      set_graph_modified();
    }
  }

  if (d->font && d->font_bold && d->font_italic) {
    get_font(d);
  }

  if (d->text) {
    const char *str;
    char *ptr;

    str = gtk_entry_get_text(GTK_ENTRY(d->text));
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

  if (d->stroke_color && putobj_stroke_color(d->stroke_color, d->Obj, d->Id))
    return;

  if (d->fill_color && putobj_fill_color(d->fill_color, d->Obj, d->Id))
    return;

  if (d->color && putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

  if (d->color2 && putobj_color2(d->color2, d->Obj, d->Id))
    return;

  d->ret = ret;
}

static void
legend_copy_clicked(GtkButton *btn, gpointer user_data)
{
  int sel;
  struct LegendDialog *d;

  d = (struct LegendDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, d->prop_cb);

  if (sel != -1) {
    legend_dialog_setup_item(d->widget, d, sel);
  }
}

static void
LegendDialogCopy(struct LegendDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, d->prop_cb)) != -1)
    legend_dialog_setup_item(d->widget, d, sel);
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
renderer_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  int n;
  double v;
  char buf[1024];

  n = GPOINTER_TO_INT(data);

  gtk_tree_model_get(model, iter, n, &v, -1);
  snprintf(buf, sizeof(buf), "%.2f", v);
  g_object_set((GObject *) renderer, "text", buf, NULL);
}

static void
column_edited(GtkCellRenderer *cell_renderer, gchar *path_str, gchar *str, int column, gpointer user_data)
{
  double d;
  char *eptr;
  GtkTreeModel *list;
  GtkTreeIter iter;
  GtkTreePath *path;
  int r;

  path = gtk_tree_path_new_from_string(path_str);
  if (path == NULL)
    return;

  list = GTK_TREE_MODEL(user_data);
  r = gtk_tree_model_get_iter(list, &iter, path);
  if (! r)
    return;

  d = strtod(str, &eptr);
  if (d != d || d == HUGE_VAL || d == - HUGE_VAL || eptr == str)
    return;

  gtk_list_store_set(GTK_LIST_STORE(list), &iter, column, d, -1);
}

static void
column_0_edited(GtkCellRenderer *cell_renderer, gchar *path_str, gchar *str, gpointer user_data)
{
  column_edited(cell_renderer, path_str, str, 0, user_data);
}

static void
column_1_edited(GtkCellRenderer *cell_renderer, gchar *path_str, gchar *str, gpointer user_data)
{
  column_edited(cell_renderer, path_str, str, 1, user_data);
}

static void
insert_column(GtkWidget *w, gpointer user_data)
{
  GtkTreeView *tree_view;
  GtkTreeModel *list;
  GtkTreeIter iter, tmp;
  GtkTreePath *path;
  int r;

  tree_view = GTK_TREE_VIEW(user_data);

  list = gtk_tree_view_get_model(tree_view);
  if (list == NULL) {
    return;
  }

  gtk_tree_view_get_cursor(tree_view, &path, NULL);
  if (path) {
    r = gtk_tree_model_get_iter(list, &iter, path);
    if (! r)
      goto End;

    gtk_list_store_insert_after(GTK_LIST_STORE(list), &tmp, &iter);
  } else {
    gtk_list_store_append(GTK_LIST_STORE(list), &tmp);
  }

  if (path) {
    gtk_tree_path_free(path);
  }

  path = gtk_tree_model_get_path(list, &tmp);

  if (path) {
    gtk_tree_view_set_cursor(tree_view, path, NULL, FALSE);
  }

 End:
  if (path)
    gtk_tree_path_free(path);
}

static void
set_delete_button_sensitivity(GtkTreeSelection *sel, gpointer user_data)
{
  GtkWidget *btn;
  int n;

  btn = GTK_WIDGET(user_data);
  n = gtk_tree_selection_count_selected_rows(sel);

  gtk_widget_set_sensitive(btn, n > 0);
}

static GtkWidget *
points_setup(struct LegendDialog *d)
{
  GtkWidget *label, *swin, *vbox, *hbox, *tree_view, *btn;
  GtkTreeModel *list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeSelection *sel;
  char *title[] = {"x", "y"};
  int i;
  GCallback edited_func[] = {G_CALLBACK(column_0_edited), G_CALLBACK(column_1_edited)};

  list = GTK_TREE_MODEL(gtk_list_store_new(POINTS_DIMENSION, G_TYPE_DOUBLE, G_TYPE_DOUBLE));
  tree_view = gtk_tree_view_new_with_model(list);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree_view), TRUE);
  gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(tree_view), FALSE);
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_GRID_LINES_VERTICAL);
  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree_view), TRUE);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

  for (i = 0; i < POINTS_DIMENSION; i++) {
    renderer = gtk_cell_renderer_spin_new();
    g_object_set((GObject *) renderer,
		 "xalign", 1.0,
		 "editable", TRUE,
		 "adjustment", gtk_adjustment_new(0,
						  -SPIN_ENTRY_MAX / 100.0,
						  SPIN_ENTRY_MAX / 100.0,
						  1,
						  10,
						  0),
		 "digits", 2,
		 NULL);

    g_signal_connect(renderer, "edited", G_CALLBACK(edited_func[i]), list);
    col = gtk_tree_view_column_new_with_attributes(title[i], renderer,
						   "text", i,
						   NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
    gtk_tree_view_column_set_cell_data_func(col,
					    renderer,
					    renderer_func,
					    GINT_TO_POINTER(i),
					    NULL);
    gtk_tree_view_column_set_expand(col, TRUE);
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif

  label = gtk_label_new_with_mnemonic(_("_Points:"));
#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_halign(label, GTK_ALIGN_START);
#else
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
#endif
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), tree_view);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width(GTK_CONTAINER(swin), 2);
  gtk_container_add(GTK_CONTAINER(swin), tree_view);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 2);

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif

  btn = gtk_button_new_from_stock(GTK_STOCK_ADD);
  g_signal_connect(btn, "clicked", G_CALLBACK(insert_column), tree_view);
  gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 4);

  btn = gtk_button_new_from_stock(GTK_STOCK_DELETE);
  g_signal_connect(btn, "clicked", G_CALLBACK(list_store_remove_selected_cb), tree_view);
  g_signal_connect(sel, "changed", G_CALLBACK(set_delete_button_sensitivity), btn);
  gtk_widget_set_sensitive(btn, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), btn, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


  d->points = tree_view;

  return vbox;
}

static void
style_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
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
draw_arrow_pixmap(GtkWidget *win, struct LegendDialog *d)
{
  int lw, len, x, w;
  cairo_t *cr;
  GdkWindow *window;

  window = gtk_widget_get_window(win);
  if (window == NULL) {
    return;
  }

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
#if GTK_CHECK_VERSION(3, 0, 0)
LegendArrowDialogPaint(GtkWidget *w, cairo_t *cr, gpointer client_data)
#else
LegendArrowDialogPaint(GtkWidget *w, GdkEvent *event, gpointer client_data)
#endif
{
  struct LegendDialog *d;
#if ! GTK_CHECK_VERSION(3, 0, 0)
  GdkWindow *win;
  cairo_t *cr;
#endif

  d = (struct LegendDialog *) client_data;

  if (d->arrow_pixmap == NULL) {
    draw_arrow_pixmap(w, d);
  }

  if (d->arrow_pixmap == NULL) {
    return;
  }

#if ! GTK_CHECK_VERSION(3, 0, 0)
  win = gtk_widget_get_window(w);
  if (win == NULL) {
    return;
  }

  cr = gdk_cairo_create(win);
#endif
  cairo_set_source_surface(cr, d->arrow_pixmap, 0, 0);
  cairo_paint(cr);
#if ! GTK_CHECK_VERSION(3, 0, 0)
  cairo_destroy(cr);
#endif
}

static void
LegendArrowDialogScaleW(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkWindow *win;
  cairo_t *cr;
#endif

  d = (struct LegendDialog *) client_data;
  d->wid = gtk_range_get_value(GTK_RANGE(w)) * 100;
  draw_arrow_pixmap(w, d);

#if GTK_CHECK_VERSION(3, 0, 0)
  win = gtk_widget_get_window(d->view);
  if (win == NULL) {
    return;
  }
  cr = gdk_cairo_create(win);
  LegendArrowDialogPaint(d->view, cr, d);
  cairo_destroy(cr);
#else
  LegendArrowDialogPaint(d->view, NULL, d);
#endif
}

static void
LegendArrowDialogScaleL(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkWindow *win;
  cairo_t *cr;
#endif

  d = (struct LegendDialog *) client_data;
  d->ang = gtk_range_get_value(GTK_RANGE(w));
  draw_arrow_pixmap(w, d);
#if GTK_CHECK_VERSION(3, 0, 0)
  win = gtk_widget_get_window(d->view);
  if (win == NULL) {
    return;
  }
  cr = gdk_cairo_create(win);
  LegendArrowDialogPaint(d->view, cr, d);
  cairo_destroy(cr);
#else
  LegendArrowDialogPaint(d->view, NULL, d);
#endif
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
LegendArrowDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *hbox2, *vbox2, *frame, *table;
  struct LegendDialog *d;
  char title[64];
  int i;

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend line %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

    w = points_setup(d);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), w);
#if GTK_CHECK_VERSION(3, 4, 0)
   gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
#else
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox2 = gtk_vbox_new(FALSE, 0);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox2 = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Type:"), FALSE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->path_type = w;

    w = combo_box_create();
    d->interpolation = w;
    add_widget_to_table(table, w, _("_Interpolation:"), FALSE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(legend_dialog_set_sensitive), d);

    gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Close path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->close_path = w;

    w = combo_box_create();
    d->arrow = w;
    add_widget_to_table(table, w, _("_Arrow:"), FALSE, i++);

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    miter_setup(d, table, i++);
    join_setup(d, table, i++);

    stroke_color_setup(d, table, i++);

    gtk_box_pack_start(GTK_BOX(hbox2), table, TRUE, TRUE, 0);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif
#if GTK_CHECK_VERSION(3, 2, 0)
    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 170, 1);
#else
    w = gtk_hscale_new_with_range(10, 170, 1);
#endif
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleL), d);
    g_signal_connect(w, "format-value", G_CALLBACK(format_value_degree), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->arrow_length = w;

    w = gtk_drawing_area_new();
    gtk_widget_set_size_request(w, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#if GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect(w, "draw", G_CALLBACK(LegendArrowDialogPaint), d);
#else
    g_signal_connect(w, "expose-event", G_CALLBACK(LegendArrowDialogPaint), d);
#endif
    d->view = w;

#if GTK_CHECK_VERSION(3, 2, 0)
    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100, 2000, 1);
#else
    w = gtk_hscale_new_with_range(100, 2000, 1);
#endif
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleW), d);
    g_signal_connect(w, "format-value", G_CALLBACK(format_value_percent), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->arrow_width = w;

    gtk_box_pack_start(GTK_BOX(hbox2), vbox, FALSE, FALSE, 0);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), hbox2);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = combo_box_create();
    d->fill_rule = w;
    add_widget_to_table(table, w, _("fill _Rule:"), FALSE, i++);

    fill_color_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox2), frame, TRUE, TRUE, 0);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox2);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

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
  GtkWidget *w, *hbox, *vbox, *frame, *table;
  struct LegendDialog *d;
  char title[64];
  int i;

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend rectangle %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

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
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    stroke_color_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    fill_color_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

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
  GtkWidget *w, *hbox, *vbox, *table, *frame;
  struct LegendDialog *d;
  char title[64];
  int i;

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend arc %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

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
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = create_direction_entry();
    add_widget_to_table(table, w, _("_Angle1:"), FALSE, i++);
    d->angle1 = w;

    w = create_direction_entry();
    add_widget_to_table(table, w, _("_Angle2:"), FALSE, i++);
    d->angle2 = w;

    w = gtk_check_button_new_with_mnemonic(_("_Pieslice"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->pieslice = w;

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
    vbox = gtk_vbox_new(FALSE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Close path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->close_path = w;

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    miter_setup(d, table, i++);
    join_setup(d, table, i++);
    stroke_color_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Stroke"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->stroke = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    fill_color_setup(d, table, i++);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    g_signal_connect(w, "toggled", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->fill = w;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_label_widget(GTK_FRAME(frame), w);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
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
LegendMarkDialogMark(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  button_set_mark_image(w, d->mark.Type);
}

static void
LegendMarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *frame, *table;
  struct LegendDialog *d;
  char title[64];
  int i;

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend mark %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    style_setup(d, table, i++);
    width_setup(d, table, i++);

    w = gtk_button_new();
    add_widget_to_table(table, w, _("_Mark:"), FALSE, i++);
    g_signal_connect(w, "clicked", G_CALLBACK(LegendMarkDialogMark), d);
    d->type = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Size:"), FALSE, i++);
    d->size = w;

    color_setup(d, table, i++);
    color2_setup(d, table, i++);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

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
legend_dialog_setup_sub(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w, *btn_box;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Pt:"), FALSE, i++);
  d->pt = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_SPACE_POINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Space:"), FALSE, i++);
  d->space = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_range(w, TEXT_OBJ_SCRIPT_SIZE_MIN, TEXT_OBJ_SCRIPT_SIZE_MAX);
  add_widget_to_table(table, w, _("_Script:"), FALSE, i++);
  d->script_size = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Font:"), FALSE, i++);
  d->font = w;

#if GTK_CHECK_VERSION(3, 2, 0)
  btn_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
  btn_box = gtk_hbutton_box_new();
#endif
  gtk_box_set_spacing(GTK_BOX(btn_box), 10);
  w = gtk_check_button_new_with_label("gtk-bold");
  gtk_button_set_use_stock(GTK_BUTTON(w), TRUE);
  d->font_bold = w;
  gtk_box_pack_start(GTK_BOX(btn_box), w, FALSE, FALSE, 0);

  w = gtk_check_button_new_with_label("gtk-italic");
  gtk_button_set_use_stock(GTK_BUTTON(w), TRUE);
  d->font_italic = w;
  gtk_box_pack_start(GTK_BOX(btn_box), w, FALSE, FALSE, 0);

  add_widget_to_table(table, btn_box, "", FALSE, i++);

  color_setup(d, table, i++);

  w = create_direction_entry();
  add_widget_to_table(table, w, _("_Direction:"), FALSE, i++);
  d->direction = w;

  w = gtk_check_button_new_with_mnemonic(_("_Raw"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->raw = w;
}

static void
insert_selcted_char(GtkIconView *icon_view, GtkTreePath *path, gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  char *ptr;
  GtkEditable *entry;
  int pos;

  model = gtk_icon_view_get_model(icon_view);
  if (! gtk_tree_model_get_iter(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, 0, &ptr, -1);

  entry = GTK_EDITABLE(user_data);
  pos = gtk_editable_get_position(entry);
  gtk_editable_insert_text(entry, ptr, -1, &pos);
  gtk_editable_set_position(entry, pos + 1);
  gtk_widget_grab_focus(GTK_WIDGET(user_data));
  gtk_editable_select_region(entry, pos, pos);

  g_free(ptr);
}

static GtkWidget *
create_character_view(GtkWidget *entry, gchar *data)
{
  GtkWidget *icon_view, *swin;
  GtkListStore *model;
  GtkTreeIter iter;
  gchar *ptr;
#if GTK_CHECK_VERSION(3, 0, 0)
  PangoLayout *layout;
  PangoRectangle ink_rect;
  int width = 0, w;
#endif

  model = gtk_list_store_new(1, G_TYPE_STRING);
  icon_view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(model));
  gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view), 0);
  gtk_icon_view_set_spacing(GTK_ICON_VIEW(icon_view), 0);
  gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(icon_view), 0);
  gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(icon_view), 0);
  gtk_icon_view_set_margin(GTK_ICON_VIEW(icon_view), 0);
#if GTK_CHECK_VERSION(2, 18, 0)
  gtk_icon_view_set_item_padding(GTK_ICON_VIEW(icon_view), 0);
#endif
  gtk_icon_view_set_columns(GTK_ICON_VIEW(icon_view), 24);
  g_signal_connect(icon_view, "item-activated", G_CALLBACK(insert_selcted_char), entry);

  for (ptr = data; *ptr; ptr = g_utf8_next_char(ptr)) {
    gunichar ch;
    gchar str[8];
    int l;

    gtk_list_store_append(model, &iter);
    ch = g_utf8_get_char(ptr);
    l = g_unichar_to_utf8(ch, str);
    str[l] = '\0';
    gtk_list_store_set(model, &iter, 0, str, -1);

#if GTK_CHECK_VERSION(3, 0, 0)
    /* fix-me: there exist extra spaces both side of strings when use GTK+3.0 */
    layout = gtk_widget_create_pango_layout(icon_view, str);
    pango_layout_get_pixel_extents(layout, &ink_rect, NULL);
    w = ink_rect.x + ink_rect.width;
    if (w > width) {
      width = w;
    }
    g_object_unref(layout);
#endif
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_icon_view_set_item_width(GTK_ICON_VIEW(icon_view), width * 1.5);
#endif

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), icon_view);

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
  GtkWidget *w, *hbox, *frame, *table;
  struct LegendDialog *d;
  char title[64];
  int i;

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend text %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_X:", FALSE, i++);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    add_widget_to_table(table, w, "_Y:", FALSE, i++);
    d->y = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);


#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    add_widget_to_table(table, w, _("_Text:"), TRUE, i++);
    d->text = w;

    legend_dialog_setup_sub(d, table, i++);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    w = create_character_panel(d->text);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 4);

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
  GtkWidget *table, *frame;

  d = (struct LegendDialog *) data;
  if (makewidget) {
    init_legend_dialog_widget_member(d);

#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif
    legend_dialog_setup_sub(d, table, 0);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 0);

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
CmLineDel(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("path")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmLineUpdate(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i, j;
  int *data, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("path")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendArrowDialog(&DlgLegendArrow, obj, data[i]);
      if (DialogExecute(TopLevel, &DlgLegendArrow) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++) {
	  data[j]--;
	}
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmRectDel(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("rectangle")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendRectCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmRectUpdate(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i, j;
  int *data, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("rectangle")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendRectCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendRectDialog(&DlgLegendRect, obj, data[i]);
      if (DialogExecute(TopLevel, &DlgLegendRect) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++) {
	  data[j]--;
	}
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmArcDel(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("arc")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmArcUpdate(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i, j;
  int *data, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("arc")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendArcDialog(&DlgLegendArc, obj, data[i]);
      if (DialogExecute(TopLevel, &DlgLegendArc) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++) {
	  data[j]--;
	}
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmMarkDel(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("mark")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendMarkCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmMarkUpdate(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i, j;
  int *data, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("mark")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendMarkCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendMarkDialog(&DlgLegendMark, obj, data[i]);
      if (DialogExecute(TopLevel, &DlgLegendMark) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++) {
	  data[j]--;
	}
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmTextDel(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("text")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendTextCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmTextUpdate(GtkAction *w, gpointer client_data)
{
  struct narray array;
  struct objlist *obj;
  int i, j;
  int *data, num;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("text")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendTextCB, &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendTextDialog(&DlgLegendText, obj, data[i]);
      if (DialogExecute(TopLevel, &DlgLegendText) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++) {
	  data[j]--;
	}
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmOptionTextDef(GtkAction *w, gpointer client_data)
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
    if (DialogExecute(TopLevel, &DlgLegendTextDef) == IDOK) {
      if (CheckIniFile()) {
	exeobj(obj, "save_config", id, 0, NULL);
      }
    }
    delobj(obj, id);
    UpdateAll2();
    if (! modified)
      reset_graph_modified();
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
ObjListUpdate(struct obj_list_data *d, int clear, list_sub_window_set_val_func func)
{
  if (Menulock || Globallock)
    return;

  if (d->text == NULL) {
    return;
  }

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, func);
  } else {
    list_sub_window_set(d, func);
  }

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), COL_ID, d->select);
  }
}

void
PathListUpdate(struct obj_list_data *d, int clear)
{
  ObjListUpdate(d, clear, path_list_set_val);
}

void
ArcListUpdate(struct obj_list_data *d, int clear)
{
  ObjListUpdate(d, clear, arc_list_set_val);
}

void
RectListUpdate(struct obj_list_data *d ,int clear)
{
  ObjListUpdate(d, clear, rect_list_set_val);
}

void
MarkListUpdate(struct obj_list_data *d, int clear)
{
  ObjListUpdate(d, clear, mark_list_set_val);
}

void
TextListUpdate(struct obj_list_data *d, int clear)
{
  ObjListUpdate(d, clear, text_list_set_val);
}

void
LegendWinUpdate(int clear)
{
  struct obj_list_data *d;
  if (Menulock || Globallock)
    return;

  for (d = NgraphApp.LegendWin.data.data; d; d = d->next) {
    d->update(d, clear);
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
  int ggc, fr, fg, fb, stroke, fill, close_path, pos[8], height = 20, lockstate, found, output, n;
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *pix;
#else
  GdkPixmap *pix;
#endif
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

#if GTK_CHECK_VERSION(3, 0, 0)
  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#else
  pix = gra2gdk_create_pixmap(local, gtk_widget_get_window(TopLevel),
			      width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#endif
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
    GRAlinestyle(ggc, n, style, 4, 0, 0, 1000);
    g_free(style);
  } else {
    GRAlinestyle(ggc, 0, NULL, 4, 0, 0, 1000);
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
	GRAdrawpoly(ggc, 4, pos, 0);
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
  if (local->linetonum && local->cairo) {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);
#else
  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, width, height);
  g_object_unref(G_OBJECT(pix));
#endif

  Globallock = lockstate;

  return pixbuf;
}

static void
path_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, n, x0, y0, w, i, interpolation, type;
  char *valstr;
  GdkPixbuf *pixbuf;

  for (i = 0; i < d->list_col_num; i++) {
    switch (i) {
    case PATH_LIST_COL_HIDDEN:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case PATH_LIST_COL_TYPE:
      getobj(d->obj, "type", row, 0, NULL, &type);
      getobj(d->obj, "interpolation", row, 0, NULL, &interpolation);

      if (type == 0) {
	char **enumlist;

	enumlist = (char **) chkobjarglist(d->obj, "type");
	list_store_set_string(GTK_WIDGET(d->text), iter, i, _(enumlist[0]));
      } else {
	char **enumlist;

	enumlist = (char **) chkobjarglist(d->obj, "interpolation");
	list_store_set_string(GTK_WIDGET(d->text), iter, i, _(enumlist[interpolation]));
      }
      break;
    case PATH_LIST_COL_ARROW:
      sgetobjfield(d->obj, row, d->list[i].name, NULL, &valstr, FALSE, FALSE, FALSE);
      list_store_set_string(GTK_WIDGET(d->text), iter, i, _(valstr));
      g_free(valstr);
      break;
    case PATH_LIST_COL_COLOR:
      pixbuf = draw_color_pixbuf(d->obj, row, OBJ_FIELD_COLOR_TYPE_STROKE, 40);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case PATH_LIST_COL_X:
      get_points(d->obj, row, &x0, &y0, &n);
      list_store_set_double(d->text, iter, PATH_LIST_COL_X, x0 / 100.0);
      list_store_set_double(d->text, iter, PATH_LIST_COL_Y, y0 / 100.0);
      list_store_set_int(d->text, iter, PATH_LIST_COL_POINTS, n);
      break;
    case PATH_LIST_COL_Y:
    case PATH_LIST_COL_POINTS:
      break;
    case PATH_LIST_COL_WIDTH:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &w);
      list_store_set_double(d->text, iter, i, w / 100.0);
      break;
    default:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_val(d->text, iter, i, d->list[i].type, &cx);
    }
  }
}

static void
rect_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int x1, x2, y1, y2, v, i;
  GdkPixbuf *pixbuf;

  for (i = 0; i < d->list_col_num; i++) {
    switch (i) {
    case RECT_LIST_COL_HIDDEN:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &v);
      v = ! v;
      list_store_set_boolean(d->text, iter, i, v);
      break;
    case RECT_LIST_COL_COLOR:
      pixbuf = draw_color_pixbuf(d->obj, row, OBJ_FIELD_COLOR_TYPE_STROKE, 40);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case RECT_LIST_COL_X:
      getobj(d->obj, "x1", row, 0, NULL, &x1);
      getobj(d->obj, "x2", row, 0, NULL, &x2);
      v = (x1 < x2) ? x1 : x2;
      list_store_set_double(d->text, iter, i, v / 100.0);
      break;
    case RECT_LIST_COL_Y:
      getobj(d->obj, "y1", row, 0, NULL, &y1);
      getobj(d->obj, "y2", row, 0, NULL, &y2);
      v = (y1 < y2) ? y1 : y2;
      list_store_set_double(d->text, iter, i, v / 100.0);
      break;
    case RECT_LIST_COL_WIDTH:
      v = abs(x1 - x2);
      list_store_set_double(d->text, iter, i, v / 100.0);
      break;
    case RECT_LIST_COL_HEIGHT:
      v = abs(y1 - y2);
      list_store_set_double(d->text, iter, i, v / 100.0);
      break;
    case RECT_LIST_COL_LWIDTH:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &v);
      list_store_set_double(d->text, iter, i, v / 100.0);
      break;
    default:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &v);
      list_store_set_val(d->text, iter, i, d->list[i].type, &v);
    }
  }
}

static void
arc_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, w, i;
  GdkPixbuf *pixbuf;

  for (i = 0; i < d->list_col_num; i++) {
    switch (i) {
    case ARC_LIST_COL_HIDDEN:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case ARC_LIST_COL_PIESLICE:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case ARC_LIST_COL_COLOR:
      pixbuf = draw_color_pixbuf(d->obj, row, OBJ_FIELD_COLOR_TYPE_STROKE, 40);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case ARC_LIST_COL_Y:
    case ARC_LIST_COL_X:
    case ARC_LIST_COL_RY:
    case ARC_LIST_COL_RX:
    case ARC_LIST_COL_ANGLE1:
    case ARC_LIST_COL_ANGLE2:
    case ARC_LIST_COL_WIDTH:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &w);
      list_store_set_double(d->text, iter, i, w / 100.0);
      break;
    default:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_val(d->text, iter, i, d->list[i].type, &cx);
    }
  }
}

static GdkPixbuf *
draw_mark_pixbuf(struct objlist *obj, int i)
{
  int ggc, fr, fg, fb, fr2, fg2, fb2,
    width = 20, height = 20, marktype,
    lockstate, found, output;
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *pix;
#else
  GdkPixmap *pix;
#endif
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

#if GTK_CHECK_VERSION(3, 0, 0)
  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#else
  pix = gra2gdk_create_pixmap(local, gtk_widget_get_window(TopLevel),
			      width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
#endif
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
  GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);

  getobj(obj, "type", i, 0, NULL, &marktype);
  GRAmark(ggc, marktype, height / 2, height / 2, height - 2,
	  fr, fg, fb, 255, fr2, fg2, fb2, 255);

  _GRAclose(ggc);
  if (local->linetonum && local->cairo) {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);
#else
  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, width, height);
  g_object_unref(G_OBJECT(pix));
#endif

  Globallock = lockstate;

  return pixbuf;
}

static void
mark_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, w, i;
  GdkPixbuf *pixbuf = NULL;

  for (i = 0; i < d->list_col_num; i++) {
    switch (i) {
    case MARK_LIST_COL_HIDDEN:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case MARK_LIST_COL_MARK:
      pixbuf = draw_mark_pixbuf(d->obj, row);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case MARK_LIST_COL_X:
    case MARK_LIST_COL_Y:
    case MARK_LIST_COL_WIDTH:
    case MARK_LIST_COL_SIZE:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &w);
      list_store_set_double(d->text, iter, i, w / 100.0);
      break;
    default:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_val(d->text, iter, i, d->list[i].type, &cx);
    }
  }
}

static void
text_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, w, i, style;
  char *str;
  GdkPixbuf *pixbuf;
#ifdef TEXT_LIST_USE_FONT_FAMILY
  struct fontmap *fmap;
#endif

  for (i = 0; i < d->list_col_num; i++) {
    switch (i) {
    case TEXT_LIST_COL_HIDDEN:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case TEXT_LIST_COL_RAW:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_boolean(d->text, iter, i, cx);
      break;
    case TEXT_LIST_COL_TEXT:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &str);
      list_store_set_string(d->text, iter, i, str);
      break;
    case TEXT_LIST_COL_FONT:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &str);
      list_store_set_string(d->text, iter, i, str);
#ifdef TEXT_LIST_USE_FONT_FAMILY
      if (str == NULL) {
	break;
      }
      fmap = gra2cairo_get_fontmap(str);
      if (fmap == NULL || fmap->fontname == NULL) {
	break;
      }
      list_store_set_string(d->text, iter, TEXT_LIST_COL_FONT_FAMILY, fmap->fontname);
#endif
      break;
    case TEXT_LIST_COL_STYLE:
      getobj(d->obj, "style", row, 0, NULL, &style);
      list_store_set_int(d->text, iter, i, (style & GRA_FONT_STYLE_ITALIC) ? PANGO_STYLE_ITALIC : 0);
      list_store_set_int(d->text, iter, TEXT_LIST_COL_WEIGHT, (style & GRA_FONT_STYLE_BOLD) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
      break;
    case TEXT_LIST_COL_COLOR:
      pixbuf = draw_color_pixbuf(d->obj, row, OBJ_FIELD_COLOR_TYPE_0, 20);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case TEXT_LIST_COL_X:
    case TEXT_LIST_COL_Y:
    case TEXT_LIST_COL_DIR:
    case TEXT_LIST_COL_PT:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &w);
      list_store_set_double(d->text, iter, i, w / 100.0);
      break;
    case TEXT_LIST_COL_WEIGHT:
#ifdef TEXT_LIST_USE_FONT_FAMILY
    case TEXT_LIST_COL_FONT_FAMILY:
#endif
      break;
    default:
      getobj(d->obj, d->list[i].name, row, 0, NULL, &cx);
      list_store_set_val(d->text, iter, i, d->list[i].type, &cx);
    }
  }
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  unsigned int i;
  int m, last_id;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  if (d->text == NULL) {
    return;
  }

  m = list_store_get_selected_int(d->text, COL_ID);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      gtk_widget_set_sensitive(d->popup_item[i], m > 0);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      last_id = -1;
      if (m >= 0) {
	last_id = chkobjlastinst(d->obj);
      }
      gtk_widget_set_sensitive(d->popup_item[i], m >= 0 && m < last_id);
      break;
    case POPUP_ITEM_HIDE:
      if (m >= 0) {
	int hidden;
	getobj(d->obj, "hidden", m, 0, NULL, &hidden);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(d->popup_item[i]), ! hidden);
      }
    default:
      gtk_widget_set_sensitive(d->popup_item[i], m >= 0);
    }
  }
}

enum CHANGE_DIR {
  CHANGE_DIR_X,
  CHANGE_DIR_Y,
};

static void
pos_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data, enum CHANGE_DIR dir)
{
  struct obj_list_data *d;
  int col, inc, ecode, m;
  double prev, val;
  GtkTreeModel *model;
  GtkTreeIter iter;
  int x, y, i;
  char *argv[3], *tmp, *ptr;


  menu_lock(FALSE);

  if (str == NULL || path == NULL)
    return;

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  d = (struct obj_list_data *) user_data;

  if (d->text == NULL) {
    return;
  }

  col = -1;
  for (i = 0; i < d->list_col_num; i++) {
    if (dir == CHANGE_DIR_X && strcmp(d->list[i].title, "x") == 0) {
      col = i;
      break;
    } else if (dir == CHANGE_DIR_Y && strcmp(d->list[i].title, "y") == 0) {
      col = i;
      break;
    }
  }
  if (col < 0) {
    return;
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->text));
  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, col, &tmp, COL_ID, &m, -1);
  if (tmp == NULL) {
    return;
  }

  prev = strtod(tmp, &ptr);
  if (prev != prev || prev == HUGE_VAL || prev == - HUGE_VAL || ptr[0] != '\0') {
    g_free(tmp);
    return;
  }
  g_free(tmp);

  inc = nround((val - prev) * 100);
  switch (dir) {
  case CHANGE_DIR_X:
    x = inc;
    y = 0;
    break;
  case CHANGE_DIR_Y:
    x = 0;
    y = inc;
    break;
  }
  argv[0] = (char *) &x;
  argv[1] = (char *) &y;
  argv[2] = NULL;

  if (inc != 0 ) {
    exeobj(d->obj, "move", m, 2, argv);
    set_graph_modified();
    d->update(d, TRUE);
  }

  return;
}

static void
rect_size_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data, char *pos1, char *pos2)
{
  struct obj_list_data *d;
  GtkTreeIter iter;
  int id, ecode;
  double val;
  int x1, x2, v, num;

  menu_lock(FALSE);

  if (str == NULL || path == NULL)
    return;

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  d = (struct obj_list_data *) user_data;

  id = tree_view_get_selected_row_int_from_path(d->text, path, &iter, COL_ID);
  num = chkobjlastinst(d->obj);
  if (id < 0 || id > num) {
    return;
  }

  getobj(d->obj, pos1, id, 0, NULL, &x1);
  getobj(d->obj, pos2, id, 0, NULL, &x2);

  if (x1 > x2) {
    v = x1;
    x1 = x2;
    x2 = v;
  }

  v = nround(val * 100);
  if (v != x2 - x1) {
    x2 = x1 + v;
    putobj(d->obj, pos1, id, &x1);
    putobj(d->obj, pos2, id, &x2);
    set_graph_modified();
    d->update(d, TRUE);
  }

  return;
}

static void
rect_width_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  rect_size_edited(cell_renderer, path, str, user_data, "x1", "x2");
}

static void
rect_height_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  rect_size_edited(cell_renderer, path, str, user_data, "y1", "y2");
}


static void
pos_x_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  pos_edited(cell_renderer, path, str, user_data, CHANGE_DIR_X);
}

static void
pos_y_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  pos_edited(cell_renderer, path, str, user_data, CHANGE_DIR_Y);
}

enum LEGEND_COMBO_ITEM {
  LEGEND_COMBO_ITEM_COLOR_0,
  LEGEND_COMBO_ITEM_COLOR_1,
  LEGEND_COMBO_ITEM_COLOR_2,
  LEGEND_COMBO_ITEM_COLOR_STROKE,
  LEGEND_COMBO_ITEM_COLOR_FILL,
  LEGEND_COMBO_ITEM_MARK,
  LEGEND_COMBO_ITEM_COLOR_TOGGLE_STROKE,
  LEGEND_COMBO_ITEM_COLOR_TOGGLE_FILL,
  LEGEND_COMBO_ITEM_CLOSE_PATH,
  LEGEND_COMBO_ITEM_STYLE,
  LEGEND_COMBO_ITEM_STYLE_BOLD,
  LEGEND_COMBO_ITEM_STYLE_ITALIC,
  LEGEND_COMBO_ITEM_NONE,
};

static void
create_mark_color_combo_box(GtkWidget *cbox, struct objlist *obj)
{
  int count;
  GtkTreeStore *list;
  GtkTreeIter iter;

  count = combo_box_get_num(cbox);
  if (count > 0)
    return;

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  gtk_tree_store_append(list, &iter, NULL);
  gtk_tree_store_set(list, &iter,
		     OBJECT_COLUMN_TYPE_STRING, _("Mark"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_NONE,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);
  combo_box_create_mark(cbox, &iter, LEGEND_COMBO_ITEM_MARK);

  gtk_tree_store_append(list, &iter, NULL);
  gtk_tree_store_set(list, &iter,
		     OBJECT_COLUMN_TYPE_STRING, _("Color 1"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_1,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);

  gtk_tree_store_append(list, &iter, NULL);
  gtk_tree_store_set(list, &iter,
		     OBJECT_COLUMN_TYPE_STRING, _("Color 2"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_2,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);
}

static void
create_color_combo_box(GtkWidget *cbox, struct objlist *obj, int id)
{
  int count, state, i;
  GtkTreeStore *list;
  GtkTreeIter iter, child;

  count = combo_box_get_num(cbox);
  if (count > 0)
    return;

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

  if (chkobjfield(obj, "stroke") < 0) {
      gtk_tree_store_append(list, &iter, NULL);
      gtk_tree_store_set(list, &iter,
			 OBJECT_COLUMN_TYPE_STRING, _("Color"),
			 OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			 OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_0,
			 OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
			 OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			 -1);
      if (chkobjfieldtype(obj, "style") == NINT) {
	int style;
	getobj(obj, "style", id, 0, NULL, &style);
	gtk_tree_store_append(list, &iter, NULL);
	gtk_tree_store_set(list, &iter,
			   OBJECT_COLUMN_TYPE_TOGGLE, style & GRA_FONT_STYLE_BOLD,
			   OBJECT_COLUMN_TYPE_STRING, _("Bold"),
			   OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			   OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_STYLE_BOLD,
			   OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
			   OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			   -1);

	gtk_tree_store_append(list, &iter, NULL);
	gtk_tree_store_set(list, &iter,
			   OBJECT_COLUMN_TYPE_TOGGLE, style & GRA_FONT_STYLE_ITALIC,
			   OBJECT_COLUMN_TYPE_STRING, _("Italic"),
			   OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			   OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_STYLE_ITALIC,
			   OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
			   OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			   -1);
      }
  } else {
    gtk_tree_store_append(list, &iter, NULL);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_STRING, _("Line style"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_NONE,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);
    for (i = 0; FwLineStyle[i].name; i++) {
      gtk_tree_store_append(list, &child, &iter);
      gtk_tree_store_set(list, &child,
			 OBJECT_COLUMN_TYPE_STRING, _(FwLineStyle[i].name),
			 OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			 OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_STYLE,
			 OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
			 OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			 -1);
    }

    getobj(obj, "stroke", id, 0, NULL, &state);
    gtk_tree_store_append(list, &iter, NULL);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_TOGGLE, state,
		       OBJECT_COLUMN_TYPE_STRING, _("Stroke"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_TOGGLE_STROKE,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);

    gtk_tree_store_append(list, &iter, NULL);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_STRING, _("Stroke color"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_STROKE,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		       -1);

    getobj(obj, "fill", id, 0, NULL, &state);
    gtk_tree_store_append(list, &iter, NULL);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_TOGGLE, state,
		       OBJECT_COLUMN_TYPE_STRING, _("Fill"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_TOGGLE_FILL,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);

    gtk_tree_store_append(list, &iter, NULL);
    gtk_tree_store_set(list, &iter,
		       OBJECT_COLUMN_TYPE_STRING, _("Fill color"),
		       OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		       OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_COLOR_FILL,
		       OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		       OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		       -1);

    if (chkobjfield(obj, "close_path") == 0) {
      getobj(obj, "close_path", id, 0, NULL, &state);
      gtk_tree_store_append(list, &iter, NULL);
      gtk_tree_store_set(list, &iter,
			 OBJECT_COLUMN_TYPE_TOGGLE, state,
			 OBJECT_COLUMN_TYPE_STRING, _("Close path"),
			 OBJECT_COLUMN_TYPE_PIXBUF, NULL,
			 OBJECT_COLUMN_TYPE_INT, LEGEND_COMBO_ITEM_CLOSE_PATH,
			 OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, TRUE,
			 OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
			 -1);
 
    }

  }
}

static void
select_type(GtkComboBox *w, gpointer user_data)
{
  int sel, col_type, mark_type, b, c, *ary, found, depth;
  struct obj_list_data *d;
  GtkTreeStore *list;
  GtkTreeIter iter;
  GtkTreePath *path;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0) {
    return;
  }

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_INT, &col_type, -1);
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);
  ary = gtk_tree_path_get_indices(path);
  depth = gtk_tree_path_get_depth(path);
  b = c = -1;

  switch (depth) {
  case 2:
    b = ary[1];
  case 1:
    break;
  default:
    return;
  }

  gtk_tree_path_free(path);

  switch (col_type) {
  case LEGEND_COMBO_ITEM_COLOR_1:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_1)) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_COLOR_2:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_2)) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_MARK:
    getobj(d->obj, "type", sel, 0, NULL, &mark_type);

    if (b == mark_type)
      return;

    putobj(d->obj, "type", sel, &b);
    break;
  default:
    return;
  }


  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
select_color(GtkComboBox *w, gpointer user_data)
{
  int sel, col_type, found, active, i, *indices, style;
  struct obj_list_data *d;
  GtkTreeStore *list;
  GtkTreeIter iter;
  GtkTreePath *path;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0) {
    return;
  }

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_INT, &col_type, -1);

  switch (col_type) {
  case LEGEND_COMBO_ITEM_COLOR_0:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_0)) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_COLOR_STROKE:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_STROKE)) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_COLOR_FILL:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_FILL)) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_COLOR_TOGGLE_STROKE:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    active = ! active;
    putobj(d->obj, "stroke", sel, &active);
    break;
  case LEGEND_COMBO_ITEM_COLOR_TOGGLE_FILL:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    active = ! active;
    putobj(d->obj, "fill", sel, &active);
    break;
  case LEGEND_COMBO_ITEM_CLOSE_PATH:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    active = ! active;
    putobj(d->obj, "close_path", sel, &active);
    break;
  case LEGEND_COMBO_ITEM_STYLE:
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);
    if (path == NULL) {
      return;
    }
    indices = gtk_tree_path_get_indices(path);
    if (indices == NULL) {
      gtk_tree_path_free(path);
    }
    i = indices[1];
    gtk_tree_path_free(path);
    if (chk_sputobjfield(d->obj, sel, "style", FwLineStyle[i].list) != 0) {
      return;
    }
    break;
  case LEGEND_COMBO_ITEM_STYLE_BOLD:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    getobj(d->obj, "style", sel, 0, NULL, &style);
    style = (style & GRA_FONT_STYLE_ITALIC) | (active ? 0 : GRA_FONT_STYLE_BOLD);
    putobj(d->obj, "style", sel, &style);
    break;
  case LEGEND_COMBO_ITEM_STYLE_ITALIC:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    getobj(d->obj, "style", sel, 0, NULL, &style);
    style = (style & GRA_FONT_STYLE_BOLD) | (active ? 0 : GRA_FONT_STYLE_ITALIC);
    putobj(d->obj, "style", sel, &style);
    break;
  default:
    return;
  }

  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

static int
start_editing_common(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  GtkTreeIter iter;
  struct obj_list_data *d;
  int sel;

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path_str, &iter, MARK_LIST_COL_ID);
  if (sel < 0) {
    return -1;
  }

  g_object_set_data(G_OBJECT(editable), "user-data", GINT_TO_POINTER(sel));

  return sel;
}

static void
start_editing_mark(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  if (start_editing_common(renderer, editable, path_str, user_data)) {
    return;
  }

  create_mark_color_combo_box(GTK_WIDGET(editable), d->obj);
  gtk_widget_show(GTK_WIDGET(editable));
  g_signal_connect(editable, "editing-done", G_CALLBACK(select_type), user_data);

  return;
}

static void
start_editing_color(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  struct obj_list_data *d;
  int sel;

  d = (struct obj_list_data *) user_data;

  sel = start_editing_common(renderer, editable, path_str, user_data);
  if (sel < 0) {
    return;
  }

  create_color_combo_box(GTK_WIDGET(editable), d->obj, sel);
  gtk_widget_show(GTK_WIDGET(editable));
  g_signal_connect(editable, "editing-done", G_CALLBACK(select_color), user_data);

 return;
}

static void
select_line_type(GtkComboBox *w, gpointer user_data)
{
  struct obj_list_data *d;
  int sel, type, interpolation, *indices, i, found;
  GtkTreeIter iter;
  GtkListStore *list;
  GtkTreePath *path;

  d = (struct obj_list_data *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0) {
    return;
  }

  list = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found) {
    return;
  }

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);
  if (path == NULL) {
    return;
  }

  indices = gtk_tree_path_get_indices(path);
  if (indices == NULL) {
    gtk_tree_path_free(path);
    return;
  }

  i = indices[0];
  gtk_tree_path_free(path);

  if (i < 0) {
    return;
  }

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0) {
    return;
  }

  getobj(d->obj, "type", sel, 0, NULL, &type);
  getobj(d->obj, "interpolation", sel, 0, NULL, &interpolation);

  if ((type == 0 && i == 0) ||
      (type != 0 && interpolation == i - 1)) {
    return;
  }

  if (i == 0) {
    putobj(d->obj, "type", sel, &i);
  } else {
    i--;
    putobj(d->obj, "interpolation", sel, &i);
    i = 1;
    putobj(d->obj, "type", sel, &i);
  }

  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

static void
start_editing_line_type(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  struct obj_list_data *d;
  int sel, count;

  d = (struct obj_list_data *) user_data;

  sel = start_editing_common(renderer, editable, path_str, user_data);
  if (sel < 0) {
    return;
  }

  g_object_set_data(G_OBJECT(editable), "user-data", GINT_TO_POINTER(sel));

  count = combo_box_get_num(GTK_WIDGET(editable));
  if (count == 0) {
    int i;
    char **enumlist;

    enumlist = (char **) chkobjarglist(d->obj, "type");
    combo_box_append_text(GTK_WIDGET(editable), _(enumlist[0]));

    enumlist = (char **) chkobjarglist(d->obj, "interpolation");
    for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
      combo_box_append_text(GTK_WIDGET(editable), _(enumlist[i]));
    }
    gtk_widget_show(GTK_WIDGET(editable));
  }

  g_signal_connect(editable, "editing-done", G_CALLBACK(select_line_type), user_data);

  return;
}

static void
edited_line_type(GtkCellRenderer *cell_renderer, gchar *path_str, gchar *str, gpointer user_data)
{
  menu_lock(FALSE);
}

static void
start_editing_font(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  GtkTreeIter iter;
  struct obj_list_data *d;
  GtkComboBox *cbox;
  int sel;

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path, &iter, TEXT_LIST_COL_ID);
  if (sel < 0) {
    return;
  }

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(renderer), "user-data", GINT_TO_POINTER(sel));

  SetFontListFromObj(GTK_WIDGET(cbox), d->obj, sel, "font");
}

static void
edited_font(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data)
{
  struct obj_list_data *d;
  int sel;
  char *ptr;

  menu_lock(FALSE);

  if (str == NULL) {
    return;
  }

  d = (struct obj_list_data *) user_data;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell_renderer), "user-data"));
  if (sel < 0) {
    return;
  }

  getobj(d->obj, "font", sel, 0, NULL, &ptr);
  if (g_strcmp0(str, ptr) == 0) {
    return;
  }

  ptr = g_strdup(str);
  if (ptr == NULL) {
    return;
  }
  putobj(d->obj, "font", sel, ptr);

  d->select = sel;
  d->update(d, FALSE);
  set_graph_modified();
}

void
CmLegendWindow(GtkToggleAction *action, gpointer client_data)
{
  int i, state;
  struct SubWin *d;
  struct obj_list_data *data;
  GList *list;
  GtkTreeViewColumn *col;
  GtkWidget *icons[LEGENDNUM];
  char *icon_files[] = {
    "ngraph_line.png",
    "ngraph_rect.png",
    "ngraph_arc.png",
    "ngraph_mark.png",
    "ngraph_text.png",
  };
  void (* UpdateFuncArray[]) (struct obj_list_data *, int) = {
    PathListUpdate,
    RectListUpdate,
    ArcListUpdate,
    MarkListUpdate,
    TextListUpdate,
  };
  void (* UpdateDialogFuncArray[]) (struct obj_list_data *, int, int) = {
    LegendWinPathUpdate,
    LegendWinRectUpdate,
    LegendWinArcUpdate,
    LegendWinMarkUpdate,
    LegendWinTextUpdate,
  };

  d = &(NgraphApp.LegendWin);

  if (action) {
    state = gtk_toggle_action_get_active(action);
  } else {
    state = TRUE;
  }

  if (d->Win) {
    sub_window_set_visibility((struct SubWin *)d, state);
    return;
  }

  if (! state) {
    return;
  }

  for (i = 0; i < LEGENDNUM; i++) {
    char *str;

    str = g_strdup_printf("%s%c%s", PIXMAPDIR, DIRSEP, icon_files[i]);
    icons[i] = gtk_image_new_from_file(str);
    g_free(str);
  }

  tree_sub_window_create(d, "Legend Window", LEGENDNUM, Llist_num, Llist, icons, Legendwin_xpm, Legendwin48_xpm);

  data = d->data.data;
  for (i = 0; i < LEGENDNUM; i++) {
    data->update = UpdateFuncArray[i];
    data->dialog = Ldlg[i];
    data->setup_dialog = UpdateDialogFuncArray[i];
    data->ev_key = NULL;
    data->obj = chkobject(legendlist[i]);

    sub_win_create_popup_menu(data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
    switch (i) {
    case LegendTypePath:
      set_combo_cell_renderer_cb(data, PATH_LIST_COL_TYPE, Llist[i], G_CALLBACK(start_editing_line_type), G_CALLBACK(edited_line_type));
      set_editable_cell_renderer_cb(data, PATH_LIST_COL_X, Llist[i], G_CALLBACK(pos_x_edited));
      set_editable_cell_renderer_cb(data, PATH_LIST_COL_Y, Llist[i], G_CALLBACK(pos_y_edited));
      set_obj_cell_renderer_cb(data, PATH_LIST_COL_COLOR, Llist[i], G_CALLBACK(start_editing_color));
      break;
    case LegendTypeRect:
      set_editable_cell_renderer_cb(data, RECT_LIST_COL_X, Llist[i], G_CALLBACK(pos_x_edited));
      set_editable_cell_renderer_cb(data, RECT_LIST_COL_Y, Llist[i], G_CALLBACK(pos_y_edited));
      set_editable_cell_renderer_cb(data, RECT_LIST_COL_WIDTH, Llist[i], G_CALLBACK(rect_width_edited));
      set_editable_cell_renderer_cb(data, RECT_LIST_COL_HEIGHT, Llist[i], G_CALLBACK(rect_height_edited));
      set_obj_cell_renderer_cb(data, RECT_LIST_COL_COLOR, Llist[i], G_CALLBACK(start_editing_color));
      break;
    case LegendTypeArc:
      set_obj_cell_renderer_cb(data, ARC_LIST_COL_COLOR, Llist[i], G_CALLBACK(start_editing_color));
      break;
    case LegendTypeMark:
      set_obj_cell_renderer_cb(data, MARK_LIST_COL_MARK, Llist[i], G_CALLBACK(start_editing_mark));
      break;
    case LegendTypeText:
      set_obj_cell_renderer_cb(data, TEXT_LIST_COL_COLOR, Llist[i], G_CALLBACK(start_editing_color));
      set_combo_cell_renderer_cb(data, TEXT_LIST_COL_FONT, Llist[i], G_CALLBACK(start_editing_font), G_CALLBACK(edited_font));
      col = gtk_tree_view_get_column(GTK_TREE_VIEW(data->text), TEXT_LIST_COL_TEXT);
      list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(col));
      if (list == NULL) {
	break;
      }
      if (list->data) {
	GtkCellRenderer *renderer;
	renderer = list->data;
	gtk_tree_view_column_add_attribute(col, renderer, "style", TEXT_LIST_COL_STYLE);
	gtk_tree_view_column_add_attribute(col, renderer, "weight", TEXT_LIST_COL_WEIGHT);
#ifdef TEXT_LIST_USE_FONT_FAMILY
	gtk_tree_view_column_add_attribute(col, renderer, "family", TEXT_LIST_COL_FONT_FAMILY);
#endif
      }
      g_list_free(list);
      break;
    }
    data = data->next;
  }
  sub_window_show_all((struct SubWin *) d);
  sub_window_set_geometry((struct SubWin *)d, TRUE);

  LegendWinUpdate(TRUE);
}
