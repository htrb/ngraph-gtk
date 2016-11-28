/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */


#include "gtk_common.h"
#include "gtk_ruler.h"
#include "mathfn.h"
#include "x11menu.h"

#include <math.h>
#include <string.h>

#define RULER_FONT_SIZE       7000
#define RULER_WIDTH           14
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5
#define MAXIMUM_SCALES        10

typedef struct _Nruler {
  int orientation, ofst, length, size;
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *backing_store;
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
  GdkPixmap *backing_store;
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */
  double lower, upper, position;
  double save_l, save_u;
  GtkWidget *widget, *parent;
} Nruler;

struct _NrulerMetric
{
  gchar *metric_name;
  gchar *abbrev;
  /* This should be points_per_unit. This is the size of the unit
   * in 1/72nd's of an inch and has nothing to do with screen pixels */
  gdouble pixels_per_unit;
  gdouble ruler_scale[MAXIMUM_SCALES];
  gint subdivide[MAXIMUM_SUBDIVIDE];        /* five possible modes of subdivision */
};

static const struct _NrulerMetric Metric = {
  "Centimeters", "Cn", 28.35, { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 }, { 1, 5, 10, 50, 100 }
};

#define RULER_DATA_KEY "nruler"

static void nruler_make_pixmap(Nruler *ruler, GtkWidget *widget, GtkWidget *parent);
static void nruler_draw_ticks(Nruler *ruler, GtkWidget *widget);
static void nruler_realize(GtkWidget *widget, gpointer user_data);
static void nruler_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data);
static gboolean nruler_destroy(GtkWidget *widget, gpointer user_data);
#if GTK_CHECK_VERSION(3, 0, 0)
static void nruler_draw_pos(Nruler *ruler, GtkWidget *widget, cairo_t *cr);
static gboolean nruler_expose(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void nruler_get_color(Nruler *ruler, GdkRGBA *fg);
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
static void nruler_draw_pos(Nruler *ruler, GtkWidget *widget);
static gboolean nruler_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static void nruler_get_color(Nruler *ruler, GdkColor *fg, GdkColor *bg);
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

GtkWidget *
nruler_new(GtkOrientation orientation)
{
  Nruler *ruler;
  GtkWidget *w, *frame;

  ruler = g_malloc0(sizeof(*ruler));
  if (ruler == NULL) {
    return NULL;
  }

  w = gtk_drawing_area_new();
  if (orientation == GTK_ORIENTATION_VERTICAL) {
    gtk_widget_set_size_request(w, RULER_WIDTH, -1);
  } else {
    gtk_widget_set_size_request(w, -1, RULER_WIDTH);
  }

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(frame), w);

  ruler->orientation = orientation;
  ruler->widget = w;
  ruler->parent = frame;

  g_object_set_data(G_OBJECT(frame), RULER_DATA_KEY, ruler);

#if GTK_CHECK_VERSION(3, 0, 0)
  g_signal_connect(w, "draw", G_CALLBACK(nruler_expose), ruler);
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
  g_signal_connect(w, "expose-event", G_CALLBACK(nruler_expose), ruler);
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

  g_signal_connect(w, "realize", G_CALLBACK(nruler_realize), ruler);
  g_signal_connect(w, "size-allocate", G_CALLBACK(nruler_size_allocate), ruler);
  g_signal_connect(frame, "unrealize", G_CALLBACK(nruler_destroy), ruler);

  return frame;
}

void
nruler_set_range(GtkWidget *frame, double lower, double upper)
{
  Nruler *ruler;

  if (frame == NULL) {
    return;
  }

  ruler = g_object_get_data(G_OBJECT(frame), RULER_DATA_KEY);
  if (ruler == NULL) {
    return;
  }

  ruler->lower = lower;
  ruler->upper = upper;

  if (gtk_widget_is_drawable(ruler->widget)) {
    gtk_widget_queue_draw(ruler->widget);
  }
}

void
nruler_set_position(GtkWidget *frame, double position)
{
  Nruler *ruler;

  if (frame == NULL) {
    return;
  }

  ruler = g_object_get_data(G_OBJECT(frame), RULER_DATA_KEY);
  if (ruler == NULL) {
    return;
  }

  ruler->position = position;

  if (gtk_widget_is_drawable(ruler->widget)) {
    gtk_widget_queue_draw(ruler->widget);
  }
}

