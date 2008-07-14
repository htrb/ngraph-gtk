/* 
 * $Id: x11lgndx.c,v 1.10 2008/07/14 14:16:48 hito Exp $
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

static double *spx, *spy, *spz;
static double *spc[6];

static gboolean LegendGaussDialogScaleH(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data);
static gboolean LegendGaussDialogScaleH(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data);
static gboolean LegendGaussDialogDiv(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data);

#define DIV_MAX 200


static void
clear_view(struct LegendGaussDialog *d)
{
  if (d->view->window) {
    gdk_window_clear(d->view->window);
    gdk_window_invalidate_rect(d->view->window, NULL, TRUE);
  }
}

void
LegendGaussDialogSetupItem(GtkWidget *w, struct LegendGaussDialog *d, int id)
{
  int n;

  SetStyleFromObjField(d->style, d->Obj, id, "style");

  SetTextFromObjField(d->width, d->Obj, id, "width");

  SetListFromObjField(d->join, d->Obj, id, "join");

  SetTextFromObjField(d->miter, d->Obj, id, "miter_limit");

  set_color(d->color, d->Obj, id, NULL);

  n = d->Dir;
  if (n >=0 && n < 4) {
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
  gtk_range_set_value(GTK_RANGE(d->sch), d->Position * 100);
  gtk_range_set_value(GTK_RANGE(d->scv), d->Param * 1000);
}

static gboolean
LegendGaussDialogPaint(GtkWidget *w, GdkEventExpose *event, gpointer client_data)
{
  struct LegendGaussDialog *d;
  int i, j, k, pw, dw, minx, miny, maxx, maxy,
    amp, wd, GC, spnum, output, found;
  double ppd, x, y = 0, tmp, spc2[6];
  GdkPixmap *pix;
  GdkWindow *win;
  GdkColor black, white;
  GdkGC *gc;
  struct objlist *gobj, *robj;
  char *inst, *name;
  struct gra2cairo_local *local;

  d = (struct LegendGaussDialog *) client_data;
  win = w->window;

  found = find_gra2gdk_inst(&name, &gobj, &inst, &robj, &output, &local);
  if (! found) {
    return FALSE;
  }

  pix = gra2gdk_create_pixmap(gobj, inst, local, win,
			      VIEW_SIZE, VIEW_SIZE,
			      255, 255, 255);
  if (pix == NULL) {
    return FALSE;
  }

  gc = gdk_gc_new(win);

  black.red = 0;
  black.green = 0;
  black.blue = 0;

  white.red = 65535;
  white.green = 65535;
  white.blue = 65535;

  pw = VIEW_SIZE - 1;

  if (d->Wdx < d->Wdy)
    dw = d->Wdy;
  else
    dw = d->Wdx;


  ppd = pw / ((double) dw);
  minx = VIEW_SIZE / 2 - d->Wdx * ppd / 2;
  miny = VIEW_SIZE / 2 - d->Wdy * ppd / 2;
  maxx = VIEW_SIZE / 2 + d->Wdx * ppd / 2;
  maxy = VIEW_SIZE / 2 + d->Wdy * ppd / 2;

  gdk_gc_set_rgb_fg_color(gc, &white);
  gdk_draw_rectangle(pix, gc, TRUE, 0, 0, VIEW_SIZE, VIEW_SIZE);

  gdk_gc_set_rgb_fg_color(gc, &black);

  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_gc_set_dashes(gc, 0, Dashes, DashesNum);
  gdk_draw_rectangle(pix, gc, FALSE, minx, miny, maxx - minx, maxy - miny);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  if ((d->Dir == 0) || (d->Dir == 1)) {
    amp = d->Wdy;
    wd = d->Wdx;
  } else {
    amp = d->Wdx;
    wd = d->Wdy;
  }

  if (d->alloc) {
    GC = _GRAopen("gra2gdk", "_output",
		   robj, inst, output, -1,
		   -1, -1, NULL, local);
    GRAlinestyle(GC, 0, NULL, 1, 0, 0, 1000);
    if (GC >= 0) {
      GRAview(GC, minx, miny, maxx, maxy, 1);

      if (d->Div > DIV_MAX)
	d->Div = DIV_MAX;

      for (i = 0; i <= (d->Div); i++) {
	x = wd / ((double) (d->Div)) * i;
	if (d->Mode == 0) {
	  tmp =
	    (x - wd * 0.5 - wd * d->Position * 0.5) /
	    (wd / (1 + 10 * d->Param) / 2);
	  y = amp * exp(-tmp * tmp);
	} else if (d->Mode == 1) {
	  tmp =
	    (x - wd * 0.5 - wd * d->Position * 0.5) /
	    (wd / (1 + 10 * d->Param) / 2);
	  y = amp / (tmp * tmp + 1);
	} else if (d->Mode == 2) {
	  if (d->Position >= 0) {
	    tmp =
	      (x - wd * 0.5 - wd * d->Position * 0.5) /
	      (-wd * 0.5 - wd * d->Position * 0.5);
	  } else {
	    tmp =
	      (x - wd * 0.5 - wd * d->Position * 0.5) /
	      (wd * 0.5 - wd * d->Position * 0.5);
	  }
	  y = amp * tmp * tmp;
	} else if (d->Mode == 3) {
	  tmp = x / (wd / (0.25 + 10 * d->Param));
	  y = amp * 0.5 * (sin(2.0 * MPI * (tmp - d->Position * 0.5)) + 1);
	}

	if (d->Dir == 0) {
	  spx[i] = nround(x);
	  spy[i] = d->Wdy - nround(y);
	} else if (d->Dir == 1) {
	  spx[i] = nround(x);
	  spy[i] = nround(y);
	} else if (d->Dir == 2) {
	  spx[i] = d->Wdx - nround(y);
	  spy[i] = d->Wdy - nround(x);
	} else if (d->Dir == 3) {
	  spx[i] = nround(y);
	  spy[i] = d->Wdy - nround(x);
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
      GRAcolor(GC, 0, 0, 0);
      GRAcurvefirst(GC, 0, NULL, NULL, NULL,
		    splinedif, splineint, NULL, spx[0], spy[0]);
      for (j = 0; j < spnum - 1; j++) {
	for (k = 0; k < 6; k++)
	  spc2[k] = spc[k][j];
	if (!GRAcurve(GC, spc2, spx[j], spy[j]))
	  break;
      }
    }
    _GRAclose(GC);
    if (local->linetonum && local->cairo) {
      cairo_stroke(local->cairo);
      local->linetonum = 0;
    }
  }
  gdk_draw_drawable(win, gc, pix, 0, 0, 0, 0, VIEW_SIZE, VIEW_SIZE);

  g_object_unref(G_OBJECT(pix));
  g_object_unref(gc);

  return FALSE;
}

static gboolean
LegendGaussDialogScaleV(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Param = value / 1000.0;

  clear_view(d);

  return FALSE;
}

static gboolean
LegendGaussDialogScaleH(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Position = value / 100.0;

  clear_view(d);

  return FALSE;
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
  if (i < 0)
    return;

  d->Dir = i;

  clear_view(d);
}

static gboolean
LegendGaussDialogDiv(GtkWidget *w, GtkScrollType scroll, gdouble value, gpointer client_data)
{
  struct LegendGaussDialog *d;

  d = (struct LegendGaussDialog *) client_data;
  d->Div = value;

  clear_view(d);

  return FALSE;
}

static void
LegendGaussDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *button, *hbox, *hbox2, *vbox;
  GSList *func_list, *dir_list;
  struct LegendGaussDialog *d;
  char title[256];

  d = (struct LegendGaussDialog *) data;
  snprintf(title, sizeof(title), _("Legend Gaussian/Lorentzian/Parabola/Sin %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);

    button = NULL;

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Sin"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Parabola"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Lorentz"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), _("_Gauss"));
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogMode), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    func_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));

    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 0);
    d->func_list = func_list;

    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = combo_box_entry_create();
    item_setup(hbox2, w, _("Line _Style:"), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);
    d->style = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
    item_setup(hbox2, w, _("_Line Width:"), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);
    d->width = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
    item_setup(hbox2, w, _("_Miter:"), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);
    d->miter = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = combo_box_create();
    item_setup(hbox2, w, _("_Join:"), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);
    d->join = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = create_color_button(wi);
    item_setup(hbox2, w, _("_Color:"), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);
    d->color = w;

    hbox2 = gtk_hbox_new(FALSE, 4);
    w = gtk_hscale_new_with_range(10, DIV_MAX, 1);
    item_setup(hbox2, w, _("_Division:"), TRUE);
    g_signal_connect(w, "change-value", G_CALLBACK(LegendGaussDialogDiv), d);
    d->div = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);

    w = gtk_hscale_new_with_range(0, 1000, 1);
    g_signal_connect(w, "change-value", G_CALLBACK(LegendGaussDialogScaleV), d);
    d->scv = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    w = gtk_drawing_area_new();
    d->view = w;
    gtk_drawing_area_size(GTK_DRAWING_AREA(w), VIEW_SIZE, VIEW_SIZE);
    g_signal_connect(w, "expose-event", G_CALLBACK(LegendGaussDialogPaint), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    w = gtk_hscale_new_with_range(-100, 100, 1);
    g_signal_connect(w, "change-value", G_CALLBACK(LegendGaussDialogScaleH), d);
    d->sch = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 4);

    hbox2 = gtk_hbox_new(FALSE, 4);

    button = NULL;

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_R");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_L");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_B");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    button = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(button), "_T");
    g_signal_connect(button, "toggled", G_CALLBACK(LegendGaussDialogDir), d);
    gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

    dir_list = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
    d->dir_list = dir_list;

    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);
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

  if (SetObjFieldFromText(d->width, d->Obj, d->Id, "width"))
    return;

  if (SetObjFieldFromList(d->join, d->Obj, d->Id, "join"))
    return;

  if (SetObjFieldFromText(d->miter, d->Obj, d->Id, "miter_limit"))
    return;

  if (putobj_color(d->color, d->Obj, d->Id, NULL))
    return;

  if ((d->Dir == 0) || (d->Dir == 1)) {
    amp = d->Wdy;
    wd = d->Wdx;
  } else {
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
    if (d->Dir == 0) {
      gx = nround(x);
      gy = d->Wdy - nround(y);
    } else if (d->Dir == 1) {
      gx = nround(x);
      gy = nround(y);
    } else if (d->Dir == 2) {
      gx = d->Wdx - nround(y);
      gy = d->Wdy - nround(x);
    } else if (d->Dir == 3) {
      gx = nround(y);
      gy = d->Wdy - nround(x);
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
    memfree(spc[i]);

  memfree(spz);
  memfree(spy);
  memfree(spx);
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
  data->Dir = 0;
  data->Mode = 0;
  data->alloc = TRUE;

  spx = (double *) memalloc(sizeof(double) * (DIV_MAX + 1));
  if (spx == NULL)
    data->alloc = FALSE;

  spy = (double *) memalloc(sizeof(double) * (DIV_MAX + 1));
  if (spy == NULL)
    data->alloc = FALSE;

  spz = (double *) memalloc(sizeof(double) * (DIV_MAX + 1));
  if (spz == NULL)
    data->alloc = FALSE;

  for (i = 0; i < 6; i++) {
    if ((spc[i] = (double *) memalloc(sizeof(double) * 201)) == NULL) {
      data->alloc = FALSE;
    }
  }
}
