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

void CmAxisNewFrame(int use_presettings);
void CmAxisNewSection(int use_presettings);
void CmAxisNewCross(int use_presettings);

void CmAxisAddFrame(void *w, gpointer client_data);
void CmAxisAddSection(void *w, gpointer client_data);
void CmAxisAddCross(void *w, gpointer client_data);
void CmAxisAddSingle(void *w, gpointer client_data);

void CmAxisUpdate(void *w, gpointer client_data);
void CmAxisDel(void *w, gpointer client_data);
void CmAxisZoom(void *w, gpointer client_data);
void CmAxisClear(void *w, gpointer client_data);
void CmAxisScaleUndo(void *w, gpointer client_data);

void CmAxisGridNew(void *w, gpointer client_data);
void CmAxisGridDel(void *w, gpointer client_data);
void CmAxisGridUpdate(void *w, gpointer client_data);
void CmOptionGridDef(void *w, gpointer client_data);

GtkWidget *create_axis_list(struct SubWin *d);

void axis_scale_push(struct objlist *obj, int id);
void AxisWinUpdate(struct obj_list_data *data, int clear, int draw);
int axis_check_history(void);
void update_viewer_axisgrid(void);
