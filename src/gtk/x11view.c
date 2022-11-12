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
#include "ogra2gdk.h"
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

#define VIEWER_DPI_MAX 620
#define VIEWER_DPI_MIN  20

static int PaintLock = FALSE, ZoomLock = FALSE, KeepMouseMode = FALSE;
static int ViewerZooming = FALSE;

#define EVAL_NUM_MAX 5000
static struct evaltype EvalList[EVAL_NUM_MAX];
static struct narray SelList;

#define IDEVMASK        101
#define IDEVMOVE        102

#if GTK_CHECK_VERSION(4, 0, 0)
static void ViewerEvSize(GtkWidget *w, int width, int height, gpointer client_data);
static void ViewerEvRealize(GtkWidget *w, gpointer client_data);
static void ViewerEvHScroll(GtkAdjustment *adj, gpointer user_data);
static void ViewerEvVScroll(GtkAdjustment *adj, gpointer user_data);
#else
static void ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data);
static void ViewerEvHScroll(GtkRange *range, gpointer user_data);
static void ViewerEvVScroll(GtkRange *range, gpointer user_data);
#endif
static gboolean ViewerEvPaint(GtkWidget *w, cairo_t *cr, gpointer client_data);
static gboolean ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d);
static gboolean ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d);
#if GTK_CHECK_VERSION(4, 0, 0)
static void ViewerEvButtonDown(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data);
static void ViewerEvButtonUp(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data);
#else
static void ViewerEvButtonDown(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data);
static void ViewerEvButtonUp(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data);
#endif
static gboolean ViewerEvKeyDown(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void ViewerEvKeyUp(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);
static void gesture_zoom(GtkGestureZoom *controller, gdouble scale, gpointer user_data);
#if GTK_CHECK_VERSION(4, 0, 0)
static void ViewerEvMouseMotion(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer client_data);
static gboolean ViewerEvScroll(GtkEventControllerScroll *w, double x, double y, gpointer client_data);
#else
static gboolean ViewerEvMouseMotion(GtkWidget *w, GdkEventMotion *e, gpointer client_data);
static gboolean ViewerEvScroll(GtkWidget *w, GdkEventScroll *e, gpointer client_data);
#endif
static void ViewUpdate(void);
static void ViewCopy(void);
#if GTK_CHECK_VERSION(4, 0, 0)
static void do_popup(gdouble x, gdouble y, struct Viewer *d);
#else
static void do_popup(GdkEventButton *event, struct Viewer *d);
#endif
static int check_focused_obj(struct narray *focusobj, struct objlist *fobj, int oid);
static int get_mouse_cursor_type(struct Viewer *d, int x, int y);
static void reorder_object(enum object_move_type type);
static void SetHRuler(const struct Viewer *d);
static void SetVRuler(const struct Viewer *d);
static void clear_focus_obj(struct Viewer *d);
static void ViewDelete(void);
static int text_dropped(const char *str, gint x, gint y, struct Viewer *d);
static int add_focus_obj(struct narray *focusobj, struct objlist *obj, int oid);
static void ShowFocusFrame(cairo_t *cr, struct Viewer *d);
static void RotateFocusedObj(int direction);
static void set_mouse_cursor_hover(struct Viewer *d, int x, int y);
static void CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom_x, double *zoom_y);
static int check_drawrable(struct objlist *obj);
static void GetLargeFrame(int *minx, int *miny, int *maxx, int *maxy, const struct Viewer *d);
static int zoom_focused_obj(int x, int y, double zoom_x, double zoom_y, char **objs, struct Viewer *d);
static void draw_focused_obj(struct Viewer *d);
static int search_axis_group(struct objlist *obj, int id, const char *group, int *findX, int *findY, int *findU, int *findR, int *findG, int *idx, int *idy, int *idu, int *idr, int *idg);
static void clear_focus_obj_pix(struct Viewer *d);
static void draw_focused_move(cairo_t *cr, double zoom, struct Viewer *d);

#define GRAY 0.5
#define DOT_LENGTH 4.0
#define EDITING_OPACITY 0.5

#define SCROLL_ANIMATION 0

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

#define SCROLL_DIV 6.0
#define SCROLL_DECELERATION_LIMIT 1.0

static gboolean
scroll_deceleration_cb(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
  struct Viewer *d;
  double x, y;

  d = (struct Viewer *) user_data;

  x = scrollbar_get_value(d->HScroll);
  y = scrollbar_get_value(d->VScroll);
  x += (d->scroll_prm.x - x) / SCROLL_DIV;
  y += (d->scroll_prm.y - y) / SCROLL_DIV;

  if (fabs(d->scroll_prm.x - x) < SCROLL_DECELERATION_LIMIT &&
      fabs(d->scroll_prm.y - y) < SCROLL_DECELERATION_LIMIT) {
    d->deceleration_prm.id = 0;
    scrollbar_set_value(d->HScroll, d->scroll_prm.x);
    scrollbar_set_value(d->VScroll, d->scroll_prm.y);
    return G_SOURCE_REMOVE;
  }

  scrollbar_set_value(d->HScroll, x);
  scrollbar_set_value(d->VScroll, y);

  return G_SOURCE_CONTINUE;
}

static void
cancel_deceleration(struct Viewer *d)
{
  if (d->deceleration_prm.id == 0) {
    return;
  }
  gtk_widget_remove_tick_callback(d->Win, d->deceleration_prm.id);
  d->deceleration_prm.id = 0;
}

static void
start_scroll_deceleration(double x, double y, struct Viewer *d)
{
  cancel_deceleration(d);
  d->scroll_prm.x = x;
  d->scroll_prm.y = y;
  d->deceleration_prm.id = gtk_widget_add_tick_callback(GTK_WIDGET(d->Win), scroll_deceleration_cb, d, NULL);
}

static void
range_increment(GtkWidget *w, double inc)
{
  double val;

  if (inc == 0) {
    return;
  }
  val = scrollbar_get_value(w);
  scrollbar_set_value(w, val + inc);
}

static void
range_increment_deceleration(double inc_x, double inc_y, struct Viewer *d)
{
  double x, y;

  if (inc_x == 0 && inc_y == 0) {
    return;
  }
  x = scrollbar_get_value(d->HScroll);
  y = scrollbar_get_value(d->VScroll);
  x += inc_x;
  y += inc_y;
  start_scroll_deceleration(x, y, d);
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

static void
copy_text(const char *str)
{
  GValue value = G_VALUE_INIT;
  GdkClipboard *clipboard;

  g_value_init(&value, G_TYPE_STRING);
  g_value_set_string(&value, str);
  clipboard = gtk_widget_get_clipboard(TopLevel);
  gdk_clipboard_set_value(clipboard, &value);
  g_value_unset(&value);
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
    copy_text(str->str);
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
      }
    }
    if (ocur->child) {
      focus_new_insts(ocur, array, objects);
    }
    ocur = ocur->next;
  }
  return;
}

struct paste_text_data
{
  struct narray *idarray;
  struct objlist *draw_obj;
  GThread *thread;
  char *text;
  struct Viewer *d;
};

static void
paste_text_finalise(gpointer user_data)
{
  char *objects[OBJ_MAX] = {NULL};
  struct paste_text_data *data;

  data = (struct paste_text_data *) user_data;
  g_thread_join(data->thread);
  focus_new_insts(data->draw_obj, data->idarray, objects);
  arrayfree(data->idarray);

  if (arraynum(data->d->focusobj) > 0) {
    set_graph_modified();
    data->d->ShowFrame = TRUE;
    gtk_widget_grab_focus(data->d->Win);
    UpdateAll(objects);
  }
  g_free(data->text);
  g_free(data);
}

static gpointer
paste_script_evaluate(gpointer user_data)
{
  struct paste_text_data *data;

  data = (struct paste_text_data *) user_data;
  eval_script(data->text, TRUE);
  data->thread = g_thread_self();
  g_idle_add_once(paste_text_finalise, data);
  return NULL;
}

static void
paste_text(const gchar *text, struct Viewer *d)
{
  struct narray *idarray;
  struct objlist *draw_obj;
  struct paste_text_data *data;

  if (text == NULL) {
    return;
  }

  if (strncmp(text, SCRIPT_IDN, SCRIPT_IDN_LEN)) {
    gint w, h;

    w = gtk_widget_get_width(d->Win);
    h = gtk_widget_get_height(d->Win);
    text_dropped(text, w / 2, h / 2, &NgraphApp.Viewer);
    return;
  }

  draw_obj = chkobject("draw");
  if (draw_obj == NULL) {
    return;
  }

  idarray = arraynew(sizeof(int));
  if (idarray == NULL) {
    return;
  }
  check_last_insts(draw_obj, idarray);

  data = g_malloc0(sizeof(*data));
  data->text = g_strdup(text);
  data->idarray = idarray;
  data->draw_obj = draw_obj;
  data->d = d;

  UnFocus();
  menu_save_undo(UNDO_TYPE_PASTE, NULL);
  g_thread_new(NULL, paste_script_evaluate, data);
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
remove_cr_in_place(char *text)
{
  int i, j;
  for (i = j = 0; text[i]; i++) {
    if (text[i] != '\r') {
      text[j++] = text[i];
    }
  }
  text[j] = '\0';
}

static void
text_async_completed(GObject *source, GAsyncResult *res, gpointer data)
{
  GdkClipboard* clipboard;
  struct Viewer *d;
  char *text;

  d = (struct Viewer *) data;
  clear_focus_obj_pix(d);
  set_focus_sensitivity(d);

  clipboard = GDK_CLIPBOARD(source);
  text = gdk_clipboard_read_text_finish(clipboard, res, NULL);
  if (text) {
    remove_cr_in_place(text);
    paste_text(text, d);
    g_free(text);
  }
  /*
  device = gtk_get_current_event_device();
  if (device && gdk_device_get_source(device) != GDK_SOURCE_KEYBOARD) {
      GdkWindow *win;
      win = gtk_widget_get_window(d->Win);
      if (win) {
	gdk_window_get_device_position(win, device, &x, &y, NULL);
	set_mouse_cursor_hover(d, x, y);
      }
    }
  }
  */
}

static void
PasteObjectsFromClipboard(void)
{
  struct Viewer *d;
  GdkClipboard *clipboard;

  d = &NgraphApp.Viewer;
  if (d->Win == NULL || (d->Mode != PointB && d->Mode != LegendB)) {
    return;
  }

  clipboard = gtk_widget_get_clipboard(TopLevel);
  gdk_clipboard_read_text_async(clipboard, NULL, text_async_completed, d);
}
#else
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
graph_dropped_response(int ret, gpointer user_data)
{
  char *fname, *cwd;

  if (! ret) {
    return;
  }

  fname = (char *) user_data;
  cwd = ngetcwd();
  if (chdir_to_ngp(fname)) {
    if (cwd) {
      g_free(cwd);
    }
    g_free(fname);
    return;
  }

  LoadNgpFile(fname, FALSE, "-f", cwd);
  if (cwd) {
    g_free(cwd);
  }
  g_free(fname);
}
#endif

int
graph_dropped(const char *str)
{
  char *ext, *cwd, *fname;

  fname = g_strdup(str);
  if (fname == NULL) {
    return 1;
  }

  ext = getextention(fname);
  if (ext == NULL) {
    g_free(fname);
    return 1;
  }

  if (strcmp0(ext, "ngp")) {
    g_free(fname);
    return 1;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  CheckSave(graph_dropped_response, fname);
#else
  if (!CheckSave()) {
    g_free(fname);
    return 1;
  }

  cwd = ngetcwd();
  if (chdir_to_ngp(fname)) {
    if (cwd) {
      g_free(cwd);
    }
    g_free(fname);
    return 1;
  }

  if (LoadNgpFile(fname, FALSE, "-f") && cwd) {
    nchdir(cwd);
  }
  if (cwd) {
    g_free(cwd);
  }
  g_free(fname);
#endif
  return 0;
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
struct data_dropped_data {
  int i, id0, file_type;
  char *name;
  struct narray *filenames;
};

static void data_dropped_sub(struct data_dropped_data *data);

static void
new_merge_obj_response(struct response_callback *cb)
{
  struct MergeDialog *d;
  struct data_dropped_data *data;

  data = (struct data_dropped_data *) cb->data;
  d = (struct MergeDialog *) cb->dialog;
  if (cb->return_value == IDCANCEL) {
    delobj(d->Obj, d->Id);
  } else {
    set_graph_modified();
  }
  data_dropped_sub(data);
}

static int
new_merge_obj(char *name, struct objlist *obj, struct data_dropped_data *data)
{
  int id;

  id = newobj(obj);

  if (id < 0)
    return 1;

  changefilename(name);
  putobj(obj, "file", id, name);
  MergeDialog(NgraphApp.MergeWin.data.data, id, -1);
  response_callback_add(&DlgMerge, new_merge_obj_response, NULL, data);
  DialogExecute(TopLevel, &DlgMerge);
  return 0;
}
#else
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
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
new_file_obj_response(struct response_callback *cb)
{
  struct FileDialog *d;
  struct data_dropped_data *data;

  data = (struct data_dropped_data *) cb->data;
  d = (struct FileDialog *) cb->dialog;
  if (cb->return_value == IDCANCEL) {
    FitDel(d->Obj, d->Id);
    delobj(d->Obj, d->Id);
  } else {
    if (cb->return_value == IDFAPPLY) {
      data->id0 = d->Id;
    }
    set_graph_modified();
    AddDataFileList(data->name);
  }

  data_dropped_sub(data);
}

static int
new_file_obj(char *name, struct objlist *obj, int multi, struct data_dropped_data *data)
{
  int id;

  id = newobj(obj);
  if (id < 0) {
    return 1;
  }

  putobj(obj, "file", id, name);
  if (data->id0 != -1) {
    copy_file_obj_field(obj, id, data->id0, FALSE);
    AddDataFileList(name);
    data_dropped_sub(data);
    return 0;
  }

  data->name = name;
  FileDialog(NgraphApp.FileWin.data.data, id, multi);
  response_callback_add(&DlgFile, new_file_obj_response, NULL, data);
  DialogExecute(TopLevel, &DlgFile);
  return 0;
}
#else
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
data_dropped_sub(struct data_dropped_data *data)
{
  char *name, *ext;
  int type, ret, i;

  i = data->i;
  data->i--;

  if (i < 0) {
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, FALSE);
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
    arrayfree2(data->filenames);
    g_free(data);
    return;
  }

  name = g_strdup(arraynget_str(data->filenames, i));
  if (name == NULL) {
    data_dropped_sub(data);
    return;
  }

  type = data->file_type;
  if (type == FILE_TYPE_AUTO) {
    ext = getextention(name);
    if (ext && strcmp0(ext, "gra") == 0) {
      type = FILE_TYPE_MERGE;
    } else {
      type = FILE_TYPE_DATA;
    }
  }

  if (type == FILE_TYPE_MERGE) {
    struct objlist *obj;
    obj = chkobject("merge");
    ret = new_merge_obj(name, obj, data);
  } else {
    struct objlist *obj;
    obj = chkobject("data");
    ret = new_file_obj(name, obj, i > 0, data);
  }

  if (ret) {
    g_free(name);
    data_dropped_sub(data);
  }
}

int
data_dropped(struct narray *filenames, int file_type)
{
  char  *arg[4];
  struct objlist *obj, *mobj;
  struct data_dropped_data *data;
  int num;

  num = arraynum(filenames);

  obj = chkobject("data");
  if (obj == NULL) {
    return 1;
  }

  mobj = chkobject("merge");
  if (mobj == NULL) {
    return 1;
  }

  data = (struct data_dropped_data *) g_malloc0(sizeof(*data));
  if (data == NULL) {
    return 1;
  }
  data->i = num - 1;
  data->id0 = -1;
  data->file_type = file_type;
  data->filenames = filenames;

  arg[0] = obj->name;
  arg[1] = mobj->name;
  arg[2] = "fit";
  arg[3] = NULL;
  menu_save_undo(UNDO_TYPE_PASTE, arg);

  data_dropped_sub(data);

  return 0;
}
#else
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
text_dropped_response(struct response_callback *cb)
{
  struct Viewer *vd;
  struct LegendDialog *d;
  d = (struct LegendDialog *) cb->dialog;
  vd = (struct Viewer *) cb->data;

  if ((cb->return_value == IDDELETE) || (cb->return_value == IDCANCEL)) {
    menu_delete_undo(vd->undo);
    delobj(d->Obj, d->Id);
  } else {
    int oid;
    char *objects[] = {"text", NULL};

    UnFocus();

    getobj(d->Obj, "oid", d->Id, 0, NULL, &oid);
    add_focus_obj(NgraphApp.Viewer.focusobj, d->Obj, oid);

    set_graph_modified();
    vd->ShowFrame = TRUE;
    gtk_widget_grab_focus(vd->Win);
    UpdateAll(objects);
  }
  PaintLock = FALSE;
}

static int
text_dropped(const char *str, gint x, gint y, struct Viewer *d)
{
  N_VALUE *inst;
  char *ptr;
  double zoom = Menulocal.PaperZoom / 10000.0;
  struct objlist *obj;
  int id, x1, y1, i, j, l;

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

  d->undo = menu_save_undo_single(UNDO_TYPE_PASTE, obj->name);
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
  response_callback_add(&DlgLegendText, text_dropped_response, NULL, d);
  DialogExecute(TopLevel, &DlgLegendText);
  return 0;
}
#else
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

    set_graph_modified();
    d->ShowFrame = TRUE;
    gtk_widget_grab_focus(d->Win);
    UpdateAll(objects);
  }
  PaintLock = FALSE;

  return 0;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented (multiple files) */
struct narray *
get_dropped_files(const GValue* value)
{
  char *fname;
  int n;
  struct narray *files;
  GdkFileList *file_list;
  GSList *list, *l;

  if (! G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
    return NULL;
  }

  files = arraynew(sizeof(char *));
  if (files == NULL){
    return NULL;
  }

  file_list = g_value_get_boxed(value);
  list = gdk_file_list_get_files(file_list);
  for (l = list; l != NULL; l = l->next) {
    GFile *file = l->data;
    fname = g_file_get_path(file);
    arrayadd(files, &fname);
  }
  g_slist_free(list);

  n = arraynum(files);
  if (n < 1) {
    arrayfree2(files);
    return NULL;
  }
  return files;
}

static int
file_dropped(const GValue* value)
{
  char *fname;
  int r, n;
  struct narray *files;

  files = get_dropped_files(value);
  if (files == NULL){
    return TRUE;
  }

  n = arraynum(files);
  r = TRUE;
  if (n == 1) {
    fname = arraynget_str(files, 0);
    r = graph_dropped(fname);
  }
  if (r) {
    return data_dropped(files, FILE_TYPE_AUTO);
  }
  arrayfree2(files);
  return r;
}

static gboolean
drag_drop_cb(GtkDropTarget* self, const GValue* value, gdouble x, gdouble y, gpointer user_data)
{
  struct Viewer *d;
  int r;

  if (Globallock || Menulock || DnDLock)
    return FALSE;;

  d = (struct Viewer *) user_data;

  r = TRUE;
  if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
    r = file_dropped(value);
  } else if (G_VALUE_HOLDS(value, G_TYPE_STRING)) {
    const char *str;
    str = g_value_get_string(value);
    if (str) {
      r = text_dropped(str, x, y, d);
    }
  }
  return ! r;
}
#else
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
    if (filenames == NULL) {
      break;
    }

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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
init_dnd(struct Viewer *d)
{
  GtkDropTarget *target;
  GType types[] = {GDK_TYPE_FILE_LIST, G_TYPE_STRING};

  target = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
  gtk_drop_target_set_gtypes(target, types, G_N_ELEMENTS(types));

  g_signal_connect(target, "drop", G_CALLBACK(drag_drop_cb), d);
  gtk_widget_add_controller(GTK_WIDGET(d->Win), GTK_EVENT_CONTROLLER(target));
}
#else
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
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  copy_text(str->str);
#else
  if (str->len > 0) {
    GtkClipboard *clip;

    clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, str->str, -1);
  }
