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

#include "strconv.h"

#include "ngraph.h"
#include "object.h"
#include "gra.h"
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

static n_list_store Llist[] = {
  {"",             G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden",   FALSE},
  {"#",            G_TYPE_INT,     TRUE, FALSE, "id",       FALSE},
  //  {N_("object"),   G_TYPE_STRING,  TRUE, FALSE, "object",   FALSE},
  {N_("object/property"), G_TYPE_STRING,  TRUE, FALSE, "property", TRUE, 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
  {"x",            G_TYPE_DOUBLE,  TRUE, TRUE,  "x",        FALSE, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {"y",            G_TYPE_DOUBLE,  TRUE, TRUE,  "y",        FALSE, - SPIN_ENTRY_MAX, SPIN_ENTRY_MAX, 100, 1000},
  {N_("lw/size"),  G_TYPE_DOUBLE,  TRUE, TRUE,  "width",    FALSE,                0, SPIN_ENTRY_MAX,  20,  100},
  {"^#",           G_TYPE_INT,     TRUE, FALSE, "oid",      FALSE},
};

#define LEGEND_WIN_COL_NUM (sizeof(Llist)/sizeof(*Llist))
#define LEGEND_WIN_COL_OID (LEGEND_WIN_COL_NUM - 1)
#define LEGEND_WIN_COL_HIDDEN 0
#define LEGEND_WIN_COL_ID     1
#define LEGEND_WIN_COL_PROP   2
#define LEGEND_WIN_COL_X      3
#define LEGEND_WIN_COL_Y      4
#define LEGEND_WIN_COL_WIDTH  5

static struct subwin_popup_list Popup_list[] = {
  {N_("_Duplicate"),      G_CALLBACK(tree_sub_window_copy), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  //  {N_("duplicate _Behind"),   G_CALLBACK(tree_sub_window_copy), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_DELETE,      G_CALLBACK(tree_sub_window_delete), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Focus"),          G_CALLBACK(tree_sub_window_focus), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Show"),           G_CALLBACK(tree_sub_window_hide), FALSE, NULL, POP_UP_MENU_ITEM_TYPE_CHECK},
  {GTK_STOCK_PROPERTIES,  G_CALLBACK(tree_sub_window_update), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, 0, NULL, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(tree_sub_window_move_top), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(tree_sub_window_move_up), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(tree_sub_window_move_down), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(tree_sub_window_move_last), TRUE, NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))
#define POPUP_ITEM_HIDE 4
#define POPUP_ITEM_TOP 7
#define POPUP_ITEM_UP 8
#define POPUP_ITEM_DOWN 9
#define POPUP_ITEM_BOTTOM 10

static gboolean LegendWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data);
static void LegendDialogCopy(struct LegendDialog *d);

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

static int ExpandRow[LEGENDNUM] = {0};

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
  d->tab = NULL;
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
      GtkWidget *widget;

      widget = get_widget(d->text);
      str = gtk_entry_get_text(GTK_ENTRY(widget));
      if (str && strncmp(str, buf, sizeof(buf) - 1)) {
	tmp = g_strdup_printf("%s%s", buf, str);
	gtk_entry_set_text(GTK_ENTRY(widget), tmp);
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
set_sensitive_with_label(GtkWidget *w, int a)
{
  GtkWidget *widget;

  gtk_widget_set_sensitive(w, a);
  widget = get_widget(w);

  if (w != widget) {
    gtk_widget_set_sensitive(widget, a);
  }
}

static void
legend_dialog_set_sensitive(GtkWidget *w, gpointer client_data)
{
  GtkWidget *widget;
  struct LegendDialog *d;
  int path_type;

  d = (struct LegendDialog *) client_data;

  path_type = PATH_TYPE_LINE;
  if (d->path_type && d->interpolation) {
    widget = get_widget(d->path_type);
    path_type = combo_box_get_active(widget);

    set_sensitive_with_label(d->interpolation, path_type == PATH_TYPE_CURVE);
  }

  if (d->stroke && d->stroke_color && d->style && d->width) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    set_sensitive_with_label(d->stroke_color, a);
    set_sensitive_with_label(d->style, a);
    set_sensitive_with_label(d->width, a);
  }

  if (d->stroke &&
      d->miter &&
      d->join &&
      d->close_path) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    set_sensitive_with_label(d->miter, a);
    set_sensitive_with_label(d->join, a);
    set_sensitive_with_label(d->close_path, a);
  }

  if (d->stroke &&
      d->interpolation &&
      d->close_path &&
      d->arrow &&
      d->arrow_length &&
      d->arrow_width) {
    int a, ca;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->stroke));
    ca = combo_box_get_active(get_widget(d->interpolation));

    set_sensitive_with_label(d->miter, a);
    set_sensitive_with_label(d->join, a);
    set_sensitive_with_label(d->arrow, a);
    set_sensitive_with_label(d->arrow_length, a);
    set_sensitive_with_label(d->arrow_width, a);

    if (path_type == PATH_TYPE_CURVE) {
      set_sensitive_with_label(d->close_path, a &&
			       (ca != INTERPOLATION_TYPE_SPLINE_CLOSE &&
				ca != INTERPOLATION_TYPE_BSPLINE_CLOSE));
    } else {
      set_sensitive_with_label(d->close_path, a);
    }
  }

  if (d->fill && d->fill_color) {
    int a;

    a = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->fill));
    set_sensitive_with_label(d->fill_color, a);

    if (d->fill_rule) {
      set_sensitive_with_label(d->fill_rule, a);
    }
  }
}

