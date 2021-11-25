/*
 * $Id: x11lgndx.c,v 1.20 2009-12-17 10:55:44 hito Exp $
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

#include <math.h>

#include "object.h"
#include "gra.h"
#include "mathfn.h"
#include "spline.h"

#include "gtk_combo.h"
#include "gtk_widget.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "ox11menu.h"
#include "x11commn.h"

#define VIEW_SIZE 128

enum LEGEND_DIRECTION {
  LEGEND_DIRECTION_RIGHT,
  LEGEND_DIRECTION_LEFT,
  LEGEND_DIRECTION_BOTTOM,
  LEGEND_DIRECTION_TOP,
};

#define LEGEND_DIRECTION_NUM (LEGEND_DIRECTION_TOP + 1)

static double *spx, *spy, *spz;
static double *spc[6];

static void LegendGaussDialogScaleH(GtkWidget *w, gpointer client_data);
static void LegendGaussDialogDiv(GtkWidget *w, gpointer client_data);

#define DIV_MAX 200
#define SCALE_V_MAX 1000.0
#define SCALE_H_MAX 100.0

static void
clear_view(struct LegendGaussDialog *d)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_queue_draw(d->view);
#else
  GdkWindow *win;

  win = gtk_widget_get_window(d->view);
  if (win) {
    gdk_window_invalidate_rect(win, NULL, TRUE);
  }
#endif
}

static void
LegendGaussDialogSetupItem(GtkWidget *w, struct LegendGaussDialog *d, int id)
{
  int n;

  SetStyleFromObjField(d->style, d->Obj, id, "style");

  SetWidgetFromObjField(d->width, d->Obj, id, "width");

  SetWidgetFromObjField(d->join, d->Obj, id, "join");

  SetWidgetFromObjField(d->miter, d->Obj, id, "miter_limit");

  set_stroke_color(d->color, d->Obj, id);

  n = d->Dir;
  if (n >= 0 && n < LEGEND_DIRECTION_NUM) {
    GtkToggleButton *btn;
    btn = GTK_TOGGLE_BUTTON(g_slist_nth_data(d->dir_list, n));
    gtk_toggle_button_set_active(btn, TRUE);
  }


  n = d->Mode;
  if (n >=0 && n < 4) {
    GtkToggleButton *btn;
    btn = GTK_TOGGLE_BUTTON(g_slist_nth_data(d->func_list, n));
    gtk_toggle_button_set_active(btn, TRUE);
  }

  gtk_range_set_value(GTK_RANGE(d->div), d->Div);
  gtk_range_set_value(GTK_RANGE(d->sch), d->Position * SCALE_H_MAX);
  gtk_range_set_value(GTK_RANGE(d->scv), d->Param * SCALE_V_MAX);
}

static gboolean
LegendGaussDialogPaint(GtkWidget *w, cairo_t *cr, gpointer client_data)
{
  struct LegendGaussDialog *d;
  int pw, dw, minx, miny, maxx, maxy,
    amp, wd, output, found;
  double ppd, dashes[] = {4.0};
  GdkWindow *win;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;
  cairo_surface_t *pix;

  d = (struct LegendGaussDialog *) client_data;

  win = gtk_widget_get_window(w);
  if (win == NULL) {
    return FALSE;
  }

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return FALSE;
  }

  pix = gra2gdk_create_pixmap(local, VIEW_SIZE, VIEW_SIZE,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return FALSE;
  }

  pw = VIEW_SIZE - 1;
  dw = (d->Wdx < d->Wdy) ? d->Wdy : d->Wdx;

  ppd = pw / ((double) dw);
  minx = VIEW_SIZE / 2 - d->Wdx * ppd / 2;
  miny = VIEW_SIZE / 2 - d->Wdy * ppd / 2;
  maxx = VIEW_SIZE / 2 + d->Wdx * ppd / 2;
  maxy = VIEW_SIZE / 2 + d->Wdy * ppd / 2;

  switch (d->Dir) {
  case LEGEND_DIRECTION_TOP:
  case LEGEND_DIRECTION_BOTTOM:
    amp = d->Wdy;
    wd = d->Wdx;
    break;
  default:
    amp = d->Wdx;
    wd = d->Wdy;
  }

  if (d->alloc) {
    int GC;
    GC = _GRAopen("gra2gdk", "_output",
		  robj, inst, output, -1, -1, -1, NULL, local);
    GRAlinestyle(GC, 0, NULL, 1, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
    if (GC >= 0) {
      int i, j, k, spnum;
      double tmp, y = 0, spc2[6];
      GRAview(GC, minx, miny, maxx, maxy, 1);

      if (d->Div > DIV_MAX) {
	d->Div = DIV_MAX;
      }

      for (i = 0; i <= (d->Div); i++) {
        double x;
	x = wd / ((double) (d->Div)) * i;
	if (d->Mode == 0) {
	  tmp = (x - wd * 0.5 - wd * d->Position * 0.5) /
	    (wd / (1 + 10 * d->Param) / 2);
	  y = amp * exp(-tmp * tmp);
	} else if (d->Mode == 1) {
	  tmp = (x - wd * 0.5 - wd * d->Position * 0.5) /
	    (wd / (1 + 10 * d->Param) / 2);
	  y = amp / (tmp * tmp + 1);
	} else if (d->Mode == 2) {
	  if (d->Position >= 0) {
	    tmp = (x - wd * 0.5 - wd * d->Position * 0.5) /
	      (-wd * 0.5 - wd * d->Position * 0.5);
	  } else {
	    tmp = (x - wd * 0.5 - wd * d->Position * 0.5) /
	      (wd * 0.5 - wd * d->Position * 0.5);
	  }
	  y = amp * tmp * tmp;
	} else if (d->Mode == 3) {
	  tmp = x / (wd / (0.25 + 10 * d->Param));
	  y = amp * 0.5 * (sin(2.0 * MPI * (tmp - d->Position * 0.5)) + 1);
	}

	switch (d->Dir) {
	case LEGEND_DIRECTION_TOP:
	  spx[i] = nround(x);
	  spy[i] = d->Wdy - nround(y);
	  break;
	case LEGEND_DIRECTION_BOTTOM:
	  spx[i] = nround(x);
	  spy[i] = nround(y);
	  break;
	case LEGEND_DIRECTION_LEFT:
	  spx[i] = d->Wdx - nround(y);
	  spy[i] = d->Wdy - nround(x);
	  break;
	case LEGEND_DIRECTION_RIGHT:
	  spx[i] = nround(y);
	  spy[i] = d->Wdy - nround(x);
	  break;
	default:
	  /* never reached*/
	  spx[i] = 0;
	  spy[i] = 0;
	}
	spz[i] = i;
	spx[i] *= ppd;
	spy[i] *= ppd;
      }
      spnum = d->Div + 1;
      spline(spz, spx, spc[0], spc[1], spc[2], spnum, SPLCND2NDDIF,
	     SPLCND2NDDIF, 0, 0);
      spline(spz, spy, spc[3], spc[4], spc[5], spnum, SPLCND2NDDIF,
	     SPLCND2NDDIF, 0, 0);
      GRAcolor(GC, 0, 0, 0, 255);
      GRAcurvefirst(GC, 0, NULL, NULL, NULL,
		    splinedif, splineint, NULL, spx[0], spy[0]);
      for (j = 0; j < spnum - 1; j++) {
	for (k = 0; k < 6; k++) {
	  spc2[k] = spc[k][j];
	}
	if (! GRAcurve(GC, spc2, spx[j], spy[j])) {
	  break;
	}
      }
    }
    _GRAclose(GC);
    gra2cairo_draw_path(local);
  }

  cairo_set_source_surface(cr, pix, 0, 0);
  cairo_paint(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, dashes, sizeof(dashes) / sizeof(*dashes), 0);
  cairo_rectangle(cr,
		  minx + CAIRO_COORDINATE_OFFSET, miny + CAIRO_COORDINATE_OFFSET,
		  maxx - minx, maxy - miny);
  cairo_stroke(cr);

  cairo_surface_destroy(pix);

  return FALSE;
}

