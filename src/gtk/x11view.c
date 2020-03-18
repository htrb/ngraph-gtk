/* -*- coding: utf-8 -*- */
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

#include "object.h"
#include "gra.h"
#include "odata.h"
#include "olegend.h"
#include "oarc.h"
#include "opath.h"
#include "mathfn.h"
#include "ioutil.h"
#include "nstring.h"
#include "shell.h"

#include "gtk_liststore.h"
#include "gtk_widget.h"
#include "gtk_ruler.h"
#include "gtk_presettings.h"
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

#define SCROLL_INC 20
#define POINT_ERROR 4

#define ZOOM_SPEED_NORMAL 1.4
#define ZOOM_SPEED_LITTLE 1.1

enum object_move_type {
  OBJECT_MOVE_TYPE_TOP,
  OBJECT_MOVE_TYPE_UP,
  OBJECT_MOVE_TYPE_DOWN,
  OBJECT_MOVE_TYPE_LAST,
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

enum ViewerAlignType {
  VIEW_ALIGN_LEFT,
  VIEW_ALIGN_RIGHT,
  VIEW_ALIGN_HCENTER,
  VIEW_ALIGN_TOP,
  VIEW_ALIGN_VCENTER,
  VIEW_ALIGN_BOTTOM,
};

#define POINT_LENGTH 5
#define FOCUS_FRAME_OFST 5
#define FOCUS_RECT_SIZE 6

static cairo_region_t *region = NULL;
static int PaintLock = FALSE, ZoomLock = FALSE, KeepMouseMode = FALSE;

#define EVAL_NUM_MAX 5000
static struct evaltype EvalList[EVAL_NUM_MAX];
static struct narray SelList;

#define IDEVMASK        101
#define IDEVMOVE        102

static void ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data);
static void ViewerEvHScroll(GtkRange *range, gpointer user_data);
static void ViewerEvVScroll(GtkRange *range, gpointer user_data);
static gboolean ViewerEvPaint(GtkWidget *w, cairo_t *cr, gpointer client_data);
static gboolean ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvButtonDown(GtkWidget *w, GdkEventButton *e, gpointer client_data);
static gboolean ViewerEvButtonUp(GtkWidget *w, GdkEventButton *e, gpointer client_data);
static gboolean ViewerEvMouseMotion(GtkWidget *w, GdkEventMotion *e, gpointer client_data);
static gboolean ViewerEvScroll(GtkWidget *w, GdkEventScroll *e, gpointer client_data);
static gboolean ViewerEvKeyDown(GtkWidget *w, GdkEventKey *e, gpointer client_data);
static gboolean ViewerEvKeyUp(GtkWidget *w, GdkEventKey *e, gpointer client_data);
static void DelList(struct objlist *obj, N_VALUE *inst, const struct Viewer *d);
static void ViewUpdate(void);
static void ViewCopy(void);
static void do_popup(GdkEventButton *event, struct Viewer *d);
static int check_focused_obj(struct narray *focusobj, struct objlist *fobj, int oid);
static int get_mouse_cursor_type(struct Viewer *d, int x, int y);
static void reorder_object(enum object_move_type type);
static void SetHRuler(const struct Viewer *d);
static void SetVRuler(const struct Viewer *d);
static void clear_focus_obj(const struct Viewer *d);
static void ViewDelete(void);
static int text_dropped(const char *str, gint x, gint y, struct Viewer *d);
static int add_focus_obj(struct narray *focusobj, struct objlist *obj, int oid);
static void ShowFocusFrame(cairo_t *cr, const struct Viewer *d);
static void AddInvalidateRect(struct objlist *obj, N_VALUE *inst);
static void AddList(struct objlist *obj, N_VALUE *inst);
static void RotateFocusedObj(int direction);
static void set_mouse_cursor_hover(struct Viewer *d, int x, int y);
static void CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom_x, double *zoom_y);
static int check_drawrable(struct objlist *obj);

#define GRAY 0.5
#define DOT_LENGTH 4.0

int
check_paint_lock(void)
{
  return PaintLock;
}

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
}

static int
mxd2py(int y)
{
  return nround(y * Menulocal.local->pixel_dot_y + Menulocal.local->offsety);
}
#endif

static int
mxp2d(int r)
{
  return ceil(r / Menulocal.local->pixel_dot_x);
}

