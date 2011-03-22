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

#include "ngraph.h"
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
  GdkWindow *win;

  win = GTK_WIDGET_GET_WINDOW(d->view);
  if (win) {
    gdk_window_invalidate_rect(win, NULL, TRUE);
  }
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
LegendGaussDialogPaint(GtkWidget *w, GdkEventExpose *event, gpointer client_data)
{
  struct LegendGaussDialog *d;
  int i, j, k, pw, dw, minx, miny, maxx, maxy,
    amp, wd, GC, spnum, output, found;
  double ppd, x, y = 0, tmp, spc2[6], dashes[] = {4.0};
  GdkPixmap *pix;
  GdkWindow *win;
  cairo_t *cr;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;

  d = (struct LegendGaussDialog *) client_data;

  win = GTK_WIDGET_GET_WINDOW(w);
  if (win == NULL) {
    return FALSE;
  }

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return FALSE;
  }

  pix = gra2gdk_create_pixmap(gobj, inst, local, win,
			      VIEW_SIZE, VIEW_SIZE,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return FALSE;
  }

  cr = gdk_cairo_create(win);

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
    GC = _GRAopen("gra2gdk", "_output",
		  robj, inst, output, -1, -1, -1, NULL, local);
    GRAlinestyle(GC, 0, NULL, 1, 0, 0, 1000);
    if (GC >= 0) {
      GRAview(GC, minx, miny, maxx, maxy, 1);

      if (d->Div > DIV_MAX) {
	d->Div = DIV_MAX;
      }

      for (i = 0; i <= (d->Div); i++) {
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
    if (local->linetonum && local->cairo) {
      cairo_stroke(local->cairo);
      local->linetonum = 0;
    }
  }

  gdk_cairo_set_source_pixmap(cr, pix, 0, 0);
  cairo_rectangle(cr, 0, 0, VIEW_SIZE, VIEW_SIZE);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, dashes, sizeof(dashes) / sizeof(*dashes), 0);
  cairo_rectangle(cr, minx, miny, maxx - minx, maxy - miny);
  cairo_stroke(cr);

  g_object_unref(G_OBJECT(pix));
  cairo_destroy(cr);

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
  GtkWidget *w, *button, *hbox, *hbox2, *vbox, *table;
  GSList *func_list, *dir_list;
  struct LegendGaussDialog *d;
  char title[256];
  int i;

  d = (struct LegendGaussDialog *) data;
  snprintf(title, sizeof(title), _("Legend Gaussian/Lorentzian/Parabola/Sin %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);

    button = NULL;

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Sin"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Parabola"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Lorentz"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Gauss"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    func_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 0);
    d->func_list = func_list;


    hbox = gtk_hbox_new(FALSE, 4);

    table = gtk_table_new(1, 2, FALSE);

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

    w = gtk_hscale_new_with_range(10, DIV_MAX, 1);
    add_widget_to_table(table, w, _("_Division:"), TRUE, i++);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogDiv), d);
    d->div = w;


    hbox2 = gtk_hbox_new(FALSE, 4);
    button = NULL;

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_T");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_B");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_L");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_R");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    dir_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
    d->dir_list = dir_list;

    add_widget_to_table(table, hbox2, _("Direction:"), TRUE, i++);

    gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);


    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_hscale_new_with_range(0, SCALE_V_MAX, 1);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogScaleV), d);
    d->scv = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    w = gtk_drawing_area_new();
    d->view = w;
    gtk_widget_set_size_request(w, VIEW_SIZE, VIEW_SIZE);

    g_signal_connect(w, "expose-event", G_CALLBACK(LegendGaussDialogPaint), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    w = gtk_hscale_new_with_range(- SCALE_H_MAX, SCALE_H_MAX, 1);
    g_signal_connect(w, "value-changed", G_CALLBACK(LegendGaussDialogScaleH), d);
    d->sch = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 0);
  }
  LegendGaussDialogSetupItem(wi, d, d->Id);
}

static void
LegendGaussDialogClose(GtkWidget *w, void *data)
{
  struct LegendGaussDialog *d;
  int ret, a, i, amp, wd, gx, gy;
  double x, y = 0, tmp;
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
    if ((spc[i] = (double *) g_malloc(sizeof(double) * 201)) == NULL) {
      data->alloc = FALSE;
    }
  }
}