static void
LegendGaussDialogScaleV(GtkWidget *w, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Param = gtk_range_get_value(GTK_RANGE(w)) / SCALE_V_MAX;

  clear_view(d);
}

static void
LegendGaussDialogScaleH(GtkWidget *w, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Position = gtk_range_get_value(GTK_RANGE(w)) / SCALE_H_MAX;

  clear_view(d);
}

static int
get_radio_index(GSList *top)
{
  int i, n;
  GSList *list;

  n = g_slist_length(top);
  for (i = 0, list = top; i < n; i++, list = list->next) {
    GtkToggleButton *btn;
    btn = GTK_TOGGLE_BUTTON(list->data);
    if (gtk_toggle_button_get_active(btn)) {
      return i;
    }
  }
  return -1;
}

static void
LegendGaussDialogMode(GtkWidget *w, gpointer client_data)
{
  struct LegendGaussDialog *d;
  int i;
  gboolean active;

  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  if (! active)
    return;

  d = (struct LegendGaussDialog *) client_data;
  i = get_radio_index(d->func_list);
  if (i < 0)
    return;

  d->Mode = i;

  clear_view(d);
}

static void
LegendGaussDialogDir(GtkWidget *w, gpointer client_data)
{
  int i;
  struct LegendGaussDialog *d;
  gboolean active;

  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  if (! active)
    return;

  d = (struct LegendGaussDialog *) client_data;
  i= get_radio_index(d->dir_list);
  if (i < 0 || i >= LEGEND_DIRECTION_NUM)
    return;

  d->Dir = i;

  clear_view(d);
}