static int
calc_mouse_x(int x, double zoom, const struct Viewer *d)
{
  return nround((mxp2d(x + d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom);
}

static int
calc_mouse_y(int y, double zoom, const struct Viewer *d)
{
  return nround((mxp2d(y + d->vscroll - d->cy) - Menulocal.TopMargin) / zoom);
}

static double
range_increment(GtkWidget *w, double inc)
{
  double val;

  val = gtk_range_get_value(GTK_RANGE(w));
  gtk_range_set_value(GTK_RANGE(w), val + inc);

  return val;
}

static char SCRIPT_IDN[] = "#! ngraph\n# clipboard\n\n";
#define SCRIPT_IDN_LEN (sizeof(SCRIPT_IDN) - 1)

struct FOCUSED_INST {
  int id;
  struct FocusObj *focus;
};

static int
compare_focused_inst(const void *a, const void *b)
{
  struct FOCUSED_INST *inst_a, *inst_b;
  inst_a = (struct FOCUSED_INST *) a;
  inst_b = (struct FOCUSED_INST *) b;
  return (inst_a->id - inst_b->id);
}

static struct FOCUSED_INST *
create_focused_inst_array_by_id_order(struct FocusObj **focus, int n)
{
  struct FOCUSED_INST *focused_inst;
  int i;

  if (focus == NULL || n < 1) {
    return NULL;
  }

  focused_inst = g_malloc(sizeof(*focused_inst) * n);
  if (focused_inst == NULL) {
    return NULL;
  }
  for (i = 0; i < n; i++) {
    int id;
    id = chkobjoid(focus[i]->obj, focus[i]->oid);
    focused_inst[i].id = id;
    focused_inst[i].focus = focus[i];
  }
  qsort(focused_inst, n, sizeof(*focused_inst), compare_focused_inst);
  return focused_inst;
}

static int
CopyFocusedObjects(void)
{
  struct narray *focus_array;
  struct FocusObj **focus;
  struct objlist *axis;
  char *s;
  int i, r, n, num;
  GString *str;
  struct FOCUSED_INST *focused_inst;

  focus_array = NgraphApp.Viewer.focusobj;
  n = arraynum(focus_array);

  if (n < 1)
    return 1;

  focus = arraydata(focus_array);
  focused_inst = create_focused_inst_array_by_id_order(focus, n);
  if (focused_inst == NULL) {
    return 1;
  }

  str = g_string_sized_new(256);
  if (str == NULL) {
    g_free(focused_inst);
    return 1;
  }

  axis = chkobject("axis");
  g_string_append(str, SCRIPT_IDN);
  num = 0;
  for (i = 0; i < n; i++) {
    struct FocusObj *inst;
    int id;
    inst = focused_inst[i].focus;
    if (inst->obj == axis) {
      g_free(focused_inst);
      g_string_free(str, TRUE);
      return 1;
    }

    id = chkobjoid(inst->obj, inst->oid);
    if (id < 0)
      continue;

    r = getobj(inst->obj, "save", id, 0, NULL, &s);
    if (r < 0 || s == NULL) {
      g_free(focused_inst);
      g_string_free(str, TRUE);
      return 1;
    }

    g_string_append(str, s);
    num++;
  }

  if (num > 0) {
    GtkClipboard* clipboard;
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, str->str, -1);
  }

  g_free(focused_inst);
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
focus_new_insts(struct objlist *parent, struct narray *array, char **objects)
{
  struct objlist *ocur;
  int i, oid;
  N_VALUE *inst;

  ocur = parent->child;
  while (chkobjparent(ocur) == parent) {
    int instnum, prev_instnum;
    instnum = chkobjlastinst(ocur);
    prev_instnum = arraynget_int(array, 0);
    arrayndel(array, 0);
    if (chkobjfield(ocur, "bbox") == 0) {
      if (instnum != prev_instnum) {
	*objects = ocur->name;
	objects++;
	*objects = NULL;
      }
      for (i = prev_instnum + 1; i <= instnum; i++) {
	getobj(ocur, "oid", i, 0, NULL, &oid);
	add_focus_obj(NgraphApp.Viewer.focusobj, ocur, oid);
	inst = chkobjinst(ocur, i);
	AddList(ocur, inst);
      }
    }
    if (ocur->child) {
      focus_new_insts(ocur, array, objects);
    }
    ocur = ocur->next;
  }
  return;
}

static void
paste_cb(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
  struct narray idarray;
  struct objlist *draw_obj;
  GdkWindow *window;
  char *objects[OBJ_MAX] = {NULL};

  if (text == NULL)
    return;

  window = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if (window == NULL)
    return;

  if (strncmp(text, SCRIPT_IDN, SCRIPT_IDN_LEN)) {
    gint w, h;

    w = gdk_window_get_width(window);
    h = gdk_window_get_height(window);
    text_dropped(text, w / 2, h / 2, &NgraphApp.Viewer);
    return;
  }

  draw_obj = chkobject("draw");
  if (draw_obj == NULL)
    return;

  arrayinit(&idarray, sizeof(int));
  check_last_insts(draw_obj, &idarray);

  UnFocus();
  menu_save_undo(UNDO_TYPE_PASTE, NULL);
  eval_script(text, TRUE);

  focus_new_insts(draw_obj, &idarray, objects);
  arraydel(&idarray);

  if (arraynum(NgraphApp.Viewer.focusobj) > 0) {
    set_graph_modified();
    NgraphApp.Viewer.allclear = FALSE;
    NgraphApp.Viewer.ShowFrame = TRUE;
    gtk_widget_grab_focus(NgraphApp.Viewer.Win);
    UpdateAll(objects);
  }
}

static void
PasteObjectsFromClipboard(void)
{
  GtkClipboard *clip;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  if (d->Win == NULL || (d->Mode != PointB && d->Mode != LegendB)) {
    return;
  }

  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  if (gtk_clipboard_wait_is_text_available(clip)) {
    GdkDevice *device;
    gint x, y;
    gtk_clipboard_request_text(clip, paste_cb, NULL);
    device = gtk_get_current_event_device(); /* fix-me: is there any other appropriate way to get the device? */
    if (device && gdk_device_get_source(device) != GDK_SOURCE_KEYBOARD) {
      GdkWindow *win;
      win = gtk_widget_get_window(d->Win);
      if (win) {
	gdk_window_get_device_position(win, device, &x, &y, NULL);
	set_mouse_cursor_hover(d, x, y);
      }
    }
  }
}

static int
graph_dropped(char *fname)
{
  char *ext;

  if (fname == NULL) {
    return 1;
  }

  ext = getextention(fname);
  if (ext == NULL)
    return 1;

  if (strcmp0(ext, "ngp")) {
    return 1;
  }

  if (!CheckSave()) {
    return 0;
  }

  LoadNgpFile(fname, FALSE, "-f");
  return 0;
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
  MergeDialog(NgraphApp.MergeWin.data.data, id, -1);
  ret = DialogExecute(TopLevel, &DlgMerge);
  if (ret == IDCANCEL) {
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

  putobj(obj, "file", id, name);
  if (*id0 != -1) {
    copy_file_obj_field(obj, id, *id0, FALSE);
    AddDataFileList(name);
    return 0;
  }

  FileDialog(NgraphApp.FileWin.data.data, id, multi);
  ret = DialogExecute(TopLevel, &DlgFile);
  if (ret == IDCANCEL) {
    FitDel(obj, id);
    delobj(obj, id);
  } else {
    if (ret == IDFAPPLY) {
      *id0 = id;
    }
    set_graph_modified();
    AddDataFileList(name);
  }

  return 0;
}

int
data_dropped(char **filenames, int num, int file_type)
{
  char *ext, *arg[4];
  int i, id0, type, ret;
  struct objlist *obj, *mobj;

  obj = chkobject("data");
  if (obj == NULL) {
    return 1;
  }

  mobj = chkobject("merge");
  if (mobj == NULL) {
    return 1;
  }

  id0 = -1;
  arg[0] = obj->name;
  arg[1] = mobj->name;
  arg[2] = "fit";
  arg[3] = NULL;
  menu_save_undo(UNDO_TYPE_PASTE, arg);
  for (i = 0; i < num; i++) {
    char *name;
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

  MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, FALSE);
  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  return 0;
}

static int
text_dropped(const char *str, gint x, gint y, struct Viewer *d)
{
  N_VALUE *inst;
  char *ptr;
  double zoom = Menulocal.PaperZoom / 10000.0;
  struct objlist *obj;
  int id, x1, y1, r, i, j, l, undo;

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

  undo = menu_save_undo_single(UNDO_TYPE_PASTE, obj->name);
  id = newobj(obj);
  if (id < 0) {
    g_free(ptr);
    return 1;
  }

  inst = chkobjinst(obj, id);
  x1 = calc_mouse_x(x, zoom, d);
  y1 = calc_mouse_y(y, zoom, d);

  CheckGrid(FALSE, 0, &x1, &y1, NULL, NULL);

  _putobj(obj, "x", inst, &x1);
  _putobj(obj, "y", inst, &y1);
  _putobj(obj, "text", inst, ptr);

  PaintLock= TRUE;

  LegendTextDialog(&DlgLegendText, obj, id);
  r = DialogExecute(TopLevel, &DlgLegendText);

  if ((r == IDDELETE) || (r == IDCANCEL)) {
    menu_delete_undo(undo);
    delobj(obj, id);
  } else {
    int oid;
    char *objects[] = {"text", NULL};

    UnFocus();

    getobj(obj, "oid", id, 0, NULL, &oid);
    add_focus_obj(NgraphApp.Viewer.focusobj, obj, oid);
    d->allclear = FALSE;
    AddList(obj, inst);

    set_graph_modified();
    d->ShowFrame = TRUE;
    gtk_widget_grab_focus(d->Win);
    UpdateAll(objects);
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
  n = 0;
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
    GtkClipboard *clip;

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
  struct EvalDialog *d;
  n_list_store list[] = {
    {"#",           G_TYPE_INT,    TRUE,  FALSE, NULL},
    {_("Line No."), G_TYPE_INT,    TRUE,  FALSE, NULL},
    {"X",           G_TYPE_STRING, TRUE,  FALSE, NULL},
    {"Y",           G_TYPE_STRING, TRUE,  FALSE, NULL},
    {"N",           G_TYPE_INT,    FALSE, FALSE, NULL},
  };


  d = (struct EvalDialog *) data;
  if (makewidget) {
    GtkWidget *w, *swin, *hbox;
    GtkTreeSelection *sel;
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

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    w = gtk_button_new_with_mnemonic(_("Select _All"));
    set_button_icon(w, "edit-select-all");
    g_signal_connect(w, "clicked", G_CALLBACK(tree_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Copy"));
    g_signal_connect(w, "clicked", G_CALLBACK(eval_dialog_copy_selected), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_widget_set_sensitive(w, FALSE);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    g_signal_connect(sel, "changed", G_CALLBACK(eval_data_sel_cb), w);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    d->show_cancel = FALSE;
    d->ok_button = _("_Close");

    gtk_window_set_default_size(GTK_WINDOW(wi), 540, 400);

    gtk_widget_show_all(GTK_WIDGET(d->vbox));
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

  d = (struct EvalDialog *) data;
  if ((d->ret == IDEVMASK) || (d->ret == IDEVMOVE)) {
    GtkTreeSelection *selected;
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


static gboolean
scrollbar_scroll_cb(GtkWidget *w, GdkEventScroll *e, gpointer client_data)
{
  gdouble x, y;

  switch (e->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_LEFT:
    range_increment(w, -SCROLL_INC);
    break;
  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_RIGHT:
    range_increment(w, SCROLL_INC);
    break;
  case GDK_SCROLL_SMOOTH:
    if (gdk_event_get_scroll_deltas((GdkEvent *) e, &x, &y)) {
      range_increment(w, y * SCROLL_INC);
    }
    return TRUE;
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

static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct Viewer *d;

  if (Menulock || Globallock) return TRUE;

  d = (struct Viewer *) client_data;
  do_popup(NULL, d);
  return TRUE;
}

void
ViewerWinSetup(void)
{
  struct Viewer *d;
  int x, y, width, height;
  GdkWindow *win;

  d = &NgraphApp.Viewer;
  Menulocal.GRAoid = -1;
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
  d->KeyMask = 0;
  region = NULL;
  OpenGC();
  OpenGRA();
  SetScroller();
  win = gtk_widget_get_window(NgraphApp.Viewer.Win);
  gdk_window_get_position(win, &x, &y);
  width = gdk_window_get_width(win);
  height = gdk_window_get_height(win);
  d->cx = width / 2;
  d->cy = height / 2;

  d->hscroll = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->vscroll = gtk_range_get_value(GTK_RANGE(d->VScroll));

  ChangeDPI();

  g_signal_connect(d->Win, "draw", G_CALLBACK(ViewerEvPaint), d);
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
			GDK_SCROLL_MASK |
			GDK_SMOOTH_SCROLL_MASK |
			GDK_KEY_RELEASE_MASK);
  gtk_widget_set_can_focus(d->Win, TRUE);

  if (d->popup) {
    gtk_menu_attach_to_widget(GTK_MENU(d->popup), GTK_WIDGET(d->Win), NULL);
  }

  g_signal_connect(d->Win, "button-press-event", G_CALLBACK(ViewerEvButtonDown), d);
  g_signal_connect(d->Win, "button-release-event", G_CALLBACK(ViewerEvButtonUp), d);
  g_signal_connect(d->Win, "motion-notify-event", G_CALLBACK(ViewerEvMouseMotion), d);
  g_signal_connect(d->Win, "scroll-event", G_CALLBACK(ViewerEvScroll), d);
  g_signal_connect(d->Win, "key-press-event", G_CALLBACK(ViewerEvKeyDown), d);
  g_signal_connect(d->Win, "key-release-event", G_CALLBACK(ViewerEvKeyUp), d);
  g_signal_connect(d->Win, "popup-menu", G_CALLBACK(ev_popup_menu), d);

  g_signal_connect(d->menu, "selection-done", G_CALLBACK(menu_activate), d);
}

void
ViewerWinClose(void)
{
  struct Viewer *d;

  d = &NgraphApp.Viewer;
  arrayfree2(d->focusobj);
  arrayfree2(d->points);

  d->focusobj = NULL;
  d->points = NULL;

  if (region) {
    cairo_region_destroy(region);
    region = NULL;
  }

}

static int
ViewerWinFileUpdate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  char *argv[7];
  int snum, hidden;
  int did, limit;
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

  fileobj = chkobject("data");
  if (! fileobj)
    goto End;

  arrayinit(&dfile, sizeof(int));

  if (check_drawrable(fileobj)) {
    goto End;
  }

  snum = chkobjlastinst(fileobj) + 1;
  if (snum == 0) {
    goto End;
  }

  snprintf(mes, sizeof(mes), _("Searching for data."));
  SetStatusBar(mes);
  ProgressDialogCreate(_("Searching for data."));

  for (i = 0; i < snum; i++) {
    dinst = chkobjinst(fileobj, i);
    if (dinst == NULL) {
      continue;
    }
    _getobj(fileobj, "hidden", dinst, &hidden);
    if (hidden) {
      continue;
    }
    _getobj(fileobj, "oid", dinst, &did);
    _exeobj(fileobj, "evaluate", dinst, 6, argv);
    _getobj(fileobj, "evaluate", dinst, &eval);
    evalnum = arraynum(eval) / 3;
    if (evalnum != 0) {
      arrayadd(&dfile, &i);
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
  int i, j;
  struct narray *mask;

  for (i = 0; i < selnum; i++) {
    int masknum, sel;
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
Evaluate(int x1, int y1, int x2, int y2, int err, struct Viewer *d)
{
  struct objlist *fileobj;
  char *argv[7];
  int snum, hidden;
  int limit;
  int i, j;
  struct narray *eval;
  int evalnum, tot;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  double line, dx, dy;
  char mes[256];

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

  if ((fileobj = chkobject("data")) == NULL)
    return;

  if (check_drawrable(fileobj)) {
    return;
  }

  snum = chkobjlastinst(fileobj) + 1;
  if (snum == 0) {
    return;
  }
  ignorestdio(&save);

  snprintf(mes, sizeof(mes), _("Evaluating."));
  SetStatusBar(mes);

  ProgressDialogCreate(_("Evaluating"));

  tot = 0;

  for (i = 0; i < snum; i++) {
    N_VALUE *dinst;
    dinst = chkobjinst(fileobj, i);
    if (dinst == NULL) {
      continue;
    }
    _getobj(fileobj, "hidden", dinst, &hidden);
    if (hidden) {
      continue;
    }
    _exeobj(fileobj, "evaluate", dinst, 6, argv);
    _getobj(fileobj, "evaluate", dinst, &eval);
    evalnum = arraynum(eval) / 3;
    for (j = 0; j < evalnum; j++) {
      if (tot >= limit) break;
      tot++;
      line = arraynget_double(eval, j * 3 + 0);
      dx = arraynget_double(eval, j * 3 + 1);
      dy = arraynget_double(eval, j * 3 + 2);
      EvalList[tot - 1].id = i;
      EvalList[tot - 1].line = nround(line);
      EvalList[tot - 1].x = dx;
      EvalList[tot - 1].y = dy;
    }
    if (tot >= limit) break;
  }

  ProgressDialogFinalize();
  ResetStatusBar();

  if (tot > 0) {
    int ret, selnum;
    EvalDialog(&DlgEval, fileobj, tot, &SelList);
    ret = DialogExecute(TopLevel, &DlgEval);
    selnum = arraynum(&SelList);

    if (selnum > 0) {
      switch (ret) {
      case IDEVMASK:
	menu_save_undo_single(UNDO_TYPE_EDIT, fileobj->name);
	mask_selected_data(fileobj, selnum, &SelList);
	arraydel(&SelList);
	argv[0] = "data";
	argv[1] = NULL;
	UpdateAll(argv);
	break;
      case IDEVMOVE:
	NSetCursor(GDK_TCROSS);
	d->Capture = TRUE;
	d->MoveData = TRUE;
	break;
      }
    }
  }
  restorestdio(&save);
}

static void
Trimming(int x1, int y1, int x2, int y2, struct Viewer *d)
{
  struct narray farray;
  struct objlist *obj;
  int maxx, maxy, minx, miny;
  int dir, room;

  if ((x1 == x2) && (y1 == y2))
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  SelectDialog(&DlgSelect, obj, _("trimming (multi select)"), AxisCB, (struct narray *) &farray, NULL);

  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    int i;
    int *array, num;
    int vx1, vy1, vx2, vy2;
    char *argv[4];
    vx1 = x1 - x2;
    vy1 = y1 - y2;
    vx2 = x2 - x1;
    vy2 = y1 - y2;

    num = arraynum(&farray);
    array = arraydata(&farray);

    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_TRIMMING, obj->name);
    }
    for (i = 0; i < num; i++) {
      double ax, ay, ip1, ip2, min, max;
      int id;
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
        int rcode1, rcode2;
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
	  room = 0;
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
    argv[0] = "data";
    argv[1] = "axis";
    argv[2] = "axisgrid";
    argv[3] = NULL;
    UpdateAll(argv);
  }
  arraydel(&farray);
}

struct view_region {
  int x1, y1, x2, y2, err;
};

static int
select_obj(const char *objname, const struct Viewer *d, struct view_region *region)
{
  struct objlist *fobj, *aobj;
  char *argv[6];
  struct narray *sarray;
  char **sdata;
  int snum, dnum;
  int did, oid;
  N_VALUE *dinst;
  int i, match, hidden, r;
  int minx, miny, maxx, maxy, err;
  struct savedstdio save;

  if (region) {
    minx = (region->x1 < region->x2) ? region->x1 : region->x2;
    miny = (region->y1 < region->y2) ? region->y1 : region->y2;
    maxx = (region->x1 > region->x2) ? region->x1 : region->x2;
    maxy = (region->y1 > region->y2) ? region->y1 : region->y2;
    err = region->err;

    argv[0] = (char *) &minx;
    argv[1] = (char *) &miny;
    argv[2] = (char *) &maxx;
    argv[3] = (char *) &maxy;
    argv[4] = (char *) &err;
    argv[5] = NULL;
  }

  fobj = chkobject(objname);
  if (! fobj) {
    return 0;
  }

  sarray = &Menulocal.drawrable;
  snum = arraynum(sarray);
  if (snum < 1) {
    return 0;
  }

  ignorestdio(&save);
  aobj = getobject("axis");
  sdata = arraydata(sarray);

  r = 0;
  for (i = 0; i < snum; i++) {
    struct objlist *dobj;
    dobj = getobject(sdata[i]);
    if (dobj == NULL) {
      continue;
    }
    if (! chkobjchild(fobj, dobj)) {
      continue;
    }
    dnum = chkobjlastinst(dobj) + 1;
    if (dnum < 1) {
      continue;
    }
    for (did = 0; did < dnum; did++) {
      dinst = chkobjinst(dobj, did);
      if (dinst == NULL) {
	continue;
      }
      _getobj(dobj, "hidden", dinst, &hidden);
      if (hidden) {
	continue;
      }
      if (dobj == aobj) {
	getobj(fobj, "group_manager", did, 0, NULL, &did);
	dinst = chkobjinst(dobj, did);
      }
      _getobj(dobj, "oid", dinst, &oid);
      if (region) {
	_exeobj(dobj, "match", dinst, 5, argv);
	_getobj(dobj, "match", dinst, &match);
	if (! match) {
	  continue;
	}
      }
      if (add_focus_obj(d->focusobj, dobj, oid)) {
	r++;
      }
    }
  }

  restorestdio(&save);
  return r;
}

static int
Match(const char *objname, int x1, int y1, int x2, int y2, int err, const struct Viewer *d)
{
  struct view_region view_region;
  view_region.x1 = x1;
  view_region.y1 = y1;
  view_region.x2 = x2;
  view_region.y2 = y2;
  view_region.err = err;
  return select_obj(objname, d, &view_region);
}

static void
ViewSelectAll(void)
{
  struct Viewer *d;
  int focus;

  d = &NgraphApp.Viewer;
  switch (d->Mode) {
  case PointB:
    select_obj("axis",   d, NULL);
    select_obj("legend", d, NULL);
    select_obj("merge",  d, NULL);
    focus = TRUE;
    break;
  case AxisB:
    select_obj("axis",   d, NULL);
    focus = TRUE;
    break;
  case LegendB:
    select_obj("legend", d, NULL);
    select_obj("merge",  d, NULL);
    focus = TRUE;
    break;
  default:
    focus = FALSE;
    break;
  }
  if (focus) {
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    set_focus_sensitivity(d);
    gtk_widget_queue_draw(d->Win);
  }
}

void
ViewerSelectAllObj(struct objlist *obj)
{
  struct Viewer *d;
  const char *name;
  int n;

  name = chkobjectname(obj);
  if (name == NULL) {
    return;
  }
  UnFocus();
  d = &NgraphApp.Viewer;
  n = select_obj(name, d, NULL);
  if (n < 1) {
    return;
  }
  d->FrameOfsX = d->FrameOfsY = 0;
  d->ShowFrame = TRUE;
  set_focus_sensitivity(d);
  gtk_widget_queue_draw(d->Win);
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
  char *field;
  struct narray *draw, drawrable;

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
    char **objname;
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
DelList(struct objlist *obj, N_VALUE *inst, const struct Viewer *d)
{
  int i, oid, oid2;
  struct objlist *obj2;
  char *field;

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
  cairo_rectangle_int_t rect;
  if (chkobjfield(obj, "bbox")) {
    return;
  }

  _exeobj(obj, "bbox", inst, 0, NULL);
  _getobj(obj, "bbox", inst, &abbox);
  bboxnum = arraynum(abbox);
  bbox = arraydata(abbox);
  if (bboxnum < 4) {
    return;
  }

  zoom = Menulocal.PaperZoom / 10000.0;

  rect.x = mxd2p(bbox[0] * zoom + Menulocal.LeftMargin) - 7;
  rect.y = mxd2p(bbox[1] * zoom + Menulocal.TopMargin) - 7;
  rect.width = mxd2p(bbox[2] * zoom + Menulocal.LeftMargin) - rect.x + 7;
  rect.height = mxd2p(bbox[3] * zoom + Menulocal.TopMargin) - rect.y + 7;

  if (region == NULL) {
    region = cairo_region_create_rectangle(&rect);
  } else {
    cairo_region_union_rectangle(region, &rect);
  }
}

static void
GetLargeFrame(int *minx, int *miny, int *maxx, int *maxy, const struct Viewer *d)
{
  int i, num;
  struct FocusObj **focus;
  struct narray *abbox;
  int bboxnum, *bbox;
  N_VALUE *inst;
  struct savedstdio save;

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
    if (inst == NULL) {
      continue;
    }

    _exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
    _getobj(focus[i]->obj, "bbox", inst, &abbox);

    bboxnum = arraynum(abbox);
    bbox = arraydata(abbox);

    if (bboxnum < 4) {
      continue;
    }

    if (bbox[0] < *minx) {
      *minx = bbox[0];
    }
    if (bbox[1] < *miny) {
      *miny = bbox[1];
    }
    if (bbox[2] > *maxx) {
      *maxx = bbox[2];
    }
    if (bbox[3] > *maxy) {
      *maxy = bbox[3];
    }
  }
  restorestdio(&save);
}

static int
coord_conv_x(int x, double zoom, const struct Viewer *d)
{
  return mxd2p(x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx + CAIRO_COORDINATE_OFFSET;
}

static int
coord_conv_y(int y, double zoom, const struct Viewer *d)
{
  return mxd2p(y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy + CAIRO_COORDINATE_OFFSET;
}

static void
GetFocusFrame(int *minx, int *miny, int *maxx, int *maxy, int ofsx, int ofsy, const struct Viewer *d)
{
  int x1, y1, x2, y2;
  double zoom;

  GetLargeFrame(&x1, &y1, &x2, &y2, d);

  zoom = Menulocal.PaperZoom / 10000.0;

  *minx = coord_conv_x((x1 + ofsx), zoom, d);
  *miny = coord_conv_y((y1 + ofsy), zoom, d);

  *maxx = coord_conv_x((x2 + ofsx), zoom, d);
  *maxy = coord_conv_y((y2 + ofsy), zoom, d);
}

static void
ShowFocusFrame(cairo_t *cr, const struct Viewer *d)
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
  int minx, miny, height, width;

  ignorestdio(&save);

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  if (Menulocal.focus_frame_type ==  N_LINE_TYPE_SOLID) {
    cairo_set_dash(cr, NULL, 0, 0);
  } else {
    double dash[] = {DOT_LENGTH};

    cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
  }
  //  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);

  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);

  if (num > 0) {
    GetFocusFrame(&x1, &y1, &x2, &y2, d->FrameOfsX, d->FrameOfsY, d);

    x1 -= FOCUS_FRAME_OFST;
    y1 -= FOCUS_FRAME_OFST;
    x2 += FOCUS_FRAME_OFST - 1;
    y2 += FOCUS_FRAME_OFST - 1;

    minx = (x1 < x2) ? x1 : x2;
    miny = (y1 < y2) ? y1 : y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);

    cairo_rectangle(cr, minx, miny, width, height);
    cairo_stroke(cr);

    cairo_rectangle(cr,
		    x1 - FOCUS_RECT_SIZE,
		    y1 - FOCUS_RECT_SIZE,
		    FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    cairo_rectangle(cr,
		    x1 - FOCUS_RECT_SIZE,
		    y2,
		    FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    cairo_rectangle(cr,
		    x2,
		    y1 - FOCUS_RECT_SIZE,
		    FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    cairo_rectangle(cr,
		    x2,
		    y2,
		    FOCUS_RECT_SIZE, FOCUS_RECT_SIZE);
    cairo_fill(cr);
  }
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num > 1) {
    for (i = 0; i < num; i++) {
      inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
      if (inst == NULL) {
	continue;
      }
      _exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
      _getobj(focus[i]->obj, "bbox", inst, &abbox);

      bboxnum = arraynum(abbox);
      bbox = arraydata(abbox);

      if (bboxnum < 4) {
	continue;
      }
      x1 = coord_conv_x((bbox[0] + d->FrameOfsX), zoom, d);
      y1 = coord_conv_y((bbox[1] + d->FrameOfsY), zoom, d);
      x2 = coord_conv_x((bbox[2] + d->FrameOfsX), zoom, d);
      y2 = coord_conv_y((bbox[3] + d->FrameOfsY), zoom, d);

      minx = (x1 < x2) ? x1 : x2;
      miny = (y1 < y2) ? y1 : y2;

      width = abs(x2 - x1);
      height = abs(y2 - y1);

      cairo_rectangle(cr, minx, miny, width, height);
      cairo_stroke(cr);
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

	cairo_rectangle(cr,
			x1 - FOCUS_RECT_SIZE / 2 - CAIRO_COORDINATE_OFFSET,
			y1 - FOCUS_RECT_SIZE / 2 - CAIRO_COORDINATE_OFFSET,
			FOCUS_RECT_SIZE,
			FOCUS_RECT_SIZE);
      }
      cairo_fill(cr);
    }
  }

  restorestdio(&save);
}

int
get_focused_obj_array(struct narray *focusobj, char **objs)
{
  int i, j, obj_n, n, axis;
  struct objlist *obj, *obj_array[OBJ_MAX] = {NULL};
  struct FocusObj **focus;

  n = arraynum(focusobj);
  focus = arraydata(focusobj);
  axis = FALSE;
  obj_n = 0;
  for (i = 0; i < n; i++) {
    obj = focus[i]->obj;
    if (! axis && strcmp(obj->name, "axis") == 0) {
      axis = TRUE;
    }
    for (j = 0; j < obj_n; j++) {
      if (obj_array[j] == obj) {
	break;
      }
    }
    if (j == obj_n) {
      obj_array[obj_n] = obj;
      obj_n++;
    }
  }
  for (i = 0; i < obj_n; i++) {
    objs[i] = obj_array[i]->name;
  }
  if (axis) {
    objs[i] = "axisgrid";
    i++;
    objs[i] = "data";
    i++;
  }
  objs[i] = NULL;
  return i;
}

static void
AlignFocusedObj(int align)
{
  int i, num, bboxnum, *bbox, minx, miny, maxx, maxy, dx, dy;
  struct FocusObj **focus;
  struct narray *abbox;
  char *argv[4], *objs[OBJ_MAX];
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;

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
    GetLargeFrame(&minx, &miny, &maxx, &maxy, d);
  }

  if (maxx < minx || maxy < miny)
    return;

  d->allclear = FALSE;
  PaintLock = TRUE;
  get_focused_obj_array(d->focusobj, objs);
  menu_save_undo(UNDO_TYPE_ALIGN, objs);
  for (i = 0; i < num; i++) {
    N_VALUE *inst;
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
    case VIEW_ALIGN_HCENTER:
      dx = (maxx + minx - bbox[2] - bbox[0]) / 2;
      break;
    case VIEW_ALIGN_RIGHT:
      dx = maxx - bbox[2];
      break;
    case VIEW_ALIGN_TOP:
      dy = miny - bbox[1];
      break;
    case VIEW_ALIGN_VCENTER:
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

    _exeobj(focus[i]->obj, "move", inst, 2, argv);
    set_graph_modified();
  }
  PaintLock = FALSE;
  UpdateAll(objs);
}

static void
execute_selected_instances(struct FocusObj **focus, int num, int argc, char **argv, char *field)
{
  int i;

  for (i = 0; i < num; i++) {
    N_VALUE *inst;
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst == NULL) {
      continue;
    }
    if (chkobjfield(focus[i]->obj, field) == 0) {
      _exeobj(focus[i]->obj, field, inst, argc, argv);
      set_graph_modified();
    }
  }
}

static void
RotateFocusedObj(int direction)
{
  int num, minx, miny, maxx, maxy, angle, type;
  int use_pivot, px, py;
  struct FocusObj **focus;
  char *argv[5], *objs[OBJ_MAX];
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;

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
    GetLargeFrame(&minx, &miny, &maxx, &maxy, d);
    use_pivot = 1;
    px = (minx + maxx) / 2;
    py = (miny + maxy) / 2;
  }
  get_focused_obj_array(d->focusobj, objs);
  menu_save_undo(UNDO_TYPE_ROTATE, objs);
  execute_selected_instances(focus, num, 4, argv, "rotate");

  PaintLock = FALSE;
  UpdateAll(objs);
}

static void
FlipFocusedObj(enum FLIP_DIRECTION dir)
{
  int num, minx, miny, maxx, maxy, type;
  int use_pivot, p;
  struct FocusObj **focus;
  char *argv[4], *objs[OBJ_MAX];
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;

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
    GetLargeFrame(&minx, &miny, &maxx, &maxy, d);
    use_pivot = 1;
    p = (dir == FLIP_DIRECTION_HORIZONTAL) ? (minx + maxx) / 2 : (miny + maxy) / 2;
  }
  get_focused_obj_array(d->focusobj, objs);
  menu_save_undo(UNDO_TYPE_FLIP, objs);
  execute_selected_instances(focus, num, 3, argv, "flip");

  PaintLock = FALSE;
  UpdateAll(objs);
}

static int
check_angle(int a)
{
  if (a < 0) {
    a %= 36000;
    if (a < 0) {
      a += 36000;
    }
  } else if (a > 36000) {
    a %= 36000;
  }

  return a;
}

static void
draw_cairo_arc(cairo_t *cr, int x, int y, int rx, int ry, int a1, int a2)
{
  double da1, da2;

  if (rx < 1 || ry < 1) {
    return;
  }

  cairo_save(cr);
  cairo_translate(cr, x, y);
  cairo_scale(cr, rx, ry);
  da1 = check_angle(a1) * (M_PI / 18000.0);
  da2 = check_angle(a1 + a2) * (M_PI / 18000.0);
  cairo_arc_negative(cr, 0.0, 0.0, 1.0, -da1, -da2);
  cairo_restore(cr);
}

static void
show_focus_line_arc(cairo_t *cr, int change, double zoom, struct objlist *obj, N_VALUE *inst, struct Viewer *d)
{
  int x, y, rx, ry, pie_slice, fill, a1, a2, close_path;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a1);
  _getobj(obj, "angle2", inst, &a2);
  _getobj(obj, "close_path", inst, &close_path);
  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "pieslice", inst, &pie_slice);

  close_path = (close_path || fill);

  switch (change) {
  case ARC_POINT_TYPE_R:
    ry -= d->LineY;
    rx -= d->LineX;
    break;
  case ARC_POINT_TYPE_ANGLE1:
  case ARC_POINT_TYPE_ANGLE2:
    if (arc_get_angle(obj, inst, d->KeyMask, change, d->MouseX2, d->MouseY2, &a1, &a2)) {
      return;
    }
    d->Angle = (change == ARC_POINT_TYPE_ANGLE1) ? a1 : (a1 + a2) % 36000;
    break;
  }

  if (rx > 0 && ry > 0) {
    rx = mxd2p(rx * zoom);
    ry = mxd2p(ry * zoom);
    x = coord_conv_x(x, zoom, d);
    y = coord_conv_y(y, zoom, d);
    draw_cairo_arc(cr, x, y, rx, ry, a1, a2);
    if (close_path) {
      if (pie_slice) {
	cairo_line_to(cr, x, y);
      }
      cairo_close_path(cr);
    }
    cairo_stroke(cr);
  }
}

static void
draw_frame_rect(cairo_t *gc, int change, double zoom, int *bbox, const struct Viewer *d)
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

  cairo_rectangle(gc, minx, miny, width, height);
  cairo_stroke(gc);
}

static void
draw_focus_line(cairo_t *gc, int change, double zoom, int bboxnum, int *bbox, int close_path, const struct Viewer *d)
{
  int j, ofsx, ofsy;

  for (j = 4; j < bboxnum; j += 2) {
    int x1, y1;
    if (change == (j - 4) / 2) {
      ofsx = d->LineX;
      ofsy = d->LineY;
    } else {
      ofsx = 0;
      ofsy = 0;
    }

    x1 = coord_conv_x(bbox[j] + ofsx, zoom, d);
    y1 = coord_conv_y(bbox[j + 1] + ofsy, zoom, d);
    if (j == 4) {
      cairo_move_to(gc, x1, y1);
    } else {
      cairo_line_to(gc, x1, y1);
    }
  }

  if (close_path) {
    cairo_close_path(gc);
  }
  cairo_stroke(gc);
}

static void
ShowFocusLine(cairo_t *cr, struct Viewer *d)
{
  int num;
  struct FocusObj **focus;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  N_VALUE *inst;
  struct savedstdio save;
  double zoom;
  char *group;
  double dash[] = {DOT_LENGTH};

  ignorestdio(&save);

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
  //  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);

  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num != 1) {
    goto End;
  }

  inst = chkobjinstoid(focus[0]->obj, focus[0]->oid);
  if (inst == NULL) {
    goto End;
  }

  _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
  _getobj(focus[0]->obj, "bbox", inst, &abbox);

  bboxnum = arraynum(abbox);
  bbox = arraydata(abbox);

  if (focus[0]->obj == chkobject("rectangle")) {
    draw_frame_rect(cr, d->ChangePoint, zoom, bbox, d);
  } else if (focus[0]->obj == chkobject("arc")) {
    show_focus_line_arc(cr, d->ChangePoint, zoom, focus[0]->obj, inst, d);
  } else if (focus[0]->obj == chkobject("path")) {
    int close_path, fill;

    _getobj(focus[0]->obj, "close_path", inst, &close_path);
    _getobj(focus[0]->obj, "fill", inst, &fill);
    close_path = (close_path || fill);
    draw_focus_line(cr, d->ChangePoint, zoom, bboxnum, bbox, close_path, d);
  } else if (focus[0]->obj == chkobject("axis")) {
    _getobj(focus[0]->obj, "group", inst, &group);
    if (group && group[0] != 'a') {
      draw_frame_rect(cr, d->ChangePoint, zoom, bbox, d);
    } else {
      draw_focus_line(cr, d->ChangePoint, zoom, bboxnum, bbox, FALSE, d);
    }
  }

 End:
  //  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  restorestdio(&save);
}

static void
ShowPoints(cairo_t *cr, const struct Viewer *d)
{
  int num, x1, y1;
  struct Point **po;
  double zoom;
  double dash[] = {DOT_LENGTH};

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  //  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);

  num = arraynum(d->points);
  po = arraydata(d->points);

  zoom = Menulocal.PaperZoom / 10000.0;

  if (d->Mode & POINT_TYPE_DRAW1) {
    if (num >= 2) {
      int x2, y2;
      int minx, miny, height, width;
      cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);

      x1 = coord_conv_x(po[0]->x, zoom, d);
      y1 = coord_conv_y(po[0]->y, zoom, d);
      x2 = coord_conv_x(po[1]->x, zoom, d);
      y2 = coord_conv_y(po[1]->y, zoom, d);

      minx = (x1 < x2) ? x1 : x2;
      miny = (y1 < y2) ? y1 : y2;

      width = abs(x2 - x1);
      height = abs(y2 - y1);

      if (d->Mode == ArcB) {
	draw_cairo_arc(cr, minx + width / 2, miny + height / 2, width / 2, height / 2, 0, 36000);
      } else {
	cairo_rectangle(cr, minx, miny, width, height);
      }
      cairo_stroke(cr);
    }
  } else {
    int i;
    cairo_set_dash(cr, NULL, 0, 0);
    for (i = 0; i < num; i++) {
      x1 = coord_conv_x(po[i]->x, zoom, d);
      y1 = coord_conv_y(po[i]->y, zoom, d);

      cairo_move_to(cr, x1 - (POINT_LENGTH - 1), y1);
      cairo_line_to(cr, x1 + POINT_LENGTH, y1);

      cairo_move_to(cr, x1, y1 - (POINT_LENGTH - 1));
      cairo_line_to(cr, x1, y1 + POINT_LENGTH);
    }
    cairo_stroke(cr);

    if (num >= 1) {
      cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
      x1 = coord_conv_x(po[0]->x, zoom, d);
      y1 = coord_conv_y(po[0]->y, zoom, d);
      cairo_move_to(cr, x1, y1);
      for (i = 1; i < num; i++) {
	x1 = coord_conv_x(po[i]->x, zoom, d);
	y1 = coord_conv_y(po[i]->y, zoom, d);

	cairo_line_to(cr, x1, y1);
      }
      cairo_stroke(cr);
    }
  }
  //  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

static void
ShowFrameRect(cairo_t *cr, const struct Viewer *d)
{
  int x1, y1, x2, y2;
  double zoom;
  int minx, miny, width, height;
  double dash[] = {DOT_LENGTH};

  if (d->MouseX1 == d->MouseX2 && d->MouseY1 == d->MouseY2) {
    return;
  }

  zoom = Menulocal.PaperZoom / 10000.0;

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
  //  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);

  x1 = coord_conv_x(d->MouseX1, zoom, d);
  y1 = coord_conv_y(d->MouseY1, zoom, d);

  x2 = coord_conv_x(d->MouseX2, zoom, d);
  y2 = coord_conv_y(d->MouseY2, zoom, d);

  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;

  width = abs(x2 - x1);
  height = abs(y2 - y1);

  cairo_rectangle(cr, minx, miny, width, height);
  cairo_stroke(cr);

  //  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

static void
ShowCrossGauge(cairo_t *cr, const struct Viewer *d)
{
  int x, y, width, height;
  double zoom;
  GdkWindow *win;

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, NULL, 0, 0);
  //  cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  width = gdk_window_get_width(win);
  height = gdk_window_get_height(win);

  zoom = Menulocal.PaperZoom / 10000.0;

  x = coord_conv_x(d->CrossX, zoom, d);
  y = coord_conv_y(d->CrossY, zoom, d);

  cairo_move_to(cr, x, 0);
  cairo_line_to(cr, x, height);

  cairo_move_to(cr, 0, y);
  cairo_line_to(cr, width, y);
  cairo_stroke(cr);

  //  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

static void
CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom_x, double *zoom_y)
{
  int offset;
  int grid;

  if ((state & GDK_CONTROL_MASK) && ! ofs && x != NULL && y != NULL) {
    if (abs(*x) > abs(*y)) {
      *y = 0;
    } else {
      *x = 0;
    }
  }

  grid = Menulocal.grid;
  if (state & GDK_SHIFT_MASK) {
    return;
  }

  if (ofs) {
    offset = grid / 2;
  } else {
    offset = 0;
  }

  if (x != NULL) {
    *x = ((*x + offset) / grid) * grid;
  }

  if (y != NULL) {
    *y = ((*y + offset) / grid) * grid;
  }

  if (zoom_x != NULL) {
    *zoom_x = nround(*zoom_x * grid) / ((double) grid);
  }
  if (zoom_y != NULL) {
    *zoom_y = nround(*zoom_y * grid) / ((double) grid);
  }
}

static void
mouse_down_point(unsigned int state, TPoint *point, struct Viewer *d)
{
  d->Capture = TRUE;

  if (arraynum(d->focusobj) && ! (state & GDK_SHIFT_MASK)) {
    d->ShowFrame = FALSE;
    clear_focus_obj(d);
  }

  d->MouseMode = MOUSEPOINT;
  d->ShowRect = TRUE;
}

#define ZOOM_RATIO_LIMIT 0.5

static void
calc_zoom(struct Viewer *d, int vx1, int vy1, int *x2, int *y2, double *zoom_x, double *zoom_y, int preserve_ratio)
{
  int vx2, vy2;
  double cc, nn, zoom2, zmx, zmy, zmr;

  vx1 -= d->RefX1 - d->MouseDX;
  vy1 -= d->RefY1 - d->MouseDY;

  vx2 = (d->RefX2 - d->RefX1);
  vy2 = (d->RefY2 - d->RefY1);

  cc = 1.0 * vx1 * vx2 + 1.0 * vy1 * vy2;
  nn = 1.0 * vx2 * vx2 + 1.0 * vy2 * vy2;

  if (x2) {
    *x2 = vx2;
  }

  if (y2) {
    *y2 = vy2;
  }
  if ((nn == 0) || (cc < 0)) {
    zoom2 = 0;
  } else {
    zoom2 = cc / nn;
  }
  if (vx2 * vx1 <= 0) {
    zmx = 0;
  } else {
    zmx = 1.0 * vx1 / vx2;
  }
  if (vy2 * vy1 <= 0) {
    zmy = 0;
  } else {
    zmy = 1.0 * vy1 / vy2;
  }
  if (zmx > 0) {
    zmr = zmy / zmx;
  } else {
    zmr = 2;
  }
  if (zoom_x && zoom_y) {
    if (preserve_ratio) {
      if (zmr > 1 + ZOOM_RATIO_LIMIT) {
	*zoom_x = 1;
	*zoom_y = zmy;
      } else if (zmr < ZOOM_RATIO_LIMIT) {
	*zoom_x = zmx;
	*zoom_y = 1;
      } else {
	*zoom_x = zoom2;
	*zoom_y = zoom2;
      }
    } else {
      *zoom_x = zmx;
      *zoom_y = zmy;
    }
  }
}

static void
set_zoom_prm(struct Viewer *d, int vx2, int vy2, double zoom_x, double zoom_y)
{
  int vx1, vy1;

  vx1 = d->RefX1 + vx2 * zoom_x;
  vy1 = d->RefY1 + vy2 * zoom_y;

  d->MouseX1 = d->RefX1;
  d->MouseY1 = d->RefY1;

  d->MouseX2 = vx1;
  d->MouseY2 = vy1;
}

static void
init_zoom(unsigned int state, struct Viewer *d)
{
  int vx1, vy1, vx2, vy2, preserve_ratio;
  double zoom_x, zoom_y;

  d->ShowFrame = FALSE;
  d->MouseDX = d->RefX2 - d->MouseX1;
  d->MouseDY = d->RefY2 - d->MouseY1;

  vx1 = d->MouseX1;
  vy1 = d->MouseY1;

  preserve_ratio = (state & GDK_CONTROL_MASK);
  calc_zoom(d, vx1, vy1, &vx2, &vy2, &zoom_x, &zoom_y, preserve_ratio);

  CheckGrid(FALSE, state, NULL, NULL, &zoom_x, &zoom_y);

  set_zoom_prm(d, vx2, vy2, zoom_x, zoom_y);

  d->ShowRect = TRUE;
}

static void
mouse_down_move(unsigned int state, TPoint *point, struct Viewer *d)
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
    GetLargeFrame(&(d->RefX2), &(d->RefY2), &(d->RefX1), &(d->RefY1), d);
    d->MouseMode = MOUSEZOOM1;
    NSetCursor(cursor);
    init_zoom(state, d);
    break;
  case GDK_TOP_RIGHT_CORNER:
    GetLargeFrame(&(d->RefX1), &(d->RefY2), &(d->RefX2), &(d->RefY1), d);
    d->MouseMode = MOUSEZOOM2;
    NSetCursor(cursor);
    init_zoom(state, d);
    break;
  case GDK_BOTTOM_RIGHT_CORNER:
    GetLargeFrame(&(d->RefX1), &(d->RefY1), &(d->RefX2), &(d->RefY2), d);
    d->MouseMode = MOUSEZOOM3;
    NSetCursor(cursor);
    init_zoom(state, d);
    break;
  case GDK_BOTTOM_LEFT_CORNER:
    GetLargeFrame(&(d->RefX2), &(d->RefY1), &(d->RefX1), &(d->RefY2), d);
    d->MouseMode = MOUSEZOOM4;
    NSetCursor(cursor);
    init_zoom(state, d);
    break;
  case GDK_CROSSHAIR:
    d->MouseMode = MOUSECHANGE;
    d->Angle = -1;
    d->ShowFrame = FALSE;
    d->ShowLine = TRUE;
    d->LineX = d->LineY = 0;
    NSetCursor(cursor);
    break;
  case GDK_FLEUR:
    d->MouseMode = MOUSEDRAG;
    break;
  }
}

static void
mouse_down_move_data(struct Viewer *d)
{
  struct objlist *fileobj, *aobjx, *aobjy;
  struct narray iarray, *move, *movex, *movey;
  int selnum, sel, i, ax, ay, anum, iline, j, movenum;
  double dx, dy;
  char *axis, *argv[3];
  int *ptr;

  fileobj = chkobject("data");
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

    if (ax == -1 || ay == -1)
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

  if (selnum > 0) {
    message_box(NULL, _("Data points are moved."), "Confirm", RESPONS_OK);
  }
  argv[0] = "data";
  argv[1] = NULL;
  UpdateAll(argv);

 ErrEnd:
  move_data_cancel(d, FALSE);
}

#define VIEWER_DPI_MAX 620
#define VIEWER_DPI_MIN  20

#if ! GTK_CHECK_VERSION(3, 22, 0)
#define ANIM_DIV 1
static void
show_zoom_animation(struct Viewer *d, TPoint *point, double zoom)
{
  cairo_pattern_t *pattern;
  cairo_matrix_t matrix;
  cairo_t *cr;
  int i;
  double inc, z;
  GdkWindow *win;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  cr = gdk_cairo_create(win);
  inc = (zoom - 1) / ANIM_DIV;

  pattern = cairo_pattern_create_for_surface(Menulocal.pix);
  cairo_set_source(cr, pattern);
  for (i = 1; i <= ANIM_DIV; i++) {
    z = 1 + inc * i;
    cairo_matrix_init(&matrix,
		      z, 0,
		      0, z,
		      (point->x + d->hscroll - d->cx) - point->x * z,
		      (point->y + d->vscroll - d->cy) - point->y * z);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_paint(cr);
  }

  cairo_pattern_destroy(pattern);
  cairo_destroy(cr);
}
#endif

#ifdef SHOW_MOVE_ANIMATION
static void
show_move_animation(struct Viewer *d, int x, int y)
{
  cairo_pattern_t *pattern;
  cairo_matrix_t matrix;
  cairo_t *cr;
  int i, n;
  double incx, incy;
  GdkWindow *win;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  if (abs(x) < 1 && abs(y) < 1) {
    return;
  }

  cr = gdk_cairo_create(win);

  n = abs(x) / 4;
  if (abs(y) / 4 > n) {
    n = abs(y) / 4;
  }

  if (n > ANIM_DIV) {
    n = ANIM_DIV;
  }

  incx = 1.0 * x / n;
  incy = 1.0 * y / n;

  pattern = cairo_pattern_create_for_surface(Menulocal.pix);
  cairo_set_source(cr, pattern);
  for (i = 1; i <= n; i++) {
    cairo_matrix_init(&matrix,
		      1, 0,
		      0, 1,
		      (d->hscroll - d->cx) + incx * i,
		      (d->vscroll - d->cy) + incy * i);
    cairo_pattern_set_matrix(pattern, &matrix);
    cairo_paint(cr);
  }

  cairo_pattern_destroy(pattern);
  cairo_destroy(cr);
}
#endif	/* SHOW_MOVE_ANIMATION */

static void
mouse_down_zoom2(unsigned int state, TPoint *point, struct Viewer *d, int zoom_out, double factor)
{
  static double saved_dpi_d = -1;
  static double saved_dpi_i = -1;
  double dpi, ratio;
  int vdpi;

  if (ZoomLock) {
    return;
  }

  ZoomLock = TRUE;
  if (state & GDK_SHIFT_MASK) {
#ifdef SHOW_MOVE_ANIMATION
    show_move_animation(d, point->x - d->cx , point->y - d->cy);
#endif	/* SHOW_MOVE_ANIMATION */
    d->hscroll -= (d->cx - point->x);
    d->vscroll -= (d->cy - point->y);

    ChangeDPI();
    goto End;
  }

  if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &vdpi) == -1) {
    goto End;
  }

  if (saved_dpi_i < 0 || saved_dpi_i != vdpi) {
    saved_dpi_i = vdpi;
    saved_dpi_d = vdpi;
  }

  dpi = (zoom_out) ? saved_dpi_d / factor : saved_dpi_d * factor;
  if (dpi < VIEWER_DPI_MIN) {
    saved_dpi_i = VIEWER_DPI_MIN;
    message_beep(TopLevel);
  } else if (dpi > VIEWER_DPI_MAX) {
    saved_dpi_i = VIEWER_DPI_MAX;
    message_beep(TopLevel);
  } else {
    saved_dpi_d = dpi;
    saved_dpi_i = nround(dpi);
  }

  ratio = vdpi / saved_dpi_i;
