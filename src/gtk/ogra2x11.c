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

#include "object.h"
#include "ioutil.h"
#include "gra.h"
#include "nstring.h"
#include "mathfn.h"
#include "nconfig.h"
#include "shell.h"

#include "init.h"
#include "gtk_widget.h"
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
#define WINSIZE_MAX 10000

static char *gtkerrorlist[] = {
  "cannot open display.",
  "not enough color cell.",
};

#define ERRNUM (sizeof(gtkerrorlist)/sizeof(*gtkerrorlist))

struct gtklocal
{
  struct objlist *obj;
  GdkCursor *blank_cursor;
  N_VALUE *inst;
  GtkWidget *mainwin, *View;
  cairo_surface_t *surface;
  char *title;
  int redraw, fit, frame;
  unsigned int windpi;
  int PaperWidth, PaperHeight;
  double bg_r, bg_g, bg_b;
  struct gra2cairo_local *local;
  int quit_main_loop;
#if OSX
  int menulock;
#endif
};

static void gtkMakeRuler(cairo_t *cr, struct gtklocal *gtklocal);
static int gtkclose(GtkWidget *widget, gpointer user_data);
static void gtkchangedpi(struct gtklocal *gtklocal);
static gboolean gtkevpaint(struct gtklocal *gtklocal, cairo_t * e);
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
static void resized(GtkWidget *widget, int w, int h, gpointer user_data);

static gboolean
gtkevpaint(struct gtklocal *gtklocal, cairo_t *cr)
{
  if (gtklocal->surface == NULL) {
    return FALSE;
  }

  if (gtklocal->redraw) {
    GRAredraw(gtklocal->obj, gtklocal->inst, TRUE, 0);
    gtklocal->redraw = FALSE;
  }

  cairo_set_source_surface(cr, gtklocal->surface, 0, 0);
  cairo_paint(cr);
  gtkMakeRuler(cr, gtklocal);

  return FALSE;
}

static void
destroyed(GtkWidget *win, gpointer user_data)
{
  struct gtklocal *local;
  local = (struct gtklocal *) user_data;
  if (local->quit_main_loop) {
    g_idle_add_once((GSourceOnceFunc) g_main_loop_quit, main_loop());
  }
  local->mainwin = NULL;
}

static int
gtkclose(GtkWidget *widget, gpointer user_data)
{
  struct gtklocal *local;
  int i, id;
  struct objlist *obj;

  obj = chkobject("gra2gtk");
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    N_VALUE *inst;
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

static void
size_allocate(GtkWidget *widget, const GdkRectangle *allocation, gpointer user_data)
{
  double w, h;
  w = allocation->width;
  h = allocation->height;
  resized(widget, w, h, user_data);
}

static void
resized(GtkWidget *widget, int w, int h, gpointer user_data)
{
  struct gtklocal *local;
  double rh, rw, ratio;
  int pw, ph, dpi;

  local = (struct gtklocal *) user_data;
  if (local == NULL) {
    return;
  }

  if (! local->fit) {
    return;
  }

  pw = local->PaperWidth;
  ph = local->PaperHeight;

  if (pw < 1 || ph < 1) {
    return;
  }

  rw = 1.0 * w / pw;
  rh = 1.0 * h / ph;

  ratio = (rh > rw) ? rw : rh;
  dpi = ratio * DPI_MAX;

  local->windpi = dpi;
  local->local->pixel_dot_x =
    local->local->pixel_dot_y = ratio;

  _putobj(local->obj, "dpi", local->inst, &dpi);

  gtkchangedpi(local);

}

static void
draw_function(GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height, gpointer user_data)
{
  (void) drawing_area;
  (void) width;
  (void) height;
  gtkevpaint(user_data, cr);
}

static int
gtkinit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  struct gra2cairo_local *local;
  int oid, width, height, delete_gra;
  GtkWidget *scrolled_window = NULL;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  gtklocal = g_malloc(sizeof(*gtklocal));
  if (gtklocal == NULL)
    goto errexit;

  gtklocal->obj = obj;
  gtklocal->inst = inst;
  gtklocal->title = NULL;
  gtklocal->mainwin = NULL;
  gtklocal->fit = FALSE;
  gtklocal->frame = TRUE;
  gtklocal->blank_cursor = NULL;
  gtklocal->quit_main_loop = FALSE;

  if (_putobj(obj, "_gtklocal", inst, gtklocal))
    goto errexit;

  if (_getobj(obj, "oid", inst, &oid))
    goto errexit;

  if (_getobj(obj, "_local", inst, &local))
    goto errexit;

  if (_getobj(obj, "fit", inst, &gtklocal->fit))
    goto errexit;

  if (_getobj(obj, "frame", inst, &gtklocal->frame))
    goto errexit;

  gtklocal->PaperWidth = 0;
  gtklocal->PaperHeight = 0;
  gtklocal->windpi = DEFAULT_DPI / 2;
  gtklocal->bg_r = 1.0;
  gtklocal->bg_g = 1.0;
  gtklocal->bg_b = 1.0;
  gtklocal->local = local;
#if OSX
  gtklocal->menulock = FALSE;
#endif

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

  delete_gra = TRUE;
  if (_putobj(obj, "delete_gra", inst, &delete_gra))
    goto errexit;

  local->pixel_dot_x =
  local->pixel_dot_y = gtklocal->windpi / (DPI_MAX * 1.0);

  width = WINWIDTH;
  height = WINHEIGHT;

  if (_putobj(obj, "width", inst, &width))
    goto errexit;

  if (_putobj(obj, "height", inst, &height))
    goto errexit;

  if (! OpenApplication()) {
    error(obj, ERRDISPLAY);
    goto errexit;
  }

  gtklocal->title = mkobjlist(obj, NULL, oid, NULL, TRUE);

  gtklocal->mainwin = gtk_window_new();
  if (TopLevel) {
    gtk_window_set_modal(GTK_WINDOW(gtklocal->mainwin), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(gtklocal->mainwin), GTK_WINDOW(TopLevel));
#if OSX
    gtklocal->menulock = Menulock;
    Menulock = TRUE;
#endif
  }
  gtk_window_set_default_size(GTK_WINDOW(gtklocal->mainwin), width, height);
  g_signal_connect(gtklocal->mainwin, "close_request", G_CALLBACK(gtkclose), gtklocal);
  g_signal_connect(gtklocal->mainwin, "destroy", G_CALLBACK(destroyed), gtklocal);

  gtk_window_set_title((GtkWindow *) gtklocal->mainwin, gtklocal->title);

  scrolled_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_window_set_child(GTK_WINDOW(gtklocal->mainwin), scrolled_window);

  gtklocal->View = gtk_drawing_area_new();
  gtk_widget_set_halign(gtklocal->View, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(gtklocal->View, GTK_ALIGN_CENTER);

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(gtklocal->View), draw_function, gtklocal, NULL);
  gtk_widget_set_can_focus(gtklocal->View, TRUE);
  g_signal_connect(gtklocal->View, "resize", G_CALLBACK(resized), gtklocal);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), gtklocal->View);

  gtklocal->surface= NULL;
  gtklocal->redraw = TRUE;

  gtkchangedpi(gtklocal);

  return 0;

