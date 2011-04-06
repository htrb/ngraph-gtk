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
void CmLineMenu(GtkMenuItem *w, gpointer client_data);
void CmPolygonMenu(GtkMenuItem *w, gpointer client_data);
void CmRectangleMenu(GtkMenuItem *w, gpointer client_data);
void CmArcMenu(GtkMenuItem *w, gpointer client_data);
void CmMarkMenu(GtkMenuItem *w, gpointer client_data);
void CmTextMenu(GtkMenuItem *w, gpointer client_data);
void CmLegendWindow(GtkToggleAction *action, gpointer client_data);
void CmOptionTextDef(void);