#if ! GTK_CHECK_VERSION(3, 22, 0)
  if (vdpi != nround(saved_dpi_i)) {
    show_zoom_animation(d, point, ratio);
  }
#endif

  vdpi = saved_dpi_i;

  if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
    d->hscroll -= (d->cx - point->x) * (1 - ratio);
    d->vscroll -= (d->cy - point->y) * (1 - ratio);
    ChangeDPI();
  }

 End:
  ZoomLock = FALSE;
}

static void
mouse_down_zoom(unsigned int state, TPoint *point, struct Viewer *d, int zoom_out)
{
  mouse_down_zoom2(state, point, d, zoom_out, ZOOM_SPEED_NORMAL);
}

static void
mouse_down_zoom_little(unsigned int state, TPoint *point, struct Viewer *d, int zoom_out)
{
  mouse_down_zoom2(state, point, d, zoom_out, ZOOM_SPEED_LITTLE);
}

static void
mouse_down_set_points(unsigned int state, struct Viewer *d, int n)
{
  int x1, y1, i;
  struct Point *po;

  if (d->Capture)
    return;

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL, NULL);

  for (i = 0; i < n; i++) {
    po = (struct Point *) g_malloc(sizeof(struct Point));
    if (po) {
      po->x = x1;
      po->y = y1;
      arrayadd(d->points, &po);
    }
  }

  d->Capture = TRUE;
}