errexit:
  if (gtklocal) {
    if (gtklocal->mainwin) {
      gtk_window_destroy(GTK_WINDOW(gtklocal->mainwin));
      gtklocal->mainwin = NULL;
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

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  if (gtklocal->blank_cursor) {
    g_object_unref(gtklocal->blank_cursor);
  }

  if (gtklocal->mainwin != NULL) {
    gtk_window_destroy(GTK_WINDOW(gtklocal->mainwin));
    gtklocal->mainwin = NULL;
  }

  if (gtklocal->surface) {
    cairo_surface_destroy(gtklocal->surface);
  }

  g_free(gtklocal->title);

#if OSX
  if (TopLevel) {
    Menulock = gtklocal->menulock;
  }
#endif
  return 0;
}

static void
clear_pixmap(struct gtklocal *local)
{
  cairo_t *cr;

  cr = cairo_create(local->surface);
  cairo_set_source_rgb(cr, local->bg_r, local->bg_g, local->bg_b);
  cairo_paint(cr);
  cairo_destroy(cr);
}

static void
redraw_window(struct gtklocal *local)
{
  if (local->View == NULL) {
    return;
  }

  gtk_widget_queue_draw(local->View);
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
  redraw_window(local);

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

  if (local->mainwin == NULL)
    return 1;

  gtk_window_present(GTK_WINDOW(local->mainwin));

  if (main_loop_is_running()) {
    local->quit_main_loop = FALSE;
  } else {
    local->quit_main_loop = TRUE;
    main_loop_run();
  }

  return 0;
}

static int
gtkfullscreen(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;
  int state;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_gtklocal", inst, &local))
    return 1;

  if (local->mainwin == NULL)
    return 1;

  state = *(int *) argv[2];

  if (state) {
    gtk_window_fullscreen(GTK_WINDOW(local->mainwin));
  } else {
    gtk_window_unfullscreen(GTK_WINDOW(local->mainwin));
  }

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

  redraw_window(local);

  return 0;
}

static double
get_color(const struct gtklocal *gtklocal, int argc, char **argv)
{
  int c;

  c = abs(*(int *) argv[2]);

  if (c > 255) {                /* c >= 0 */
    c = 255;
  }

  return c / 255.0;
}

static int
gtkbg(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  const char *field;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  field = argv[1];
  switch(field[1]) {
  case 'R':
    gtklocal->bg_r = get_color(gtklocal, argc, argv);
    break;
  case 'G':
    gtklocal->bg_g = get_color(gtklocal, argc, argv);
    break;
  case 'B':
    gtklocal->bg_b = get_color(gtklocal, argc, argv);
    break;
  }
  return 0;
}

static int
gtkredraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_gtklocal", inst, &gtklocal))
    return 1;

  redraw_window(gtklocal);
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
  cairo_surface_t *pixmap;


  if ((gtklocal->PaperWidth == 0) || (gtklocal->PaperHeight == 0)) {
    return;
  }

  width = dot2pixel(gtklocal, gtklocal->PaperWidth);
  height = dot2pixel(gtklocal, gtklocal->PaperHeight);

  pixmap = gtklocal->surface;
  if (pixmap) {
    int pw, ph;

    ph = cairo_image_surface_get_height(pixmap);
    pw = cairo_image_surface_get_width(pixmap);

    if (pw != width || ph != height) {
      cairo_surface_destroy(pixmap);
      pixmap = NULL;
    }
  }

  if (pixmap == NULL) {
    pixmap = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    gtklocal->surface = pixmap;

    clear_pixmap(gtklocal);

    gtklocal->redraw = TRUE;

    if (gtklocal->local->cairo) {
      cairo_destroy(gtklocal->local->cairo);
    }

    gtklocal->local->cairo = cairo_create(pixmap);
  }

  gtk_widget_set_size_request(gtklocal->View, width, height);
}