static void
legend_dialog_setup_item(GtkWidget *w, struct LegendDialog *d, int id)
{
  unsigned int i;
  int x1, y1, x2, y2;
  GtkWidget *widget;
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

    widget = get_widget(d->arrow_width);
    gtk_range_set_value(GTK_RANGE(widget), d->wid / 100);

    widget = get_widget(d->arrow_length);
    gtk_range_set_value(GTK_RANGE(widget), d->ang);
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
    widget = get_widget(d->text);
    gtk_entry_set_text(GTK_ENTRY(widget), buf);
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

  if (d->tab) {
    d->tab_active = gtk_notebook_get_current_page(GTK_NOTEBOOK(d->tab));
  }

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
  d->width = add_widget_to_table(table, w, _("_Line width:"), FALSE, i++);
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
  }

  vbox = gtk_vbox_new(FALSE, 4);

  label = gtk_label_new_with_mnemonic(_("_Points:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), tree_view);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width(GTK_CONTAINER(swin), 2);
  gtk_container_add(GTK_CONTAINER(swin), tree_view);
  gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 2);

  hbox = gtk_hbox_new(FALSE, 4);

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
  d->style = add_widget_to_table(table, w, _("Line _Style:"), TRUE, i);
}

static void
miter_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  d->miter = add_widget_to_table(table, w, _("_Miter:"), FALSE, i++);
}

static void
join_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = combo_box_create();
  d->join = add_widget_to_table(table, w, _("_Join:"), FALSE, i++);
}

static void
color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color = add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
color2_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color2 = add_widget_to_table(table, w, _("_Color2:"), FALSE, i);
}

static void
fill_color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->fill_color = add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
stroke_color_setup(struct LegendDialog *d, GtkWidget *table, int i)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->stroke_color = add_widget_to_table(table, w, _("_Color:"), FALSE, i);
}