static gboolean
ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  double zoom;
  int pos;

  if (Menulock || Globallock)
    return FALSE;

  if (region) {
    return FALSE;
  }

  zoom = Menulocal.PaperZoom / 10000.0;

  d->MouseX1 = d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY1 = d->MouseY2 = calc_mouse_y(point->y, zoom, d);

  d->MouseMode = MOUSENONE;

  if (d->MoveData) {
    menu_save_undo_single(UNDO_TYPE_ORDER, "data");
    mouse_down_move_data(d);
    return TRUE;
  }

  switch (d->Mode) {
  case PointB:
  case LegendB:
  case AxisB:
    pos = NGetCursor();
    if (pos == GDK_LEFT_PTR) {
      if (state & GDK_CONTROL_MASK) {
	NSetCursor(GDK_FLEUR);
	d->MouseMode = MOUSESCROLLE;
	return TRUE;
      } else {
	mouse_down_point(state, point, d);
      }
    } else {
      mouse_down_move(state, point, d);
    }
    break;
  case TrimB:
  case DataB:
  case EvalB:
    if (state & GDK_CONTROL_MASK) {
      NSetCursor(GDK_FLEUR);
      d->MouseMode = MOUSESCROLLE;
      return TRUE;
    } else {
      d->Capture = TRUE;
      d->MouseMode = MOUSEPOINT;
      d->ShowRect = TRUE;
    }
    break;
  case MarkB:
  case TextB:
    mouse_down_set_points(state, d, 1);
    break;
  case ZoomB:
    mouse_down_zoom(state, point, d, state & GDK_CONTROL_MASK);
    break;
  default:
    mouse_down_set_points(state, d, 2);
    break;
  }

  gtk_widget_queue_draw(d->Win);

  return TRUE;
}

