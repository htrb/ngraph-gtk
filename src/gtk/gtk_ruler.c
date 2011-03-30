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

#include <math.h>
#include <string.h>

#define RULER_FONT_SIZE       7000
#define RULER_WIDTH           14
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5
#define MAXIMUM_SCALES        10

static void nruler_make_pixmap(Nruler *ruler);
static void nruler_draw_ticks(Nruler *ruler);
static void nruler_draw_pos(Nruler *ruler);
static void nruler_realize(GtkWidget *widget, gpointer user_data);
static void nruler_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data);
static void nruler_size_request(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data);
static gboolean nruler_destroy(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean nruler_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);


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


static Nruler *
ruler_new(int orientation)
{
  Nruler *ruler;
  GtkWidget *w;

  ruler = g_malloc0(sizeof(*ruler));
  if (ruler == NULL) {
    return NULL;
  }

  w = gtk_drawing_area_new();
  ruler->orientation = orientation;
  ruler->widget = w;

  g_signal_connect(w, "expose-event", G_CALLBACK(nruler_expose), ruler);
  g_signal_connect(w, "realize", G_CALLBACK(nruler_realize), ruler);
  g_signal_connect(w, "size-allocate", G_CALLBACK(nruler_size_allocate), ruler);
  g_signal_connect(w, "size-request", G_CALLBACK(nruler_size_request), ruler);
  g_signal_connect(w, "destroy-event", G_CALLBACK(nruler_destroy), ruler);

  return ruler;
}


Nruler *
hruler_new(void)
{
  return ruler_new(GTK_ORIENTATION_HORIZONTAL);
}

Nruler *
vruler_new(void)
{
  return ruler_new(GTK_ORIENTATION_VERTICAL);
}

void
nruler_set_range(Nruler *ruler, double lower, double upper)
{
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
nruler_set_position(Nruler *ruler, double position)
{
  if (ruler == NULL) {
    return;
  }

  ruler->position = position;

  if (gtk_widget_is_drawable(ruler->widget)) {
    gtk_widget_queue_draw(ruler->widget);
  }
}

static gboolean 
nruler_destroy(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  Nruler *ruler;

  ruler = (Nruler *) user_data; 
  if (ruler) {
    g_free(ruler);
  }

  return FALSE;
}

static void 
nruler_size_request(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{
  Nruler *ruler;
  GtkStyle *style;


  ruler = (Nruler *) user_data;
  style = gtk_widget_get_style(widget);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    requisition->width  = style->xthickness * 2 + 1;
    requisition->height = style->ythickness * 2 + RULER_WIDTH;
  } else {
    requisition->width  = style->xthickness * 2 + RULER_WIDTH;
    requisition->height = style->ythickness * 2 + 1;
  }
}

static void 
nruler_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
  Nruler *ruler;

  ruler = (Nruler *) user_data;

  nruler_make_pixmap(ruler);
}

static void
nruler_realize(GtkWidget *widget, gpointer user_data)
{
  Nruler *ruler;

  ruler = (Nruler *) user_data;

  nruler_make_pixmap(ruler);
}

static gboolean
nruler_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if (gtk_widget_is_drawable(widget)) {
    Nruler *ruler = (Nruler *) user_data;

    nruler_draw_ticks(ruler);
    nruler_draw_pos(ruler);
  }

  return FALSE;
}

static void
nruler_make_pixmap(Nruler *ruler)
{
  GtkWidget *widget;
  gint width;
  gint height;
  GtkAllocation allocation;

  widget = ruler->widget;
  gtk_widget_get_allocation(widget, &allocation);

  if (ruler->backing_store) {
    gdk_drawable_get_size(ruler->backing_store, &width, &height);
    if ((width == allocation.width) &&
	(height == allocation.height)) {
      return;
    }

    g_object_unref(ruler->backing_store);
  }

  ruler->backing_store = gdk_pixmap_new(GTK_WIDGET_GET_WINDOW(widget),
					allocation.width,
					allocation.height,
					-1);

  ruler->save_l = 0;
  ruler->save_u = 0;
}

