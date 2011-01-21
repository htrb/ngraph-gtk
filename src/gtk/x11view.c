/* 
 * $Id: x11view.c,v 1.186 2010-03-04 08:30:17 hito Exp $
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "olegend.h"
#include "oarc.h"
#include "opath.h"
#include "mathfn.h"
#include "ioutil.h"
#include "nstring.h"
#include "shell.h"

#include "gtk_liststore.h"
#include "gtk_widget.h"
#include "strconv.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "ogra2cairo.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11graph.h"
#include "x11file.h"
#include "x11axis.h"
#include "x11info.h"
#include "x11cood.h"
#include "x11lgnd.h"
#include "x11view.h"
#include "x11commn.h"
#include "x11merge.h"

#define ID_BUF_SIZE 16
#define SCROLL_INC 20
#define POINT_ERROR 4

enum ViewerPopupIdn {
  VIEW_UPDATE = 1,
  VIEW_DELETE,
  VIEW_CUT,
  VIEW_COPY,
  VIEW_PASTE,
  VIEW_DUP,
  VIEW_TOP,
  VIEW_LAST,
  VIEW_UP,
  VIEW_DOWN,
  VIEW_CROSS,
  VIEW_ALIGN_LEFT,
  VIEW_ALIGN_RIGHT,
  VIEW_ALIGN_HCENTER,
  VIEW_ALIGN_TOP,
  VIEW_ALIGN_VCENTER,
  VIEW_ALIGN_BOTTOM,
  VIEW_ROTATE_CLOCKWISE,
  VIEW_ROTATE_COUNTER_CLOCKWISE,
  VIEW_FLIP_HORIZONTAL,
  VIEW_FLIP_VERTICAL,
};

enum object_move_type {
  OBJECT_MOVE_TYPE_TOP,
  OBJECT_MOVE_TYPE_UP,
  OBJECT_MOVE_TYPE_DOWN,
  OBJECT_MOVE_TYPE_LAST,
};

struct viewer_popup
{
  char *title;
  gboolean use_stock;
  enum ViewerPopupIdn idn;
  struct viewer_popup *submenu;
  int submenu_item_num;
  enum pop_up_menu_item_type type;
};

#define Button1 1
#define Button2 2
#define Button3 3
#define Button4 4
#define Button5 5

enum EvalDialogColType {
  EVAL_DIALOG_COL_TYPE_ID,
  EVAL_DIALOG_COL_TYPE_LN,
  EVAL_DIALOG_COL_TYPE_X,
  EVAL_DIALOG_COL_TYPE_Y,
  EVAL_DIALOG_COL_TYPE_N,
};

#define POINT_LENGTH 5
#define FOCUS_FRAME_OFST 5
#define FOCUS_RECT_SIZE 6

static GdkRegion *region = NULL;
static int PaintLock = FALSE, ZoomLock = FALSE, DefaultMode = 0, KeepMouseMode = FALSE;

#define EVAL_NUM_MAX 5000
static struct evaltype EvalList[EVAL_NUM_MAX];
static struct narray SelList;
// static char evbuf[256];

#define IDEVMASK        101
#define IDEVMOVE        102

static void ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data);
static void ViewerEvHScroll(GtkRange *range, gpointer user_data);
static void ViewerEvVScroll(GtkRange *range, gpointer user_data);
static gboolean ViewerEvPaint(GtkWidget *w, GdkEventExpose *e, gpointer client_data);
static gboolean ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvButtonDown(GtkWidget *w, GdkEventButton *e, gpointer client_data);
static gboolean ViewerEvButtonUp(GtkWidget *w, GdkEventButton *e, gpointer client_data);
static gboolean ViewerEvMouseMotion(GtkWidget *w, GdkEventMotion *e, gpointer client_data);
static gboolean ViewerEvPopupMenu(GtkWidget *w, gpointer client_data);
static gboolean ViewerEvScroll(GtkWidget *w, GdkEventScroll *e, gpointer client_data);
static gboolean ViewerEvKeyDown(GtkWidget *w, GdkEventKey *e, gpointer client_data);
static gboolean ViewerEvKeyUp(GtkWidget *w, GdkEventKey *e, gpointer client_data);
static void ViewerPopupMenu(GtkWidget *w, gpointer client_data);
static void DelList(struct objlist *obj, N_VALUE *inst);
static void ViewUpdate(void);
static void ViewCopy(void);
static void do_popup(GdkEventButton *event, struct Viewer *d);
static int check_focused_obj(struct narray *focusobj, struct objlist *fobj, int oid);
static int get_mouse_cursor_type(struct Viewer *d, int x, int y);
static void reorder_object(enum object_move_type type);
static void move_data_cancel(struct Viewer *d, gboolean show_message);
static void SetHRuler(struct Viewer *d);
static void SetVRuler(struct Viewer *d);
static void clear_focus_obj(struct narray *focusobj);
static void ViewDelete(void);
static int text_dropped(const char *str, gint x, gint y, struct Viewer *d);
static int add_focus_obj(struct narray *focusobj, struct objlist *obj, int oid);
static void ShowFocusFrame(GdkGC *gc);
static void AddInvalidateRect(struct objlist *obj, N_VALUE *inst);
static void AddList(struct objlist *obj, N_VALUE *inst);
static void RotateFocusedObj(int direction);
static void set_mouse_cursor_hover(struct Viewer *d, int x, int y);
static void CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom);


static int
mxd2p(int r)
{
  return nround(r * Menulocal.local->pixel_dot_x);
}

#if 0
static int
mxd2px(int x)
{
  return nround(x * Menulocal.local->pixel_dot_x + Menulocal.local->offsetx);
  //  return nround(x * Menulocal.pixel_dot + Menulocal.offsetx - Menulocal.scrollx);
}

static int
mxd2py(int y)
{
  return nround(y * Menulocal.local->pixel_dot_y + Menulocal.local->offsety);
  //  return nround(y * Menulocal.pixel_dot + Menulocal.offsety - Menulocal.scrolly);
}
#endif

static int
mxp2d(int r)
{
  return ceil(r / Menulocal.local->pixel_dot_x);
  //  return nround(r / Menulocal.pixel_dot);
}

static double 
range_increment(GtkWidget *w, double inc)
{
  double val;

  val = gtk_range_get_value(GTK_RANGE(w));
  val += inc;
  gtk_range_set_value(GTK_RANGE(w), val);

  return val;
}

static char SCRIPT_IDN[] = "#! ngraph\n# clipboard\n\n";
#define SCRIPT_IDN_LEN (sizeof(SCRIPT_IDN) - 1)

static int
CopyFocusedObjects(void)
{
  struct narray *focus_array;
  struct FocusObj **focus;
  struct objlist *axis, *text;
  char *s;
  int i, r, n, id, num;
  GtkClipboard* clipboard;
  GString *str;

  focus_array = NgraphApp.Viewer.focusobj;
  n = arraynum(focus_array);

  if (n < 1)
    return 1;

  focus = arraydata(focus_array);

  str = g_string_sized_new(256);
  if (str == NULL)
    return 1;

  axis = chkobject("axis");
  text = chkobject("text");
  g_string_append(str, SCRIPT_IDN);
  num = 0;
  for (i = 0; i < n; i++) {
    if (focus[i]->obj == axis) {
      g_string_free(str, TRUE);
      return 1;
    }

    id = chkobjoid(focus[i]->obj, focus[i]->oid);
    if (id < 0)
      continue;

    r = getobj(focus[i]->obj, "save", id, 0, NULL, &s);
    if (r < 0 || s == NULL) {
      g_string_free(str, TRUE);
      return 1;
    }

    g_string_append(str, s);
    num++;
  }

  if (num > 0) {
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, str->str, -1);
  }

  g_string_free(str, TRUE);

  return 0;
}

static int
CutFocusedObjects(void)
{
  if (CopyFocusedObjects())
    return 1;

  ViewDelete();
  return 0;
}

static void
check_last_insts(struct objlist *parent, struct narray *array)
{
  struct objlist *ocur;
  int instnum;

  ocur = parent->child;
  while (chkobjparent(ocur) == parent) {
    instnum = chkobjlastinst(ocur);
    arrayadd(array, &instnum);
    if (ocur->child) {
      check_last_insts(ocur, array);
    }
    ocur = ocur->next;
  }
  return;
}

static void
focus_new_insts(struct objlist *parent, struct narray *array)
{
  struct objlist *ocur;
  int i, instnum, prev_instnum, oid;
  N_VALUE *inst;

  ocur = parent->child;
  while (chkobjparent(ocur) == parent) {
    instnum = chkobjlastinst(ocur);
    prev_instnum = arraynget_int(array, 0);
    arrayndel(array, 0);
    if (chkobjfield(ocur, "bbox") == 0) {
      for (i = prev_instnum + 1; i <= instnum; i++) {
	getobj(ocur, "oid", i, 0, NULL, &oid);
	add_focus_obj(NgraphApp.Viewer.focusobj, ocur, oid);
	inst = chkobjinst(ocur, i);
	AddList(ocur, inst);
	AddInvalidateRect(ocur, inst);
      }
    }
    if (ocur->child) {
      focus_new_insts(ocur, array);
    }
    ocur = ocur->next;
  }
  return;
}


static void 
paste_cb(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
  struct narray idarray;
  GdkGC *dc;
  struct objlist *draw_obj;

  if (text == NULL)
    return;

  if (strncmp(text, SCRIPT_IDN, SCRIPT_IDN_LEN)) {
    gint w, h;

    gdk_window_get_geometry(NgraphApp.Viewer.Win->window, NULL, NULL, &w, &h, NULL);
    text_dropped(text, w / 2, h / 2, &NgraphApp.Viewer);
    return;
  }

  draw_obj = chkobject("draw");
  if (draw_obj == NULL)
    return;

  arrayinit(&idarray, sizeof(int));
  check_last_insts(draw_obj, &idarray);

  UnFocus();

  eval_script(text, TRUE);

  focus_new_insts(draw_obj, &idarray);
  arraydel(&idarray);

  if (arraynum(NgraphApp.Viewer.focusobj) > 0) {
    set_graph_modified();
    dc = gdk_gc_new(NgraphApp.Viewer.gdk_win);
    NgraphApp.Viewer.allclear = FALSE;
    ShowFocusFrame(dc);
    NgraphApp.Viewer.ShowFrame = TRUE;
    g_object_unref(G_OBJECT(dc));
    gtk_widget_grab_focus(NgraphApp.Viewer.Win);
    UpdateAll();
  }
}

static void
PasteObjectsFromClipboard(void)
{
  GtkClipboard *clip;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  if (d->Mode != PointB && d->Mode != LegendB)
    return;

  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  if (gtk_clipboard_wait_is_text_available(clip)) {
    gint x, y;
    gtk_clipboard_request_text(clip, paste_cb, NULL);
    gtk_widget_get_pointer(d->Win, &x, &y);
    set_mouse_cursor_hover(d, x, y);
  }
}

static int
graph_dropped(char *fname)
{
  int load;
  char *ext;

  if (fname == NULL) {
    return 1;
  }

  load = FALSE;

  ext = getextention(fname);
  if (ext == NULL)
    return 1;

  if (strcmp0(ext, "prm") == 0) {

    if (!CheckSave())
      return 0;

    LoadPrmFile(fname);
    load = TRUE;
  } else if (strcmp0(ext, "ngp") == 0) {
    if (!CheckSave())
      return 0;

    LoadNgpFile(fname, Menulocal.ignorepath, Menulocal.expand,
		Menulocal.expanddir, FALSE, "-f");
    load = TRUE;
  }

  if (load) {
    CmViewerDrawB(NULL, NULL);
    return 0;
  }
  return 1;
}

static int
new_merge_obj(char *name, struct objlist *obj)
{
  int id, ret;

  id = newobj(obj);

  if (id < 0)
    return 1;

  changefilename(name);
  putobj(obj, "file", id, name);
  MergeDialog(&DlgMerge, obj, id, -1);
  ret = DialogExecute(TopLevel, &DlgMerge);
  if ((ret == IDDELETE) || (ret == IDCANCEL)) {
    delobj(obj, id);
  } else {
    set_graph_modified();
  }

  return 0;
}


static int
arc_get_angle(struct objlist *obj, N_VALUE *inst, unsigned int round, int point, int px, int py, int *angle1, int *angle2)
{
  int x, y, rx, ry, a1, a2;
  double dx, dy, r, angle;

  if (inst == NULL)
    return 1;

  if (point != ARC_POINT_TYPE_ANGLE1 && point != ARC_POINT_TYPE_ANGLE2)
    return 1;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a1);
  _getobj(obj, "angle2", inst, &a2);

  if (rx < 1 || ry < 1)
    return 1;

  dx = 1.0 * (px - x) / rx;
  dy = 1.0 * (y - py) / ry;
  r = sqrt(dx * dx + dy * dy);

  if (dx == 0 && dy == 0)
    return 1;

  if (dx >= 0 && dy >= 0) {
    if (dx > dy) {
      angle = acos(dx / r) / MPI * 180;
    } else { 
      angle = asin(dy / r) / MPI * 180;
    }
  } else if (dx < 0 && dy >= 0) {
    if (-dx > dy) {
      angle = acos(-dx / r) / MPI * 180;
    } else { 
      angle = asin(dy / r) / MPI * 180;
    }
    angle = 180 - angle;
  } else if (dx < 0 && dy < 0) {
    if (-dx > -dy) {
      angle = acos(-dx / r) / MPI * 180;
    } else { 
      angle = asin(-dy / r) / MPI * 180;
    }
    angle += 180;
  } else {
    if (dx > -dy) {
      angle = acos(dx / r) / MPI * 180;
    } else { 
      angle = asin(-dy / r) / MPI * 180;
    }
    angle = 360 - angle;
  }


  if (round  & GDK_CONTROL_MASK) {
    int tmp;

    angle = nround(angle);
    tmp = angle / 15;
    angle = tmp * 15;
  } else if (! (round & GDK_SHIFT_MASK)) {
    angle = nround(angle);
  }

  angle *= 100;

  switch (point) {
  case ARC_POINT_TYPE_ANGLE1:
    a2 += a1;
    a1 = angle;
    a2 -= a1;
    break;
  case ARC_POINT_TYPE_ANGLE2:
    a2 = angle - a1;
    break;
  }

  a1 %= 36000;
  if (a1 < 0)
    a1 += 36000;

  a2 %= 36000;
  if (a2 < 0) {
    a2 += 36000;
  }

  if (a2 < 500)
    a2 = 36000;

  if (angle1)
    *angle1 = a1;

  if (angle2)
    *angle2 = a2;

  return 0;
}

static int
new_file_obj(char *name, struct objlist *obj, int *id0, int multi)
{
  int id, ret;

  id = newobj(obj);
  if (id < 0) {
    return 1;
  }

  AddDataFileList(name);
  putobj(obj, "file", id, name);
  if (*id0 != -1) {
    copy_file_obj_field(obj, id, *id0, FALSE);
  } else {
    FileDialog(&DlgFile, obj, id, multi);
    ret = DialogExecute(TopLevel, &DlgFile);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      FitDel(obj, id);
      delobj(obj, id);
    } else {
      if (ret == IDFAPPLY)
	*id0 = id;
      set_graph_modified();
    }
  }

  return 0;
}

int
data_dropped(char **filenames, int num, int file_type)
{
  char *name, *ext;
  int i, id0, type, ret;
  struct objlist *obj, *mobj;

  obj = chkobject("file");
  if (obj == NULL) {
    return 1;
  }

  mobj = chkobject("merge");
  if (mobj == NULL) {
    return 1;
  }

  id0 = -1;
  for (i = 0; i < num; i++) {
    name = g_filename_from_uri(filenames[i], NULL, NULL);
    if (name == NULL) {
      continue;
    }

    type = file_type;
    if (type == FILE_TYPE_AUTO) {
      ext = getextention(name);
      if (ext && strcmp0(ext, "gra") == 0) {
	type = FILE_TYPE_MERGE;
      } else {
	type = FILE_TYPE_DATA;
      }
    }

    ret = 1;
    if (type == FILE_TYPE_MERGE) {
      ret = new_merge_obj(name, mobj);
    } else {
      ret = new_file_obj(name, obj, &id0, i < num - 1);
    }

    if (ret) {
      g_free(name);
      continue;
    }
  }

  MergeWinUpdate(TRUE);
  FileWinUpdate(TRUE);
  return 0;
}

static int
text_dropped(const char *str, gint x, gint y, struct Viewer *d)
{
  N_VALUE *inst;
  char *ptr;
  double zoom = Menulocal.PaperZoom / 10000.0;
  struct objlist *obj;
  int id, x1, y1, r, i, j, l;

  obj = chkobject("text");

  if (obj == NULL)
    return 1;

  l = strlen(str);
  ptr = g_malloc(l * 2 + 1);

  if (ptr == NULL)
    return 1;

  for (i = j = 0; i < l; i++, j++) {
    switch (str[i]) {
    case '\n':
      ptr[j] = '\\';
      j++;
      ptr[j] = 'n';
      break;
    case '%':
    case '^':
    case '_':
    case '\\':
      ptr[j] = '\\';
      j++;
      ptr[j] = str[i];
      break;
    default:
      ptr[j] = str[i];
      break;
    }
  }
  ptr[j] = '\0';

  id = newobj(obj);
  if (id < 0) {
    g_free(ptr);
    return 1;
  }

  inst = chkobjinst(obj, id);
  x1 = (mxp2d(x + d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom;
  y1 = (mxp2d(y + d->vscroll - d->cy) - Menulocal.TopMargin) / zoom;

  CheckGrid(FALSE, 0, &x1, &y1, NULL);

  _putobj(obj, "x", inst, &x1);
  _putobj(obj, "y", inst, &y1);
  _putobj(obj, "text", inst, ptr);

  PaintLock= TRUE;

  LegendTextDialog(&DlgLegendText, obj, id);
  r = DialogExecute(TopLevel, &DlgLegendText);

  if ((r == IDDELETE) || (r == IDCANCEL)) {
    delobj(obj, id);
  } else {
    GdkGC *dc;
    int oid;

    UnFocus();

    getobj(obj, "oid", id, 0, NULL, &oid);
    add_focus_obj(NgraphApp.Viewer.focusobj, obj, oid);
    d->allclear = FALSE;
    AddList(obj, inst);
    AddInvalidateRect(obj, inst);

    set_graph_modified();
    dc = gdk_gc_new(d->gdk_win);
    ShowFocusFrame(dc);
    d->ShowFrame = TRUE;
    g_object_unref(G_OBJECT(dc));
    gtk_widget_grab_focus(d->Win);
    UpdateAll();
  }
  PaintLock = FALSE;

  return 0;
}

static void 
drag_drop_cb(GtkWidget *w, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  gchar **filenames, *str;
  int num, r, success;
  struct Viewer *d;

  success = FALSE;
  if (Globallock || Menulock || DnDLock)
    goto End;

  d = (struct Viewer *) user_data;

  switch (info) {
  case DROP_TYPE_TEXT:
    str = (gchar *) gtk_selection_data_get_text(data);
    if (str) {
      r = text_dropped(str, x, y, d);
      g_free(str);
      success = (! r);
    }
    break;
  case DROP_TYPE_FILE:
    filenames = gtk_selection_data_get_uris(data);

    num = g_strv_length(filenames);

    r = 1;
    if (num == 1) {
      char *fname;
      fname = g_filename_from_uri(filenames[0], NULL, NULL);
      if (fname == NULL) {
	g_strfreev(filenames);
	break;
      }
      r = graph_dropped(fname);
      g_free(fname);
    }

    if (r && data_dropped(filenames, num, FILE_TYPE_AUTO) == 0) {
      success = TRUE;
    } else {
      success = TRUE;
    }

    g_strfreev(filenames);
    break;
  }

 End:
  gtk_drag_finish(context, success, FALSE, time);
}


static void
init_dnd(struct Viewer *d)
{
  GtkWidget *widget;
  GtkTargetEntry target[] = {
    {"text/uri-list", 0, DROP_TYPE_FILE},
  };
  GtkTargetList *list;

  widget = d->Win;

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, target, sizeof(target) / sizeof(*target), GDK_ACTION_COPY);

  list = gtk_drag_dest_get_target_list(widget);
  gtk_target_list_add_text_targets(list, DROP_TYPE_TEXT);

  g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_drop_cb), d);
}

static void
eval_dialog_set_parent_cal(GtkWidget *w, GtkTreeIter *iter, int id, int n)
{
  tree_store_set_int(w, iter, EVAL_DIALOG_COL_TYPE_ID, id);
  tree_store_set_int(w, iter, EVAL_DIALOG_COL_TYPE_LN, n);
  tree_store_set_int(w, iter, EVAL_DIALOG_COL_TYPE_N, -1);
}

static void
EvalDialogSetupItem(GtkWidget *w, struct EvalDialog *d)
{
  int i, id, n;
  GtkTreeIter iter, parent;
  char buf[64];

  tree_store_clear(d->list);

  id = -1;
  for (i = d->Num - 1; i >= 0; i--) {
    if (id != EvalList[i].id) {
      if (id >= 0) {
	eval_dialog_set_parent_cal(d->list, &parent, id, n);
      }
      tree_store_prepend(d->list, &parent, NULL);
      id = EvalList[i].id;
      n = 0;
    }

    tree_store_prepend(d->list, &iter, &parent);
    tree_store_set_int(d->list, &iter, EVAL_DIALOG_COL_TYPE_ID, EvalList[i].id);
    tree_store_set_int(d->list, &iter, EVAL_DIALOG_COL_TYPE_LN, EvalList[i].line);

    snprintf(buf, sizeof(buf), "%+.15e", EvalList[i].x);
    tree_store_set_string(d->list, &iter, EVAL_DIALOG_COL_TYPE_X, buf);

    snprintf(buf, sizeof(buf), "%+.15e", EvalList[i].y);
    tree_store_set_string(d->list, &iter, EVAL_DIALOG_COL_TYPE_Y, buf);

    tree_store_set_int(d->list, &iter, EVAL_DIALOG_COL_TYPE_N, i);

    n++;
  }

  eval_dialog_set_parent_cal(d->list, &parent, id, n);

  gtk_tree_view_expand_all(GTK_TREE_VIEW(d->list));
}

static void
eval_dialog_copy_selected(GtkWidget *w, gpointer *user_data)
{
  GtkTreeView *tv;
  GtkClipboard *clip;
  GtkTreeSelection *sel;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GList *list, *ptr;
  GString *str;

  tv = GTK_TREE_VIEW(user_data);
  sel = gtk_tree_view_get_selection(tv);
  list = gtk_tree_selection_get_selected_rows(sel, &model);

  str = g_string_sized_new(256);
  if (str == NULL)
    return;

  for (ptr = g_list_first(list); ptr; ptr = g_list_next(ptr)) {
    gboolean found;
    int id, ln;
    char *x, *y;

    found = gtk_tree_model_get_iter(model, &iter, ptr->data);
    if (! found)
      continue;

    if (gtk_tree_path_get_depth(ptr->data) < 2) {
      gtk_tree_model_get(model, &iter,
			 EVAL_DIALOG_COL_TYPE_ID, &id,
			 EVAL_DIALOG_COL_TYPE_LN, &ln,
			 -1);
      g_string_append_printf(str, "%d %d\n", id, ln);
    } else {
      gtk_tree_model_get(model, &iter,
			 EVAL_DIALOG_COL_TYPE_ID, &id,
			 EVAL_DIALOG_COL_TYPE_LN, &ln,
			 EVAL_DIALOG_COL_TYPE_X,  &x,
			 EVAL_DIALOG_COL_TYPE_Y,  &y,
			 -1);

      if (x && y) {
	g_string_append_printf(str, "%d %d %s %s\n", id, ln, x, y);
      }

      g_free(x);
      g_free(y);
    }
  }

  if (str->len > 0) {
    clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clip, str->str, -1);

    clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, str->str, -1);
  }

  g_string_free(str, TRUE);

  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);
}

static gboolean 
eval_data_sel_cb(GtkTreeSelection *sel, gpointer user_data)
{
  int n;
  GtkWidget *w;

  w = GTK_WIDGET(user_data);

  n = gtk_tree_selection_count_selected_rows(sel);
  gtk_widget_set_sensitive(w, n);

  return FALSE;
}

static void
EvalDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *hbox;
  GtkTreeSelection *sel;
  struct EvalDialog *d;
  n_list_store list[] = {
    {"#",           G_TYPE_INT,    TRUE,  FALSE, NULL, FALSE},
    {_("Line No."), G_TYPE_INT,    TRUE,  FALSE, NULL, FALSE},
    {"X",           G_TYPE_STRING, TRUE,  FALSE, NULL, FALSE},
    {"Y",           G_TYPE_STRING, TRUE,  FALSE, NULL, FALSE},
    {"N",           G_TYPE_INT,    FALSE, FALSE, NULL, FALSE},
  };


  d = (struct EvalDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   _("_Mask"), IDEVMASK,
			   _("_Move"), IDEVMOVE,
			   NULL);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w = tree_store_create(sizeof(list) / sizeof(*list), list);
    tree_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(w, "clicked", G_CALLBACK(tree_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_COPY);
    g_signal_connect(w, "clicked", G_CALLBACK(eval_dialog_copy_selected), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(w, FALSE);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    g_signal_connect(sel, "changed", G_CALLBACK(eval_data_sel_cb), w);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    d->show_cancel = FALSE;
    d->ok_button = GTK_STOCK_CLOSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), 500, 400);
  }
  EvalDialogSetupItem(wi, d);
}

static void 
select_data_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  struct EvalDialog *d;
  int a;

  a = gtk_tree_path_get_depth(path);
  if (a < 2)
    return;

  d = (struct EvalDialog *) data;

  gtk_tree_model_get(model, iter, EVAL_DIALOG_COL_TYPE_N, &a, -1);
  if (a >= 0) {
    arrayadd(d->sel, &a);
  }
}

static void
EvalDialogClose(GtkWidget *w, void *data)
{
  struct EvalDialog *d;
  GtkTreeSelection *selected;

  d = (struct EvalDialog *) data;
  if ((d->ret == IDEVMASK) || (d->ret == IDEVMOVE)) {
    selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    gtk_tree_selection_selected_foreach(selected, select_data_cb, d);
  }
}

void
EvalDialog(struct EvalDialog *data,
	   struct objlist *obj, int num, struct narray *iarray)
{
  data->SetupWindow = EvalDialogSetup;
  data->CloseWindow = EvalDialogClose;
  data->Obj = obj;
  data->Num = num;
  arrayinit(iarray, sizeof(int));
  data->sel = iarray;
}

static GtkWidget *
create_menu(struct viewer_popup *popup, int n, struct Viewer *d)
{
  GtkWidget *menu, *item;
  int i, j = 0;

  menu = gtk_menu_new();

  for (i = 0; i < n; i++) {
    switch (popup[i].type) {
    case POP_UP_MENU_ITEM_TYPE_NORMAL:
    case POP_UP_MENU_ITEM_TYPE_MENU:
      if (popup[i].use_stock) {
	item = gtk_image_menu_item_new_from_stock(popup[i].title, NULL);
      } else {
	item = gtk_menu_item_new_with_mnemonic(_(popup[i].title));
      }

      if (d) {
	d->popup_item[j] = item;
	j++;
      }

      if (popup[i].submenu) {
	GtkWidget *submenu;
	submenu = create_menu(popup[i].submenu, popup[i].submenu_item_num, NULL);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
      } else {
	g_signal_connect(item, "activate", G_CALLBACK(ViewerPopupMenu), GINT_TO_POINTER(popup[i].idn));
      }
      break;
    case POP_UP_MENU_ITEM_TYPE_CHECK:
      item = gtk_check_menu_item_new_with_mnemonic(_(popup[i].title));
      g_signal_connect(item, "toggled", G_CALLBACK(ViewerPopupMenu), GINT_TO_POINTER(popup[i].idn));

      if (d) {
	d->popup_item[j] = item;
	j++;
      }
      break;
    default:
      item = gtk_separator_menu_item_new();
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  return menu;
}

static GtkWidget *
create_popup_menu(struct Viewer *d)
{
  struct viewer_popup align_popup[] = {
    {N_("_Left"),              FALSE, VIEW_ALIGN_LEFT,    NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("_Vertical center"),   FALSE, VIEW_ALIGN_VCENTER, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("_Right"),             FALSE, VIEW_ALIGN_RIGHT,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {NULL, 0, 0, NULL, 0, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
    {N_("_Top"),               FALSE, VIEW_ALIGN_TOP,     NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("_Horizontal center"), FALSE, VIEW_ALIGN_HCENTER, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("_Bottom"),            FALSE, VIEW_ALIGN_BOTTOM,  NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
  };

  struct viewer_popup rotate_popup[] = {
    {N_("_90 degree clockwise"),         FALSE, VIEW_ROTATE_CLOCKWISE,        NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("9_0 degree counter-clockwise"), FALSE, VIEW_ROTATE_COUNTER_CLOCKWISE, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
  };

  struct viewer_popup flip_popup[] = {
    {N_("flip _Horizontally"), FALSE, VIEW_FLIP_HORIZONTAL, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("flip _Vertically"),   FALSE, VIEW_FLIP_VERTICAL,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
  };

  struct viewer_popup popup[] = {
    {GTK_STOCK_CUT,         TRUE,  VIEW_CUT,    NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_COPY,        TRUE,  VIEW_COPY,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_PASTE,       TRUE,  VIEW_PASTE,  NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {N_("_Duplicate"),      FALSE, VIEW_DUP,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_DELETE,      TRUE,  VIEW_DELETE, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {NULL, 0, 0, NULL, 0, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
    {GTK_STOCK_PROPERTIES,  TRUE,  VIEW_UPDATE, NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {NULL, 0, 0, NULL, 0, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
    {N_("_Align"),          FALSE, 0, align_popup,  sizeof(align_popup) / sizeof(*align_popup), POP_UP_MENU_ITEM_TYPE_MENU},
    {N_("_Rotate"),         FALSE, 0, rotate_popup, sizeof(rotate_popup) / sizeof(*rotate_popup), POP_UP_MENU_ITEM_TYPE_MENU},
    {N_("_Flip"),           FALSE, 0, flip_popup,   sizeof(rotate_popup) / sizeof(*rotate_popup), POP_UP_MENU_ITEM_TYPE_MENU},
    {N_("cross _Gauge"),     FALSE, VIEW_CROSS,  NULL, 0, POP_UP_MENU_ITEM_TYPE_CHECK},
    {NULL, 0, 0, NULL, 0, POP_UP_MENU_ITEM_TYPE_SEPARATOR},
    {GTK_STOCK_GOTO_TOP,    TRUE,  VIEW_TOP,    NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_GO_UP,       TRUE,  VIEW_UP,     NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_GO_DOWN,     TRUE,  VIEW_DOWN,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
    {GTK_STOCK_GOTO_BOTTOM, TRUE,  VIEW_LAST,   NULL, 0, POP_UP_MENU_ITEM_TYPE_NORMAL},
  };

#define VIEWER_POPUP_ITEM_CUT     0
#define VIEWER_POPUP_ITEM_COPY    1
#define VIEWER_POPUP_ITEM_PASTE   2
#define VIEWER_POPUP_ITEM_DUP     3
#define VIEWER_POPUP_ITEM_DEL     4
#define VIEWER_POPUP_ITEM_PROP    5
#define VIEWER_POPUP_ITEM_ALIGN   6
#define VIEWER_POPUP_ITEM_ROTATE  7
#define VIEWER_POPUP_ITEM_FLIP    8
#define VIEWER_POPUP_ITEM_CROSS   9
#define VIEWER_POPUP_ITEM_TOP    10
#define VIEWER_POPUP_ITEM_UP     11
#define VIEWER_POPUP_ITEM_DOWN   12
#define VIEWER_POPUP_ITEM_BOTTOM 13

#if VIEWER_POPUP_ITEM_BOTTOM + 1 != VIEWER_POPUP_ITEM_NUM
#error invalid array size (struct Viewer.popup_item)
#endif
  return create_menu(popup, sizeof(popup) / sizeof(*popup), d);
}

static gboolean
scrollbar_scroll_cb(GtkWidget *w, GdkEventScroll *e, gpointer client_data)
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

  switch (e->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_LEFT:
    range_increment(w, -SCROLL_INC);
    break;
  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_RIGHT:
     range_increment(w, SCROLL_INC);
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

static void
menu_activate(GtkMenuShell *menushell, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  if (d->MoveData) {
   move_data_cancel(d, FALSE);
  }
}

void
ViewerWinSetup(void)
{
  struct Viewer *d;
  int x, y, width, height;

  d = &(NgraphApp.Viewer);
  d->gdk_win = d->Win->window;
  Menulocal.GRAoid = -1;
  Menulocal.GRAinst = NULL;
  d->Mode = PointB;
  d->Capture = FALSE;
  d->MoveData = FALSE;
  d->MouseMode = MOUSENONE;
  d->focusobj = arraynew(sizeof(struct FocusObj *));
  d->points = arraynew(sizeof(struct Point *));
  d->FrameOfsX = 0;
  d->FrameOfsY = 0;
  d->LineX = 0;
  d->LineY = 0;
  d->Angle = -1;
  d->CrossX = 0;
  d->CrossY = 0;
  d->ShowFrame = FALSE;
  d->ShowLine = FALSE;
  d->ShowRect = FALSE;
  d->allclear = TRUE;
  d->ignoreredraw = FALSE;
  region = NULL;
  OpenGC();
  OpenGRA();
  SetScroller();
  gdk_window_get_position(d->gdk_win, &x, &y);
  gdk_drawable_get_size(d->gdk_win, &width, &height);
  d->width = width;
  d->height = height;
  d->cx = width / 2;
  d->cy = height / 2;

  d->hscroll = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->vscroll = gtk_range_get_value(GTK_RANGE(d->VScroll));

  ChangeDPI(TRUE);

  g_signal_connect(d->Win, "expose-event", G_CALLBACK(ViewerEvPaint), d);
  g_signal_connect(d->Win, "size-allocate", G_CALLBACK(ViewerEvSize), d);

  g_signal_connect(d->HScroll, "value-changed", G_CALLBACK(ViewerEvHScroll), d);
  g_signal_connect(d->VScroll, "value-changed", G_CALLBACK(ViewerEvVScroll), d);
  g_signal_connect(d->HScroll, "scroll-event", G_CALLBACK(scrollbar_scroll_cb), d);
  g_signal_connect(d->VScroll, "scroll-event", G_CALLBACK(scrollbar_scroll_cb), d);

  init_dnd(d);

  gtk_widget_add_events(d->Win,
			GDK_POINTER_MOTION_MASK |
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON_RELEASE_MASK |
			GDK_BUTTON_PRESS_MASK |
			GDK_KEY_PRESS_MASK |
			GDK_KEY_RELEASE_MASK);
  GTK_WIDGET_SET_FLAGS(d->Win, GTK_CAN_FOCUS);

  g_signal_connect(d->Win, "button-press-event", G_CALLBACK(ViewerEvButtonDown), d);
  g_signal_connect(d->Win, "button-release-event", G_CALLBACK(ViewerEvButtonUp), d);
  g_signal_connect(d->Win, "motion-notify-event", G_CALLBACK(ViewerEvMouseMotion), d);
  g_signal_connect(d->Win, "popup-menu", G_CALLBACK(ViewerEvPopupMenu), d);
  g_signal_connect(d->Win, "scroll-event", G_CALLBACK(ViewerEvScroll), d);
  g_signal_connect(d->Win, "key-press-event", G_CALLBACK(ViewerEvKeyDown), d);
  g_signal_connect(d->Win, "key-release-event", G_CALLBACK(ViewerEvKeyUp), d);

  g_signal_connect(d->menu, "selection-done", G_CALLBACK(menu_activate), d);

  d->popup = create_popup_menu(d);
  gtk_widget_show_all(d->popup);
  gtk_menu_attach_to_widget(GTK_MENU(d->popup), d->Win, NULL);
}

void
ViewerWinClose(void)
{
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  CloseGC();
  CloseGRA();
  arrayfree2(d->focusobj);
  arrayfree2(d->points);

  if (region) {
    gdk_region_destroy(region);
    region = NULL;
  }

}

static int
ViewerWinFileUpdate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  char *argv[7];
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did, id, limit;
  char *dfield;
  N_VALUE *dinst;
  int i;
  struct narray *eval;
  int evalnum;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  char mes[256];
  struct narray dfile;
  int ret;

  ret = FALSE;
  ignorestdio(&save);

  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;
  maxx = (x1 > x2) ? x1 : x2;
  maxy = (y1 > y2) ? y1 : y2;

  limit = 1;

  argv[0] = (char *) &minx;
  argv[1] = (char *) &miny;
  argv[2] = (char *) &maxx;
  argv[3] = (char *) &maxy;
  argv[4] = (char *) &err;
  argv[5] = (char *) &limit;
  argv[6] = NULL;

  fileobj = chkobject("file");
  if (! fileobj)
    goto End;

  arrayinit(&dfile, sizeof(int));

  if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
    goto End;

  if ((snum = arraynum(sarray)) == 0)
    goto End;

  sdata = arraydata(sarray);

  snprintf(mes, sizeof(mes), _("Searching for data."));
  SetStatusBar(mes);
  ProgressDialogCreate(_("Searching for data."));

  for (i = 1; i < snum; i++) {
    dobj = getobjlist(sdata[i], &did, &dfield, NULL);
    if (dobj && chkobjchild(fileobj, dobj)) {
      dinst = chkobjinstoid(dobj, did);
      if (dinst) {
	_getobj(dobj, "id", dinst, &id);
	_exeobj(dobj, "evaluate", dinst, 6, argv);
	_getobj(dobj, "evaluate", dinst, &eval);
	evalnum = arraynum(eval) / 3;
	if (evalnum != 0)
	  arrayadd(&dfile, &id);
      }
    }
  }

  ProgressDialogFinalize();
  ResetStatusBar();

  ret = update_file_obj_multi(fileobj, &dfile, FALSE);
  arraydel(&dfile);

 End:
  restorestdio(&save);
  return ret;
}

static void
mask_selected_data(struct objlist *fileobj, int selnum, struct narray *sel_list)
{
  int masknum, i, j, sel;
  struct narray *mask;

  for (i = 0; i < selnum; i++) {
    sel = arraynget_int(sel_list, i);
    getobj(fileobj, "mask", EvalList[sel].id, 0, NULL, &mask);

    if (mask == NULL) {
      mask = arraynew(sizeof(int));
      putobj(fileobj, "mask", EvalList[sel].id, mask);
    }

    masknum = arraynum(mask);

    if (masknum == 0 || (arraynget_int(mask, masknum - 1)) < EvalList[sel].line) {
      arrayadd(mask, &(EvalList[sel].line));
      exeobj(fileobj, "modified", EvalList[sel].id, 0, NULL);
      set_graph_modified();
    } else if ((arraynget_int(mask, 0)) > EvalList[sel].line) {
      arrayins(mask, &(EvalList[sel].line), 0);
      exeobj(fileobj, "modified", EvalList[sel].id, 0, NULL);
      set_graph_modified();
    } else {
      if (bsearch_int(arraydata(mask), masknum, EvalList[sel].line, &j) == 0) {
	arrayins(mask, &(EvalList[sel].line), j);
	exeobj(fileobj, "modified", EvalList[sel].id, 0, NULL);
	set_graph_modified();
      }
    }
  }
}

static void
Evaluate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  char *argv[7];
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did, id, limit;
  char *dfield;
  N_VALUE *dinst;
  int i, j;
  struct narray *eval;
  int evalnum, tot;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  double line, dx, dy;
  char mes[256];
  int ret;
  int selnum;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;
  maxx = (x1 > x2) ? x1 : x2;
  maxy = (y1 > y2) ? y1 : y2;

  limit = EVAL_NUM_MAX;

  argv[0] = (char *) &minx;
  argv[1] = (char *) &miny;
  argv[2] = (char *) &maxx;
  argv[3] = (char *) &maxy;
  argv[4] = (char *) &err;
  argv[5] = (char *) &limit;
  argv[6] = NULL;

  if ((fileobj = chkobject("file")) == NULL)
    return;

  ignorestdio(&save);

  snprintf(mes, sizeof(mes), _("Evaluating."));
  SetStatusBar(mes);

  if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
    return;

  if ((snum = arraynum(sarray)) == 0)
    return;

  ProgressDialogCreate(_("Evaluating"));

  sdata = arraydata(sarray);
  tot = 0;

  for (i = 1; i < snum; i++) {
    dobj = getobjlist(sdata[i], &did, &dfield, NULL);
    if (dobj && fileobj == dobj) {
      dinst = chkobjinstoid(dobj, did);
      if (dinst) {
	_getobj(dobj, "id", dinst, &id);
	_exeobj(dobj, "evaluate", dinst, 6, argv);
	_getobj(dobj, "evaluate", dinst, &eval);
	evalnum = arraynum(eval) / 3;
	for (j = 0; j < evalnum; j++) {
	  if (tot >= limit) break;
	  tot++;
	  line = arraynget_double(eval, j * 3 + 0);
	  dx = arraynget_double(eval, j * 3 + 1);
	  dy = arraynget_double(eval, j * 3 + 2);
	  EvalList[tot - 1].id = id;
	  EvalList[tot - 1].line = nround(line);
	  EvalList[tot - 1].x = dx;
	  EvalList[tot - 1].y = dy;
	}
	if (tot >= limit) break;
      }
    }
  }

  ProgressDialogFinalize();
  ResetStatusBar();

  if (tot > 0) {
    EvalDialog(&DlgEval, fileobj, tot, &SelList);
    ret = DialogExecute(TopLevel, &DlgEval);
    selnum = arraynum(&SelList);

    if (ret == IDEVMASK) {
      mask_selected_data(fileobj, selnum, &SelList);
      arraydel(&SelList);
    } else if ((ret == IDEVMOVE) && (selnum > 0)) {
      NSetCursor(GDK_TCROSS);
      d->Capture = TRUE;
      d->MoveData = TRUE;
    }
  }
  restorestdio(&save);
}

static void
Trimming(int x1, int y1, int x2, int y2)
{
  struct narray farray;
  struct objlist *obj;
  int i, id;
  int *array, num;
  int vx1, vy1, vx2, vy2;
  int maxx, maxy, minx, miny;
  int dir, rcode1, rcode2, room;
  double ax, ay, ip1, ip2, min, max;
  char *argv[4];
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if ((x1 == x2) && (y1 == y2))
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  SelectDialog(&DlgSelect, obj, AxisCB, (struct narray *) &farray, NULL);

  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    vx1 = x1 - x2;
    vy1 = y1 - y2;
    vx2 = x2 - x1;
    vy2 = y1 - y2;

    num = arraynum(&farray);
    array = arraydata(&farray);

    for (i = 0; i < num; i++) {
      id = array[i];
      getobj(obj, "direction", id, 0, NULL, &dir);

      ax = cos(dir / 18000.0 * MPI);
      ay = -sin(dir / 18000.0 * MPI);

      ip1 = ax * vx1 + ay * vy1;
      ip2 = ax * vx2 + ay * vy2;

      if (fabs(ip1) > fabs(ip2)) {
	if (ip1 > 0) {
	  maxx = x1;
	  maxy = y1;
	  minx = x2;
	  miny = y2;
	} else if (ip1 < 0) {
	  maxx = x2;
	  maxy = y2;
	  minx = x1;
	  miny = y1;
	} else {
	  maxx = minx = 0;
	  maxy = miny = 0;
	}
      } else {
	if (ip2 > 0) {
	  maxx = x2;
	  maxy = y1;
	  minx = x1;
	  miny = y2;
	} else if (ip2 < 0) {
	  maxx = x1;
	  maxy = y2;
	  minx = x2;
	  miny = y1;
	} else {
	  maxx = minx = 0;
	  maxy = miny = 0;
	}
      }

      if ((minx != maxx) && (miny != maxy)) {
	argv[0] = (char *) &minx;
	argv[1] = (char *) &miny;
	argv[2] = NULL;
	rcode1 = getobj(obj, "coordinate", id, 2, argv, &min);

	argv[0] = (char *) &maxx;
	argv[1] = (char *) &maxy;
	argv[2] = NULL;
	rcode2 = getobj(obj, "coordinate", id, 2, argv, &max);

	if ((rcode1 != -1) && (rcode2 != -1)) {
	  axis_scale_push(obj, id);
	  room = 1;
	  argv[0] = (char *) &min;
	  argv[1] = (char *) &max;
	  argv[2] = (char *) &room;
	  argv[3] = NULL;
	  exeobj(obj, "scale", id, 3, argv);
	  set_graph_modified();
	}
      }
    }
    AdjustAxis();
    d->allclear = TRUE;
    UpdateAll();
  }
  arraydel(&farray);
}

static int
Match(char *objname, int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fobj;
  char *argv[6];
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did;
  char *dfield;
  N_VALUE *dinst;
  int i, match, r;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);


  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;
  maxx = (x1 > x2) ? x1 : x2;
  maxy = (y1 > y2) ? y1 : y2;

  argv[0] = (char *) &minx;
  argv[1] = (char *) &miny;
  argv[2] = (char *) &maxx;
  argv[3] = (char *) &maxy;
  argv[4] = (char *) &err;
  argv[5] = NULL;

  fobj = chkobject(objname);
  if (! fobj)
    return 0;

  if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
    return 0;

  if ((snum = arraynum(sarray)) == 0)
    return 0;

  ignorestdio(&save);

  sdata = arraydata(sarray);

  r = 0;
  for (i = 1; i < snum; i++) {
    dobj = getobjlist(sdata[i], &did, &dfield, NULL);

    if (! dobj || ! chkobjchild(fobj, dobj))
      continue;

    dinst = chkobjinstoid(dobj, did);
    if (! dinst)
      continue;

    _exeobj(dobj, "match", dinst, 5, argv);
    _getobj(dobj, "match", dinst, &match);
    if (! match)
      continue;

    if (add_focus_obj(d->focusobj, dobj, did)) {
      r++;
    }
  }

  restorestdio(&save);
  return r;
}

static void
AddList(struct objlist *obj, N_VALUE *inst)
{
  int addi;
  struct objlist *aobj;
  char *afield;
  int i, j, po, num, oid, id, id2;
  struct objlist **objlist;
  struct objlist *obj2;
  N_VALUE *inst2, *ainst;
  char *field, **objname;
  struct narray *draw, drawrable;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  aobj = obj;
  ainst = inst;
  afield = "draw";
  addi = -1;
  _getobj(obj, "id", inst, &id);

  draw = &(Menulocal.drawrable);
  num = arraynum(draw);

  if (num == 0) {
    arrayinit(&drawrable, sizeof(char *));
    menuadddrawrable(chkobject("draw"), &drawrable);
    draw = &drawrable;
    num = arraynum(draw);
  }

  objlist = (struct objlist **) g_malloc(sizeof(struct objlist *) * num);
  if (objlist == NULL)
    return;

  po = 0;
  for (i = 0; i < num; i++) {
    objname = (char **) arraynget(draw, i);
    objlist[i] = chkobject(*objname);
    if (objlist[i] == obj)
      po = i;
  }

  i = 1;
  j = 0;
  while ((obj2 = GRAgetlist(Menulocal.GC, &oid, &field, i)) != NULL) {
    for (; j < num; j++) {
      if (objlist[j] == obj2) break;
    }
    if (j == po) {
      inst2 = chkobjinstoid(obj2, oid);
      if (inst2 == NULL) {
	GRAdellist(Menulocal.GC, i);
	continue;
      }
      _getobj(obj2, "id", inst2, &id2);
      if (id2 > id) {
	addi = i;
	g_free(objlist);
	mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
	if (draw != &(Menulocal.drawrable)) {
	  arraydel2(draw);
	}
	return;
      }
    } else if (j > po) {
      addi = i;
      g_free(objlist);
      mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
      if (draw != &(Menulocal.drawrable)) {
	arraydel2(draw);
      }
      return;
    }
    i++;
  }
  addi = i;
  g_free(objlist);
  mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
  if (draw != &(Menulocal.drawrable))
    arraydel2(draw);
}

static void
DelList(struct objlist *obj, N_VALUE *inst)
{
  int i, oid, oid2;
  struct objlist *obj2;
  char *field;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  _getobj(obj, "oid", inst, &oid);
  i = 0;
  while ((obj2 = GRAgetlist(Menulocal.GC, &oid2, &field, i)) != NULL) {
    if ((obj2 == obj) && (oid == oid2))
      mx_dellist(Menulocal.obj, Menulocal.inst, i);
    i++;
  }
}

static void
AddInvalidateRect(struct objlist *obj, N_VALUE *inst)
{
  struct narray *abbox;
  int bboxnum, *bbox;
  double zoom;
  struct Viewer *d;
  GdkRectangle rect;

  d = &(NgraphApp.Viewer);
  if (chkobjfield(obj, "bbox"))
    return;

  _exeobj(obj, "bbox", inst, 0, NULL);
  _getobj(obj, "bbox", inst, &abbox);
  bboxnum = arraynum(abbox);
  bbox = arraydata(abbox);
  if (bboxnum >= 4) {
    zoom = Menulocal.PaperZoom / 10000.0;

    rect.x = mxd2p(bbox[0] * zoom + Menulocal.LeftMargin) - 7;
    rect.y = mxd2p(bbox[1] * zoom + Menulocal.TopMargin) - 7;
    rect.width = mxd2p(bbox[2] * zoom + Menulocal.LeftMargin) - rect.x + 7;
    rect.height = mxd2p(bbox[3] * zoom + Menulocal.TopMargin) - rect.y + 7;

    if (region == NULL)
      region = gdk_region_new();

    gdk_region_union_with_rect(region, &rect);
  }
}


static void
GetLargeFrame(int *minx, int *miny, int *maxx, int *maxy)
{
  int i, num;
  struct FocusObj **focus;
  struct narray *abbox;
  int bboxnum, *bbox;
  N_VALUE *inst;
  struct savedstdio save;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  ignorestdio(&save);
  *minx = *miny = *maxx = *maxy = 0;

  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);

  inst = chkobjinstoid(focus[0]->obj, focus[0]->oid);
  if (inst) {
    _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
    _getobj(focus[0]->obj, "bbox", inst, &abbox);

    bboxnum = arraynum(abbox);
    bbox = arraydata(abbox);

    if (bboxnum >= 4) {
      *minx = bbox[0];
      *miny = bbox[1];
      *maxx = bbox[2];
      *maxy = bbox[3];
    }
  }
  for (i = 1; i < num; i++) {
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst) {
      _exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
      _getobj(focus[i]->obj, "bbox", inst, &abbox);

      bboxnum = arraynum(abbox);
      bbox = arraydata(abbox);

      if (bboxnum >= 4) {
	if (bbox[0] < *minx)
	  *minx = bbox[0];
	if (bbox[1] < *miny)
	  *miny = bbox[1];
	if (bbox[2] > *maxx)
	  *maxx = bbox[2];
	if (bbox[3] > *maxy)
	  *maxy = bbox[3];
      }
    }
  }
  restorestdio(&save);
}

static int 
coord_conv_x(int x, double zoom, struct Viewer *d)
{
  return mxd2p(x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
}

static int 
coord_conv_y(int y, double zoom, struct Viewer *d)
{
  return mxd2p(y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;
}

static void
GetFocusFrame(int *minx, int *miny, int *maxx, int *maxy, int ofsx, int ofsy)
{
  int x1, y1, x2, y2;
  double zoom;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  GetLargeFrame(&x1, &y1, &x2, &y2);

  zoom = Menulocal.PaperZoom / 10000.0;

  *minx = coord_conv_x((x1 + ofsx), zoom, d);
  *miny = coord_conv_y((y1 + ofsy), zoom, d);

  *maxx = coord_conv_x((x2 + ofsx), zoom, d);
  *maxy = coord_conv_y((y2 + ofsy), zoom, d);
}

static void
ShowFocusFrame(GdkGC *gc)
{
  int i, j, num;
  struct FocusObj **focus;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  int x1, y1, x2, y2;
  N_VALUE *inst;
  struct savedstdio save;
  double zoom;
  struct Viewer *d;
  int minx, miny, height, width;

  d = &(NgraphApp.Viewer);
  ignorestdio(&save);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_line_attributes(gc, 1,
			     (Menulocal.focus_frame_type ==  GDK_LINE_SOLID) ? GDK_LINE_SOLID : GDK_LINE_ON_OFF_DASH,
			     GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_gc_set_function(gc, GDK_XOR);

  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);

  if (num > 0) {
    GetFocusFrame(&x1, &y1, &x2, &y2, d->FrameOfsX, d->FrameOfsY);

    x1 -= FOCUS_FRAME_OFST;
    y1 -= FOCUS_FRAME_OFST;
    x2 += FOCUS_FRAME_OFST - 1;
    y2 += FOCUS_FRAME_OFST - 1;

    minx = (x1 < x2) ? x1 : x2;
    miny = (y1 < y2) ? y1 : y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);

    gdk_draw_rectangle(d->gdk_win, gc, FALSE, minx, miny, width, height);

    gdk_draw_rectangle(d->gdk_win, gc, TRUE,
		       x1 - FOCUS_RECT_SIZE,
		       y1 - FOCUS_RECT_SIZE,
		       FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    gdk_draw_rectangle(d->gdk_win, gc, TRUE,
		       x1 - FOCUS_RECT_SIZE,
		       y2,
		       FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    gdk_draw_rectangle(d->gdk_win, gc, TRUE,
		       x2,
		       y1 - FOCUS_RECT_SIZE,
		       FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    gdk_draw_rectangle(d->gdk_win, gc, TRUE,
		       x2,
		       y2,
		       FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
  }
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num > 1) {
    for (i = 0; i < num; i++) {
      inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
      if (inst) {
	_exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
	_getobj(focus[i]->obj, "bbox", inst, &abbox);

	bboxnum = arraynum(abbox);
	bbox = arraydata(abbox);

	if (bboxnum >= 4) {
	  x1 = coord_conv_x((bbox[0] + d->FrameOfsX), zoom, d);
	  y1 = coord_conv_y((bbox[1] + d->FrameOfsY), zoom, d);
	  x2 = coord_conv_x((bbox[2] + d->FrameOfsX), zoom, d);
	  y2 = coord_conv_y((bbox[3] + d->FrameOfsY), zoom, d);

	  minx = (x1 < x2) ? x1 : x2;
	  miny = (y1 < y2) ? y1 : y2;

	  width = abs(x2 - x1);
	  height = abs(y2 - y1);

	  gdk_draw_rectangle(d->gdk_win, gc, FALSE, minx, miny, width, height);
	}
      }
    }
  } else if (num == 1) {
    i = 0;
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst) {
      _exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
      _getobj(focus[i]->obj, "bbox", inst, &abbox);

      bboxnum = arraynum(abbox);
      bbox = arraydata(abbox);

      for (j = 4; j < bboxnum; j += 2) {
	x1 = coord_conv_x((bbox[j] + d->FrameOfsX), zoom, d);
	y1 = coord_conv_y((bbox[j + 1] + d->FrameOfsY), zoom, d);

	gdk_draw_rectangle(d->gdk_win, gc, TRUE,
			   x1 - FOCUS_RECT_SIZE / 2,
			   y1 - FOCUS_RECT_SIZE / 2,
			   FOCUS_RECT_SIZE,
			   FOCUS_RECT_SIZE);
      }
    }
  }
  gdk_gc_set_function(gc, GDK_COPY);

  restorestdio(&save);
}

static void
AlignFocusedObj(int align)
{
  int i, num, bboxnum, *bbox, minx, miny, maxx, maxy, dx, dy;
  struct FocusObj **focus;
  struct narray *abbox;
  char *argv[4];
  N_VALUE *inst;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  num = arraynum(d->focusobj);

  if (num < 1) {
    return;
  }

  focus = arraydata(d->focusobj);

  if (num == 1) {
    maxx = Menulocal.PaperWidth;
    maxy = Menulocal.PaperHeight;
    minx = 0;
    miny = 0;
  } else {
    GetLargeFrame(&minx, &miny, &maxx, &maxy);
  }

  if (maxx < minx || maxy < miny)
    return;

  d->allclear = FALSE;

  PaintLock = TRUE;

  for (i = 0; i < num; i++) {
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst == NULL) {
      continue;
    }
    _getobj(focus[i]->obj, "bbox", inst, &abbox);

    bboxnum = arraynum(abbox);
    bbox = arraydata(abbox);

    if (bboxnum < 4) {
      continue;
    }

    dx = dy = 0;
    switch (align) {
    case VIEW_ALIGN_LEFT:
      dx = minx - bbox[0];
      break;
    case VIEW_ALIGN_VCENTER:
      dx = (maxx + minx - bbox[2] - bbox[0]) / 2;
      break;
    case VIEW_ALIGN_RIGHT:
      dx = maxx - bbox[2];
      break;
    case VIEW_ALIGN_TOP:
      dy = miny - bbox[1];
      break;
    case VIEW_ALIGN_HCENTER:
      dy = (maxy + miny - bbox[3] - bbox[1]) / 2;
      break;
    case VIEW_ALIGN_BOTTOM:
      dy = maxy - bbox[3];
      break;
    }
    
    if (dx == 0 && dy == 0)
      continue;

    argv[0] = (char *) &dx;
    argv[1] = (char *) &dy;
    argv[2] = NULL;

    if (focus[i]->obj == chkobject("axis")) {
      d->allclear = TRUE;
    }

    AddInvalidateRect(focus[i]->obj, inst);
    _exeobj(focus[i]->obj, "move", inst, 2, argv);
    set_graph_modified();
    AddInvalidateRect(focus[i]->obj, inst);
  }
  PaintLock = FALSE;
  UpdateAll();
}

static void
execute_selected_instances(struct FocusObj **focus, int num, int argc, char **argv, char *field)
{
  N_VALUE *inst;
  int i;

  for (i = 0; i < num; i++) {
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst == NULL) {
      continue;
    }
    if (chkobjfield(focus[i]->obj, field) == 0) {
      AddInvalidateRect(focus[i]->obj, inst);
      _exeobj(focus[i]->obj, field, inst, argc, argv);
      set_graph_modified();
      AddInvalidateRect(focus[i]->obj, inst);
    }
  }
}

static void
RotateFocusedObj(int direction)
{
  int num, minx, miny, maxx, maxy, angle, type;
  int use_pivot, px, py;
  struct FocusObj **focus;
  char *argv[5];
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  num = check_focused_obj_type(d, &type);
  if (num < 1 || (type & FOCUS_OBJ_TYPE_MERGE)) {
    return;
  }

  angle = (direction == ROTATE_CLOCKWISE) ? 27000 : 9000;

  focus = arraydata(d->focusobj);

  PaintLock = TRUE;

  argv[0] = (char *) &angle;
  argv[1] = (char *) &use_pivot;
  argv[2] = (char *) &px;
  argv[3] = (char *) &py;
  argv[4] = NULL;

  if (num == 1) {
    use_pivot = 0;
    px = 0;
    py = 0;
  } else {
    GetLargeFrame(&minx, &miny, &maxx, &maxy);
    use_pivot = 1;
    px = (minx + maxx) / 2;
    py = (miny + maxy) / 2;
  }

  execute_selected_instances(focus, num, 4, argv, "rotate");

  PaintLock = FALSE;
  UpdateAll();
}

static void
FlipFocusedObj(enum FLIP_DIRECTION dir)
{
  int num, minx, miny, maxx, maxy, type;
  int use_pivot, p;
  struct FocusObj **focus;
  char *argv[4];
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  num = check_focused_obj_type(d, &type);
  if (num < 1 || (type & FOCUS_OBJ_TYPE_MERGE)) {
    return;
  }

  focus = arraydata(d->focusobj);

  PaintLock = TRUE;

  argv[0] = (char *) &dir;
  argv[1] = (char *) &use_pivot;
  argv[2] = (char *) &p;
  argv[3] = NULL;

  if (num == 1) {
    use_pivot = 0;
    p = 0;
  } else {
    GetLargeFrame(&minx, &miny, &maxx, &maxy);
    use_pivot = 1;
    p = (dir == FLIP_DIRECTION_HORIZONTAL) ? (minx + maxx) / 2 : (miny + maxy) / 2;
  }

  execute_selected_instances(focus, num, 3, argv, "flip");

  PaintLock = FALSE;
  UpdateAll();
}

static void
show_focus_line_arc(GdkGC *gc, int clear, unsigned int state, int change, double zoom, struct objlist *obj, N_VALUE *inst, struct Viewer *d)
{
  int x, y, rx, ry, a1, a2;
  static unsigned int prev_state = 0;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a1);
  _getobj(obj, "angle2", inst, &a2);

  switch (change) {
  case ARC_POINT_TYPE_R:
    ry -= d->LineY;
    rx -= d->LineX;
    break;
  case ARC_POINT_TYPE_ANGLE1:
  case ARC_POINT_TYPE_ANGLE2:
    if (clear) {
      unsigned int tmp;
      tmp = state;
      state = prev_state;
      prev_state = tmp;
    } else {
      prev_state = state;
    }
    if (arc_get_angle(obj, inst, state, change, d->MouseX2, d->MouseY2, &a1, &a2))
      return;
    d->Angle = (change == ARC_POINT_TYPE_ANGLE1) ? a1 : (a1 + a2) % 36000;
    break;
  }

  if (! Menulocal.do_not_use_arc_for_draft && rx > 0 && ry > 0) {
    rx = mxd2p(rx * zoom);
    ry = mxd2p(ry * zoom);
    x = coord_conv_x(x, zoom, d);
    y = coord_conv_y(y, zoom, d);
    gdk_draw_arc(d->gdk_win, gc, FALSE, x - rx, y - ry, rx * 2, ry * 2, a1 / 100.0 * 64, a2 / 100.0 * 64);
  }
}

static void
draw_frame_rect(GdkGC *gc, int change, double zoom, int *bbox, struct Viewer *d)
{
  
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  int minx, miny, height, width;

  switch (change) {
  case 0:
    x1 = coord_conv_x(bbox[4] + d->LineX, zoom, d);
    y1 = coord_conv_y(bbox[5] + d->LineY, zoom, d);
    x2 = coord_conv_x(bbox[8], zoom, d);
    y2 = coord_conv_y(bbox[9], zoom, d);
    break;
  case 1:
    x1 = coord_conv_x(bbox[4], zoom, d);
    y1 = coord_conv_y(bbox[5] + d->LineY, zoom, d);
    x2 = coord_conv_x(bbox[8] + d->LineX, zoom, d);
    y2 = coord_conv_y(bbox[9], zoom, d);
    break;
  case 2:
    x1 = coord_conv_x(bbox[4], zoom, d);
    y1 = coord_conv_y(bbox[5], zoom, d);
    x2 = coord_conv_x(bbox[8] + d->LineX, zoom, d);
    y2 = coord_conv_y(bbox[9] + d->LineY, zoom, d);
    break;
  case 3:
    x1 = coord_conv_x(bbox[4] + d->LineX, zoom, d);
    y1 = coord_conv_y(bbox[5], zoom, d);
    x2 = coord_conv_x(bbox[8], zoom, d);
    y2 = coord_conv_y(bbox[9] + d->LineY, zoom, d);
    break;
  }
  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;

  width = abs(x2 - x1);
  height = abs(y2 - y1);

  gdk_draw_rectangle(d->gdk_win, gc, FALSE, minx, miny, width, height);
}

static void
draw_focus_line(GdkGC *gc, int change, double zoom, int bboxnum, int *bbox, struct Viewer *d)
{
  int j, ofsx, ofsy, x0, y0, x1, y1;

  for (j = 4; j < bboxnum; j += 2) {
    if (change == (j - 4) / 2) {
      ofsx = d->LineX;
      ofsy = d->LineY;
    } else {
      ofsx = 0;
      ofsy = 0;
    }

    x1 = coord_conv_x(bbox[j] + ofsx, zoom, d);
    y1 = coord_conv_y(bbox[j + 1] + ofsy, zoom, d);
    if (j != 4) {
      gdk_draw_line(d->gdk_win, gc, x0, y0, x1, y1);
    }
    x0 = x1;
    y0 = y1;
  }
}

static void
ShowFocusLine(GdkGC *gc, int clear, unsigned int state, int change)
{
  int num;
  struct FocusObj **focus;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  N_VALUE *inst;
  struct savedstdio save;
  double zoom;
  int frame;
  char *group;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  ignorestdio(&save);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  num = arraynum(d->focusobj);

  focus = arraydata(d->focusobj);
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num != 1)
    goto End;

  inst = chkobjinstoid(focus[0]->obj, focus[0]->oid);
  if (inst == NULL)
    goto End;

  _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
  _getobj(focus[0]->obj, "bbox", inst, &abbox);

  bboxnum = arraynum(abbox);
  bbox = arraydata(abbox);

  frame = FALSE;

  if (focus[0]->obj == chkobject("rectangle")) {
    frame = TRUE;
  } else if (focus[0]->obj == chkobject("arc")) {
    show_focus_line_arc(gc, clear, state, change, zoom, focus[0]->obj, inst, d);
    goto End;
  } else if (focus[0]->obj == chkobject("axis")) {
    _getobj(focus[0]->obj, "group", inst, &group);
    if (group && group[0] != 'a') {
      frame = TRUE;
    }
  }

  if (!frame) {
    draw_focus_line(gc, change, zoom, bboxnum, bbox, d);
  } else {
    draw_frame_rect(gc, change, zoom, bbox, d);
  }

 End:
  gdk_gc_set_function(gc, GDK_COPY);
  restorestdio(&save);
}

static void
ShowPoints(GdkGC *gc)
{
  int i, num, x0 = 0, y0 = 0, x1, y1, x2, y2;
  struct Point **po;
  double zoom;
  struct Viewer *d;
  int minx, miny, height, width;

  d = &(NgraphApp.Viewer);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);

  num = arraynum(d->points);
  po = arraydata(d->points);

  zoom = Menulocal.PaperZoom / 10000.0;

  if (d->Mode & POINT_TYPE_DRAW1) {
    if (num >= 2) {
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);

      x1 = coord_conv_x(po[0]->x, zoom, d);
      y1 = coord_conv_y(po[0]->y, zoom, d);
      x2 = coord_conv_x(po[1]->x, zoom, d);
      y2 = coord_conv_y(po[1]->y, zoom, d);

      minx = (x1 < x2) ? x1 : x2;
      miny = (y1 < y2) ? y1 : y2;

      width = abs(x2 - x1);
      height = abs(y2 - y1);

      if (d->Mode == ArcB && ! Menulocal.do_not_use_arc_for_draft) {
	gdk_draw_arc(d->gdk_win, gc, FALSE, minx, miny, width, height, 0, 360 * 64);
      } else {
	gdk_draw_rectangle(d->gdk_win, gc, FALSE, minx, miny, width, height);
      }
    }
  } else {
    gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    for (i = 0; i < num; i++) {
      x1 = coord_conv_x(po[i]->x, zoom, d);
      y1 = coord_conv_y(po[i]->y, zoom, d);

      gdk_draw_line(d->gdk_win, gc,
		    x1 - (POINT_LENGTH - 1), y1,
		    x1 + POINT_LENGTH, y1);
      gdk_draw_line(d->gdk_win, gc,
		    x1, y1 - (POINT_LENGTH - 1),
		    x1, y1 + POINT_LENGTH);
    }

    gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
    if (num >= 1) {
      x1 = coord_conv_x(po[0]->x, zoom, d);
      y1 = coord_conv_y(po[0]->y, zoom, d);

      x0 = x1;
      y0 = y1;
    }
    for (i = 1; i < num; i++) {
      x1 = coord_conv_x(po[i]->x, zoom, d);
      y1 = coord_conv_y(po[i]->y, zoom, d);

      gdk_draw_line(d->gdk_win, gc, x0, y0, x1, y1);

      x0 = x1;
      y0 = y1;
    }
  }
  gdk_gc_set_function(gc, GDK_COPY);
}

static void
ShowFrameRect(GdkGC *gc)
{
  int x1, y1, x2, y2;
  double zoom;
  struct Viewer *d;
  int minx, miny, width, height;

  d = &(NgraphApp.Viewer);

  zoom = Menulocal.PaperZoom / 10000.0;

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);

  if ((d->MouseX1 != d->MouseX2) || (d->MouseY1 != d->MouseY2)) {
    x1 = coord_conv_x(d->MouseX1, zoom, d);
    y1 = coord_conv_y(d->MouseY1, zoom, d);

    x2 = coord_conv_x(d->MouseX2, zoom, d);
    y2 = coord_conv_y(d->MouseY2, zoom, d);

    minx = (x1 < x2) ? x1 : x2;
    miny = (y1 < y2) ? y1 : y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);

    gdk_draw_rectangle(d->gdk_win, gc, FALSE, minx, miny, width, height);
  }
  gdk_gc_set_function(gc, GDK_COPY);
}

static void
ShowCrossGauge(GdkGC *gc)
{
  int x, y, width, height;
  double zoom;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  gdk_drawable_get_size(d->gdk_win, &width, &height);

  zoom = Menulocal.PaperZoom / 10000.0;

  x = coord_conv_x(d->CrossX, zoom, d);
  y = coord_conv_y(d->CrossY, zoom, d);

  gdk_draw_line(d->gdk_win, gc, x, 0, x, height);
  gdk_draw_line(d->gdk_win, gc, 0, y, width, y);
  gdk_gc_set_function(gc, GDK_COPY);
}

static void
CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom)
{
  int offset;
  int grid;

  if ((state & GDK_CONTROL_MASK) && (!ofs)) {
    if ((x != NULL) && (y != NULL)) {
      if (abs(*x) > abs(*y))
	*y = 0;
      else
	*x = 0;
    }
  }
  grid = Menulocal.grid;
  if (!(state & GDK_SHIFT_MASK)) {
    if (ofs) {
      offset = grid / 2;
    } else {
      offset = 0;
    }

    if (x != NULL)
      *x = ((*x + offset) / grid) * grid;

    if (y != NULL)
      *y = ((*y + offset) / grid) * grid;

    if (zoom != NULL)
      *zoom = nround(*zoom * grid) / ((double) grid);
  }
}

static void
mouse_down_point(unsigned int state, TPoint *point, struct Viewer *d, GdkGC *dc)
{
  double zoom;

  zoom = Menulocal.PaperZoom / 10000.0;

  d->Capture = TRUE;

  if (arraynum(d->focusobj) && ! (state & GDK_SHIFT_MASK)) {
    ShowFocusFrame(dc);
    d->ShowFrame = FALSE;
    clear_focus_obj(d->focusobj);
  }

  d->MouseMode = MOUSEPOINT;
  d->ShowRect = TRUE;
}

static void
init_zoom(unsigned int state, struct Viewer *d, GdkGC *dc)
{
  int vx1, vy1, vx2, vy2;
  double cc, nn, zoom2;

  ShowFocusFrame(dc);

  d->ShowFrame = FALSE;
  d->MouseDX = d->RefX2 - d->MouseX1;
  d->MouseDY = d->RefY2 - d->MouseY1;

  vx1 = d->MouseX1;
  vy1 = d->MouseY1;

  vx1 -= d->RefX1 - d->MouseDX;
  vy1 -= d->RefY1 - d->MouseDY;

  vx2 = (d->RefX2 - d->RefX1);
  vy2 = (d->RefY2 - d->RefY1);

  cc = vx1 * vx2 + vy1 * vy2;
  nn = vx2 * vx2 + vy2 * vy2;

  if ((nn == 0) || (cc < 0)) {
    zoom2 = 0;
  } else {
    zoom2 = cc / nn;
  }

  CheckGrid(FALSE, state, NULL, NULL, &zoom2);

  vx1 = d->RefX1 + vx2 * zoom2;
  vy1 = d->RefY1 + vy2 * zoom2;

  d->MouseX1 = d->RefX1;
  d->MouseY1 = d->RefY1;

  d->MouseX2 = vx1;
  d->MouseY2 = vy1;

  ShowFrameRect(dc);

  d->ShowRect = TRUE;
}

static void
mouse_down_move(unsigned int state, TPoint *point, struct Viewer *d, GdkGC *dc)
{
  int cursor;

  cursor = get_mouse_cursor_type(d, point->x, point->y);

  if (cursor == GDK_LEFT_PTR) {
    NSetCursor(cursor);
    return;
  }

  d->Capture = TRUE;

  switch (cursor) {
  case GDK_TOP_LEFT_CORNER:
    GetLargeFrame(&(d->RefX2), &(d->RefY2), &(d->RefX1), &(d->RefY1));
    d->MouseMode = MOUSEZOOM1;
    NSetCursor(cursor);
    init_zoom(state, d, dc);
    break;
  case GDK_TOP_RIGHT_CORNER:
    GetLargeFrame(&(d->RefX1), &(d->RefY2), &(d->RefX2), &(d->RefY1));
    d->MouseMode = MOUSEZOOM2;
    NSetCursor(cursor);
    init_zoom(state, d, dc);
    break;
  case GDK_BOTTOM_RIGHT_CORNER:
    GetLargeFrame(&(d->RefX1), &(d->RefY1), &(d->RefX2), &(d->RefY2));
    d->MouseMode = MOUSEZOOM3;
    NSetCursor(cursor);
    init_zoom(state, d, dc);
    break;
  case GDK_BOTTOM_LEFT_CORNER:
    GetLargeFrame(&(d->RefX2), &(d->RefY1), &(d->RefX1), &(d->RefY2));
    d->MouseMode = MOUSEZOOM4;
    NSetCursor(cursor);
    init_zoom(state, d, dc);
    break;
  case GDK_CROSSHAIR:
    d->MouseMode = MOUSECHANGE;
    d->Angle = -1;
    ShowFocusFrame(dc);
    d->ShowFrame = FALSE;
    d->ShowLine = TRUE;
    d->LineX = d->LineY = 0;
    ShowFocusLine(dc, FALSE, state, d->ChangePoint);
    NSetCursor(cursor);
    break;
  case GDK_FLEUR:
    d->MouseMode = MOUSEDRAG;
    break;
  }
}

static void
mouse_down_move_data(TPoint *point, struct Viewer *d)
{
  struct objlist *fileobj, *aobjx, *aobjy;
  struct narray iarray, *move, *movex, *movey;
  int selnum, sel, i, ax, ay, anum, iline, j, movenum;
  double dx, dy;
  char *axis, *argv[3];
  int *ptr;

  fileobj = chkobject("file");
  if (fileobj == NULL)
    goto ErrEnd;

  selnum = arraynum(&SelList);

  for (i = 0; i < selnum; i++) {
    sel = arraynget_int(&SelList, i);

    if (getobj(fileobj, "axis_x", EvalList[sel].id, 0, NULL, &axis) == -1)
      goto ErrEnd;

    arrayinit(&iarray, sizeof(int));

    if (getobjilist(axis, &aobjx, &iarray, FALSE, NULL)) {
      ax = -1;
    } else {
      anum = arraynum(&iarray);
      ax = (anum < 1) ? -1 : (arraylast_int(&iarray));
      arraydel(&iarray);
    }

    if (getobj(fileobj, "axis_y", EvalList[sel].id, 0, NULL, &axis) == -1)
      goto ErrEnd;

    arrayinit(&iarray, sizeof(int));

    if (getobjilist(axis, &aobjy, &iarray, FALSE, NULL)) {
      ay = -1;
    } else {
      anum = arraynum(&iarray);
      ay = (anum < 1) ? -1 : (arraylast_int(&iarray));
      arraydel(&iarray);
    }

    if (ax == -1 || ax == -1)
      goto ErrEnd;

    argv[0] = (char *) &(d->MouseX1);
    argv[1] = (char *) &(d->MouseY1);
    argv[2] = NULL;

    if (getobj(aobjx, "coordinate", ax, 2, argv, &dx) == -1 ||
	getobj(aobjy, "coordinate", ay, 2, argv, &dy) == -1)
      goto ErrEnd;

    if (exeobj(fileobj, "move_data_adjust", EvalList[sel].id, 0, NULL) == -1)
      goto ErrEnd;

    if (getobj(fileobj, "move_data", EvalList[sel].id, 0, NULL, &move) == -1)
      goto ErrEnd;

    if (getobj(fileobj, "move_data_x", EvalList[sel].id, 0, NULL, &movex) == -1)
      goto ErrEnd;

    if (getobj(fileobj, "move_data_y", EvalList[sel].id, 0, NULL, &movey) == - 1)
      goto ErrEnd;

    if (move == NULL) {
      move = arraynew(sizeof(int));
      putobj(fileobj, "move_data", EvalList[sel].id, move);
    }

    if (movex == NULL) {
      movex = arraynew(sizeof(double));
      putobj(fileobj, "move_data_x", EvalList[sel].id, movex);
    }

    if (movey == NULL) {
      movey = arraynew(sizeof(double));
      putobj(fileobj, "move_data_y", EvalList[sel].id, movey);
    }

    movenum = arraynum(move);

    for (j = 0; j < movenum; j++) {
      ptr = (int *) arraynget(move, j);
      if (ptr) {
	iline = * ptr;
	if (iline == EvalList[sel].line)
	  break;
      }
    }

    if (j == movenum) {
      arrayadd(move, &(EvalList[sel].line));
      arrayadd(movex, &dx);
      arrayadd(movey, &dy);
      set_graph_modified();
    } else {
      arrayput(move, &(EvalList[sel].line), j);
      arrayput(movex, &dx, j);
      arrayput(movey, &dy, j);
      set_graph_modified();
    }
  }

  if (selnum > 0)
    message_box(NULL, _("Data points are moved."), "Confirm", RESPONS_OK);

 ErrEnd:
  move_data_cancel(d, FALSE);
}

static void
mouse_down_zoom(unsigned int state, TPoint *point, struct Viewer *d, int zoom_out)
{
  int vdpi;

  if (ZoomLock)
    return;

  ZoomLock = TRUE;
  if (state & GDK_SHIFT_MASK) {
    d->hscroll -= (d->cx - point->x);
    d->vscroll -= (d->cy - point->y);

    ChangeDPI(TRUE);
  } else if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &vdpi) != -1) {
    if (zoom_out) {
      if ((int) (vdpi / sqrt(2)) >= 20) {
	vdpi = nround(vdpi / sqrt(2));

	if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
	  d->hscroll -= (d->cx - point->x);
	  d->vscroll -= (d->cy - point->y);

	  //	    ChangeDPI(FALSE);
	  ChangeDPI(TRUE);
	}
      } else {
	message_beep(TopLevel);
      }
    } else {
      if ((int) (vdpi * sqrt(2)) <= 620) {
	vdpi = nround(vdpi * sqrt(2));
	if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
	  d->hscroll -= (d->cx - point->x);
	  d->vscroll -= (d->cy - point->y);
	  //	    ChangeDPI(FALSE);
	  ChangeDPI(TRUE);
	}
      } else {
	message_beep(TopLevel);
      }
    }
  }
  ZoomLock = FALSE;
}

static void
mouse_down_set_points(unsigned int state, struct Viewer *d, GdkGC *dc, int n)
{
  int x1, y1, i;
  struct Point *po;

  if (d->Capture)
    return;

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL);

  for (i = 0; i < n; i++) {
    po = (struct Point *) g_malloc(sizeof(struct Point));
    if (po) {
      po->x = x1;
      po->y = y1;
      arrayadd(d->points, &po);
    }
  }

  ShowPoints(dc);
  d->Capture = TRUE;
}

static gboolean
ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;
  double zoom;
  int pos;

  if (Menulock || Globallock)
    return FALSE;

  if (region)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  d->MouseX1 = d->MouseX2 = (mxp2d(point->x - d->cx + d->hscroll)
			     - Menulocal.LeftMargin) / zoom;

  d->MouseY1 = d->MouseY2 = (mxp2d(point->y - d->cy + d->vscroll)
			     - Menulocal.TopMargin) / zoom;

  d->MouseMode = MOUSENONE;


  if (d->MoveData) {
    mouse_down_move_data(point, d);
    return TRUE;
  }

  dc = gdk_gc_new(d->gdk_win);

  switch (d->Mode) {
  case PointB:
  case LegendB:
  case AxisB:
    pos = NGetCursor();
    if (pos == GDK_LEFT_PTR) {
      mouse_down_point(state, point, d, dc);
    } else {
      mouse_down_move(state, point, d, dc);
    }
    break;
  case TrimB:
  case DataB:
  case EvalB:
    d->Capture = TRUE;
    d->MouseMode = MOUSEPOINT;
    d->ShowRect = TRUE;
    break;
  case MarkB:
  case TextB:
    mouse_down_set_points(state, d, dc, 1);
    break;
  case ZoomB:
    mouse_down_zoom(state, point, d, state & GDK_CONTROL_MASK);
    break;
  default:
    mouse_down_set_points(state, d, dc, 2);
    break;
  }

  g_object_unref(G_OBJECT(dc));

  return TRUE;
}

static void
mouse_up_point(unsigned int state, TPoint *point, struct Viewer *d, GdkGC *dc, double zoom)
{
  int x1, x2, y1, y2, err;

  d->Capture = FALSE;
  ShowFrameRect(dc);

  d->ShowRect = FALSE;

  d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  x2 = d->MouseX2;
  y2 = d->MouseY2;

  err = mxp2d(POINT_ERROR) / zoom;

  switch (d->Mode) {
  case PointB:
  case AxisB:
    Match("axis", x1, y1, x2, y2, err);
    /* fall-through */
  case LegendB:
    if (d->Mode != AxisB) {
      Match("legend", x1, y1, x2, y2, err);
      Match("merge", x1, y1, x2, y2, err);
    }
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    ShowFocusFrame(dc);
    break;
  case TrimB:
    Trimming(x1, y1, x2, y2);
    break;
  case DataB:
    if (ViewerWinFileUpdate(x1, y1, x2, y2, err)) {
      UpdateAll();
    }
    break;
  case EvalB:
    Evaluate(x1, y1, x2, y2, err);
    break;
  default:
    /* never reached */
    break;
  }
}

