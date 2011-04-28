/* 
 * $Id: x11graph.h,v 1.3 2009-06-09 06:38:53 hito Exp $
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

void CmGraphQuit(GtkAction *w, gpointer client_data);
void CmGraphNewMenu(GtkAction *w, gpointer client_data);
void CmGraphHistory(GtkRecentChooser *w, gpointer client_data);
void CmGraphLoad(GtkAction *w, gpointer client_data);
void CmGraphSave(GtkAction *w, gpointer client_data);
void CmGraphOverWrite(GtkAction *w, gpointer client_data);
void CmGraphShell(GtkAction *w, gpointer client_data);
void CmGraphDirectory(GtkAction *w, gpointer client_data);
void CmHelpHelp(GtkAction *w, gpointer client_data);
void CmHelpAbout(GtkAction *w, gpointer client_data);
void CmGraphSwitch(GtkAction *w, gpointer client_data);
void CmGraphPage(GtkAction *w, gpointer client_data);

int set_paper_type(int w, int h);
