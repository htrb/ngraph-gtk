/* 
 * $Id: x11view.h,v 1.6 2008/09/10 09:46:44 hito Exp $
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

#define DROP_TYPE_FILE 0
#define DROP_TYPE_TEXT 1

#define FILE_TYPE_AUTO  0
#define FILE_TYPE_MERGE 1
#define FILE_TYPE_DATA  2

struct focuslist
{
  struct objlist *obj;
  int oid;
  int ofsx, ofsy;
};

void ViewerWinSetup(void);
void ViewerWinClose(void);
void ViewerWinUpdate(int clear);
void OpenGC(void);
void CloseGC(void);
void SetScroller(void);
void Focus(struct objlist *fobj, int id, int add);
void UnFocus(int init_cursor);
void ChangeDPI(int redraw);
void Draw(int SelectFile);
void CmViewerDraw(void);
void CmViewerClear(void);
void CmViewerDrawB(GtkWidget *w, gpointer client_data);
void CmViewerClearB(GtkWidget *w, gpointer client_data);
void CmViewerButtonArm(GtkToolItem *w, gpointer client_data);
gboolean CmViewerButtonPressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
int data_dropped(char **filenames, int num, int file_type);