static void
mouse_up_drag(unsigned int state, TPoint *point, double zoom, struct Viewer *d, GdkGC *dc)
{
  int i, dx, dy, num, axis;
  char *argv[5];
  N_VALUE *inst;
  struct FocusObj *focus;
  struct objlist *obj;

  axis = FALSE;

  if ((d->MouseX1 != d->MouseX2) || (d->MouseY1 != d->MouseY2)) {
    ShowFocusFrame(dc);

    d->ShowFrame = FALSE;

    d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		  - Menulocal.LeftMargin) / zoom;

    d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		  - Menulocal.TopMargin) / zoom;

    dx = d->MouseX2 - d->MouseX1;
    dy = d->MouseY2 - d->MouseY1;

    CheckGrid(FALSE, state, &dx, &dy, NULL);

    num = arraynum(d->focusobj);

    PaintLock = TRUE;

    if (dx != 0 || dy != 0) {
      argv[0] = (char *) &dx;
      argv[1] = (char *) &dy;
      argv[2] = NULL;

      for (i = num - 1; i >= 0; i--) {
	focus = *(struct FocusObj **) arraynget(d->focusobj, i);
	obj = focus->obj;

	if (obj == chkobject("axis"))
	  axis = TRUE;

	inst = chkobjinstoid(focus->obj, focus->oid);
	if (inst) {
	  AddInvalidateRect(obj, inst);
	  _exeobj(obj, "move", inst, 2, argv);
	  set_graph_modified();
	  AddInvalidateRect(obj, inst);
	}
      }
    }

    PaintLock = FALSE;
    d->FrameOfsX = d->FrameOfsY = 0;
    ShowFocusFrame(dc);
    d->ShowFrame = TRUE;
    if (d->Mode == LegendB || (d->Mode == PointB && !axis))
      d->allclear=FALSE;
    UpdateAll();
  }
}

