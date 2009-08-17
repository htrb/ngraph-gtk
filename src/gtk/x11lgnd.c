/* 
 * $Id: x11lgnd.c,v 1.57 2009/08/17 07:09:46 hito Exp $
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
#include "otext.h"
#include "jnstring.h"
#include "nstring.h"
#include "mathfn.h"
#include "shellcm.h"

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
  {N_("object/property"), G_TYPE_STRING,  TRUE, FALSE, "property", FALSE, 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
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
  N_("line"),
  N_("curve"),
  N_("polygon"),
  N_("rectangle"),
  N_("arc"),
  N_("mark"),
  N_("text"),
};

static struct LegendDialog *Ldlg[] = {
  &DlgLegendCurve,
  &DlgLegendPoly,
  &DlgLegendArrow,
  &DlgLegendRect,
  &DlgLegendArc,
  &DlgLegendMark,
  &DlgLegendText,
};

static int ExpandRow[LEGENDNUM] = {0};

enum LegendType {
  LegendTypeLine = 0,
  LegendTypeCurve,
  LegendTypePoly,
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
  int num, *data;
  char *s;

  s = (char *) memalloc(CB_BUF_SIZE);
  if (s == NULL)
    return NULL;

  getobj(obj, "points", id, 0, NULL, &array);
  num = arraynum(array);
  data = (int *) arraydata(array);
  if (num < 2) {
    snprintf(s, CB_BUF_SIZE, "------");
  } else {
    snprintf(s, CB_BUF_SIZE, "(X:%.2f Y:%.2f)-", data[0] / 100.0, data[1] / 100.0);
  }
  return s;
}

static char *
LegendRectCB(struct objlist *obj, int id)
{
  int x1, y1;
  char *s;

  s = (char *) memalloc(CB_BUF_SIZE);
  if (s == NULL)

  getobj(obj, "x1", id, 0, NULL, &x1);
  getobj(obj, "y1", id, 0, NULL, &y1);
  snprintf(s, CB_BUF_SIZE, "X1:%.2f Y1:%.2f", x1 / 100.0, y1 / 100.0);
  return s;
}

static char *
LegendArcCB(struct objlist *obj, int id)
{
  int x1, y1;
  char *s;

  s = (char *) memalloc(CB_BUF_SIZE);
  if (s == NULL)
    return NULL;

  getobj(obj, "x", id, 0, NULL, &x1);
  getobj(obj, "y", id, 0, NULL, &y1);
  snprintf(s, CB_BUF_SIZE, "X:%.2f Y:%.2f", x1 / 100.0, y1 / 100.0);
  return s;
}

static char *
LegendTextCB(struct objlist *obj, int id)
{
  char *text, *s;

  getobj(obj, "text", id, 0, NULL, &text);

  if (text) {
#ifdef JAPANESE
/* SJIS ---> UTF-8 */
    s = sjis_to_utf8(text);
#else
    s = nstrdup(text);
#endif
  } else {
    s = nstrdup("");
  }
  return s;
}

static void
init_legend_dialog_widget_member(struct LegendDialog *d)
{
  d->style = NULL;
  d->points = NULL;
  d->interpolation = NULL;
  d->width = NULL;
  d->miter = NULL;
  d->join = NULL;
  d->color = NULL;
  d->color2 = NULL;
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
  d->jfont = NULL;
}

static void
set_font(GtkWidget *w, char *name, struct LegendDialog *d, int id, int jfont)
{
  SetFontListFromObj(w, d->Obj, d->Id, name, jfont);
}

static void
get_font(char *name, struct LegendDialog *d, int jfont)
{
  if (jfont) {
    SetObjFieldFromFontList(d->jfont, d->Obj, d->Id, name, jfont);
  } else {
    SetObjFieldFromFontList(d->font, d->Obj, d->Id, name, jfont);
  }
}

static void
set_fonts(struct LegendDialog *d, int id)
{
  set_font(d->font, "font", d, id, FALSE);
#ifdef JAPANESE
  set_font(d->jfont, "jfont", d, id, TRUE);
#endif
}