#endif

  g_string_free(str, TRUE);

  g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
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

#if GTK_CHECK_VERSION(4, 0, 0)
    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
#else
    swin = gtk_scrolled_window_new(NULL, NULL);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w = tree_store_create(sizeof(list) / sizeof(*list), list);
    tree_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
    gtk_container_add(GTK_CONTAINER(swin), w);
#endif

    w = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(w), swin);
    gtk_box_append(GTK_BOX(d->vbox), w);
#else
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 4);
#endif

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    w = gtk_button_new_with_mnemonic(_("Select _All"));
    set_button_icon(w, "edit-select-all");
    g_signal_connect(w, "clicked", G_CALLBACK(tree_store_select_all_cb), d->list);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif

    w = gtk_button_new_with_mnemonic(_("_Copy"));
    g_signal_connect(w, "clicked", G_CALLBACK(eval_dialog_copy_selected), d->list);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    gtk_widget_set_sensitive(w, FALSE);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    g_signal_connect(sel, "changed", G_CALLBACK(eval_data_sel_cb), w);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
#endif

    d->show_cancel = FALSE;
    d->ok_button = _("_Close");

    gtk_window_set_default_size(GTK_WINDOW(wi), 540, 400);

#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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


#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
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
#endif

#if 0
static void
menu_activate(GtkMenuShell *menushell, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  if (d->MoveData) {
    move_data_cancel(d, FALSE);
  }
}
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean
ev_popup_menu(GtkWidget *w, gpointer client_data)
{
  struct Viewer *d;

  if (Menulock || Globallock) return TRUE;

  d = (struct Viewer *) client_data;

  do_popup(NULL, d);
  return TRUE;
}
#endif

static void
update_drag(GtkGestureDrag *gesture, gdouble offset_x, gdouble offset_y, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  if (! d->drag_prm.active) {
    return;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  scrollbar_set_value(d->HScroll, d->drag_prm.x - offset_x);
  scrollbar_set_value(d->VScroll, d->drag_prm.y - offset_y);
#else
  gtk_range_set_value(GTK_RANGE(d->HScroll), d->drag_prm.x - offset_x);
  gtk_range_set_value(GTK_RANGE(d->VScroll), d->drag_prm.y - offset_y);
#endif
}

static void
begin_drag(GtkGestureDrag *gesture, gdouble start_x, gdouble start_y, gpointer user_data)
{
  struct Viewer *d;
  int cursor;

  d = (struct Viewer *) user_data;

  cancel_deceleration(d);

  switch (d->Mode) {
  case PointB:
  case LegendB:
  case AxisB:
  case ZoomB:
    cursor = get_mouse_cursor_type(d, start_x, start_y);
    d->drag_prm.active = (cursor == GDK_LEFT_PTR);
    break;
  default:
    d->drag_prm.active = FALSE;
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  d->drag_prm.x = scrollbar_get_value(d->HScroll);
  d->drag_prm.y = scrollbar_get_value(d->VScroll);
#else
  d->drag_prm.x = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->drag_prm.y = gtk_range_get_value(GTK_RANGE(d->VScroll));
#endif
}

static void
end_drag(GtkGestureDrag *gesture, gdouble start_x, gdouble start_y, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;
  d->drag_prm.active = FALSE;
}

static void
long_press_cb(GtkGesture *gesture, gdouble x, gdouble y, gpointer user_data)
{
  GdkEventSequence *sequence;

  sequence = gtk_gesture_single_get_current_sequence(GTK_GESTURE_SINGLE(gesture));
  gtk_gesture_set_sequence_state(gesture, sequence, GTK_EVENT_SEQUENCE_DENIED);
}

static void
long_press_cancelled_cb(GtkGesture *gesture, gpointer user_data)
{
  GdkEventSequence *sequence;
  GdkEvent *event;
#if GTK_CHECK_VERSION(4, 0, 0)
  GdkEventType type;
#endif

  sequence = gtk_gesture_get_last_updated_sequence(gesture);
  event = gtk_gesture_get_last_event(gesture, sequence);

#if GTK_CHECK_VERSION(4, 0, 0)
  type = gdk_event_get_event_type(event);
  if (type == GDK_TOUCH_BEGIN || type == GDK_BUTTON_PRESS) {
    gtk_gesture_set_sequence_state(gesture, sequence, GTK_EVENT_SEQUENCE_DENIED);
  }
#else
if (event->type == GDK_TOUCH_BEGIN || event->type == GDK_BUTTON_PRESS) {
    gtk_gesture_set_sequence_state(gesture, sequence, GTK_EVENT_SEQUENCE_DENIED);
  }
#endif
}

static double
get_deceleration_position(double a, double v0, double t)
{
#if HAVE_EXPM1
  return - expm1(- t * a) * v0 / a;
#else
  return (1 - exp(- t * a)) * v0 / a;
#endif
}

#define SWIPE_RESISTANCE 5.0

static gboolean
deceleration_cb(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
  struct Viewer *d;
  gint64 current_time;
  gdouble t;
  double x0, y0, x, y;

  d = (struct Viewer *) user_data;

  current_time = gdk_frame_clock_get_frame_time(frame_clock);
  t = (current_time - d->deceleration_prm.start) / 1000000.0;

#if GTK_CHECK_VERSION(4, 0, 0)
  x0 = scrollbar_get_value(d->HScroll);
  y0 = scrollbar_get_value(d->VScroll);
#else
  x0 = gtk_range_get_value(GTK_RANGE(d->HScroll));
  y0 = gtk_range_get_value(GTK_RANGE(d->VScroll));
#endif
  x = d->drag_prm.x - get_deceleration_position(SWIPE_RESISTANCE, d->drag_prm.vx, t);
  y = d->drag_prm.y - get_deceleration_position(SWIPE_RESISTANCE, d->drag_prm.vy, t);

#if GTK_CHECK_VERSION(4, 0, 0)
  scrollbar_set_value(d->HScroll, x);
  scrollbar_set_value(d->VScroll, y);
  x = scrollbar_get_value(d->HScroll);
  y = scrollbar_get_value(d->VScroll);
#else
  gtk_range_set_value(GTK_RANGE(d->HScroll), x);
  gtk_range_set_value(GTK_RANGE(d->VScroll), y);
  x = gtk_range_get_value(GTK_RANGE(d->HScroll));
  y = gtk_range_get_value(GTK_RANGE(d->VScroll));
#endif
  if (fabs(x0 - x) < SCROLL_DECELERATION_LIMIT && fabs(y0 - y) < SCROLL_DECELERATION_LIMIT) {
    d->deceleration_prm.id = 0;
    return G_SOURCE_REMOVE;
  }

  return G_SOURCE_CONTINUE;
}

static void
swipe_cb(GtkGestureSwipe *gesture, gdouble velocity_x, gdouble velocity_y, gpointer user_data)
{
  struct Viewer *d;
  GdkFrameClock *frame_clock;

  d = (struct Viewer *) user_data;
  if (! d->drag_prm.active) {
    return;
  }

  g_return_if_fail(d->deceleration_prm.id == 0);

  frame_clock = gtk_widget_get_frame_clock(GTK_WIDGET(d->Win));
  d->deceleration_prm.start = gdk_frame_clock_get_frame_time (frame_clock);

#if GTK_CHECK_VERSION(4, 0, 0)
  d->drag_prm.x = scrollbar_get_value(d->HScroll);
  d->drag_prm.y = scrollbar_get_value(d->VScroll);
#else
  d->drag_prm.x = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->drag_prm.y = gtk_range_get_value(GTK_RANGE(d->VScroll));
#endif
  d->drag_prm.vx = velocity_x;
  d->drag_prm.vy = velocity_y;

  d->deceleration_prm.id = gtk_widget_add_tick_callback(GTK_WIDGET(d->Win), deceleration_cb, d, NULL);
}

static void
add_event_drag(GtkWidget *widget, struct Viewer *d)
{
  GtkGesture *ev_drag, *ev_swipe, *ev_long_press;

#if GTK_CHECK_VERSION(4, 0, 0)
  ev_drag = gtk_gesture_drag_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev_drag));
#else
  ev_drag = gtk_gesture_drag_new(widget);
#endif
  gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(ev_drag), TRUE);

  g_signal_connect(ev_drag, "drag-update", G_CALLBACK(update_drag), d);
  g_signal_connect(ev_drag, "drag-begin", G_CALLBACK(begin_drag), d);
  g_signal_connect(ev_drag, "drag-end", G_CALLBACK(end_drag), d);

#if GTK_CHECK_VERSION(4, 0, 0)
  ev_swipe = gtk_gesture_swipe_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev_swipe));
#else
  ev_swipe = gtk_gesture_swipe_new(widget);
#endif
  gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(ev_swipe), TRUE);
  gtk_gesture_group(ev_swipe, ev_drag);
  g_signal_connect(ev_swipe, "swipe", G_CALLBACK(swipe_cb), d);

