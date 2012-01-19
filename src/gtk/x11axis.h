/*
 * $Id: x11axis.h,v 1.3 2009-05-14 10:25:27 hito Exp $
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

char *AxisCB(struct objlist *obj, int id);

void CmAxisNewFrame(GtkAction *w, gpointer client_data);
void CmAxisNewSection(GtkAction *w, gpointer client_data);
void CmAxisNewCross(GtkAction *w, gpointer client_data);
void CmAxisNewSingle(GtkAction *w, gpointer client_data);

void CmAxisUpdate(GtkAction *w, gpointer client_data);
void CmAxisDel(GtkAction *w, gpointer client_data);
void CmAxisZoom(GtkAction *w, gpointer client_data);
void CmAxisClear(GtkAction *w, gpointer client_data);
void CmAxisScaleUndo(GtkAction *w, gpointer client_data);

void CmAxisGridNew(GtkAction *w, gpointer client_data);
void CmAxisGridDel(GtkAction *w, gpointer client_data);
void CmAxisGridUpdate(GtkAction *w, gpointer client_data);

void CmAxisWindow(GtkToggleAction *action, gpointer client_data);

void axis_scale_push(struct objlist *obj, int id);
void AxisWinUpdate(int clear);
