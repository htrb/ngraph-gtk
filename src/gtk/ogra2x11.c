/* 
 * $Id: ogra2x11.c,v 1.4 2008/06/23 02:51:51 hito Exp $
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

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

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
#include "jstring.h"
#include "mathfn.h"
#include "nconfig.h"

#include "main.h"
#include "x11gui.h"
#include "ogra2x11.h"

#define NAME "gra2gtk"
#define PARENT "gra2"
#define NVERSION  "1.00.00"
#define GRA2GTKCONF "[gra2gtk]"
#define LINETOLIMIT 500

extern int OpenApplication();
extern int OpenApplication();

#define ERRDISPLAY 100
#define ERRCMAP 101

#define WINWIDTH 578
#define WINHEIGHT 819

static char *gtkerrorlist[] = {
  "cannot open display.",
  "not enough color cell.",
};

#define ERRNUM (sizeof(gtkerrorlist)/sizeof(*gtkerrorlist))


#define GTKFONTCASH 64		/* must be greater than 1 */
#define GTKCOLORDEPTH 2

enum
  { NORMAL = 0, BOLD, ITALIC, BOLDITALIC, OBLIQUE, BOLDOBLIQUE };

struct fontmap;

struct fontmap
{
  char *fontalias;
  char *fontname;
  int type;
  int twobyte, symbol;
  struct fontmap *next;
};

struct fontlocal
{
  char *fontalias;
  PangoFontDescription *font;
  int fontsize, fonttype, fontdir, symbol;
  //  int iso8859;
};

struct gtklocal
{
  struct objlist *obj;
  char *inst;
  GtkWidget *mainwin, *View;
  GdkPixmap *win;
  GdkWindow *window;
  char *title;
  int autoredraw, redraw;
  GdkGC *gc;
  unsigned int winwidth, winheight, windpi;
  GdkColormap *cmap;
  int cdepth;
  double pixel_dot;
  int offsetx, offsety;
  int cpx, cpy;
  struct fontmap *fontmaproot;
  int loadfont;
  int loadfontf;
  char *fontalias;
  double fontsize, fontspace, fontcos, fontsin, fontdir;
  struct fontlocal font[GTKFONTCASH];
  GdkPoint points[LINETOLIMIT];
  int linetonum;
  int PaperWidth, PaperHeight;
  int backingstore;
  int minus_hyphen;
  int bg_r, bg_g, bg_b;
};

static void gtkMakeRuler(struct gtklocal *gtklocal);
static int gtk_evloop(struct objlist *obj, char *inst, char *rval, int argc,
		      char **argv);
static int gtkflush(struct objlist *obj, char *inst, char *rval, int argc,
		    char **argv);
static int gtkloadconfig(struct objlist *obj, struct gtklocal *gtklocal);
static void gtk_redraw(struct objlist *obj, void *inst,
		       struct gtklocal *gtklocal);
static void gtkclose(GtkWidget * widget, gpointer user_data);
static void gtkchangedpi(struct gtklocal *gtklocal);;
static gboolean gtkevpaint(GtkWidget * w, GdkEventExpose * e,
			   gpointer user_data);
static void gtkevsize(GtkWidget * w, GtkRequisition * reqest,
		      gpointer call_data);
static int gtkinit(struct objlist *obj, char *inst, char *rval, int argc,
		   char **argv);
static int gtkdone(struct objlist *obj, char *inst, char *rval, int argc,
		   char **argv);
static int gtkclear(struct objlist *obj, char *inst, char *rval, int argc,
		    char **argv);
static int gtkdpi(struct objlist *obj, char *inst, char *rval, int argc,
		  char **argv);
static int gtkautoredraw(struct objlist *obj, char *inst, char *rval,
			 int argc, char **argv);
static int gtkstorememory(struct objlist *obj, char *inst, char *rval,
			  int argc, char **argv);
static int gtkredraw(struct objlist *obj, char *inst, char *rval, int argc,
		     char **argv);
static int dot2pixel(struct gtklocal *gtklocal, int r);
static int dot2pixelx(struct gtklocal *gtklocal, int x);
static int dot2pixely(struct gtklocal *gtklocal, int y);
static int pixel2dot(struct gtklocal *gtklocal, int r);
static int pixel2dotx(struct gtklocal *gtklocal, int x);
static int pixel2doty(struct gtklocal *gtklocal, int y);
static int gtk_output(struct objlist *obj, char *inst, char *rval, int argc,
		      char **argv);
static int gtk_charwidth(struct objlist *obj, char *inst, char *rval,
			 int argc, char **argv);
static int gtk_charheight(struct objlist *obj, char *inst, char *rval,
			  int argc, char **argv);

static GdkColor *gtkRGB(struct gtklocal *gtklocal, int R, int G, int B);

