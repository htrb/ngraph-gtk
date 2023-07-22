/*
 * $Id: x11view.h,v 1.12 2009-05-01 09:15:59 hito Exp $
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

#define ROTATE_CLOCKWISE 0
#define ROTATE_COUNTERCLOCKWISE 1

struct Point
{
  int x, y;
};

struct FocusObj
{
  struct objlist *obj;
  int oid;
};

enum FOCU_OBJ_TYPE {
  FOCUS_OBJ_TYPE_AXIS   = 0x01,
  FOCUS_OBJ_TYPE_MERGE  = 0x02,
  FOCUS_OBJ_TYPE_LEGEND = 0x04,
  FOCUS_OBJ_TYPE_TEXT   = 0x08,
};

enum FOCUS_MODE {
  FOCUS_MODE_NORMAL = 0,
  FOCUS_MODE_APPEND = 1,
  FOCUS_MODE_TOGGLE = 2,
};

void ViewerWinSetup(void);
void ViewerWinClose(void);
void ViewerWinUpdate(char const **objects);
void ViewerSelectAllObj(struct objlist *obj);
void OpenGC(void);
void CloseGC(void);
void SetScroller(void);
void Focus(struct objlist *fobj, int id, enum FOCUS_MODE mode);
void UnFocus(void);
void ChangeDPI(void);
void CmViewerClear(void *w, gpointer client_data);
void CmViewerDraw(void *w, gpointer client_data);
typedef void (* draw_cb) (gpointer);
void Draw(int SelectFile, draw_cb cb, gpointer user_data);
void CmViewerButtonPressed(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data);
int data_dropped(struct narray *filenames, int file_type);
struct narray *get_dropped_files(const GValue* value);
void draw_paper_frame(void);
void CmEditMenuCB(void *w, gpointer client_data);
void ViewCross(int state);
void ViewerUpdateCB(void *w, gpointer client_data);
int check_focused_obj_type(const struct Viewer *d, int *type);
void move_data_cancel(struct Viewer *d, gboolean show_message);
int check_paint_lock(void);
void update_bg(void);
int get_focused_obj_array(struct narray *focusobj, char **objs);
int graph_dropped(const char *fname);
