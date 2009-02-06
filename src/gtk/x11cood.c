/* 
 * $Id: x11cood.c,v 1.7 2009/02/06 08:25:14 hito Exp $
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

#include "ngraph.h"
#include "object.h"

#include "gtk_subwin.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11cood.h"

void
CoordWinSetFont(const char *font)
{
  const char *ptr;
  PangoAttrList *pattr;
  PangoFontDescription *desc;
  GtkLabel *label;
  struct CoordWin *d;

  d = &(NgraphApp.CoordWin);

  label = GTK_LABEL(d->text);

  if (label == NULL)
    return;

  pattr = gtk_label_get_attributes(label);
  if (pattr == NULL) {
    pattr = pango_attr_list_new();
    gtk_label_set_attributes(GTK_LABEL(label), pattr);
  }

  ptr = (font) ? font : "Monospace";

  desc = pango_font_description_from_string(ptr);
  pango_attr_list_change(pattr, pango_attr_font_desc_new(desc));
  pango_font_description_free(desc);
}

void
CoordWinSetCoord(int x, int y)
{
  struct objlist *obj;
  static int bufsize = 0;
  int l, i, j, num;
  char *argv[3];
  double a;
  char *name;
  struct CoordWin *d;

  d = &(NgraphApp.CoordWin);

  obj = chkobject("axis");

  if (obj == NULL || d->text == NULL)
    return;

  num = chkobjlastinst(obj) + 1;
  l = 45 * (num + 1);

  if (l > bufsize) {
    memfree(d->str);
    d->str = (char *) memalloc(l);
    bufsize = l;
  }

  if (d->str == NULL) {
    bufsize = 0;
    return;
  }

  j = 0;
  j += snprintf(d->str + j, bufsize - j, "(X:%6.2f  Y:%6.2f)\n", x / 100.0, y / 100.0);
  argv[0] = (char *) &x;
  argv[1] = (char *) &y;
  argv[2] = NULL;
  for (i = 0; i < num; i++) {
    getobj(obj, "group", i, 0, NULL, &name);
    if (getobj(obj, "coordinate", i, 2, argv, &a) != -1) {
      j += snprintf(d->str + j, bufsize - j, "%d %5s %+.7e\n", i, name, a);
    }
  }

  gtk_label_set_text(GTK_LABEL(d->text), d->str);
}

void
CoordWinUpdate(int clear)
{
}

/*
void
CoordWinUnmap(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct CoordWin *d;
  Position x, y, x0, y0;
  Dimension w0, h0;

  d = &(NgraphApp.CoordWin);
  if (d->Win != NULL) {
    XtVaGetValues(d->Win, XmNx, &x, XmNy, &y,
		  XmNwidth, &w0, XmNheight, &h0, NULL);
    menulocal.coordwidth = w0;
    menulocal.coordheight = h0;
    XtTranslateCoords(TopLevel, 0, 0, &x0, &y0);
    menulocal.coordx = x - x0;
    menulocal.coordy = y - y0;
    XtDestroyWidget(d->Win);
    d->Win = NULL;
    d->text = NULL;
    XmToggleButtonSetState(XtNameToWidget
			   (TopLevel, "*windowmenu.button_4"), False, False);
  }
}
*/

void
CmCoordinateWindow(GtkWidget *w, gpointer client_data)
{
  struct CoordWin *d;

  d = &(NgraphApp.CoordWin);

  if (d->Win) {
    sub_window_toggle_visibility((struct SubWin *) d);
  } else {
    GtkWidget *dlg;

    d ->type = TypeCoordWin;

    dlg = label_sub_window_create((struct SubWin *)d, "Coordinate Window", Coordwin_xpm, Coordwin48_xpm);

    sub_window_show((struct SubWin *) d);
    sub_window_set_geometry((struct SubWin *) d, TRUE);
    CoordWinSetFont(Menulocal.coordwin_font);
  }
}