static void
nruler_draw_ticks(Nruler *ruler)
{
  GtkWidget *widget;
  cairo_t *cr;
  gint i, j, len;
  gint width, height;
  gint xthickness;
  gint ythickness;
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
  GtkStyle *style;
  GtkStateType state;

  widget = ruler->widget;
  if (! gtk_widget_is_drawable(widget)) {
    return;
  }

  if (ruler->save_l == ruler->lower && ruler->save_u == ruler->upper) {
    return;
  }

  gtk_widget_get_allocation(widget, &allocation);
  style = gtk_widget_get_style(widget);
  state = gtk_widget_get_state(widget);
  xthickness = style->xthickness;
  ythickness = style->ythickness;

  layout = gtk_widget_create_pango_layout(widget, "012456789");
  fs = pango_font_description_new();
  pango_font_description_set_size(fs, RULER_FONT_SIZE);
  pango_layout_set_font_description(layout, fs);
  pango_font_description_free(fs);

  pango_layout_get_extents(layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS(ink_rect.height) + 2;

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    width = allocation.width;
    height = allocation.height - ythickness * 2;
  } else {
    width = allocation.height;
    height = allocation.width - ythickness * 2;
  }

  gtk_paint_box(style, ruler->backing_store,
		GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		NULL, widget,
		ruler->orientation == GTK_ORIENTATION_HORIZONTAL ?
		"hruler" : "vruler",
		0, 0,
		allocation.width, allocation.height);

  cr = gdk_cairo_create(ruler->backing_store);
  gdk_cairo_set_source_color(cr, &style->fg[state]);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    cairo_rectangle(cr,
		    xthickness,
		    height + ythickness,
		    allocation.width - 2 * xthickness,
		    1);
  } else {
    cairo_rectangle(cr,
		    height + xthickness,
		    ythickness,
		    1,
		    allocation.height - 2 * ythickness);
  }

  upper = ruler->upper / Metric.pixels_per_unit;
  lower = ruler->lower / Metric.pixels_per_unit;;

  if (upper - lower == 0) {
    goto out;
  }

  increment = (gdouble) width / (upper - lower);

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
    ideal_length = height / (i + 1) - 1;
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
      pos = nround((cur - lower) * increment);

      if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
	cairo_rectangle(cr,
			pos, height + ythickness - length,
			1, length);
      } else {
	cairo_rectangle(cr,
			height + xthickness - length, pos,
			length, 1);
      }

      /* draw label */
      if (i == 0) {
	int ofst;

	len = g_snprintf(unit_str, sizeof(unit_str), "%d", (int) cur);
	ofst = PANGO_PIXELS(logical_rect.y - ink_rect.y);

	if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
	  pango_layout_set_text(layout, unit_str, -1);
	  pango_layout_get_extents(layout, &logical_rect, NULL);
	  cairo_move_to(cr,
			pos + 2,
			ythickness + ofst);
	  pango_cairo_show_layout(cr, layout);
	} else {
	  for (j = 0; j < len; j++) {
	    pango_layout_set_text(layout, unit_str + j, 1);
	    pango_layout_get_extents(layout, NULL, &logical_rect);
	    cairo_move_to(cr,
			  xthickness + 1,
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

static void
nruler_draw_pos(Nruler *ruler)
{
  GtkWidget *widget;
  gint x, y;
  gint width, height;
  gint bs_width, bs_height;
  gint xthickness;
  gint ythickness;
  gdouble increment;
  GtkAllocation allocation;
  GtkStyle *style;
  GtkStateType state;
  cairo_t *cr;

  widget = ruler->widget;

  style = gtk_widget_get_style(widget);
  state = gtk_widget_get_state(widget);
  gtk_widget_get_allocation(widget, &allocation);

  if (! gtk_widget_is_drawable(widget)) {
    return;
  }

  xthickness = style->xthickness;
  ythickness = style->ythickness;
  width = allocation.width;
  height = allocation.height;

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    height -= ythickness * 2;
    bs_width = height / 2 + 2;
    bs_width |= 1;  /* make sure it's odd */
    bs_height = bs_width / 2 + 1;
  } else {
    width -= xthickness * 2;
    bs_height = width / 2 + 2;
    bs_height |= 1;  /* make sure it's odd */
    bs_width = bs_height / 2 + 1;
  }

  if (bs_width <= 0 || bs_height <= 0) {
    return;
  }

  cr = gdk_cairo_create(GTK_WIDGET_GET_WINDOW(widget));

  /*  If a backing store exists, restore the ruler  */
  if (ruler->backing_store) {
    gdk_cairo_set_source_pixmap(cr, ruler->backing_store, 0, 0);
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
  }

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    increment = (gdouble) width / (ruler->upper - ruler->lower);

    x = nround((ruler->position - ruler->lower) * increment) - bs_width / 2;
    y = (height + bs_height) / 2;
  } else {
    increment = (gdouble) height / (ruler->upper - ruler->lower);

    x = (width + bs_width) / 2;
    y = nround((ruler->position - ruler->lower) * increment) - bs_height / 2;
  }

  gdk_cairo_set_source_color(cr, &style->fg[state]);

  cairo_move_to(cr, x, y);

  if (ruler->orientation == GTK_ORIENTATION_HORIZONTAL) {
    cairo_line_to(cr, x + bs_width / 2.0, y + bs_height);
    cairo_line_to(cr, x + bs_width, y);
  } else {
    cairo_line_to(cr, x + bs_width, y + bs_height / 2.0);
    cairo_line_to(cr, x, y + bs_height);
  }

  cairo_fill(cr);

  cairo_destroy(cr);
}