static gboolean
nruler_destroy(GtkWidget *widget, gpointer user_data)
{
  Nruler *ruler;

  g_object_set_data(G_OBJECT(widget), RULER_DATA_KEY, NULL);

  ruler = (Nruler *) user_data;
  if (ruler) {
    if (ruler->backing_store) {
#if GTK_CHECK_VERSION(3, 0, 0)
      cairo_surface_destroy(ruler->backing_store);
#else
      g_object_unref(ruler->backing_store);
#endif
    }
    g_free(ruler);
  }

  return FALSE;
}

static void
nruler_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
  Nruler *ruler;

  ruler = (Nruler *) user_data;

  nruler_make_pixmap(ruler, widget, ruler->parent);
}

static void
nruler_realize(GtkWidget *widget, gpointer user_data)
{
  Nruler *ruler;

  ruler = (Nruler *) user_data;

  nruler_make_pixmap(ruler, widget, ruler->parent);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean
nruler_expose(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  if (gtk_widget_is_drawable(widget)) {
    Nruler *ruler = (Nruler *) user_data;

    nruler_draw_ticks(ruler, widget);
    nruler_draw_pos(ruler, widget, cr);
  }

  return FALSE;
}
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
static gboolean
nruler_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if (gtk_widget_is_drawable(widget)) {
    Nruler *ruler = (Nruler *) user_data;

    nruler_draw_ticks(ruler, widget);
    nruler_draw_pos(ruler, widget);
  }

  return FALSE;
}
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

static void
nruler_make_pixmap(Nruler *ruler, GtkWidget *widget, GtkWidget *parent)
{
  gint width;
  gint height;
  GtkAllocation allocation, parent_allocation;

  if (! gtk_widget_is_drawable(widget)) {
    return;
  }

  gtk_widget_get_allocation(widget, &allocation);
  gtk_widget_get_allocation(parent, &parent_allocation);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    ruler->length = parent_allocation.width;
    ruler->size = allocation.height;
    ruler->ofst = (parent_allocation.width - allocation.width) / 2;
  } else {
    ruler->length = parent_allocation.height;
    ruler->size = allocation.width;
    ruler->ofst = (parent_allocation.height - allocation.height) / 2;
  }

  if (ruler->backing_store) {
#if GTK_CHECK_VERSION(3, 0, 0)
    width = cairo_image_surface_get_width(ruler->backing_store);
    height = cairo_image_surface_get_height(ruler->backing_store);
#elif GTK_CHECK_VERSION(2, 24, 0)
    gdk_pixmap_get_size(ruler->backing_store, &width, &height);
#else
    gdk_drawable_get_size(ruler->backing_store, &width, &height);
#endif
    if ((width == allocation.width) &&
	(height == allocation.height)) {
      return;
    }

#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_surface_destroy(ruler->backing_store);
#else
    g_object_unref(ruler->backing_store);
#endif
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  ruler->backing_store = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
						    allocation.width,
						    allocation.height);
#else
  ruler->backing_store = gdk_pixmap_new(gtk_widget_get_window(widget),
					allocation.width,
					allocation.height,
					-1);
#endif

  ruler->save_l = 0;
  ruler->save_u = 0;
}