static void
mouse_up_point(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x1, x2, y1, y2, err;

  d->Capture = FALSE;
  d->ShowRect = FALSE;

  d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY2 = calc_mouse_y(point->y, zoom, d);

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  x2 = d->MouseX2;
  y2 = d->MouseY2;

  err = mxp2d(POINT_ERROR) / zoom;

  switch (d->Mode) {
  case PointB:
  case AxisB:
    Match("axis", x1, y1, x2, y2, err, d);
    /* fall-through */
  case LegendB:
    if (d->Mode != AxisB) {
      Match("legend", x1, y1, x2, y2, err, d);
      Match("merge", x1, y1, x2, y2, err, d);
    }
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    break;
  case TrimB:
    Trimming(x1, y1, x2, y2, d);
    break;
  case DataB:
    if (ViewerWinFileUpdate(x1, y1, x2, y2, err)) {
      char *objects[] = {"data", NULL};
      UpdateAll(objects);
    }
    break;
  case EvalB:
    Evaluate(x1, y1, x2, y2, err, d);
    break;
  default:
    /* never reached */
    break;
  }
}

static int
move_objects(int dx, int dy, struct Viewer *d, char **objs)
{
  int num, axis;
  struct FocusObj *focus;

  d->ShowFrame = FALSE;
  num = arraynum(d->focusobj);
  axis = FALSE;
  PaintLock = TRUE;
  if (dx != 0 || dy != 0) {
    char *argv[5];
    int i;
    menu_save_undo(UNDO_TYPE_MOVE, objs);
    argv[0] = (char *) &dx;
    argv[1] = (char *) &dy;
    argv[2] = NULL;
    for (i = num - 1; i >= 0; i--) {
      struct objlist *obj;
      N_VALUE *inst;
      focus = *(struct FocusObj **) arraynget(d->focusobj, i);
      obj = focus->obj;
      if (obj == chkobject("axis"))
	axis = TRUE;
      inst = chkobjinstoid(focus->obj, focus->oid);
      if (inst) {
	_exeobj(obj, "move", inst, 2, argv);
	set_graph_modified();
      }
    }
  }
  PaintLock = FALSE;
  d->FrameOfsX = d->FrameOfsY = 0;
  d->ShowFrame = TRUE;
  return axis;
}

static void
add_data_grid_to_objs(char **objs)
{
  int i, axis, data, axisgrid;
  if (objs == NULL) {
    return;
  }
  i = 0;
  axis = FALSE;
  data = FALSE;
  axisgrid = FALSE;
  while (objs[i]) {
    if (strcmp(objs[i], "axis") == 0) {
      axis = TRUE;
    } else if (strcmp(objs[i], "data") == 0) {
      data = TRUE;
    } else if (strcmp(objs[i], "axisgrid") == 0) {
      axisgrid = TRUE;
    }
    i++;
  }
  if (axis) {
    if (! data) {
      objs[i] = "data";
      i++;
    }
    if (! axisgrid) {
      objs[i] = "axisgrid";
      i++;
    }
    objs[i] = NULL;
  }
}

static void
mouse_up_drag(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int dx, dy, axis;
  char *objs[OBJ_MAX];

  if (d->MouseX1 == d->MouseX2 && d->MouseY1 == d->MouseY2) {
    return;
  }

  d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY2 = calc_mouse_y(point->y, zoom, d);

  dx = d->MouseX2 - d->MouseX1;
  dy = d->MouseY2 - d->MouseY1;
  CheckGrid(FALSE, state, &dx, &dy, NULL, NULL);
  get_focused_obj_array(d->focusobj, objs);
  axis = move_objects(dx, dy, d, objs);
  if (d->Mode == LegendB || (d->Mode == PointB && !axis)) {
    d->allclear=FALSE;
  }
  if (axis) {
    add_data_grid_to_objs(objs);
  }
  UpdateAll(objs);
}

static int
check_zoom(double zoom)
{
  int zm;
  if (zoom * 10000 > G_MAXINT) {
    return -1;
  }
  zm = nround(zoom * 10000);
  if (zm < 1000) {
    zm = 1000;
  }
  return zm;
}

static void
mouse_up_zoom(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int vx1, vy1, zmx, zmy, axis, preserve_ratio;
  double zoom_x, zoom_y;
  char *objs[OBJ_MAX];
  struct FocusObj *focus;

  axis = FALSE;

  d->ShowRect = FALSE;

  vx1 = calc_mouse_x(point->x, zoom, d);
  vy1 = calc_mouse_y(point->y, zoom, d);

  d->MouseX2 = vx1;
  d->MouseY2 = vy1;

  preserve_ratio = (state & GDK_CONTROL_MASK);
  calc_zoom(d, vx1, vy1, NULL, NULL, &zoom_x, &zoom_y, preserve_ratio);

  if ((d->Mode != DataB) && (d->Mode != EvalB)) {
    CheckGrid(FALSE, state, NULL, NULL, &zoom_x, &zoom_y);
  }

  zmx = check_zoom(zoom_x);
  zmy = check_zoom(zoom_y);
  if (zmx < 0 || zmy < 0) {
    return;
  }

  objs[0] = NULL;
  if (zmx != 10000 || zmy != 10000) {
    int i, num;
    char *argv[6];
    argv[0] = (char *) &zmx;
    argv[1] = (char *) &zmy;
    argv[2] = (char *) &(d->RefX1);
    argv[3] = (char *) &(d->RefY1);
    argv[4] = (char *) &Menulocal.preserve_width;
    argv[5] = NULL;

    num = arraynum(d->focusobj);
    PaintLock = TRUE;

    if (num > 0) {
      get_focused_obj_array(d->focusobj, objs);
      menu_save_undo(UNDO_TYPE_ZOOM, objs);
    }
    for (i = num - 1; i >= 0; i--) {
      N_VALUE *inst;
      struct objlist *obj;
      focus = *(struct FocusObj **) arraynget(d->focusobj, i);
      obj = focus->obj;

      if (obj == chkobject("axis")) {
	axis = TRUE;
      }

      inst = chkobjinstoid(focus->obj, focus->oid);
      if (inst) {
	_exeobj(obj, "zooming", inst, 5, argv);
	set_graph_modified();
      }
    }
  }

  PaintLock = FALSE;

  d->FrameOfsX = d->FrameOfsY = 0;
  d->ShowFrame = TRUE;

  if (d->Mode == LegendB || (d->Mode == PointB && !axis)) {
    d->allclear = FALSE;
  }

  UpdateAll(objs);
}

static void
mouse_up_change(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int dx, dy, axis;
  struct FocusObj *focus;
  struct objlist *obj;

  axis = FALSE;

  d->ShowLine = FALSE;
  obj = NULL;

  if ((d->MouseX1 != d->MouseX2) || (d->MouseY1 != d->MouseY2)) {
    char *argv[5];
    d->MouseX2 = calc_mouse_x(point->x, zoom, d);
    d->MouseY2 = calc_mouse_y(point->y, zoom, d);

    dx = d->MouseX2 - d->MouseX1;
    dy = d->MouseY2 - d->MouseY1;

    if ((d->Mode != DataB) && (d->Mode != EvalB)) {
      CheckGrid(FALSE, state, &dx, &dy, NULL, NULL);
    }

    if (dx != 0 || dy != 0) {
      N_VALUE *inst;
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
	menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
	_exeobj(obj, "change", inst, 3, argv);
	set_graph_modified();
      }

      PaintLock = FALSE;
    }
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
    if (d->Mode == LegendB || (d->Mode == PointB && !axis)) {
      d->allclear = FALSE;
    }
    argv[0] = (obj) ? obj->name : NULL;
    argv[1] = NULL;
    if (axis) {
      argv[1] = "data";
      argv[2] = "axisgrid";
      argv[3] = NULL;
    }
    UpdateAll(argv);
  } else {
    d->FrameOfsX = d->FrameOfsY = 0;
    d->ShowFrame = TRUE;
  }
}

static void
mouse_up_lgend1(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x1, y1, num;

  d->Capture = FALSE;

  d->MouseX1 = calc_mouse_x(point->x, zoom, d);
  d->MouseY1 = calc_mouse_y(point->y, zoom, d);

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL, NULL);

  num = arraynum(d->points);
  if (num >= 1) {
    struct Point *po;
    po = *(struct Point **) arraynget(d->points, 0);
    po->x = x1;
    po->y = y1;
  }
  if (arraynum(d->points) == 1) {
    ViewerEvLButtonDblClk(state, point, d);
  }
}

static void
mouse_up_lgend2(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int num, x1, y1;
  struct Point *po;

  d->MouseX1 = calc_mouse_x(point->x, zoom, d);
  d->MouseY1 = calc_mouse_y(point->y, zoom, d);

  x1 = d->MouseX1;
  y1 = d->MouseY1;

  CheckGrid(TRUE, state, &x1, &y1, NULL, NULL);

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

  if ((d->Mode & POINT_TYPE_DRAW1) || d->Mode == SingleB) {
    if (arraynum(d->points) == 3) {
      d->Capture = FALSE;
      ViewerEvLButtonDblClk(state, point, d);
    }
  }
}

static void
set_drag_info(struct Viewer *d)
{
  char buf[32];

  snprintf(buf, sizeof(buf), "(% .2f, % .2f)", d->FrameOfsX / 100.0, d->FrameOfsY / 100.0);
  gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
}

#define CLEAR_DRAG_INFO 0
#if CLEAR_DRAG_INFO
static void
reset_drag_info(struct Viewer *d)
{
  gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), NULL);
}
#endif

static void
SetPoint(struct Viewer *d, int x, int y)
{
  //  x += Menulocal.LeftMargin;
  //  y += Menulocal.TopMargin;

  if (NgraphApp.Message && GTK_WIDGET_VISIBLE(NgraphApp.Message)) {
    char buf[128];
    struct Point *po;
    unsigned int num;
    snprintf(buf, sizeof(buf), "% 6.2f, % 6.2f", x / 100.0, y / 100.0);
    gtk_label_set_text(GTK_LABEL(NgraphApp.Message_pos), buf);

    switch (d->MouseMode) {
    case MOUSECHANGE:
      if (d->Angle >= 0) {
	snprintf(buf, sizeof(buf), "%6.2f°", d->Angle / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      } else {
	snprintf(buf, sizeof(buf), "(% .2f, % .2f)", d->LineX / 100.0, d->LineY / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      }
      break;
    case MOUSEZOOM1:
    case MOUSEZOOM2:
    case MOUSEZOOM3:
    case MOUSEZOOM4:
      snprintf(buf, sizeof(buf), "% .2f%%, % .2f%%", d->ZoomX * 100, d->ZoomY * 100);
      gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      break;
    case MOUSEDRAG:
      set_drag_info(d);
      break;
    default:
      num =  arraynum(d->points);
      po = (num > 1) ? (* (struct Point **) arraynget(d->points, num - 2)) : NULL;
      if (d->Capture && po) {
	snprintf(buf, sizeof(buf), "(% .2f, % .2f)", (x - po->x) / 100.0, (y - po->y) / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      } else {
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), NULL);
      }
    }
  }

  nruler_set_position(NgraphApp.Viewer.HRuler, N2GTK_RULER_METRIC(x));
  nruler_set_position(NgraphApp.Viewer.VRuler, N2GTK_RULER_METRIC(y));

  CoordWinSetCoord(x, y);
}

static gboolean
ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d)
{
  double zoom;

  if (Menulock || Globallock)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  if (d->MouseMode == MOUSESCROLLE) {
    NSetCursor(GDK_LEFT_PTR);
    d->MouseMode = MOUSENONE;
    return FALSE;
  }

  if (! d->Capture)
    return TRUE;

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
      mouse_up_drag(state, point, zoom, d);
      break;
    case MOUSEZOOM1:
    case MOUSEZOOM2:
    case MOUSEZOOM3:
    case MOUSEZOOM4:
      mouse_up_zoom(state, point, zoom, d);
      break;
    case MOUSECHANGE:
      mouse_up_change(state, point, zoom, d);
      break;
    case MOUSEPOINT:
      mouse_up_point(state, point, zoom, d);
#if 0
      if (d->Mode & POINT_TYPE_POINT) {
	d->allclear = FALSE;
	UpdateAll(NULL);
      } else {
	gtk_widget_queue_draw(d->Win);
      }
#else
      gtk_widget_queue_draw(d->Win);
#endif
      break;
    case MOUSENONE:
    case MOUSESCROLLE:
      break;
    }
    NSetCursor(get_mouse_cursor_type(d, point->x, point->y));
    d->MouseMode = MOUSENONE;
    SetPoint(d, d->MouseX2, d->MouseY2);
    break;
  case MarkB:
  case TextB:
    mouse_up_lgend1(state, point, zoom, d);
    break;
  default:
    mouse_up_lgend2(state, point, zoom, d);
  }

  set_focus_sensitivity(d);

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
create_legend1(struct Viewer *d)
{
  int num;
  struct objlist *obj = NULL;
  struct Point *po;
  char *objects[2];

  d->Capture = FALSE;
  num = arraynum(d->points);

  if (d->Mode == MarkB) {
    obj = chkobject("mark");
  } else {
    obj = chkobject("text");
  }

  if (obj) {
    int id, x1, y1, undo;
    undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
    id = newobj(obj);
    if (id >= 0) {
      int ret;
      N_VALUE *inst;
      presetting_set_obj_field(obj, id);
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
	menu_delete_undo(undo);
      } else {
	AddList(obj, inst);
	set_graph_modified();
      }
      PaintLock = FALSE;
    }
  }

  arraydel2(d->points);
  d->allclear = FALSE;
  objects[0] = obj->name;
  objects[1] = NULL;
  UpdateAll(objects);
}

static void
create_path(struct Viewer *d)
{
  struct objlist *obj = NULL;
  struct narray *parray;
  struct Point *po;
  N_VALUE *inst;
  int i, num, id, ret = IDCANCEL, undo;
  char *objects[2];

  d->Capture = FALSE;
  num = arraynum(d->points);
  obj = chkobject("path");

  if (num < 3 || obj == NULL) {
    goto ExitCreatePath;
  }

  undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
  id = newobj(obj);
  if (id < 0) {
    menu_delete_undo(undo);
    goto ExitCreatePath;
  }

  presetting_set_obj_field(obj, id);
  inst = chkobjinst(obj, id);
  parray = arraynew(sizeof(int));

  for (i = 0; i < num - 1; i++) {
    po = *(struct Point **) arraynget(d->points, i);
    arrayadd(parray, &po->x);
    arrayadd(parray, &po->y);
  }

  _putobj(obj, "points", inst, parray);
  PaintLock = TRUE;

  LegendArrowDialog(&DlgLegendArrow, obj, id);
  ret = DialogExecute(TopLevel, &DlgLegendArrow);

  if (ret == IDDELETE || ret == IDCANCEL) {
    menu_delete_undo(undo);
    delobj(obj, id);
  } else {
    AddList(obj, inst);
    set_graph_modified();
  }

  PaintLock = FALSE;

 ExitCreatePath:
  arraydel2(d->points);

  d->allclear = FALSE;
  objects[0] = obj->name;
  objects[1] = NULL;
  UpdateAll(objects);
}