static void
legend_dialog_setup_item(GtkWidget *w, struct LegendDialog *d, int id)
{
  unsigned int i;
  int x1, y1, x2, y2;
  struct lwidget lw[] = {
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
    {d->fill_rule, "fill"},
    {d->frame, "frame"},
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
    GtkWidget *img;

    getobj(d->Obj, "type", id, 0, NULL, &a);
    if (a < 0)
      a = 89;
    if (a > 89)
      a = 89;

    img = gtk_image_new_from_pixmap(NgraphApp.markpix[a], NULL);
    gtk_button_set_image(GTK_BUTTON(d->type), img);
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
#ifdef JAPANESE
    /* SJIS ---> UTF-8 */
    {
      char *tmp;
      tmp = sjis_to_utf8(buf);
      if (tmp) {
	memfree(buf);
	buf = strdup(tmp);
	free(tmp);
      }
    }
#endif
    gtk_entry_set_text(GTK_ENTRY(d->text), buf);
    memfree(buf);
  }

  if (d->font)
    set_fonts(d, id);

  if (d->color)
    set_color(d->color, d->Obj, id, NULL);

  if (d->color2)
    set_color2(d->color2, d->Obj, id);
}

static void
legend_dialog_close(GtkWidget *w, void *data)
{
  struct LegendDialog *d = (struct LegendDialog *) data;
  unsigned int i;
  int ret, x1, y1, x2, y2, oval;
  struct lwidget lw[] = {
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
    {d->fill_rule, "fill"},
    {d->frame, "frame"},
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

  if (d->font) {
    get_font("font", d, FALSE);
  }
#ifdef JAPANESE
  if (d->jfont) {
    get_font("jfont", d, TRUE);
  }
#endif

  if (d->text) {
    const char *str;
    char *ptr;

    str = gtk_entry_get_text(GTK_ENTRY(d->text));
    if (str == NULL)
      return;

    entry_completion_append(NgraphApp.legend_text_list, str);
    gtk_entry_set_completion(GTK_ENTRY(d->text), NULL);

    ptr = strdup(str);

    if (ptr) {
      char *org_str;
#ifdef JAPANESE
/* UTF-8 ---> SJIS */
      char *tmp;
      tmp = utf8_to_sjis(ptr);
      if (tmp) {
	free(ptr);
	ptr = tmp;
      }
#endif
      sgetobjfield(d->Obj, d->Id, "text", NULL, &org_str, FALSE, FALSE, FALSE);
      if (org_str == NULL || strcmp(ptr, org_str)) {
	if (sputobjfield(d->Obj, d->Id, "text", ptr) != 0) {
	  memfree(org_str);
	  free(ptr);
	  return;
	}
	set_graph_modified();
      }
      memfree(org_str);
      free(ptr);
    }
  }

  if (d->color && putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

 if (d->color2 && putobj_color2(d->color2, d->Obj, d->Id))
    return;

  d->ret = ret;
}

static void
LegendDialogCopy(struct LegendDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, d->prop_cb)) != -1)
    legend_dialog_setup_item(d->widget, d, sel);
}

static void
width_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  d->width = w;
  item_setup(box, w, _("_Line width:"), TRUE);
}

static void
points_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = create_text_entry(FALSE, TRUE);
  d->points = w;
  item_setup(box, w, _("_Points:"), TRUE);
}

static void
style_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  d->style = w;
  item_setup(box, w, _("Line _Style:"), TRUE);
}

static void
miter_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  d->miter = w;
  item_setup(box, w, _("_Miter:"), FALSE);
}

static void
join_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = combo_box_create();
  d->join = w;
  item_setup(box, w, _("_Join:"), FALSE);
}

static void
color_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color = w;
  item_setup(box, w, _("_Color:"), FALSE);
}

static void
color2_setup(struct LegendDialog *d, GtkWidget *box)
{
  GtkWidget *w;

  w = create_color_button(d->widget);
  d->color2 = w;
  item_setup(box, w, _("_Color2:"), FALSE);
}

static void
LegendCurveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *vbox, *hbox, *w;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;

  snprintf(title, sizeof(title), _("Legend curve %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "curve");

    vbox = gtk_vbox_new(FALSE, 5);

    hbox = gtk_hbox_new(FALSE, 5);
    points_setup(d, hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);


    hbox = gtk_hbox_new(FALSE, 5);

    style_setup(d, hbox);

    w = combo_box_create();
    item_setup(hbox, w, _("_Interpolation:"), FALSE);
    d->interpolation = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);


    hbox = gtk_hbox_new(FALSE, 5);

    width_setup(d, hbox);
    miter_setup(d, hbox);
    join_setup(d, hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    hbox = gtk_hbox_new(FALSE, 5);
    color_setup(d, hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 2);

    d->prop_cb = LegendLineCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
}

void
LegendCurveDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendCurveDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}