static int
gtkloadconfig(struct objlist *obj, struct gtklocal *gtklocal)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3, *f4, symbol[] = "Sym";
  struct fontmap *fcur, *fnew;
  int val;
  char *endptr;
  int len;

  fp = openconfig(GRA2GTKCONF);
  if (fp == NULL)
    return 0;

  fcur = NULL;
  while ((tok = getconfig(fp, &str)) != NULL) {
    s2 = str;
    if (strcmp(tok, "font_map") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0]) != NULL); s2++);
      f4 = getitok2(&s2, &len, "");
      if ((f1 != NULL) && (f2 != NULL) && (f3 != NULL) && (f4 != NULL)) {
	if ((fnew = memalloc(sizeof(struct fontmap))) == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}
	if (fcur == NULL) {
	  gtklocal->fontmaproot = fnew;
	} else {
	  fcur->next = fnew;
	}
	fcur = fnew;
	fcur->next = NULL;
	fcur->fontalias = f1;
	fcur->symbol = ! strncmp(f1, symbol, sizeof(symbol) - 1);
	if (strcmp(f2, "bold") == 0) {
	  fcur->type = BOLD;
	} else if (strcmp(f2, "italic") == 0) {
	  fcur->type = ITALIC;
	} else if (strcmp(f2, "bold_italic") == 0) {
	  fcur->type = BOLDITALIC;
	} else {
	  fcur->type = NORMAL;
	}
	memfree(f2);
	val = strtol(f3, &endptr, 10);
	fcur->twobyte = val;
	fcur->fontname = f4;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
	memfree(f4);
      }
    } else if (strcmp(tok, "backing_store") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->backingstore = val;
      memfree(f1);
    } else if (strcmp(tok, "win_dpi") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->windpi = val;
      memfree(f1);
    } else if (strcmp(tok, "win_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->winwidth = val;
      memfree(f1);
    } else if (strcmp(tok, "win_height") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->winheight = val;
      memfree(f1);
    } else if (strcmp(tok, "color_depth") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->cdepth = val;
      memfree(f1);
    } else if (strcmp(tok, "auto_redraw") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	if (val == 0)
	  gtklocal->autoredraw = FALSE;
	else
	  gtklocal->autoredraw = TRUE;
      }
      memfree(f1);
    } else if (strcmp(tok, "minus_hyphen") == 0) {
      f1 = getitok2(&s2, &len, " \x09,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	gtklocal->minus_hyphen = val;
      memfree(f1);
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

static void
gtk_redraw(struct objlist *obj, void *inst, struct gtklocal *gtklocal)
{
  GRAredraw(obj, inst, FALSE, FALSE);
  gdk_flush();
}

static gboolean
gtkevpaint(GtkWidget * w, GdkEventExpose * e, gpointer user_data)
{
  struct gtklocal *gtklocal;
  GdkRectangle rect;

  gtklocal = (struct gtklocal *) user_data;

  if (e->count != 0)
    return TRUE;

  if (gtklocal->win == NULL)
    return FALSE;

  if (gtklocal->redraw) {
    gtk_redraw(gtklocal->obj, gtklocal->inst, gtklocal);
    gtklocal->redraw = FALSE;
    gtkMakeRuler(gtklocal);
  }

  rect.x = 0;
  rect.y = 0;
  rect.width = SHRT_MAX;
  rect.height = SHRT_MAX;
  gdk_gc_set_clip_rectangle(gtklocal->gc, &rect);
  gdk_draw_drawable(gtklocal->window, gtklocal->gc, gtklocal->win, 0, 0, 0, 0, -1, -1);

  return FALSE;
}

static void
gtkclose(GtkWidget * widget, gpointer user_data)
{
  char *inst;
  struct gtklocal *local;
  int i, id;
  struct objlist *obj;

  obj = chkobject("gra2gtk");
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    inst = chkobjinst(obj, i);
    _getobj(obj, "_local", inst, &local);
    if (local->mainwin == widget) {
      _getobj(obj, "id", inst, &id);
      delobj(obj, id);
      break;
    }
  }
  return;
}

static void
gtkevsize(GtkWidget * w, GtkRequisition * reqest, gpointer user_data)
{
  struct gtklocal *gtklocal;

  gtklocal = (struct gtklocal *) user_data;
  gtkchangedpi(gtklocal);
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_w:
    if (e->state & GDK_CONTROL_MASK) 
      gtk_widget_destroy(GTK_WIDGET(user_data));
    return TRUE;
  }
  return FALSE;
}