static void
create_legend3(struct Viewer *d)
{
  int num, x1, y1, x2, y2;
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
      int id, undo;
      undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
      id = newobj(obj);
      if (id >= 0) {
        N_VALUE *inst;
        int ret = IDCANCEL;
        presetting_set_obj_field(obj, id);
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
	  menu_delete_undo(undo);
	} else {
	  AddList(obj, inst);
	  set_graph_modified();
	}
	PaintLock = FALSE;
      }
    }
  }

  arraydel2(d->points);
  if (obj) {
    char *objects[2];
    d->allclear = FALSE;
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}

static void
create_legendx(struct Viewer *d)
{
  int num, x1, y1, x2, y2, type, fill;
  struct objlist *obj = NULL;
  struct Point **pdata;

  d->Capture = FALSE;
  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("path");

    if (obj) {
      int id, undo;
      undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
      id = newobj(obj);

      if (id >= 0) {
        N_VALUE *inst;
        presetting_set_obj_field(obj, id);
	type = PATH_TYPE_CURVE;
	putobj(obj, "type", id, &type);
	fill = FALSE;
	putobj(obj, "fill", id, &fill);
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
          int ret;
	  LegendGaussDialog(&DlgLegendGauss, obj, id, x1, y1,
			    x2 - x1, y2 - y1);
	  ret = DialogExecute(TopLevel, &DlgLegendGauss);

	  if (ret != IDOK) {
	    delobj(obj, id);
	    menu_delete_undo(undo);
	  } else {
	    AddList(obj, inst);
	    set_graph_modified();
	  }
	}
	PaintLock = FALSE;
      }
    }
  }
  arraydel2(d->points);
  if (obj) {
    char *objects[2];
    d->allclear = FALSE;
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}

static void
create_single_axis(struct Viewer *d)
{
  int num;
  struct objlist *obj = NULL;
  struct Point **pdata;

  d->Capture = FALSE;
  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("axis");
    if (obj != NULL) {
      int id, x1, y1, lenx, dir, undo;
      undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
      if ((id = newobj(obj)) >= 0) {
        int x2, y2, ret;
        double fx1, fy1;
        N_VALUE *inst;

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

	presetting_set_obj_field(obj, id);
	AxisDialog(NgraphApp.AxisWin.data.data, id, TRUE);
	ret = DialogExecute(TopLevel, &DlgAxis);

	if (ret == IDCANCEL) {
	  menu_delete_undo(undo);
	  delobj(obj, id);
	} else {
	  AddList(obj, inst);
	  set_graph_modified();
	}
      }
    }
  }
  arraydel2(d->points);
  if (obj) {
    char *objects[2];
    d->allclear = TRUE;
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}

static void
create_axis(struct Viewer *d)
{
  int idx, idy, idu, idr, idg, oidx, oidy, type,
    num, x1, y1, lenx, leny;
  struct objlist *obj = NULL, *obj2;
  struct Point **pdata;
  struct narray group;

  d->Capture = FALSE;
  num = arraynum(d->points);
  pdata = arraydata(d->points);

  if (num >= 3) {
    obj = chkobject("axis");
    obj2 = chkobject("axisgrid");

    if (obj && obj2) {
      int undo, x2, y2, ret = IDCANCEL;
      char *argv[3];
      argv[0] = obj->name;
      argv[1] = obj2->name;
      argv[2] = NULL;
      undo = menu_save_undo(UNDO_TYPE_CREATE, argv);
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
          char *ref;
	  getobj(obj, "oid", idx, 0, NULL, &oidx);
	  ref = g_strdup_printf("axis:^%d", oidx);
	  if (ref) {
	    putobj(obj2, "axis_x", idg, ref);
	  }

	  getobj(obj, "oid", idy, 0, NULL, &oidy);
	  ref = g_strdup_printf("axis:^%d", oidy);
	  if (ref) {
	    putobj(obj2, "axis_y", idg, ref);
	  }
	}
      } else {
	idg = -1;
      }

      if (d->Mode == FrameB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	presetting_set_obj_field(obj, idu);
	presetting_set_obj_field(obj, idr);
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, &idg, FALSE);

	ret = DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == SectionB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	presetting_set_obj_field(obj, idu);
	presetting_set_obj_field(obj, idr);
	presetting_set_obj_field(obj2, idg);
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, &idg, TRUE);

	ret = DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == CrossB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	ret = DialogExecute(TopLevel, &DlgCross);
      }

      if (ret == IDCANCEL) {
	menu_undo_internal(undo);
      } else {
        N_VALUE *inst;
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
      d->allclear = TRUE;
      argv[0] = obj->name;
      argv[1] = obj2->name;
      argv[2] = NULL;
      UpdateAll(argv);
    }
  }
  arraydel2(d->points);
}

static gboolean
ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d)
{
  if (Menulock || Globallock)
    return FALSE;

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
    create_legend1(d);
    break;
  case PathB:
    create_path(d);
    break;
  case RectB:
  case ArcB:
    create_legend3(d);
    break;
  case GaussB:
    create_legendx(d);
    break;
  case SingleB:
    create_single_axis(d);
    break;
  case FrameB:
  case SectionB:
  case CrossB:
    create_axis(d);
    break;
  case ZoomB:
    break;
  }

  if ((d->Mode & POINT_TYPE_DRAW_ALL) && ! KeepMouseMode) {
    set_pointer_mode(-1);
  }

  return TRUE;
}

void
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
  int num;
  struct Point *po;
  double zoom;

  if (Menulock || Globallock)
    return FALSE;

  if (d->MoveData) {
    move_data_cancel(d, TRUE);
  } else if (d->Capture) {
    zoom = Menulocal.PaperZoom / 10000.0;
    switch (d->Mode) {
    case PathB:
      num = arraynum(d->points);
      if (num > 0) {
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
	    CheckGrid(TRUE, state, &(po->x), &(po->y), NULL, NULL);
	  }
	}
	break;
      case RectB:
      case ArcB:
      case GaussB:
      case SingleB:
      case FrameB:
      case SectionB:
      case CrossB:
	arraydel2(d->points);
	d->Capture = FALSE;
	break;
      default:
	break;
      }
    }
  } else if (d->Mode == ZoomB) {
    mouse_down_zoom(state, point, d, ! (state & GDK_CONTROL_MASK));
  } else if (d->MouseMode == MOUSENONE) {
    do_popup(e, d);
  }

  gtk_widget_queue_draw(d->Win);

  return TRUE;
}

static gboolean
ViewerEvMButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  if (Menulock || Globallock)
    return FALSE;

  if (d->Mode == ZoomB) {
#ifdef SHOW_MOVE_ANIMATION
    show_move_animation(d, point->x - d->cx , point->y - d->cy);
#endif	/* SHOW_MOVE_ANIMATION */
    d->hscroll -= (d->cx - point->x);
    d->vscroll -= (d->cy - point->y);
    ChangeDPI();
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

  GetFocusFrame(&x1, &y1, &x2, &y2, d->FrameOfsX, d->FrameOfsY, d);

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
update_frame_rect(TPoint *point, struct Viewer *d, double zoom)
{
  d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY2 = calc_mouse_y(point->y, zoom, d);
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
  if (po2 == NULL) {
    return;
  }

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
mouse_move_drag(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x, y;

  d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY2 = calc_mouse_y(point->y, zoom, d);

  x = d->MouseX2 - d->MouseX1;
  y = d->MouseY2 - d->MouseY1;

  CheckGrid(FALSE, state, &x, &y, NULL, NULL);

  d->FrameOfsX = x;
  d->FrameOfsY = y;
}

static void
mouse_move_zoom(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  double zoom_x, zoom_y, preserve_ratio;
  int vx1, vx2, vy1, vy2;

  vx1 = calc_mouse_x(point->x, zoom, d);
  vy1 = calc_mouse_y(point->y, zoom, d);

  preserve_ratio = (state & GDK_CONTROL_MASK);
  calc_zoom(d, vx1, vy1, &vx2, &vy2, &zoom_x, &zoom_y, preserve_ratio);

  if ((d->Mode != DataB) && (d->Mode != EvalB))
    CheckGrid(FALSE, state, NULL, NULL, &zoom_x, &zoom_y);

  d->ZoomX = zoom_x;
  d->ZoomY = zoom_y;

  set_zoom_prm(d, vx2, vy2, zoom_x, zoom_y);
}

static void
mouse_move_change(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int x, y;

  d->MouseX2 = calc_mouse_x(point->x, zoom, d);
  d->MouseY2 = calc_mouse_y(point->y, zoom, d);

  x = d->MouseX2 - d->MouseX1;
  y = d->MouseY2 - d->MouseY1;

  if ((d->Mode != DataB) && (d->Mode != EvalB)) {
    CheckGrid(FALSE, state, &x, &y, NULL, NULL);
  }

  d->LineX = x;
  d->LineY = y;
}

static void
mouse_move_scroll(TPoint *point, const struct Viewer *d)
{
  int h, w;
  GdkWindow *win;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  w = gdk_window_get_width(win);
  h = gdk_window_get_height(win);
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
mouse_move_draw(unsigned int state, int *dx, int *dy, const struct Viewer *d)
{
  struct Point *po;

  if (arraynum(d->points) == 0) {
    return;
  }

  po = *(struct Point **) arraylast(d->points);

  if (state & GDK_CONTROL_MASK) {
    if (d->Mode & POINT_TYPE_DRAW1) {
      calc_integer_ratio(d->points, dx, dy);
    } else if (d->Mode & POINT_TYPE_DRAW2) {
      calc_snap_angle(d->points, dx, dy);
      if (! (state & GDK_SHIFT_MASK)) {
	CheckGrid(FALSE, 0, dx, dy, NULL, NULL);
      }
    }
  }

  if (po != NULL) {
    po->x = *dx;
    po->y = *dy;
  }
}

static gboolean
ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d)
{
  int dx, dy;
  double zoom;

  if (Menulock || Globallock) {
    return FALSE;
  }

  if (gtk_widget_get_window(d->Win) == NULL) {
    return FALSE;
  }

  d->KeyMask = state;
  zoom = Menulocal.PaperZoom / 10000.0;

  dx = calc_mouse_x(point->x, zoom, d);
  dy = calc_mouse_y(point->y, zoom, d);

  if (d->MouseMode == MOUSESCROLLE) {
    range_increment(d->HScroll, mxd2p(d->MouseX1 - dx));
    range_increment(d->VScroll, mxd2p(d->MouseY1 - dy));
    return FALSE;
  }

  if ((d->Mode != DataB) &&
      (d->Mode != EvalB) &&
      (d->Mode != ZoomB) &&
      (d->MouseMode != MOUSEPOINT) &&
      (((d->Mode != PointB) && (d->Mode != LegendB) && (d->Mode != AxisB)) ||
       (d->MouseMode != MOUSENONE))) {
    CheckGrid(TRUE, state, &dx, &dy, NULL, NULL);
  }

  d->CrossX = dx;
  d->CrossY = dy;

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
	mouse_move_drag(state, point, zoom, d);
	break;
      case MOUSEZOOM1:
      case MOUSEZOOM2:
      case MOUSEZOOM3:
      case MOUSEZOOM4:
	mouse_move_zoom(state, point, zoom, d);
	break;
      case MOUSECHANGE:
	mouse_move_change(state, point, zoom, d);
	break;
      case MOUSEPOINT:
	update_frame_rect(point, d, zoom);
	break;
      case MOUSENONE:
      case MOUSESCROLLE:
	break;
      }
    } else if (d->Mode & POINT_TYPE_POINT) {
      if (d->MouseMode == MOUSEPOINT) {
	update_frame_rect(point, d, zoom);
      }
    } else {
      mouse_move_draw(state, &dx, &dy, d);
    }
  }

  SetPoint(d, dx, dy);

  if ((region == NULL && Menulocal.show_cross && gtk_widget_is_drawable(d->Win)) ||
      (d->Mode & POINT_TYPE_DRAW_ALL) ||
      d->MouseMode != MOUSENONE) {
    gtk_widget_queue_draw(d->Win);
  }

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

#if ! GTK_CHECK_VERSION(3, 22, 0)
static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct Viewer *d;
  GdkWindow *win;

  d = (struct Viewer *) user_data;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  gdk_window_get_origin(win, x, y);
}
#endif

int
check_focused_obj_type(const struct Viewer *d, int *type)
{
  int num, i, t;
  static struct objlist *axis, *merge, *legend, *text;

  num = arraynum(d->focusobj);

  if (axis == NULL)
    axis = chkobject("axis");

  if (merge == NULL)
    merge = chkobject("merge");

  if (legend == NULL)
    legend = chkobject("legend");

  if (text == NULL)
    text = chkobject("text");

  if (axis == NULL || merge == NULL || legend == NULL || text == NULL) {
    return 0;
  }

  t = 0;
  for (i = 0; i < num; i++) {
    struct FocusObj *focus;
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
#if GTK_CHECK_VERSION(3, 22, 0)
  if (! gtk_widget_get_realized(d->popup)) {
    gtk_widget_realize(d->popup);
  }
  gtk_menu_popup_at_pointer(GTK_MENU(d->popup), ((GdkEvent *)event));
#else
  int button, event_time;
  GtkMenuPositionFunc func = NULL;

  if (d->popup == NULL) {
    return;
  }

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
    func = popup_menu_position;
  }

  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d, button, event_time);
#endif
}