static void
LegendPolyDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *hbox, *vbox, *w;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend polygon %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);

    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "polygon");

    vbox = gtk_vbox_new(FALSE, 5);

    hbox = gtk_hbox_new(FALSE, 5);
    points_setup(d, hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    hbox = gtk_hbox_new(FALSE, 5);
    style_setup(d, hbox);
    width_setup(d,  hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    hbox = gtk_hbox_new(FALSE, 5);
    miter_setup(d, hbox);
    join_setup(d, hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    hbox = gtk_hbox_new(FALSE, 5);
    color_setup(d, hbox);

    w = combo_box_create();
    item_setup(hbox, w, _("_Fill:"), FALSE);
    d->fill_rule = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 2);

    d->prop_cb = LegendLineCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
}

void
LegendPolyDialog(struct LegendDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = LegendPolyDialogSetup;
  data->CloseWindow = legend_dialog_close;
  data->Obj = obj;
  data->Id = id;
}


static void
draw_arrow_pixmap(GtkWidget *win, struct LegendDialog *d)
{
  int lw, len, x, w;
  static GdkColor black, white;
  GdkPoint points[3];
  static GdkPixmap *pixmap = NULL;
  static GdkGC *gc;

  if (win->window == NULL)
    return;

  if (pixmap == NULL) {
    pixmap = gdk_pixmap_new(win->window, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE, -1);

    gc = gdk_gc_new(pixmap);

    black.red = 0;
    black.green = 0;
    black.blue = 0;

    white.red = 65535;
    white.green = 65535;
    white.blue = 65535;
  }

  gdk_gc_set_rgb_bg_color(gc, &white);
  gdk_gc_set_rgb_fg_color(gc, &white);
  gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, ARROW_VIEW_SIZE, ARROW_VIEW_SIZE);

  gdk_gc_set_rgb_fg_color(gc, &black);

  lw = ARROW_VIEW_SIZE / 20;

  len = d->wid * 0.5 / tan(d->ang * 0.5 * MPI / 180);
  x = nround(lw * (len / 10000.0));
  gdk_draw_rectangle(pixmap, gc, TRUE,
		     x,
		     (ARROW_VIEW_SIZE - lw) / 2,
		     ARROW_VIEW_SIZE - x, lw);

  w = nround(lw * (d->wid / 20000.0));
  points[0].x = 0;
  points[0].y = ARROW_VIEW_SIZE / 2;
  points[1].x = x;
  points[1].y = ARROW_VIEW_SIZE / 2 - w;
  points[2].x = x;
  points[2].y = ARROW_VIEW_SIZE / 2 + w;

  gdk_draw_polygon(pixmap, gc, TRUE, points, sizeof(points) / sizeof(*points));

  d->arrow_pixmap = pixmap;
}

