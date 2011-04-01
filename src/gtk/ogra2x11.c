/* 
 * $Id: ogra2x11.c,v 1.34 2010-01-04 05:11:28 hito Exp $
 * 
 * This file is part of "Ngraph for GTK".
 * 
 * Copyright (C) 2002,  Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for GTK" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License,  or (at your option) any later version.
 * 
 * "Ngraph for GTK" is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  write to the Free Software
 * Foundation,  Inc.,  59 Temple Place - Suite 330,  Boston,  MA  02111-1307,  USA.
 * 
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "strconv.h"

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "gra.h"
#include "nstring.h"
#include "mathfn.h"
#include "nconfig.h"
#include "shell.h"

#include "main.h"
#include "x11menu.h"
#include "x11gui.h"
#include "ogra2x11.h"
#include "ogra2cairo.h"

#define NAME "gra2gtk"
#define ALIAS "gra2win:gra2x11"
#define PARENT "gra2cairo"
#define NVERSION  "1.00.00"
#define GRA2GTKCONF "[gra2gtk]"

#define ERRDISPLAY 100
#define ERRCMAP 101

#define WINWIDTH 578
#define WINHEIGHT 819

static char *gtkerrorlist[] = {
  "cannot open display.",
  "not enough color cell.",
};

#define ERRNUM (sizeof(gtkerrorlist)/sizeof(*gtkerrorlist))


struct gtklocal
{
  struct objlist *obj;
  N_VALUE *inst;
  GtkWidget *mainwin, *View;
  GdkPixmap *win;
  GdkWindow *window;
  char *title;
  int redraw;
  unsigned int winwidth, winheight, windpi;
  int PaperWidth, PaperHeight;
  double bg_r, bg_g, bg_b;
  struct gra2cairo_local *local;
};

static void gtkMakeRuler(cairo_t *cr, struct gtklocal *gtklocal);
static int gtk_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		      char **argv);
static int gtkloadconfig(struct gtklocal *gtklocal);
static int gtkclose(GtkWidget *widget, GdkEvent  *event, gpointer user_data);
static void gtkchangedpi(struct gtklocal *gtklocal);
static gboolean gtkevpaint(GtkWidget * w, GdkEventExpose * e,
			   gpointer user_data);
static int gtkinit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		   char **argv);
static int gtkdone(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		   char **argv);
static int gtkclear(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		    char **argv);
static int gtkredraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		     char **argv);
static int dot2pixel(struct gtklocal *gtklocal, int r);
static int gtk_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		      char **argv);

static int
gtkloadconfig(struct gtklocal *gtklocal)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1;
  int val;
  char *endptr;
  int len;

  fp = openconfig(GRA2GTKCONF);
  if (fp == NULL)
    return 0;

  while ((tok = getconfig(fp, &str)) != NULL) {
    s2 = str;
    if  (strcmp(tok, "win_dpi") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->windpi = val;
      g_free(f1);
    } else if (strcmp(tok, "win_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->winwidth = val;
      g_free(f1);
    } else if (strcmp(tok, "win_height") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->winheight = val;
      g_free(f1);
    } else if (strcmp(tok, "use_external_viewer") == 0) {
    } else {
      fprintf(stderr, "(%s): configuration '%s' in section %s is not used.\n", AppName, tok, GRA2GTKCONF);
    }
    g_free(tok);
    g_free(str);
  }
  closeconfig(fp);
  return 0;
}

static gboolean
gtkevpaint(GtkWidget *w, GdkEventExpose *e, gpointer user_data)
{
  struct gtklocal *gtklocal;
  cairo_t *cr;

  gtklocal = (struct gtklocal *) user_data;

  if (e->count != 0)
    return TRUE;

  if (gtklocal->win == NULL)
    return FALSE;

  cr = gdk_cairo_create(GTK_WIDGET_GET_WINDOW(w));

  if (gtklocal->redraw) {
    GRAredraw(gtklocal->obj, gtklocal->inst, FALSE, FALSE);
    gtklocal->redraw = FALSE;
  }

  gdk_cairo_set_source_pixmap(cr, gtklocal->win, 0, 0);
  gdk_cairo_region(cr, e->region);
  cairo_fill(cr);
  gtkMakeRuler(cr, gtklocal);
  cairo_destroy(cr);

  return FALSE;
}

static int
gtkclose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  N_VALUE *inst;
  struct gtklocal *local;
  int i, id;
  struct objlist *obj;

  obj = chkobject("gra2gtk");
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    inst = chkobjinst(obj, i);
    _getobj(obj, "_gtklocal", inst, &local);
    if (local->mainwin == widget) {
      _getobj(obj, "id", inst, &id);
      delobj(obj, id);
      break;
    }
  }
  return FALSE;
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_w:
    if (e->state & GDK_CONTROL_MASK) {
      gtk_widget_destroy(gtklocal->mainwin);
      gtklocal->mainwin = NULL;
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}

static int
gtkinit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  struct gra2cairo_local *local;
  GdkWindow *win;
  struct objlist *robj;
  int idn, oid;
  GtkWidget *scrolled_window = NULL, *vbox = NULL;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  gtklocal = g_malloc(sizeof(*gtklocal));
  if (gtklocal == NULL)
    goto errexit;

  gtklocal->obj = obj;
  gtklocal->inst = inst;
  gtklocal->title = NULL;
  gtklocal->mainwin = NULL;

  if (_putobj(obj, "_gtklocal", inst, gtklocal))
    goto errexit;

  if (_getobj(obj, "oid", inst, &oid))
    goto errexit;

  if (_getobj(obj, "_local", inst, &local))
    goto errexit;

  gtklocal->PaperWidth = 0;
  gtklocal->PaperHeight = 0;
  gtklocal->winwidth = WINWIDTH;
  gtklocal->winheight = WINHEIGHT;
  gtklocal->windpi = DEFAULT_DPI;
  gtklocal->bg_r = 1.0;
  gtklocal->bg_g = 1.0;
  gtklocal->bg_b = 1.0;
  gtklocal->local = local;

  if (gtkloadconfig(gtklocal))
    goto errexit;

  if (gtklocal->windpi < 1)
    gtklocal->windpi = 1;

  if (gtklocal->windpi > DPI_MAX)
    gtklocal->windpi = DPI_MAX;

  if (_putobj(obj, "dpi", inst, &(gtklocal->windpi)))
    goto errexit;

  if (_putobj(obj, "dpix", inst, &(gtklocal->windpi)))
    goto errexit;

  if (_putobj(obj, "dpiy", inst, &(gtklocal->windpi)))
    goto errexit;

  local->pixel_dot_x = 
  local->pixel_dot_y = gtklocal->windpi / (DPI_MAX * 1.0);

  if (gtklocal->winwidth < 1)
    gtklocal->winwidth = 1;

  if (gtklocal->winwidth > 10000)
    gtklocal->winwidth = 10000;

  if (gtklocal->winheight < 1)
    gtklocal->winheight = 1;

  if (gtklocal->winheight > 10000)
    gtklocal->winheight = 10000;

  if (! OpenApplication()) {
    error(obj, ERRDISPLAY);
    goto errexit;
  }

  gtklocal->title = mkobjlist(obj, NULL, oid, NULL, TRUE);

  gtklocal->mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped(gtklocal->mainwin,
			   "delete-event",
			   G_CALLBACK(gtkclose), gtklocal->mainwin);

  g_signal_connect(gtklocal->mainwin, "key-press-event", G_CALLBACK(ev_key_down), gtklocal);

  gtk_window_set_title((GtkWindow *) gtklocal->mainwin, gtklocal->title);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request((GtkWidget *) scrolled_window,
			      gtklocal->winwidth, gtklocal->winheight);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gtklocal->mainwin), vbox);

  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  gtklocal->View = gtk_drawing_area_new();
  g_signal_connect(gtklocal->View,
		   "expose-event", G_CALLBACK(gtkevpaint), gtklocal);

  gtk_widget_set_size_request(gtklocal->View,
			      gtklocal->winwidth, gtklocal->winheight);

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					(scrolled_window), gtklocal->View);

  gtk_widget_show_all(gtklocal->mainwin);

  win = GTK_WIDGET_GET_WINDOW(gtklocal->View);

  gtklocal->win = NULL;
  gtklocal->window = win;
  gtklocal->redraw = TRUE;

  if (chkobjfield(obj, "_evloop")) {
    goto errexit;
  }

  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn == -1) {
    goto errexit;
  }

  registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, gtklocal);

  gtkchangedpi(gtklocal);

  gtk_evloop(obj, inst, NULL, argc, argv);
  return 0;

errexit:
  if (gtklocal) {
    if (gtklocal->mainwin) {
      gtk_widget_destroy(gtklocal->mainwin);
    }

    if (gtklocal->mainwin) {
      g_free(gtklocal->title);
    }
    g_free(gtklocal);
  }

  local = gra2cairo_free(obj, inst);
  g_free(local);

  return 1;
}

static int
gtkdone(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  int idn;
  struct objlist *robj;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  if (gtklocal->mainwin != NULL) {
    gtk_widget_destroy(gtklocal->mainwin);
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }

  if (gtklocal->win)
    g_object_unref(gtklocal->win);

  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn != -1)
    unregisterevloop(robj, idn, inst);

  g_free(gtklocal->title);

  return 0;
}

static void
clear_pixmap(struct gtklocal *local)
{
  cairo_t *cr;

  cr = gdk_cairo_create(local->win);
  cairo_set_source_rgb(cr, local->bg_r, local->bg_g, local->bg_b);
  cairo_paint(cr);
  cairo_destroy(cr);
}

static int
gtkclear(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &local))
    return 1;

  clear_pixmap(local);

  gdk_window_invalidate_rect(local->window, NULL, TRUE);

  return 0;
}

static int
gtkpresent(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &local))
    return 1;

  reset_event();
  msleep(100);
  gtk_window_present(GTK_WINDOW(local->mainwin));

  return 0;
}

static int
gtkflush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &local))
    return 1;

  gdk_window_invalidate_rect(local->window, NULL, TRUE);
  return 0;
}

static double
get_color(struct gtklocal *gtklocal, int argc, char **argv)
{
  int c;

  c = abs(*(int *) argv[2]);

  if (c < 0) {
    c = 0;
  } else if (c > 255) {
    c = 255;
  }

  return c / 255.0;
}


static int
gtkbr(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  gtklocal->bg_r = get_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkbb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  gtklocal->bg_b = get_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkbg(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  gtklocal->bg_g = get_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkredraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  gdk_window_invalidate_rect(gtklocal->window, NULL, TRUE);
  return 0;
}

static int
gtk_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
  return 0;
}

static int
dot2pixel(struct gtklocal *gtklocal, int r)
{
  return nround(r * gtklocal->local->pixel_dot_x);
}

static void
gtkchangedpi(struct gtklocal *gtklocal)
{
  int width, height;
  GdkPixmap *pixmap;


  if ((gtklocal->PaperWidth == 0) || (gtklocal->PaperHeight == 0)) {
    return;
  }

  width = dot2pixel(gtklocal, gtklocal->PaperWidth);
  height = dot2pixel(gtklocal, gtklocal->PaperHeight);

  pixmap = gtklocal->win;
  if (pixmap) {
    int pw, ph;

    gdk_drawable_get_size(GDK_DRAWABLE(pixmap), &pw, &ph);
    if (pw != width || ph != height) {
      g_object_unref(G_OBJECT(pixmap));
      pixmap = NULL;
    }
  }

  if (pixmap == NULL) {
    pixmap = gdk_pixmap_new(gtklocal->window, width, height, -1);
    gtklocal->win = pixmap;

    clear_pixmap(gtklocal);

    gtklocal->redraw = TRUE;

    if (gtklocal->local->cairo) {
      cairo_destroy(gtklocal->local->cairo);
    }

    gtklocal->local->cairo = gdk_cairo_create(pixmap);
  }

  gtk_widget_set_size_request(gtklocal->View, width, height);
}

static void
gtkMakeRuler(cairo_t *cr, struct gtklocal *gtklocal)
{
  int width, height;
  GdkWindow *win;

  width = gtklocal->PaperWidth;
  height = gtklocal->PaperHeight;
  win = gtklocal->win;

  if (width == 0 || height == 0 || win == NULL)
    return;

  cairo_save(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_rectangle(cr,
		  CAIRO_COORDINATE_OFFSET, CAIRO_COORDINATE_OFFSET,
		  dot2pixel(gtklocal, width) - 1, dot2pixel(gtklocal, height) - 1);
  cairo_stroke(cr);
  cairo_restore(cr);
}

static int
gtk_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char code, *cstr;
  int *cpar;
  struct gtklocal *gtklocal;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *) argv[2];

  code = *(char *) (argv[3]);
  cpar = (int *) argv[4];
  cstr = argv[5];

  switch (code) {
  case 'I':
    if (_getobj(obj, "_gtklocal", inst, &gtklocal))
      return 1;

    gtklocal->PaperWidth = cpar[3];
    gtklocal->PaperHeight = cpar[4];
    gtkchangedpi(gtklocal);
    break;
  case 'E':
    break;
  }

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static int
gtk_set_dpi(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int dpi;
  struct gtklocal *local;

  dpi = abs(*(int *) argv[2]);

  _getobj(obj, "_gtklocal", inst, &local);

  if (dpi < 1)
    dpi = 1;
  if (dpi > DPI_MAX)
    dpi = DPI_MAX;

  local->windpi = dpi;
  local->local->pixel_dot_x =
    local->local->pixel_dot_y = dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;

  gtkchangedpi(local);

  return 0;
}

static struct objtable gra2gtk[] = {
  {"init", NVFUNC, NEXEC, gtkinit, NULL, 0},
  {"done", NVFUNC, NEXEC, gtkdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"dpix", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"dpiy", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"expose", NVFUNC, NREAD | NEXEC, gtkredraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, gtkflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, gtkclear, "", 0},
  {"present", NVFUNC, NREAD | NEXEC, gtkpresent, "", 0},
  {"BR", NINT, NREAD | NWRITE, gtkbr, NULL, 0},
  {"BG", NINT, NREAD | NWRITE, gtkbg, NULL, 0},
  {"BB", NINT, NREAD | NWRITE, gtkbb, NULL, 0},
  {"_gtklocal", NPOINTER, 0, NULL, NULL, 0},
  {"_output", NVFUNC, 0, gtk_output, NULL, 0},
  {"_strwidth", NIFUNC, 0, gra2cairo_strwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_evloop", NVFUNC, 0, gtk_evloop, NULL, 0},
};

#define TBLNUM (sizeof(gra2gtk) / sizeof(*gra2gtk))

void *
addgra2gtk(void)
/* addgra2gtk() returns NULL on error */
{
  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gra2gtk,
		   ERRNUM, gtkerrorlist, NULL, NULL);
}