static void
mouse_up_zoom(unsigned int state, TPoint *point, double zoom, struct Viewer *d, GdkGC *dc)
{
  int vx1, vy1, vx2, vy2, zm, i, num, axis;
  double cc, nn, zoom2;
  char *argv[5];
  N_VALUE *inst;
  struct FocusObj *focus;
  struct objlist *obj;

  axis = FALSE;

  ShowFrameRect(dc);
  d->ShowRect = FALSE;

  vx1 = (mxp2d(point->x - d->cx + d->hscroll)
	 - Menulocal.LeftMargin) / zoom;

  vy1 = (mxp2d(point->y - d->cy + d->vscroll)
	 - Menulocal.TopMargin) / zoom;

  d->MouseX2 = vx1;
  d->MouseY2 = vy1;

  vx1 -= d->RefX1 - d->MouseDX;
  vy1 -= d->RefY1 - d->MouseDY;

  vx2 = (d->RefX2 - d->RefX1);
  vy2 = (d->RefY2 - d->RefY1);

  cc = vx1 * vx2 + vy1 * vy2;
  nn = vx2 * vx2 + vy2 * vy2;

  if ((nn == 0) || (cc < 0)) {
    zoom2 = 0;
  } else {
    zoom2 = cc / nn;
  }

  if ((d->Mode != DataB) && (d->Mode != EvalB)) {
    CheckGrid(FALSE, state, NULL, NULL, &zoom2);
  }

  zm = nround(zoom2 * 10000);

  if (zm < 1000)
    zm = 1000;

  if (zm != 10000) {
    argv[0] = (char *) &zm;
    argv[1] = (char *) &(d->RefX1);
    argv[2] = (char *) &(d->RefY1);
    argv[3] = (char *) &Menulocal.preserve_width;
    argv[4] = NULL;

    num = arraynum(d->focusobj);
    PaintLock = TRUE;

    for (i = num - 1; i >= 0; i--) {
      focus = *(struct FocusObj **) arraynget(d->focusobj, i);
      obj = focus->obj;

      if (obj == chkobject("axis"))
	axis = TRUE;

      inst = chkobjinstoid(focus->obj, focus->oid);
      if (inst) {
	AddInvalidateRect(obj, inst);
	_exeobj(obj, "zooming", inst, 4, argv);
	set_graph_modified();
	AddInvalidateRect(obj, inst);
      }
    }
  }

  PaintLock = FALSE;

  d->FrameOfsX = d->FrameOfsY = 0;
  d->ShowFrame = TRUE;

  ShowFocusFrame(dc);
  if (d->Mode == LegendB || (d->Mode == PointB && !axis))
    d->allclear = FALSE;

  UpdateAll();
}