static void
LegendArrowDialogPaint(GtkWidget *w, GdkEvent *event, gpointer client_data)
{
  struct LegendDialog *d;
  GdkWindow *win;
  GdkGC *gc;

  d = (struct LegendDialog *) client_data;

  if (d->arrow_pixmap == NULL) {
    draw_arrow_pixmap(w, d);
  }

  if (d->arrow_pixmap) {
    win = w->window;
    gc = gdk_gc_new(win);
    gdk_draw_drawable(win, gc, d->arrow_pixmap, 0, 0, 0, 0, -1, -1);
    g_object_unref(gc);
  }
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
  GtkWidget *w, *hbox, *hbox2, *vbox, *vbox2;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend line %d"), d->Id); 
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "line");

    vbox2 = gtk_vbox_new(FALSE, 4);

    hbox = gtk_vbox_new(FALSE, 4);
    points_setup(d, hbox);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);
    style_setup(d, hbox2);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 4);
    width_setup(d, hbox2);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 4);
    miter_setup(d, hbox2);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 4);
    join_setup(d, hbox2);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

    color_setup(d, vbox);

    w = combo_box_create();
    item_setup(vbox, w, _("_Arrow:"), FALSE);
    d->arrow = w;

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);


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

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox2, TRUE, TRUE, 0);

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
  GtkWidget *w, *hbox, *vbox;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend rectangle %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "rectangle");

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_X:", FALSE);
    d->x1 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_Y:", FALSE);
    d->y1 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, _("_Width:"), FALSE);
    d->x2 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, _("_Height:"), FALSE);
    d->y2 = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    style_setup(d, hbox);
    width_setup(d, hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    color_setup(d, hbox);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->fill = w;

    color2_setup(d, hbox);

    w = gtk_check_button_new_with_mnemonic(_("_Frame"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->frame = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);

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
  GtkWidget *w, *hbox, *vbox;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend arc %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "arc");

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_X:", FALSE);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_Y:", FALSE);
    d->y = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    item_setup(hbox, w, "_RX:", FALSE);
    d->rx = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    item_setup(hbox, w, "_RY:", FALSE);
    d->ry = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Pieslice"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->pieslice = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, TRUE, TRUE);
    item_setup(hbox, w, _("_Angle1:"), FALSE);
    d->angle1 = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_ANGLE, TRUE, TRUE);
    item_setup(hbox, w, _("_Angle2:"), FALSE);
    d->angle2 = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    style_setup(d, hbox);
    width_setup(d, hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    color_setup(d, hbox);

    w = gtk_check_button_new_with_mnemonic(_("_Fill"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->fill = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);

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
  GtkWidget *img;

  d = (struct LegendDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  img = gtk_image_new_from_pixmap(NgraphApp.markpix[d->mark.Type], NULL);
  gtk_button_set_image(GTK_BUTTON(w), img);
}

static void
LegendMarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend mark %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "mark");

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_X:", FALSE);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_Y:", FALSE);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    style_setup(d, hbox);
    width_setup(d, hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_button_new();
    item_setup(hbox, w, _("_Mark:"), FALSE);
    g_signal_connect(w, "clicked", G_CALLBACK(LegendMarkDialogMark), d);
    d->type = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    item_setup(hbox, w, _("_Size:"), FALSE);
    d->size = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    color_setup(d, hbox);
    color2_setup(d, hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);

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
legend_dialog_setup_sub(struct LegendDialog *d, GtkWidget *vbox)
{
  GtkWidget *w, *hbox;

  hbox = gtk_hbox_new(FALSE, 4);

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_POINT, TRUE, TRUE);
  item_setup(hbox, w, _("_Pt:"), FALSE);
  d->pt = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_SPACE_POINT, TRUE, TRUE);
  item_setup(hbox, w, _("_Space:"), FALSE);
  d->space = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, TRUE, TRUE);
  spin_entry_set_range(w, TEXT_OBJ_SCRIPT_SIZE_MIN, TEXT_OBJ_SCRIPT_SIZE_MAX);
  item_setup(hbox, w, _("_Script:"), FALSE);
  d->script_size = w;

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);

  w = combo_box_create();
  item_setup(hbox, w, _("_Font:"), FALSE);
  d->font = w;

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

#ifdef JAPANESE
  hbox = gtk_hbox_new(FALSE, 4);

  w = combo_box_create();
  item_setup(hbox, w, _("_Jfont:"), FALSE);
  d->jfont = w;

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif

  hbox = gtk_hbox_new(FALSE, 4);

  color_setup(d, hbox);

  w = create_direction_entry();
  item_setup(hbox, w, _("_Direction:"), FALSE);
  d->direction = w;

  w = gtk_check_button_new_with_mnemonic(_("_Raw"));
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  d->raw = w;

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
}

static void
LegendTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox;
  struct LegendDialog *d;
  char title[64];

  d = (struct LegendDialog *) data;
  snprintf(title, sizeof(title), _("Legend text %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    init_legend_dialog_widget_member(d);

    gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_DELETE, IDDELETE);
    w = gtk_dialog_add_button(GTK_DIALOG(wi), GTK_STOCK_COPY, IDCOPY);
    g_signal_connect(w, "show", G_CALLBACK(set_sensitivity_by_check_instance), "text");

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_X:", FALSE);
    d->x = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, TRUE, TRUE);
    item_setup(hbox, w, "_Y:", FALSE);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Text:"), TRUE);
    d->text = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    legend_dialog_setup_sub(d, vbox);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);

    d->prop_cb = LegendTextCB;
  }
  legend_dialog_setup_item(wi, d, d->Id);
  gtk_entry_set_completion(GTK_ENTRY(d->text), NgraphApp.legend_text_list);
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
  GtkWidget *vbox;

  d = (struct LegendDialog *) data;
  if (makewidget) {
    init_legend_dialog_widget_member(d);

    vbox = gtk_vbox_new(FALSE, 4);

    legend_dialog_setup_sub(d, vbox);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
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

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("line")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
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
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("line")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendArrowDialog(&DlgLegendArrow, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendArrow)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
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
CmCurveDel(void)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("curve")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmCurveUpdate(void)
{
  struct narray array;
  struct objlist *obj;
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("curve")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendCurveDialog(&DlgLegendCurve, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendCurve)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmCurveMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmCurveUpdate();
    break;
  case MenuIdLegendDel:
    CmCurveDel();
    break;
  }
}