static int
gtkinit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  struct fontmap *fcur;
  GdkWindow *win;
  GdkGC *gc = NULL;
  int oid;
  struct objlist *robj;
  int idn;
  GtkWidget *scrolled_window = NULL, *vbox = NULL;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  gtklocal = memalloc(sizeof(struct gtklocal));
  if (gtklocal == NULL)
    return 1;

  gtklocal->obj = obj;
  gtklocal->inst = inst;
  gtklocal->fontmaproot = NULL;
  gtklocal->title = NULL;

  if (_putobj(obj, "_local", inst, gtklocal))
    goto errexit;

  if (_getobj(obj, "oid", inst, &oid))
    goto errexit;

  gtklocal->PaperWidth = 0;
  gtklocal->PaperHeight = 0;
  gtklocal->winwidth = WINWIDTH;
  gtklocal->winheight = WINHEIGHT;
  gtklocal->windpi = DEFAULT_DPI;
  gtklocal->autoredraw = TRUE;
  gtklocal->cdepth = GTKCOLORDEPTH;
  gtklocal->backingstore = FALSE;
  gtklocal->minus_hyphen = TRUE;
  gtklocal->bg_r = 0xff;
  gtklocal->bg_g = 0xff;
  gtklocal->bg_b = 0xff;

  if (gtkloadconfig(obj, gtklocal))
    goto errexit;

  if (gtklocal->windpi < 1)
    gtklocal->windpi = 1;

  if (gtklocal->windpi > DPI_MAX)
    gtklocal->windpi = DPI_MAX;

  if (gtklocal->winwidth < 1)
    gtklocal->winwidth = 1;

  if (gtklocal->winwidth > 10000)
    gtklocal->winwidth = 10000;

  if (gtklocal->winheight < 1)
    gtklocal->winheight = 1;

  if (gtklocal->winheight > 10000)
    gtklocal->winheight = 10000;

  if (gtklocal->cdepth < 2)
    gtklocal->cdepth = 2;

  if (gtklocal->cdepth > 8)
    gtklocal->cdepth = 8;

  if (_putobj(obj, "dpi", inst, &(gtklocal->windpi)))
    goto errexit;

  if (_putobj(obj, "auto_redraw", inst, &(gtklocal->autoredraw)))
    goto errexit;

  if (!OpenApplication())
    goto errexit;

  gtklocal->title = mkobjlist(obj, NULL, oid, NULL, TRUE);

  gtklocal->mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  if (gtklocal->mainwin == NULL)
    goto errexit;

  g_signal_connect_swapped(gtklocal->mainwin,
			   "destroy",
			   G_CALLBACK(gtkclose), gtklocal->mainwin);

  g_signal_connect(gtklocal->mainwin, "key-press-event", G_CALLBACK(ev_key_down), gtklocal->mainwin);

  g_signal_connect(gtklocal->mainwin,
		   "expose-event", G_CALLBACK(gtkevpaint), gtklocal);

  //  g_signal_connect(gtklocal->mainwin, "size-request", G_CALLBACK(gtkevsize), gtklocal);

  gtk_window_set_title((GtkWindow *) gtklocal->mainwin, gtklocal->title);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  if (scrolled_window == NULL)
    goto errexit;
  gtk_widget_set_size_request((GtkWidget *) scrolled_window,
			      gtklocal->winwidth, gtklocal->winheight);

  vbox = gtk_vbox_new(FALSE, 0);
  if (vbox == NULL)
    goto errexit;

  gtk_container_add(GTK_CONTAINER(gtklocal->mainwin), vbox);

  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  gtklocal->View = gtk_drawing_area_new();
  if (gtklocal->View == NULL)
    goto errexit;

  gtk_widget_set_size_request(gtklocal->View,
			      gtklocal->winwidth, gtklocal->winheight);

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
					(scrolled_window), gtklocal->View);

  gtk_widget_show_all(gtklocal->mainwin);
  gtklocal->cmap = gdk_colormap_get_system();

  win = gtklocal->View->window;
  gc = gdk_gc_new(win);

  gtklocal->win = NULL;
  gtklocal->window = win;
  gtklocal->gc = gc;
  gtklocal->offsetx = 0;
  gtklocal->offsety = 0;
  gtklocal->cpx = 0;
  gtklocal->cpy = 0;
  gtklocal->pixel_dot = gtklocal->windpi / 25.4 / 100;
  gtklocal->fontalias = NULL;

  gtklocal->loadfont = 0;
  gtklocal->loadfontf = -1;
  gtklocal->linetonum = 0;

  gtklocal->redraw = TRUE;

  if (chkobjfield(obj, "_evloop")) {
    goto errexit;
  }

  if ((idn = getobjtblpos(obj, "_evloop", &robj)) == -1) {
    goto errexit;
  }

  registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, gtklocal);

  gtkchangedpi(gtklocal);

  gtk_evloop(obj, inst, NULL, argc, argv);
  return 0;

errexit:
  if (gtklocal->mainwin) {
    g_object_unref(gtklocal->gc);
    g_object_unref(gtklocal->win);
    gtk_widget_destroy(gtklocal->mainwin);
  }

  fcur = gtklocal->fontmaproot;
  memfree(gtklocal->title);
  memfree(gtklocal);
  return 1;
}

static int
gtkdone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  struct fontmap *fcur, *fdel;
  int idn, i;
  struct objlist *robj;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  if (gtklocal->mainwin != NULL) {
    gtk_widget_destroy(gtklocal->mainwin);
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }

  idn = getobjtblpos(obj, "_evloop", &robj);
  if (idn != -1)
    unregisterevloop(robj, idn, inst);

  fcur = gtklocal->fontmaproot;
  while (fcur != NULL) {
    fdel = fcur;
    fcur = fcur->next;
    memfree(fdel->fontalias);
    memfree(fdel->fontname);
    memfree(fdel);
  }

  for (i = 0; i < gtklocal->loadfont; i++) {
    memfree(gtklocal->font[i].fontalias);
    pango_font_description_free(gtklocal->font[i].font);
    gtklocal->font[i].fontalias = NULL;
    gtklocal->font[i].font = NULL;
  }
  gtklocal->loadfont = 0;

  memfree(gtklocal->title);
  memfree(gtklocal->fontalias);

  return 0;
}

static int
gtkflush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  if (gtklocal->linetonum != 0) {
    gdk_draw_lines(gtklocal->win, gtklocal->gc,
		   gtklocal->points, gtklocal->linetonum);
    gtklocal->linetonum = 0;
  }
  gdk_flush();
  return 0;
}

static int
gtkclear(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv))
    return 1;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  if (gtklocal->linetonum != 0) {
    gdk_draw_lines(gtklocal->win, gtklocal->gc,
		   gtklocal->points, gtklocal->linetonum);
    gtklocal->linetonum = 0;
  }
  gdk_flush();
  gtklocal->PaperWidth = 0;
  gtklocal->PaperHeight = 0;
  gtkchangedpi(gtklocal);
  return 0;
}

static int
gtkdpi(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  int dpi;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  dpi = abs(*(int *) argv[2]);

  if (dpi < 1)
    dpi = 1;

  if (dpi > DPI_MAX)
    dpi = DPI_MAX;

  gtklocal->windpi = dpi;
  gtklocal->pixel_dot = gtklocal->windpi / 25.4 / 100;
  *(int *) argv[2] = dpi;
  gtkchangedpi(gtklocal);
  return 0;
}

static int
set_color(struct gtklocal *gtklocal, int argc, char **argv)
{
  int c;

  c = abs(*(int *) argv[2]);

  if (c < 0)
    c = 1;

  if (c > 0xff)
    c = 0xff;

  return c;
}