#if GTK_CHECK_VERSION(4, 0, 0)
  ev_long_press = gtk_gesture_long_press_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev_long_press));
#else
  ev_long_press = gtk_gesture_long_press_new(widget);
#endif
  gtk_gesture_single_set_touch_only(GTK_GESTURE_SINGLE(ev_long_press), TRUE);
  gtk_gesture_group(ev_long_press, ev_drag);
  g_signal_connect(ev_long_press, "pressed", G_CALLBACK(long_press_cb), d);
  g_signal_connect(ev_long_press, "cancelled", G_CALLBACK(long_press_cancelled_cb), d);

  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(ev_drag), GTK_PHASE_CAPTURE);
  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(ev_swipe), GTK_PHASE_CAPTURE);
  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(ev_long_press), GTK_PHASE_CAPTURE);
}

static void
add_event_button(GtkWidget *widget, struct Viewer *d)
{
  GtkGesture *ev;
#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_gesture_click_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev));
#else
  ev = gtk_gesture_multi_press_new(widget);
#endif
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(ev), 0);
  g_signal_connect(ev, "pressed", G_CALLBACK(ViewerEvButtonDown), d);
  g_signal_connect(ev, "released", G_CALLBACK(ViewerEvButtonUp), d);
}

static int
zoom_begin_obj(double x, double y, struct Viewer *d)
{
  int mx, my;
  double zoom;

  zoom = Menulocal.PaperZoom / 10000.0;
  mx = d->zoom_prm.x = calc_mouse_x(x, zoom, d);
  my = d->zoom_prm.y = calc_mouse_y(y, zoom, d);
  d->zoom_prm.focused = arraynum(d->focusobj);
  if (d->zoom_prm.focused < 1) {
    return FALSE;
  }
  GetLargeFrame(&(d->RefX2), &(d->RefY2), &(d->RefX1), &(d->RefY1), d);
  if (mx < d->RefX2 || mx > d->RefX1 || my < d->RefY2 || my > d->RefY1) {
    d->zoom_prm.focused = 0;
    return FALSE;
  }

  d->ShowRect = TRUE;
  d->ShowFrame = FALSE;
  return TRUE;
}

static void
zoom_begin(GtkGesture *gesture, GdkEventSequence *sequence, gpointer user_data)
{
  struct Viewer *d;
  double x, y;
  int dpi;

  d = (struct Viewer *) user_data;

  cancel_deceleration(d);
  gtk_gesture_get_bounding_box_center(gesture, &x, &y);

  d->zoom_prm.scale = 1;
  if (zoom_begin_obj(x, y, d)) {
    return;
  }
  if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &dpi) == -1) {
    return;
  }
  d->zoom_prm.x = x;
  d->zoom_prm.y = y;
  d->zoom_prm.dpi = dpi;
  ViewerZooming = TRUE;
}

static double
check_dpi_zoom_scale(int dpi, double scale)
{
  double new_dpi;

  new_dpi = dpi * scale;
  if (new_dpi < VIEWER_DPI_MIN) {
    new_dpi = VIEWER_DPI_MIN;
  } else if (new_dpi > VIEWER_DPI_MAX) {
    new_dpi = VIEWER_DPI_MAX;
  }
  return new_dpi / dpi;
}

static void
zoom_end_viewer(struct Viewer *d)
{
  int dpi;
  double scale;

  ViewerZooming = FALSE;

  if (ZoomLock) {
    return;
  }

  scale = check_dpi_zoom_scale(d->zoom_prm.dpi, d->zoom_prm.scale);
  dpi = d->zoom_prm.dpi * scale;
  if (dpi == d->zoom_prm.dpi) {
    return;
  }

  ZoomLock = TRUE;

  if (putobj(Menulocal.obj, "dpi", 0, &dpi) != -1) {
    d->hscroll -= (d->cx - d->zoom_prm.x) * (1 - 1.0 / scale);
    d->vscroll -= (d->cy - d->zoom_prm.y) * (1 - 1.0 / scale);
    ChangeDPI();
  }
  ZoomLock = FALSE;
}

static void
zoom_end(GtkGesture *gesture, GdkEventSequence *sequence, gpointer user_data)
{
  struct Viewer *d;
  char *objs[OBJ_MAX];

  d = (struct Viewer *) user_data;

  gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), NULL);
  if (d->zoom_prm.focused < 1) {
    zoom_end_viewer(d);
    return;
  }
  d->ShowRect = FALSE;
  d->ShowFrame = TRUE;
  clear_focus_obj_pix(d);
  if (zoom_focused_obj(d->zoom_prm.x, d->zoom_prm.y, d->zoom_prm.scale, d->zoom_prm.scale, objs, d)) {
    d->zoom_prm.focused = 0;
    return;
  }
  d->zoom_prm.focused = 0;
  UpdateAll(objs);
}

static void
zoom_cancel(GtkGesture *gesture, GdkEventSequence *sequence, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  ViewerZooming = FALSE;

  if (d->zoom_prm.focused > 0) {
    d->ShowRect = FALSE;
    d->ShowFrame = TRUE;
  }
}

static void
add_event_zoom(GtkWidget *widget, struct Viewer *d)
{
  GtkGesture *ev;
#if GTK_CHECK_VERSION(4, 0, 0)
  ev = gtk_gesture_zoom_new();
  gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(ev));
#else
  ev = gtk_gesture_zoom_new(widget);
#endif
  g_signal_connect(ev, "begin", G_CALLBACK(zoom_begin), d);
  g_signal_connect(ev, "end", G_CALLBACK(zoom_end), d);
  g_signal_connect(ev, "cancel", G_CALLBACK(zoom_cancel), d);
  g_signal_connect(ev, "scale-changed", G_CALLBACK(gesture_zoom), d);
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
add_event_motion(GtkWidget *widget, struct Viewer *d)
{
  GtkEventController *ev;

  ev = gtk_event_controller_motion_new();
  gtk_widget_add_controller(widget, ev);
  g_signal_connect(ev, "motion", G_CALLBACK(ViewerEvMouseMotion), d);
}

static void
add_event_scroll(GtkWidget *widget, struct Viewer *d)
{
  GtkEventController *ev;

  ev = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
  gtk_widget_add_controller(widget, ev);
  g_signal_connect(ev, "scroll", G_CALLBACK(ViewerEvScroll), d);
}
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean
hscroll_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
  struct Viewer *d;
  d = (struct Viewer *) user_data;
  start_scroll_deceleration(value, d->vscroll, d);
  return TRUE;
}

static gboolean
vscroll_change_value_cb(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
  struct Viewer *d;
  d = (struct Viewer *) user_data;
  start_scroll_deceleration(d->hscroll, value, d);
  return TRUE;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
draw_function(GtkDrawingArea* drawing_area, cairo_t* cr, int width, int height, gpointer user_data)
{
  ViewerEvPaint(GTK_WIDGET(drawing_area), cr, user_data);
}
#endif

void
ViewerWinSetup(void)
{
  struct Viewer *d;
  int width, height;
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkAdjustment *adj;
#else
  int x, y;
  GdkWindow *win;
#endif

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
  d->ignoreredraw = FALSE;
  d->KeyMask = 0;
  d->drag_prm.active = FALSE;
  d->deceleration_prm.id = 0;
  OpenGC();
  OpenGRA();
  SetScroller();
#if GTK_CHECK_VERSION(4, 0, 0)
  width = gtk_widget_get_size(NgraphApp.Viewer.Win, GTK_ORIENTATION_HORIZONTAL);
  height = gtk_widget_get_size(NgraphApp.Viewer.Win, GTK_ORIENTATION_VERTICAL);
#else
  win = gtk_widget_get_window(NgraphApp.Viewer.Win);
  gdk_window_get_position(win, &x, &y);
  width = gdk_window_get_width(win);
  height = gdk_window_get_height(win);
#endif
  d->cx = width / 2;
  d->cy = height / 2;
  d->focused_pix = NULL;

#if GTK_CHECK_VERSION(4, 0, 0)
  d->hscroll = scrollbar_get_value(d->HScroll);
  d->vscroll = scrollbar_get_value(d->VScroll);
#else
  d->hscroll = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->vscroll = gtk_range_get_value(GTK_RANGE(d->VScroll));
#endif

  ChangeDPI();

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(d->Win), draw_function, d, NULL);
  g_signal_connect(d->Win, "resize", G_CALLBACK(ViewerEvSize), d);
  g_signal_connect(d->Win, "realize", G_CALLBACK(ViewerEvRealize), d);
#else
  g_signal_connect(d->Win, "draw", G_CALLBACK(ViewerEvPaint), d);
  g_signal_connect(d->Win, "size-allocate", G_CALLBACK(ViewerEvSize), d);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(d->HScroll));
  g_signal_connect(adj, "value-changed", G_CALLBACK(ViewerEvHScroll), d);

  adj = gtk_scrollbar_get_adjustment(GTK_SCROLLBAR(d->VScroll));
  g_signal_connect(adj, "value-changed", G_CALLBACK(ViewerEvVScroll), d);

  gtk_widget_set_focusable(d->Win, TRUE);

  init_dnd(d);
#else
  g_signal_connect(d->HScroll, "value-changed", G_CALLBACK(ViewerEvHScroll), d);
  g_signal_connect(d->VScroll, "value-changed", G_CALLBACK(ViewerEvVScroll), d);
  g_signal_connect(d->HScroll, "change-value", G_CALLBACK(hscroll_change_value_cb), d);
  g_signal_connect(d->VScroll, "change-value", G_CALLBACK(vscroll_change_value_cb), d);
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
#endif
  gtk_widget_set_can_focus(d->Win, TRUE);

  if (d->popup) {
#if GTK_CHECK_VERSION(4, 0, 0)
    widget_set_parent(d->popup, d->Win);
#else
    gtk_menu_attach_to_widget(GTK_MENU(d->popup), GTK_WIDGET(d->Win), NULL);
#endif
  }

  add_event_drag(d->Win, d);
  add_event_key(d->Win, G_CALLBACK(ViewerEvKeyDown), G_CALLBACK(ViewerEvKeyUp),  d);
  add_event_button(d->Win, d);
  add_event_zoom(d->Win, d);
#if GTK_CHECK_VERSION(4, 0, 0)
  add_event_motion(d->Win, d);
  add_event_scroll(d->Win, d);
#else
  g_signal_connect(d->Win, "motion-notify-event", G_CALLBACK(ViewerEvMouseMotion), d);
  g_signal_connect(d->Win, "scroll-event", G_CALLBACK(ViewerEvScroll), d);
  g_signal_connect(d->Win, "popup-menu", G_CALLBACK(ev_popup_menu), d);
#endif

#if 0
  g_signal_connect(d->menu, "selection-done", G_CALLBACK(menu_activate), d);
#endif
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
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
viewer_win_file_update_response(int ret, gpointer user_data)
{
  struct narray *dfile;
  dfile = (struct narray *) user_data;
  arrayfree(dfile);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct fileupdate_data
{
  char *argv[7];
  struct savedstdio save;
  struct objlist *fileobj;
  int err, limit;
  int minx, miny, maxx, maxy;
  struct narray *dfile;
};

static void
fileupdate_finalize(gpointer user_data)
{
  struct fileupdate_data *data;
  char *objects[] = {"data", NULL};
  struct narray *dfile;
  struct objlist *fileobj;

  data = (struct fileupdate_data *) user_data;
  restorestdio(&data->save);
  dfile = data->dfile;
  fileobj = data->fileobj;
  update_file_obj_multi(fileobj, dfile, FALSE, viewer_win_file_update_response, dfile);
  UpdateAll(objects);
  g_free(data);
}

static void
fileupdate_func(gpointer user_data)
{
  struct objlist *fileobj;
  int snum, hidden;
  int i, did;
  N_VALUE *dinst;
  struct fileupdate_data *data;
  struct narray *eval, *dfile;
  int evalnum;

  data = (struct fileupdate_data *) user_data;

  dfile = data->dfile;
  fileobj = data->fileobj;
  snum = chkobjlastinst(fileobj) + 1;
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
    _exeobj(fileobj, "evaluate", dinst, 6, data->argv);
    _getobj(fileobj, "evaluate", dinst, &eval);
    evalnum = arraynum(eval) / 3;
    if (evalnum != 0) {
      arrayadd(dfile, &i);
    }
  }
}

static void
ViewerWinFileUpdate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  int snum, hidden;
  int did, limit;
  N_VALUE *dinst;
  int i;
  char mes[256];
  int ret;
  struct narray *dfile;
  struct fileupdate_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  ret = FALSE;

  data->minx = (x1 < x2) ? x1 : x2;
  data->miny = (y1 < y2) ? y1 : y2;
  data->maxx = (x1 > x2) ? x1 : x2;
  data->maxy = (y1 > y2) ? y1 : y2;
  data->limit = 1;
  data->err = err;

  fileobj = chkobject("data");
  if (! fileobj) {
    g_free(data);
    return;
  }

  if (check_drawrable(fileobj)) {
    g_free(data);
    return;
  }

  snum = chkobjlastinst(fileobj) + 1;
  if (snum == 0) {
    g_free(data);
    return;
  }

  dfile = arraynew(sizeof(int));
  if (dfile == NULL) {
    g_free(data);
    return;
  }

  ignorestdio(&data->save);

  snprintf(mes, sizeof(mes), _("Searching for data."));
  SetStatusBar(mes);

  data->argv[0] = (char *) &data->minx;
  data->argv[1] = (char *) &data->miny;
  data->argv[2] = (char *) &data->maxx;
  data->argv[3] = (char *) &data->maxy;
  data->argv[4] = (char *) &data->err;
  data->argv[5] = (char *) &data->limit;
  data->argv[6] = NULL;
  data->fileobj = fileobj;
  data->dfile = dfile;

  ProgressDialogCreate(_("Searching for data."), fileupdate_func, fileupdate_finalize, data);
}
#else
static int
ViewerWinFileUpdate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  char *argv[7];
  int snum, hidden;
  int did, limit;
  N_VALUE *dinst;
  struct narray *dfile;
  int i;
  struct narray *eval;
  int evalnum;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  char mes[256];
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

  if (check_drawrable(fileobj)) {
    goto End;
  }

  snum = chkobjlastinst(fileobj) + 1;
  if (snum == 0) {
    goto End;
  }

  dfile = arraynew(sizeof(int));
  if (dfile == NULL) {
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
      arrayadd(dfile, &i);
    }
  }

  ProgressDialogFinalize();
  ResetStatusBar();

  ret = update_file_obj_multi(fileobj, dfile, FALSE);
  arrayfree(dfile);
 End:
  restorestdio(&save);
  return ret;
}
#endif