static void
mouse_up_change(unsigned int state, TPoint *point, double zoom, struct Viewer *d, GdkGC *dc)
{
  int dx, dy, axis;
  char *argv[5];
  N_VALUE *inst;
  struct FocusObj *focus;
  struct objlist *obj;

  axis = FALSE;

  ShowFocusLine(dc, FALSE, state, d->ChangePoint);
  d->ShowLine = FALSE;

  if ((d->MouseX1 != d->MouseX2) || (d->MouseY1 != d->MouseY2)) {
    d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		  - Menulocal.LeftMargin) / zoom;

    d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		  - Menulocal.TopMargin) / zoom;

    dx = d->MouseX2 - d->MouseX1;
    dy = d->MouseY2 - d->MouseY1;

    if ((d->Mode != DataB) && (d->Mode != EvalB)) {
      CheckGrid(FALSE, state, &dx, &dy, NULL);
    }

    if (dx != 0 || dy != 0) {
      argv[0] = (char *) &(d->ChangePoint);
      argv[1] = (char *) &dx;
      argv[2] = (char *) &dy;
      argv[3] = NULL;

      PaintLock = TRUE;

      focus = *(struct FocusObj **) arraynget(d->focusobj, 0);

      obj = focus->obj;
      inst = chkobjinstoid(focus->obj, focus->oid);

      if (obj == chkobject("arc") &&
	  (d->ChangePoint == ARC_POINT_TYPE_ANGLE1 || d->ChangePoint == ARC_POINT_TYPE_ANGLE2)) {
	if (arc_get_angle(obj, inst, state, d->ChangePoint, d->MouseX2, d->MouseY2, &dx, &dy))
	  inst = NULL;
      } else if (obj == chkobject("axis")) {
	axis = TRUE;
      }

      if (inst) {
	AddInvalidateRect(obj, inst);
	_exeobj(obj, "change", inst, 3, argv);
	set_graph_modified();
	AddInvalidateRect(obj, inst);
      }

      PaintLock = FALSE;
    }
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    ShowFocusFrame(dc);
    if (d->Mode == LegendB || (d->Mode == PointB && !axis))
      d->allclear = FALSE;
    UpdateAll();
  } else {
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    ShowFocusFrame(dc);
  }
}

