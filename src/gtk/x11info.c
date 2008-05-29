/* 
 * $Id: x11info.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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
#include "x11info.h"

#define MAX_LENGTH 512

void
InfoWinDrawInfoText(const char *str)
{
  GtkTextBuffer *buf;
  GtkTextIter iter;
  gint len;
  GtkTextMark *mark;

  if (NgraphApp.InfoWin.text == NULL)
    return;

  if (str == NULL)
    return;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(NgraphApp.InfoWin.text));

  gtk_text_buffer_get_end_iter(buf, &iter);
  gtk_text_buffer_insert(buf, &iter, str, -1);
  len = gtk_text_buffer_get_line_count(buf);

  if (len > Menulocal.info_size) {
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter(buf, &start);
    gtk_text_buffer_get_iter_at_line(buf, &end, len - Menulocal.info_size);
    gtk_text_buffer_delete(buf, &start, &end);
  }
  
  gtk_text_buffer_get_end_iter(buf, &iter);
  gtk_text_buffer_place_cursor(buf, &iter);
  mark = gtk_text_buffer_get_selection_bound(buf);
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(NgraphApp.InfoWin.text), mark);
}

void
InfoWinClear(void)
{
  GtkTextBuffer *buf;

  if (NgraphApp.InfoWin.text == NULL)
    return;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(NgraphApp.InfoWin.text));
  gtk_text_buffer_set_text(buf, "", 0);
 
}

void
InfoWinUpdate(int clear)
{
}

/*
void
InfoWinUnmap(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct InfoWin *d;
  Position x, y, x0, y0;
  Dimension w0, h0;

  d = &(NgraphApp.InfoWin);
  if (d->Win != NULL) {
    XtVaGetValues(d->Win, XmNx, &x, XmNy, &y,
		  XmNwidth, &w0, XmNheight, &h0, NULL);
    menulocal.dialogwidth = w0;
    menulocal.dialogheight = h0;
    XtTranslateCoords(TopLevel, 0, 0, &x0, &y0);
    menulocal.dialogx = x - x0;
    menulocal.dialogy = y - y0;
    XtDestroyWidget(d->Win);
    d->Win = NULL;
    d->text = NULL;
    XmToggleButtonSetState(XtNameToWidget
			   (TopLevel, "*windowmenu.button_5"), False, False);
  }
}
*/

void
CmInformationWindow(GtkWidget *w, gpointer data)
{
  struct InfoWin *d;

  d = &(NgraphApp.InfoWin);
  d ->type = TypeInfoWin;

  if (d->Win) {
    if (GTK_WIDGET_VISIBLE(d->Win)) { 
      gtk_widget_hide_all(d->Win);
    } else {
      gtk_widget_show_all(d->Win);
    }
  } else {
    GtkWidget *dlg;

    dlg = text_sub_window_create((struct SubWin *)d, "Information Window", Infowin_xpm);
    gtk_widget_show_all(dlg);
  }
}