void
CmPolyDel(void)
{
  struct narray array;
  struct objlist *obj;
  int i;
  int num, *data;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("polygon")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, data[i]);
      set_graph_modified();
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmPolyUpdate(void)
{
  struct narray array;
  struct objlist *obj;
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("polygon")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendLineCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendPolyDialog(&DlgLegendPoly, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendPoly)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
      }
    }
    LegendWinUpdate(TRUE);
  }
  arraydel(&array);
}

void
CmPolygonMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdLegendUpdate:
    CmPolyUpdate();
    break;
  case MenuIdLegendDel:
    CmPolyDel();
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

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("rectangle")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendRectCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
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
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("rectangle")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendRectCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendRectDialog(&DlgLegendRect, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendRect)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
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

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("arc")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
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
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("arc")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendArcDialog(&DlgLegendArc, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendArc)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
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

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("mark")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
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
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("mark")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendArcCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendMarkDialog(&DlgLegendMark, obj, data[i]);
	set_graph_modified();
      if ((ret = DialogExecute(TopLevel, &DlgLegendMark)) == IDDELETE) {
	delobj(obj, data[i]);
	for (j = i + 1; j < num; j++)
	  data[j]--;
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

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("text")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendTextCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
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
  int i, j, ret;
  int *data, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("text")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, LegendTextCB, (struct narray *) &array, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&array);
    data = (int *) arraydata(&array);
    for (i = 0; i < num; i++) {
      LegendTextDialog(&DlgLegendText, obj, data[i]);
      if ((ret = DialogExecute(TopLevel, &DlgLegendText)) == IDDELETE) {
	delobj(obj, data[i]);
	set_graph_modified();
	for (j = i + 1; j < num; j++)
	  data[j]--;
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

  if (Menulock || GlobalLock)
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

  if (Menulock || GlobalLock)
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
  case LegendTypeLine:
    lgd->SetupWindow = LegendArrowDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendArrowDialog(&DlgLegendArrow, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendArrow);
    break;
  case LegendTypeCurve: 
    lgd->SetupWindow = LegendCurveDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendCurveDialog(&DlgLegendCurve, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendCurve);
    break;
  case LegendTypePoly:
    lgd->SetupWindow = LegendPolyDialogSetup;
    lgd->CloseWindow = legend_dialog_close;
    LegendPolyDialog(&DlgLegendPoly, d->obj[id], sub_id);
    ret = DialogExecute(d->Win, &DlgLegendPoly);
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
  points = (int *) arraydata(array);
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

static void
legend_list_set_val(struct LegendWin *d, GtkTreeIter *iter, int type, int row)
{
  int cx, x0, y0, x2, y2, mark, w, frame;
  unsigned int i = 0;
  char *valstr, *text, *ex, buf[256], buf2[256];
  char **enumlist;
  const char *str;

  for (i = 0; i < LEGEND_WIN_COL_NUM; i++) {
    switch (i) {
    case LEGEND_WIN_COL_HIDDEN:
      getobj(d->obj[type], Llist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      tree_store_set_boolean(GTK_WIDGET(d->text), iter, i, cx);
      break;
    case LEGEND_WIN_COL_PROP:
      ex = NULL;
      switch (type) {
      case LegendTypeLine:
	sgetobjfield(d->obj[type], row, "arrow", NULL, &valstr, FALSE, FALSE, FALSE);
	snprintf(buf2, sizeof(buf2), _("arrow:%s"), _(valstr));
	memfree(valstr);
	get_points(buf, sizeof(buf), d->obj[type], row, &x0, &y0, TRUE, buf2);
	break;
      case LegendTypeCurve:
	getobj(d->obj[type], "interpolation", row, 0, NULL, &w);
	enumlist = (char **) chkobjarglist(d->obj[type], "interpolation");
	snprintf(buf2, sizeof(buf2), _("interpolation:%s"), _(enumlist[w]));
	get_points(buf, sizeof(buf), d->obj[type], row, &x0, &y0, TRUE, buf2);
	break;
      case LegendTypePoly:
	getobj(d->obj[type], "fill", row, 0, NULL, &w);
	if (w) {
	  enumlist = (char **) chkobjarglist(d->obj[type], "fill");
	  snprintf(buf2, sizeof(buf2), _("fill:%s"), _(enumlist[w]));
	}
	get_points(buf, sizeof(buf), d->obj[type], row, &x0, &y0, w == 0, (w) ? buf2 : NULL);
	break;
      case LegendTypeRect:
	getobj(d->obj[type], "fill", row, 0, NULL, &w);
	getobj(d->obj[type], "frame", row, 0, NULL, &frame);
	str = get_style_string(d->obj[type], row, "style");
	getobj(d->obj[type], "x1", row, 0, NULL, &x0);
	getobj(d->obj[type], "y1", row, 0, NULL, &y0);
	getobj(d->obj[type], "x2", row, 0, NULL, &x2);
	getobj(d->obj[type], "y2", row, 0, NULL, &y2);
	snprintf(buf, sizeof(buf), _("w:%.2f h:%.2f  style:%s%s%s"),
		 abs(x0 - x2) / 100.0,
		 abs(y0 - y2) / 100.0,
		 (str) ? _(str) : _("custom"),
		 (w) ? _("  fill") : "",
		 (frame) ? _("  frame") : ""
		 );
	break;
      case LegendTypeArc:
	getobj(d->obj[type], "fill", row, 0, NULL, &w);
	str = get_style_string(d->obj[type], row, "style");
	getobj(d->obj[type], "x", row, 0, NULL, &x0);
	getobj(d->obj[type], "y", row, 0, NULL, &y0);
	getobj(d->obj[type], "rx", row, 0, NULL, &x2);
	getobj(d->obj[type], "ry", row, 0, NULL, &y2);
	snprintf(buf, sizeof(buf), "rx:%.2f ry:%.2f  %s%s",
		 x2 / 100.0,
		 y2 / 100.0,
		 (w) ? _("fill") : _("style:"),
		 (w) ? "" : ((str) ? _(str) : _("custom")));
	break;
      case LegendTypeMark:
	getobj(d->obj[type], "x", row, 0, NULL, &x0);
	getobj(d->obj[type], "y", row, 0, NULL, &y0);
	getobj(d->obj[type], "type", row, 0, NULL, &mark);
	if (mark >= 0 && mark < MarkCharNum) {
	  char *mc = MarkChar[mark];
	  snprintf(buf, sizeof(buf), _("%s%stype:%-2d"), mc, (mc[0]) ? " " : "", mark);
	} else {
	  snprintf(buf, sizeof(buf), _("type:%-2d"), mark);
	}
	break;
      case LegendTypeText:
	getobj(d->obj[type], "x", row, 0, NULL, &x0);
	getobj(d->obj[type], "y", row, 0, NULL, &y0);
	getobj(d->obj[type], "text", row, 0, NULL, &text);
	{
	  char *tmp;
#ifdef JAPANESE
/* SJIS ---> UTF-8 */
	  tmp = sjis_to_utf8(text);
	  if (tmp) {
	    tree_store_set_string(GTK_WIDGET(d->text), iter, i, tmp);
	    free(tmp);
	  }
#else
	  tree_store_set_string(GTK_WIDGET(d->text), iter, i, text);
#endif
	}
	break;
      default:
	buf[0] = '\0';
      }
      if (type != LegendTypeText) {
	tree_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
      }
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
  for (k = 0; k < LEGENDNUM; k++) {
    tree_store_append(GTK_WIDGET(d->text), &iter, NULL);
    tree_store_set_string(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_PROP, _(legendlist[k]));
    d->legend[k] = chkobjlastinst(d->obj[k]);
    tree_store_set_int(GTK_WIDGET(d->text), &iter, LEGEND_WIN_COL_ID, d->legend[k] + 1);
    for (i = 0; i <= d->legend[k]; i++) {
      tree_store_append(GTK_WIDGET(d->text), &child, &iter);
      legend_list_set_val(d, &child, k, i);
    }
    path = gtk_tree_path_new_from_indices(k, -1);
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

  if (Menulock || GlobalLock)
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

  ecode = str_calc(str, &val, NULL);
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

  ecode = str_calc(str, &val, NULL);
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
CmLegendWindow(GtkWidget *w, gpointer client_data)
{
  int i;
  struct LegendWin *d;

  d = &(NgraphApp.LegendWin);
  d ->type = TypeLegendWin;

  for (i = 0; i < LEGENDNUM; i++) {
    d->obj[i] = chkobject(legendlist[i]);
    d->legend[i] = chkobjlastinst(d->obj[i]);
  }

  if (d->Win) {
    sub_window_toggle_visibility((struct SubWin *) d);
  } else {
    GtkWidget *dlg;

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
}