static int
gtkbr(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  gtklocal->bg_r = set_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkbb(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  gtklocal->bg_b = set_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkbg(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  gtklocal->bg_g = set_color(gtklocal, argc, argv);
  return 0;
}

static int
gtkautoredraw(struct objlist *obj, char *inst, char *rval,
	      int argc, char **argv)
{
  struct gtklocal *gtklocal;
  char *arg;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  arg = argv[1];
  gtklocal->autoredraw = abs(*(int *) argv[2]);

  return 0;
}

static int
gtkstorememory(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  return 0;
}

static int
gtkredraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  gtk_redraw(obj, inst, gtklocal);
  return 0;
}

static int
gtk_evloop(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
  return 0;
}

static int
dot2pixel(struct gtklocal *gtklocal, int r)
{
  return nround(r * gtklocal->pixel_dot);
}

static int
dot2pixelx(struct gtklocal *gtklocal, int x)
{
  return nround(x * gtklocal->pixel_dot + gtklocal->offsetx);
}

static int
dot2pixely(struct gtklocal *gtklocal, int y)
{
  return nround(y * gtklocal->pixel_dot + gtklocal->offsety);
}

static int
pixel2dot(struct gtklocal *gtklocal, int r)
{
  return nround(r / gtklocal->pixel_dot);
}

static int
pixel2dotx(struct gtklocal *gtklocal, int x)
{
  return nround((x - gtklocal->offsetx) / gtklocal->pixel_dot);
}

static int
pixel2doty(struct gtklocal *gtklocal, int y)
{
  return nround((y - gtklocal->offsety) / gtklocal->pixel_dot);
}

static void
gtkchangedpi(struct gtklocal *gtklocal)
{
  int width, height, pw, ph;
  GdkPixmap *pixmap;


  if ((gtklocal->PaperWidth == 0) || (gtklocal->PaperHeight == 0)) {
    return;
  }

  width = dot2pixel(gtklocal, gtklocal->PaperWidth);
  height = dot2pixel(gtklocal, gtklocal->PaperHeight);

  pixmap = gtklocal->win;
  if (pixmap) {
    gdk_drawable_get_size(GDK_DRAWABLE(pixmap), &pw, &ph);
    if (pw != width || ph != height) {
      g_object_unref(G_OBJECT(pixmap));
      pixmap = NULL;
    }
  }

  if (pixmap == NULL) {
    GdkColor color;

    pixmap = gdk_pixmap_new(gtklocal->window, width, height, -1);
    gtklocal->win = pixmap;

    color.red = gtklocal->bg_r * 0xff;
    color.green = gtklocal->bg_g * 0xff;
    color.blue = gtklocal->bg_b * 0xff;

    gdk_gc_set_rgb_fg_color(gtklocal->gc, &color);
    gdk_draw_rectangle(pixmap, gtklocal->gc, TRUE, 0, 0, width, height);

    gtklocal->redraw = TRUE;
  }


  gtk_widget_set_size_request(gtklocal->View, width, height);
}

static GdkColor *
gtkRGB(struct gtklocal *gtklocal, int R, int G, int B)
{
  static GdkColor col;
  double r, g, b;

  r = R * gtklocal->cdepth / 256;
  if (r >= gtklocal->cdepth) {
    r = gtklocal->cdepth - 1;
  } else if (r < 0) {
    r = 0;
  }

  g = G * gtklocal->cdepth / 256;
  if (g >= gtklocal->cdepth) {
    g = gtklocal->cdepth - 1;
  } else if (g < 0) {
    g = 0;
  }

  b = B * gtklocal->cdepth / 256;
  if (b >= gtklocal->cdepth) {
    b = gtklocal->cdepth - 1;
  } else if (b < 0) {
    b = 0;
  }

  col.red = r * 65535 / (gtklocal->cdepth - 1);
  col.green = g * 65535 / (gtklocal->cdepth - 1);
  col.blue = b * 65535 / (gtklocal->cdepth - 1);

  return &col;
}

static void
gtkMakeRuler(struct gtklocal *gtklocal)
{
  int width, height;
  GdkColor *col1;
  GdkWindow *win;
  GdkGC *gc;

  width = gtklocal->PaperWidth;
  height = gtklocal->PaperHeight;
  win = gtklocal->win;

  if (width == 0 || height == 0 || win == NULL)
    return;

  gc = gdk_gc_new(gtklocal->window);
  gdk_gc_set_function(gc, GDK_XOR);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT,
			     GDK_JOIN_MITER);
  col1 = gtkRGB(gtklocal, 127, 127, 127);
  gdk_gc_set_rgb_fg_color(gc, col1);
  gdk_draw_rectangle(win, gc, FALSE,
		     0, 0,
		     dot2pixel(gtklocal, width) - 1, dot2pixel(gtklocal, height) - 1);
  gdk_gc_set_function(gc, GDK_COPY);
  g_object_unref(gc);
}

int
gdkgc_set_arc_mode(GdkGC * gc, int arc_mode)
{
  return XSetArcMode(gdk_x11_gc_get_xdisplay(gc), gdk_x11_gc_get_xgc(gc),
		     arc_mode);
}

int
gdkgc_set_fill_rule(GdkGC * gc, int fill_rule)
{
  return XSetFillRule(gdk_x11_gc_get_xdisplay(gc), gdk_x11_gc_get_xgc(gc),
		      fill_rule);
}