static gboolean
ViewerEvScroll(GtkWidget *w, GdkEventScroll *e, gpointer client_data)
{
  struct Viewer *d;
  TPoint point;
  gdouble x, y;

  point.x = e->x;
  point.y = e->y;

  d = (struct Viewer *) client_data;

  switch (e->direction) {
  case GDK_SCROLL_UP:
    if (e->state & GDK_CONTROL_MASK) {
      mouse_down_zoom_little(0, &point, d, FALSE);
    } else {
      range_increment(d->VScroll, -SCROLL_INC);
    }
    return TRUE;
  case GDK_SCROLL_DOWN:
    if (e->state & GDK_CONTROL_MASK) {
      mouse_down_zoom_little(0, &point, d, TRUE);
    } else {
      range_increment(d->VScroll, SCROLL_INC);
    }
    return TRUE;
  case GDK_SCROLL_LEFT:
    range_increment(d->HScroll, -SCROLL_INC);
    return TRUE;
  case GDK_SCROLL_RIGHT:
    range_increment(d->HScroll, SCROLL_INC);
    return TRUE;
  case GDK_SCROLL_SMOOTH:
    if (gdk_event_get_scroll_deltas((GdkEvent *) e, &x, &y)) {
      if ((e->state & GDK_CONTROL_MASK) && y != 0) {
	mouse_down_zoom_little(0, &point, d, y > 0);
      } else {
	range_increment(d->HScroll, x * SCROLL_INC);
	range_increment(d->VScroll, y * SCROLL_INC);
      }
    }
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

  d->KeyMask = e->state;

  point.x = e->x;
  point.y = e->y;

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

  d->KeyMask = e->state;

  point.x = e->x;
  point.y = e->y;

  switch (e->button) {
  case Button1:
    return ViewerEvLButtonUp(e->state, &point, d);
  }

  return FALSE;
}

static void
move_focus_frame(GdkEventKey *e, struct Viewer *d)
{
  int dx = 0, dy = 0, mv;
  double zoom;

  zoom = Menulocal.PaperZoom / 10000.0;
  mv = (e->state & GDK_SHIFT_MASK) ? Menulocal.grid / 10 : Menulocal.grid;

  switch (e->keyval) {
  case GDK_KEY_Down:
    dy = mv;
    break;
  case GDK_KEY_Up:
    dy = -mv;
    break;
  case GDK_KEY_Right:
    dx = mv;
    break;
  case GDK_KEY_Left:
    dx = -mv;
    break;
  default:
    return;
  }

  if (dx != 0 || dy != 0) {
    d->FrameOfsX += dx / zoom;
    d->FrameOfsY += dy / zoom;
    d->MouseMode = MOUSEDRAG;
    set_drag_info(d);
    gtk_widget_queue_draw(d->Win);
  }
}

static int
viewer_key_scroll(GdkEventKey *e, struct Viewer *d)
{
  switch (e->keyval) {
  case GDK_KEY_Up:
    range_increment(d->VScroll, -SCROLL_INC);
    return TRUE;
  case GDK_KEY_Down:
    range_increment(d->VScroll, SCROLL_INC);
    return TRUE;
  case GDK_KEY_Left:
    range_increment(d->HScroll, -SCROLL_INC);
    return TRUE;
  case GDK_KEY_Right:
    range_increment(d->HScroll, SCROLL_INC);
    return TRUE;
  }
  return FALSE;
}

static gboolean
ViewerEvKeyDown(GtkWidget *w, GdkEventKey *e, gpointer client_data)
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

  if (Menulock || Globallock)
    goto EXIT_PRAPAGATE;

  switch (e->keyval) {
  case GDK_KEY_Escape:
    if (d->MoveData) {
      move_data_cancel(d, TRUE);
    } else {
      UnFocus();
    }
    set_pointer_mode(-1);
    goto EXIT_PRAPAGATE;
  case GDK_KEY_space:
    CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
    return TRUE;
  case GDK_KEY_Page_Up:
    range_increment(d->VScroll, -SCROLL_INC * 4);
    return TRUE;
  case GDK_KEY_Page_Down:
    range_increment(d->VScroll, SCROLL_INC * 4);
    return TRUE;
  case GDK_KEY_Down:
  case GDK_KEY_Up:
  case GDK_KEY_Left:
  case GDK_KEY_Right:
    if (arraynum(d->focusobj) == 0) {
      return viewer_key_scroll(e, d);
    }

    if (((d->MouseMode == MOUSENONE) || (d->MouseMode == MOUSEDRAG)) &&
	(d->Mode & POINT_TYPE_POINT)) {
      move_focus_frame(e, d);
      return TRUE;
    }
    break;
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_PLUS);
      return TRUE;
    }
    break;
  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  case GDK_KEY_Return:
    ViewUpdate();
    break;
  default:
    break;
  }

 EXIT_PRAPAGATE:
  set_focus_sensitivity(d);
  return FALSE;
}

static gboolean
ViewerEvKeyUp(GtkWidget *w, GdkEventKey *e, gpointer client_data)
{
  struct Viewer *d;
  char *objs[OBJ_MAX];
  int dx, dy;
  int axis;

  if (Menulock || Globallock)
    return FALSE;

  d = (struct Viewer *) client_data;

  switch (e->keyval) {
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  case GDK_KEY_Down:
  case GDK_KEY_Up:
  case GDK_KEY_Left:
  case GDK_KEY_Right:
    if (d->MouseMode != MOUSEDRAG)
      break;

    dx = d->FrameOfsX;
    dy = d->FrameOfsY;
    get_focused_obj_array(d->focusobj, objs);
    axis = move_objects(dx, dy, d, objs);
    if (! axis) {
      d->allclear = FALSE;
    } else {
      add_data_grid_to_objs(objs);
    }
    UpdateAll(objs);
    d->MouseMode = MOUSENONE;
#if CLEAR_DRAG_INFO
    reset_drag_info(d);
#endif
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

  d = (struct Viewer *) client_data;

  d->cx = allocation->width / 2;
  d->cy = allocation->height / 2;

  ChangeDPI();
}

static gboolean
ViewerEvPaint(GtkWidget *w, cairo_t *cr, gpointer client_data)
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

  if (ZoomLock) {
    return TRUE;
  }

  if (Menulocal.pix && Menulocal.bg) {
    cairo_set_source_surface(cr, Menulocal.bg,
			     nround(- d->hscroll + d->cx),
			     nround(- d->vscroll + d->cy));
    cairo_paint(cr);
    cairo_set_source_surface(cr, Menulocal.pix,
			     nround(- d->hscroll + d->cx),
			     nround(- d->vscroll + d->cy));
    cairo_paint(cr);
  }

  if (! Globallock) {
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(cr, 1);
    /* I think it is not necessary to check chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid). */
    if (d->ShowFrame) {
      ShowFocusFrame(cr, d);
    }

    ShowPoints(cr, d);

    if (d->ShowLine) {
      ShowFocusLine(cr, d);
    }

    if (d->ShowRect) {
      ShowFrameRect(cr, d);
    }

    if (Menulocal.show_cross) {
      ShowCrossGauge(cr, d);
    }
  }

  if (! PaintLock && region) {
    cairo_region_destroy(region);
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
  gtk_widget_queue_draw(d->Win);
}

static void
ViewerEvHScroll(GtkRange *range, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  d->hscroll = gtk_range_get_value(range);
  SetHRuler(d);
  gtk_widget_queue_draw(d->Win);
}

void
ViewerWinUpdate(char **objects)
{
  int i, num, lock_state;
  struct FocusObj **focus;
  struct Viewer *d;

  lock_state = PaintLock;
  PaintLock = TRUE;

  d = &NgraphApp.Viewer;
  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) == NULL) {
    CloseGC();
    CloseGRA();
    OpenGRA();
    OpenGC();
    SetScroller();
    ChangeDPI();
  }
  CheckPage();
  num = arraynum(d->focusobj);
  focus = arraydata(d->focusobj);
  for (i = num - 1; i >= 0; i--) {
    if (chkobjoid(focus[i]->obj, focus[i]->oid) == -1) {
      arrayndel2(d->focusobj, i);
    }
  }

  if (arraynum(d->focusobj) == 0) {
    clear_focus_obj(d);
  }

  if (d->allclear) {
    mx_clear(NULL, objects);
    mx_redraw(Menulocal.obj, Menulocal.inst, objects);
  } else if (region) {
    Menulocal.region = region;
    gra2cairo_clip_region(Menulocal.local, region);
    mx_redraw(Menulocal.obj, Menulocal.inst, objects);
    gra2cairo_clip_region(Menulocal.local, NULL);
    Menulocal.region = NULL;
  }

  PaintLock = lock_state;
  gtk_widget_queue_draw(d->Win);

  d->allclear = TRUE;
}

static void
SetHRuler(const struct Viewer *d)
{
  gdouble x1, x2, zoom;
  int width;
  GdkWindow *win;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  width = gdk_window_get_width(win);
  zoom = Menulocal.PaperZoom / 10000.0;
  x1 = N2GTK_RULER_METRIC(calc_mouse_x(0, zoom, d));
  x2 = x1 + N2GTK_RULER_METRIC(mxp2d(width)) / zoom;

  nruler_set_range(d->HRuler, x1, x2);
}

static void
SetVRuler(const struct Viewer *d)
{
  gdouble  y1, y2, zoom;
  int height;
  GdkWindow *win;

  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }

  height = gdk_window_get_height(win);
  zoom = Menulocal.PaperZoom / 10000.0;
  y1 = N2GTK_RULER_METRIC(calc_mouse_y(0, zoom, d));
  y2 = y1 + N2GTK_RULER_METRIC(mxp2d(height)) / zoom;

  nruler_set_range(d->VRuler, y1, y2);
}

#define CHECK_FOCUSED_OBJ_ERROR -1
#define CHECK_FOCUSED_OBJ_NOT_FOUND -2

static int
check_focused_obj(struct narray *focusobj, struct objlist *fobj, int oid)
{
  int i, num;

  if (fobj == NULL)
    return CHECK_FOCUSED_OBJ_ERROR;

  num = arraynum(focusobj);
  for (i = 0; i < num; i++) {
    struct FocusObj *focus;
    focus = *(struct FocusObj **) arraynget(focusobj, i);
    if (focus == NULL)
      continue;

    if (fobj == focus->obj && oid == focus->oid) {
      return i;
    }
  }
  return CHECK_FOCUSED_OBJ_NOT_FOUND;
}

static void
set_toolbox_mode_by_focus_obj(const struct Viewer *d)
{
  int type, n;
  n = check_focused_obj_type(d, &type);
  if (n == 0 || type == FOCUS_OBJ_TYPE_MERGE) {
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
  } else {
    set_toolbox_mode(TOOLBOX_MODE_SETTING_PANEL);
  }
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

  set_toolbox_mode_by_focus_obj(&NgraphApp.Viewer);
  return TRUE;
}

static void
clear_focus_obj(const struct Viewer *d)
{
  arraydel2(d->focusobj);
  set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
}

static int
check_drawrable(struct objlist *obj)
{
  struct narray *array;
  int i, n;
  array = &Menulocal.drawrable;
  n = arraynum(array);
  for (i = 0; i < n; i++) {
    char *name;
    name = arraynget_str(array, i);
    if (g_strcmp0(name, obj->name) == 0) {
      return 0;
    }
  }
  return 1;
}

void
Focus(struct objlist *fobj, int id, enum FOCUS_MODE mode)
{
  int oid, focus;
  N_VALUE *inst;
  int man, hidden, legend, axis, merge;
  struct Viewer *d;
  struct savedstdio save;

  if (fobj == NULL) {
    return;
  }
  d = &NgraphApp.Viewer;

  legend = chkobjchild(chkobject("legend"), fobj);
  axis = chkobjchild(chkobject("axis"), fobj);
  merge = chkobjchild(chkobject("merge"), fobj);
  if (! legend && ! axis && ! merge) {
    return;
  }

  if (check_drawrable(fobj)) {
    return;
  }

  if (mode == FOCUS_MODE_NORMAL) {
    UnFocus();
  }

  inst = chkobjinst(fobj, id);
  _getobj(fobj, "oid", inst, &oid);
  _getobj(fobj, "hidden", inst, &hidden);
  if (hidden) {
    return;
  }

  ignorestdio(&save);
  if (axis) {
    getobj(fobj, "group_manager", id, 0, NULL, &man);
    if (man >= 0) {
      getobj(fobj, "oid", man, 0, NULL, &oid);
    }
  }
  focus = check_focused_obj(d->focusobj, fobj, oid);
  if (focus >= 0) {
    arrayndel2(d->focusobj, focus);
  }
  if (focus < 0 || mode != FOCUS_MODE_TOGGLE) {
    add_focus_obj(d->focusobj, fobj, oid);
  }
  d->MouseMode = MOUSENONE;
  set_toolbox_mode_by_focus_obj(d);
  if (arraynum(d->focusobj) == 0) {
    UnFocus();
  }

  d->allclear = FALSE;
  //  UpdateAll();
  d->ShowFrame = TRUE;

  /* this is inconvenient when one use single window mode. */
  /* gtk_widget_grab_focus(d->Win); */
  gtk_widget_queue_draw(d->Win);
  restorestdio(&save);
}

void
UnFocus(void)
{
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  if (arraynum(d->focusobj) != 0) {
    clear_focus_obj(d);
  }

  d->ShowFrame = FALSE;
  if (arraynum(d->points) != 0) {
    arraydel2(d->points);
  }

  set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
  gtk_widget_queue_draw(d->Win);
}

#define GRID_MIN 16.0
#define GRIG_COLOR 0.7
static void
draw_grid(cairo_t *cr, int w, int h)
{
  int grid, x, y;
  double dw, dashes[] = {1.0, 1.0};
  grid = Menulocal.grid;
  dw = mxd2p(grid);
  if (dw < GRID_MIN) {
    grid *= ceil(GRID_MIN / dw);
  }
  cairo_set_source_rgba(cr, GRIG_COLOR, GRIG_COLOR, GRIG_COLOR, 1);
  cairo_set_dash(cr, dashes, 2, 0);
  for (x = grid; x < Menulocal.PaperWidth; x += grid) {
    double dx;
    dx = mxd2p(x) + 1;
    cairo_move_to(cr, dx, 0);
    cairo_line_to(cr, dx, h);
    cairo_stroke(cr);
  }
  for (y = grid; y < Menulocal.PaperHeight; y += grid) {
    double dy;
    dy = mxd2p(y) + 1;
    cairo_move_to(cr, 0, dy);
    cairo_line_to(cr, w, dy);
    cairo_stroke(cr);
  }
}

void
update_bg(void)
{
  int w, h;
  cairo_t *cr;
  if (Menulocal.bg == NULL) {
    return;
  }
  cr = cairo_create(Menulocal.bg);
  cairo_set_source_rgb(cr, Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  cairo_paint(cr);

  w = cairo_image_surface_get_width(Menulocal.bg) - CAIRO_COORDINATE_OFFSET;
  h = cairo_image_surface_get_height(Menulocal.bg) - CAIRO_COORDINATE_OFFSET;

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, NULL, 0, 0);
  cairo_rectangle(cr, CAIRO_COORDINATE_OFFSET, CAIRO_COORDINATE_OFFSET, w, h);
  cairo_stroke(cr);
  if (Menulocal.show_grid) {
    draw_grid(cr, w, h);
  }

  cairo_destroy(cr);
}

static void
create_layers(void)
{
  int i, n;
  struct narray *array;

  array = &Menulocal.drawrable;
  n = arraynum(array);
  for (i = 0; i < n; i++) {
    char *obj;
    obj = arraynget_str(array, i);
    init_layer(obj);
  }
}

static void
create_pix(int w, int h)
{
  GdkWindow *window;

  window = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if (window == NULL) {
    return;
  }

  if (w == 0) {
    w = 1;
  }

  if (h == 0) {
    h = 1;
  }

  if (Menulocal.pix) {
    cairo_surface_destroy(Menulocal.pix);
  }
  Menulocal.pix = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w + 1, h + 1);

  if (Menulocal.bg) {
    cairo_surface_destroy(Menulocal.bg);
  }
  Menulocal.bg = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w + 1, h + 1);

  create_layers();

  /* draw background */
  update_bg();

  gra2cairo_set_antialias(Menulocal.local, Menulocal.antialias);
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

  create_pix(width, height);

  Menulocal.region = NULL;
}

void
SetScroller(void)
{
  int width, height, x, y;
  struct Viewer *d;

  d = &NgraphApp.Viewer;
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

  d->hscroll = x;
  d->vscroll = y;
}