static void
mouse_up_lgend1(unsigned int state, TPoint *point, double zoom, struct Viewer *d, GdkGC *dc)
{
  int x1, y1, num;
  struct Point *po;

  d->Capture = FALSE;
  ShowPoints(dc);

  d->MouseX1 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY1 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL);

  num = arraynum(d->points);
  if (num >= 1) {
    po = *(struct Point **) arraynget(d->points, 0);
    po->x = x1;
    po->y = y1;
  }
  ShowPoints(dc);
  if (arraynum(d->points) == 1) {
    ViewerEvLButtonDblClk(state, point, d);
  }
}

static void
mouse_up_lgend2(unsigned int state, TPoint *point, double zoom, struct Viewer *d, GdkGC *dc)
{
  int num, x1, y1;
  struct Point *po;

  ShowPoints(dc);

  d->MouseX1 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY1 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL);

  num = arraynum(d->points);

  if (num >= 2) {
    po = *(struct Point **) arraynget(d->points, num - 2);
  }

  if ((num < 2) || (po->x != x1) || (po->y != y1)) {
    po = (struct Point *) g_malloc(sizeof(struct Point));
    if (po) {
      po->x = x1;
      po->y = y1;

      arrayadd(d->points, &po);
    }
  }

  ShowPoints(dc);

  if ((d->Mode & POINT_TYPE_DRAW1) || d->Mode == SingleB) {
    if (arraynum(d->points) == 3) {
      d->Capture = FALSE;
      ViewerEvLButtonDblClk(state, point, d);
    }
  }
}

static gboolean
ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;
  double zoom;

  if (Menulock || Globallock)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  if (! d->Capture)
    return TRUE;

  dc = gdk_gc_new(d->gdk_win);

  switch (d->Mode) {
  case PointB:
  case LegendB:
  case AxisB:
  case TrimB:
  case DataB:
  case EvalB:
    d->Capture = FALSE;
    switch (d->MouseMode) {
    case MOUSEDRAG:
      mouse_up_drag(state, point, zoom, d, dc);
      break;
    case MOUSEZOOM1:
    case MOUSEZOOM2:
    case MOUSEZOOM3:
    case MOUSEZOOM4:
      mouse_up_zoom(state, point, zoom, d, dc);
      break;
    case MOUSECHANGE:
      mouse_up_change(state, point, zoom, d, dc);
      break;
    case MOUSEPOINT:
      mouse_up_point(state, point, d, dc, zoom);
      if (d->Mode & POINT_TYPE_POINT) {
	d->allclear = FALSE;
	UpdateAll();
      }
      break;
    case MOUSENONE:
      break;
    }
    NSetCursor(get_mouse_cursor_type(d, point->x, point->y));
    d->MouseMode = MOUSENONE;
    SetPoint(d, d->MouseX2, d->MouseY2);
    break;
  case MarkB:
  case TextB:
    mouse_up_lgend1(state, point, zoom, d, dc);
    break;
  default:
    mouse_up_lgend2(state, point, zoom, d, dc);
  }

  g_object_unref(G_OBJECT(dc));

  return TRUE;
}

static void
swapint(int *a, int *b)
{
  int tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

static void
create_legend1(struct Viewer *d, GdkGC *dc)
{
  int id, num, x1, y1, ret;
  N_VALUE *inst;
  struct objlist *obj = NULL;
  struct Point *po;

  d->Capture = FALSE;
  num = arraynum(d->points);

  if (d->Mode == MarkB) {
    obj = chkobject("mark");
  } else {
    obj = chkobject("text");
  }

  if (obj) {
    id = newobj(obj);
    if (id >= 0) {
      if (num >= 1) {
	po = *(struct Point **) arraynget(d->points, 0);
	x1 = po->x;
	y1 = po->y;
      }

      inst = chkobjinst(obj, id);
      _putobj(obj, "x", inst, &x1);
      _putobj(obj, "y", inst, &y1);
      PaintLock = TRUE;

      if (d->Mode == MarkB) {
	LegendMarkDialog(&DlgLegendMark, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendMark);
      } else {
	LegendTextDialog(&DlgLegendText, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendText);
      }

      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	delobj(obj, id);
      } else {
	AddList(obj, inst);
	AddInvalidateRect(obj, inst);
	set_graph_modified();
      }
      PaintLock = FALSE;
    }
  }

  ShowPoints(dc);
  arraydel2(d->points);
  d->allclear = FALSE;
}

static void
create_path(struct Viewer *d, GdkGC *dc)
{
  struct objlist *obj = NULL;
  struct narray *parray;
  struct Point *po;
  N_VALUE *inst;
  int i, num, id, ret = IDCANCEL;

  d->Capture = FALSE;
  num = arraynum(d->points);
  obj = chkobject("path");

  if (num < 3 || obj == NULL) {
    goto ExitCreatePath;
  }

  id = newobj(obj);
  if (id < 0) {
    goto ExitCreatePath;
  }

  inst = chkobjinst(obj, id);
  parray = arraynew(sizeof(int));

  for (i = 0; i < num - 1; i++) {
    po = *(struct Point **) arraynget(d->points, i);
    arrayadd(parray, &(po->x));
    arrayadd(parray, &(po->y));
  }

  _putobj(obj, "points", inst, parray);
  PaintLock = TRUE;

  LegendArrowDialog(&DlgLegendArrow, obj, id);
  ret = DialogExecute(TopLevel, &DlgLegendArrow);

  if (ret == IDDELETE || ret == IDCANCEL) {
    delobj(obj, id);
  } else {
    AddList(obj, inst);
    AddInvalidateRect(obj, inst);
    set_graph_modified();
  }

  PaintLock = FALSE;

 ExitCreatePath:
  ShowPoints(dc);
  arraydel2(d->points);

  d->allclear = FALSE;
}

static void
create_legend3(struct Viewer *d, GdkGC *dc)
{
  int id, num, x1, y1, x2, y2, ret = IDCANCEL;
  N_VALUE *inst;
  struct objlist *obj = NULL;
  struct Point **pdata;

  d->Capture = FALSE;

  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    if (d->Mode == RectB) {
      obj = chkobject("rectangle");
    } else if (d->Mode == ArcB) {
      obj = chkobject("arc");
    }

    if (obj) {
      id = newobj(obj);
      if (id >= 0) {
	inst = chkobjinst(obj, id);
	x1 = pdata[0]->x;
	y1 = pdata[0]->y;
	x2 = pdata[1]->x;
	y2 = pdata[1]->y;

	if (x1 > x2)
	  swapint(&x1, &x2);

	if (y1 > y2)
	  swapint(&y1, &y2);

	PaintLock = TRUE;

	if (d->Mode == RectB) {
	  _putobj(obj, "x1", inst, &x1);
	  _putobj(obj, "y1", inst, &y1);
	  _putobj(obj, "x2", inst, &x2);
	  _putobj(obj, "y2", inst, &y2);
	  LegendRectDialog(&DlgLegendRect, obj, id);
	  ret = DialogExecute(TopLevel, &DlgLegendRect);
	} else if (d->Mode == ArcB) {
	  int x, y, rx, ry;

	  x = (x1 + x2) / 2;
	  y = (y1 + y2) / 2;
	  rx = abs(x1 - x);
	  ry = abs(y1 - y);
	  _putobj(obj, "x", inst, &x);
	  _putobj(obj, "y", inst, &y);
	  _putobj(obj, "rx", inst, &rx);
	  _putobj(obj, "ry", inst, &ry);
	  LegendArcDialog(&DlgLegendArc, obj, id);
	  ret = DialogExecute(TopLevel, &DlgLegendArc);
	}

	if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	  delobj(obj, id);
	} else {
	  AddList(obj, inst);
	  AddInvalidateRect(obj, inst);
	  set_graph_modified();
	}
	PaintLock = FALSE;
      }
    }
  }

  ShowPoints(dc);
  arraydel2(d->points);
  d->allclear = FALSE;
}

static void
create_legendx(struct Viewer *d, GdkGC *dc)
{
  int id, num, x1, y1, x2, y2, ret = IDCANCEL, type;
  N_VALUE *inst;
  struct objlist *obj = NULL;
  struct Point **pdata;

  d->Capture = FALSE;
  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("path");

    if (obj) {
      id = newobj(obj);

      if (id >= 0) {
	type = PATH_TYPE_CURVE;
	putobj(obj, "type", id, &type);
	inst = chkobjinst(obj, id);
	x1 = pdata[0]->x;
	y1 = pdata[0]->y;
	x2 = pdata[1]->x;
	y2 = pdata[1]->y;

	if (x1 > x2)
	  swapint(&x1, &x2);

	if (y1 > y2)
	  swapint(&y1, &y2);

	PaintLock = TRUE;

	if ((x1 != x2) && (y1 != y2)) {
	  LegendGaussDialog(&DlgLegendGauss, obj, id, x1, y1,
			    x2 - x1, y2 - y1);
	  ret = DialogExecute(TopLevel, &DlgLegendGauss);

	  if (ret != IDOK) {
	    delobj(obj, id);
	  } else {
	    AddList(obj, inst);
	    AddInvalidateRect(obj, inst);
	    set_graph_modified();
	  }
	}
	PaintLock = FALSE;
      }
    }
  }
  ShowPoints(dc);
  arraydel2(d->points);
  d->allclear = FALSE;
}

static void
create_single_axis(struct Viewer *d, GdkGC *dc)
{
  int id, num, x1, y1, x2, y2, lenx, dir, ret = IDCANCEL;
  double fx1, fy1;
  N_VALUE *inst;
  struct objlist *obj = NULL;
  struct Point **pdata;

  d->Capture = FALSE;

  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("axis");
    if (obj != NULL) {
      if ((id = newobj(obj)) >= 0) {
	inst = chkobjinst(obj, id);
	x1 = pdata[0]->x;
	y1 = pdata[0]->y;
	x2 = pdata[1]->x;
	y2 = pdata[1]->y;
	fx1 = x2 - x1;
	fy1 = y2 - y1;
	lenx = nround(sqrt(fx1 * fx1 + fy1 * fy1));

	if (fx1 == 0) {
	  if (fy1 >= 0) {
	    dir = 27000;
	  } else {
	    dir = 9000;
	  }
	} else {
	  dir = nround(atan(-fy1 / fx1) / MPI * 18000);

	  if (fx1 < 0)
	    dir += 18000;

	  if (dir < 0)
	    dir += 36000;

	  if (dir >= 36000)
	    dir -= 36000;
	}

	inst = chkobjinst(obj, id);

	_putobj(obj, "x", inst, &x1);
	_putobj(obj, "y", inst, &y1);
	_putobj(obj, "length", inst, &lenx);
	_putobj(obj, "direction", inst, &dir);

	AxisDialog(&DlgAxis, obj, id, TRUE);
	ret = DialogExecute(TopLevel, &DlgAxis);

	if (ret == IDDELETE || ret == IDCANCEL) {
	  delobj(obj, id);
	} else {
	  AddList(obj, inst);
	  set_graph_modified();
	}
      }
    }
  }
  ShowPoints(dc);
  arraydel2(d->points);
  d->allclear = TRUE;
}

static void
create_axis(struct Viewer *d, GdkGC *dc)
{
  int idx, idy, idu, idr, idg, oidx, oidy, type,
    num, x1, y1, x2, y2, lenx, leny, ret = IDCANCEL;
  N_VALUE *inst;
  char *argv[2], *ref;
  struct objlist *obj = NULL, *obj2;
  struct Point **pdata;
  struct narray group;

  d->Capture = FALSE;
  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("axis");
    obj2 = chkobject("axisgrid");

    if (obj != NULL) {
      x1 = pdata[0]->x;
      y1 = pdata[0]->y;
      x2 = pdata[1]->x;
      y2 = pdata[1]->y;
      lenx = abs(x1 - x2);
      leny = abs(y1 - y2);
      x1 = (x1 < x2) ? x1 : x2;
      y1 = (y1 > y2) ? y1 : y2;
      idx = newobj(obj);
      idy = newobj(obj);

      if (d->Mode != CrossB) {
	idu = newobj(obj);
	idr = newobj(obj);
	arrayinit(&group, sizeof(int));
	if (d->Mode == FrameB) {
	  type = 1;
	} else {
	  type = 2;
	}

	arrayadd(&group, &type);
	arrayadd(&group, &idx);
	arrayadd(&group, &idy);
	arrayadd(&group, &idu);
	arrayadd(&group, &idr);
	arrayadd(&group, &x1);
	arrayadd(&group, &y1);
	arrayadd(&group, &lenx);
	arrayadd(&group, &leny);

	argv[0] = (char *) &group;
	argv[1] = NULL;
	exeobj(obj, "default_grouping", idr, 1, argv);
	arraydel(&group);

      } else {
	arrayinit(&group, sizeof(int));
	type = 3;

	arrayadd(&group, &type);
	arrayadd(&group, &idx);
	arrayadd(&group, &idy);
	arrayadd(&group, &x1);
	arrayadd(&group, &y1);
	arrayadd(&group, &lenx);
	arrayadd(&group, &leny);

	argv[0] = (char *) &group;
	argv[1] = NULL;

	exeobj(obj, "default_grouping", idx, 1, argv);
	arraydel(&group);
      }
      if ((d->Mode == SectionB) && (obj2 != NULL)) {
	idg = newobj(obj2);
	if (idg >= 0) {
	  getobj(obj, "oid", idx, 0, NULL, &oidx);

	  ref = g_malloc(ID_BUF_SIZE);
	  if (ref) {
	    snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
	    putobj(obj2, "axis_x", idg, ref);
	  }

	  getobj(obj, "oid", idy, 0, NULL, &oidy);

	  ref = g_malloc(ID_BUF_SIZE);
	  if (ref) {
	    snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidy);
	    putobj(obj2, "axis_y", idg, ref);
	  }
	}
      } else {
	idg = -1;
      }

      if (d->Mode == FrameB) {
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, &idg, FALSE);

	ret = DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == SectionB) {
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, &idg, TRUE);

	ret = DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == CrossB) {
	CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	ret = DialogExecute(TopLevel, &DlgCross);
      }

      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	if (d->Mode != CrossB) {
	  delobj(obj, idr);
	  delobj(obj, idu);
	}

	delobj(obj, idy);
	delobj(obj, idx);

	if ((idg != -1) && (obj2 != NULL)) {
	  delobj(obj2, idg);
	}
      } else {
	inst = chkobjinst(obj, idx);
	if (inst)
	  AddList(obj, inst);

	inst = chkobjinst(obj, idy);
	if (inst)
	  AddList(obj, inst);

	if (d->Mode != CrossB) {
	  inst = chkobjinst(obj, idu);
	  if (inst)
	    AddList(obj, inst);

	  inst = chkobjinst(obj, idr);
	  if (inst)
	    AddList(obj, inst);
	}
	if ((idg != -1) && (obj2 != NULL)) {

	  inst = chkobjinst(obj2, idg);
	  if (inst)
	    AddList(obj2, inst);

	}
	set_graph_modified();
      }
    }
  }
  ShowPoints(dc);
  arraydel2(d->points);
  d->allclear = TRUE;
}

static gboolean
ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;

  if (Menulock || Globallock)
    return FALSE;

  dc = gdk_gc_new(d->gdk_win);

  switch (d->Mode) {
  case PointB:
  case LegendB:
  case AxisB:
    d->Capture = FALSE;
    ViewUpdate();
    break;
  case TrimB:
  case DataB:
  case EvalB:
    break;
  case MarkB:
  case TextB:
    create_legend1(d, dc);
    break;
  case PathB:
    create_path(d, dc);
    break;
  case RectB:
  case ArcB:
    create_legend3(d, dc);
    break;
  case GaussB:
    create_legendx(d, dc);
    break;
  case SingleB:
    create_single_axis(d, dc);
    break;
  case FrameB:
  case SectionB:
  case CrossB:
    create_axis(d, dc);
    break;
  case ZoomB:
    break;
  }

  if ((d->Mode & POINT_TYPE_DRAW_ALL) && ! KeepMouseMode)
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(NgraphApp.viewb[DefaultMode]), TRUE);

  g_object_unref(G_OBJECT(dc));
  UpdateAll();

  return TRUE;
}

static void
move_data_cancel(struct Viewer *d, gboolean show_message)
{
  arraydel(&SelList);
  d->MoveData = FALSE;
  d->Capture = FALSE;
  NSetCursor(GDK_LEFT_PTR);

  if (show_message)
    message_box(NULL, _("Moving data points is canceled."), "Confirm", RESPONS_OK);
}

static gboolean
ViewerEvRButtonDown(unsigned int state, TPoint *point, struct Viewer *d, GdkEventButton *e)
{
  GdkGC *dc;
  int num;
  struct Point *po;
  double zoom;

  if (Menulock || Globallock)
    return FALSE;

  if (d->MoveData) {
    move_data_cancel(d, TRUE);
    return TRUE;
  }

  if (d->Capture) {
    zoom = Menulocal.PaperZoom / 10000.0;
    dc = gdk_gc_new(d->gdk_win);
    switch (d->Mode) {
    case PathB:
      num = arraynum(d->points);
      if (num > 0) {
	ShowPoints(dc);
	arrayndel2(d->points, num - 1);
	if (num <= 2) {
	  arraydel2(d->points);
	  d->Capture = FALSE;
	} else {
	  po = *(struct Point **) arraylast(d->points);
	  if (po != NULL) {
	    d->MouseX1 = (mxp2d(d->hscroll + point->x - d->cx)
			  - Menulocal.LeftMargin) / zoom;
	    d->MouseY1 = (mxp2d(d->vscroll + point->y - d->cy)
			  - Menulocal.TopMargin) / zoom;
	    po->x = d->MouseX1;
	    po->y = d->MouseY1;
	    CheckGrid(TRUE, state, &(po->x), &(po->y), NULL);
	  }
	  ShowPoints(dc);
	}
	break;
      case RectB:
      case ArcB:
      case GaussB:
      case SingleB:
      case FrameB:
      case SectionB:
      case CrossB:
	ShowPoints(dc);
	arraydel2(d->points);
	d->Capture = FALSE;
	break;
      default:
	break;
      }
    }
    g_object_unref(G_OBJECT(dc));
    return TRUE;
  }

  if (d->Mode == ZoomB) {
    mouse_down_zoom(state, point, d, ! (state & GDK_CONTROL_MASK));
  } else if (d->MouseMode == MOUSENONE) {
    do_popup(e, d);
  }

  return TRUE;
}

static gboolean
ViewerEvMButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  if (Menulock || Globallock)
    return FALSE;

  if (d->Mode == ZoomB) {
    d->hscroll -= (d->cx - point->x);
    d->vscroll -= (d->cy - point->y);
    ChangeDPI(TRUE);
  } else {
    ViewerEvLButtonDown(state, point, d);
    ViewerEvLButtonUp(state, point, d);
    ViewerEvLButtonDblClk(state, point, d);
  }

  return FALSE;
}