static void
draw_arrow_pixmap(GtkWidget *win, struct LegendDialog *d)
{
  int lw, len, x, w;
  static GdkPixmap *pixmap = NULL;
  static cairo_t *cr;
  GdkWindow *window;

  window = GTK_WIDGET_GET_WINDOW(win);
  if (window == NULL) {
    return;
  }

  if (pixmap == NULL) {
    pixmap = gdk_pixmap_new(window, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE, -1);
    cr = gdk_cairo_create(pixmap);
  }

  cairo_set_source_rgb(cr, Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  cairo_rectangle(cr,
		  CAIRO_COORDINATE_OFFSET, CAIRO_COORDINATE_OFFSET,
		  ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
  cairo_fill(cr);

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

  d->arrow_pixmap = pixmap;
}

static void
LegendArrowDialogPaint(GtkWidget *w, GdkEvent *event, gpointer client_data)
{
  struct LegendDialog *d;
  GdkWindow *win;
  cairo_t *cr;

  d = (struct LegendDialog *) client_data;

  if (d->arrow_pixmap == NULL) {
    draw_arrow_pixmap(w, d);
  }

  if (d->arrow_pixmap == NULL) {
    return;
  }

  win = GTK_WIDGET_GET_WINDOW(w);
  if (win == NULL) {
    return;
  }

  cr = gdk_cairo_create(win);
  gdk_cairo_set_source_pixmap(cr, d->arrow_pixmap, 0, 0);
  cairo_rectangle(cr,
		  CAIRO_COORDINATE_OFFSET, CAIRO_COORDINATE_OFFSET,
		  ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
  cairo_fill(cr);
  cairo_destroy(cr);
}

static void
LegendArrowDialogScaleW(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;
  d->wid = gtk_range_get_value(GTK_RANGE(w)) * 100;
  draw_arrow_pixmap(w, d);
  LegendArrowDialogPaint(d->view, NULL, d);
}

static void
LegendArrowDialogScaleL(GtkWidget *w, gpointer client_data)
{
  struct LegendDialog *d;

  d = (struct LegendDialog *) client_data;
  d->ang = gtk_range_get_value(GTK_RANGE(w));
  draw_arrow_pixmap(w, d);
  LegendArrowDialogPaint(d->view, NULL, d);
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

    hbox = gtk_hbox_new(FALSE, 4);

    w = points_setup(d);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), w);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new(FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 4);
    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = combo_box_create();
    add_widget_to_table(table, w, _("_Type:"), FALSE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(legend_dialog_set_sensitive), d);
    d->path_type = w;

    w = combo_box_create();
    d->interpolation = add_widget_to_table(table, w, _("_Interpolation:"), FALSE, i++);
    g_signal_connect(w, "changed", G_CALLBACK(legend_dialog_set_sensitive), d);

    gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = gtk_check_button_new_with_mnemonic(_("_Close path"));
    add_widget_to_table(table, w, NULL, FALSE, i++);
    d->close_path = w;

    w = combo_box_create();
    d->arrow = add_widget_to_table(table, w, _("_Arrow:"), FALSE, i++);

    style_setup(d, table, i++);
    width_setup(d, table, i++);
    miter_setup(d, table, i++);
    join_setup(d, table, i++);

    stroke_color_setup(d, table, i++);

    gtk_box_pack_start(GTK_BOX(hbox2), table, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(10, 170, 1);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleL), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->arrow_length = w;

    d->arrow_pixmap = NULL;
    w = gtk_drawing_area_new();
    gtk_widget_set_size_request(w, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "expose-event", G_CALLBACK(LegendArrowDialogPaint), d);
    d->view = w;

    w = gtk_hscale_new_with_range(100, 2000, 1);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendArrowDialogScaleW), d);
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

    table = gtk_table_new(1, 2, FALSE);

    i = 0;
    w = combo_box_create();
    d->fill_rule = add_widget_to_table(table, w, _("fill _Rule:"), FALSE, i++);

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

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

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

    vbox = gtk_vbox_new(FALSE, 0);
    table = gtk_table_new(1, 2, FALSE);

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

    table = gtk_table_new(1, 2, FALSE);

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
    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    init_legend_dialog_widget_member(d);

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

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


    table = gtk_table_new(1, 2, FALSE);

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

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);


    table = gtk_table_new(1, 2, FALSE);

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


    table = gtk_table_new(1, 2, FALSE);

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

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

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


    table = gtk_table_new(1, 2, FALSE);

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

  btn_box = gtk_hbutton_box_new();
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
  }

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
text_dialog_show_tab(GtkWidget *w, gpointer user_data)
{
  struct LegendDialog *d;
  d = (struct LegendDialog *) user_data;
  gtk_notebook_set_current_page(GTK_NOTEBOOK(d->tab), d->tab_active);
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

    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

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


    table = gtk_table_new(1, 2, FALSE);

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
    g_signal_connect(w, "show", G_CALLBACK(text_dialog_show_tab), d);
    d->tab = w;
    d->tab_active = 0;

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

    table = gtk_table_new(1, 2, FALSE);
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
CmLineDel(void)
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
CmLineUpdate(void)
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
CmLineMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmLineUpdate();
    break;
  case MenuIdLegendDel:
    CmLineDel();
    break;
  }
}

void
CmRectDel(void)
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
CmRectUpdate(void)
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
CmRectangleMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmRectUpdate();
    break;
  case MenuIdLegendDel:
    CmRectDel();
    break;
  }
}

