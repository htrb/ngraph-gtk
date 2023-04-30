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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *window;
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
static int gtk_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
		      char **argv);
#endif
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

static void
cursor_moved(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;

  if (gtklocal->blank_cursor) {
    gtk_widget_set_cursor(gtklocal->mainwin, NULL);
    g_object_unref(gtklocal->blank_cursor);
    gtklocal->blank_cursor = NULL;
  }
}

static void
button_released(
#if GTK_CHECK_VERSION(4, 0, 0)
		GtkGestureClick *gesture,
#else
		GtkGestureMultiPress *gesture,
#endif
		gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  struct gtklocal *gtklocal;
  guint button;

  gtklocal = (struct gtklocal *) user_data;
  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

  gtklocal->action.type = ACTION_TYPE_BUTTON;
  gtklocal->action.val = button;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static gboolean
scrolled(GtkEventControllerScroll* self, gdouble dx, gdouble dy, gpointer user_data)
{
  struct gtklocal *gtklocal;
  gtklocal = (struct gtklocal *) user_data;
  gtklocal->action.type = ACTION_TYPE_SCROLL;
  gtklocal->action.val = dy;
  return FALSE;
}
#else
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
#endif

static void
add_event_button(GtkWidget *widget, struct gtklocal *gtklocal)
{
  GtkGesture *ev;
#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_gesture_click_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev));
#else
  ev = gtk_gesture_multi_press_new(widget);
#endif
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(ev), 0);
  g_signal_connect(ev, "released", G_CALLBACK(button_released), gtklocal);
}

static void
add_event_motion(GtkWidget *widget, struct gtklocal *gtklocal)
{
  GtkEventController *ev;

#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_event_controller_motion_new();
  gtk_widget_add_controller(widget, ev);
#else
  ev = gtk_event_controller_motion_new(widget);
#endif
  g_signal_connect(ev, "motion", G_CALLBACK(cursor_moved), gtklocal);
}

#if GTK_CHECK_VERSION(4, 0, 0)
void
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

static void
draw_function(GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height, gpointer user_data)
{
  gtkevpaint(GTK_WIDGET(drawing_area), cr, user_data);
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
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkEventController *ev;
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtklocal->mainwin = gtk_window_new();
#else
  gtklocal->mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif
  if (TopLevel) {
    gtk_window_set_modal(GTK_WINDOW(gtklocal->mainwin), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(gtklocal->mainwin), GTK_WINDOW(TopLevel));
#if OSX
    gtklocal->menulock = Menulock;
    Menulock = TRUE;
#endif
  }
  gtk_window_set_default_size(GTK_WINDOW(gtklocal->mainwin), width, height);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  g_signal_connect_swapped(gtklocal->mainwin, "close_request", G_CALLBACK(gtkclose), gtklocal->mainwin);
#else
  g_signal_connect_swapped(gtklocal->mainwin,
			   "delete-event",
			   G_CALLBACK(gtkclose), gtklocal->mainwin);
#endif

  add_event_key(gtklocal->mainwin, G_CALLBACK(ev_key_down), NULL,  gtklocal);

  gtk_window_set_title((GtkWindow *) gtklocal->mainwin, gtklocal->title);

#if GTK_CHECK_VERSION(4, 0, 0)
  scrolled_window = gtk_scrolled_window_new();
#else
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_child(GTK_WINDOW(gtklocal->mainwin), scrolled_window);
#else
  gtk_container_add(GTK_CONTAINER(gtklocal->mainwin), scrolled_window);
#endif

  gtklocal->View = gtk_drawing_area_new();
  gtk_widget_set_halign(gtklocal->View, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(gtklocal->View, GTK_ALIGN_CENTER);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(gtklocal->View), draw_function, gtklocal, NULL);
  gtk_widget_set_can_focus(gtklocal->View, TRUE);
  g_signal_connect(gtklocal->View, "resize", G_CALLBACK(resized), gtklocal);
/* must be implemented */
#else
  g_signal_connect(gtklocal->View, "draw",
		   G_CALLBACK(gtkevpaint), gtklocal);

  g_signal_connect(gtklocal->mainwin, "size-allocate",
		   G_CALLBACK(size_allocate), gtklocal);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), gtklocal->View);