static void
nruler_draw_ticks(Nruler *ruler, GtkWidget *widget)
{
  cairo_t *cr;
  gint i, j, len;
  gint length, ideal_length;
  gdouble lower, upper;		/* Upper and lower limits, in ruler units */
  gdouble increment;		/* Number of pixels per unit */
  gint scale;			/* Number of units per major unit */
  gdouble subd_incr;
  gdouble start, end, cur;
  gchar unit_str[32];
  gint digit_height;
  gint text_width;
  gint text_height;
  gint pos;
  PangoLayout *layout;
  PangoRectangle logical_rect, ink_rect;
  PangoFontDescription *fs;
  GtkAllocation allocation;
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkRGBA fg;
  GtkStyleContext *context;
#else
  GdkColor bg, fg;
#endif

  if (! gtk_widget_is_drawable(widget)) {
    return;
  }

  if (ruler->save_l == ruler->lower && ruler->save_u == ruler->upper) {
    return;
  }

  gtk_widget_get_allocation(widget, &allocation);

  layout = gtk_widget_create_pango_layout(widget, "012456789");
  fs = pango_font_description_new();
  pango_font_description_set_size(fs, RULER_FONT_SIZE);
  pango_layout_set_font_description(layout, fs);
  pango_font_description_free(fs);

  pango_layout_get_extents(layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS(ink_rect.height) + 2;

#if GTK_CHECK_VERSION(3, 0, 0)
  nruler_get_color(ruler, &fg);
  cr = cairo_create(ruler->backing_store);
  context = gtk_widget_get_style_context(TopLevel);
  gtk_render_background(context, cr,
			0, 0, allocation.width, allocation.height);
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
  nruler_get_color(ruler, &fg, &bg);
  cr = gdk_cairo_create(ruler->backing_store);
  gdk_cairo_set_source_color(cr, &bg);
  cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
  cairo_fill(cr);
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

#if GTK_CHECK_VERSION(3, 0, 0)
  gdk_cairo_set_source_rgba(cr, &fg);
#else
  gdk_cairo_set_source_color(cr, &fg);
#endif

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    cairo_rectangle(cr, 0, allocation.height - 1, allocation.width, 1);
  } else {
    cairo_rectangle(cr, allocation.width - 1, 0, 1, allocation.height);
  }
  upper = ruler->upper / Metric.pixels_per_unit;
  lower = ruler->lower / Metric.pixels_per_unit;

  if (upper - lower == 0) {
    goto out;
  }

  increment = (gdouble) ruler->length / (upper - lower);

  /* determine the scale H
   *  We calculate the text size as for the vruler instead of using
   *  text_width = gdk_string_width(font, unit_str), so that the result
   *  for the scale looks consistent with an accompanying vruler
   */
  /* determine the scale V
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.
   */

  scale = ceil(ruler->upper / Metric.pixels_per_unit);
  len = g_snprintf(unit_str, sizeof(unit_str), "%d", scale);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    text_width = len * digit_height + 1;

    for (scale = 0; scale < MAXIMUM_SCALES; scale++) {
      if (Metric.ruler_scale[scale] * fabs(increment) > 2 * text_width) {
	break;
      }
    }
  } else {
    text_height = len * digit_height + 1;

    for (scale = 0; scale < MAXIMUM_SCALES; scale++) {
      if (Metric.ruler_scale[scale] * fabs(increment) > 2 * text_height){
	break;
      }
    }
  }

  if (scale == MAXIMUM_SCALES) {
    scale = MAXIMUM_SCALES - 1;
  }

  /* drawing starts here */
  length = 0;
  for (i = MAXIMUM_SUBDIVIDE - 1; i >= 0; i--) {
    subd_incr = Metric.ruler_scale[scale] / Metric.subdivide[i];
    if (subd_incr * fabs(increment) <= MINIMUM_INCR) {
      continue;
    }

    /* Calculate the length of the tickmarks. Make sure that
     * this length increases for each set of ticks
     */
    ideal_length = ruler->size / (i + 1) - 1;
    if (ideal_length > ++length) {
      length = ideal_length;
    }

    if (lower < upper) {
      start = floor(lower / subd_incr) * subd_incr;
      end = ceil(upper / subd_incr) * subd_incr;
    } else {
      start = floor(upper / subd_incr) * subd_incr;
      end = ceil(lower / subd_incr) * subd_incr;
    }

    for (cur = start; cur <= end; cur += subd_incr) {
      pos = nround((cur - lower) * increment) - ruler->ofst;

      if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
	cairo_rectangle(cr,
			pos, ruler->size - length,
			1, length);
      } else {
	cairo_rectangle(cr,
			ruler->size - length, pos,
			length, 1);
      }

      /* draw label */
      if (i == 0) {
	int ofst;

	len = g_snprintf(unit_str, sizeof(unit_str), "%d", (int) cur);
	ofst = PANGO_PIXELS(logical_rect.y - ink_rect.y);

	if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
	  pango_layout_set_text(layout, unit_str, -1);
	  cairo_move_to(cr,
			pos + 2,
			ofst + 1);
	  pango_cairo_show_layout(cr, layout);
	} else {
	  for (j = 0; j < len; j++) {
	    pango_layout_set_text(layout, unit_str + j, 1);
	    cairo_move_to(cr,
			  1,
			  pos + digit_height * j + 2 + ofst);
	    pango_cairo_show_layout(cr, layout);
	  }
	}
      }
    }
  }

  cairo_fill(cr);

  ruler->save_l = ruler->lower;
  ruler->save_u = ruler->upper;
 out:
  cairo_destroy(cr);

  g_object_unref(layout);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void
