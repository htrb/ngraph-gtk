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

struct action_type
{
  enum {
    ACTION_TYPE_NONE,
    ACTION_TYPE_KEY,
    ACTION_TYPE_BUTTON,
    ACTION_TYPE_SCROLL,
  } type;
  int val;
};

struct gtklocal
{
  struct objlist *obj;
  GdkCursor *blank_cursor;
  N_VALUE *inst;
  GtkWidget *mainwin, *View;
  cairo_surface_t *surface;
  GdkWindow *window;
  char *title;
  int redraw, fit, frame;
  unsigned int windpi;
  int PaperWidth, PaperHeight;
  struct action_type action;
  double bg_r, bg_g, bg_b;
  struct gra2cairo_local *local;
#if OSX
  int menulock;
#endif
};

static void gtkMakeRuler(cairo_t *cr, struct gtklocal *gtklocal);
static int gtk_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		      char **argv);
static int gtkclose(GtkWidget *widget, GdkEvent  *event, gpointer user_data);
static void gtkchangedpi(struct gtklocal *gtklocal);
static gboolean gtkevpaint(GtkWidget * w, cairo_t * e,
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

static gboolean
gtkevpaint(GtkWidget *w, cairo_t *cr, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

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

static int
gtkclose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
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
  return TRUE;
}

#if GTK_CHECK_VERSION(3, 24, 0)
static gboolean
ev_key_down(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  gtklocal->action.type = ACTION_TYPE_KEY;
  gtklocal->action.val = keyval;

  switch (keyval) {
  case GDK_KEY_w:
    if (state & GDK_CONTROL_MASK) {
      gtkclose(gtklocal->mainwin, NULL, NULL);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}
#else
static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;

  gtklocal->action.type = ACTION_TYPE_KEY;
  gtklocal->action.val = e->keyval;

  switch (e->keyval) {
  case GDK_KEY_w:
    if (e->state & GDK_CONTROL_MASK) {
      gtkclose(w, NULL, NULL);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}
#endif

void
size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
  struct gtklocal *local;
  double w, h, rh, rw, ratio;
  int pw, ph, dpi;

  local = (struct gtklocal *) user_data;
  if (local == NULL) {
    return;
  }

  if (! local->fit) {
    return;
  }

  w = allocation->width;
  h = allocation->height;

  pw = local->PaperWidth;
  ph = local->PaperHeight;

  if (pw < 1 || ph < 1) {
    return;
  }

  rw = w / pw;
  rh = h / ph;

  ratio = (rh > rw) ? rw : rh;
  dpi = ratio * DPI_MAX;

  local->windpi = dpi;
  local->local->pixel_dot_x =
    local->local->pixel_dot_y = ratio;

  _putobj(local->obj, "dpi", local->inst, &dpi);

  gtkchangedpi(local);

}

static gboolean
cursor_moved(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  if (gtklocal->blank_cursor) {
    gdk_window_set_cursor(gtk_widget_get_window(gtklocal->mainwin), NULL);
    g_object_unref(gtklocal->blank_cursor);
    gtklocal->blank_cursor = NULL;
  }

  return FALSE;
}

#if GTK_CHECK_VERSION(3, 24, 0)
static void
button_released(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct gtklocal *gtklocal;
  guint button;

  gtklocal = (struct gtklocal *) user_data;
  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  gtklocal->action.type = ACTION_TYPE_BUTTON;
  gtklocal->action.val = button;
}
#else
static gboolean
button_released(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  gtklocal->action.type = ACTION_TYPE_BUTTON;
  gtklocal->action.val = event->button;

  return FALSE;
}
#endif

static gboolean
scrolled(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  switch (event->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_LEFT:
  case GDK_SCROLL_RIGHT:
    gtklocal->action.type = ACTION_TYPE_SCROLL;
    gtklocal->action.val = event->direction;
    break;
  case GDK_SCROLL_SMOOTH:
    break;
  }

  return FALSE;
}

#if GTK_CHECK_VERSION(3, 24, 0)
static void
add_event_key(GtkWidget *widget, struct gtklocal *gtklocal)
{
  GtkEventController *ev;

  ev = gtk_event_controller_key_new(widget);
  g_signal_connect(ev, "key-pressed", G_CALLBACK(ev_key_down), gtklocal);
}

#endif

static int
gtkinit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  struct gra2cairo_local *local;
  struct objlist *robj;
  int idn, oid, width, height;
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
  gtklocal->action.type = ACTION_TYPE_NONE;
  gtklocal->action.val = 0;

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

  gtklocal->mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (TopLevel) {
    gtk_window_set_modal(GTK_WINDOW(gtklocal->mainwin), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(gtklocal->mainwin), GTK_WINDOW(TopLevel));
#if OSX
    gtklocal->menulock = Menulock;
    Menulock = TRUE;
#endif
  }
  gtk_window_set_default_size(GTK_WINDOW(gtklocal->mainwin), width, height);
  g_signal_connect_swapped(gtklocal->mainwin,
			   "delete-event",
			   G_CALLBACK(gtkclose), gtklocal->mainwin);

#if GTK_CHECK_VERSION(3, 24, 0)
  add_event_key(gtklocal->mainwin, gtklocal);
#else
  g_signal_connect(gtklocal->mainwin, "key-press-event", G_CALLBACK(ev_key_down), gtklocal);
#endif

  gtk_window_set_title((GtkWindow *) gtklocal->mainwin, gtklocal->title);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(gtklocal->mainwin), scrolled_window);

  gtklocal->View = gtk_drawing_area_new();
  gtk_widget_set_halign(gtklocal->View, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(gtklocal->View, GTK_ALIGN_CENTER);

  g_signal_connect(gtklocal->View, "draw",
		   G_CALLBACK(gtkevpaint), gtklocal);

  g_signal_connect(gtklocal->mainwin, "size-allocate",
		   G_CALLBACK(size_allocate), gtklocal);

  gtk_container_add(GTK_CONTAINER(scrolled_window), gtklocal->View);

  gtk_widget_show_all(gtklocal->mainwin);

  gtklocal->surface= NULL;
  gtklocal->redraw = TRUE;

  gtk_widget_add_events(gtklocal->mainwin, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect(gtklocal->mainwin, "motion-notify-event", G_CALLBACK(cursor_moved), gtklocal);
  g_signal_connect(gtklocal->mainwin, "button-release-event", G_CALLBACK(button_released), gtklocal);
  g_signal_connect(gtklocal->mainwin, "scroll-event", G_CALLBACK(scrolled), gtklocal);

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

  if (gtklocal->blank_cursor) {
    g_object_unref(gtklocal->blank_cursor);
  }

  if (gtklocal->mainwin != NULL) {
    gtk_widget_destroy(gtklocal->mainwin);
    gtklocal->mainwin = NULL;
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }

  if (gtklocal->surface) {
    cairo_surface_destroy(gtklocal->surface);
  }

  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn != -1)
    unregisterevloop(robj, idn, inst);

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
  GdkWindow *win;

  if (local->View == NULL) {
    return;
  }

  win = gtk_widget_get_window(local->View);
  if (win == NULL) {
    return;
  }

  gdk_window_invalidate_rect(win, NULL, TRUE);
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

  reset_event();
  msleep(100);
  gtk_window_present(GTK_WINDOW(local->mainwin));

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
  int *cpar;
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

  if (fit) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(local->mainwin, &allocation);
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

  switch (argv[1][0]) {
  case 'h':
    height = size;
    _getobj(obj, "width", inst, &width);
    gtk_window_resize(GTK_WINDOW(local->mainwin), width, height);
    break;
  case 'w':
    width = size;
    _getobj(obj, "height", inst, &height);
    gtk_window_resize(GTK_WINDOW(local->mainwin), width, height);
    break;
  }

  return 0;
}

static int
gtkwait_action(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;
  char *name;

  _getobj(obj, "_gtklocal", inst, &local);

  g_free(rval->str);
  rval->str = NULL;

  if (local->mainwin == NULL) {
    return 1;
  }

  if (local->blank_cursor == NULL) {
    local->blank_cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR);
    gdk_window_set_cursor(gtk_widget_get_window(local->mainwin), local->blank_cursor);
  }

  local->action.type = ACTION_TYPE_NONE;
  local->action.val = 0;
  while (local->action.type == ACTION_TYPE_NONE) {
    gtk_evloop(NULL, NULL, NULL, 0, NULL);
    msleep(100);
  }

  switch (local->action.type) {
  case ACTION_TYPE_NONE:
    break;
  case ACTION_TYPE_KEY:
    name = gdk_keyval_name(local->action.val);
    if (name) {
      rval->str = g_strdup(name);
    }
    break;
  case ACTION_TYPE_BUTTON:
    rval->str = g_strdup_printf("button%d", local->action.val);
    break;
  case ACTION_TYPE_SCROLL:
    switch (local->action.val) {
    case GDK_SCROLL_UP:
      rval->str = g_strdup("scroll_up");
      break;
    case GDK_SCROLL_DOWN:
      rval->str = g_strdup("scroll_down");
      break;
    case GDK_SCROLL_LEFT:
      rval->str = g_strdup("scroll_left");
      break;
    case GDK_SCROLL_RIGHT:
      rval->str = g_strdup("scroll_right");
      break;
    }
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
  {"wait_action",NSFUNC, NREAD | NEXEC, gtkwait_action, "", 0},
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