static void
mask_selected_data(struct objlist *fileobj, int selnum, struct narray *sel_list)
{
  int i, j;
  struct narray *mask;

  for (i = 0; i < selnum; i++) {
    int masknum, sel;
    N_VALUE *inst;
    sel = arraynget_int(sel_list, i);
    getobj(fileobj, "mask", EvalList[sel].id, 0, NULL, &mask);

    if (mask == NULL) {
      mask = arraynew(sizeof(int));
      putobj(fileobj, "mask", EvalList[sel].id, mask);
    }

    masknum = arraynum(mask);

    inst = chkobjinst(fileobj, EvalList[sel].id);
    if (masknum == 0 || (arraynget_int(mask, masknum - 1)) < EvalList[sel].line) {
      arrayadd(mask, &(EvalList[sel].line));
      _exeobj(fileobj, "modified", inst, 0, NULL);
      set_graph_modified();
    } else if ((arraynget_int(mask, 0)) > EvalList[sel].line) {
      arrayins(mask, &(EvalList[sel].line), 0);
      _exeobj(fileobj, "modified", inst, 0, NULL);
      set_graph_modified();
    } else {
      if (bsearch_int(arraydata(mask), masknum, EvalList[sel].line, &j) == 0) {
	arrayins(mask, &(EvalList[sel].line), j);
	_exeobj(fileobj, "modified", inst, 0, NULL);
	set_graph_modified();
      }
    }
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
evaluate_response(struct response_callback *cb)
{
  int selnum;
  struct objlist *fileobj;
  struct Viewer *vd;
  struct EvalDialog *d;

  vd = (struct Viewer *) cb->data;
  d = (struct EvalDialog *) cb->dialog;
  fileobj = d->Obj;
  selnum = arraynum(&SelList);
  if (selnum > 0) {
    char *argv[2];
    switch (cb->return_value) {
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
      vd->Capture = TRUE;
      vd->MoveData = TRUE;
      break;
    }
  }
}

struct evaluate_data
{
  struct Viewer *d;
  struct objlist *fileobj;
  int minx, miny, maxx, maxy;
  int err, limit, snum, tot;
  char *argv[7];
};

static void
evaluate_finalize(gpointer user_data)
{
  struct evaluate_data *data;
  data = (struct evaluate_data *) user_data;
  ResetStatusBar();

  if (data->tot > 0) {
    EvalDialog(&DlgEval, data->fileobj, data->tot, &SelList);
    response_callback_add(&DlgEval, evaluate_response, NULL, data->d);
    DialogExecute(TopLevel, &DlgEval);
  }
  g_free(data);
}

static void
evaluate_main(gpointer user_data)
{
  int evalnum;
  int tot;
  int i, j, hidden;
  struct narray *eval;
  struct savedstdio save;
  double line, dx, dy;
  struct evaluate_data *data;

  data = (struct evaluate_data *) user_data;
  tot = 0;

  ignorestdio(&save);
  for (i = 0; i < data->snum; i++) {
    N_VALUE *dinst;
    dinst = chkobjinst(data->fileobj, i);
    if (dinst == NULL) {
      continue;
    }
    _getobj(data->fileobj, "hidden", dinst, &hidden);
    if (hidden) {
      continue;
    }
    _exeobj(data->fileobj, "evaluate", dinst, 6, data->argv);
    _getobj(data->fileobj, "evaluate", dinst, &eval);
    evalnum = arraynum(eval) / 3;
    for (j = 0; j < evalnum; j++) {
      if (tot >= data->limit) break;
      tot++;
      line = arraynget_double(eval, j * 3 + 0);
      dx = arraynget_double(eval, j * 3 + 1);
      dy = arraynget_double(eval, j * 3 + 2);
      EvalList[tot - 1].id = i;
      EvalList[tot - 1].line = nround(line);
      EvalList[tot - 1].x = dx;
      EvalList[tot - 1].y = dy;
    }
    if (tot >= data->limit) break;
  }
  data->tot = tot;
  restorestdio(&save);
}

static void
Evaluate(int x1, int y1, int x2, int y2, int err, struct Viewer *d)
{
  struct objlist *fileobj;
  int snum;
  char mes[256];
  struct evaluate_data *data;

  if ((fileobj = chkobject("data")) == NULL) {
    return;
  }

  if (check_drawrable(fileobj)) {
    return;
  }

  snum = chkobjlastinst(fileobj) + 1;
  if (snum == 0) {
    return;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->minx = (x1 < x2) ? x1 : x2;
  data->miny = (y1 < y2) ? y1 : y2;
  data->maxx = (x1 > x2) ? x1 : x2;
  data->maxy = (y1 > y2) ? y1 : y2;

  data->limit = EVAL_NUM_MAX;

  data->argv[0] = (char *) &data->minx;
  data->argv[1] = (char *) &data->miny;
  data->argv[2] = (char *) &data->maxx;
  data->argv[3] = (char *) &data->maxy;
  data->argv[4] = (char *) &data->err;
  data->argv[5] = (char *) &data->limit;
  data->argv[6] = NULL;

  data->fileobj = fileobj;
  data->snum = snum;
  data->d = d;

  snprintf(mes, sizeof(mes), _("Evaluating."));
  SetStatusBar(mes);

  ProgressDialogCreate(_("Evaluating"), evaluate_main, evaluate_finalize, data);
}
#else
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

#if ! GTK_CHECK_VERSION(4, 0, 0)
  ProgressDialogFinalize();
#endif
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
struct trimming_data {
  int x1, y1, x2, y2;
};

static void
trimming_response(struct response_callback *cb)
{
  struct SelectDialog *sel;
  struct narray *farray;
  struct objlist *obj;
  struct trimming_data *data;
  int x1, y1, x2, y2;
  int maxx, maxy, minx, miny;

  data = (struct trimming_data *) cb->data;
  x1 = data->x1;
  x2 = data->x2;
  y1 = data->y1;
  y2 = data->y2;
  sel = (struct SelectDialog *) cb->dialog;
  farray = sel->sel;
  obj = sel->Obj;

  if (cb->return_value == IDOK) {
    int i;
    int *array, num;
    int vx1, vy1, vx2, vy2;
    char *argv[4];
    vx1 = x1 - x2;
    vy1 = y1 - y2;
    vx2 = x2 - x1;
    vy2 = y1 - y2;

    num = arraynum(farray);
    array = arraydata(farray);

    if (num > 0) {
      menu_save_undo_single(UNDO_TYPE_TRIMMING, obj->name);
    }
    for (i = 0; i < num; i++) {
      double ax, ay, ip1, ip2, min, max;
      int id, dir;
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
	  int room;
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
    argv[0] = "data";
    argv[1] = "axis";
    argv[2] = "axisgrid";
    argv[3] = NULL;
    UpdateAll(argv);
  }
  arrayfree(farray);
  g_free(data);
}

static void
Trimming(int x1, int y1, int x2, int y2)
{
  struct narray *farray;
  struct objlist *obj;
  struct trimming_data *data;

  if ((x1 == x2) && (y1 == y2))
    return;

  if ((obj = chkobject("axis")) == NULL)
    return;

  if (chkobjlastinst(obj) == -1)
    return;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    g_free(data);
    return;
  }
  data->x1 = x1;
  data->x2 = x2;
  data->y1 = y1;
  data->y2 = y2;
  SelectDialog(&DlgSelect, obj, _("trimming (multi select)"), AxisCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, trimming_response, NULL, data);
  DialogExecute(TopLevel, &DlgSelect);
}
#else
static void
Trimming(int x1, int y1, int x2, int y2)
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
    argv[0] = "data";
    argv[1] = "axis";
    argv[2] = "axisgrid";
    argv[3] = NULL;
    UpdateAll(argv);
  }
  arraydel(&farray);
}
#endif

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

  set_pointer_mode(PointerModeFocus);
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

static void
set_pointer_mode_by_obj(struct objlist *obj)
{
  int pointer_mode;
  const char *name;

  name = chkobjectname(obj);
  if (g_strcmp0(name, "axis")) {
    pointer_mode = PointerModeFocusLegend;
  } else {
    pointer_mode = PointerModeFocusAxis;
  }
  set_pointer_mode(pointer_mode);
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
  set_pointer_mode_by_obj(obj);
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
show_focus_elements(cairo_t *cr, const struct Viewer *d, double zoom, int *data, int num)
{
  int j;
  for (j = 0; j < num; j += 2) {
    int x1, y1;
    x1 = coord_conv_x((data[j] + d->FrameOfsX), zoom, d);
    y1 = coord_conv_y((data[j + 1] + d->FrameOfsY), zoom, d);

    cairo_rectangle(cr,
		    x1 - FOCUS_RECT_SIZE / 2 - CAIRO_COORDINATE_OFFSET,
		    y1 - FOCUS_RECT_SIZE / 2 - CAIRO_COORDINATE_OFFSET,
		    FOCUS_RECT_SIZE,
		    FOCUS_RECT_SIZE);
  }
  cairo_fill(cr);
}

static void
ShowFocusFrame(cairo_t *cr, struct Viewer *d)
{
  int i, num;
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
      show_focus_elements(cr, d, zoom, bbox + 4, bboxnum - 4);
    }
  }

  if (d->MouseMode ==  MOUSEDRAG && (d->FrameOfsX  || d->FrameOfsY)) {
    draw_focused_move(cr, zoom, d);
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
    a += 36000;
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
show_focus_line_set_dash(cairo_t *cr, struct narray *style, double zoom)
{
  int *data;
  double *tmp;
  int i, n;

  n = arraynum(style);
  if (n < 2) {
    cairo_set_dash(cr, NULL, 0, 0);
    return;
  }
  tmp = g_malloc(sizeof(*tmp) * n);
  if (tmp == NULL) {
    return;
  }
  data = arraydata(style);
  for (i = 0; i < n; i++) {
    tmp[i] = mxd2p(data[i] * zoom);
  }
  cairo_set_dash(cr, tmp, n, 0);
  g_free(tmp);
}

static void
set_support_attribute(cairo_t *cr)
{
  double dash[] = {DOT_LENGTH};
  cairo_set_line_width(cr, 1);
  cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
}

static void
show_focus_line_common(cairo_t *cr, double zoom, struct objlist *obj, N_VALUE *inst, const struct Point *expo, int close_path)
{
  int stroke, sr, sg, sb, fill, fr, fg, fb, width, join;
  struct narray *style;

  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "fill_R", inst, &fr);
  _getobj(obj, "fill_G", inst, &fg);
  _getobj(obj, "fill_B", inst, &fb);
  _getobj(obj, "stroke", inst, &stroke);
  _getobj(obj, "stroke_R", inst, &sr);
  _getobj(obj, "stroke_G", inst, &sg);
  _getobj(obj, "stroke_B", inst, &sb);
  _getobj(obj, "width", inst, &width);
  _getobj(obj, "style", inst, &style);
  _getobj(obj, "join", inst, &join);

  show_focus_line_set_dash(cr, style, zoom);
  if (stroke) {
    cairo_set_line_join(cr, join);
    cairo_set_line_width(cr, mxd2p(width * zoom));
    cairo_set_source_rgba(cr, sr / 255.0, sg / 255.0, sb / 255.0, EDITING_OPACITY);
    cairo_stroke_preserve(cr);
  }
  if (fill) {
    if (expo) {
      cairo_line_to(cr, expo->x, expo->y);
    }
    cairo_set_source_rgba(cr, fr / 255.0, fg / 255.0, fb / 255.0, EDITING_OPACITY);
    cairo_fill_preserve(cr);
  }
  if (close_path) {
    cairo_close_path(cr);
  }
  set_support_attribute(cr);
  cairo_stroke(cr);
}

static void
show_focus_line_arc(cairo_t *cr, int change, double zoom, struct objlist *obj, N_VALUE *inst, struct Viewer *d)
{
  int x, y, rx, ry, fill, pie_slice, a1, a2, close_path;
  int stroke;
  struct Point expo, *poptr;
  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "rx", inst, &rx);
  _getobj(obj, "ry", inst, &ry);
  _getobj(obj, "angle1", inst, &a1);
  _getobj(obj, "angle2", inst, &a2);
  _getobj(obj, "close_path", inst, &close_path);
  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "stroke", inst, &stroke);
  _getobj(obj, "pieslice", inst, &pie_slice);

  if (! stroke) {
    close_path = TRUE;
  }

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

  poptr = NULL;
  if (rx > 0 && ry > 0) {
    rx = mxd2p(rx * zoom);
    ry = mxd2p(ry * zoom);
    x = coord_conv_x(x, zoom, d);
    y = coord_conv_y(y, zoom, d);
    cairo_save(cr);
    draw_cairo_arc(cr, x, y, rx, ry, a1, a2);
    if (close_path) {
      if (pie_slice) {
	cairo_line_to(cr, x, y);
      }
      cairo_close_path(cr);
    } else {
      if (pie_slice) {
	expo.x = x;
	expo.y = y;
	poptr = &expo;
      }
    }
    show_focus_line_common(cr, zoom, obj, inst, poptr, TRUE);
    cairo_restore(cr);
  }
}

static void
draw_frame_rect(cairo_t *gc, int change, double zoom, int *bbox, struct objlist *obj, N_VALUE *inst, const struct Viewer *d)
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
  show_focus_line_common(gc, zoom, obj, inst, NULL, FALSE);
}