nruler_get_color(Nruler *ruler, GdkRGBA *fg)
{
  static GtkStyleContext *style = NULL;
  static GdkRGBA saved_fg = {0};

  if (fg == NULL) {
    return;
  }

  if (style == NULL) {
    style = gtk_widget_get_style_context(TopLevel);
    gtk_style_context_get_color(style, GTK_STATE_FLAG_NORMAL, &saved_fg);
  }
  *fg = saved_fg;
}
#else
static void
nruler_get_color(Nruler *ruler, GdkColor *fg, GdkColor *bg)
{
  GtkStateType state;
  GtkStyle *style;

  state = gtk_widget_get_state(ruler->parent);
  style = gtk_widget_get_style(ruler->parent);
  if (fg) {
    *fg = style->fg[state];
  }

  if (bg) {
    *bg = style->bg[state];
  }
}
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

static void
#if GTK_CHECK_VERSION(3, 0, 0)
nruler_draw_pos(Nruler *ruler, GtkWidget *widget, cairo_t *cr)
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
nruler_draw_pos(Nruler *ruler, GtkWidget *widget)
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */
{
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gdouble increment;
  GtkAllocation allocation;
#if GTK_CHECK_VERSION(3, 0, 0)
  GdkRGBA fg;
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
  GdkColor fg;
  cairo_t *cr;
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

  gtk_widget_get_allocation(widget, &allocation);

  if (! gtk_widget_is_drawable(widget)) {
    return;
  }


  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    width = ruler->length;
    height = ruler->size;
    bs_width = height / 2 + 2;
    bs_width |= 1;  /* make sure it's odd */
    bs_height = bs_width / 2 + 1;
  } else {
    width = ruler->size;
    height = ruler->length;
    bs_height = width / 2 + 2;
    bs_height |= 1;  /* make sure it's odd */
    bs_width = bs_height / 2 + 1;
  }

  if (bs_width <= 0 || bs_height <= 0) {
    return;
  }

#if ! GTK_CHECK_VERSION(3, 0, 0)
  cr = gdk_cairo_create(gtk_widget_get_window(widget));
#endif	/* ! GTK_CHECK_VERSION(3, 0, 0) */

  /*  If a backing store exists, restore the ruler  */
  if (ruler->backing_store) {
#if GTK_CHECK_VERSION(3, 0, 0)
    cairo_set_source_surface(cr, ruler->backing_store, 0, 0);
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
    gdk_cairo_set_source_pixmap(cr, ruler->backing_store, 0, 0);
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
  }

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    increment = (gdouble) width / (ruler->upper - ruler->lower);

    x = nround((ruler->position - ruler->lower) * increment) - bs_width / 2 - ruler->ofst;
    y = (height + bs_height) / 2 - 1;
  } else {
    increment = (gdouble) height / (ruler->upper - ruler->lower);

    x = (width + bs_width) / 2 - 1;
    y = nround((ruler->position - ruler->lower) * increment) - bs_height / 2 - ruler->ofst;
  }

#if GTK_CHECK_VERSION(3, 0, 0)
  nruler_get_color(ruler, &fg);
  gdk_cairo_set_source_rgba(cr, &fg);
#else	/* GTK_CHECK_VERSION(3, 0, 0) */
  nruler_get_color(ruler, &fg, NULL);
  gdk_cairo_set_source_color(cr, &fg);
#endif	/* GTK_CHECK_VERSION(3, 0, 0) */

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_move_to(cr, x, y);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    cairo_line_to(cr, x + bs_width / 2.0, y + bs_height);
    cairo_line_to(cr, x + bs_width, y);
  } else {
    cairo_line_to(cr, x + bs_width, y + bs_height / 2.0);
    cairo_line_to(cr, x, y + bs_height);
  }

  cairo_fill(cr);

#if ! GTK_CHECK_VERSION(3, 0, 0)
  cairo_destroy(cr);
#endif	/* ! GTK_CHECK_VERSION(3, 0, 0) */
}