static double
get_range_max(GtkWidget *w)
{
  GtkAdjustment *adj;
  double val;

  adj = gtk_range_get_adjustment(GTK_RANGE(w));
  val = (adj) ? gtk_adjustment_get_upper(adj) : 0;

  return val;
}

void
ChangeDPI(void)
{
  int width, height, XPos, YPos, XRange = 0, YRange = 0;
  gint w, h;
  double ratex, ratey;
  struct objlist *obj;
  struct narray *array;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  XRange = get_range_max(d->HScroll);
  YRange = get_range_max(d->VScroll);

  XPos = nround(d->hscroll);
  YPos = nround(d->vscroll);

  if (XPos < 0) {
    XPos = 0;
  }
  if (XPos > XRange) {
    XPos = XRange;
  }

  if (YPos < 0) {
    YPos = 0;
  }
  if (YPos > YRange) {
    YPos = YRange;
  }

  ratex = (XRange == 0) ? 0 : XPos / (double) XRange;
  ratey = (YRange == 0) ? 0 : YPos / (double) YRange;

  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);

  if (Menulocal.pix) {
    w = cairo_image_surface_get_width(Menulocal.pix) - 1;
    h = cairo_image_surface_get_height(Menulocal.pix) - 1;
  } else {
    h = w = 0;
  }

  if (w != width || h != height) {
    create_pix(width, height);
    mx_redraw(Menulocal.obj, Menulocal.inst, NULL);
  }

  XPos = nround(width * ratex);
  YPos = nround(height * ratey);

  gtk_range_set_range(GTK_RANGE(d->HScroll), 0, width);
  gtk_range_set_value(GTK_RANGE(d->HScroll), XPos);
  d->hscroll = XPos;

  gtk_range_set_range(GTK_RANGE(d->VScroll), 0, height);
  gtk_range_set_value(GTK_RANGE(d->VScroll), YPos);
  d->vscroll = YPos;

  if ((obj = chkobject("text")) != NULL) {
    int i, num;
    num = chkobjlastinst(obj);
    for (i = 0; i <= num; i++) {
      N_VALUE *inst;
      inst = chkobjinst(obj, i);
      _getobj(obj, "bbox", inst, &array);
      arrayfree(array);
      _putobj(obj, "bbox", inst, NULL);
    }
  }

  gtk_widget_queue_draw(d->Win);

  SetHRuler(d);
  SetVRuler(d);
}

void
CloseGC(void)
{
  if (Menulocal.region != NULL) {
    cairo_region_destroy(Menulocal.region);
  }

  Menulocal.region = NULL;

  UnFocus();
}

void
ReopenGC(void)
{
  Menulocal.local->pixel_dot_x =
  Menulocal.local->pixel_dot_y =
    Menulocal.windpi / 25.4 / 100;

  if (Menulocal.region != NULL) {
    cairo_region_destroy(Menulocal.region);
  }
  Menulocal.region = NULL;
}

void
Draw(int SelectFile)
{
  struct Viewer *d;
  N_VALUE *gra_inst;

  draw_notify(FALSE);
  d = &NgraphApp.Viewer;

  if (SelectFile && !SetFileHidden())
    return;

  ProgressDialogCreate(_("Scaling"));

  FileAutoScale();
  AdjustAxis();
  FitClear();

  SetStatusBar(_("Drawing."));
  ProgressDialogSetTitle(_("Drawing"));

  ReopenGC();
  if (region) {
    cairo_region_destroy(region);
  }
  region = NULL;

  gra_inst = chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid);
  if (gra_inst != NULL) {
    d->ignoreredraw = TRUE;
    _exeobj(Menulocal.GRAobj, "clear", gra_inst, 0, NULL);
    //    reset_event();
    /* XmUpdateDisplay(d->Win); */
    _exeobj(Menulocal.GRAobj, "draw", gra_inst, 0, NULL);
    _exeobj(Menulocal.GRAobj, "flush", gra_inst, 0, NULL);
    d->ignoreredraw = FALSE;
  }

  ResetStatusBar();

  ProgressDialogFinalize();

  gtk_widget_queue_draw(d->Win);

  if (SelectFile) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  }
}

void
CmViewerDraw(void *w, gpointer client_data)
{
  int select_file;

  if (Menulock || Globallock)
    return;

  select_file = GPOINTER_TO_INT(client_data);

  Draw(select_file);

  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, DRAW_NONE);
}

static int
search_axis_group(struct objlist *obj, int id, const char *group,
		  int *findX, int *findY, int *findU, int *findR, int *findG,
		  int *idx, int *idy, int *idu, int *idr, int *idg)
{
  int j, id2, type;
  struct objlist *aobj, *gobj;
  char *group2;

  *findX = *findY = *findU = *findR = *findG = FALSE;
  type = group[0];

  for (j = 0; j <= id; j++) {
    N_VALUE *inst2;
    inst2 = chkobjinst(obj, j);
    _getobj(obj, "group", inst2, &group2);
    _getobj(obj, "id", inst2, &id2);

    if (group2 == NULL || group2[0] != type)
      continue;

    if (strcmp(group + 2, group2 + 2) != 0)
      continue;

    if (group2[1] == 'X') {
      *findX = TRUE;
      *idx = id2;
    } else if (group2[1] == 'Y') {
      *findY = TRUE;
      *idy = id2;
    } else if (group2[1] == 'U') {
      *findU = TRUE;
      *idu = id2;
    } else if (group2[1] == 'R') {
      *findR = TRUE;
      *idr = id2;
    }
  }

  gobj = chkobject("axisgrid");
  if ((type == 's' || type == 'f') &&
      *findX && *findY &&
      ! check_drawrable(gobj)) {
    int snum;

    snum = chkobjlastinst(gobj) + 1;
    for (j = 0; j < snum; j++) {
      int aid1, aid2;
      N_VALUE *dinst;
      dinst = chkobjinst(gobj, j);
      if (dinst == NULL) {
	continue;
      }
      aid1 = get_axis_id(gobj, dinst, &aobj, AXIS_X);
      aid2 = get_axis_id(gobj, dinst, &aobj, AXIS_Y);
      if (aid1 >= 0 && aid2 >= 0 && obj == aobj && aid1 == *idx && aid2 == *idy) {
	*findG = TRUE;
	_getobj(gobj, "id", dinst, idg);
	break;
      }
    }
  }
  return type;
}

static void
ViewUpdate(void)
{
  int i, id, num, modified, undo;
  struct FocusObj *focus;
  struct objlist *obj, *dobj = NULL;
  N_VALUE *inst, *dinst;
  int ret;
  int x1, y1;
  int idx = 0, idy = 0, idu = 0, idr = 0, idg, lenx, leny;
  int findX, findY, findU, findR, findG;
  char type;
  char *group, *objs[OBJ_MAX];
  int axis;
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;
  num = arraynum(d->focusobj);
  if (num < 1) {
    return;
  }

  d->ShowFrame = FALSE;
  d->ShowRect = FALSE;

  get_focused_obj_array(d->focusobj, objs);
  undo = menu_save_undo(UNDO_TYPE_EDIT, objs);
  axis = FALSE;
  PaintLock = TRUE;
  modified = FALSE;

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

      if (group && group[0] != 'a') {
	type = search_axis_group(obj, id, group,
				 &findX, &findY, &findU, &findR, &findG,
				 &idx, &idy, &idu, &idr, &idg);

	if (((type == 's') || (type == 'f'))
	    && findX && findY && findU && findR) {

	  dobj = chkobject("axisgrid");
	  if (! findG) {
	    idg = -1;
	  }

	  getobj(obj, "y", idx, 0, NULL, &y1);
	  getobj(obj, "x", idy, 0, NULL, &x1);
	  getobj(obj, "y", idu, 0, NULL, &leny);
	  getobj(obj, "x", idr, 0, NULL, &lenx);

	  leny = y1 - leny;
	  lenx = lenx - x1;

	  SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
			idx, idy, idu, idr, dobj, &idg, type == 's');

	  ret = DialogExecute(TopLevel, &DlgSection);

	  if (! findG && idg != -1) {
	    dinst = chkobjinst(dobj, idg);
	    if (dinst) {
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
	}
      } else {
	AxisDialog(NgraphApp.AxisWin.data.data, id, TRUE);
	ret = DialogExecute(TopLevel, &DlgAxis);
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
	MergeDialog(NgraphApp.MergeWin.data.data, id, 0);
	ret = DialogExecute(TopLevel, &DlgMerge);
      }

      if (ret == IDDELETE) {
	set_graph_modified();
	delobj(obj, id);
      }

      if (ret == IDOK)
	AddInvalidateRect(obj, inst);
    }
    if (ret != IDCANCEL) {
      modified = TRUE;
    }
  }
  PaintLock = FALSE;

  if (arraynum(d->focusobj) == 0)
    clear_focus_obj(d);

  if (modified) {
    if (! axis) {
      d->allclear = FALSE;
    }
    UpdateAll(objs);
  } else {
    menu_undo_internal(undo);
  }
  d->ShowFrame = TRUE;
}

static void
ViewDelete(void)
{
  int i, id, num;
  struct FocusObj *focus;
  int axis;
  struct Viewer *d;
  char *objs[OBJ_MAX];

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;
  if ((d->MouseMode != MOUSENONE) ||
      (d->Mode != PointB &&
       d->Mode != LegendB &&
       d->Mode != AxisB)) {
    return;
  }

  num = arraynum(d->focusobj);
  if (num < 1) {
    return;
  }

  d->ShowFrame = FALSE;
  axis = FALSE;
  PaintLock = TRUE;

  get_focused_obj_array(d->focusobj, objs);
  menu_save_undo(UNDO_TYPE_DELETE, objs);
  for (i = num - 1; i >= 0; i--) {
    struct objlist *obj;
    N_VALUE *inst;
    focus = *(struct FocusObj **) arraynget(d->focusobj, i);
    obj = focus->obj;

    inst = chkobjinstoid(obj, focus->oid);
    if (inst == NULL)
      continue;

    AddInvalidateRect(obj, inst);
    DelList(obj, inst, d);
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
    UpdateAll(objs);

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
  char *objects[2];

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;

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

  menu_save_undo_single(UNDO_TYPE_ORDER, obj->name);
  DelList(obj, inst, d);
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
  objects[0] = obj->name;
  objects[1] = NULL;
  UpdateAll(objects);
}

static void
ncopyobj(struct objlist *obj, int id1, int id2)
{
  char *field[] = {"name", NULL};

  copy_obj_field(obj, id1, id2, field);
}

static void
ViewCopyAxis(struct objlist *obj, int id, struct FocusObj *focus, N_VALUE *inst)
{
  int id2;
  N_VALUE *inst2;
  int findX, findY, findU, findR, findG;
  int oidx, oidy;
  int idx = 0, idy = 0, idu = 0, idr = 0, idg;
  int idx2, idy2, idu2, idr2;
  char *group;
  int tp;
  struct narray agroup;

  _getobj(obj, "group", inst, &group);

  if (group && group[0] != 'a') {
    char *axisx, *axisy;
    char type;
    char *argv[2];
    type = search_axis_group(obj, id, group,
			     &findX, &findY, &findU, &findR, &findG,
			     &idx, &idy, &idu, &idr, &idg);

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
	  axisx = g_strdup_printf("axis:^%d", oidx);
	  if (axisx) {
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
	  axisy = g_strdup_printf("axis:^%d", oidy);
	  if(axisy) {
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
        struct objlist *dobj;
        int idg2;
	dobj = chkobject("axisgrid");
	if ((idg2 = newobj(dobj)) >= 0) {
	  ncopyobj(dobj, idg2, idg);
	  inst2 = chkobjinst(dobj, idg2);
	  if (idx2 >= 0 && idu2 >= 0) {
	    axisx = g_strdup_printf("axis:^%d", oidx);
	    if (axisx) {
	      putobj(dobj, "axis_x", idg2, axisx);
	    }
	  }
	  if (idy2 >= 0 && idr2 >= 0) {
	    axisy = g_strdup_printf("axis:^%d", oidy);
	    if (axisy) {
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
	axisy = g_strdup_printf("axis:^%d", oidy);
	if (axisy) {
	  putobj(obj, "adjust_axis", idx2, axisy);
	}
	axisx = g_strdup_printf("axis:^%d", oidx);
	if (axisx) {
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
  int i, id2, num;
  struct FocusObj *focus;
  struct objlist *obj;
  N_VALUE *inst, *inst2;
  int axis = FALSE;
  struct Viewer *d;
  char *objs[OBJ_MAX];
  struct FOCUSED_INST *focused_inst;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;
  if (d->MouseMode != MOUSENONE || ! (d->Mode & POINT_TYPE_POINT)) {
    return;
  }

  num = arraynum(d->focusobj);
  if (num < 1) {
    return;
  }

  d->ShowFrame = FALSE;
  axis = FALSE;
  PaintLock = TRUE;

  focused_inst = create_focused_inst_array_by_id_order(arraydata(d->focusobj), num);
  if (focused_inst == NULL) {
    return;
  }

  get_focused_obj_array(d->focusobj, objs);
  menu_save_undo(UNDO_TYPE_COPY, objs);
  for (i = 0; i < num; i++) {
    int id;
    focus = focused_inst[i].focus;
    id = focused_inst[i].id;
    if (focus == NULL)
      continue;

    inst = chkobjinstoid(focus->obj, focus->oid);
    if (inst == NULL)
      continue;

    obj = focus->obj;
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
  g_free(focused_inst);

  if (! axis)
    d->allclear = FALSE;

  UpdateAll(objs);
  d->ShowFrame = TRUE;
}

void
ViewCross(int state)
{
  struct Viewer *d;

  if (Menulock || Globallock)
    return;

  d = &NgraphApp.Viewer;

  Menulocal.show_cross = state;
  if (gtk_widget_is_drawable(d->Win)) {
    gtk_widget_queue_draw(d->Win);
  }
}

void
ViewerUpdateCB(void *w, gpointer client_data)
{
  ViewUpdate();
}

gboolean
CmViewerButtonPressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  KeepMouseMode = (event->state & GDK_SHIFT_MASK);
  return FALSE;
}

void
CmEditMenuCB(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;

  switch (GPOINTER_TO_INT(client_data)) {
  case MenuIdEditRedo:
    menu_redo();
    break;
  case MenuIdEditUndo:
    menu_undo();
    break;
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
  case MenuIdEditDuplicate:
    ViewCopy();
    break;
  case MenuIdEditSelectAll:
    ViewSelectAll();
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
  case MenuIdAlignBottom:
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
  case MenuIdEditOrderTop:
    reorder_object(OBJECT_MOVE_TYPE_TOP);
    break;
  case MenuIdEditOrderUp:
    reorder_object(OBJECT_MOVE_TYPE_UP);
    break;
  case MenuIdEditOrderDown:
    reorder_object(OBJECT_MOVE_TYPE_DOWN);
    break;
  case MenuIdEditOrderBottom:
    reorder_object(OBJECT_MOVE_TYPE_LAST);
    break;
  }
  set_focus_sensitivity(&(NgraphApp.Viewer));
}