static void
draw_focus_axis(cairo_t *gc, int change, double zoom, int *bbox, struct objlist *obj, N_VALUE *inst, const struct Viewer *d)
{
  int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  int minx, miny, height, width, single_axis;
  char *group;

  _getobj(obj, "group", inst, &group);
  single_axis = (group == NULL || group[0] == 'a');
  switch (change) {
  case 0:
    x1 = coord_conv_x(bbox[4] + d->LineX, zoom, d);
    y1 = coord_conv_y(bbox[5] + d->LineY, zoom, d);
    if (single_axis) {
      x2 = coord_conv_x(bbox[6], zoom, d);
      y2 = coord_conv_y(bbox[7], zoom, d);
    } else {
      x2 = coord_conv_x(bbox[8], zoom, d);
      y2 = coord_conv_y(bbox[9], zoom, d);
    }
    break;
  case 1:
    x1 = coord_conv_x(bbox[4], zoom, d);
    if (single_axis) {
      y1 = coord_conv_y(bbox[5], zoom, d);
      x2 = coord_conv_x(bbox[6] + d->LineX, zoom, d);
      y2 = coord_conv_y(bbox[7] + d->LineY, zoom, d);
    } else {
      y1 = coord_conv_y(bbox[5] + d->LineY, zoom, d);
      x2 = coord_conv_x(bbox[8] + d->LineX, zoom, d);
      y2 = coord_conv_y(bbox[9], zoom, d);
    }
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

  set_support_attribute(gc);
  if (single_axis) {
    cairo_move_to(gc, x1, y1);
    cairo_line_to(gc, x2, y2);
  } else {
    cairo_rectangle(gc, minx, miny, width, height);
  }
  cairo_stroke(gc);
}

static int
draw_curve_common(cairo_t *cr, int *data, int num, int intp)
{
  struct narray expand_points;
  int *cdata;
  int r, n, i;
  arrayinit(&expand_points, sizeof(int));
  r = curve_expand_points(data, num, intp, &expand_points);
  if (r) {
    return 1;
  }
  n = arraynum(&expand_points) / 2;
  if (n < 1) {
    arraydel(&expand_points);
    g_free(data);
    return 1;
  }
  cdata = arraydata(&expand_points);
  cairo_move_to(cr, cdata[0], cdata[1]);
  for (i = 1; i < n; i += 1) {
    cairo_line_to(cr, cdata[i * 2], cdata[i * 2 + 1]);
  }
  arraydel(&expand_points);
  return 0;
}

static int
draw_focus_curve(cairo_t *cr, int *po, int num, int intp, double zoom, const struct Viewer *d)
{
  int *data;
  int i, r;
  if (num < 6) {
    return 1;
  }
  if (po[num - 1] == po[num - 3] && po[num -2] == po[num - 4]) {
    num -= 2;
    if (num < 6) {
      return 1;
    }
  }
  data = g_malloc(sizeof(*data) * num);
  if (data == NULL) {
    return 1;
  }
  for (i = 0; i < num / 2; i++) {
    data[i * 2] = coord_conv_x(po[i * 2], zoom, d);
    data[i * 2 + 1] = coord_conv_y(po[i * 2 + 1], zoom, d);
  }
  r = draw_curve_common(cr, data, num / 2, intp);
  g_free(data);
  return r;
}

static void
draw_focus_line(cairo_t *gc, int change, double zoom, int bboxnum, int *bbox, struct objlist *obj, N_VALUE *inst, const struct Viewer *d)
{
  int j, ofsx, ofsy, intp, type, r, fill_rule;
  int *data;
  int fill, close_path;

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "interpolation", inst, &intp);
  data = g_malloc(sizeof(*data) * (bboxnum - 4));
  if (data == NULL) {
    return;
  }

  for (j = 0; j < bboxnum - 4; j += 2) {
    if (change == j / 2) {
      ofsx = d->LineX;
      ofsy = d->LineY;
    } else {
      ofsx = 0;
      ofsy = 0;
    }
    data[j] = bbox[j + 4] + ofsx;
    data[j + 1] = bbox[j + 5] + ofsy;
  }
  r = TRUE;
  if (type) {
    r = draw_focus_curve(gc, data, bboxnum - 4, intp, zoom, d);
  }
  if (r) {
    for (j = 0; j < bboxnum - 4; j += 2) {
      int x1, y1;
      x1 = coord_conv_x(data[j], zoom, d);
      y1 = coord_conv_y(data[j + 1], zoom, d);
      if (j == 0) {
	cairo_move_to(gc, x1, y1);
      } else {
	cairo_line_to(gc, x1, y1);
      }
    }
  }
  _getobj(obj, "fill", inst, &fill);
  _getobj(obj, "close_path", inst, &close_path);
  _getobj(obj, "fill_rule", inst, &fill_rule);
  cairo_set_fill_rule(gc, (fill_rule) ? CAIRO_FILL_RULE_WINDING : CAIRO_FILL_RULE_EVEN_ODD);
  show_focus_line_common(gc, zoom, obj, inst, NULL, fill || close_path);
  set_support_attribute(gc);
  show_focus_elements(gc, d, zoom, data, bboxnum - 4);
  g_free(data);
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
  double dash[] = {DOT_LENGTH};

  ignorestdio(&save);

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);

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
    draw_frame_rect(cr, d->ChangePoint, zoom, bbox, focus[0]->obj, inst, d);
  } else if (focus[0]->obj == chkobject("arc")) {
    show_focus_line_arc(cr, d->ChangePoint, zoom, focus[0]->obj, inst, d);
  } else if (focus[0]->obj == chkobject("path")) {
    draw_focus_line(cr, d->ChangePoint, zoom, bboxnum, bbox, focus[0]->obj, inst, d);
  } else if (focus[0]->obj == chkobject("axis")) {
    draw_focus_axis(cr, d->ChangePoint, zoom, bbox, focus[0]->obj, inst, d);
  }

 End:
  restorestdio(&save);
}

static void
set_dash(cairo_t *cr, struct presettings *setting, double zoom)
{
  int style, i;
  double dash[LINE_STYLE_ELEMENT_MAX];
  struct line_style *line_style;

  style = setting->line_style;
  if (style == 0) {
    cairo_set_dash(cr, NULL, 0, 0);
    return;
  }

  line_style = FwLineStyle + style;
  for (i = 0; i < line_style->num; i++) {
    dash[i] = mxd2p(line_style->nlist[i] * zoom);
  }
  cairo_set_dash(cr, dash, line_style->num, 0);
}

static void
draw_points(cairo_t *cr, const struct Viewer *d, struct Point **po, int num, double zoom)
{
  int i;
  for (i = 0; i < num; i++) {
    int x1, y1;
    x1 = coord_conv_x(po[i]->x, zoom, d);
    y1 = coord_conv_y(po[i]->y, zoom, d);

    cairo_move_to(cr, x1 - (POINT_LENGTH - 1), y1);
    cairo_line_to(cr, x1 + POINT_LENGTH, y1);

    cairo_move_to(cr, x1, y1 - (POINT_LENGTH - 1));
    cairo_line_to(cr, x1, y1 + POINT_LENGTH);
  }
  cairo_set_dash(cr, NULL, 0, 0);
  cairo_set_source_rgba(cr, GRAY, GRAY, GRAY, 1);
  cairo_set_line_width(cr, 1);
  cairo_stroke(cr);
}

static int
draw_points_curve(cairo_t *cr, const struct Viewer *d, struct Point **po, int num, int intp, double zoom)
{
  int *data;
  int i, r;
  if (num < 3 || intp < 0) {
    return 1;
  }
  if (po[num -1]->x == po[num - 2]->x && po[num -1]->y == po[num - 2]->y) {
    num--;
    if (num < 3) {
      return 1;
    }
  }
  data = g_malloc(sizeof(*data) * num * 2);
  if (data == NULL) {
    return 1;
  }
  for (i = 0; i < num; i++) {
    data[i * 2] = coord_conv_x(po[i]->x, zoom, d);
    data[i * 2 + 1] = coord_conv_y(po[i]->y, zoom, d);
  }
  r = draw_curve_common(cr, data, num, intp);
  g_free(data);
  return r;
}

static void
ShowPoints(cairo_t *cr, const struct Viewer *d)
{
  int num, x1, y1, stroke, fill;
  struct Point **po;
  double zoom, lw;
  double r, g, b;
  struct presettings setting;

  presetting_get(&setting);
  num = arraynum(d->points);
  po = arraydata(d->points);

  zoom = Menulocal.PaperZoom / 10000.0;

  r = setting.r1 / 255.0;
  g = setting.g1 / 255.0;
  b = setting.b1 / 255.0;

  cairo_save(cr);
  lw = mxd2p(setting.line_width * zoom);
  fill = setting.fill;
  stroke = setting.stroke;
  if ((setting.fill == 0 && setting.stroke == 0) || d->Mode == GaussB) {
    double dash[] = {DOT_LENGTH};
    cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);
    stroke = TRUE;
    fill = FALSE;
    r = g = b = GRAY;
    lw = 1;
  } else {
    set_dash(cr, &setting, zoom);
  }
  if (d->Mode & POINT_TYPE_DRAW1) {
    if (num >= 2) {
      int x2, y2;
      int minx, miny, height, width;

      x1 = coord_conv_x(po[0]->x, zoom, d);
      y1 = coord_conv_y(po[0]->y, zoom, d);
      x2 = coord_conv_x(po[1]->x, zoom, d);
      y2 = coord_conv_y(po[1]->y, zoom, d);

      minx = (x1 < x2) ? x1 : x2;
      miny = (y1 < y2) ? y1 : y2;

      width = abs(x2 - x1);
      height = abs(y2 - y1);

      switch (d->Mode) {
      case ArcB:
	draw_cairo_arc(cr, minx + width / 2, miny + height / 2, width / 2, height / 2, 0, 36000);
	break;
      case CrossB:
	fill = FALSE;
	cairo_move_to(cr, minx, miny);
	cairo_line_to(cr, minx, miny + height);
	cairo_line_to(cr, minx + width, miny + height);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	break;
      case FrameB:
      case SectionB:
	fill = FALSE;
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	/* fall-through */
      case GaussB:
      case RectB:
      default:
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
	cairo_rectangle(cr, minx, miny, width, height);
      }

      if (fill) {
	cairo_set_source_rgba(cr, setting.r2 / 255.0, setting.g2 / 255.0, setting.b2 / 255.0, EDITING_OPACITY);
	if (stroke) {
	  cairo_fill_preserve(cr);
	} else {
	  cairo_fill(cr);
	}
      }
      if (stroke) {
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
	cairo_set_line_width(cr, lw);
	cairo_set_source_rgba(cr, r, g, b, EDITING_OPACITY);
	cairo_stroke(cr);
      }
    }
  } else {
    if (num >= 1) {
      int line;
      line = draw_points_curve(cr, d, po, num, setting.interpolation, zoom);
      if (line) {
        int i;
	x1 = coord_conv_x(po[0]->x, zoom, d);
	y1 = coord_conv_y(po[0]->y, zoom, d);
	cairo_move_to(cr, x1, y1);
	for (i = 1; i < num; i++) {
	  x1 = coord_conv_x(po[i]->x, zoom, d);
	  y1 = coord_conv_y(po[i]->y, zoom, d);

	  cairo_line_to(cr, x1, y1);
	}
      }
      if (d->Mode == SingleB) {
	fill = FALSE;
	stroke = TRUE;
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
      } else {
	if (setting.close_path) {
	  cairo_close_path(cr);
	}
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
      }
      if (fill) {
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_set_source_rgba(cr, setting.r2 / 255.0, setting.g2 / 255.0, setting.b2 / 255.0, EDITING_OPACITY);
	if (stroke) {
	  cairo_fill_preserve(cr);
	} else {
	  cairo_fill(cr);
	}
      }
      if (stroke) {
	cairo_set_line_join(cr, (cairo_line_join_t) setting.join);
	cairo_set_line_width(cr, lw);
	cairo_set_source_rgba(cr, r, g, b, EDITING_OPACITY);
	cairo_stroke(cr);
      }
    }
  }
  draw_points(cr, d, po, num, zoom);
  cairo_restore(cr);
}

static void
draw_focused_axis(struct objlist *obj, int id, int argc, char **argv)
{
  int idx, idy, idu, idr, idg;
  int findX, findY, findU, findR, findG;
  char *group;

  getobj(obj, "group", id, 0, NULL, &group);
  if (group == NULL || group[0] == 'a') {
    exeobj(obj, "draw", id, argc, argv);
    return;
  }

  search_axis_group(obj, id, group,
                    &findX, &findY, &findU, &findR, &findG,
                    &idx, &idy, &idu, &idr, &idg);
  if (findX) {
    exeobj(obj, "draw", idx, argc, argv);
  }
  if (findY) {
    exeobj(obj, "draw", idy, argc, argv);
  }
  if (findU) {
    exeobj(obj, "draw", idu, argc, argv);
  }
  if (findR) {
    exeobj(obj, "draw", idr, argc, argv);
  }
  if (findG) {
    struct objlist *dobj;
    dobj = chkobject("axisgrid");
    exeobj(dobj, "draw", idg, argc, argv);
  }
}

static void
clear_focus_obj_pix(struct Viewer *d)
{
  if (d->focused_pix) {
    cairo_surface_destroy(d->focused_pix);
  }
  d->focused_pix = NULL;
}

static void
draw_focused_each_obj(struct Viewer *d, int GC)
{
  char *argv[2];
  struct objlist *aobj;
  struct savedstdio save;
  int i, num;
  aobj = getobject("axis");
  argv[0]=(char *)&GC;
  argv[1]=NULL;
  ignorestdio(&save);
  num = arraynum(d->focusobj);
  for (i = 0; i < num; i++) {
    struct FocusObj *focus;
    struct objlist *obj;
    N_VALUE *inst;
    int id;
    focus = *(struct FocusObj **) arraynget(d->focusobj, i);
    if (focus == NULL) {
      continue;
    }
    inst = chkobjinstoid(focus->obj, focus->oid);
    if (inst == NULL) {
      continue;
    }
    obj = focus->obj;
    _getobj(obj, "id", inst, &id);
    if (obj == aobj) {
      draw_focused_axis(obj, id, 1, argv);
    } else {
      exeobj(obj, "draw", id, 1, argv);
    }
  }
  _GRAclose(GC);
  restorestdio(&save);
}

static void
draw_focused_obj(struct Viewer *d)
{
  cairo_surface_t *pix;
  int GC, id, found, output, w, h, num, dpi, dpi_org, a;
  struct objlist *obj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;

  num = arraynum(d->focusobj);
  if (num < 1) {
    return;
  }
  found = find_gra2gdk_inst(&obj, &inst, &robj, &output, &local);
  if (! found) {
    return;
  }

  if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &dpi) == -1) {
    return;
  }
  w = cairo_image_surface_get_width(Menulocal.bg);
  h = cairo_image_surface_get_height(Menulocal.bg);
  pix = gra2gdk_create_pixmap(local, w, h, -1, -1, -1);
  if (pix == NULL) {
    return;
  }
  _getobj(obj, "id", inst, &id);
  a = 255 * EDITING_OPACITY;
  putobj(obj, "force_opacity", id, &a);
  getobj(obj, "dpi", id, 0, NULL, &dpi_org);
  putobj(obj, "dpi", id, &dpi);
  GC = _GRAopen("gra2gdk", "_output", robj, inst, output, -1, -1, -1, NULL, local);
  if (GC < 0) {
    cairo_surface_destroy(pix);
    return;
  }
  GRAinit(GC, Menulocal.LeftMargin, Menulocal.TopMargin, Menulocal.PaperWidth, Menulocal.PaperHeight, Menulocal.PaperZoom);
  GRAview(GC, 0, 0, Menulocal.PaperWidth, Menulocal.PaperHeight, 0);
  draw_focused_each_obj(d, GC);
  a = 0;
  putobj(obj, "force_opacity", id, &a);
  putobj(obj, "dpi", id, &dpi_org);
  _GRAclose(GC);
  if (d->focused_pix) {
    cairo_surface_destroy(d->focused_pix);
  }
  d->focused_pix = pix;
}