#else
  gtk_container_add(GTK_CONTAINER(scrolled_window), gtklocal->View);
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show_all(gtklocal->mainwin);
#endif

  gtklocal->surface= NULL;
  gtklocal->redraw = TRUE;

#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_add_events(gtklocal->mainwin, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK);
#endif
  add_event_motion(gtklocal->mainwin, gtklocal);
  add_event_button(gtklocal->mainwin, gtklocal);
#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(gtklocal->mainwin, GTK_EVENT_CONTROLLER(ev));
  g_signal_connect(ev, "scroll", G_CALLBACK(scrolled), gtklocal);
#else
  g_signal_connect(gtklocal->mainwin, "scroll-event", G_CALLBACK(scrolled), gtklocal);

  if (chkobjfield(obj, "_evloop")) {
    goto errexit;
  }

  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn == -1) {
    goto errexit;
  }

  registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, gtklocal);
#endif

  gtkchangedpi(gtklocal);

#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_evloop(obj, inst, NULL, argc, argv);
#endif
  return 0;

errexit:
  if (gtklocal) {
    if (gtklocal->mainwin) {
#if GTK_CHECK_VERSION(4, 0, 0)
      gtk_window_destroy(GTK_WINDOW(gtklocal->mainwin));
#else
      gtk_widget_destroy(gtklocal->mainwin);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
    GMainContext *context;
    gtk_window_destroy(GTK_WINDOW(gtklocal->mainwin));
    gtklocal->mainwin = NULL;
    context = g_main_context_default();
    while (g_main_context_pending(context)) {
      g_main_context_iteration(context, TRUE);
    }
#else
    gtk_widget_destroy(gtklocal->mainwin);
    gtklocal->mainwin = NULL;
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
#endif
  }

  if (gtklocal->surface) {
    cairo_surface_destroy(gtklocal->surface);
  }

#if ! GTK_CHECK_VERSION(4, 0, 0)
  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn != -1)
    unregisterevloop(robj, idn, inst);
#endif

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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif

  if (local->View == NULL) {
    return;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_queue_draw(local->View);
#else
  win = gtk_widget_get_window(local->View);
  if (win == NULL) {
    return;
  }

  gdk_window_invalidate_rect(win, NULL, TRUE);
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(local->mainwin);
#else
  reset_event();
  msleep(100);
#endif
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

#if ! GTK_CHECK_VERSION(4, 0, 0)
static int
gtk_evloop(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
  return 0;
}
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_window_set_default_size(GTK_WINDOW(local->mainwin), width, height);
#else
    gtk_window_resize(GTK_WINDOW(local->mainwin), width, height);
#endif
    break;
  case 'w':
    width = size;
    _getobj(obj, "height", inst, &height);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_window_set_default_size(GTK_WINDOW(local->mainwin), width, height);
#else
    gtk_window_resize(GTK_WINDOW(local->mainwin), width, height);
#endif
    break;
  }

  return 0;
}

static int
gtkwait_action(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gtklocal *local;
  const char *name;

  _getobj(obj, "_gtklocal", inst, &local);

  g_free(rval->str);
  rval->str = NULL;

  if (local->mainwin == NULL) {
    return 1;
  }

  if (local->blank_cursor == NULL) {
#if GTK_CHECK_VERSION(4, 0, 0)
    local->blank_cursor = gdk_cursor_new_from_name("none", NULL);
    gtk_widget_set_cursor(local->mainwin, local->blank_cursor);
#else
    local->blank_cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR);
    gdk_window_set_cursor(gtk_widget_get_window(local->mainwin), local->blank_cursor);
#endif
  }

  local->action.type = ACTION_TYPE_NONE;
  local->action.val = 0;
  while (local->action.type == ACTION_TYPE_NONE) {
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_evloop(NULL, NULL, NULL, 0, NULL);
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  {"_evloop", NVFUNC, 0, gtk_evloop, NULL, 0},
#endif
};

#define TBLNUM (sizeof(gra2gtk) / sizeof(*gra2gtk))

void *
addgra2gtk(void)
/* addgra2gtk() returns NULL on error */
{
  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gra2gtk,
		   ERRNUM, gtkerrorlist, NULL, NULL);
}
