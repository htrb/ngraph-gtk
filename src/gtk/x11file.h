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

const char *get_plot_info_str(struct objlist *obj, int id, int src);
void FileWinUpdate(struct obj_list_data *data, int clear, int draw);
void load_data(const char *name);
void init_dnd_file(struct SubWin *d, int type);

void CmRangeAdd(GSimpleAction *action, GVariant *parameter, gpointer client_data);
void CmFileOpen(GSimpleAction *action, GVariant *parameter, gpointer client_data);
void CmFileClose(void *w, gpointer client_data);
void CmFileUpdate(void *w, gpointer client_data);
void CmFileEdit(void *w, gpointer client_data);
void CmFileMath(void *w, gpointer client_data);
void CmFileSaveData(void *w, gpointer client_data);

GtkWidget *create_data_list(struct SubWin *d);

void CmOptionFileDef(void *w, gpointer client_data);
void update_file_obj_multi(struct objlist *obj, struct narray *farray, int newfile, response_cb cb, gpointer user_data);
void copy_file_obj_field(struct objlist *obj, int id, int sel, int copy_filename);
void button_set_mark_image(GtkWidget *w, int type);