static void
draw_focused_zoom(cairo_t *cr, int px, int py, double zoom, struct Viewer *d)
{
  cairo_pattern_t *pattern;
  cairo_matrix_t matrix;
  int cx, cy;

  if (d->focused_pix == NULL) {
    draw_focused_obj(d);
    if (d->focused_pix == NULL) {
      return;
    }
  }
  cx = coord_conv_x(px, zoom, d);
  cy = coord_conv_y(py, zoom, d);
  pattern = cairo_pattern_create_for_surface(d->focused_pix);
  cairo_matrix_init_identity(&matrix);
  cairo_matrix_translate(&matrix,
			 cx + d->hscroll - d->cx - cx / d->ZoomX,
			 cy + d->vscroll - d->cy - cy / d->ZoomY);
  cairo_matrix_scale(&matrix, 1.0 / d->ZoomX, 1.0 / d->ZoomY);
  cairo_set_source(cr, pattern);
  cairo_pattern_set_matrix(pattern, &matrix);
  cairo_paint(cr);
  cairo_pattern_destroy(pattern);
}

static void
draw_focused_move(cairo_t *cr, double zoom, struct Viewer *d)
{
  int x, y;

  if (d->focused_pix == NULL) {
    draw_focused_obj(d);
    if (d->focused_pix == NULL) {
      return;
    }
  }
  x = mxd2p(d->FrameOfsX) * zoom;
  y = mxd2p(d->FrameOfsY) * zoom;
  cairo_set_source_surface(cr, d->focused_pix,
			   nround(- d->hscroll + d->cx + x),
			   nround(- d->vscroll + d->cy + y));
  cairo_paint(cr);
}

static void
ShowFrameRect(cairo_t *cr, struct Viewer *d)
{
  int x1, y1, x2, y2;
  double zoom;
  int minx, miny, width, height;
  double dash[] = {DOT_LENGTH};

  if (d->MouseX1 == d->MouseX2 && d->MouseY1 == d->MouseY2) {
    return;
  }

  zoom = Menulocal.PaperZoom / 10000.0;

  if (d->zoom_prm.focused > 0) {
    d->ZoomX = d->zoom_prm.scale;
    d->ZoomY = d->zoom_prm.scale;
    draw_focused_zoom(cr, d->zoom_prm.x, d->zoom_prm.y, zoom, d);
  } else if (d->MouseMode == MOUSEZOOM1 ||
	     d->MouseMode == MOUSEZOOM2 ||
	     d->MouseMode == MOUSEZOOM3 ||
	     d->MouseMode == MOUSEZOOM4) {
    draw_focused_zoom(cr, d->MouseX1, d->MouseY1, zoom, d);
  }

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, dash, sizeof(dash) / sizeof(*dash), 0);

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
}

static void
ShowCrossGauge(cairo_t *cr, const struct Viewer *d)
{
  int x, y, width, height;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  width = gtk_widget_get_size(d->Win, GTK_ORIENTATION_HORIZONTAL);
  height = gtk_widget_get_size(d->Win, GTK_ORIENTATION_VERTICAL);
#else
  width = gdk_window_get_width(win);
  height = gdk_window_get_height(win);
#endif

  x = d->CrossX;
  y = d->CrossY;

  cairo_set_source_rgb(cr, GRAY, GRAY, GRAY);
  cairo_set_dash(cr, NULL, 0, 0);

  cairo_move_to(cr, x, 0);
  cairo_line_to(cr, x, height);

  cairo_move_to(cr, 0, y);
  cairo_line_to(cr, width, y);
  cairo_stroke(cr);
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
mouse_down_point(unsigned int state, struct Viewer *d)
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

static int
check_zoom_obj(struct Viewer *d)
{
  char *objs[OBJ_MAX];
  int i, n;
  n = get_focused_obj_array(d->focusobj, objs);
  if (n > 2) {
    return FALSE;
  }

  for (i = 0; i < n; i++) {
    if (g_strcmp0(objs[i], "text") && g_strcmp0(objs[i], "mark")) {
      return FALSE;
    }
  }
  return TRUE;
}

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
    if (check_zoom_obj(d)) {
      *zoom_x = zoom2;
      *zoom_y = zoom2;
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
  d->ZoomX = 1;
  d->ZoomY = 1;
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
    range_increment_deceleration(- d->cx + point->x, - d->cy + point->y, d);
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
gesture_zoom_obj(gdouble scale, double gx, double gy,  struct Viewer *d)
{
  int x, y, minx, miny, maxx, maxy;
  double zoom;

  zoom = Menulocal.PaperZoom / 10000.0;

  minx = d->RefX2;
  maxx = d->RefX1;
  miny = d->RefY2;
  maxy = d->RefY1;

  x = d->zoom_prm.x = calc_mouse_x(gx, zoom, d);
  y = d->zoom_prm.y = calc_mouse_y(gy, zoom, d);
  if (x < minx) {
    x = minx;
  } else if (x > maxx) {
    x = maxx;
  }
  if (y < miny) {
    y = miny;
  } else if (y > maxy) {
    y = maxy;
  }
  d->MouseX1 = x - (x - minx) * scale;
  d->MouseY1 = y - (y - miny) * scale;
  d->MouseX2 = x + (maxx - x) * scale;
  d->MouseY2 = y + (maxy - y) * scale;

  gtk_widget_queue_draw(d->Win);
}

static void
draw_zoom(cairo_t *cr, struct Viewer *d)
{
  cairo_pattern_t *pattern;
  cairo_matrix_t matrix;
  double scale;

  scale = 1.0 / check_dpi_zoom_scale(d->zoom_prm.dpi, d->zoom_prm.scale);
  cairo_matrix_init(&matrix,
		    scale, 0,
		    0, scale,
		    d->zoom_prm.x + d->hscroll - d->cx - d->zoom_prm.x * scale,
		    d->zoom_prm.y + d->vscroll - d->cy - d->zoom_prm.y * scale);

  pattern = cairo_pattern_create_for_surface(Menulocal.bg);
  cairo_set_source(cr, pattern);
  cairo_pattern_set_matrix(pattern, &matrix);
  cairo_paint(cr);
  cairo_pattern_destroy(pattern);

  pattern = cairo_pattern_create_for_surface(Menulocal.pix);
  cairo_set_source(cr, pattern);
  cairo_pattern_set_matrix(pattern, &matrix);
  cairo_paint(cr);
  cairo_pattern_destroy(pattern);
}

static void
show_gesture_zooming_scale(double scale)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "% .2f%%", scale * 100);
  gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
}

static void
gesture_zoom(GtkGestureZoom *controller, gdouble scale, gpointer user_data)
{
  struct Viewer *d;
  double x, y;
  int dpi;

  d = (struct Viewer *) user_data;

  gtk_gesture_get_bounding_box_center(GTK_GESTURE(controller), &x, &y);
  if (d->zoom_prm.focused > 0) {
    d->zoom_prm.scale = scale;
    show_gesture_zooming_scale(scale);
    gesture_zoom_obj(scale, x, y, d);
    return;
  }

  d->zoom_prm.x = x;
  d->zoom_prm.y = y;

  dpi = d->zoom_prm.dpi * scale;
  d->zoom_prm.scale = 1.0 * dpi / d->zoom_prm.dpi;
  show_gesture_zooming_scale(d->zoom_prm.scale);

  main_window_redraw();
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
	mouse_down_point(state, d);
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
    Trimming(x1, y1, x2, y2);
    break;
  case DataB:
#if GTK_CHECK_VERSION(4, 0, 0)
    ViewerWinFileUpdate(x1, y1, x2, y2, err);
#else
    if (ViewerWinFileUpdate(x1, y1, x2, y2, err)) {
      char *objects[] = {"data", NULL};
      UpdateAll(objects);
    }
#endif
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
  if (dx == 0 && dy == 0) {
    return;
  }
  get_focused_obj_array(d->focusobj, objs);
  axis = move_objects(dx, dy, d, objs);
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

static int
zoom_focused_obj(int x, int y, double zoom_x, double zoom_y, char **objs, struct Viewer *d)
{
  struct FocusObj *focus;
  int i, num, zmx, zmy;
  char *argv[6];

  zmx = check_zoom(zoom_x);
  zmy = check_zoom(zoom_y);

  objs[0] = NULL;

  if (zmx < 0 || zmy < 0) {
    return 1;
  }

  if (zmx == 10000 && zmy == 10000) {
    return 1;
  }

  argv[0] = (char *) &zmx;
  argv[1] = (char *) &zmy;
  argv[2] = (char *) &x;
  argv[3] = (char *) &y;
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
    inst = chkobjinstoid(focus->obj, focus->oid);
    if (inst) {
      _exeobj(obj, "zooming", inst, 5, argv);
      set_graph_modified();
    }
  }

  PaintLock = FALSE;
  return 0;
}

static void
mouse_up_zoom(unsigned int state, TPoint *point, double zoom, struct Viewer *d)
{
  int vx1, vy1, preserve_ratio;
  double zoom_x, zoom_y;
  char *objs[OBJ_MAX];

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

  d->FrameOfsX = d->FrameOfsY = 0;
  d->ShowFrame = TRUE;

  if (zoom_focused_obj(d->RefX1, d->RefY1, zoom_x, zoom_y, objs, d)) {
    gtk_widget_queue_draw(d->Win);
    return;
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

  clear_focus_obj_pix(d);
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

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
struct create_drawble_data {
  struct objlist *obj, *obj2;
  int id, undo;
};

static struct create_drawble_data *
create_drawble_data_new(struct objlist *obj, struct objlist *obj2, int id, int undo)
{
  struct create_drawble_data *data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  data->obj = obj;
  data->obj2 = obj2;
  data->id = id;
  data->undo = undo;
  return data;
}

static void
create_drawble_response(struct response_callback *cb)
{
  struct create_drawble_data *data;
  char *objects[3];
  struct Viewer *d;

  d = &NgraphApp.Viewer;
  data = (struct create_drawble_data *) cb->data;
  if (data == NULL) {
    PaintLock = FALSE;
    return;
  }

  if (cb->return_value != IDOK) {
    if (data->id < 0) {
      menu_undo_internal(data->undo);
    } else {
      delobj(data->obj, data->id);
      menu_delete_undo(data->undo);
    }
  } else {
    set_graph_modified();
  }
  PaintLock = FALSE;

  objects[0] = data->obj->name;
  objects[1] = (data->obj2) ? data->obj2->name : NULL;
  objects[2] = NULL;
  UpdateAll(objects);
  g_free(data);

  if ((d->Mode & POINT_TYPE_DRAW_ALL) && ! KeepMouseMode) {
    set_pointer_mode(PointerModeDefault);
  }
}

static void
add_drawble_response(void *dialog, struct objlist *obj, struct objlist *obj2, int id, int undo)
{
  struct create_drawble_data *data;
  data = create_drawble_data_new(obj, obj2, id, undo);
  response_callback_add(dialog, create_drawble_response, NULL, data);
}

static void
create_legend1(struct Viewer *d)
{
  int num;
  struct objlist *obj = NULL;
  struct Point *po;
  int id, x1, y1, undo;

  d->Capture = FALSE;
  num = arraynum(d->points);

  if (d->Mode == MarkB) {
    obj = chkobject("mark");
  } else {
    obj = chkobject("text");
  }

  if (obj == NULL) {
    arraydel2(d->points);
    return;
  }

  undo = menu_save_undo_single(UNDO_TYPE_CREATE, obj->name);
  id = newobj(obj);
  if (id >= 0) {
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
      add_drawble_response(&DlgLegendMark, obj, NULL, id, undo);
      DialogExecute(TopLevel, &DlgLegendMark);
    } else {
      LegendTextDialog(&DlgLegendText, obj, id);
      add_drawble_response(&DlgLegendText, obj, NULL, id, undo);
      DialogExecute(TopLevel, &DlgLegendText);
    }
  }
  arraydel2(d->points);
}
#else
static void
create_legend1(struct Viewer *d)
{
  int num;
  struct objlist *obj = NULL;
  struct Point *po;
  char *objects[2];
  int id, x1, y1, undo;

  d->Capture = FALSE;
  num = arraynum(d->points);

  if (d->Mode == MarkB) {
    obj = chkobject("mark");
  } else {
    obj = chkobject("text");
  }

  if (obj == NULL) {
    return;
  }

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
      set_graph_modified();
    }
    PaintLock = FALSE;
  }

  arraydel2(d->points);
  objects[0] = obj->name;
  objects[1] = NULL;
  UpdateAll(objects);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