static void
LegendGaussDialogDiv(GtkWidget *w, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Div = gtk_range_get_value(GTK_RANGE(w));

  clear_view(d);
}

static void
LegendGaussDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GSList *func_list, *dir_list;
  struct LegendGaussDialog *d;
  char title[256];

  d = (struct LegendGaussDialog *) data;
  snprintf(title, sizeof(title), _("Legend Gaussian/Lorentzian/Parabola/Sin %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *button, *hbox, *hbox2, *vbox, *table;
    int i;
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *group = NULL;
#endif
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    button = NULL;

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_Sin"));
    group = button;
    func_list = g_slist_append(NULL, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Sin"));
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), button);
#else
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_Parabola"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    func_list = g_slist_append(func_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Parabola"));
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), button);
#else
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_Lorentz"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    func_list = g_slist_append(func_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Lorentz"));
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_Gauss"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    func_list = g_slist_append(func_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Gauss"));
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

#if ! GTK_CHECK_VERSION(4, 0, 0)
    func_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 0);
#endif
    d->func_list = func_list;


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    table = gtk_grid_new();

    i = 0;
    w = combo_box_entry_create();
    add_widget_to_table(table, w, _("Line _Style:"), TRUE, i++);
    d->style = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Line Width:"), FALSE, i++);
    d->width = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    add_widget_to_table(table, w, _("_Miter:"), FALSE, i++);
    d->miter = w;

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Join:"), FALSE, i++);
    d->join = w;

    w = create_color_button(wi);
    add_widget_to_table(table, w, _("_Color:"), FALSE, i++);
    d->color = w;

    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, DIV_MAX, 1);
    set_scale_mark(w, GTK_POS_BOTTOM, 20, 20);
    add_widget_to_table(table, w, _("_Division:"), TRUE, i++);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogDiv), d);
    d->div = w;


    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    button = NULL;

#if GTK_CHECK_VERSION(4, 0, 0)
    group = NULL;
    button = gtk_check_button_new_with_mnemonic(_("_T"));
    dir_list = g_slist_append(NULL, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_T");
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox2), button);
#else
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_B"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    dir_list = g_slist_append(dir_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_B");
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox2), button);
#else
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_L"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    dir_list = g_slist_append(dir_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_L");
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox2), button);
#else
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    button = gtk_check_button_new_with_mnemonic(_("_R"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(button) GTK_CHECK_BUTTON(group));
    dir_list = g_slist_append(dir_list, button);
#else
    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_R");
#endif
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox2), button);
#else
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    dir_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
#endif
    d->dir_list = dir_list;

    add_widget_to_table(table, hbox2, _("Direction:"), TRUE, i++);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), table);
#else
    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
#endif


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, SCALE_V_MAX, 1);
    set_scale_mark(w, GTK_POS_BOTTOM, 100, 200);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogScaleV), d);
    d->scv = w;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#endif

    w = gtk_drawing_area_new();
    d->view = w;
    gtk_widget_set_size_request(w, VIEW_SIZE, VIEW_SIZE);

    g_signal_connect(w, "draw", G_CALLBACK(LegendGaussDialogPaint), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#endif

    w = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, - SCALE_H_MAX, SCALE_H_MAX, 1);
    set_scale_mark(w, GTK_POS_TOP, -100, 25);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogScaleH), d);
    d->sch = w;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(hbox), vbox);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
  LegendGaussDialogSetupItem(wi, d, d->Id);
}