static void
gtkMakeRuler(cairo_t *cr, struct gtklocal *gtklocal)
{
  int width, height;

  width = gtklocal->PaperWidth;
  height = gtklocal->PaperHeight;

  if (width == 0 || height == 0 || gtklocal->surface == NULL || ! gtklocal->frame)
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
  char code;
  const int *cpar;
  struct gtklocal *gtklocal;

  code = *(char *) (argv[3]);
  cpar = (int *) argv[4];

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
  int dpi, fit;
  struct gtklocal *local;

  dpi = abs(*(int *) argv[2]);

  _getobj(obj, "_gtklocal", inst, &local);

  fit = FALSE;
  local->fit = FALSE;
  _putobj(obj, "fit", inst, &fit);

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

static int
gtk_set_fit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int fit;
  struct gtklocal *local;

  _getobj(obj, "_gtklocal", inst, &local);

  fit = *(int *) argv[2];

  local->fit = fit;

  if (fit && local->mainwin) {
    GdkRectangle allocation;
    allocation.width = gtk_widget_get_width (GTK_WIDGET (local->mainwin));
    allocation.height = gtk_widget_get_height (GTK_WIDGET (local->mainwin));
    size_allocate(local->mainwin, &allocation, local);
  }

  return 0;
}

static int
gtk_set_frame(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;

  _getobj(obj, "_gtklocal", inst, &local);

  local->frame = *(int *) argv[2];

  return 0;
}

static int
gtk_set_size(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int width, height, size;
  struct gtklocal *local;

  _getobj(obj, "_gtklocal", inst, &local);

  size = *(int *) argv[2];
  if (size < 1 || size > WINSIZE_MAX) {
    return 1;
  }

  if (local->mainwin == NULL)
    return 1;

  switch (argv[1][0]) {
  case 'h':
    height = size;
    _getobj(obj, "width", inst, &width);
    gtk_window_set_default_size(GTK_WINDOW(local->mainwin), width, height);
    break;
  case 'w':
    width = size;
    _getobj(obj, "height", inst, &height);
    gtk_window_set_default_size(GTK_WINDOW(local->mainwin), width, height);
    break;
  }

  return 0;
}

static struct objtable gra2gtk[] = {
  {"init", NVFUNC, NEXEC, gtkinit, NULL, 0},
  {"done", NVFUNC, NEXEC, gtkdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"dpix", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"dpiy", NINT, NREAD | NWRITE, gtk_set_dpi, NULL, 0},
  {"width", NINT, NREAD | NWRITE, gtk_set_size, NULL, 0},
  {"height", NINT, NREAD | NWRITE, gtk_set_size, NULL, 0},
  {"fit", NBOOL, NREAD | NWRITE, gtk_set_fit, NULL, 0},
  {"frame", NBOOL, NREAD | NWRITE, gtk_set_frame, NULL, 0},
  {"expose", NVFUNC, NREAD | NEXEC, gtkredraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, gtkflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, gtkclear, "", 0},
  {"present", NVFUNC, NREAD | NEXEC, gtkpresent, "", 0},
  {"fullscreen", NVFUNC, NREAD | NEXEC, gtkfullscreen, "b", 0},
  {"BR", NINT, NREAD | NWRITE, gtkbg, NULL, 0},
  {"BG", NINT, NREAD | NWRITE, gtkbg, NULL, 0},
  {"BB", NINT, NREAD | NWRITE, gtkbg, NULL, 0},
  {"_gtklocal", NPOINTER, 0, NULL, NULL, 0},
  {"_output", NVFUNC, 0, gtk_output, NULL, 0},
  {"_strwidth", NIFUNC, 0, gra2cairo_strwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
};

#define TBLNUM (sizeof(gra2gtk) / sizeof(*gra2gtk))

void *
addgra2gtk(void)
/* addgra2gtk() returns NULL on error */
{
  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gra2gtk,
		   ERRNUM, gtkerrorlist, NULL, NULL);
}