void
CmArcDel(void)
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
CmArcUpdate(void)
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
CmArcMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmArcUpdate();
    break;
  case MenuIdLegendDel:
    CmArcDel();
    break;
  }
}

void
CmMarkDel(void)
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
CmMarkUpdate(void)
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
CmMarkMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmMarkUpdate();
    break;
  case MenuIdLegendDel:
    CmMarkDel();
    break;
  }
}

void
CmTextDel(void)
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
CmTextUpdate(void)
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
CmTextMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmTextUpdate();
    break;
  case MenuIdLegendDel:
    CmTextDel();
    break;
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
LegendWinLegendUpdate(void *data, struct objlist *obj, int id, int sub_id)
{
  int num;
  int update, ret;
  struct LegendWin *d;
  struct LegendDialog *lgd;

  d = &(NgraphApp.LegendWin);

  if (Menulock || Globallock)
    return;

  UnFocus();

  num = 0;
  update = FALSE;

  if (id < 0 || sub_id > d->legend[id])
    return;

  lgd = Ldlg[id];

  ret = IDCANCEL;
  lgd->Obj = obj;
  lgd->Id = sub_id;
  switch (id) {
  case LegendTypePath:
    lgd->SetupWindow = LegendArrowDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendArrowDialog(&DlgLegendArrow, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendArrow);
    break;
  case LegendTypeRect:
    lgd->SetupWindow = LegendRectDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendRectDialog(&DlgLegendRect, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendRect);
    break;
  case LegendTypeArc:
    lgd->SetupWindow = LegendArcDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendArcDialog(&DlgLegendArc, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendArc);
    break;
  case LegendTypeMark:
    lgd->SetupWindow = LegendMarkDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendMarkDialog(&DlgLegendMark, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendMark);
    break;
  case LegendTypeText:
    lgd->SetupWindow = LegendTextDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendTextDialog(&DlgLegendText, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendText);
    break; 
 }
  if (ret == IDDELETE) {
    delobj(d->obj[id], sub_id);
    set_graph_modified();
    update = TRUE;
    d->select = -1;
    d->legend_type = -1;
  } else if (ret == IDOK) {
    update = TRUE;
  }

  if (update)
    LegendWinUpdate(FALSE);
}

void
LegendWinUpdate(int clear)
{
  int i;
  struct LegendWin *d;

  d = &(NgraphApp.LegendWin);
  for (i = 0; i < LEGENDNUM; i++)
    d->legend[i] = chkobjlastinst(d->obj[i]);

  LegendWinExpose(NULL, NULL, NULL);

  if (! clear && d->select >= 0 && d->legend_type >= 0) {
    tree_store_select_nth(GTK_WIDGET(d->text), d->legend_type, d->select);
  }
}

static int
legend_list_must_rebuild(struct LegendWin *d)
{
  int n, k, r = FALSE;
  GtkTreeIter iter;
  gboolean state;
  GtkTreePath *path;

  state = tree_store_get_iter_first(GTK_WIDGET(d->text), &iter);

  if (! state)
    return TRUE;

  for (k = 0; k < LEGENDNUM; k++) {
    n = tree_store_get_child_num(GTK_WIDGET(d->text), &iter);
    path = gtk_tree_path_new_from_indices(k, -1);
    ExpandRow[k] = gtk_tree_view_row_expanded(GTK_TREE_VIEW(d->text), path);
    if (n != d->legend[k] + 1) {
      ExpandRow[k] = r = TRUE;
    }
    gtk_tree_path_free(path);
    tree_store_iter_next(GTK_WIDGET(d->text), &iter);
  }
  return r;
}

static void
get_points(char *buf, int len, struct objlist *obj, int id, int *x, int *y, int style, char *ex)
{
  int *points;
  const char *str;
  struct narray *array;

  getobj(obj, "points", id, 0, NULL, &array);
  points = arraydata(array);
  if (arraynum(array) < 2) {
    *x = 0;
    *y = 0;
  } else {
    *x = points[0];
    *y = points[1];
  }

  str = get_style_string(obj, id, "style");

  snprintf(buf, len, _("points:%-3d %s%s%s%s"),
	   arraynum(array) / 2,
	   (style) ? _("style:") : "",
	   (style) ? ((str) ? _(str) : _("custom")) : "",
	   (style) ? "  " : "",
	   CHK_STR(ex));
}