static int
get_mouse_cursor_type(struct Viewer *d, int x, int y)
{
  int j, x1, y1, x2, y2, num, cursor, bboxnum, *bbox;
  N_VALUE *inst;
  struct narray *abbox;
  struct FocusObj **focus;
  double zoom;

  if (d->MoveData)
    return GDK_TCROSS;

  num = arraynum(d->focusobj);
  if (num == 0)
    return GDK_LEFT_PTR;

  GetFocusFrame(&x1, &y1, &x2, &y2, d->FrameOfsX, d->FrameOfsY);

  if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
    cursor = GDK_FLEUR;
  } else if (x > x1 - FOCUS_RECT_SIZE - FOCUS_FRAME_OFST &&
	     x < x1 - FOCUS_FRAME_OFST &&
	     y > y1 - FOCUS_RECT_SIZE - FOCUS_FRAME_OFST &&
	     y < y1 - FOCUS_FRAME_OFST) {
    cursor = GDK_TOP_LEFT_CORNER;
  } else if (x > x1 - FOCUS_RECT_SIZE - FOCUS_FRAME_OFST &&
	     x < x1 - FOCUS_FRAME_OFST &&
	     y < y2 + FOCUS_RECT_SIZE + FOCUS_FRAME_OFST - 1 &&
	     y > y2 + FOCUS_FRAME_OFST - 1) {
    cursor = GDK_BOTTOM_LEFT_CORNER;
  } else if (x < x2 + FOCUS_RECT_SIZE + FOCUS_FRAME_OFST - 1 &&
	     x > x2 + FOCUS_FRAME_OFST - 1 &&
	     y > y1 - FOCUS_RECT_SIZE - FOCUS_FRAME_OFST &&
	     y < y1 - FOCUS_FRAME_OFST) {
    cursor = GDK_TOP_RIGHT_CORNER;
  } else if (x < x2 + FOCUS_RECT_SIZE + FOCUS_FRAME_OFST - 1 &&
	     x > x2 + FOCUS_FRAME_OFST - 1 &&
	     y < y2 + FOCUS_RECT_SIZE + FOCUS_FRAME_OFST - 1 &&
	     y > y2 + FOCUS_FRAME_OFST - 1) {
    cursor = GDK_BOTTOM_RIGHT_CORNER;
  } else {
    cursor = GDK_LEFT_PTR;
  }

  if (num > 1)
    return cursor;

  focus = arraydata(d->focusobj);
  inst = chkobjinstoid(focus[0]->obj, focus[0]->oid);
  if (inst == NULL)
    return cursor;

  zoom = Menulocal.PaperZoom / 10000.0;

  _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
  _getobj(focus[0]->obj, "bbox", inst, &abbox);

  bboxnum = arraynum(abbox);
  bbox = arraydata(abbox);

  for (j = 4; j < bboxnum; j += 2) {
    x1 = coord_conv_x((bbox[j] + d->FrameOfsX), zoom, d);
    y1 = coord_conv_y((bbox[j + 1] + d->FrameOfsY), zoom, d);

    if (x > x1 - FOCUS_RECT_SIZE / 2 &&
	x < x1 + FOCUS_RECT_SIZE / 2 &&
	y > y1 - FOCUS_RECT_SIZE / 2 &&
	y < y1 + FOCUS_RECT_SIZE / 2) {
      cursor = GDK_CROSSHAIR;
      d->ChangePoint = (j - 4) / 2;
      break;
    }
  }

  return cursor;
}

static void
set_mouse_cursor_hover(struct Viewer *d, int x, int y)
{
  if (d->Mode != PointB && d->Mode != LegendB && d->Mode != AxisB)
    return;

  NSetCursor(get_mouse_cursor_type(d, x, y));
}

static void
update_frame_rect(TPoint *point, struct Viewer *d, GdkGC *dc, double zoom)
{
  ShowFrameRect(dc);
  d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;
  ShowFrameRect(dc);
}

#define SQRT3 1.73205080756888

static void
calc_snap_angle(struct narray *points, int *dx, int *dy)
{
  struct Point *po2;
  int x, y, w, h, n;
  double angle, l;

  x = *dx;
  y = *dy;

  n = arraynum(points);
  if (n < 2)
    return;

  po2 = *(struct Point **) arraynget(points, n - 2);

  w = x - po2->x;
  h = y - po2->y;

  if (h == 0 || w == 0)
    return;

  l = sqrt(w * w + h * h);

  if (w / h) {
    angle = acos(w / l);
    if (h < 0)
      angle = -angle;
  } else {
    angle = asin(h / l);
    if (w < 0)
      angle = -angle;
  }

  if (angle < 0)
    angle += 2 * MPI;

  angle *= 180 / MPI;

  if (angle < 15) {
    y = po2->y;
  } else if (angle < 37) {  /* 30 */
    y = po2->y + w / SQRT3;
  } else if (angle < 52) {  /* 45 */
    y = po2->y + w;
  } else if (angle < 75) {  /* 60 */
    x = po2->x + h / SQRT3;
  } else if (angle < 105) { /* 90 */
    x = po2->x;
  } else if (angle < 127) { /* 120 */
    x = po2->x - h / SQRT3;
  } else if (angle < 142) { /* 135 */
    y = po2->y - w;
  } else if (angle < 165) { /* 150 */
    y = po2->y - w / SQRT3;
  } else if (angle < 195) { /* 180 */
    y = po2->y;
  } else if (angle < 217) { /* 210 */
    y = po2->y + w / SQRT3;
  } else if (angle < 232) { /* 225 */
    y = po2->y + w;
  } else if (angle < 255) { /* 240 */
    x = po2->x + h / SQRT3;
  } else if (angle < 285) { /* 270 */
    x = po2->x;
  } else if (angle < 307) { /* 300 */
    x = po2->x - h / SQRT3;
  } else if (angle < 322) { /* 315 */
    y = po2->y - w;
  } else if (angle < 345) { /* 330 */
    y = po2->y - w / SQRT3;
  } else {                  /* 360 */
    y = po2->y;
  }

  *dx = x;
  *dy = y;
}

static void
calc_integer_ratio(struct narray *points, int *dx, int *dy)
{
  struct Point *po2;
  int x, y, w, h;

  x = *dx;
  y = *dy;

  po2 = *(struct Point **) arraynget(points, 0);

  if (pow == NULL)
    return;

  w = abs(x - po2->x);
  h = abs(y - po2->y);
  if (w < h) {
    w *= (w) ? h / w: 0;
    if (y > po2->y) {
      y = po2->y + w;
    } else {
      y = po2->y - w;
    }
  } else {
    h *= (h) ? w / h: 0;
    if (x > po2->x) {
      x = po2->x + h;
    } else {
      x = po2->x - h;
    }
  }

  *dx = x;
  *dy = y;
}

static void
mouse_move_drag(GdkGC *dc, unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x, y;

  ShowFocusFrame(dc);
  d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;

  x = d->MouseX2 - d->MouseX1;
  y = d->MouseY2 - d->MouseY1;

  CheckGrid(FALSE, state, &x, &y, NULL);

  d->FrameOfsX = x;
  d->FrameOfsY = y;

  ShowFocusFrame(dc);
}

static void
mouse_move_zoom(GdkGC *dc, unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  double cc, nn, zoom2;
  int vx1, vx2, vy1, vy2;

  ShowFrameRect(dc);

  vx1 = (mxp2d(point->x - d->cx + d->hscroll)
	 - Menulocal.LeftMargin) / zoom;

  vy1 = (mxp2d(point->y - d->cy + d->vscroll)
	 - Menulocal.TopMargin) / zoom;

  vx1 -= d->RefX1 - d->MouseDX;
  vy1 -= d->RefY1 - d->MouseDY;

  vx2 = (d->RefX2 - d->RefX1);
  vy2 = (d->RefY2 - d->RefY1);

  cc = vx1 * vx2 + vy1 * vy2;
  nn = vx2 * vx2 + vy2 * vy2;

  if ((nn == 0) || (cc < 0)) {
    zoom2 = 0;
  } else {
    zoom2 = cc / nn;
  }

  if ((d->Mode != DataB) && (d->Mode != EvalB))
    CheckGrid(FALSE, state, NULL, NULL, &zoom2);

  d->Zoom = zoom2;

  vx1 = d->RefX1 + vx2 * zoom2;
  vy1 = d->RefY1 + vy2 * zoom2;

  d->MouseX1 = d->RefX1;
  d->MouseY1 = d->RefY1;

  d->MouseX2 = vx1;
  d->MouseY2 = vy1;

  ShowFrameRect(dc);
}

static void
mouse_move_change(GdkGC *dc, unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x, y;

  ShowFocusLine(dc, TRUE, state, d->ChangePoint);

  d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		- Menulocal.LeftMargin) / zoom;

  d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		- Menulocal.TopMargin) / zoom;

  x = d->MouseX2 - d->MouseX1;
  y = d->MouseY2 - d->MouseY1;

  if ((d->Mode != DataB) && (d->Mode != EvalB)) {
    CheckGrid(FALSE, state, &x, &y, NULL);
  }

  d->LineX = x;
  d->LineY = y;

  ShowFocusLine(dc, FALSE, state, d->ChangePoint);
}

static void
mouse_move_scroll(TPoint *point, struct Viewer *d)
{
  int h, w;

  gdk_window_get_geometry(d->gdk_win, NULL, NULL, &w, &h, NULL);
  if (point->y > h) {
    range_increment(d->VScroll, SCROLL_INC);
  } else if (point->y < 0) {
    range_increment(d->VScroll, -SCROLL_INC);
  }

  if (point->x > w) {
    range_increment(d->HScroll, SCROLL_INC);
  } else if (point->x < 0) {
    range_increment(d->HScroll, -SCROLL_INC);
  }
}

static void
mouse_move_draw(GdkGC *dc, unsigned int state, TPoint *point, int *dx, int *dy, struct Viewer *d)
{
  ShowPoints(dc);

  if (arraynum(d->points) != 0) {
    struct Point *po;
    po = *(struct Point **) arraylast(d->points);

    if (state & GDK_CONTROL_MASK) {
      if (d->Mode & POINT_TYPE_DRAW1) {
	calc_integer_ratio(d->points, dx, dy);
      } else if (d->Mode & POINT_TYPE_DRAW2) {
	calc_snap_angle(d->points, dx, dy);
	if (! (state & GDK_SHIFT_MASK)) {
	  CheckGrid(FALSE, 0, dx, dy, NULL);
	}
      }
    }

    if (po != NULL) {
      po->x = *dx;
      po->y = *dy;
    }
  }
  ShowPoints(dc);
}

static gboolean
ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;
  int dx, dy;
  double zoom;

  dc = Menulocal.gc;

  if (Menulock || Globallock || ! d->gdk_win || dc == NULL)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  dx = (mxp2d(point->x + d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom;
  dy = (mxp2d(point->y + d->vscroll - d->cy) - Menulocal.TopMargin) / zoom;

  if ((d->Mode != DataB) &&
      (d->Mode != EvalB) &&
      (d->Mode != ZoomB) &&
      (d->MouseMode != MOUSEPOINT) &&
      (((d->Mode != PointB) && (d->Mode != LegendB) && (d->Mode != AxisB)) ||
       (d->MouseMode != MOUSENONE))) {
    CheckGrid(TRUE, state, &dx, &dy, NULL);
  }

  if (region == NULL && Menulocal.show_cross) {
    ShowCrossGauge(dc);

    d->CrossX = dx;
    d->CrossY = dy;

    ShowCrossGauge(dc);
  }

  mouse_move_scroll(point, d);

  if (! d->Capture) {
    set_mouse_cursor_hover(d, point->x, point->y);
  } else {
    int pos;

    pos = NGetCursor();
    if (pos == GDK_FLEUR ||
	pos == GDK_TOP_LEFT_CORNER ||
	pos == GDK_BOTTOM_LEFT_CORNER ||
	pos == GDK_TOP_RIGHT_CORNER ||
	pos == GDK_BOTTOM_RIGHT_CORNER ||
	pos == GDK_CROSSHAIR ||
	(d->Mode & POINT_TYPE_TRIM)) {

      switch (d->MouseMode) {
      case MOUSEDRAG:
	mouse_move_drag(dc, state, point, zoom, d);
	break;
      case MOUSEZOOM1:
      case MOUSEZOOM2:
      case MOUSEZOOM3:
      case MOUSEZOOM4:
	mouse_move_zoom(dc, state, point, zoom, d);
	break;
      case MOUSECHANGE:
	mouse_move_change(dc, state, point, zoom, d);
	break;
      case MOUSEPOINT:
	update_frame_rect(point, d, dc, zoom);
	break;
      case MOUSENONE:
	break;
      }
    } else if (d->Mode & POINT_TYPE_POINT) {
      if (d->MouseMode == MOUSEPOINT) {
	update_frame_rect(point, d, dc, zoom);
      }
    } else {
      mouse_move_draw(dc, state, point, &dx, &dy, d);
    }
  }

  SetPoint(d, dx, dy);

  return FALSE;
}

static gboolean
ViewerEvMouseMotion(GtkWidget *w, GdkEventMotion *e, gpointer client_data)
{
  TPoint point;
#if 0
  static guint32 etime = 0;

  if (e->time - etime < 100)
    return FALSE;

  etime = e->time;
#endif

  point.x = e->x;
  point.y = e->y;
  ViewerEvMouseMove(e->state, &point, (struct Viewer *) client_data);
  gdk_event_request_motions(e); /* handles is_hint events */

  return FALSE;
}

static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct Viewer *d;
  int cx, cy;

  d = (struct Viewer *) user_data;

  gdk_window_get_geometry(d->gdk_win, &cx, &cy, NULL, NULL, NULL);
  gtk_window_get_position(GTK_WINDOW(TopLevel), x, y);

  *x += cx;
  *y += cy;
}

int
check_focused_obj_type(struct Viewer *d, int *type)
{
  int num, i, t;
  static struct objlist *axis, *merge, *legend, *text;
  struct FocusObj *focus;  

  num = arraynum(d->focusobj);

  if (axis == NULL)
    axis = chkobject("axis");

  if (merge == NULL)
    merge = chkobject("merge");

  if (legend == NULL)
    legend = chkobject("legend");

  if (text == NULL)
    text = chkobject("text");

  t = 0;
  for (i = 0; i < num; i++) {
    focus = *(struct FocusObj **) arraynget(d->focusobj, i);
    if (chkobjchild(legend, focus->obj)) {
      t |= FOCUS_OBJ_TYPE_LEGEND;
      if (chkobjchild(text, focus->obj)) {
	t |= FOCUS_OBJ_TYPE_TEXT;
      }
    } else if (chkobjchild(axis, focus->obj)) {
      t |= FOCUS_OBJ_TYPE_AXIS;
    } else if (chkobjchild(merge, focus->obj)) {
      t |= FOCUS_OBJ_TYPE_MERGE;
    }
  }

  if (type)
    *type = t;

  return num;
}

static void
do_popup(GdkEventButton *event, struct Viewer *d)
{
  int button, event_time, i, num, type;
  GtkMenuPositionFunc func = NULL;
  GtkClipboard *clip;
  gboolean state;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
    func = popup_menu_position;
  }

  for (i = 0; i < VIEWER_POPUP_ITEM_NUM; i++) {
    gtk_widget_set_sensitive(d->popup_item[i], FALSE);
  }

  gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_CROSS], TRUE);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(d->popup_item[VIEWER_POPUP_ITEM_CROSS]), Menulocal.show_cross);

  num = check_focused_obj_type(d, &type);

  switch (d->Mode) {
  case PointB:
  case LegendB:
    clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    state = gtk_clipboard_wait_is_text_available(clip);
    gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_PASTE], state);
  case AxisB:
    if (num > 0) {
      if (! (type & FOCUS_OBJ_TYPE_AXIS) &&
	  (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_MERGE))) {
	gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_CUT], TRUE);
	gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_COPY], TRUE);
      }
      gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_DUP], TRUE);
      gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_DEL], TRUE);
      gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_PROP], TRUE);
      gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_ALIGN], TRUE);
      if (! (type & FOCUS_OBJ_TYPE_MERGE) && (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_AXIS))) {
	gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_ROTATE], TRUE);
      }
      if (! (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_TEXT)) && (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_AXIS))) {
	gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_FLIP], TRUE);
      }
    }
    if (num == 1) {
      struct FocusObj *focus;
      int id, last_id;

      focus = * (struct FocusObj **) arraynget(d->focusobj, 0);
      if (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_MERGE)) {
	id = chkobjoid(focus->obj, focus->oid);
	last_id = chkobjlastinst(focus->obj);

	if (id > 0) {
	  gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_TOP], TRUE);
	  gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_UP], TRUE);
	}
	if (id < last_id) {
	  gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_DOWN], TRUE);
	  gtk_widget_set_sensitive(d->popup_item[VIEWER_POPUP_ITEM_BOTTOM], TRUE);
	}
      }
    }
  default:
    break;
  }
  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d, button, event_time);
}


static gboolean
ViewerEvPopupMenu(GtkWidget *w, gpointer client_data)
{
  do_popup(NULL, client_data);
  return TRUE;
}

guint32 ViewerTime = 0;
TPoint ViewerPoint;

static gboolean
ViewerEvScroll(GtkWidget *w, GdkEventScroll *e, gpointer client_data)
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

  switch (e->direction) {
  case GDK_SCROLL_UP:
    range_increment(d->VScroll, -SCROLL_INC);
    return TRUE;
  case GDK_SCROLL_DOWN:
     range_increment(d->VScroll, SCROLL_INC);
    return TRUE;
  case GDK_SCROLL_LEFT:
    range_increment(d->HScroll, -SCROLL_INC);
    return TRUE;
  case GDK_SCROLL_RIGHT:
    range_increment(d->HScroll, SCROLL_INC);
    return TRUE;
  }
  return FALSE;
}

static gboolean
ViewerEvButtonDown(GtkWidget *w, GdkEventButton *e, gpointer client_data)
{
  struct Viewer *d;
  TPoint point;

  d = (struct Viewer *) client_data;

  point.x = e->x;
  point.y = e->y;

  ViewerTime = e->time;
  ViewerPoint.x = point.x;
  ViewerPoint.y = point.y;

  gtk_widget_grab_focus(w);

  switch (e->button) {
  case Button1:
    if (e->type == GDK_BUTTON_PRESS) {
      return ViewerEvLButtonDown(e->state, &point, d);
    } else {
      return ViewerEvLButtonDblClk(e->state, &point, d);
    }
    break;
  case Button2:
    return ViewerEvMButtonDown(e->state, &point, d);
  case Button3:
    return ViewerEvRButtonDown(e->state, &point, d, e);
  }

  return FALSE;
}

static gboolean
ViewerEvButtonUp(GtkWidget *w, GdkEventButton *e, gpointer client_data)
{
  struct Viewer *d;
  TPoint point;

  d = (struct Viewer *) client_data;

  point.x = e->x;
  point.y = e->y;

  switch (e->button) {
  case Button1:
    return ViewerEvLButtonUp(e->state, &point, d);
  }

  return FALSE;
}

static gboolean
ViewerEvKeyDown(GtkWidget *w, GdkEventKey *e, gpointer client_data)
{
  struct Viewer *d;
  GdkGC *dc;
  int dx = 0, dy = 0, mv, n;
  double zoom;

  if (Menulock || Globallock)
    return FALSE;

  d = (struct Viewer *) client_data;

  switch (e->keyval) {
  case GDK_Escape:
    if (d->MoveData) {
      move_data_cancel(d, TRUE);
    } else {
      UnFocus();
    }
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(NgraphApp.viewb[DefaultMode]), TRUE);
    return FALSE;
  case GDK_space:
    CmViewerDrawB(NULL, NULL);
    return TRUE;
  case GDK_Delete:
    ViewDelete();
    return TRUE;
  case GDK_Return:
    ViewUpdate();
    return TRUE;
  case GDK_Insert:
    ViewCopy();
    return TRUE;
  case GDK_Home:
    if (e->state & GDK_SHIFT_MASK) {
      reorder_object(OBJECT_MOVE_TYPE_TOP);
      return TRUE;
    }
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK) {
      reorder_object(OBJECT_MOVE_TYPE_LAST);
      return TRUE;
    }
    break;
  case GDK_Page_Up:
    range_increment(d->VScroll, -SCROLL_INC * 4);
    return TRUE;
  case GDK_Page_Down:
    range_increment(d->VScroll, SCROLL_INC * 4);
    return TRUE;
  case GDK_Down:
  case GDK_Up:
  case GDK_Left:
  case GDK_Right:
    n = arraynum(d->focusobj);

    if (n == 0) {
      switch (e->keyval) {
      case GDK_Up:
	range_increment(d->VScroll, -SCROLL_INC);
	return TRUE;
      case GDK_Down:
	range_increment(d->VScroll, SCROLL_INC);
	return TRUE;
      case GDK_Left:
	range_increment(d->HScroll, -SCROLL_INC);
	return TRUE;
      case GDK_Right:
	range_increment(d->HScroll, SCROLL_INC);
	return TRUE;
      }
      return FALSE;
    }

    if (((d->MouseMode == MOUSENONE) || (d->MouseMode == MOUSEDRAG))
	&& (d->Mode & POINT_TYPE_POINT)) {
      dc = gdk_gc_new(d->gdk_win);
      zoom = Menulocal.PaperZoom / 10000.0;
      ShowFocusFrame(dc);
      if (e->state & GDK_SHIFT_MASK) {
	mv = Menulocal.grid / 10;
      } else {
	mv = Menulocal.grid;
      }

      if (e->keyval == GDK_Down) {
	dx = 0;
	dy = mv;
      } else if (e->keyval == GDK_Up) {
	dx = 0;
	dy = -mv;
      } else if (e->keyval == GDK_Right) {
	dx = mv;
	dy = 0;
      } else if (e->keyval == GDK_Left) {
	dx = -mv;
	dy = 0;
      }

      d->FrameOfsX += dx / zoom;
      d->FrameOfsY += dy / zoom;

      ShowFocusFrame(dc);

      g_object_unref(G_OBJECT(dc));
      d->MouseMode = MOUSEDRAG;
      return TRUE;
    }
    break;
  case GDK_Shift_L:
  case GDK_Shift_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_PLUS);
      return TRUE;
    }
    break;
  case GDK_Control_L:
  case GDK_Control_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  default:
    break;
  }
  return FALSE;
}

