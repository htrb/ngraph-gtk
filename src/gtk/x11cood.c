/*
 * $Id: x11cood.c,v 1.15 2009-11-16 09:13:05 hito Exp $
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

#include "ntime.h"
#include "object.h"
#include "axis.h"

#include "gtk_subwin.h"
#include "gtk_widget.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11cood.h"

void
CoordWinSetFont(const char *font)
{
  if (NgraphApp.CoordWin.data.text && font) {
    set_widget_font(NgraphApp.CoordWin.data.text, font);
  }
}

void
CoordWinSetCoord(int x, int y)
{
  struct objlist *obj;
  int i, num, type;
  char *argv[3];
  double a;
  char *name;
  struct SubWin *d;
  static GString *str = NULL;
  static int lock = FALSE;

  d = &(NgraphApp.CoordWin);

  obj = chkobject("axis");

  if (d->Win == NULL ||
      obj == NULL ||
      d->data.text == NULL) {
    return;
  }

  num = chkobjlastinst(obj) + 1;

  if (lock) {
    return;
  }
  lock = TRUE;

  if (str == NULL) {
    str = g_string_new("");
    gtk_label_set_text(GTK_LABEL(d->data.text), "");
    if (str == NULL) {
      lock = FALSE;
      return;
    }
  }

  g_string_printf(str, "(X:%6.2f  Y:%6.2f)", x / 100.0, y / 100.0);
  argv[0] = (char *) &x;
  argv[1] = (char *) &y;
  argv[2] = NULL;
  for (i = 0; i < num; i++) {
    if (getobj(obj, "coordinate", i, 2, argv, &a) == -1) {
      continue;
    }
    getobj(obj, "group", i, 0, NULL, &name);
    getobj(obj, "type", i, 0, NULL, &type);
    g_string_append_printf(str, "\n%2d %5s %+.7e", i, name, a);
    if (type == AXIS_TYPE_MJD) {
      char *s;

      s = nstrftime("\n        %F %T", a);
      if (s) {
	g_string_append(str, s);
	g_free(s);
      }
    }
  }

  gtk_label_set_text(GTK_LABEL(d->data.text), str->str);

  lock = FALSE;
}

GtkWidget *
CoordWinCreate(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  label_sub_window_create(d);
  CoordWinSetFont(Menulocal.coordwin_font);
  return d->Win;
}