static int
mxloadfont(struct gtklocal *gtklocal, int top)
{
  struct fontlocal font;
  struct fontmap *fcur;
  char *fontname;
  int twobyte = FALSE, type = NORMAL, fontcashfind, i, store, symbol = FALSE;
  PangoFontDescription *pfont;
  PangoStyle style;
  PangoWeight weight;
  static PangoLanguage *lang_ja = NULL, *lang = NULL;

  fontcashfind = -1;

  if (lang == NULL) {
    lang = pango_language_from_string("en-US");
    lang_ja = pango_language_from_string("ja-JP");
  }

  for (i = 0; i < gtklocal->loadfont; i++) {
    if (strcmp((gtklocal->font[i]).fontalias, gtklocal->fontalias) == 0) {
      fontcashfind=i;
      break;
    }
  }

  if (fontcashfind != -1) {
    if (top) {
      font = gtklocal->font[fontcashfind];
      for (i = fontcashfind - 1; i >= 0; i--) {
	gtklocal->font[i + 1] = gtklocal->font[i];
      }
      gtklocal->font[0] = font;
      return 0;
    } else {
      return fontcashfind;
    }
  }

  fontname = NULL;

  for (fcur = gtklocal->fontmaproot; fcur; fcur=fcur->next) {
    if (strcmp(gtklocal->fontalias, fcur->fontalias) == 0) {
      fontname = fcur->fontname;
      type = fcur->type;
      twobyte = fcur->twobyte;
      symbol = fcur->symbol;
      break;
    }
  }

  pfont = pango_font_description_new();
  pango_font_description_set_family(pfont, fontname);

  switch (type) {
  case ITALIC:
  case BOLDITALIC:
    style = PANGO_STYLE_ITALIC;
    break;
  case OBLIQUE:
  case BOLDOBLIQUE:
    style = PANGO_STYLE_OBLIQUE;
    break;
  default:
    style = PANGO_STYLE_NORMAL;
    break;
  }
  pango_font_description_set_style(pfont, style);

  switch (type) {
  case BOLD:
  case BOLDITALIC:
  case BOLDOBLIQUE:
    weight = PANGO_WEIGHT_BOLD;
    break;
  default:
    weight = PANGO_WEIGHT_NORMAL;
    break;
  }
  pango_font_description_set_weight(pfont, weight);

  if (gtklocal->loadfont == GTKFONTCASH) {
    i = GTKFONTCASH - 1;

    memfree((gtklocal->font[i]).fontalias);
    gtklocal->font[i].fontalias = NULL;

    pango_font_description_free((gtklocal->font[i]).font);
    gtklocal->font[i].font = NULL;

    gtklocal->loadfont--;
  }

  if (top) {
    for (i = gtklocal->loadfont - 1; i >= 0; i--) {
      gtklocal->font[i + 1] = gtklocal->font[i];
    }
    store=0;
  } else {
    store = gtklocal->loadfont;
  }

  gtklocal->font[store].fontalias = nstrdup(gtklocal->fontalias);
  if ((gtklocal->font[store]).fontalias == NULL) {
    pango_font_description_free(pfont);
    gtklocal->font[store].font = NULL;
    return -1;
  }

  gtklocal->font[store].font = pfont;
  gtklocal->font[store].fonttype = type;
  gtklocal->font[store].symbol = symbol;

  gtklocal->loadfont++;

  return store;
}