static gboolean
ViewerEvKeyUp(GtkWidget *w, GdkEventKey *e, gpointer client_data)
{
  struct Viewer *d;
  GdkGC *dc;
  int num, i, dx, dy;
  struct FocusObj *focus;
  N_VALUE *inst;
  struct objlist *obj;
  char *argv[4];
  int axis = FALSE;

  if (Menulock || Globallock)
    return FALSE;

  d = (struct Viewer *) client_data;

  switch (e->keyval) {
  case GDK_Shift_L:
  case GDK_Shift_R:
  case GDK_Control_L:
  case GDK_Control_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  case GDK_Down:
  case GDK_Up:
  case GDK_Left:
  case GDK_Right:
    if (d->MouseMode != MOUSEDRAG)
      break;

    dc = gdk_gc_new(d->gdk_win);
    ShowFocusFrame(dc);
    dx = d->FrameOfsX;
    dy = d->FrameOfsY;
    argv[0] = (char *) &dx;
    argv[1] = (char *) &dy;
    argv[2] = NULL;
    num = arraynum(d->focusobj);
    axis = FALSE;
    PaintLock = TRUE;
    for (i = num - 1; i >= 0; i--) {
      focus = *(struct FocusObj **) arraynget(d->focusobj, i);
      obj = focus->obj;
      if (obj == chkobject("axis"))
	axis = TRUE;
      if ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL) {
	AddInvalidateRect(obj, inst);
	_exeobj(obj, "move", inst, 2, argv);
	set_graph_modified();
	AddInvalidateRect(obj, inst);
      }
    }
    PaintLock = FALSE;
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    ShowFocusFrame(dc);
    g_object_unref(G_OBJECT(dc));
    if (! axis)
      d->allclear = FALSE;
    UpdateAll();
    d->MouseMode = MOUSENONE;
    return TRUE;
  default:
    break;
  }

  return FALSE;
}

static void
ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data)
{
  struct Viewer *d;
  int x, y;
  int width, height;

  d = (struct Viewer *) client_data;

  x = allocation->x;
  y = allocation->y;
  width =allocation->width;
  height = allocation->height;
    
  d->cx = width / 2;
  d->cy = height / 2;
  ChangeDPI(TRUE);
  d->width = width;
  d->height = height;
}

static gboolean
ViewerEvPaint(GtkWidget *w, GdkEventExpose *e, gpointer client_data)
{
  GdkGC *gc;
  struct Viewer *d;
  GdkRectangle rect;

  d = (struct Viewer *) client_data;
  gc = Menulocal.gc;

  if ((e && e->count != 0) || gc == NULL)
    return TRUE;


  Menulocal.scrollx = d->hscroll - d->cx;
  Menulocal.scrolly = d->vscroll - d->cy;

  if (Menulocal.pix) {
    gdk_region_get_clipbox(e->region, &rect);
    gdk_draw_drawable(e->window, gc, Menulocal.pix,
		      d->hscroll - d->cx + rect.x,
		      d->vscroll - d->cy + rect.y,
		      rect.x, rect.y,
		      rect.width, rect.height);
  }

  if (! Globallock) {
    /* I think it does not need to check chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid). */
    if (d->ShowFrame)
      ShowFocusFrame(gc);

    ShowPoints(gc);

    if (d->ShowLine)
      ShowFocusLine(gc, FALSE, 0, d->ChangePoint);

    if (d->ShowRect)
      ShowFrameRect(gc);

    if (Menulocal.show_cross)
      ShowCrossGauge(gc);
  }

  if (! PaintLock && region) {
    gdk_region_destroy(region);
    region = NULL;
  }
  return FALSE;
}

static void
ViewerEvVScroll(GtkRange *range, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  d->vscroll = gtk_range_get_value(range);
  SetVRuler(d);
  gdk_window_invalidate_rect(d->gdk_win, NULL, TRUE);
}

static void
ViewerEvHScroll(GtkRange *range, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  d->hscroll = gtk_range_get_value(range);
  SetHRuler(d);
  gdk_window_invalidate_rect(d->gdk_win, NULL, TRUE);
}

void
ViewerWinUpdate(int clear)
{
  int i, num;
  struct FocusObj **focus;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) == NULL) {
    CloseGC();
    CloseGRA();
    OpenGRA();
    OpenGC();
    SetScroller();
    ChangeDPI(TRUE);
  }
  CheckPage();
  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);
  for (i = num - 1; i >= 0; i--) {
    if (chkobjoid(focus[i]->obj, focus[i]->oid) == -1)
      arrayndel2(d->focusobj, i);
  }

  if (arraynum(d->focusobj) == 0)
    clear_focus_obj(d->focusobj);

  if (d->allclear) {
    mx_clear(NULL);
    mx_redraw(Menulocal.obj, Menulocal.inst);
  } else if (region) {
    Menulocal.region = region;
    gra2cairo_clip_region(Menulocal.local, region);
    mx_redraw(Menulocal.obj, Menulocal.inst);
    gra2cairo_clip_region(Menulocal.local, NULL);
    Menulocal.region = NULL;
  }
  gdk_window_invalidate_rect(d->gdk_win, NULL, TRUE);

  d->allclear = TRUE;
}

static void
SetHRuler(struct Viewer *d)
{
  double x1, x2, zoom;
  int width;

  gdk_drawable_get_size(d->gdk_win, &width, NULL);
  zoom = Menulocal.PaperZoom / 10000.0;
  x1 = N2GTK_RULER_METRIC(mxp2d(d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom;
  x2 = x1 + N2GTK_RULER_METRIC(mxp2d(width)) / zoom;

  gtk_ruler_set_range(GTK_RULER(d->HRuler), x1, x2, 0, x2);
}

static void
SetVRuler(struct Viewer *d)
{
  double y1, y2, zoom;
  int height;

  gdk_drawable_get_size(d->gdk_win, NULL, &height);
  zoom = Menulocal.PaperZoom / 10000.0;
  y1 = N2GTK_RULER_METRIC(mxp2d(d->vscroll - d->cy) - Menulocal.TopMargin) / zoom;
  y2 = y1 + N2GTK_RULER_METRIC(mxp2d(height)) / zoom;

  gtk_ruler_set_range(GTK_RULER(d->VRuler), y1, y2, 0, y2);
}

#define CHECK_FOCUSED_OBJ_ERROR -1
#define CHECK_FOCUSED_OBJ_NOT_FOUND -2

static int
check_focused_obj(struct narray *focusobj, struct objlist *fobj, int oid)
{
  int i, num;
  struct FocusObj *focus;

  if (fobj == NULL)
    return CHECK_FOCUSED_OBJ_ERROR;

  num = arraynum(focusobj);
  for (i = 0; i < num; i++) {
    focus = *(struct FocusObj **) arraynget(focusobj, i);
    if (focus == NULL)
      continue;

    if (fobj == focus->obj && oid == focus->oid) {
      return i;
    }
  }
  return CHECK_FOCUSED_OBJ_NOT_FOUND;
}

static int
add_focus_obj(struct narray *focusobj, struct objlist *obj, int oid)
{
  struct FocusObj *focus;
  int r;

  if (chkobjfield(obj, "bbox")) 
    return FALSE;

  r = check_focused_obj(focusobj, obj, oid);
  if (r != CHECK_FOCUSED_OBJ_NOT_FOUND)
    return FALSE;

  focus = (struct FocusObj *) g_malloc(sizeof(struct FocusObj));
  if (! focus)
    return FALSE;

  focus->obj = obj;
  focus->oid = oid;
  arrayadd(focusobj, &focus);

  return TRUE;
}

static void
clear_focus_obj(struct narray *focusobj)
{
  arraydel2(focusobj);
}

void
Focus(struct objlist *fobj, int id, int add)
{
  int oid, focus;
  N_VALUE *inst;
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did;
  char *dfield;
  int i;
  struct savedstdio save;
  GdkGC *dc;
  int man;
  struct Viewer *d;

  if (fobj == NULL)
    return;

  d = &(NgraphApp.Viewer);

  if (! chkobjchild(chkobject("legend"), fobj) &&
      ! chkobjchild(chkobject("axis"), fobj) &&
      ! chkobjchild(chkobject("merge"), fobj)) {
    return;
  }

  if (! add)
    UnFocus();

  inst = chkobjinst(fobj, id);

  _getobj(fobj, "oid", inst, &oid);


  if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
    return;

  snum = arraynum(sarray);
  if (snum == 0)
    return;

  ignorestdio(&save);

  sdata = arraydata(sarray);

  for (i = 1; i < snum; i++) {
    dobj = getobjlist(sdata[i], &did, &dfield, NULL);
    if (! dobj || (fobj != dobj) || (did != oid)) {
      continue;
    }

    if (chkobjchild(chkobject("axis"), dobj)) {
      getobj(fobj, "group_manager", id, 0, NULL, &man);
      if (man < 0)
	continue;

      getobj(fobj, "oid", man, 0, NULL, &did);
    }

    focus = check_focused_obj(d->focusobj, fobj, did);
    if (focus >= 0) {
      arrayndel2(d->focusobj, focus);
      break;
    }

    add_focus_obj(d->focusobj, dobj, did);

    d->MouseMode = MOUSENONE;
    break;
  }

  if (arraynum(d->focusobj) == 0) {
    UnFocus();
  }

  dc = gdk_gc_new(d->gdk_win);
  d->allclear = FALSE;
  UpdateAll();
  ShowFocusFrame(dc);
  d->ShowFrame = TRUE;
  g_object_unref(G_OBJECT(dc));
  gtk_widget_grab_focus(d->Win);

  restorestdio(&save);
}

void
UnFocus(void)
{
  GdkGC *gc;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if (arraynum(d->focusobj) != 0) {
    gc = gdk_gc_new(d->gdk_win);
    ShowFocusFrame(gc);
    g_object_unref(G_OBJECT(gc));
    clear_focus_obj(d->focusobj);
  }

  d->ShowFrame = FALSE;
  if (arraynum(d->points) != 0) {
    gc = gdk_gc_new(d->gdk_win);
    ShowPoints(gc);
    g_object_unref(G_OBJECT(gc));
    arraydel2(d->points);
  }
}

static void
create_pix(int w, int h)
{
  GdkRectangle rect;

  if (Menulocal.gc == NULL) {
    return;
  }


  if (w == 0) {
    w = 1;
  }

  if (h == 0) {
    h = 1;
  }

  if (Menulocal.local->cairo) {
    cairo_destroy(Menulocal.local->cairo);
  }

  Menulocal.local->cairo = NULL;

  if (Menulocal.pix){
    g_object_unref(Menulocal.pix);
  }

  Menulocal.pix = gdk_pixmap_new(NgraphApp.Viewer.Win->window, w, h, -1);
  rect.x = 0;
  rect.y = 0;
  rect.width = w;
  rect.height = h;

  gdk_gc_set_clip_rectangle(Menulocal.gc, &rect);
  gdk_gc_set_rgb_fg_color(Menulocal.gc, &white);
  gdk_draw_rectangle(Menulocal.pix, Menulocal.gc, TRUE, 0, 0, w, h);
  draw_paper_frame();
  gdk_gc_set_clip_rectangle(Menulocal.gc, NULL);

  Menulocal.local->cairo = gdk_cairo_create(Menulocal.pix);
  gra2cairo_set_antialias(Menulocal.local, Menulocal.local->antialias);
#if 0
  cairo_set_tolerance(Menulocal.local->cairo, 0.1);
#endif
  Menulocal.local->offsetx = 0;
  Menulocal.local->offsety = 0;
}

void
OpenGC(void)
{
  int width, height;

  if (Menulocal.gc)
    return;

  Menulocal.local->pixel_dot_x =
  Menulocal.local->pixel_dot_y =
    Menulocal.windpi / 25.4 / 100;
  Menulocal.local->offsetx = 0;
  Menulocal.local->offsety = 0;

  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);

  if (width == 0)
    width = 1;

  if (height == 0)
    height = 1;

  Menulocal.win = NgraphApp.Viewer.Win->window;
  Menulocal.gc = gdk_gc_new(Menulocal.win);
  create_pix(width, height);

  Menulocal.offsetx = 0;
  Menulocal.offsety = 0;

  Menulocal.scrollx = 0;
  Menulocal.scrolly = 0;
  Menulocal.region = NULL;
}

void
SetScroller(void)
{
  int width, height, x, y;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);
  x = width / 2;
  y = height / 2;

  gtk_range_set_range(GTK_RANGE(d->HScroll), 0, width);
  gtk_range_set_value(GTK_RANGE(d->HScroll), x);
  gtk_range_set_increments(GTK_RANGE(d->HScroll), 10, 40);

  gtk_range_set_range(GTK_RANGE(d->VScroll), 0, height);
  gtk_range_set_value(GTK_RANGE(d->VScroll), y);
  gtk_range_set_increments(GTK_RANGE(d->VScroll), 10, 40);

  d->hupper = width;
  d->vupper = height;

  d->hscroll = x;
  d->vscroll = y;
}

void
ChangeDPI(int redraw)
{
  int width, height, i, num, XPos, YPos, XRange = 0, YRange = 0;
  gint w, h;
  N_VALUE *inst;
  double ratex, ratey;
  struct objlist *obj;
  struct narray *array;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  XRange = d->hupper;
  YRange = d->vupper;

  XPos = d->hscroll;
  YPos = d->vscroll;

  if (XPos < 0)
    XPos = 0;

  if (YPos < 0)
    YPos = 0;

  if (XPos > XRange)
    XPos = XRange;

  if (YPos > YRange)
    YPos = YRange;

  if (XRange == 0)
    ratex = 0;
  else
    ratex = XPos / (double) XRange;

  if (YRange == 0)
    ratey = 0;
  else
    ratey = YPos / (double) YRange;

  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);

  if (Menulocal.pix) {
    gdk_drawable_get_size(Menulocal.pix, &w, &h);
  }else { 
    h = w = 0;
  }

  if (w != width || h != height) {
    create_pix(width, height);
    mx_redraw(Menulocal.obj, Menulocal.inst);
  }

  XPos = width * ratex;
  YPos = height * ratey;

  gtk_range_set_range(GTK_RANGE(d->HScroll), 0, width);
  gtk_range_set_value(GTK_RANGE(d->HScroll), XPos);
  d->hupper = width;
  d->hscroll = XPos;

  gtk_range_set_range(GTK_RANGE(d->VScroll), 0, height);
  gtk_range_set_value(GTK_RANGE(d->VScroll), YPos);
  d->vupper = height;
  d->vscroll = YPos;

  Menulocal.scrollx = -d->cx + XPos;
  Menulocal.scrolly = -d->cy + YPos;

  if ((obj = chkobject("text")) != NULL) {
    num = chkobjlastinst(obj);
    for (i = 0; i <= num; i++) {
      inst = chkobjinst(obj, i);
      _getobj(obj, "bbox", inst, &array);
      arrayfree(array);
      _putobj(obj, "bbox", inst, NULL);
    }
  }

  if (redraw) {
    gdk_window_invalidate_rect(d->gdk_win, NULL, TRUE);
  }

  SetHRuler(d);
  SetVRuler(d);
}

void
CloseGC(void)
{
  if (Menulocal.gc == NULL)
    return;
 
  g_object_unref(G_OBJECT(Menulocal.gc));
  Menulocal.gc = NULL;

  if (Menulocal.region != NULL)
    gdk_region_destroy(Menulocal.region);

  Menulocal.region = NULL;

  UnFocus();
}

void
ReopenGC(void)
{
  if (Menulocal.gc) {
    g_object_unref(G_OBJECT(Menulocal.gc));
  }
  Menulocal.gc = gdk_gc_new(Menulocal.win);
  Menulocal.offsetx = 0;
  Menulocal.offsety = 0;
  Menulocal.local->pixel_dot_x =
  Menulocal.local->pixel_dot_y =
    Menulocal.windpi / 25.4 / 100;

  if (Menulocal.region != NULL)
    gdk_region_destroy(Menulocal.region);

  Menulocal.region = NULL;
}

void
draw_paper_frame(void)
{
  int w, h;

  if (Menulocal.gc == NULL || Menulocal.pix == NULL)
    return;

  gdk_gc_set_function(Menulocal.gc, GDK_COPY);
  gdk_gc_set_line_attributes(Menulocal.gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  w = mxd2p(Menulocal.PaperWidth) - 1;
  h = mxd2p(Menulocal.PaperHeight) - 1;

  gdk_gc_set_rgb_fg_color(Menulocal.gc, &gray);

  gdk_draw_rectangle(Menulocal.pix, Menulocal.gc, FALSE, 0, 0, w, h);
}

void
Draw(int SelectFile)
{
  int SShowFrame, SShowLine, SShowRect, SShowCross;
  GdkGC *gc;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  if (SelectFile && !SetFileHidden())
    return;

  ProgressDialogCreate(_("Scaling"));

  FitClear();
  FileAutoScale();
  AdjustAxis();

  SetStatusBar(_("Drawing."));
  ProgressDialogSetTitle(_("Drawing"));

  gc = gdk_gc_new(d->gdk_win);

  if (d->ShowFrame)
    ShowFocusFrame(gc);

  if (d->ShowLine)
    ShowFocusLine(gc, FALSE, 0, d->ChangePoint);

  if (d->ShowRect)
    ShowFrameRect(gc);

  if (Menulocal.show_cross)
    ShowCrossGauge(gc);

  SShowFrame = d->ShowFrame;
  SShowLine = d->ShowLine;
  SShowRect = d->ShowRect;
  SShowCross = Menulocal.show_cross;

  d->ShowFrame = d->ShowLine = d->ShowRect = Menulocal.show_cross = FALSE;
  ReopenGC();
  if (region)
    gdk_region_destroy(region);

  region = NULL;

  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) != NULL) {
    d->ignoreredraw = TRUE;
    _exeobj(Menulocal.GRAobj, "clear", Menulocal.GRAinst, 0, NULL);
    //    reset_event();
    /* XmUpdateDisplay(d->Win); */
    _exeobj(Menulocal.GRAobj, "draw", Menulocal.GRAinst, 0, NULL);
    _exeobj(Menulocal.GRAobj, "flush", Menulocal.GRAinst, 0, NULL);
    d->ignoreredraw = FALSE;
  }

  d->ShowFrame = SShowFrame;
  d->ShowLine = SShowLine;
  d->ShowRect = SShowRect;
  Menulocal.show_cross = SShowCross;

  draw_paper_frame();

  if (d->ShowFrame)
    ShowFocusFrame(gc);

  if (d->ShowLine)
    ShowFocusLine(gc, FALSE, 0, d->ChangePoint);

  if (d->ShowRect)
    ShowFrameRect(gc);

  if (Menulocal.show_cross)
    ShowCrossGauge(gc);

  g_object_unref(G_OBJECT(gc));
  ResetStatusBar();

  ProgressDialogFinalize();

  gdk_window_invalidate_rect(d->gdk_win, NULL, TRUE);
}

static void
Clear(void)
{
  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) != NULL) {
    UnFocus();
    _exeobj(Menulocal.GRAobj, "clear", Menulocal.GRAinst, 0, NULL);
    ReopenGC();
  }
  InfoWinClear();
}

void
CmViewerDraw(void)
{
  if (Menulock || Globallock)
    return;

  Draw(TRUE);

  FileWinUpdate(TRUE);
  AxisWinUpdate(TRUE);
}

void
CmViewerDrawB(GtkWidget *w, gpointer client_data)
{

  if (Menulock || Globallock)
    return;

  Draw(FALSE);

  FileWinUpdate(TRUE);
  AxisWinUpdate(TRUE);
}

void
CmViewerClear(void)
{
  if (Menulock || Globallock)
    return;

  Clear();

  FileWinUpdate(TRUE);
}

void
CmViewerClearB(GtkWidget *w, gpointer client_data)
{
  CmViewerClear();
}