enum COLOR_TYPE {
  COLOR_TYPE_1,
  COLOR_TYPE_2,
  COLOR_TYPE_FILL,
  COLOR_TYPE_STROKE,
};

static void
legend_list_set_color(struct LegendWin *d, GtkTreeIter *iter, int type, int row, int color_type)
{
  int r, g, b;
  char color[256], *rc, *gc, *bc;

  switch (color_type) {
  case COLOR_TYPE_STROKE:
    rc = "stroke_R";
    gc = "stroke_G";
    bc = "stroke_B";
    break;
  case COLOR_TYPE_FILL:
    rc = "fill_R";
    gc = "fill_G";
    bc = "fill_B";
    break;
  case COLOR_TYPE_2:
    rc = "R2";
    gc = "G2";
    bc = "B2";
    break;
  default:
    rc = "R";
    gc = "G";
    bc = "B";
  }

  getobj(d->obj[type], rc, row, 0, NULL, &r);
  getobj(d->obj[type], gc, row, 0, NULL, &g);
  getobj(d->obj[type], bc, row, 0, NULL, &b);
  snprintf(color, sizeof(color), "#%02x%02x%02x", r & 0xff, g & 0xff, b &0xff);
  tree_store_set_string(GTK_WIDGET(d->text), iter, LEGEND_WIN_COL_NUM, color);
}

static void
legend_list_set_property(struct LegendWin *d, GtkTreeIter *iter, int type, int row, unsigned int i, int *x0, int *y0)
{
  int x2, y2, mark, path_type, stroke, fill, len, intp;
  char *valstr, *text, buf[256], buf2[256];
  char **enum_intp, **enum_path_type;
  const char *str;

  switch (type) {
  case LegendTypePath:
    getobj(d->obj[type], "type", row, 0, NULL, &path_type);
    getobj(d->obj[type], "fill", row, 0, NULL, &fill);
    getobj(d->obj[type], "stroke", row, 0, NULL, &stroke);
    sgetobjfield(d->obj[type], row, "arrow", NULL, &valstr, FALSE, FALSE, FALSE);
    getobj(d->obj[type], "interpolation", row, 0, NULL, &intp);
    enum_intp = (char **) chkobjarglist(d->obj[type], "interpolation");
    enum_path_type = (char **) chkobjarglist(d->obj[type], "type");

    len = snprintf(buf, sizeof(buf), "%s ", _(enum_path_type[path_type]));
    snprintf(buf2, sizeof(buf2), "%s%s%s%s%s%s", 
	     (path_type) ? _(enum_intp[intp]) : "",
	     (path_type) ? " " : "",
	     (fill) ? _("fill") : "",
	     (fill) ? " " : "",
	     (stroke) ? _("arrow:") : "",
	     (stroke) ? _(valstr) : "");
    g_free(valstr);
    get_points(buf + len, sizeof(buf) - len, d->obj[type], row, x0, y0, stroke, buf2);
    legend_list_set_color(d, iter, type, row, (fill) ? COLOR_TYPE_FILL : COLOR_TYPE_STROKE);
    break;
  case LegendTypeRect:
    getobj(d->obj[type], "fill", row, 0, NULL, &fill);
    getobj(d->obj[type], "stroke", row, 0, NULL, &stroke);
    str = get_style_string(d->obj[type], row, "style");
    getobj(d->obj[type], "x1", row, 0, NULL, x0);
    getobj(d->obj[type], "y1", row, 0, NULL, y0);
    getobj(d->obj[type], "x2", row, 0, NULL, &x2);
    getobj(d->obj[type], "y2", row, 0, NULL, &y2);
    snprintf(buf, sizeof(buf), _("w:%.2f h:%.2f%s%s%s"),
	     abs(*x0 - x2) / 100.0,
	     abs(*y0 - y2) / 100.0,
	     (stroke) ? _("  style:") : "",
	     (stroke) ? ((str) ? _(str) : _("custom")) : "",
	     (fill)  ? _("  fill") : ""
	     );
    legend_list_set_color(d, iter, type, row, (fill) ? COLOR_TYPE_FILL : COLOR_TYPE_STROKE);
    break;
  case LegendTypeArc:
    getobj(d->obj[type], "fill", row, 0, NULL, &fill);
    getobj(d->obj[type], "stroke", row, 0, NULL, &stroke);
    str = get_style_string(d->obj[type], row, "style");
    getobj(d->obj[type], "x", row, 0, NULL, x0);
    getobj(d->obj[type], "y", row, 0, NULL, y0);
    getobj(d->obj[type], "rx", row, 0, NULL, &x2);
    getobj(d->obj[type], "ry", row, 0, NULL, &y2);
    snprintf(buf, sizeof(buf), "rx:%.2f ry:%.2f%s%s%s",
	     x2 / 100.0,
	     y2 / 100.0,
	     (stroke) ? _("  style:") : "",
	     (stroke) ? ((str) ? _(str) : _("custom")) : "",
	     (fill) ? _("  fill") : "");
    legend_list_set_color(d, iter, type, row, (fill) ? COLOR_TYPE_FILL : COLOR_TYPE_STROKE);
    break;
  case LegendTypeMark:
    getobj(d->obj[type], "x", row, 0, NULL, x0);
    getobj(d->obj[type], "y", row, 0, NULL, y0);
    getobj(d->obj[type], "type", row, 0, NULL, &mark);
    if (mark >= 0 && mark < MarkCharNum) {
      char *mc = MarkChar[mark];
      snprintf(buf, sizeof(buf), _("%s%stype:%-2d"), mc, (mc[0]) ? " " : "", mark);
    } else {
      snprintf(buf, sizeof(buf), _("type:%-2d"), mark);
    }
    legend_list_set_color(d, iter, type, row, COLOR_TYPE_1);
    break;
  case LegendTypeText:
    getobj(d->obj[type], "x", row, 0, NULL, x0);
    getobj(d->obj[type], "y", row, 0, NULL, y0);
    getobj(d->obj[type], "text", row, 0, NULL, &text);
    tree_store_set_string(GTK_WIDGET(d->text), iter, i, text);
    legend_list_set_color(d, iter, type, row, COLOR_TYPE_1);
    break;
  default:
    buf[0] = '\0';
  }
  if (type != LegendTypeText) {
    tree_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
  }
}