static void
draw_str(struct gtklocal *gtklocal, GdkDrawable *pix, char *str, int font, int size, int space, int *fw, int *ah, int *dh)
{
  PangoLayout *layout;
  PangoAttribute *attr;
  PangoAttrList *alist;
  PangoContext *context;
  PangoMatrix matrix = PANGO_MATRIX_INIT;
  PangoLayoutIter *piter;
  int x, y, w, h, width, height, baseline;

  layout = gtk_widget_create_pango_layout(gtklocal->mainwin, "");
  context = pango_layout_get_context(layout);
  pango_matrix_rotate(&matrix, gtklocal->fontdir);
  pango_context_set_matrix(context, &matrix);

  alist = pango_attr_list_new();

  attr = pango_attr_size_new_absolute(dot2pixel(gtklocal, size) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  attr = pango_attr_letter_spacing_new(dot2pixel(gtklocal, space) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  pango_layout_set_font_description(layout, gtklocal->font[font].font);
  pango_layout_set_attributes(layout, alist);

  pango_layout_set_text(layout, str, -1);

  pango_layout_get_pixel_size(layout, &w, &h);
  piter = pango_layout_get_iter(layout);
  baseline = pango_layout_iter_get_baseline(piter) / PANGO_SCALE;

  x = dot2pixelx(gtklocal, gtklocal->cpx);
  y = dot2pixely(gtklocal, gtklocal->cpy);

#if 0
  if (pix) {
    PangoLayoutLine *pline;
    PangoRectangle prect;
    int ascent, descent, s = dot2pixel(gtklocal, size);

    pline = pango_layout_get_line_readonly(layout, 0);
    pango_layout_line_get_pixel_extents(pline, &prect, NULL);
    ascent = PANGO_ASCENT(prect);
    descent = PANGO_DESCENT(prect);

    gdk_draw_rectangle(gtklocal->pix, gtklocal->gc, FALSE, x, y  - baseline, s, s);

    gdk_gc_set_rgb_fg_color(gtklocal->gc, &red);
    gdk_draw_rectangle(gtklocal->pix, gtklocal->gc, FALSE, x, y - baseline, w, h);
    gdk_draw_line(gtklocal->pix, gtklocal->gc, x, y - baseline, x + w, y - baseline);

    gdk_gc_set_rgb_fg_color(gtklocal->gc, &blue);

    gdk_draw_rectangle(gtklocal->pix, gtklocal->gc, FALSE, x + prect.x, y - ascent, prect.width, prect.height);
    gdk_draw_line(gtklocal->pix, gtklocal->gc, x + prect.x, y, x + prect.x + prect.width, y);

    gdk_gc_set_rgb_fg_color(gtklocal->gc, &black);
  }
#endif

  if (gtklocal->fontdir <= 90) {
    width = gtklocal->fontcos * w + gtklocal->fontsin * baseline;
    height = gtklocal->fontcos * baseline + gtklocal->fontsin * w;
    x -= gtklocal->fontsin * baseline;
    y -= height;
  } else if (gtklocal->fontdir < 180) {
    width = - gtklocal->fontcos * w + gtklocal->fontsin * baseline;
    height = - gtklocal->fontcos * baseline + gtklocal->fontsin * w;
    x -= width;
    y -= height - gtklocal->fontsin * baseline;
  } else if (gtklocal->fontdir <= 270) {
    width = - gtklocal->fontcos * w - gtklocal->fontsin * baseline;
    height = - gtklocal->fontcos * baseline - gtklocal->fontsin * w;
    x -= width + gtklocal->fontsin * baseline;
  } else if (gtklocal->fontdir < 360) {
    width = gtklocal->fontcos * w - gtklocal->fontsin * baseline;
    height = gtklocal->fontcos * baseline - gtklocal->fontsin * w;
    y += gtklocal->fontsin * baseline;
  }

  if (fw)
    *fw = w;

  if (ah)
    *ah = baseline;

  if (dh)
    *dh = h - baseline;

  if (pix && str) {
    gdk_draw_layout(pix, gtklocal->gc, x, y, layout);
    gtklocal->cpx += pixel2dot(gtklocal, w * gtklocal->fontcos);
    gtklocal->cpy -= pixel2dot(gtklocal, w * gtklocal->fontsin);
  }

  pango_layout_iter_free(piter);
  pango_attr_list_unref(alist);
  g_object_unref(layout);
}

static int
gtk_output(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gtklocal *gtklocal;
  char code;
  int *cpar;
  char *cstr, *tmp, *tmp2;
  int i, j, l, x, y, x1, y1, x2, y2;
  GdkRectangle rect;
  int width, dashn = 0;
  GdkJoinStyle join;
  GdkCapStyle cap;
  GdkLineStyle style;
  gint8 *dashlist = NULL;
  int arcmode;
  GdkPoint *xpoint;
  GdkGC *gc;
  double fontsize,fontspace,fontdir,fontsin,fontcos;
  int fontcashsize,fontcashdir;

  gtklocal = (struct gtklocal *) argv[2];

  gc = gtklocal->gc;

  code = *(char *) (argv[3]);
  cpar = (int *) argv[4];
  cstr = argv[5];

  if (gtklocal->linetonum != 0) {
    if ((code != 'T') || (gtklocal->linetonum >= LINETOLIMIT)) {
      gdk_draw_lines(gtklocal->win, gc,
		     gtklocal->points, gtklocal->linetonum);
      gtklocal->linetonum = 0;
    }
  }
  switch (code) {
  case 'I':
    gtklocal->PaperWidth = cpar[3];
    gtklocal->PaperHeight = cpar[4];
    gtkchangedpi(gtklocal);
    break;
  case '%':
  case 'X':
    break;
  case 'E':
    gdk_flush();
    break;
  case 'V':
    gtklocal->offsetx = dot2pixel(gtklocal, cpar[1]);
    gtklocal->offsety = dot2pixel(gtklocal, cpar[2]);
    gtklocal->cpx = 0;
    gtklocal->cpy = 0;
    if (cpar[5]) {
      rect.x = dot2pixel(gtklocal, cpar[1]);
      rect.y = dot2pixel(gtklocal, cpar[2]);
      rect.width = dot2pixel(gtklocal, cpar[3]) - rect.x;
      rect.height = dot2pixel(gtklocal, cpar[4]) - rect.y;
    } else {
      rect.x = 0;
      rect.y = 0;
      rect.width = SHRT_MAX;
      rect.height = SHRT_MAX;
    }
    gdk_gc_set_clip_rectangle(gc, &rect);
    break;
  case 'A':
    if (cpar[1] == 0) {
      style = GDK_LINE_SOLID;
    } else {
      style = GDK_LINE_ON_OFF_DASH;
      if ((dashlist = memalloc(sizeof(char) * cpar[1])) == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = dot2pixel(gtklocal, cpar[6 + i]);
	if (dashlist[i] <= 0) {
	  dashlist[i] = 1;
	}
	dashn = cpar[1];
      }
    }

    width = dot2pixel(gtklocal, cpar[2]);

    if (cpar[3] == 2) {
      cap = GDK_CAP_PROJECTING;
    } else if (cpar[3] == 1) {
      cap = GDK_CAP_ROUND;
    } else {
      cap = GDK_CAP_BUTT;
    }

    if (cpar[4] == 2) {
      join = GDK_JOIN_BEVEL;
    } else if (cpar[4] == 1) {
      join = GDK_JOIN_ROUND;
    } else {
      join = GDK_JOIN_MITER;
    }
    gdk_gc_set_line_attributes(gc, width, style, cap, join);

    if (style != GDK_LINE_SOLID)
      gdk_gc_set_dashes(gc, 0, dashlist, dashn);

    if (cpar[1] != 0)
      memfree(dashlist);

    break;
  case 'G':
    gdk_gc_set_rgb_fg_color(gc, gtkRGB(gtklocal, cpar[1], cpar[2], cpar[3]));
    break;
  case 'M':
    gtklocal->cpx = cpar[1];
    gtklocal->cpy = cpar[2];
    break;
  case 'N':
    gtklocal->cpx += cpar[1];
    gtklocal->cpy += cpar[2];
    break;
  case 'L':
    gdk_draw_line(gtklocal->win, gc,
		  dot2pixelx(gtklocal, cpar[1]), dot2pixely(gtklocal,
							    cpar[2]),
		  dot2pixelx(gtklocal, cpar[3]), dot2pixely(gtklocal,
							    cpar[4]));
    break;
  case 'T':
    x = dot2pixelx(gtklocal, cpar[1]);
    y = dot2pixely(gtklocal, cpar[2]);
    if (gtklocal->linetonum == 0) {
      gtklocal->points[0].x = dot2pixelx(gtklocal, gtklocal->cpx);
      gtklocal->points[0].y = dot2pixely(gtklocal, gtklocal->cpy);;
      gtklocal->linetonum++;
    }
    gtklocal->points[gtklocal->linetonum].x = x;
    gtklocal->points[gtklocal->linetonum].y = y;
    gtklocal->linetonum++;
    gtklocal->cpx = cpar[1];
    gtklocal->cpy = cpar[2];
    break;
  case 'C':
    if (cpar[7] == 0) {
      gdk_draw_arc(gtklocal->win, gc,
		   FALSE,
		   dot2pixelx(gtklocal, cpar[1] - cpar[3]),
		   dot2pixely(gtklocal, cpar[2] - cpar[4]),
		   dot2pixel(gtklocal, 2 * cpar[3]),
		   dot2pixel(gtklocal, 2 * cpar[4]),
		   (int) cpar[5] * 64 / 100, (int) cpar[6] * 64 / 100);
    } else {
      if ((dot2pixel(gtklocal, cpar[3]) < 2)
	  && (dot2pixel(gtklocal, cpar[4]) < 2)) {
	gdk_draw_point(gtklocal->win, gc,
		       dot2pixelx(gtklocal, cpar[1]),
		       dot2pixely(gtklocal, cpar[2]));
      } else {
	if (cpar[7] == 1) {
	  arcmode = ArcPieSlice;
	} else {
	  arcmode = ArcChord;
	}
	gdkgc_set_arc_mode(gc, arcmode);
	gdk_draw_arc(gtklocal->win, gc,
		     TRUE,
		     dot2pixelx(gtklocal, cpar[1] - cpar[3]),
		     dot2pixely(gtklocal, cpar[2] - cpar[4]),
		     dot2pixel(gtklocal, 2 * cpar[3]),
		     dot2pixel(gtklocal, 2 * cpar[4]),
		     (int) cpar[5] * 64 / 100, (int) cpar[6] * 64 / 100);
      }
    }
    break;
  case 'B':
    if (cpar[1] <= cpar[3]) {
      x1 = dot2pixelx(gtklocal, cpar[1]);
      x2 = dot2pixel(gtklocal, cpar[3] - cpar[1]);
    } else {
      x1 = dot2pixelx(gtklocal, cpar[3]);
      x2 = dot2pixel(gtklocal, cpar[1] - cpar[3]);
    }
    if (cpar[2] <= cpar[4]) {
      y1 = dot2pixely(gtklocal, cpar[2]);
      y2 = dot2pixel(gtklocal, cpar[4] - cpar[2]);
    } else {
      y1 = dot2pixely(gtklocal, cpar[4]);
      y2 = dot2pixel(gtklocal, cpar[2] - cpar[4]);
    }
    if (cpar[5] == 0) {
      gdk_draw_rectangle(gtklocal->win, gc, FALSE, x1, y1, x2 + 1, y2 + 1);
    } else {
      gdk_draw_rectangle(gtklocal->win, gc, TRUE, x1, y1, x2 + 1, y2 + 1);
    }
    break;
  case 'P':
    gdk_draw_point(gtklocal->win, gc,
		   dot2pixelx(gtklocal, cpar[1]), dot2pixely(gtklocal,
							     cpar[2]));
    break;
  case 'R':
    if (cpar[1] == 0)
      break;

    xpoint = memalloc(sizeof(GdkPoint) * cpar[1]);
    if (xpoint == NULL)
      break;

    for (i = 0; i < cpar[1]; i++) {
      xpoint[i].x = dot2pixelx(gtklocal, cpar[i * 2 + 2]);
      xpoint[i].y = dot2pixely(gtklocal, cpar[i * 2 + 3]);
    }
    gdk_draw_polygon(gtklocal->win, gc, FALSE, xpoint, cpar[1]);
    memfree(xpoint);
    break;
  case 'D':
    if (cpar[1] == 0)
      break;

    xpoint = memalloc(sizeof(GdkPoint) * cpar[1]);
    if (xpoint == NULL)
      break;

    for (i = 0; i < cpar[1]; i++) {
      xpoint[i].x = dot2pixelx(gtklocal, cpar[i * 2 + 3]);
      xpoint[i].y = dot2pixely(gtklocal, cpar[i * 2 + 4]);
    }

    if (cpar[2] == 1) {
      gdkgc_set_fill_rule(gc, EvenOddRule);
    } else {
      gdkgc_set_fill_rule(gc, WindingRule);
    }

    gdk_draw_polygon(gtklocal->win, gc, cpar[2] != 0, xpoint, cpar[1]);
    memfree(xpoint);
    break;
  case 'F':
    memfree(gtklocal->fontalias);
    gtklocal->fontalias = nstrdup(cstr);
    break;
  case 'H':
    fontspace = cpar[2] / 72.0 * 25.4;
    gtklocal->fontspace = fontspace;
    fontsize = cpar[1] / 72.0 * 25.4;
    gtklocal->fontsize = fontsize;
    fontcashsize = dot2pixel(gtklocal, fontsize);
    fontcashdir = cpar[3];
    fontdir = cpar[3] * MPI / 18000.0;
    fontsin = sin(fontdir);
    fontcos = cos(fontdir);
    gtklocal->fontdir = (cpar[3] % 36000) / 100.0;
    gtklocal->fontsin = fontsin;
    gtklocal->fontcos = fontcos;

    gtklocal->loadfontf = mxloadfont(gtklocal, TRUE);
    break;
  case 'S':
    if (gtklocal->loadfontf == -1)
      break;

    tmp = strdup(cstr);
    if (tmp == NULL)
      break;

    l = strlen(cstr);
    for (j = i = 0; i <= l; i++, j++) {
      char c;
      if (cstr[i] == '\\') {
	i++;
        if (cstr[i] == 'x') {
	  i++;
	  c = toupper(cstr[i]);
          if (c >= 'A') {
	    tmp[j] = c - 'A' + 10;
	  } else { 
	    tmp[j] = cstr[i] - '0';
	  }

	  i++;
	  tmp[j] *= 16;
	  c = toupper(cstr[i]);
          if (c >= 'A'){
	    tmp[j] += c - 'A' + 10;
	  } else {
	    tmp[j] += cstr[i] - '0';
	  }
        } else if (cstr[i] != '\0') {
          tmp[j] = cstr[i];
        }
      } else {
        tmp[j] = cstr[i];
      }
    }

    tmp2= iso8859_to_utf8(tmp);
    if (tmp2 == NULL) {
      free(tmp);
      break;
    }

    if (gtklocal->font[0].symbol) {
      char *ptr;

      ptr = ascii2greece(tmp2);
      if (ptr) {
	free(tmp2);
	tmp2 = ptr;
      }
    }
    draw_str(gtklocal, gtklocal->win, tmp2, 0, gtklocal->fontsize, gtklocal->fontspace, NULL, NULL, NULL);
    free(tmp2);
    free(tmp);
    break;
  case 'K':
    tmp2 = sjis_to_utf8(cstr);
    if (tmp2 == NULL) 
      break;
    draw_str(gtklocal, gtklocal->win, tmp2, 0, gtklocal->fontsize, gtklocal->fontspace, NULL, NULL, NULL);
    free(tmp2);
    break;
  default:
    break;
  }
  return 0;
}

static int
gtk_charwidth(struct objlist *obj, char *inst, char *rval,
	      int argc, char **argv)
{
  struct gtklocal *gtklocal;
  char ch[3], *font, *tmp;
  double size, dir, s, c;
  int cashpos, width;;

  ch[0] = (*(unsigned int *)(argv[3]) & 0xff);
  ch[1] = (*(unsigned int *)(argv[3]) & 0xff00) >> 8;
  ch[2] = '\0';

  size = (*(int *)(argv[4])) / 72.0 * 25.4;
  font = (char *)(argv[5]);

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  tmp = gtklocal->fontalias;
  gtklocal->fontalias = font;

  cashpos = mxloadfont(gtklocal, FALSE);

  gtklocal->fontalias = tmp;

  if (cashpos == -1) {
    *(int *) rval = nround(size * 0.600);
    return 0;
  }

  dir = gtklocal->fontdir;
  s = gtklocal->fontsin;
  c = gtklocal->fontcos;

  gtklocal->fontdir = 0;
  gtklocal->fontsin = 0;
  gtklocal->fontcos = 1;

  if (ch[1]) {
    tmp = sjis_to_utf8(ch);
    draw_str(gtklocal, NULL, tmp, cashpos, size, 0, &width, NULL, NULL);
    *(int *) rval = pixel2dot(gtklocal, width);
    free(tmp);
  } else {
    tmp = iso8859_to_utf8(ch);
    draw_str(gtklocal, NULL, tmp, cashpos, size, 0, &width, NULL, NULL);
    *(int *) rval = pixel2dot(gtklocal, width);
    free(tmp);
  }

  gtklocal->fontsin = s;
  gtklocal->fontcos = c;
  gtklocal->fontdir = dir;

  return 0;
}

static int
gtk_charheight(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  struct gtklocal *gtklocal;
  char *font, *tmp;
  double size, dir, s, c;
  char *func;
  int height, descent, ascent, cashpos;
  //  XFontStruct *fontstruct;
  struct fontmap *fcur;
  int twobyte;

  func = (char *)argv[1];
  size = (*(int *)(argv[3])) / 72.0 * 25.4;
  font = (char *)(argv[4]);

  if (_getobj(obj, "_local", inst, &gtklocal))
    return 1;

  if (strcmp0(func, "_charascent") == 0) {
    height = TRUE;
  } else {
    height = FALSE;
  }

  fcur = gtklocal->fontmaproot;
  twobyte = FALSE;

  while (fcur) {
    if (strcmp(font, fcur->fontalias) == 0) {
      twobyte = fcur->twobyte;
      break;
    }
    fcur = fcur->next;
  }

  tmp = gtklocal->fontalias;
  gtklocal->fontalias = font;
  cashpos = mxloadfont(gtklocal, FALSE);
  gtklocal->fontalias = tmp;

  if (cashpos < 0) {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }
  }


  dir = gtklocal->fontdir;
  s = gtklocal->fontsin;
  c = gtklocal->fontcos;

  gtklocal->fontdir = 0;
  gtklocal->fontsin = 0;
  gtklocal->fontcos = 1;

  draw_str(gtklocal, NULL, "A", cashpos, size, 0, NULL, &ascent, &descent);

  if (height) {
    *(int *)rval = pixel2dot(gtklocal, ascent);
  } else {
    *(int *)rval = pixel2dot(gtklocal, descent);
  }

  gtklocal->fontsin = s;
  gtklocal->fontcos = c;
  gtklocal->fontdir = dir;

  return 0;
}


struct objtable gra2gtk[] = {
  {"init", NVFUNC, NEXEC, gtkinit, NULL, 0},
  {"done", NVFUNC, NEXEC, gtkdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, gtkdpi, NULL, 0},
  {"auto_redraw", NBOOL, NREAD | NWRITE, gtkautoredraw, NULL, 0},
  {"store_in_memory", NBOOL, NREAD | NWRITE, gtkstorememory, NULL, 0},
  {"redraw", NVFUNC, NREAD | NEXEC, gtkredraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, gtkflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, gtkclear, "", 0},
  {"BR", NINT, NREAD | NWRITE, gtkbr, NULL, 0},
  {"BG", NINT, NREAD | NWRITE, gtkbg, NULL, 0},
  {"BB", NINT, NREAD | NWRITE, gtkbb, NULL, 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
  {"_output", NVFUNC, 0, gtk_output, NULL, 0},
  {"_charwidth", NIFUNC, 0, gtk_charwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gtk_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gtk_charheight, NULL, 0},
  {"_evloop", NVFUNC, 0, gtk_evloop, NULL, 0},
};

#define TBLNUM (sizeof(gra2gtk) / sizeof(*gra2gtk))


void *
addgra2gtk(void)
/* addgra2gtk() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, gra2gtk,
		   ERRNUM, gtkerrorlist, NULL, NULL);
}
