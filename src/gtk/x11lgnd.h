/*
 * $Id: x11lgnd.h,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
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

typedef void (* legend_cb_func)(GtkMenuItem *w, gpointer client_data);


void LegendWinUpdate(int clear);

void CmLineUpdate(void *w, gpointer client_data);
void CmLineDel(void *w, gpointer client_data);
void CmRectUpdate(void *w, gpointer client_data);
void CmRectDel(void *w, gpointer client_data);
void CmArcUpdate(void *w, gpointer client_data);
void CmArcDel(void *w, gpointer client_data);
void CmMarkUpdate(void *w, gpointer client_data);
void CmMarkDel(void *w, gpointer client_data);
void CmTextUpdate(void *w, gpointer client_data);
void CmTextDel(void *w, gpointer client_data);

void LegendWinState(struct SubWin *d, int state);
void CmOptionTextDef(void *w, gpointer client_data);