static void
ViewUpdate(void)
{
  int i, id, id2, did, num;
  struct FocusObj *focus;
  struct objlist *obj, *dobj = NULL, *aobj;
  N_VALUE *inst, *inst2, *dinst;
  char *dfield;
  int ret, section;
  int x1, y1;
  int aid = 0, idx = 0, idy = 0, idu = 0, idr = 0, idg, j, lenx, leny;
  int findX, findY, findU, findR, findG;
  char *axisx, *axisy;
  int matchx, matchy;
  struct narray iarray, *sarray;
  int snum;
  char **sdata;
  GdkGC *dc;
  char type;
  char *group, *group2;
  int axis;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);
  dc = gdk_gc_new(d->gdk_win);

  ShowFocusFrame(dc);
  d->ShowFrame = FALSE;

  num = arraynum(d->focusobj);
  axis = FALSE;
  PaintLock = TRUE;

  for (i = num - 1; i >= 0; i--) {
    focus = *(struct FocusObj **) arraynget(d->focusobj, i);
    if (focus == NULL)
      continue;

    inst = chkobjinstoid(focus->obj, focus->oid);
    if (inst == NULL)
      continue;

    obj = focus->obj;
    _getobj(obj, "id", inst, &id);
    ret = IDCANCEL;

    if (obj == chkobject("axis")) {
      axis = TRUE;
      _getobj(obj, "group", inst, &group);

      if ((group != NULL) && (group[0] != 'a')) {
	findX = findY = findU = findR = findG = FALSE;
	type = group[0];

	for (j = 0; j <= id; j++) {
	  inst2 = chkobjinst(obj, j);
	  _getobj(obj, "group", inst2, &group2);
	  _getobj(obj, "id", inst2, &id2);

	  if (group2 == NULL || group2[0] != type)
	    continue;

	  if (strcmp(group + 2, group2 + 2))
	    continue;

	  if (group2[1] == 'X') {
	    findX = TRUE;
	    idx = id2;
	  } else if (group2[1] == 'Y') {
	    findY = TRUE;
	    idy = id2;
	  } else if (group2[1] == 'U') {
	    findU = TRUE;
		idu = id2;
	  } else if (group2[1] == 'R') {
	    findR = TRUE;
	    idr = id2;
	  }
	}

	if (((type == 's') || (type == 'f')) && findX && findY
	    && !_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray)
	    && ((snum = arraynum(sarray)) >= 0)) {
	  sdata = arraydata(sarray);

	  for (j = 1; j < snum; j++) {
	    if (((dobj = getobjlist(sdata[j], &did, &dfield, NULL)) != NULL)
		&& (dobj == chkobject("axisgrid"))) {
	      if ((dinst = chkobjinstoid(dobj, did)) != NULL) {
		_getobj(dobj, "axis_x", dinst, &axisx);
		_getobj(dobj, "axis_y", dinst, &axisy);
		matchx = matchy = FALSE;
		if (axisx != NULL) {
		  arrayinit(&iarray, sizeof(int));
		  if (!getobjilist(axisx, &aobj, &iarray, FALSE, NULL)) {
		    if ((arraynum(&iarray) >= 1)
			&& (obj == aobj)
			&& (arraylast_int(&iarray)
			    == idx))
		      matchx = TRUE;
		  }
		  arraydel(&iarray);
		}

		if (axisy != NULL) {
		  arrayinit(&iarray, sizeof(int));
		  if (!getobjilist(axisy, &aobj, &iarray, FALSE, NULL)) {
		    if ((arraynum(&iarray) >= 1)
			&& (obj == aobj)
			&& (arraylast_int(&iarray)
			    == idy))
		      matchy = TRUE;
		  }
		  arraydel(&iarray);
		}

		if (matchx && matchy) {
		  findG = TRUE;
		  _getobj(dobj, "id", dinst, &idg);
		  break;
		}
	      }
	    }
	  }
	}

	if (((type == 's') || (type == 'f'))
	    && findX && findY && findU && findR) {

	  if (!findG) {
	    dobj = chkobject("axisgrid");
	    idg = -1;
	  }

	  if (type == 's')
	    section = TRUE;
	  else
	    section = FALSE;

	  getobj(obj, "y", idx, 0, NULL, &y1);
	  getobj(obj, "x", idy, 0, NULL, &x1);
	  getobj(obj, "y", idu, 0, NULL, &leny);
	  getobj(obj, "x", idr, 0, NULL, &lenx);

	  leny = y1 - leny;
	  lenx = lenx - x1;

	  SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
			idx, idy, idu, idr, dobj, &idg, section);

	  ret = DialogExecute(TopLevel, &DlgSection);

	  if (ret == IDDELETE) {
	    AxisDel(id);
	    set_graph_modified();
	    arrayndel2(d->focusobj, i);
	  }

	  if (!findG) {
	    if (idg != -1) {
	      if ((dinst = chkobjinst(dobj, idg)) != NULL)
		AddList(dobj, dinst);
	    }
	  }
	} else if ((type == 'c') && findX && findY) {
	  getobj(obj, "x", idx, 0, NULL, &x1);
	  getobj(obj, "y", idy, 0, NULL, &y1);
	  getobj(obj, "length", idx, 0, NULL, &lenx);
	  getobj(obj, "length", idy, 0, NULL, &leny);

	  CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	  ret = DialogExecute(TopLevel, &DlgCross);

	  if (ret == IDDELETE) {
	    AxisDel(aid);
	    set_graph_modified();
	    arrayndel2(d->focusobj, i);
	  }
	}
      } else {
	AxisDialog(&DlgAxis, obj, id, TRUE);
	ret = DialogExecute(TopLevel, &DlgAxis);

	if (ret == IDDELETE) {
	  AxisDel(id);
	  set_graph_modified();
	  arrayndel2(d->focusobj, i);
	}
      }
    } else {
      AddInvalidateRect(obj, inst);

      if (obj == chkobject("path")) {
	LegendArrowDialog(&DlgLegendArrow, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendArrow);
      } else if (obj == chkobject("rectangle")) {
	LegendRectDialog(&DlgLegendRect, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendRect);
      } else if (obj == chkobject("arc")) {
	LegendArcDialog(&DlgLegendArc, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendArc);
      } else if (obj == chkobject("mark")) {
	LegendMarkDialog(&DlgLegendMark, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendMark);
      } else if (obj == chkobject("text")) {
	LegendTextDialog(&DlgLegendText, obj, id);
	ret = DialogExecute(TopLevel, &DlgLegendText);
      } else if (obj == chkobject("merge")) {
	MergeDialog(&DlgMerge, obj, id, 0);
	ret = DialogExecute(TopLevel, &DlgMerge);
      }

      if (ret == IDDELETE) {
	set_graph_modified();
	delobj(obj, id);
      }

      if (ret == IDOK)
	AddInvalidateRect(obj, inst);
    }
  }
  PaintLock = FALSE;

  if (arraynum(d->focusobj) == 0)
    clear_focus_obj(d->focusobj);

  if (! axis)
    d->allclear = FALSE;

  UpdateAll();
  ShowFocusFrame(dc);
  d->ShowFrame = TRUE;
  g_object_unref(G_OBJECT(dc));
}

static void
ViewDelete(void)
{
  int i, id, num;
  struct FocusObj *focus;
  struct objlist *obj;
  N_VALUE *inst;
  GdkGC *dc;
  int axis;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);
  if ((d->MouseMode != MOUSENONE) ||
      (d->Mode != PointB &&
       d->Mode != LegendB &&
       d->Mode != AxisB)) {
    return;
  }

  dc = gdk_gc_new(d->gdk_win);
  ShowFocusFrame(dc);
  d->ShowFrame = FALSE;

  axis = FALSE;
  PaintLock = TRUE;

  num = arraynum(d->focusobj);

  for (i = num - 1; i >= 0; i--) {
    focus = *(struct FocusObj **) arraynget(d->focusobj, i);
    obj = focus->obj;

    inst = chkobjinstoid(obj, focus->oid);
    if (inst == NULL)
      continue;

    AddInvalidateRect(obj, inst);
    DelList(obj, inst);
    _getobj(obj, "id", inst, &id);

    if (obj == chkobject("axis")) {
      AxisDel(id);
      axis = TRUE;
    } else {
      delobj(obj, id);
    }
    set_graph_modified();
  }
  PaintLock = FALSE;

  if (! axis)
    d->allclear = FALSE;

  if (num != 0)
    UpdateAll();

  NSetCursor(GDK_LEFT_PTR);
}

static void
reorder_object(enum object_move_type type)
{
  int id, num;
  struct FocusObj *focus;
  struct objlist *obj;
  N_VALUE *inst;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  if (d->MouseMode != MOUSENONE ||
      (d->Mode != PointB && 
       d->Mode != LegendB && d->Mode != AxisB))
       return;

  num = arraynum(d->focusobj);

  if (num != 1)
    return;

  focus = *(struct FocusObj **) arraynget(d->focusobj, 0);
  obj = focus->obj;

  if (! chkobjchild(chkobject("legend"), obj) && ! chkobjchild(chkobject("merge"), obj))
    return;

  inst = chkobjinstoid(obj, focus->oid);
  if (inst == NULL)
    return;

  DelList(obj, inst);
  _getobj(obj, "id", inst, &id);
  switch (type) {
  case OBJECT_MOVE_TYPE_TOP:
    movetopobj(obj, id);
    break;
  case OBJECT_MOVE_TYPE_LAST:
    movelastobj(obj, id);
    break;
  case OBJECT_MOVE_TYPE_UP:
    moveupobj(obj, id);
    break;
  case OBJECT_MOVE_TYPE_DOWN:
    movedownobj(obj, id);
    break;
  }
  AddList(obj, inst);
  set_graph_modified();
  d->allclear = TRUE;
  UpdateAll();
}

static void
ncopyobj(struct objlist *obj, int id1, int id2)
{
  copy_obj_field(obj, id1, id2, NULL);
}

static void
ViewCopyAxis(struct objlist *obj, int id, struct FocusObj *focus, N_VALUE *inst)
{
  int j, id2, did;
  struct objlist *dobj, *aobj;
  N_VALUE *inst2, *dinst;
  char *dfield;
  int findX, findY, findU, findR, findG;
  char *axisx, *axisy;
  int matchx, matchy;
  struct narray iarray, *sarray;
  int snum;
  char **sdata;
  int oidx, oidy;
  int idx = 0, idy = 0, idu = 0, idr = 0, idg;
  int idx2, idy2, idu2, idr2, idg2;
  char type;
  char *group, *group2;
  int tp;
  struct narray agroup;
  char *argv[2];

  _getobj(obj, "group", inst, &group);

  if ((group != NULL) && (group[0] != 'a')) {
    findX = findY = findU = findR = findG = FALSE;
    type = group[0];

    for (j = 0; j <= id; j++) {
      inst2 = chkobjinst(obj, j);
      _getobj(obj, "group", inst2, &group2);
      _getobj(obj, "id", inst2, &id2);

      if (group2 == NULL || group2[0] != type)
	continue;

      if (strcmp(group + 2, group2 + 2) != 0)
	continue;

      if (group2[1] == 'X') {
	findX = TRUE;
	idx = id2;
      } else if (group2[1] == 'Y') {
	findY = TRUE;
	idy = id2;
      } else if (group2[1] == 'U') {
	findU = TRUE;
	idu = id2;
      } else if (group2[1] == 'R') {
	findR = TRUE;
	idr = id2;
      }
    }

    if (((type == 's') || (type == 'f')) && findX && findY
	&& !_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray)
	&& ((snum = arraynum(sarray)) >= 0)) {
      sdata = arraydata(sarray);
      for (j = 1; j < snum; j++) {
	dobj = getobjlist(sdata[j], &did, &dfield, NULL);
	if (dobj == NULL || dobj != chkobject("axisgrid"))
	  continue;

	dinst = chkobjinstoid(dobj, did);
	if (dinst == NULL)
	  continue;

	_getobj(dobj, "axis_x", dinst, &axisx);
	_getobj(dobj, "axis_y", dinst, &axisy);
	matchx = matchy = FALSE;

	if (axisx) {
	  arrayinit(&iarray, sizeof(int));
	  if (!getobjilist(axisx, &aobj, &iarray, FALSE, NULL)) {
	    if ((arraynum(&iarray) >= 1)
		&& (obj == aobj)
		&& (arraylast_int(&iarray) == idx))
	      matchx = TRUE;
	  }
	  arraydel(&iarray);
	}
	if (axisy) {
	  arrayinit(&iarray, sizeof(int));
	  if (!getobjilist(axisy, &aobj, &iarray, FALSE, NULL)) {
	    if ((arraynum(&iarray) >= 1)
		&& (obj == aobj)
		&& (arraylast_int(&iarray) == idy))
	      matchy = TRUE;
	  }
	  arraydel(&iarray);
	}
	if (matchx && matchy) {
	  findG = TRUE;
	  _getobj(dobj, "id", dinst, &idg);
	  break;
	}
      }
    }

    if (((type == 's') || (type == 'f'))
	&& findX && findY && findU && findR) {
      if ((idx2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idx2, idx);
	inst2 = chkobjinst(obj, idx2);
	_getobj(obj, "oid", inst2, &oidx);
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }

      if ((idy2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idy2, idy);
	inst2 = chkobjinst(obj, idy2);
	_getobj(obj, "oid", inst2, &oidy);
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }

      if ((idu2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idu2, idu);
	inst2 = chkobjinst(obj, idu2);
	if (idx2 >= 0) {
	  axisx = (char *) g_malloc(ID_BUF_SIZE);
	  if (axisx) {
	    snprintf(axisx, ID_BUF_SIZE, "axis:^%d", oidx);
	    putobj(obj, "reference", idu2, axisx);
	  }
	}
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }

      if ((idr2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idr2, idr);
	inst2 = chkobjinst(obj, idr2);
	if (idy2 >= 0) {
	  axisy = (char *) g_malloc(ID_BUF_SIZE);
	  if(axisy) {
	    snprintf(axisy, ID_BUF_SIZE, "axis:^%d", oidy);
	    putobj(obj, "reference", idr2, axisy);
	  }
	}

	arrayinit(&agroup, sizeof(int));

	if (type == 'f')
	  tp = 1;
	else
	  tp = 2;

	arrayadd(&agroup, &tp);
	arrayadd(&agroup, &idx2);
	arrayadd(&agroup, &idy2);
	arrayadd(&agroup, &idu2);
	arrayadd(&agroup, &idr2);

	argv[0] = (char *) &agroup;
	argv[1] = NULL;

	exeobj(obj, "grouping", idr2, 1, argv);

	arraydel(&agroup);

	_getobj(obj, "oid", inst2, &(focus->oid));

	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);

	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }

      if (findG) {
	dobj = chkobject("axisgrid");
	if ((idg2 = newobj(dobj)) >= 0) {
	  ncopyobj(dobj, idg2, idg);
	  inst2 = chkobjinst(dobj, idg2);
	  if (idx2 >= 0 && idu2 >= 0) {
	    axisx = (char *) g_malloc(ID_BUF_SIZE);
	    if (axisx) {
	      snprintf(axisx, ID_BUF_SIZE, "axis:^%d", oidx);
	      putobj(dobj, "axis_x", idg2, axisx);
	    }
	  }
	  if (idy2 >= 0 && idr2 >= 0) {
	    axisy = (char *) g_malloc(ID_BUF_SIZE);
	    if (axisy) {
	      snprintf(axisy, ID_BUF_SIZE, "axis:^%d", oidy);
	      putobj(dobj, "axis_y", idg2, axisy);
	    }
	  }
	  AddList(dobj, inst2);
	  set_graph_modified();
	}
      }
    } else if ((type == 'c') && findX && findY) {
      if ((idx2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idx2, idx);
	inst2 = chkobjinst(obj, idx2);
	_getobj(obj, "oid", inst2, &oidx);
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }

      if ((idy2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idy2, idy);
	inst2 = chkobjinst(obj, idy2);
	_getobj(obj, "oid", inst2, &oidy);
	arrayinit(&agroup, sizeof(int));
	tp = 3;

	arrayadd(&agroup, &tp);
	arrayadd(&agroup, &idx2);
	arrayadd(&agroup, &idy2);

	argv[0] = (char *) &agroup;
	argv[1] = NULL;

	exeobj(obj, "grouping", idy2, 1, argv);
	arraydel(&agroup);

	focus->oid = oidy;
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();

      } else {
	AddInvalidateRect(obj, inst);
      }

      if (idx2 >= 0 && idy2 >= 0) {
	axisy = (char *) g_malloc(ID_BUF_SIZE);
	if (axisy) {
	  snprintf(axisy, ID_BUF_SIZE, "axis:^%d", oidy);
	  putobj(obj, "adjust_axis", idx2, axisy);
	}
	axisx = (char *) g_malloc(ID_BUF_SIZE);
	if (axisx) {
	  snprintf(axisx, ID_BUF_SIZE, "axis:^%d", oidx);
	  putobj(obj, "adjust_axis", idy2, axisx);
	}
      }
    }
  } else {
    if ((id2 = newobj(obj)) >= 0) {
      ncopyobj(obj, id2, id);
      inst2 = chkobjinst(obj, id2);
      _getobj(obj, "oid", inst2, &(focus->oid));
      AddList(obj, inst2);
      AddInvalidateRect(obj, inst2);
      set_graph_modified();
    } else {
      AddInvalidateRect(obj, inst);
    }
  }
}

static void
ViewCopy(void)
{
  int i, id, id2, num;
  struct FocusObj *focus;
  struct objlist *obj;
  N_VALUE *inst, *inst2;
  GdkGC *dc;
  int axis = FALSE;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  if (d->MouseMode != MOUSENONE || ! (d->Mode & POINT_TYPE_POINT))
    return;

  dc = gdk_gc_new(d->gdk_win);

  ShowFocusFrame(dc);
  d->ShowFrame = FALSE;

  axis = FALSE;
  PaintLock = TRUE;

  num = arraynum(d->focusobj);

  for (i = 0; i < num; i++) {
    focus = * (struct FocusObj **) arraynget(d->focusobj, i);
    if (focus == NULL)
      continue;

    inst = chkobjinstoid(focus->obj, focus->oid);
    if (inst == NULL)
      continue;

    obj = focus->obj;
    _getobj(obj, "id", inst, &id);

    if (obj == chkobject("axis")) {
      axis = TRUE;
      ViewCopyAxis(obj, id, focus, inst);
    } else {
      if ((id2 = newobj(obj)) >= 0) {
	ncopyobj(obj, id2, id);
	inst2 = chkobjinst(obj, id2);
	_getobj(obj, "oid", inst2, &(focus->oid));
	AddList(obj, inst2);
	AddInvalidateRect(obj, inst2);
	set_graph_modified();
      } else {
	AddInvalidateRect(obj, inst);
      }
    }
  }
  PaintLock = FALSE;

  if (! axis)
    d->allclear = FALSE;

  UpdateAll();
  ShowFocusFrame(dc);
  d->ShowFrame = TRUE;
  g_object_unref(G_OBJECT(dc));
}

void
ViewCross(int state)
{
  GdkGC *dc;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &(NgraphApp.Viewer);

  if (state != Menulocal.show_cross) {
    dc = gdk_gc_new(d->gdk_win);
    ShowCrossGauge(dc);
    Menulocal.show_cross = state;
    g_object_unref(G_OBJECT(dc));
  }
}

static void
ViewerPopupMenu(GtkWidget *w, gpointer client_data)
{
  switch ((int) client_data) {
  case VIEW_UPDATE:
    ViewUpdate();
    break;
  case VIEW_DELETE:
    ViewDelete();
    break;
  case VIEW_DUP:
    ViewCopy();
    break;
  case VIEW_CUT:
    CutFocusedObjects();
    break;
  case VIEW_COPY:
    CopyFocusedObjects();
    break;
  case VIEW_PASTE:
    PasteObjectsFromClipboard();
    break;
  case VIEW_TOP:
    reorder_object(OBJECT_MOVE_TYPE_TOP);
    break;
  case VIEW_LAST:
    reorder_object(OBJECT_MOVE_TYPE_LAST);
    break;
  case VIEW_UP:
    reorder_object(OBJECT_MOVE_TYPE_UP);
    break;
  case VIEW_DOWN:
    reorder_object(OBJECT_MOVE_TYPE_DOWN);
    break;
  case VIEW_CROSS:
    ViewCross(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
    break;
  case VIEW_ALIGN_LEFT:
  case VIEW_ALIGN_HCENTER:
  case VIEW_ALIGN_RIGHT:
  case VIEW_ALIGN_TOP:
  case VIEW_ALIGN_VCENTER:
  case VIEW_ALIGN_BOTTOM:
    AlignFocusedObj((int) client_data);
    break;
  case VIEW_ROTATE_CLOCKWISE:
    RotateFocusedObj(ROTATE_CLOCKWISE);
    break;
  case VIEW_ROTATE_COUNTER_CLOCKWISE:
    RotateFocusedObj(ROTATE_COUNTERCLOCKWISE);
    break;
  case VIEW_FLIP_HORIZONTAL:
    FlipFocusedObj(FLIP_DIRECTION_HORIZONTAL);
    break;
  case VIEW_FLIP_VERTICAL:
    FlipFocusedObj(FLIP_DIRECTION_VERTICAL);
    break;
  }
}

gboolean
CmViewerButtonPressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  KeepMouseMode = (event->state & GDK_SHIFT_MASK);
  return FALSE;
}

void
CmEditMenuCB(GtkToolItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdEditCut:
    CutFocusedObjects();
    break;
  case MenuIdEditCopy:
    CopyFocusedObjects();
    break;
  case MenuIdEditPaste:
    PasteObjectsFromClipboard();
    break;
  case MenuIdEditDelete:
    ViewDelete();
    break;
  case MenuIdAlignLeft:
    AlignFocusedObj(VIEW_ALIGN_LEFT);
    break;
  case MenuIdAlignVCenter:
    AlignFocusedObj(VIEW_ALIGN_VCENTER);
    break;
  case MenuIdAlignRight:
    AlignFocusedObj(VIEW_ALIGN_RIGHT);
    break;
  case MenuIdAlignTop:
    AlignFocusedObj(VIEW_ALIGN_TOP);
    break;
  case MenuIdAlignHCenter:
    AlignFocusedObj(VIEW_ALIGN_HCENTER);
    break;
  case MenuIdAlignHBottom:
    AlignFocusedObj(VIEW_ALIGN_BOTTOM);
    break;
  case MenuIdEditRotateCW:
    RotateFocusedObj(ROTATE_CLOCKWISE);
    break;
  case MenuIdEditRotateCCW:
    RotateFocusedObj(ROTATE_COUNTERCLOCKWISE);
    break;
  case MenuIdEditFlipHorizontally:
    FlipFocusedObj(FLIP_DIRECTION_HORIZONTAL);
    break;
  case MenuIdEditFlipVertically:
    FlipFocusedObj(FLIP_DIRECTION_VERTICAL);
    break;
  }
}

void
CmViewerButtonArm(GtkToolItem *w, gpointer client_data)
{
  int Mode = PointB;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  if (! gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(w)))
    return;

  Mode = (int) client_data;

  UnFocus();

  switch (Mode) {
  case PointB:
    DefaultMode = 0;
    NSetCursor(GDK_LEFT_PTR);
    break;
  case LegendB:
    DefaultMode = 1;
    NSetCursor(GDK_LEFT_PTR);
    break;
  case AxisB:
    DefaultMode = 2;
    NSetCursor(GDK_LEFT_PTR);
    break;
  case TrimB:
  case DataB:
  case EvalB:
    NSetCursor(GDK_LEFT_PTR);
    break;
  case TextB:
    NSetCursor(GDK_XTERM);
    break;
  case ZoomB:
    NSetCursor(GDK_TARGET);
    break;
  default:
    NSetCursor(GDK_PENCIL);
  }
  NgraphApp.Viewer.Mode = Mode;
  NgraphApp.Viewer.Capture = FALSE;
  NgraphApp.Viewer.MouseMode = MOUSENONE;

  if (d->MoveData) {
    move_data_cancel(d, TRUE);
  }
}
