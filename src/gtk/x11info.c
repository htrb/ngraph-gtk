/*
 * $Id: x11info.c,v 1.9 2009-07-22 14:53:31 hito Exp $
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

#include "object.h"

#include "gtk_widget.h"
#include "gtk_subwin.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11info.h"

void
InfoWinSetFont(char *font)
{
  if (NgraphApp.InfoWin.data.text && font) {
    set_widget_font(NgraphApp.InfoWin.data.text, font);
  }
}

void
InfoWinDrawInfoText(const char *str)
{
  GtkTextBuffer *buf;
  GtkTextIter iter;
  gint len;
  GtkTextMark *mark;

  if (str == NULL)
    return;

  if (NgraphApp.InfoWin.Win == NULL) {
    return;
  }

  if (NgraphApp.InfoWin.data.text == NULL) {
    return;
  }

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(NgraphApp.InfoWin.data.text));

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
  gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(NgraphApp.InfoWin.data.text), mark);
}

void
InfoWinClear(void)
{
  GtkTextBuffer *buf;

  if (NgraphApp.InfoWin.data.text == NULL)
    return;

  buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(NgraphApp.InfoWin.data.text));
  gtk_text_buffer_set_text(buf, "", 0);
}

GtkWidget *
InfoWinCreate(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  text_sub_window_create(d);
  InfoWinSetFont(Menulocal.infowin_font);
  return d->Win;
}