create_path(struct Viewer *d)
{
  struct objlist *obj = NULL;
  struct narray *parray;
  struct Point *po;
  N_VALUE *inst;
  int i, num, id, undo;

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
  add_drawble_response(&DlgLegendArrow, obj, NULL, id, undo);
  DialogExecute(TopLevel, &DlgLegendArrow);

 ExitCreatePath:
  arraydel2(d->points);
  return;
}
#else
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
    set_graph_modified();
  }

  PaintLock = FALSE;

 ExitCreatePath:
  arraydel2(d->points);

  objects[0] = obj->name;
  objects[1] = NULL;
  UpdateAll(objects);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
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
	  add_drawble_response(&DlgLegendRect, obj, NULL, id, undo);
	  DialogExecute(TopLevel, &DlgLegendRect);
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
	  add_drawble_response(&DlgLegendArc, obj, NULL, id, undo);
	  DialogExecute(TopLevel, &DlgLegendArc);
	}
	PaintLock = FALSE;
      }
    }
  }
  arraydel2(d->points);
}
#else
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
	  set_graph_modified();
	}
	PaintLock = FALSE;
      }
    }
  }

  arraydel2(d->points);
  if (obj) {
    char *objects[2];
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
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
        presetting_set_obj_field(obj, id);
	type = PATH_TYPE_CURVE;
	putobj(obj, "type", id, &type);
	fill = FALSE;
	putobj(obj, "fill", id, &fill);
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
	  add_drawble_response(&DlgLegendGauss, obj, NULL, id, undo);
	  DialogExecute(TopLevel, &DlgLegendGauss);
	}
	PaintLock = FALSE;
      }
    }
  }
  arraydel2(d->points);
}
#else
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
        presetting_set_obj_field(obj, id);
	type = PATH_TYPE_CURVE;
	putobj(obj, "type", id, &type);
	fill = FALSE;
	putobj(obj, "fill", id, &fill);
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
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
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
        int x2, y2;
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
	add_drawble_response(&DlgAxis, obj, NULL, id, undo);
	DialogExecute(TopLevel, &DlgAxis);
      }
    }
  }
  arraydel2(d->points);
}
#else
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
	  set_graph_modified();
	}
      }
    }
  }
  arraydel2(d->points);
  if (obj) {
    char *objects[2];
    objects[0] = obj->name;
    objects[1] = NULL;
    UpdateAll(objects);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
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
      int undo, x2, y2;
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
		      idx, idy, idu, idr, obj2, idg, FALSE);

	add_drawble_response(&DlgSection, obj, obj2, -1, undo);
	DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == SectionB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	presetting_set_obj_field(obj, idu);
	presetting_set_obj_field(obj, idr);
	presetting_set_obj_field(obj2, idg);
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, idg, TRUE);

	add_drawble_response(&DlgSection, obj, obj2, -1, undo);
	DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == CrossB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	add_drawble_response(&DlgCross, obj, obj2, -1, undo);
	DialogExecute(TopLevel, &DlgCross);
      }
    }
  }
  arraydel2(d->points);
}
#else
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
		      idx, idy, idu, idr, obj2, idg, FALSE);

	ret = DialogExecute(TopLevel, &DlgSection);
      } else if (d->Mode == SectionB) {
	presetting_set_obj_field(obj, idx);
	presetting_set_obj_field(obj, idy);
	presetting_set_obj_field(obj, idu);
	presetting_set_obj_field(obj, idr);
	presetting_set_obj_field(obj2, idg);
	SectionDialog(&DlgSection, x1, y1, lenx, leny, obj,
		      idx, idy, idu, idr, obj2, idg, TRUE);

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
	set_graph_modified();
      }
      argv[0] = obj->name;
      argv[1] = obj2->name;
      argv[2] = NULL;
      UpdateAll(argv);
    }
  }
  arraydel2(d->points);
}
#endif

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

#if ! GTK_CHECK_VERSION(4, 0, 0)
  if ((d->Mode & POINT_TYPE_DRAW_ALL) && ! KeepMouseMode) {
    set_pointer_mode(PointerModeDefault);
  }
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvRButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
#else
ViewerEvRButtonDown(unsigned int state, TPoint *point, struct Viewer *d, GdkEventButton *e)
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
    do_popup(point->x, point->y, d);
#else
    do_popup(e, d);
#endif
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
    range_increment_deceleration(point->x - d->cx, point->y - d->cy, d);
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
mouse_move_scroll(TPoint *point, struct Viewer *d)
{
  int h, w;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif
  double dx, dy;

#if ! GTK_CHECK_VERSION(4, 0, 0)
  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  w = gtk_widget_get_size(d->Win, GTK_ORIENTATION_HORIZONTAL);
  h = gtk_widget_get_size(d->Win, GTK_ORIENTATION_VERTICAL);
#else
  w = gdk_window_get_width(win);
  h = gdk_window_get_height(win);
#endif
  dx = dy = 0;
  if (point->y > h) {
    dy = SCROLL_INC;
  } else if (point->y < 0) {
    dy = -SCROLL_INC;
  }

  if (point->x > w) {
    dx = SCROLL_INC;
  } else if (point->x < 0) {
    dx = -SCROLL_INC;
  }
#if SCROLL_ANIMATION
  range_increment_deceleration(dx, dy, d);
#else
  range_increment(d->HScroll, dx);
  range_increment(d->VScroll, dy);
#endif
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

static void
set_cross_gauge_position(int dx, int dy, double zoom, struct Viewer *d)
{
  d->CrossX = coord_conv_x(dx, zoom, d);
  d->CrossY = coord_conv_y(dy, zoom, d);
}

static gboolean
ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d)
{
  int dx, dy;
  double zoom;

  if (Menulock || Globallock) {
    return FALSE;
  }

  if (d->drag_prm.active) {
    return FALSE;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  if (gtk_widget_get_window(d->Win) == NULL) {
    return FALSE;
  }
#endif

  d->KeyMask = state;
  zoom = Menulocal.PaperZoom / 10000.0;

  dx = calc_mouse_x(point->x, zoom, d);
  dy = calc_mouse_y(point->y, zoom, d);

  if (d->MouseMode == MOUSESCROLLE) {
    set_cross_gauge_position(dx, dy, zoom, d);
#if SCROLL_ANIMATION
    range_increment_deceleration(mxd2p(d->MouseX1 - dx), mxd2p(d->MouseY1 - dy), d);
#else
    range_increment(d->HScroll, mxd2p(d->MouseX1 - dx));
    range_increment(d->VScroll, mxd2p(d->MouseY1 - dy));
#endif
    return FALSE;
  }

  if ((d->Mode != DataB) &&
      (d->Mode != EvalB) &&
      (d->Mode != TrimB) &&
      (d->Mode != ZoomB) &&
      (d->MouseMode != MOUSEPOINT) &&
      (((d->Mode != PointB) && (d->Mode != LegendB) && (d->Mode != AxisB)) ||
       (d->MouseMode != MOUSENONE))) {
    CheckGrid(TRUE, state, &dx, &dy, NULL, NULL);
  }

  set_cross_gauge_position(dx, dy, zoom, d);
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

  if ((Menulocal.show_cross && gtk_widget_is_drawable(d->Win)) || (d->Mode & POINT_TYPE_DRAW_ALL) ||
      d->MouseMode != MOUSENONE) {
    gtk_widget_queue_draw(d->Win);
  }

  return FALSE;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
ViewerEvMouseMotion(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer client_data)
{
  struct Viewer *d;
  TPoint point;
  GdkModifierType state;

  d = (struct Viewer *) client_data;
  point.x = x;
  point.y = y;
  state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(controller));
  ViewerEvMouseMove(state, &point, d);
  //  gdk_event_request_motions(e); /* handles is_hint events */
}
#else
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
#if GTK_CHECK_VERSION(4, 0, 0)
do_popup(gdouble x, gdouble y, struct Viewer *d)
#else
do_popup(GdkEventButton *event, struct Viewer *d)
#endif
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GdkRectangle rect;
  rect.x = x;
  rect.y = y;
  rect.width = 1;
  rect.height = 1;
  gtk_popover_set_pointing_to(GTK_POPOVER(d->popup), &rect);
  gtk_popover_popup(GTK_POPOVER(d->popup));
#else
  if (! gtk_widget_get_realized(d->popup)) {
    gtk_widget_realize(d->popup);
  }
  gtk_menu_popup_at_pointer(GTK_MENU(d->popup), ((GdkEvent *)event));
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
static gboolean
ViewerEvScroll(GtkEventControllerScroll *self, double x, double y, gpointer client_data)
{
  struct Viewer *d;
  GdkModifierType state;

  d = (struct Viewer *) client_data;
  state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(self));

  if (state & GDK_CONTROL_MASK) {
    GdkDevice *device;
    double wx, wy;
    TPoint point;
    device = gtk_event_controller_get_current_event_device(GTK_EVENT_CONTROLLER(self));
    if (device == NULL) {
      return FALSE;
    }
    gdk_device_get_surface_at_position(device, &wx, &wy);
    point.x = wx;
    point.y = wy;
    if (y > 0) {
      mouse_down_zoom_little(0, &point, d, FALSE);
    } else {
      mouse_down_zoom_little(0, &point, d, TRUE);
    }
  } else {
    GdkScrollUnit unit;
    double factor;
    unit = gtk_event_controller_scroll_get_unit(self);
    factor = (unit == GDK_SCROLL_UNIT_WHEEL) ? SCROLL_INC : 1.0;
    range_increment(d->HScroll, x * factor);
    range_increment(d->VScroll, y * factor);
  }
  return TRUE;
}
#else
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
#if OSX
	range_increment(d->HScroll, x);
	range_increment(d->VScroll, y);
#else
	range_increment(d->HScroll, x * SCROLL_INC);
	range_increment(d->VScroll, y * SCROLL_INC);
#endif
      }
    }
    return TRUE;
  }
  return FALSE;
}
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
static GdkModifierType
get_key_modifier(GtkGestureSingle *gesture)
{
  GdkModifierType state;
  const GdkEvent *event;
  GdkEventSequence *sequence;

  sequence = gtk_gesture_single_get_current_sequence(gesture);
  event = gtk_gesture_get_last_event(GTK_GESTURE(gesture), sequence);
  if (gdk_event_get_state(event, &state)) {
    return state;
  }
  return 0;
}
#endif

