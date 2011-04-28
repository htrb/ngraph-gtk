/* 
 * $Id: x11file.h,v 1.4 2009-07-26 13:01:40 hito Exp $
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

struct evaltype
{
  int id;
  int line;
  double x, y;
};

void FileWinUpdate(int clear);
void CmFileHistory(GtkWidget *w, gpointer client_data);

void CmFileNew(GtkAction *w, gpointer client_data);
void CmFileOpen(GtkAction *w, gpointer client_data);
void CmFileClose(GtkAction *w, gpointer client_data);
void CmFileUpdate(GtkAction *w, gpointer client_data);
void CmFileEdit(GtkAction *w, gpointer client_data);
void CmFileMath(GtkAction *w, gpointer client_data);
void CmFileSaveData(GtkAction *w, gpointer client_data);

void CmFileWindow(GtkToggleAction *action, gpointer client_data);
void FileWindowUnmap(GtkWidget *w, gpointer client_data);
void CmOptionFileDef(GtkAction *w, gpointer client_data);
int update_file_obj_multi(struct objlist *obj, struct narray *farray, int newfile);
void copy_file_obj_field(struct objlist *obj, int id, int sel, int copy_filename);
void button_set_mark_image(GtkWidget *w, int type);