static void
legend_list_set_val(struct LegendWin *d, GtkTreeIter *iter, int type, int row)
{
  int cx, x0, y0, w;
  unsigned int i = 0;

  for (i = 0; i < LEGEND_WIN_COL_NUM; i++) {
    switch (i) {
    case LEGEND_WIN_COL_HIDDEN:
      getobj(d->obj[type], Llist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      tree_store_set_boolean(GTK_WIDGET(d->text), iter, i, cx);
      break;
    case LEGEND_WIN_COL_PROP:
      legend_list_set_property(d, iter, type, row, i, &x0, &y0);
      break;
    case LEGEND_WIN_COL_X:
      tree_store_set_double(GTK_WIDGET(d->text), iter, i, x0 / 100.0);
      break;
    case LEGEND_WIN_COL_Y:
      tree_store_set_double(GTK_WIDGET(d->text), iter, i, y0 / 100.0);
      break;
    case LEGEND_WIN_COL_WIDTH:
      switch (type) {
      case LegendTypeText:
	getobj(d->obj[type], "pt", row, 0, NULL, &w);
	break;
      case LegendTypeMark:
	getobj(d->obj[type], "size", row, 0, NULL, &w);
	break;
      default:
	getobj(d->obj[type], "width", row, 0, NULL, &w);
      }
      tree_store_set_double(GTK_WIDGET(d->text), iter, i, w / 100.0);
      break;
    default:
      getobj(d->obj[type], Llist[i].name, row, 0, NULL, &cx);
      tree_store_set_val(GTK_WIDGET(d->text), iter, i, Llist[i].type, &cx);
    }
  }
}

static void
legend_list_build(struct LegendWin *d)
{
  GtkTreeIter iter, child;
  int i, k;
  GtkTreePath *path;

  tree_store_clear(GTK_WIDGET(d->text));
  for (k = LEGENDNUM - 1; k >= 0 ; k--) {
    tree_store_prepend(GTK_WIDGET(d->text), &iter, NULL);
    tree_store_set_string(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_PROP, _(legendlist[k]));
    d->legend[k] = chkobjlastinst(d->obj[k]);
    tree_store_set_int(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_ID, d->legend[k] + 1);
    for (i = d->legend[k]; i >= 0 ; i--) {
      tree_store_prepend(GTK_WIDGET(d->text), &child, &iter);
      legend_list_set_val(d, &child, k, i);
    }
    path = gtk_tree_path_new_from_indices(0, -1);
    if (ExpandRow[k]) {
      gtk_tree_view_expand_row(GTK_TREE_VIEW(d->text), path, FALSE);
    }
    gtk_tree_path_free(path);
  }
}

static void
legend_list_set(struct LegendWin *d)
{
  GtkTreeIter iter, child;
  int i, k;
  gboolean state;

  tree_store_get_iter_first(GTK_WIDGET(d->text), &iter);

  for (k = 0; k < LEGENDNUM; k++) {
    tree_store_set_string(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_PROP, _(legendlist[k]));
    d->legend[k] = chkobjlastinst(d->obj[k]);
    tree_store_set_int(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_ID, d->legend[k] + 1);

    state = tree_store_get_iter_children(GTK_WIDGET(d->text), &child, &iter);
    i = 0;
    while (state) {
      legend_list_set_val(d, &child, k, i);
      state = tree_store_iter_next(GTK_WIDGET(d->text), &child);
      i++;
    }
    tree_store_iter_next(GTK_WIDGET(d->text), &iter);
  }
}

static gboolean
LegendWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data)
{
  struct LegendWin *d;

  if (Menulock || Globallock)
    return FALSE;

  d = &(NgraphApp.LegendWin);
  if (GTK_WIDGET(d->text) == NULL)
    return FALSE;

  if (legend_list_must_rebuild(d)) {
    legend_list_build(d);
  } else {
    legend_list_set(d);
  }

  return FALSE;
}

 /*
void
LegendWindowUnmap(GtkWidget *w, gpointer client_data)
{
  struct LegendWin *d;
  Position x, y, x0, y0;
  Dimension w0, h0;

  d = &(NgraphApp.LegendWin);
  if (d->Win != NULL) {
    XtVaGetValues(d->Win, XmNx, &x, XmNy, &y,
		  XmNwidth, &w0, XmNheight, &h0, NULL);
    menulocal.legendwidth = w0;
    menulocal.legendheight = h0;
    XtTranslateCoords(TopLevel, 0, 0, &x0, &y0);
    menulocal.legendx = x - x0;
    menulocal.legendy = y - y0;
    XtDestroyWidget(d->Win);
    d->Win = NULL;
    d->text = NULL;
    XmToggleButtonSetState(XtNameToWidget
			   (TopLevel, "*windowmenu.button_2"), False, False);
  }
}
 */

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  unsigned int i; 
  int sel, n, m, last_id;
  struct LegendWin *d;

  d = (struct LegendWin *) user_data;

  sel = tree_store_get_selected_nth(GTK_WIDGET(d->text), &n, &m);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      gtk_widget_set_sensitive(d->popup_item[i], sel && m > 0);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      last_id = -1;
      if (sel) {
	last_id = chkobjlastinst(d->obj[n]);
      }
      gtk_widget_set_sensitive(d->popup_item[i], sel && m >= 0 && m < last_id);
      break;
    case POPUP_ITEM_HIDE:
      if (sel && m >= 0) {
	int hidden;
	getobj(d->obj[n], "hidden", m, 0, NULL, &hidden);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(d->popup_item[i]), ! hidden);
      }
    default:
      gtk_widget_set_sensitive(d->popup_item[i], sel && m >= 0);
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
  struct LegendWin *d;
  int depth, col, *ary, inc, ecode;
  GtkTreePath *tree_path;
  double prev, val;
  char *tmp, *ptr;

  menu_lock(FALSE);

  if (str == NULL || path == NULL)
    return;

  switch (dir) {
  case CHANGE_DIR_X:
    col = LEGEND_WIN_COL_X;
    break;
  case CHANGE_DIR_Y:
    col = LEGEND_WIN_COL_Y;
    break;
  default:
    return;
  }

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  d = (struct LegendWin *) user_data;

  tree_path = gtk_tree_path_new_from_string(path);
  if (tree_path == NULL)
    return;

  depth = gtk_tree_path_get_depth(tree_path);
  if (depth < 2) {
    gtk_tree_path_free(tree_path);
    return;
  }

  ary = gtk_tree_path_get_indices(tree_path);

  tmp = tree_store_path_get_string(GTK_WIDGET(d->text), tree_path, col);
  if (tmp == NULL) {
    gtk_tree_path_free(tree_path);
    return;
  }

  prev = strtod(tmp, &ptr);
  if (prev != prev || prev == HUGE_VAL || prev == - HUGE_VAL || ptr[0] != '\0') {
    gtk_tree_path_free(tree_path);
    g_free(tmp);
    return;
  }

  g_free(tmp);

  if (ary[0] >= 0 && ary[0] < LEGENDNUM && ary[1] >= 0 && ary[1] <= d->legend[ary[0]]) {
    int x, y;
    char *argv[3];

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
      exeobj(d->obj[ary[0]], "move", ary[1], 2, argv);
      set_graph_modified();
      LegendWinUpdate(FALSE);
    }
  }

  gtk_tree_path_free(tree_path);
  return;
}