static void
LegendGaussDialogClose(GtkWidget *w, void *data)
{
  struct LegendGaussDialog *d;
  int ret, a, i, amp, wd, gx, gy;
  double y = 0, tmp;
  struct narray *parray;

  d = (struct LegendGaussDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "style"))
    return;

  if (SetObjFieldFromWidget(d->width, d->Obj, d->Id, "width"))
    return;

  if (SetObjFieldFromWidget(d->join, d->Obj, d->Id, "join"))
    return;

  if (SetObjFieldFromWidget(d->miter, d->Obj, d->Id, "miter_limit"))
    return;

  if (putobj_stroke_color(d->color, d->Obj, d->Id))
    return;

  switch (d->Dir) {
  case LEGEND_DIRECTION_TOP:
  case LEGEND_DIRECTION_BOTTOM:
    amp = d->Wdy;
    wd = d->Wdx;
    break;
  default:
    amp = d->Wdx;
    wd = d->Wdy;
  }

  parray = arraynew(sizeof(int));
  for (i = 0; i <= d->Div; i++) {
    double x;
    x = wd / ((double) (d->Div)) * i;
    if (d->Mode == 0) {
      tmp =
	(x - wd * 0.5 -
	 wd * d->Position * 0.5) / (wd / (1 + 10 * d->Param) / 2);
      y = amp * exp(-tmp * tmp);
    } else if (d->Mode == 1) {
      tmp =
	(x - wd * 0.5 -
	 wd * d->Position * 0.5) / (wd / (1 + 10 * d->Param) / 2);
      y = amp / (tmp * tmp + 1);
    } else if (d->Mode == 2) {
      if (d->Position >= 0) {
	tmp =
	  (x - wd * 0.5 - wd * d->Position * 0.5) / (-wd * 0.5 -
						     wd * d->Position * 0.5);
      } else {
	tmp =
	  (x - wd * 0.5 - wd * d->Position * 0.5) / (wd * 0.5 -
						     wd * d->Position * 0.5);
      }
      y = amp * tmp * tmp;
    } else if (d->Mode == 3) {
      tmp = x / (wd / (0.25 + 10 * d->Param));
      y = amp * 0.5 * (sin(2.0 * MPI * (tmp - d->Position * 0.5)) + 1);
    }
    switch (d->Dir) {
    case LEGEND_DIRECTION_TOP:
      gx = nround(x);
      gy = d->Wdy - nround(y);
      break;
    case LEGEND_DIRECTION_BOTTOM:
      gx = nround(x);
      gy = nround(y);
      break;
    case LEGEND_DIRECTION_LEFT:
      gx = d->Wdx - nround(y);
      gy = d->Wdy - nround(x);
      break;
    case LEGEND_DIRECTION_RIGHT:
      gx = nround(y);
      gy = d->Wdy - nround(x);
      break;
    default:
      /* never reached */
      gx = 0;
      gy = 0;
    }
    gx += d->Minx;
    gy += d->Miny;
    arrayadd(parray, &gx);
    arrayadd(parray, &gy);
  }

  putobj(d->Obj, "points", d->Id, parray);

  a = 0;
  putobj(d->Obj, "interpolation", d->Id, &a);

  d->ret = ret;

  for (i = 0; i < 6; i++)
    g_free(spc[i]);

  g_free(spz);
  g_free(spy);
  g_free(spx);
}

void
LegendGaussDialog(struct LegendGaussDialog *data,
		  struct objlist *obj, int id,
		  int minx, int miny, int wdx, int wdy)
{
  int i;

  data->SetupWindow = LegendGaussDialogSetup;
  data->CloseWindow = LegendGaussDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->Minx = minx;
  data->Miny = miny;
  data->Wdx = wdx;
  data->Wdy = wdy;
  data->Div = 20;
  data->Position = 0;
  data->Param = 0.175;
  data->Dir = LEGEND_DIRECTION_TOP;
  data->Mode = 0;
  data->alloc = TRUE;

  spx = (double *) g_malloc(sizeof(double) * (DIV_MAX + 1));
  if (spx == NULL)
    data->alloc = FALSE;

  spy = (double *) g_malloc(sizeof(double) * (DIV_MAX + 1));
  if (spy == NULL)
    data->alloc = FALSE;

  spz = (double *) g_malloc(sizeof(double) * (DIV_MAX + 1));
  if (spz == NULL)
    data->alloc = FALSE;

  for (i = 0; i < 6; i++) {
    spc[i] = (double *) g_malloc(sizeof(double) * (DIV_MAX + 1));
    if (spc[i] == NULL) {
      data->alloc = FALSE;
    }
  }
}