static void
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvButtonDown(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data)
#else
ViewerEvButtonDown(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data)
#endif
{
  struct Viewer *d;
  GtkWidget *w;
  TPoint point;
  guint button;
  GdkModifierType state;

  d = (struct Viewer *) client_data;
  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
#if GTK_CHECK_VERSION(4, 0, 0)
  state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
#else
  state = get_key_modifier(GTK_GESTURE_SINGLE(gesture));
#endif

  d->KeyMask = state;

  point.x = x;
  point.y = y;

  w = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
  gtk_widget_grab_focus(w);

  switch (button) {
  case Button1:
    if (n_press == 1) {
      ViewerEvLButtonDown(state, &point, d);
    } else {
      ViewerEvLButtonDblClk(state, &point, d);
    }
    break;
  case Button2:
    ViewerEvMButtonDown(state, &point, d);
    break;
  case Button3:
#if GTK_CHECK_VERSION(4, 0, 0)
    ViewerEvRButtonDown(state, &point, d);
#else
    ViewerEvRButtonDown(state, &point, d, NULL);
#endif
    break;
  }
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvButtonUp(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data)
#else
ViewerEvButtonUp(GtkGestureMultiPress *gesture, gint n_press, gdouble x, gdouble y, gpointer client_data)
#endif
{
  struct Viewer *d;
  TPoint point;
  guint button;
  GdkModifierType state;

  d = (struct Viewer *) client_data;
  button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
#if GTK_CHECK_VERSION(4, 0, 0)
  state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
#else
  state = get_key_modifier(GTK_GESTURE_SINGLE(gesture));
#endif

  d->KeyMask = state;

  point.x = x;
  point.y = y;

  switch (button) {
  case Button1:
    ViewerEvLButtonUp(state, &point, d);
  }
}

static void
move_focus_frame(guint keyval, GdkModifierType state, struct Viewer *d)
{
  int dx = 0, dy = 0, mv;
  double zoom;

  zoom = Menulocal.PaperZoom / 10000.0;
  mv = (state & GDK_SHIFT_MASK) ? Menulocal.grid / 10 : Menulocal.grid;

  switch (keyval) {
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
viewer_key_scroll(guint keyval, struct Viewer *d)
{
  switch (keyval) {
  case GDK_KEY_Up:
    range_increment_deceleration(0, -SCROLL_INC, d);
    return TRUE;
  case GDK_KEY_Down:
    range_increment_deceleration(0, SCROLL_INC, d);
    return TRUE;
  case GDK_KEY_Left:
    range_increment_deceleration(-SCROLL_INC, 0, d);
    return TRUE;
  case GDK_KEY_Right:
    range_increment_deceleration(SCROLL_INC, 0, d);
    return TRUE;
  }
  return FALSE;
}

static gboolean
ViewerEvKeyDown(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  if (Menulock || Globallock)
    goto EXIT_PROPAGATE;

  switch (keyval) {
  case GDK_KEY_Escape:
    if (d->MoveData) {
      move_data_cancel(d, TRUE);
    } else {
      UnFocus();
    }
    set_pointer_mode(PointerModeDefault);
    goto EXIT_PROPAGATE;
  case GDK_KEY_space:
    CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
    return TRUE;
  case GDK_KEY_Page_Up:
    range_increment_deceleration(0, -SCROLL_INC * 4, d);
    return TRUE;
  case GDK_KEY_Page_Down:
    range_increment_deceleration(0, SCROLL_INC * 4, d);
    return TRUE;
  case GDK_KEY_Down:
  case GDK_KEY_Up:
  case GDK_KEY_Left:
  case GDK_KEY_Right:
    if (arraynum(d->focusobj) == 0) {
      return viewer_key_scroll(keyval, d);
    }

    if (((d->MouseMode == MOUSENONE) || (d->MouseMode == MOUSEDRAG)) &&
	(d->Mode & POINT_TYPE_POINT)) {
      move_focus_frame(keyval, state, d);
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

 EXIT_PROPAGATE:
  set_focus_sensitivity(d);
  return FALSE;
}

static void
ViewerEvKeyUp(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct Viewer *d;
  char *objs[OBJ_MAX];
  int dx, dy;
  int axis;

  if (Menulock || Globallock)
    return;

  d = (struct Viewer *) user_data;

  switch (keyval) {
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
    if (d->Mode == ZoomB) {
      NSetCursor(GDK_TARGET);
      return;
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
    if (axis) {
      add_data_grid_to_objs(objs);
    }
    clear_focus_obj_pix(&NgraphApp.Viewer);
    UpdateAll(objs);
    d->MouseMode = MOUSENONE;
#if CLEAR_DRAG_INFO
    reset_drag_info(d);
#endif
    return;
#if GTK_CHECK_VERSION(4, 0, 0)
  case GDK_KEY_Menu:
    do_popup(0, 0, d);
    break;
#endif
  default:
    break;
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
ViewerEvRealize(GtkWidget *w, gpointer client_data)
{
  int width, height;
  width = gtk_drawing_area_get_content_width(GTK_DRAWING_AREA(w));
  height = gtk_drawing_area_get_content_height(GTK_DRAWING_AREA(w));
  ViewerEvSize(w, width, height, client_data);
}
#endif

static void
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvSize(GtkWidget *w, int width, int height, gpointer client_data)
#else
ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data)
#endif
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  d->cx = width / 2;
  d->cy = height / 2;
#else
  d->cx = allocation->width / 2;
  d->cy = allocation->height / 2;
#endif

  ChangeDPI();
}

static gboolean
ViewerEvPaint(GtkWidget *w, cairo_t *cr, gpointer client_data)
{
  struct Viewer *d;

  d = (struct Viewer *) client_data;

  if (ViewerZooming) {
    draw_zoom(cr, d);
    return TRUE;
  }

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

  return FALSE;
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvVScroll(GtkAdjustment *adj, gpointer user_data)
#else
ViewerEvVScroll(GtkRange *range, gpointer user_data)
#endif
{
  struct Viewer *d;
  double y;

  d = (struct Viewer *) user_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  y = gtk_adjustment_get_value(adj);
#else
  y = gtk_range_get_value(range);
#endif
  if (d->vscroll == y) {
    return;
  }
  d->vscroll = y;
  SetVRuler(d);
  gtk_widget_queue_draw(d->Win);
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
ViewerEvHScroll(GtkAdjustment *adj, gpointer user_data)
#else
ViewerEvHScroll(GtkRange *range, gpointer user_data)
#endif
{
  struct Viewer *d;
  double x;

  d = (struct Viewer *) user_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  x = gtk_adjustment_get_value(adj);
#else
  x = gtk_range_get_value(range);
#endif
  if (d->hscroll == x) {
    return;
  }
  d->hscroll = x;
  SetHRuler(d);
  gtk_widget_queue_draw(d->Win);
}

void
ViewerWinUpdate(char const **objects)
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

  mx_redraw(Menulocal.obj, Menulocal.inst, objects);

  PaintLock = lock_state;
  gtk_widget_queue_draw(d->Win);
}

static void
SetHRuler(const struct Viewer *d)
{
  gdouble x1, x2, zoom;
  int width;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  width = gtk_widget_get_size(d->Win, GTK_ORIENTATION_HORIZONTAL);
#else
  width = gdk_window_get_width(win);
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
  win = gtk_widget_get_window(d->Win);
  if (win == NULL) {
    return;
  }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  height = gtk_widget_get_size(d->Win, GTK_ORIENTATION_VERTICAL);
#else
  height = gdk_window_get_height(win);
#endif
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
  clear_focus_obj_pix(&NgraphApp.Viewer);
  return TRUE;
}

static void
clear_focus_obj(struct Viewer *d)
{
  arraydel2(d->focusobj);
  set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
  clear_focus_obj_pix(d);
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

  set_pointer_mode_by_obj(fobj);
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
#define GRID_COLOR 0.7
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
  cairo_set_source_rgba(cr, GRID_COLOR, GRID_COLOR, GRID_COLOR, 1);
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
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  GdkWindow *window;

  window = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if (window == NULL) {
    return;
  }
#endif

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
}

void
SetScroller(void)
{
  int width, height, x, y;
  struct Viewer *d;

  d = &NgraphApp.Viewer;
  cancel_deceleration(d);

  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);
  x = width / 2;
  y = height / 2;


#if GTK_CHECK_VERSION(4, 0, 0)
  scrollbar_set_range(d->HScroll, 0, width);
  scrollbar_set_value(d->HScroll, x);
  scrollbar_set_increment(d->HScroll, 10, 40);

  scrollbar_set_range(d->VScroll, 0, height);
  scrollbar_set_value(d->VScroll, y);
  scrollbar_set_increment(d->VScroll, 10, 40);
#else
  gtk_range_set_range(GTK_RANGE(d->HScroll), 0, width);
  gtk_range_set_value(GTK_RANGE(d->HScroll), x);
  gtk_range_set_increments(GTK_RANGE(d->HScroll), 10, 40);

  gtk_range_set_range(GTK_RANGE(d->VScroll), 0, height);
  gtk_range_set_value(GTK_RANGE(d->VScroll), y);
  gtk_range_set_increments(GTK_RANGE(d->VScroll), 10, 40);
#endif

  d->hscroll = x;
  d->vscroll = y;
}

static double
get_range_max(GtkWidget *w)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  return scrollbar_get_max(w);
#else
  GtkAdjustment *adj;
  double val;

  adj = gtk_range_get_adjustment(GTK_RANGE(w));
  val = (adj) ? gtk_adjustment_get_upper(adj) : 0;

  return val;
#endif
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

  cancel_deceleration(d);

#if GTK_CHECK_VERSION(4, 0, 0)
  scrollbar_set_range(d->HScroll, 0, width);
  scrollbar_set_value(d->HScroll, XPos);
#else
  gtk_range_set_range(GTK_RANGE(d->HScroll), 0, width);
  gtk_range_set_value(GTK_RANGE(d->HScroll), XPos);
#endif
  d->hscroll = XPos;

#if GTK_CHECK_VERSION(4, 0, 0)
  scrollbar_set_range(d->VScroll, 0, height);
  scrollbar_set_value(d->VScroll, YPos);
#else
  gtk_range_set_range(GTK_RANGE(d->VScroll), 0, height);
  gtk_range_set_value(GTK_RANGE(d->VScroll), YPos);
#endif
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
  UnFocus();
}

void
ReopenGC(void)
{
  Menulocal.local->pixel_dot_x =
  Menulocal.local->pixel_dot_y =
    Menulocal.windpi / 25.4 / 100;
}

#if GTK_CHECK_VERSION(4, 0, 0)
struct draw_data {
  int select_file;
  draw_cb cb;
  gpointer data;
};

static void
draw_finalize(gpointer user_data)
{
  struct Viewer *d;
  struct draw_data *data;

  data = (struct draw_data *) user_data;

  d = &NgraphApp.Viewer;
  ResetStatusBar();
  gtk_widget_queue_draw(d->Win);
  if (data->select_file) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  }
  if (data->cb) {
    data->cb(data->data);
  }
  g_free(data);
}

static void
draw_func(gpointer user_data)
{
  N_VALUE *gra_inst;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  FileAutoScale();
  AdjustAxis();
  FitClear();

  ProgressDialogSetTitle(_("Drawing"));
  ReopenGC();

  gra_inst = chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid);
  if (gra_inst) {
    d->ignoreredraw = TRUE;
    _exeobj(Menulocal.GRAobj, "clear", gra_inst, 0, NULL);
    _exeobj(Menulocal.GRAobj, "draw", gra_inst, 0, NULL);
    _exeobj(Menulocal.GRAobj, "flush", gra_inst, 0, NULL);
    d->ignoreredraw = FALSE;
  }
}

static void
draw(struct draw_data *data)
{
  SetStatusBar(_("Drawing."));
  ProgressDialogCreate(_("Scaling"), draw_func, draw_finalize, data);
}

static void
draw_response(int res, gpointer user_data)
{
  if (res) {
    struct draw_data *data;
    data = (struct draw_data *) user_data;
    draw(data);
  }
}

void
Draw(int SelectFile, draw_cb cb, gpointer user_data)
{
  struct draw_data *data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->cb = cb;
  data->select_file = SelectFile;
  data->data = user_data;
  draw_notify(FALSE);
  if (SelectFile) {
    SetFileHidden(draw_response, data);
    return;
  }
  draw(data);
}
#else
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

#if ! GTK_CHECK_VERSION(4, 0, 0)
  ProgressDialogFinalize();
#endif

  gtk_widget_queue_draw(d->Win);

  if (SelectFile) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
viewer_draw_finalize(gpointer user_data)
{
  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, FALSE);
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, DRAW_NONE);
}

void
CmViewerDraw(void *w, gpointer client_data)
{
  int select_file;

  if (Menulock || Globallock)
    return;

  select_file = GPOINTER_TO_INT(client_data);

  Draw(select_file, viewer_draw_finalize, NULL);
}
#else
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
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void
update_focused_obj_finalize(struct Viewer *d)
{
  PaintLock = FALSE;

  if (arraynum(d->focusobj) == 0) {
    clear_focus_obj(d);
  }

  if (d->modified) {
    UpdateAll(d->objs);
  } else {
    menu_undo_internal(d->undo);
  }
  d->ShowFrame = TRUE;

  /* Called in the function ViewerEvLButtonUp */
  clear_focus_obj_pix(d);
  set_focus_sensitivity(d);
}

static void view_update_response(struct response_callback *cb);

static void
update_focused_obj(struct Viewer *d, int i)
{
  int id;
  struct FocusObj *focus;
  struct objlist *obj, *dobj = NULL;
  N_VALUE *inst;
  int x1, y1;
  int idx = 0, idy = 0, idu = 0, idr = 0, idg, lenx, leny;
  int findX, findY, findU, findR, findG;
  char type;
  char *group;

  if (i < 0) {
    update_focused_obj_finalize(d);
    return;
  }

  focus = *(struct FocusObj **) arraynget(d->focusobj, i);
  if (focus == NULL) {
    update_focused_obj(d, i - 1);
    return;
  }

  inst = chkobjinstoid(focus->obj, focus->oid);
  if (inst == NULL) {
    update_focused_obj(d, i - 1);
    return;
  }

  obj = focus->obj;
  _getobj(obj, "id", inst, &id);

  if (obj == chkobject("axis")) {
    d->obj = NULL;
    d->id = -1;

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
		      idx, idy, idu, idr, dobj, idg, type == 's');

	response_callback_add(&DlgSection, view_update_response, NULL, GINT_TO_POINTER(i));
	DialogExecute(TopLevel, &DlgSection);
      } else if ((type == 'c') && findX && findY) {
	getobj(obj, "x", idx, 0, NULL, &x1);
	getobj(obj, "y", idy, 0, NULL, &y1);
	getobj(obj, "length", idx, 0, NULL, &lenx);
	getobj(obj, "length", idy, 0, NULL, &leny);

	CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	response_callback_add(&DlgCross, view_update_response, NULL, GINT_TO_POINTER(i));
	DialogExecute(TopLevel, &DlgCross);
      }
    } else {
      AxisDialog(NgraphApp.AxisWin.data.data, id, TRUE);
      response_callback_add(&DlgAxis, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgAxis);
    }
  } else {
    d->obj = obj;
    d->id = id;
    if (obj == chkobject("path")) {
      LegendArrowDialog(&DlgLegendArrow, obj, id);
      response_callback_add(&DlgLegendArrow, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgLegendArrow);
    } else if (obj == chkobject("rectangle")) {
      LegendRectDialog(&DlgLegendRect, obj, id);
      response_callback_add(&DlgLegendRect, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgLegendRect);
    } else if (obj == chkobject("arc")) {
      LegendArcDialog(&DlgLegendArc, obj, id);
      response_callback_add(&DlgLegendArc, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgLegendArc);
    } else if (obj == chkobject("mark")) {
      LegendMarkDialog(&DlgLegendMark, obj, id);
      response_callback_add(&DlgLegendMark, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgLegendMark);
    } else if (obj == chkobject("text")) {
      LegendTextDialog(&DlgLegendText, obj, id);
      response_callback_add(&DlgLegendText, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgLegendText);
    } else if (obj == chkobject("merge")) {
      MergeDialog(NgraphApp.MergeWin.data.data, id, 0);
      response_callback_add(&DlgMerge, view_update_response, NULL, GINT_TO_POINTER(i));
      DialogExecute(TopLevel, &DlgMerge);
    }
  }
}

static void
view_update_response(struct response_callback *cb)
{
  struct Viewer *d;
  struct objlist *obj;
  int id, i;

  i = GPOINTER_TO_INT(cb->data);
  d = &NgraphApp.Viewer;
  obj = d->obj;
  id = d->id;
  if (cb->return_value == IDDELETE) {
    set_graph_modified();
    delobj(obj, id);
  }
  if (cb->return_value != IDCANCEL) {
    d->modified = TRUE;
  }
  update_focused_obj(d, i - 1);
}

static void
ViewUpdate(void)
{
  int num;
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

  get_focused_obj_array(d->focusobj, d->objs);
  d->undo = menu_save_undo(UNDO_TYPE_EDIT, d->objs);
  PaintLock = TRUE;

  d->modified = FALSE;
  update_focused_obj(d, num - 1);
}
#else
static void
ViewUpdate(void)
{
  int i, id, num, undo;
  struct FocusObj *focus;
  struct objlist *obj, *dobj = NULL;
  N_VALUE *inst;
  int ret, modified;
  int x1, y1;
  int idx = 0, idy = 0, idu = 0, idr = 0, idg, lenx, leny;
  int findX, findY, findU, findR, findG;
  char type;
  char *group, *objs[OBJ_MAX];
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
			idx, idy, idu, idr, dobj, idg, type == 's');

	  ret = DialogExecute(TopLevel, &DlgSection);
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
    }
    if (ret != IDCANCEL) {
      modified = TRUE;
    }
  }
  PaintLock = FALSE;

  if (arraynum(d->focusobj) == 0)
    clear_focus_obj(d);

  if (modified) {
    UpdateAll(objs);
  } else {
    menu_undo_internal(undo);
  }
  d->ShowFrame = TRUE;
}
#endif

static void
ViewDelete(void)
{
  int i, id, num;
  struct FocusObj *focus;
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

    _getobj(obj, "id", inst, &id);

    if (obj == chkobject("axis")) {
      AxisDel(id);
    } else {
      delobj(obj, id);
    }
    set_graph_modified();
  }
  PaintLock = FALSE;

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
  set_graph_modified();
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
	set_graph_modified();
      }

      if ((idy2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idy2, idy);
	inst2 = chkobjinst(obj, idy2);
	_getobj(obj, "oid", inst2, &oidy);
	set_graph_modified();
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
	set_graph_modified();
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

	set_graph_modified();
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
	  set_graph_modified();
	}
      }
    } else if ((type == 'c') && findX && findY) {
      if ((idx2 = newobj(obj)) >= 0) {
	ncopyobj(obj, idx2, idx);
	inst2 = chkobjinst(obj, idx2);
	_getobj(obj, "oid", inst2, &oidx);
	set_graph_modified();
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
	set_graph_modified();
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
      set_graph_modified();
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
      ViewCopyAxis(obj, id, focus, inst);
    } else {
      if ((id2 = newobj(obj)) >= 0) {
	ncopyobj(obj, id2, id);
	inst2 = chkobjinst(obj, id2);
	_getobj(obj, "oid", inst2, &(focus->oid));
	set_graph_modified();
      }
    }
  }
  PaintLock = FALSE;
  g_free(focused_inst);

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

#if GTK_CHECK_VERSION(4, 0, 0)
void
CmViewerButtonPressed(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  GdkModifierType state;
  state = gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
  KeepMouseMode = (state & GDK_SHIFT_MASK);
  gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}
#else
gboolean
CmViewerButtonPressed(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  KeepMouseMode = (event->state & GDK_SHIFT_MASK);
  return FALSE;
}
#endif

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
  clear_focus_obj_pix(&(NgraphApp.Viewer));
  set_focus_sensitivity(&(NgraphApp.Viewer));
}