static void
width_edited(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data, enum CHANGE_DIR dir)
{
  struct LegendWin *d;
  int depth, *ary, ecode;
  GtkTreePath *tree_path;
  double val;

  menu_lock(FALSE);

  if (str == NULL || path == NULL)
    return;

  ecode = str_calc(str, &val, NULL, NULL);
  if (ecode || val != val || val == HUGE_VAL || val == - HUGE_VAL) {
    return;
  }

  d = (struct LegendWin *) user_data;

  tree_path = gtk_tree_path_new_from_string(path);
  if (tree_path == NULL)
    return;

  depth = gtk_tree_path_get_depth(tree_path);
  if (depth < 2) {
    gtk_tree_path_free(tree_path);
    return;
  }

  ary = gtk_tree_path_get_indices(tree_path);

  if (ary[0] >= 0 && ary[0] < LEGENDNUM && ary[1] >= 0 && ary[1] <= d->legend[ary[0]]) {
    int w, prev;
    struct objlist *obj, *textobj, *markobj;
    char *field;

    obj = d->obj[ary[0]];

    textobj = chkobject("text");
    markobj = chkobject("mark");
    if (obj == NULL || textobj == NULL || markobj == NULL)
      goto End;

    if (obj == textobj) {
      field = "pt";
    } else if (obj == markobj) {
      field = "size";
    } else {
      field = "width";
    }

    w = nround(val * 100);
    getobj(obj, field, ary[1], 0, NULL, &prev);
    if (prev != w) {
      putobj(obj, field, ary[1], &w);
      set_graph_modified();
      LegendWinUpdate(FALSE);
    }
  }

 End:
  gtk_tree_path_free(tree_path);

  return;
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

void
CmLegendWindow(GtkToggleAction *action, gpointer client_data)
{
  int i, state;
  struct LegendWin *d;
  GtkWidget *dlg;

  d = &(NgraphApp.LegendWin);

  for (i = 0; i < LEGENDNUM; i++) {
    d->obj[i] = chkobject(legendlist[i]);
    d->legend[i] = chkobjlastinst(d->obj[i]);
  }

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

  d->update = LegendWinUpdate;
  d->dialog = NULL;
  d->setup_dialog = LegendWinLegendUpdate;
  d->ev_key = NULL;

  dlg = tree_sub_window_create(d, "Legend Window", LEGEND_WIN_COL_NUM, Llist, Legendwin_xpm, Legendwin48_xpm);

  g_signal_connect(dlg, "expose-event", G_CALLBACK(LegendWinExpose), NULL);

  for (i = 0; i < LEGENDNUM; i++) {
    d->obj[i] = chkobject(legendlist[i]);
    d->legend[i] = chkobjlastinst(d->obj[i]);
  }

  sub_win_create_popup_menu((struct SubWin *)d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
  legend_list_build(d);
  gtk_tree_view_expand_all(GTK_TREE_VIEW(d->text));
  gtk_widget_show_all(dlg);

  set_editable_cell_renderer_cb((struct SubWin *)d, LEGEND_WIN_COL_X, Llist, G_CALLBACK(pos_x_edited));
  set_editable_cell_renderer_cb((struct SubWin *)d, LEGEND_WIN_COL_Y, Llist, G_CALLBACK(pos_y_edited));
  set_editable_cell_renderer_cb((struct SubWin *)d, LEGEND_WIN_COL_WIDTH, Llist, G_CALLBACK(width_edited));

  sub_window_show_all((struct SubWin *) d);
  sub_window_set_geometry((struct SubWin *)d, TRUE);
}
