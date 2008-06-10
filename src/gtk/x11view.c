
/* 
 * $Id: x11view.c,v 1.20 2008/06/10 11:31:11 hito Exp $
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
#include <math.h>

#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "mathfn.h"
#include "ioutil.h"
#include "nstring.h"

#include "gtk_liststore.h"
#include "strconv.h"

#include "x11gui.h"
#include "x11dialg.h"
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

#define ID_BUF_SIZE 16
#define SCROLL_INC 20

struct pointslist
{
  int x, y;
};

enum ViewerPopupIdn {
  VIEW_UPDATE,
  VIEW_DELETE,
  VIEW_COPY,
  VIEW_TOP,
  VIEW_LAST,
  VIEW_CROSS,
};

struct viewer_popup
{
  char *title;
  gboolean use_stock;
  enum ViewerPopupIdn idn;
};

#define MOUSENONE   0
#define MOUSEPOINT  1
#define MOUSEDRAG   2
#define MOUSEZOOM1  3
#define MOUSEZOOM2  4
#define MOUSEZOOM3  5
#define MOUSEZOOM4  6
#define MOUSECHANGE 7

#define Button1 1
#define Button2 2
#define Button3 3
#define Button4 4
#define Button5 5

static GdkRegion *region = NULL;
static int PaintLock = FALSE, ZoomLock = FALSE;

#define EVAL_NUM_MAX 5000
static struct evaltype EvalList[EVAL_NUM_MAX];
static struct narray SelList;
// static char evbuf[256];

#define IDEVMASK        101
#define IDEVMOVE        102

static void ViewerEvSize(GtkWidget *w, GtkAllocation *allocation, gpointer client_data);
static gboolean ViewerEvHScroll(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
static gboolean ViewerEvVScroll(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data);
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
static void AddInvalidateRect(struct objlist *obj, char *inst);
static void AddList(struct objlist *obj, char *inst);
static void DelList(struct objlist *obj, char *inst);
static void MakeRuler(GdkGC *gc);
static void ViewUpdate(void);
static void ViewDelete(void);
static void ViewTop(void);
static void ViewLast(void);
static void ViewCopy(void);
static void ViewCross(void);
static void do_popup(GdkEventButton *event, struct Viewer *d);

static int
graph_dropped(char *fname)
{
  int load;
  char *ext;

  if (fname) {
    load = FALSE;
    if ((ext = getextention(fname)) != NULL) {
      if (strcmp0(ext, "prm") == 0) {

	if (!CheckSave())
	  return 1;

	LoadPrmFile(fname);
	load = TRUE;
      } else if (strcmp0(ext, "ngp") == 0) {
	if (!CheckSave())
	  return 1;;

	LoadNgpFile(fname, Menulocal.ignorepath, Menulocal.expand,
		    Menulocal.expanddir, FALSE, NULL);
	load = TRUE;
      }
    }
    g_free(fname);
    if (load) {
      NgraphApp.Changed = FALSE;
      CmViewerDrawB(NULL, NULL);
      return 1;
    }
  }
  return 0;
}

static int
data_dropped(char **filenames, int num)
{
  char *fname, *name, *field;
  int i, j, id, id0, perm, type, ret;
  struct objlist *obj;

  if ((obj = chkobject("file")) == NULL) {
    return 1;
  }

  id0 = -1;
  for (i = 0; i < num; i++) {
    id = newobj(obj);
    if (id < 0)
      continue;

    fname = g_filename_from_uri(filenames[i], NULL, NULL);
    if (fname == NULL)
      continue;

    name = nstrdup(fname);
    g_free(fname);

    if (name == NULL) {
      return 1;
    }

    AddDataFileList(name);
    putobj(obj, "file", id, name);
    if (id0 != -1) {
      for (j = 0; j < chkobjfieldnum(obj); j++) {
	field = chkobjfieldname(obj, j);
	perm = chkobjperm(obj, field);
	type = chkobjfieldtype(obj, field);
	if ((strcmp2(field, "name") != 0)
	    && (strcmp2(field, "file") != 0)
	    && (strcmp2(field, "fit") != 0)
	    && ((perm & NREAD) != 0) && ((perm & NWRITE) != 0)
	      && (type < NVFUNC))
	  copyobj(obj, field, id, id0);
      }
      FitCopy(obj, id, id0);
    } else {
      FileDialog(&DlgFile, obj, id, 0);
      ret = DialogExecute(TopLevel, &DlgFile);
      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	FitDel(obj, id);
	delobj(obj, id);
      } else {
	if (ret == IDFAPPLY)
	  id0 = id;
	NgraphApp.Changed = TRUE;
      }
    }
  }

  FileWinUpdate(TRUE);
  return 0;
}

static int
text_dropped(char *str, gint x, gint y, struct Viewer *d)
{
  char *inst, *tmp, *ptr;
  double zoom = Menulocal.PaperZoom / 10000.0;
  struct objlist *obj;
  int id, x1, y1, r, i, j, l;

  obj = chkobject("text");

  if (obj == NULL)
    return 1;

  l = strlen(str);
  ptr = malloc(l * 2);

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

#ifdef JAPANESE
  {
    char *tmp2;

    tmp2 = utf8_to_sjis(ptr);
    if (tmp2 == NULL) {
      free(ptr);
      return 1;
    }

    tmp = nstrdup(tmp2);
    free(tmp2);
  }
#else
  tmp = nstrdup(ptr);
#endif

  free(ptr);
  if (tmp == NULL) {
    return 1;
  }
    
  id = newobj(obj);
  if (id < 0) {
    memfree(tmp);
    return 1;
  }

  inst = chkobjinst(obj, id);
  x1 = (mxp2d(x + d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom;
  y1 = (mxp2d(y + d->vscroll - d->cy) - Menulocal.TopMargin) / zoom;

  _putobj(obj, "x", inst, &x1);
  _putobj(obj, "y", inst, &y1);
  _putobj(obj, "text", inst, tmp);
  
  PaintLock= TRUE;

  LegendTextDialog(&DlgLegendText, obj, id);
  r = DialogExecute(TopLevel, &DlgLegendText);
      
  if ((r == IDDELETE) || (r == IDCANCEL)) {
    delobj(obj, id);
  } else {
    d->allclear = FALSE;
    AddList(obj, inst);
    AddInvalidateRect(obj, inst);
    NgraphApp.Changed = TRUE;
    UpdateAll();
  }
  PaintLock = FALSE;

  return 0;
}

#define DROP_TYPE_FILE 0
#define DROP_TYPE_TEXT 1

static void 
drag_drop_cb(GtkWidget *w, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  gchar **filenames, *fname, *str;
  int num, r;
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  switch (info) {
  case DROP_TYPE_TEXT:
    str = (gchar *) gtk_selection_data_get_text(data);
    if (str) {
      text_dropped(str, x, y, d);
      g_free(str);
    }
    break;
  case DROP_TYPE_FILE:
    filenames = gtk_selection_data_get_uris(data);

    num = g_strv_length(filenames);

    r = 0;
    if (num == 1) {
      fname = g_filename_from_uri(filenames[0], NULL, NULL);
      r = graph_dropped(fname);
    }

    if (! r) {
      data_dropped(filenames,  num);
    }

    g_strfreev(filenames);
    gtk_drag_finish(context, TRUE, FALSE, time);
    break;
  }
}


static void
init_dnd(struct Viewer *d)
{
  GtkWidget *widget;

  widget = d->Win;

  GtkTargetEntry target[] = {
    {"text/uri-list", 0, DROP_TYPE_FILE},
    {"text/plain", 0, DROP_TYPE_TEXT},
  };

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, target, sizeof(target) / sizeof(*target), GDK_ACTION_COPY);
  g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_drop_cb), d);
}

static void
EvalDialogSetupItem(GtkWidget *w, struct EvalDialog *d)
{
  int i;
  GtkTreeIter iter;

  list_store_clear(d->list);
  for (i = 0; i < d->Num; i++) {
    list_store_append(d->list, &iter);
    list_store_set_int(d->list, &iter, 0, EvalList[i].id);
    list_store_set_int(d->list, &iter, 1, EvalList[i].line);
    list_store_set_double(d->list, &iter, 2, EvalList[i].x);
    list_store_set_double(d->list, &iter, 3, EvalList[i].y);
  }
}

static void
EvalDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *hbox;
  struct EvalDialog *d;
  n_list_store list[] = {
    {"#",       G_TYPE_INT,    TRUE, FALSE, NULL},
    {_("Line No."), G_TYPE_INT,    TRUE, FALSE, NULL},
    {"X",       G_TYPE_DOUBLE, TRUE, FALSE, NULL},
    {"Y",       G_TYPE_DOUBLE, TRUE, FALSE, NULL},
  };


  d = (struct EvalDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   _("_Mask"), IDEVMASK,
			   _("_Move"), IDEVMOVE,
			   NULL);

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    d->show_cancel = FALSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), 300, 400);
  }
  EvalDialogSetupItem(wi, d);
}

static void 
select_data_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  struct EvalDialog *d;
  int *ptr, a;

  d = (struct EvalDialog *) data;

  ptr = gtk_tree_path_get_indices(path);
  a = ptr[0];
  arrayadd(d->sel, &a);
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
create_popup_menu(struct Viewer *d)
{
  struct viewer_popup popup[] = {
    {GTK_STOCK_PROPERTIES,  TRUE,  VIEW_UPDATE},
    {GTK_STOCK_DELETE,      TRUE,  VIEW_DELETE},
    {"_Duplicate",          FALSE, VIEW_COPY},
    {GTK_STOCK_GOTO_TOP,    TRUE,  VIEW_TOP},
    {GTK_STOCK_GOTO_BOTTOM, TRUE,  VIEW_LAST},
    {"_Show cross",         FALSE, VIEW_CROSS},
    {NULL},
  };
  GtkWidget *menu, *item;
  int i = 0;

  menu = gtk_menu_new();

  for (i = 0; popup[i].title; i++) {
    if (popup[i].use_stock) {
      item = gtk_image_menu_item_new_from_stock(popup[i].title, NULL);
    } else {
      item = gtk_menu_item_new_with_mnemonic(popup[i].title);
    }
    d->popup_item[i] = item;
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(ViewerPopupMenu), GINT_TO_POINTER(popup[i].idn));
  }

  return menu;
}

static gboolean
scrollbar_scroll_cb(GtkWidget *w, GdkEventScroll *e, gpointer client_data)
{
  struct Viewer *d;
  double val;

  d = &(NgraphApp.Viewer);

  switch (e->direction) {
  case GDK_SCROLL_UP:
  case GDK_SCROLL_LEFT:
    val = gtk_range_get_value(GTK_RANGE(w));
    val -= SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(w), val);
    break;
  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_RIGHT:
    val = gtk_range_get_value(GTK_RANGE(w));
    val += SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(w), val);
    break;
  default:
    return FALSE;
  }

  if (client_data) {
    ViewerEvHScroll(NULL, GTK_SCROLL_STEP_DOWN, val, d);
  } else {
    ViewerEvVScroll(NULL, GTK_SCROLL_STEP_DOWN, val, d);
  }
  return TRUE;
}

void
ViewerWinSetup(void)
{
  struct Viewer *d;
  int x, y, width, height;

  d = &(NgraphApp.Viewer);
  d->win = NgraphApp.Viewer.Win->window;
  Menulocal.GRAoid = -1;
  Menulocal.GRAinst = NULL;
  d->Mode = PointB;
  d->Capture = FALSE;
  d->MoveData = FALSE;
  d->MouseMode = MOUSENONE;
  d->focusobj = arraynew(sizeof(struct focuslist *));
  d->points = arraynew(sizeof(struct pointslist *));
  d->FrameOfsX = 0;
  d->FrameOfsY = 0;
  d->LineX = 0;
  d->LineY = 0;
  d->CrossX = 0;
  d->CrossY = 0;
  d->ShowFrame = FALSE;
  d->ShowLine = FALSE;
  d->ShowRect = FALSE;
  d->ShowCross = FALSE;
  d->allclear = TRUE;
  d->ignoreredraw = FALSE;
  region = NULL;
  OpenGRA();
  OpenGC();
  SetScroller();
  ChangeDPI(TRUE);
  gdk_window_get_position(d->win, &x, &y);
  gdk_drawable_get_size(d->win, &width, &height);
  d->width = width;
  d->height = height;
  d->cx = width / 2;
  d->cy = height / 2;

  d->hscroll = gtk_range_get_value(GTK_RANGE(d->HScroll));
  d->vscroll = gtk_range_get_value(GTK_RANGE(d->VScroll));

  g_signal_connect(d->Win, "expose-event", G_CALLBACK(ViewerEvPaint), NULL);
  g_signal_connect(d->Win, "size-allocate", G_CALLBACK(ViewerEvSize), NULL);
  init_dnd(d);

  g_signal_connect(d->HScroll, "change-value", G_CALLBACK(ViewerEvHScroll), NULL);
  g_signal_connect(d->VScroll, "change-value", G_CALLBACK(ViewerEvVScroll), NULL);
  g_signal_connect(d->HScroll, "scroll-event", G_CALLBACK(scrollbar_scroll_cb), GINT_TO_POINTER(1));
  g_signal_connect(d->VScroll, "scroll-event", G_CALLBACK(scrollbar_scroll_cb), NULL);

  gtk_widget_add_events(d->Win,
			GDK_POINTER_MOTION_MASK |
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

  d->popup = create_popup_menu(d);
  gtk_widget_show_all(d->popup);
  gtk_menu_attach_to_widget(GTK_MENU(d->popup), NgraphApp.Viewer.Win, NULL);
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
  //  XUndefineCursor(XtDisplay(d->Win), XtWindow(d->Win));
  if (region) {
    gdk_region_destroy(region);
    region = NULL;
  }

}

int
ViewerWinFileUpdate(int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fileobj;
  char *argv[7];
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did, id, limit;
  char *dfield, *dinst;
  int i, j;
  struct narray *eval;
  int evalnum;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  char mes[256];
  struct narray dfile;
  int *ddata, dnum;
  int ret, ret2;

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
  if (fileobj) {
    arrayinit(&dfile, sizeof(int));
    snprintf(mes, sizeof(mes), _("Searching for data."));
    SetStatusBar(mes);

    if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
      return ret;

    if ((snum = arraynum(sarray)) == 0)
      return ret;

    sdata = (char **) arraydata(sarray);

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

    ResetStatusBar();

    dnum = arraynum(&dfile);
    ddata = (int *) arraydata(&dfile);

    for (i = 0; i < dnum; i++) {
      id = ddata[i];
      FileDialog(&DlgFile, fileobj, id, 0);
      ret2 = DialogExecute(TopLevel, &DlgFile);
      if (ret2 == IDDELETE) {
	FitDel(fileobj, id);
	delobj(fileobj, id);
	NgraphApp.Changed = TRUE;
	for (j = i + 1; j < dnum; j++) {
	  if (ddata[j] > id) {
	    ddata[j] = ddata[j] - 1;
	  }
	}
      } else if (ret2 != IDCANCEL) {
	NgraphApp.Changed = TRUE;
      }
      ret = TRUE;
    }
    arraydel(&dfile);
  }
  restorestdio(&save);
  return ret;
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
  char *dfield, *dinst;
  int i, j;
  struct narray *eval;
  int evalnum, tot;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  double line, dx, dy;
  char mes[256];
  int ret;
  int selnum, sel;
  int masknum, iline;
  struct narray *mask;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  ignorestdio(&save);

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

  if ((fileobj = chkobject("file")) != NULL) {
    snprintf(mes, sizeof(mes), _("Evaluating."));
    SetStatusBar(mes);

    if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
      return;

    if ((snum = arraynum(sarray)) == 0)
      return;

    ProgressDialogCreate(_("Evaluating"));

    sdata = (char **) arraydata(sarray);
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
	    line = *(double *) arraynget(eval, j * 3 + 0);
	    dx = *(double *) arraynget(eval, j * 3 + 1);
	    dy = *(double *) arraynget(eval, j * 3 + 2);
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
	for (i = 0; i < selnum; i++) {
	  sel = *(int *) arraynget(&SelList, i);
	  getobj(fileobj, "mask", EvalList[sel].id, 0, NULL, &mask);
	  if (mask == NULL) {
	    mask = arraynew(sizeof(int));
	    putobj(fileobj, "mask", EvalList[sel].id, mask);
	  }
	  masknum = arraynum(mask);
	  for (j = 0; j < masknum; j++) {
	    iline = *(int *) arraynget(mask, j);
	    if (iline == EvalList[sel].line)
	      break;
	  }
	  if (j == masknum) {
	    arrayadd(mask, &(EvalList[sel].line));
	    NgraphApp.Changed = TRUE;
	  }
	}
	arraydel(&SelList);
      } else if ((ret == IDEVMOVE) && (selnum > 0)) {
	SetCursor(GDK_LEFT_PTR);
	d->Capture = TRUE;
	d->MoveData = TRUE;
      }
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
    array = (int *) arraydata(&farray);

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
	  exeobj(obj, "scale_push", id, 0, NULL);
	  room = 1;
	  argv[0] = (char *) &min;
	  argv[1] = (char *) &max;
	  argv[2] = (char *) &room;
	  argv[3] = NULL;
	  exeobj(obj, "scale", id, 3, argv);
	  NgraphApp.Changed = TRUE;
	}
      }
    }
    AdjustAxis();
    d->allclear = TRUE;
    UpdateAll();
  }
  arraydel(&farray);
}


static void
Match(char *objname, int x1, int y1, int x2, int y2, int err)
{
  struct objlist *fobj;
  char *argv[6];
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did;
  char *dfield, *dinst;
  int i, match;
  struct focuslist *focus;
  int minx, miny, maxx, maxy;
  struct savedstdio save;
  struct Viewer *d;

  set_draw_lock(DrawLockExpose);

  d = &(NgraphApp.Viewer);

  ignorestdio(&save);

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
  if (fobj) {
    if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
      return;

    if ((snum = arraynum(sarray)) == 0)
      return;

    sdata = (char **) arraydata(sarray);

    for (i = 1; i < snum; i++) {
      dobj = getobjlist(sdata[i], &did, &dfield, NULL);
      if (dobj && chkobjchild(fobj, dobj)) {
	dinst = chkobjinstoid(dobj, did);
	if (dinst) {
	  _exeobj(dobj, "match", dinst, 5, argv);
	  _getobj(dobj, "match", dinst, &match);
	  if (match) {
	    focus = (struct focuslist *) memalloc(sizeof(struct focuslist));
	    if (focus) {
	      focus->obj = dobj;
	      focus->oid = did;
	      arrayadd(d->focusobj, &focus);
	    }
	  }
	}
      }
    }
  }
  restorestdio(&save);

  set_draw_lock(DrawLockNone);
}

static void
AddList(struct objlist *obj, char *inst)
{
  int addi;
  struct objlist *aobj;
  char *ainst;
  char *afield;
  int i, j, po, num, oid, id, id2;
  struct objlist **objlist;
  struct objlist *obj2;
  char *inst2;
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

  objlist = (struct objlist **) memalloc(sizeof(struct objlist *) * num);
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
      _getobj(obj2, "id", inst2, &id2);
      if (id2 > id) {
	addi = i;
	memfree(objlist);
	mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
	if (draw != &(Menulocal.drawrable)) {
	  arraydel2(draw);
	}
	return;
      }
    } else if (j > po) {
      addi = i;
      memfree(objlist);
      mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
      if (draw != &(Menulocal.drawrable)) {
	arraydel2(draw);
      }
      return;
    }
    i++;
  }
  addi = i;
  memfree(objlist);
  mx_inslist(Menulocal.obj, Menulocal.inst, aobj, ainst, afield, addi);
  if (draw != &(Menulocal.drawrable))
    arraydel2(draw);
}

static void
DelList(struct objlist *obj, char *inst)
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
AddInvalidateRect(struct objlist *obj, char *inst)
{
  struct narray *abbox;
  int bboxnum, *bbox;
  double zoom;
  struct Viewer *d;
  GdkRectangle rect;

  set_draw_lock(DrawLockExpose);

  d = &(NgraphApp.Viewer);
  _exeobj(obj, "bbox", inst, 0, NULL);
  _getobj(obj, "bbox", inst, &abbox);
  bboxnum = arraynum(abbox);
  bbox = (int *) arraydata(abbox);
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

  set_draw_lock(DrawLockNone);
}


static void
GetLargeFrame(int *minx, int *miny, int *maxx, int *maxy)
{
  int i, num;
  struct focuslist **focus;
  struct narray *abbox;
  int bboxnum, *bbox;
  char *inst;
  struct savedstdio save;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  ignorestdio(&save);
  *minx = *miny = *maxx = *maxy = 0;

  num = arraynum(d->focusobj);
  focus = (struct focuslist **) arraydata(d->focusobj);

  inst = chkobjinstoid(focus[0]->obj, focus[0]->oid);
  if (inst) {
    _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
    _getobj(focus[0]->obj, "bbox", inst, &abbox);

    bboxnum = arraynum(abbox);
    bbox = (int *) arraydata(abbox);

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
      bbox = (int *) arraydata(abbox);

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

static void
GetFocusFrame(int *minx, int *miny, int *maxx, int *maxy, int ofsx, int ofsy)
{
  int x1, y1, x2, y2;
  double zoom;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  GetLargeFrame(&x1, &y1, &x2, &y2);

  zoom = Menulocal.PaperZoom / 10000.0;

  *minx =
    mxd2p((x1 + ofsx) * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;

  *miny =
    mxd2p((y1 + ofsy) * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

  *maxx =
    mxd2p((x2 + ofsx) * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;

  *maxy =
    mxd2p((y2 + ofsy) * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;
}

static void
ShowFocusFrame(GdkGC *gc)
{
  int i, j, num;
  struct focuslist **focus;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  int x1, y1, x2, y2;
  char *inst;
  struct savedstdio save;
  double zoom;
  struct Viewer *d;
  int minx, miny, height, width;

  d = &(NgraphApp.Viewer);
  ignorestdio(&save);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_gc_set_dashes(gc, 0, Dashes, 2);
  gdk_gc_set_function(gc, GDK_XOR);

  num = arraynum(d->focusobj);
  focus = (struct focuslist **) arraydata(d->focusobj);

  if (num > 0) {
    GetFocusFrame(&x1, &y1, &x2, &y2, d->FrameOfsX, d->FrameOfsY);

    x1 -= 5;
    y1 -= 5;
    x2 += 4;
    y2 += 4;

    minx = (x1 < x2) ? x1 : x2;
    miny = (y1 < y2) ? y1 : y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);

    gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
    gdk_draw_rectangle(d->win, gc, TRUE, x1 - 6, y1 - 6, 6, 6);
    gdk_draw_rectangle(d->win, gc, TRUE, x1 - 6, y2,     6, 6);
    gdk_draw_rectangle(d->win, gc, TRUE, x2,     y1 - 6, 6, 6);
    gdk_draw_rectangle(d->win, gc, TRUE, x2,     y2,     6, 6);
  }
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num > 1) {
    for (i = 0; i < num; i++) {
      inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
      if (inst) {
	_exeobj(focus[i]->obj, "bbox", inst, 0, NULL);
	_getobj(focus[i]->obj, "bbox", inst, &abbox);

	bboxnum = arraynum(abbox);
	bbox = (int *) arraydata(abbox);

	if (bboxnum >= 4) {
	  x1 =
	    mxd2p((bbox[0] + d->FrameOfsX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;
	  y1 =
	    mxd2p((bbox[1] + d->FrameOfsY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;
	  x2 =
	    mxd2p((bbox[2] + d->FrameOfsX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;
	  y2 =
	    mxd2p((bbox[3] + d->FrameOfsY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;

	  minx = (x1 < x2) ? x1 : x2;
	  miny = (y1 < y2) ? y1 : y2;

	  width = abs(x2 - x1);
	  height = abs(y2 - y1);

	  gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
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
      bbox = (int *) arraydata(abbox);

      for (j = 4; j < bboxnum; j += 2) {
	x1 =
	  mxd2p((bbox[j] + d->FrameOfsX) * zoom +
		Menulocal.LeftMargin) - d->hscroll + d->cx;
	y1 =
	  mxd2p((bbox[j + 1] + d->FrameOfsY) * zoom +
		Menulocal.TopMargin) - d->vscroll + d->cy;

	gdk_draw_rectangle(d->win, gc, TRUE, x1 - 3, y1 - 3, 6, 6);
      }
    }
  }
  gdk_gc_set_function(gc, GDK_COPY);
  restorestdio(&save);
}

static void
ShowFocusLine(GdkGC *gc, int change)
{
  int j, num;
  struct focuslist **focus;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, ofsx, ofsy;
  char *inst;
  struct savedstdio save;
  double zoom;
  int frame;
  char *group;
  int minx, miny, height, width;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  ignorestdio(&save);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
  gdk_gc_set_dashes(gc, 0, Dashes, 2);
  num = arraynum(d->focusobj);

  focus = (struct focuslist **) arraydata(d->focusobj);
  zoom = Menulocal.PaperZoom / 10000.0;

  if (num == 1) {
    if ((inst = chkobjinstoid(focus[0]->obj, focus[0]->oid)) != NULL) {
      _exeobj(focus[0]->obj, "bbox", inst, 0, NULL);
      _getobj(focus[0]->obj, "bbox", inst, &abbox);

      bboxnum = arraynum(abbox);
      bbox = (int *) arraydata(abbox);

      frame = FALSE;

      if (focus[0]->obj == chkobject("rectangle"))
	frame = TRUE;

      if (focus[0]->obj == chkobject("axis")) {
	_getobj(focus[0]->obj, "group", inst, &group);
	if ((group != NULL) && (group[0] != 'a')) {
	  frame = TRUE;
	}
      }

      if (!frame) {
	for (j = 4; j < bboxnum; j += 2) {
	  if (change == (j - 4) / 2) {
	    ofsx = d->LineX;
	    ofsy = d->LineY;
	  } else {
	    ofsx = 0;
	    ofsy = 0;
	  }

	  x1 = mxd2p((bbox[j] + ofsx) * zoom + Menulocal.LeftMargin)
	    - d->hscroll + d->cx;

	  y1 = mxd2p((bbox[j + 1] + ofsy) * zoom + Menulocal.TopMargin)
	    - d->vscroll + d->cy;
	  if (j != 4) {
	    gdk_draw_line(d->win, gc, x0, y0, x1, y1);
	  }
	  x0 = x1;
	  y0 = y1;
	}
      } else {
	if (change == 0) {
	  x1 =
	    mxd2p((bbox[4] + d->LineX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;

	  y1 =
	    mxd2p((bbox[5] + d->LineY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;

	  x2 =
	    mxd2p((bbox[8]) * zoom + Menulocal.LeftMargin) -
	    d->hscroll + d->cx;

	  y2 =
	    mxd2p((bbox[9]) * zoom + Menulocal.TopMargin) -
	    d->vscroll + d->cy;
	} else if (change == 1) {
	  x1 = mxd2p((bbox[4]) * zoom + Menulocal.LeftMargin)
	    - d->hscroll + d->cx;

	  y1 =
	    mxd2p((bbox[5] + d->LineY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;

	  x2 =
	    mxd2p((bbox[8] + d->LineX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;

	  y2 =
	    mxd2p((bbox[9]) * zoom + Menulocal.TopMargin) -
	    d->vscroll + d->cy;
	} else if (change == 2) {
	  x1 = mxd2p((bbox[4]) * zoom + Menulocal.LeftMargin)
	    - d->hscroll + d->cx;

	  y1 = mxd2p((bbox[5]) * zoom + Menulocal.TopMargin)
	    - d->vscroll + d->cy;

	  x2 =
	    mxd2p((bbox[8] + d->LineX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;

	  y2 =
	    mxd2p((bbox[9] + d->LineY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;
	} else if (change == 3) {
	  x1 =
	    mxd2p((bbox[4] + d->LineX) * zoom +
		  Menulocal.LeftMargin) - d->hscroll + d->cx;

	  y1 =
	    mxd2p((bbox[5]) * zoom + Menulocal.TopMargin) -
	    d->vscroll + d->cy;

	  x2 =
	    mxd2p((bbox[8]) * zoom + Menulocal.LeftMargin) -
	    d->hscroll + d->cx;

	  y2 =
	    mxd2p((bbox[9] + d->LineY) * zoom +
		  Menulocal.TopMargin) - d->vscroll + d->cy;
	}
	minx = (x1 < x2) ? x1 : x2;
	miny = (y1 < y2) ? y1 : y2;

	width = abs(x2 - x1);
	height = abs(y2 - y1);

	gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
      }
    }
  }
  gdk_gc_set_function(gc, GDK_COPY);
  restorestdio(&save);
}

static void
ShowPoints(GdkGC *gc)
{
  int i, num, x0 = 0, y0 = 0, x1, y1, x2, y2;
  struct pointslist **po;
  double zoom;
  struct Viewer *d;
  int minx, miny, height, width;

  d = &(NgraphApp.Viewer);

  gdk_gc_set_rgb_fg_color(gc, &gray);
  gdk_gc_set_function(gc, GDK_XOR);

  num = arraynum(d->points);
  po = (struct pointslist **) arraydata(d->points);

  zoom = Menulocal.PaperZoom / 10000.0;

  if ((d->Mode == RectB) || (d->Mode == ArcB) || (d->Mode == GaussB)
      || (d->Mode == FrameB) || (d->Mode == SectionB) || (d->Mode == CrossB)) {

    if (num == 2) {
      gdk_gc_set_line_attributes(gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
      gdk_gc_set_dashes(gc, 0, Dashes, 2);

      x1 = mxd2p(po[0]->x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
      y1 = mxd2p(po[0]->y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;
      x2 = mxd2p(po[1]->x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
      y2 = mxd2p(po[1]->y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

      minx = (x1 < x2) ? x1 : x2;
      miny = (y1 < y2) ? y1 : y2;

      width = abs(x2 - x1);
      height = abs(y2 - y1);

      gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
    }
  } else {
    gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

    for (i = 0; i < num; i++) {
      x1 = mxd2p(po[i]->x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
      y1 = mxd2p(po[i]->y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

      gdk_draw_line(d->win, gc, x1 - 4, y1, x1 + 5, y1);
      gdk_draw_line(d->win, gc, x1, y1 - 4, x1, y1 + 5);
    }
    if (num >= 1) {
      x1 = mxd2p(po[0]->x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
      y1 = mxd2p(po[0]->y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

      x0 = x1;
      y0 = y1;
    }
    for (i = 1; i < num; i++) {
      x1 = mxd2p(po[i]->x * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
      y1 = mxd2p(po[i]->y * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

      gdk_draw_line(d->win, gc, x0, y0, x1, y1);

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
  gdk_gc_set_dashes(gc, 0, Dashes, 2);

  if ((d->MouseX1 != d->MouseX2) || (d->MouseY1 != d->MouseY2)) {
    x1 = mxd2p(d->MouseX1 * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
    y1 = mxd2p(d->MouseY1 * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

    x2 = mxd2p(d->MouseX2 * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
    y2 = mxd2p(d->MouseY2 * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

    minx = (x1 < x2) ? x1 : x2;
    miny = (y1 < y2) ? y1 : y2;

    width = abs(x2 - x1);
    height = abs(y2 - y1);

    gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
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

  gdk_window_get_position(d->win, &x, &y);
  gdk_drawable_get_size(d->win, &width, &height);

  zoom = Menulocal.PaperZoom / 10000.0;

  x = mxd2p(d->CrossX * zoom + Menulocal.LeftMargin) - d->hscroll + d->cx;
  y = mxd2p(d->CrossY * zoom + Menulocal.TopMargin) - d->vscroll + d->cy;

  gdk_draw_line(d->win, gc, x, 0, x, height);
  gdk_draw_line(d->win, gc, 0, y, width, y);
  gdk_gc_set_function(gc, GDK_COPY);
}

static void
CheckGrid(int ofs, unsigned int state, int *x, int *y, double *zoom)
{
  int offset;
  int grid;

  if ((state &GDK_CONTROL_MASK) && (!ofs)) {
    if ((x != NULL) && (y != NULL)) {
      if (abs(*x) > abs(*y))
	*y = 0;
      else
	*x = 0;
    }
  }
  grid = Mxlocal->grid;
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

static gboolean
ViewerEvLButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;
  int minx, miny, maxx, maxy;
  int x1, y1, vx1, vx2, vy1, vy2;
  double cc, nn;
  struct pointslist *po;
  double zoom, zoom2;
  int i, j;
  struct narray *abbox;
  int bboxnum;
  int *bbox;
  char *inst;
  struct focuslist *focus;
  int vdpi;
  int selnum, sel, movenum, iline;
  struct narray *move, *movex, *movey;
  struct objlist *fileobj, *aobjx, *aobjy;
  char *argv[3];
  double dx, dy;
  int ax, ay, anum;
  struct narray iarray;
  char *axis;

  if (Menulock || GlobalLock)
    return FALSE;

  if (region)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  d->MouseX1 = d->MouseX2 = (mxp2d(point->x - d->cx + d->hscroll)
			     - Menulocal.LeftMargin) / zoom;

  d->MouseY1 = d->MouseY2 = (mxp2d(point->y - d->cy + d->vscroll)
			     - Menulocal.TopMargin) / zoom;

  d->MouseMode = MOUSENONE;

  dc = gdk_gc_new(d->win);

  if (d->MoveData) {
    fileobj = chkobject("file");
    if (fileobj) {
      selnum = arraynum(&SelList);

      for (i = 0; i < selnum; i++) {
	sel = *(int *) arraynget(&SelList, i);

	getobj(fileobj, "axis_x", EvalList[sel].id, 0, NULL, &axis);
	arrayinit(&iarray, sizeof(int));

	if (getobjilist(axis, &aobjx, &iarray, FALSE, NULL)) {
	  ax = -1;
	} else {
	  anum = arraynum(&iarray);
	  ax = (anum < 1) ? -1 : (*(int *) arraylast(&iarray));
	  arraydel(&iarray);
	}

	getobj(fileobj, "axis_y", EvalList[sel].id, 0, NULL, &axis);
	arrayinit(&iarray, sizeof(int));

	if (getobjilist(axis, &aobjy, &iarray, FALSE, NULL)) {
	  ay = -1;
	} else {
	  anum = arraynum(&iarray);
	  ay = (anum < 1) ? -1 : (*(int *) arraylast(&iarray));
	  arraydel(&iarray);
	}
	if ((ax != -1) && (ax != -1)) {
	  argv[0] = (char *) &(d->MouseX1);
	  argv[1] = (char *) &(d->MouseY1);
	  argv[2] = NULL;

	  if ((getobj(aobjx, "coordinate", ax, 2, argv, &dx) != -1)
	      && (getobj(aobjy, "coordinate", ay, 2, argv, &dy) != -1)) {

	    getobj(fileobj, "move_data", EvalList[sel].id, 0, NULL, &move);
	    getobj(fileobj, "move_data_x", EvalList[sel].id, 0, NULL, &movex);
	    getobj(fileobj, "move_data_y", EvalList[sel].id, 0, NULL, &movey);

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

	    if (arraynum(movex) < movenum) {
	      for (j = movenum - 1; j >= arraynum(movex); j--) {
		arrayndel(move, j);
	      }
	      movenum = arraynum(movex);
	    }

	    if (arraynum(movey) < movenum) {
	      for (j = movenum - 1; j >= arraynum(movey); j--) {
		arrayndel(move, j);
		arrayndel(movex, j);
	      }
	      movenum = arraynum(movey);
	    }

	    for (j = 0; j < movenum; j++) {
	      iline = *(int *) arraynget(move, j);
	      if (iline == EvalList[sel].line)
		break;
	    }

	    if (j == movenum) {
	      arrayadd(move, &(EvalList[sel].line));
	      arrayadd(movex, &dx);
	      arrayadd(movey, &dy);
	      NgraphApp.Changed = TRUE;
	    } else {
	      arrayput(move, &(EvalList[sel].line), j);
	      arrayput(movex, &dx, j);
	      arrayput(movey, &dy, j);
	      NgraphApp.Changed = TRUE;
	    }
	  }
	}
      }
      MessageBox(TopLevel, "Data points are moved.", "Confirm", MB_OK);
    }
    arraydel(&SelList);
    d->MoveData = FALSE;
    d->Capture = FALSE;
    SetCursor(GDK_LEFT_PTR);
  } else if ((d->Mode == PointB) || (d->Mode == LegendB)
	     || (d->Mode == AxisB)) {
    d->Capture = TRUE;

    if (arraynum(d->focusobj) != 0) {
      GetFocusFrame(&minx, &miny, &maxx, &maxy, 0, 0);

      if ((minx - 10 <= point->x) && (point->x <= minx - 4)
	  && (miny - 10 <= point->y) && (point->y <= miny - 4)) {
	GetLargeFrame(&(d->RefX2), &(d->RefY2), &(d->RefX1), &(d->RefY1));
	d->MouseMode = MOUSEZOOM1;
	SetCursor(GDK_TOP_LEFT_CORNER);
      } else if ((maxx + 4 <= point->x) && (point->x <= maxx + 10)
		 && (miny - 10 <= point->y) && (point->y <= miny - 4)) {
	GetLargeFrame(&(d->RefX1), &(d->RefY2), &(d->RefX2), &(d->RefY1));
	d->MouseMode = MOUSEZOOM2;
	SetCursor(GDK_TOP_RIGHT_CORNER);
      } else if ((maxx + 4 <= point->x) && (point->x <= maxx + 10)
		 && (maxy + 4 <= point->y) && (point->y <= maxy + 10)) {
	GetLargeFrame(&(d->RefX1), &(d->RefY1), &(d->RefX2), &(d->RefY2));
	d->MouseMode = MOUSEZOOM3;
	SetCursor(GDK_BOTTOM_RIGHT_CORNER);
      } else if ((minx - 10 <= point->x) && (point->x <= minx - 4)
		 && (maxy + 4 <= point->y) && (point->y <= maxy + 10)) {
	GetLargeFrame(&(d->RefX2), &(d->RefY1), &(d->RefX1), &(d->RefY2));
	d->MouseMode = MOUSEZOOM4;
	SetCursor(GDK_BOTTOM_LEFT_CORNER);
      } else {
	focus = *(struct focuslist **) arraynget(d->focusobj, 0);
	if ((arraynum(d->focusobj) == 1)
	    && ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL)) {

	  _exeobj(focus->obj, "bbox", inst, 0, NULL);
	  _getobj(focus->obj, "bbox", inst, &abbox);

	  bboxnum = arraynum(abbox);
	  bbox = (int *) arraydata(abbox);

	  for (j = 4; j < bboxnum; j += 2) {
	    x1 = mxd2p(bbox[j] * zoom + Menulocal.LeftMargin)
	      - d->hscroll + d->cx;

	    y1 = mxd2p(bbox[j + 1] * zoom + Menulocal.TopMargin)
	      - d->vscroll + d->cy;

	    if ((x1 - 3 <= point->x) && (point->x <= x1 + 3)
		&& (y1 - 3 <= point->y) && (point->y <= y1 + 3))
	      break;
	  }

	  if (j < bboxnum) {
	    d->MouseMode = MOUSECHANGE;
	    d->ChangePoint = (j - 4) / 2;
	    ShowFocusFrame(dc);
	    d->ShowFrame = FALSE;
	    SetCursor(GDK_CROSSHAIR);
	    d->ShowLine = TRUE;
	    d->LineX = d->LineY = 0;
	    ShowFocusLine(dc, d->ChangePoint);
	  }
	}
      }

      if (d->MouseMode == MOUSENONE) {
	if ((minx <= point->x) && (point->x <= maxx)
	    && (miny <= point->y) && (point->y <= maxy)) {
	  d->MouseMode = MOUSEDRAG;
	} else {
	  ShowFocusFrame(dc);
	  d->ShowFrame = FALSE;
	  arraydel2(d->focusobj);
	}
      } else if ((MOUSEZOOM1 <= d->MouseMode)
		 && (d->MouseMode <= MOUSEZOOM4)) {
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

	SetZoom(zoom2);

	vx1 = d->RefX1 + vx2 * zoom2;
	vy1 = d->RefY1 + vy2 * zoom2;

	d->MouseX1 = d->RefX1;
	d->MouseY1 = d->RefY1;

	d->MouseX2 = vx1;
	d->MouseY2 = vy1;

	ShowFrameRect(dc);

	d->ShowRect = TRUE;
      }
    }
    if (arraynum(d->focusobj) == 0) {
      d->MouseMode = MOUSEPOINT;
      d->ShowRect = TRUE;
    }
  } else if ((d->Mode == TrimB) || (d->Mode == DataB) || (d->Mode == EvalB)) {
    d->Capture = TRUE;
    d->MouseMode = MOUSEPOINT;
    d->ShowRect = TRUE;
  } else if ((d->Mode == MarkB) || (d->Mode == TextB)) {
    if (!d->Capture) {
      x1 = d->MouseX1;
      y1 = d->MouseY1;

      CheckGrid(TRUE, state, &x1, &y1, NULL);

      po = (struct pointslist *) memalloc(sizeof(struct pointslist));
      if (po) {
	po->x = x1;
	po->y = y1;
	arrayadd(d->points, &po);
      }

      ShowPoints(dc);
      d->Capture = TRUE;
    }
  } else if (d->Mode == ZoomB) {
    if (! ZoomLock) {
      ZoomLock = TRUE;
      if (state & GDK_SHIFT_MASK) {
	d->hscroll -= (d->cx - point->x);
	d->vscroll -= (d->cy - point->y);

	ChangeDPI(TRUE);
      } else if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &vdpi) != -1) {
	if (state & GDK_CONTROL_MASK) {
	  if ((int) (vdpi / sqrt(2)) >= 20) {
	    vdpi = nround(vdpi / sqrt(2));

	    if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
	      d->hscroll -= (d->cx - point->x);
	      d->vscroll -= (d->cy - point->y);

	    //	    ChangeDPI(FALSE);
	      ChangeDPI(TRUE);
	    }
	  } else {
	    MessageBeep(TopLevel);
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
	    MessageBeep(TopLevel);
	  }
	}
      }
      ZoomLock = FALSE;
    }
  } else {
    if (!(d->Capture)) {
      x1 = d->MouseX1;
      y1 = d->MouseY1;

      CheckGrid(TRUE, state, &x1, &y1, NULL);

      po = (struct pointslist *) memalloc(sizeof(struct pointslist));
      if (po) {
	po->x = x1;
	po->y = y1;
	arrayadd(d->points, &po);
      }

      po = (struct pointslist *) memalloc(sizeof(struct pointslist));
      if (po) {
	po->x = x1;
	po->y = y1;
	arrayadd(d->points, &po);
      }

      ShowPoints(dc);
      d->Capture = TRUE;
    }
  }
  g_object_unref(G_OBJECT(dc));

  return TRUE;
}

static gboolean
ViewerEvLButtonUp(unsigned int state, TPoint *point, struct Viewer *d)
{
  int x1, y1, x2, y2, err;
  GdkGC *dc;
  int i, num, dx, dy;
  struct focuslist *focus;
  struct objlist *obj;
  char *inst;
  char *argv[5];
  struct pointslist *po;
  int vx1, vx2, vy1, vy2;
  double cc, nn, zoom, zoom2;
  int zm;
  int axis;

  if (Menulock || GlobalLock)
    return FALSE;

  dc = gdk_gc_new(d->win);

  zoom = Menulocal.PaperZoom / 10000.0;
  axis = FALSE;

  if (d->Capture) {
    if ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB)
	|| (d->Mode == TrimB) || (d->Mode == DataB) || (d->Mode == EvalB)) {

      if (d->MouseMode == MOUSEDRAG) {
	d->Capture = FALSE;

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

	  argv[0] = (char *) &dx;
	  argv[1] = (char *) &dy;
	  argv[2] = NULL;

	  num = arraynum(d->focusobj);

	  PaintLock = TRUE;

	  for (i = num - 1; i >= 0; i--) {
	    focus = *(struct focuslist **) arraynget(d->focusobj, i);
	    obj = focus->obj;

	    if (obj == chkobject("axis"))
	      axis = TRUE;

	    if ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL) {
	      AddInvalidateRect(obj, inst);
	      _exeobj(obj, "move", inst, 2, argv);
	      NgraphApp.Changed = TRUE;
	      AddInvalidateRect(obj, inst);
	    }
	  }

	  PaintLock = FALSE;
	  d->FrameOfsX = d->FrameOfsY = 0;
	  ShowFocusFrame(dc);
	  d->ShowFrame = TRUE;

	  if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis))) {
	    d->allclear = FALSE;
	  }
	  UpdateAll();
	}
	d->MouseMode = MOUSENONE;
      } else if ((MOUSEZOOM1 <= d->MouseMode) && (d->MouseMode <= MOUSEZOOM4)) {
	d->Capture = FALSE;
	ShowFrameRect(dc);
	d->ShowRect = FALSE;

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

	if ((d->Mode != DataB) && (d->Mode != EvalB)) {
	  CheckGrid(FALSE, state, NULL, NULL, &zoom2);
	}

	zm = nround(zoom2 * 10000);
	ResetZoom();

	argv[0] = (char *) &zm;
	argv[1] = (char *) &(d->RefX1);
	argv[2] = (char *) &(d->RefY1);
	argv[3] = (char *) &Menulocal.preserve_width;
	argv[4] = NULL;

	num = arraynum(d->focusobj);
	PaintLock = TRUE;

	for (i = num - 1; i >= 0; i--) {
	  focus = *(struct focuslist **) arraynget(d->focusobj, i);
	  obj = focus->obj;

	  if (obj == chkobject("axis"))
	    axis = TRUE;

	  if ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL) {
	    AddInvalidateRect(obj, inst);
	    _exeobj(obj, "zooming", inst, 4, argv);
	    NgraphApp.Changed = TRUE;
	    AddInvalidateRect(obj, inst);
	  }
	}

	PaintLock = FALSE;

	d->FrameOfsX = d->FrameOfsY = 0;
	d->ShowFrame = TRUE;

	ShowFocusFrame(dc);

	if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis))) {
	  d->allclear = FALSE;
	}

	UpdateAll();
	SetCursor(GDK_LEFT_PTR);
	d->MouseMode = MOUSENONE;
      } else if (d->MouseMode == MOUSECHANGE) {
	d->Capture = FALSE;
	ShowFocusLine(dc, d->ChangePoint);
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

	  argv[0] = (char *) &(d->ChangePoint);
	  argv[1] = (char *) &dx;
	  argv[2] = (char *) &dy;
	  argv[3] = NULL;

	  focus = *(struct focuslist **) arraynget(d->focusobj, 0);

	  obj = focus->obj;

	  if (obj == chkobject("axis")) {
	    axis = TRUE;
	  }
	  PaintLock = TRUE;

	  if ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL) {
	    AddInvalidateRect(obj, inst);
	    _exeobj(obj, "change", inst, 3, argv);
	    NgraphApp.Changed = TRUE;
	    AddInvalidateRect(obj, inst);
	  }

	  PaintLock = FALSE;
	  d->FrameOfsX = d->FrameOfsY = 0;
	  d->ShowFrame = TRUE;
	  ShowFocusFrame(dc);

	  if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis))) {
	    d->allclear = FALSE;
	  }
	  UpdateAll();
	} else {
	  d->FrameOfsX = d->FrameOfsY = 0;
	  d->ShowFrame = TRUE;
	  ShowFocusFrame(dc);
	}
	SetCursor(GDK_LEFT_PTR);
	d->MouseMode = MOUSENONE;
      } else if (d->MouseMode == MOUSEPOINT) {
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

	err = mxp2d(2) / zoom;
	if (d->Mode == PointB) {
	  Match("legend", x1, y1, x2, y2, err);
	  Match("axis", x1, y1, x2, y2, err);
	  Match("merge", x1, y1, x2, y2, err);

	  d->FrameOfsX = d->FrameOfsY = 0;
	  d->ShowFrame = TRUE;

	  ShowFocusFrame(dc);
	} else if (d->Mode == LegendB) {
	  Match("legend", x1, y1, x2, y2, err);
	  Match("merge", x1, y1, x2, y2, err);

	  d->FrameOfsX = d->FrameOfsY = 0;
	  d->ShowFrame = TRUE;

	  ShowFocusFrame(dc);
	} else if (d->Mode == AxisB) {
	  Match("axis", x1, y1, x2, y2, err);

	  d->FrameOfsX = d->FrameOfsY = 0;
	  d->ShowFrame = TRUE;

	  ShowFocusFrame(dc);
	} else if (d->Mode == TrimB) {
	  Trimming(x1, y1, x2, y2);
	} else if (d->Mode == DataB) {
	  if (ViewerWinFileUpdate(x1, y1, x2, y2, err)) {
	    UpdateAll();
	  }
	} else {
	  Evaluate(x1, y1, x2, y2, err);
	}
      }
      d->MouseMode = MOUSENONE;
    } else if ((d->Mode == MarkB) || (d->Mode == TextB)) {
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
	po = *(struct pointslist **) arraynget(d->points, 0);
	po->x = x1;
	po->y = y1;
      }
      ShowPoints(dc);
      if (arraynum(d->points) == 1) {
	ViewerEvLButtonDblClk(state, point, d);
      }
    } else if ((d->Mode != MarkB) && (d->Mode != TextB)) {
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
	po = *(struct pointslist **) arraynget(d->points, num - 2);
      }
      if ((num < 2) || (po->x != x1) || (po->y != y1)) {
	po = (struct pointslist *) memalloc(sizeof(struct pointslist));
	if (po) {
	  po->x = x1;
	  po->y = y1;

	  arrayadd(d->points, &po);
	}
      }

      ShowPoints(dc);

      if ((d->Mode == RectB) || (d->Mode == ArcB) || (d->Mode == GaussB)
	  || (d->Mode == FrameB) || (d->Mode == SectionB)
	  || (d->Mode == CrossB) || (d->Mode == SingleB)) {

	if (arraynum(d->points) == 3) {
	  d->Capture = FALSE;
	  ViewerEvLButtonDblClk(state, point, d);
	}
      }
    }
  }
  g_object_unref(G_OBJECT(dc));

  return TRUE;
}

static gboolean
ViewerEvLButtonDblClk(unsigned int state, TPoint *point, struct Viewer *d)
{
  int i, id, num;
  struct objlist *obj = NULL, *obj2;
  char *inst;
  int ret = IDCANCEL;
  GdkGC *dc;
  int x1, y1, x2, y2, x, y, rx, ry;
  struct pointslist *po, **pdata;
  struct narray *parray;
  int idx, idy, idu, idr, idg, lenx, leny, oidx, oidy;
  double fx1, fy1;
  int dir;
  struct narray group;
  int type;
  char *argv[2];
  char *ref;

  if (Menulock || GlobalLock)
    return FALSE;

  if ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB)) {
    d->Capture = FALSE;
    ViewUpdate();
  } else if ((d->Mode != TrimB) && (d->Mode != DataB) && (d->Mode != EvalB)) {
    dc = gdk_gc_new(d->win);

    if ((d->Mode == MarkB) || (d->Mode == TextB)) {
      d->Capture = FALSE;
      num = arraynum(d->points);

      if (d->Mode == MarkB) {
	obj = chkobject("mark");
      } else {
	obj = chkobject("text");
      }

      if (obj != NULL) {
	if ((id = newobj(obj)) >= 0) {
	  if (num >= 1) {
	    po = *(struct pointslist **) arraynget(d->points, 0);
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
	    NgraphApp.Changed = TRUE;
	  }
	  PaintLock = FALSE;
	}
      }

      ShowPoints(dc);
      arraydel2(d->points);
      d->allclear = FALSE;

    } else if ((d->Mode == LineB) || (d->Mode == CurveB) || (d->Mode == PolyB)) {
      d->Capture = FALSE;
      num = arraynum(d->points);

      if (num >= 3) {
	if (d->Mode == LineB) {
	  obj = chkobject("line");
	} else if (d->Mode == CurveB) {
	  obj = chkobject("curve");
	} else if (d->Mode == PolyB) {
	  obj = chkobject("polygon");
	}

	if (obj != NULL) {
	  if ((id = newobj(obj)) >= 0) {
	    inst = chkobjinst(obj, id);
	    parray = arraynew(sizeof(int));

	    for (i = 0; i < num - 1; i++) {
	      po = *(struct pointslist **) arraynget(d->points, i);
	      arrayadd(parray, &(po->x));
	      arrayadd(parray, &(po->y));
	    }

	    _putobj(obj, "points", inst, parray);
	    PaintLock = TRUE;

	    if (d->Mode == LineB) {
	      LegendArrowDialog(&DlgLegendArrow, obj, id);
	      ret = DialogExecute(TopLevel, &DlgLegendArrow);
	    } else if (d->Mode == CurveB) {
	      LegendCurveDialog(&DlgLegendCurve, obj, id);
	      ret = DialogExecute(TopLevel, &DlgLegendCurve);
	    } else if (d->Mode == PolyB) {
	      LegendPolyDialog(&DlgLegendPoly, obj, id);
	      ret = DialogExecute(TopLevel, &DlgLegendPoly);
	    }
	    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	      delobj(obj, id);
	    } else {
	      AddList(obj, inst);
	      AddInvalidateRect(obj, inst);
	      NgraphApp.Changed = TRUE;
	    }
	    PaintLock = FALSE;
	  }
	}
      }

      ShowPoints(dc);
      arraydel2(d->points);

      d->allclear = FALSE;
    } else if ((d->Mode == RectB) || (d->Mode == ArcB)) {
      d->Capture = FALSE;

      num = arraynum(d->points);
      pdata = (struct pointslist **) arraydata(d->points);

      if (num >= 3) {
	if (d->Mode == RectB) {
	  obj = chkobject("rectangle");
	} else if (d->Mode == ArcB) {
	  obj = chkobject("arc");
	}

	if (obj != NULL) {
	  if ((id = newobj(obj)) >= 0) {
	    inst = chkobjinst(obj, id);
	    x1 = pdata[0]->x;
	    y1 = pdata[0]->y;
	    x2 = pdata[1]->x;
	    y2 = pdata[1]->y;
	    PaintLock = TRUE;

	    if (d->Mode == RectB) {
	      _putobj(obj, "x1", inst, &x1);
	      _putobj(obj, "y1", inst, &y1);
	      _putobj(obj, "x2", inst, &x2);
	      _putobj(obj, "y2", inst, &y2);
	      LegendRectDialog(&DlgLegendRect, obj, id);
	      ret = DialogExecute(TopLevel, &DlgLegendRect);
	    } else if (d->Mode == ArcB) {
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
	      NgraphApp.Changed = TRUE;
	    }
	    PaintLock = FALSE;
	  }
	}
      }

      ShowPoints(dc);
      arraydel2(d->points);
      d->allclear = FALSE;

    } else if (d->Mode == GaussB) {
      d->Capture = FALSE;
      num = arraynum(d->points);
      pdata = (struct pointslist **) arraydata(d->points);

      if (num >= 3) {
	obj = chkobject("curve");

	if (obj) {
	  id = newobj(obj);

	  if (id >= 0) {
	    inst = chkobjinst(obj, id);
	    x1 = pdata[0]->x;
	    y1 = pdata[0]->y;
	    x2 = pdata[1]->x;
	    y2 = pdata[1]->y;

	    if (x1 > x2) {
	      x = x1;
	      x1 = x2;
	      x2 = x;
	    }

	    if (y1 > y2) {
	      y = y1;
	      y1 = y2;
	      y2 = y;
	    }

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
		NgraphApp.Changed = TRUE;
	      }
	    }
	    PaintLock = FALSE;
	  }
	}
      }
      ShowPoints(dc);
      arraydel2(d->points);
      d->allclear = FALSE;
    } else if (d->Mode == SingleB) {
      d->Capture = FALSE;

      num = arraynum(d->points);
      pdata = (struct pointslist **) arraydata(d->points);

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
	      NgraphApp.Changed = TRUE;
	    }
	  }
	}
      }
      ShowPoints(dc);
      arraydel2(d->points);
      d->allclear = TRUE;
    } else if ((d->Mode == FrameB) || (d->Mode == SectionB)
	       || (d->Mode == CrossB)) {
      d->Capture = FALSE;
      num = arraynum(d->points);
      pdata = (struct pointslist **) arraydata(d->points);

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

	      ref = memalloc(ID_BUF_SIZE);
	      if (ref) {
		snprintf(ref, ID_BUF_SIZE, "axis:^%d", oidx);
		putobj(obj2, "axis_x", idg, ref);
	      }

	      getobj(obj, "oid", idy, 0, NULL, &oidy);

	      ref = memalloc(ID_BUF_SIZE);
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
	    NgraphApp.Changed = TRUE;
	  }
	}
      }
      ShowPoints(dc);
      arraydel2(d->points);
      d->allclear = TRUE;
    }
    g_object_unref(G_OBJECT(dc));
    UpdateAll();
  }

  return TRUE;
}

static gboolean
ViewerEvRButtonDown(unsigned int state, TPoint *point, struct Viewer *d, GdkEventButton *e)
{
  GdkGC *dc;
  int num;
  struct pointslist *po;
  double zoom;
  int vdpi;

  if (Menulock || GlobalLock)
    return FALSE;

  if (d->MoveData) {
    arraydel(&SelList);
    d->MoveData = FALSE;
    d->Capture = FALSE;
    SetCursor(GDK_LEFT_PTR);
    MessageBox(TopLevel, "Moving data points is canceled.", "Confirm", MB_OK);
  } else if (d->Capture && ((d->Mode == LineB) || (d->Mode == CurveB)
			    || (d->Mode == PolyB))) {
    zoom = Menulocal.PaperZoom / 10000.0;
    dc = gdk_gc_new(d->win);
    num = arraynum(d->points);
    if (num > 0) {
      ShowPoints(dc);
      arrayndel2(d->points, num - 1);
      if (num <= 2) {
	arraydel2(d->points);
	d->Capture = FALSE;
      } else {
	po = *(struct pointslist **) arraylast(d->points);
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
    }
    g_object_unref(G_OBJECT(dc));
  } else if (d->Mode == ZoomB) {
    if (! ZoomLock) {
      ZoomLock = TRUE;
      if (state & GDK_SHIFT_MASK) {
	d->hscroll -= (d->cx - point->x);
	d->vscroll -= (d->cy - point->y);
	ChangeDPI(TRUE);
      } else if (getobj(Menulocal.obj, "dpi", 0, 0, NULL, &vdpi) != -1) {
	if (state & GDK_CONTROL_MASK) {
	  if ((int) (vdpi * sqrt(2)) <= 620) {
	    vdpi = nround(vdpi * sqrt(2));
	    if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
	      d->hscroll -= (d->cx - point->x);
	      d->vscroll -= (d->cy - point->y);
	      //	    ChangeDPI(FALSE);
	      ChangeDPI(TRUE);
	    }
	  } else
	    MessageBeep(TopLevel);
	} else {
	  if ((int) (vdpi / sqrt(2)) >= 20) {
	    vdpi = nround(vdpi / sqrt(2));
	    if (putobj(Menulocal.obj, "dpi", 0, &vdpi) != -1) {
	      d->hscroll -= (d->cx - point->x);
	      d->vscroll -= (d->cy - point->y);
	      //	    ChangeDPI(FALSE);
	      ChangeDPI(TRUE);
	    }
	  } else
	    MessageBeep(TopLevel);
	}
      }
      ZoomLock = FALSE;
    }
  } else if (!(d->MoveData) && !(d->Capture) && (d->MouseMode == MOUSENONE)) {
    do_popup(e, d);
  }

  return TRUE;
}

static gboolean
ViewerEvMButtonDown(unsigned int state, TPoint *point, struct Viewer *d)
{
  if (Menulock || GlobalLock)
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

static gboolean
ViewerEvMouseMove(unsigned int state, TPoint *point, struct Viewer *d)
{
  GdkGC *dc;
  struct pointslist *po;
  int x, y, dx, dy, vx1, vx2, vy1, vy2;
  double cc, nn;
  double zoom, zoom2;

  if (Menulock || GlobalLock)
    return FALSE;

  zoom = Menulocal.PaperZoom / 10000.0;

  dx = (mxp2d(point->x + d->hscroll - d->cx) - Menulocal.LeftMargin) / zoom;
  dy = (mxp2d(point->y + d->vscroll - d->cy) - Menulocal.TopMargin) / zoom;

  if ((d->Mode != DataB) && (d->Mode != EvalB) && (d->Mode != ZoomB) &&
      (d->MouseMode != MOUSEPOINT)
      && (((d->Mode != PointB) && (d->Mode != LegendB) && (d->Mode != AxisB))
	  || (d->MouseMode != MOUSENONE))) {
    CheckGrid(TRUE, state, &dx, &dy, NULL);
  }

  CoordWinSetCoord(dx, dy);
  SetPoint(dx, dy);
  dc = gdk_gc_new(d->win);

  if (region == NULL) {
    if (d->ShowCross)
      ShowCrossGauge(dc);

    d->CrossX = dx;
    d->CrossY = dy;

    if (d->ShowCross)
      ShowCrossGauge(dc);
  }

  if (d->Capture) {
    if ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB)
	|| (d->Mode == TrimB) || (d->Mode == DataB) || (d->Mode == EvalB)) {

      if (d->MouseMode == MOUSEDRAG) {

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

      } else if ((MOUSEZOOM1 <= d->MouseMode)
		 && (d->MouseMode <= MOUSEZOOM4)) {

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

	SetZoom(zoom2);

	vx1 = d->RefX1 + vx2 * zoom2;
	vy1 = d->RefY1 + vy2 * zoom2;

	d->MouseX1 = d->RefX1;
	d->MouseY1 = d->RefY1;

	d->MouseX2 = vx1;
	d->MouseY2 = vy1;

	ShowFrameRect(dc);

      } else if (d->MouseMode == MOUSECHANGE) {
	ShowFocusLine(dc, d->ChangePoint);

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

	ShowFocusLine(dc, d->ChangePoint);

      } else if (d->MouseMode == MOUSEPOINT) {
	ShowFrameRect(dc);

	d->MouseX2 = (mxp2d(point->x + d->hscroll - d->cx)
		      - Menulocal.LeftMargin) / zoom;

	d->MouseY2 = (mxp2d(point->y + d->vscroll - d->cy)
		      - Menulocal.TopMargin) / zoom;

	ShowFrameRect(dc);
      }
    } else {

      ShowPoints(dc);

      if (arraynum(d->points) != 0) {
	po = *(struct pointslist **) arraylast(d->points);

	if (po != NULL) {
	  po->x = dx;
	  po->y = dy;
	}
      }
      ShowPoints(dc);
    }
  }
  g_object_unref(G_OBJECT(dc));

  return TRUE;
}

static gboolean
ViewerEvMouseMotion(GtkWidget *w, GdkEventMotion *e, gpointer client_data)
{
  struct Viewer *d;
  TPoint point;

  d = (struct Viewer *) client_data;

  point.x = e->x;
  point.y = e->y;
  return ViewerEvMouseMove(e->state, &point, d);
}

static void
popup_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
  struct Viewer *d;
  int cx, cy;

  d = (struct Viewer *) user_data;

  gdk_window_get_geometry(d->win, &cx, &cy, NULL, NULL, NULL);
  gtk_window_get_position(GTK_WINDOW(TopLevel), x, y);

  *x += cx;
  *y += cy;
}

static void
do_popup(GdkEventButton *event, struct Viewer *d)
{
  int button, event_time, i, num;
  GtkMenuPositionFunc func = NULL;
  struct focuslist *focus;
  struct objlist *obj;

  if (event) {
    button = event->button;
    event_time = event->time;
  } else {
    button = 0;
    event_time = gtk_get_current_event_time();
    func = popup_menu_position;
  }

  for (i = 0; i < VIEWER_POPUP_ITEM_NUM - 1; i++) {
    gtk_widget_set_sensitive(d->popup_item[i], FALSE);
  }
  if ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB)) {
    num = arraynum(d->focusobj);
    if (num > 0) {
      for (i = 0; i < 4; i++) {
	gtk_widget_set_sensitive(d->popup_item[i], TRUE);
      }
    }
    if (num == 1) {
      focus = *(struct focuslist **) arraynget(d->focusobj, 0);
      obj = focus->obj;
      for (i = 4; i < VIEWER_POPUP_ITEM_NUM; i++) {
	gtk_widget_set_sensitive(d->popup_item[i], TRUE);
      }
    }
  }
  gtk_menu_popup(GTK_MENU(d->popup), NULL, NULL, func, d,
		 button, event_time);
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
  double val;

  d = (struct Viewer *) client_data;

  switch (e->direction) {
  case GDK_SCROLL_UP:
    val = gtk_range_get_value(GTK_RANGE(d->VScroll));
    val -= SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(d->VScroll), val);
    ViewerEvVScroll(NULL, GTK_SCROLL_STEP_UP, val, d);
    return TRUE;
  case GDK_SCROLL_DOWN:
    val = gtk_range_get_value(GTK_RANGE(d->VScroll));
    val += SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(d->VScroll), val);
    ViewerEvVScroll(NULL, GTK_SCROLL_STEP_DOWN, val, d);
    return TRUE;
  case GDK_SCROLL_LEFT:
    val = gtk_range_get_value(GTK_RANGE(d->HScroll));
    val -= SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(d->HScroll), val);
    ViewerEvHScroll(NULL, GTK_SCROLL_STEP_UP, val, d);
    return TRUE;
  case GDK_SCROLL_RIGHT:
    val = gtk_range_get_value(GTK_RANGE(d->HScroll));
    val += SCROLL_INC;
    gtk_range_set_value(GTK_RANGE(d->HScroll), val);
    ViewerEvHScroll(NULL, GTK_SCROLL_STEP_DOWN, val, d);
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
  int dx = 0, dy = 0, mv;
  double zoom;

  if (Menulock || GlobalLock)
    return FALSE;

  d = (struct Viewer *) client_data;

  switch (e->keyval) {
  case GDK_Escape:
    NgraphApp.Interrupt = TRUE;
    return FALSE;
  case GDK_KP_Space:
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
      ViewTop();
      return TRUE;
    }
    break;
  case GDK_End:
    if (e->state & GDK_SHIFT_MASK) {
      ViewLast();
      return TRUE;
    }
    break;
  case GDK_Down:
  case GDK_Up:
  case GDK_Left:
  case GDK_Right:
    if (((d->MouseMode == MOUSENONE) || (d->MouseMode == MOUSEDRAG))
	&& ((d->Mode == PointB) || (d->Mode == LegendB)
	    || (d->Mode == AxisB))) {
      dc = gdk_gc_new(d->win);
      zoom = Menulocal.PaperZoom / 10000.0;
      ShowFocusFrame(dc);
      if (e->state & GDK_SHIFT_MASK) {
	mv = Mxlocal->grid / 10;
      } else {
	mv = Mxlocal->grid;
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
      SetCursor(GDK_PLUS);
      return TRUE;
    }
    break;
  case GDK_Control_L:
  case GDK_Control_R:
    if (d->Mode == ZoomB) {
      SetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  case GDK_BackSpace:
    ViewCross();
    return TRUE;
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
  struct focuslist *focus;
  char *inst;
  struct objlist *obj;
  char *argv[4];
  int axis;

  if (Menulock || GlobalLock)
    return FALSE;

  d = (struct Viewer *) client_data;

  switch (e->keyval) {
  case GDK_Shift_L:
  case GDK_Shift_R:
  case GDK_Control_L:
  case GDK_Control_R:
    if (d->Mode == ZoomB) {
      SetCursor(GDK_TARGET);
      return TRUE;
    }
    break;
  case GDK_Down:
  case GDK_Up:
  case GDK_Left:
  case GDK_Right:
    if (d->MouseMode == MOUSEDRAG) {
      dc = gdk_gc_new(d->win);
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
	focus = *(struct focuslist **) arraynget(d->focusobj, i);
	obj = focus->obj;
	if (obj == chkobject("axis"))
	  axis = TRUE;
	if ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL) {
	  AddInvalidateRect(obj, inst);
	  _exeobj(obj, "move", inst, 2, argv);
	  NgraphApp.Changed = TRUE;
	  AddInvalidateRect(obj, inst);
	}
      }
      PaintLock = FALSE;
      d->FrameOfsX = d->FrameOfsY = 0;
      d->ShowFrame = TRUE;
      ShowFocusFrame(dc);
      g_object_unref(G_OBJECT(dc));
      if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis)))
	d->allclear = FALSE;
      UpdateAll();
      d->MouseMode = MOUSENONE;
      return TRUE;
    }
    break;
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

  d = &(NgraphApp.Viewer);

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

  d = &(NgraphApp.Viewer);

  if ((e->count != 0) || GlobalLock)
    return TRUE;

  if (region == NULL)
    region = gdk_region_new();

  gc = gdk_gc_new(e->window);

  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid)) {
    /*
    struct savedstdio save;
    if (Mxlocal->autoredraw && ! d->ignoreredraw) {
      ignorestdio(&save);
      mx_redraw(Menulocal.obj, Menulocal.inst);
      restorestdio(&save);
    }
    */
    Mxlocal->scrollx = -d->cx + d->hscroll;
    Mxlocal->scrolly = -d->cy + d->vscroll;

    if (Mxlocal->pix) {
      gdk_region_get_clipbox(e->region, &rect);
      gdk_draw_drawable(e->window, gc, Mxlocal->pix,
			-d->cx + d->hscroll + rect.x,
			-d->cy + d->vscroll + rect.y,
			rect.x, rect.y,
			rect.width, rect.height);
    }
    MakeRuler(gc);

    if (d->ShowFrame)
      ShowFocusFrame(gc);

    ShowPoints(gc);

    if (d->ShowLine)
      ShowFocusLine(gc, d->ChangePoint);

    if (d->ShowRect)
      ShowFrameRect(gc);

    if (d->ShowCross)
      ShowCrossGauge(gc);

    g_object_unref(G_OBJECT(gc));
  }

  if (!PaintLock) {
    gdk_region_destroy(region);
    region = NULL;
  }

  gtk_widget_grab_focus(w);

  return FALSE;
}

static gboolean
ViewerEvVScroll(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
  struct Viewer *d;
  int dy, x, y, width, height;
  GdkGC *gc;

  d = &(NgraphApp.Viewer);

  dy = value - d->vscroll;

  if (dy != 0) {
    gc = gdk_gc_new(d->win);

    gdk_window_get_position(d->win, &x, &y);
    gdk_drawable_get_size(d->win, &width, &height);
    gdk_draw_drawable(d->win, gc, d->win, 0, dy, width, height, 0, 0);
    g_object_unref(gc);
  }
  gdk_window_invalidate_rect(d->win, NULL, TRUE);

  d->vscroll = value;

  return FALSE;
}

static gboolean
ViewerEvHScroll(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
  struct Viewer *d;
  int dx;
  int x, y, width, height;
  GdkGC *gc;

  d = &(NgraphApp.Viewer);

  dx = value - d->hscroll;

  if (dx != 0) {
    gc = gdk_gc_new(d->win);

    gdk_window_get_position(d->win, &x, &y);
    gdk_drawable_get_size(d->win, &width, &height);
    gdk_draw_drawable(d->win, gc, d->win, dx, 0, width, height, 0, 0);
    g_object_unref(gc);
  }
  gdk_window_invalidate_rect(d->win, NULL, TRUE);

  d->hscroll = value;

  return FALSE;
}

void
ViewerWinUpdate(int clear)
{
  int i, num;
  struct focuslist **focus;
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
  focus = (struct focuslist **) arraydata(d->focusobj);
  for (i = num - 1; i >= 0; i--) {
    if (chkobjoid(focus[i]->obj, focus[i]->oid) == -1)
      arrayndel2(d->focusobj, i);
  }

  if (d->allclear) {
    mx_clear(NULL);
    mx_redraw(Menulocal.obj, Menulocal.inst);
  } else if (region) {
    Mxlocal->region = region;
    mx_redraw(Menulocal.obj, Menulocal.inst);
    Mxlocal->region = NULL;
  }
  gdk_window_invalidate_rect(d->win, NULL, TRUE);

  d->allclear = TRUE;
}

static void
MakeRuler(GdkGC *gc)
{
  int Width, Height;
  int x, y, len;
  struct Viewer *d;
  int x1, y1, x2, y2, px, py, plen;
  int minx, miny, width, height;

  d = &(NgraphApp.Viewer);

  Width = Menulocal.PaperWidth;
  Height = Menulocal.PaperHeight;

  gdk_gc_set_function(gc, GDK_COPY);
  gdk_gc_set_line_attributes(gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

  x1 = mxd2p(0) - Mxlocal->scrollx;
  y1 = mxd2p(0) - Mxlocal->scrolly;

  x2 = mxd2p(Width) - 1 - Mxlocal->scrollx;
  y2 = mxd2p(Height) - 1 - Mxlocal->scrolly;

  minx = (x1 < x2) ? x1 : x2;
  miny = (y1 < y2) ? y1 : y2;

  width = abs(x2 - x1);
  height = abs(y2 - y1);

  gdk_gc_set_rgb_fg_color(gc, &gray);

  gdk_draw_rectangle(d->win, gc, FALSE, minx, miny, width, height);
  if (Mxlocal->ruler) {
    gdk_gc_set_rgb_fg_color(gc, &gray);
    for (x = 500; x < Width; x += 500) {
      if (!(x % 10000)) {
	gdk_gc_set_rgb_fg_color(gc, &red);
      }

      if (!(x % 10000)) {
	len = 225;
      } else if (!(x % 1000)) {
	len = 150;
      } else {
	len = 75;
      }

      px = mxd2p(x) - Mxlocal->scrollx;
      plen = mxd2p(len);

      gdk_draw_line(d->win, gc, px, y1, px, y1 + plen);
      gdk_draw_line(d->win, gc, px, y2, px, y2 - plen);

      if (!(x % 10000)) {
	gdk_gc_set_rgb_fg_color(gc, &gray);
      }
    }
    for (y = 500; y < Height; y += 500) {
      if (!(y % 10000)) {
	gdk_gc_set_rgb_fg_color(gc, &red);
      }

      if (!(y % 10000)) {
	len = 225;
      } else if (!(y % 1000)) {
	len = 150;
      } else {
	len = 75;
      }

      py = mxd2p(y) - Mxlocal->scrolly;
      plen = mxd2p(len);

      gdk_draw_line(d->win, gc, x1, py, x1 + plen, py);
      gdk_draw_line(d->win, gc, x2, py, x2 - plen, py);

      if (!(y % 10000)) {
	gdk_gc_set_rgb_fg_color(gc, &gray);
      }
    }
  }
}

void
Focus(struct objlist *fobj, int id)
{
  int oid;
  char *inst;
  struct narray *sarray;
  char **sdata;
  int snum;
  struct objlist *dobj;
  int did;
  char *dfield;
  int i;
  struct focuslist *focus;
  struct savedstdio save;
  GdkGC *dc;
  int man;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  if (chkobjchild(chkobject("legend"), fobj)) {
    CmViewerButtonArm(NgraphApp.viewb[1], NULL);
  } else if (chkobjchild(chkobject("axis"), fobj)) {
    CmViewerButtonArm(NgraphApp.viewb[10], NULL);
  } else {
    return;
  }

  UnFocus();
  dc = gdk_gc_new(d->win);

  inst = chkobjinst(fobj, id);

  _getobj(fobj, "oid", inst, &oid);

  ignorestdio(&save);

  if (fobj) {
    if (_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray))
      return;

    if ((snum = arraynum(sarray)) == 0)
      return;

    sdata = (char **) arraydata(sarray);

    for (i = 1; i < snum; i++) {
      dobj = getobjlist(sdata[i], &did, &dfield, NULL);
      if (dobj && (fobj == dobj) && (did == oid)) {
	focus = (struct focuslist *) memalloc(sizeof(struct focuslist));
	if (focus) {
	  if (chkobjchild(chkobject("axis"), dobj)) {
	    getobj(fobj, "group_manager", id, 0, NULL, &man);
	    getobj(fobj, "oid", man, 0, NULL, &did);
	  }
	  focus->obj = dobj;
	  focus->oid = did;
	  arrayadd(d->focusobj, &focus);
	  d->ShowFrame = TRUE;
	  ShowFocusFrame(dc);
	}
	d->MouseMode = MOUSENONE;
	break;
      }
    }
  }
  g_object_unref(G_OBJECT(dc));
  restorestdio(&save);
}


void
UnFocus(void)
{
  GdkGC *gc;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if (arraynum(d->focusobj) != 0) {
    gc = gdk_gc_new(d->win);
    ShowFocusFrame(gc);
    g_object_unref(G_OBJECT(gc));
    arraydel2(d->focusobj);
  }

  d->ShowFrame = FALSE;
  if (arraynum(d->points) != 0) {
    gc = gdk_gc_new(d->win);
    ShowPoints(gc);
    g_object_unref(G_OBJECT(gc));
    arraydel2(d->points);
  }
}

static void
create_pix(int w, int h)
{
  GdkRectangle rect;

  if (w == 0)
    w = 1;

  if (h == 0)
    h = 1;

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  Mxlocal->pix = gdk_pixmap_new(NgraphApp.Viewer.Win->window, w, h, -1);
  rect.x = 0;
  rect.y = 0;
  rect.width = w;
  rect.height = h;

  gdk_gc_set_clip_rectangle(Mxlocal->gc, &rect);
  gdk_gc_set_rgb_fg_color(Mxlocal->gc, &white);
  gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, TRUE, 0, 0, w, h);
}

void
OpenGC(void)
{
  int width, height, i;

  if (Mxlocal->gc != 0)
    return;

  Disp = gdk_display_get_default();

  width = mxd2p(Menulocal.PaperWidth);
  height = mxd2p(Menulocal.PaperHeight);

  if (width == 0)
    width = 1;

  if (height == 0)
    height = 1;

  Mxlocal->win = NgraphApp.Viewer.Win->window;
  Mxlocal->gc = gdk_gc_new(Mxlocal->win);
  create_pix(width, height);

  Mxlocal->offsetx = 0;
  Mxlocal->offsety = 0;
  Mxlocal->cpx = 0;
  Mxlocal->cpy = 0;
  Mxlocal->pixel_dot = Mxlocal->windpi / 25.4 / 100;
  Mxlocal->fontalias = NULL;

  for (i = 0; i < GTKFONTCASH; i++)
    (Mxlocal->font[i]).fontalias = NULL;

  Mxlocal->loadfont = 0;
  Mxlocal->loadfontf = -1;
  Mxlocal->linetonum = 0;
  Mxlocal->scrollx = 0;
  Mxlocal->scrolly = 0;
  Mxlocal->region = NULL;
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
  char *inst;
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

  if (Mxlocal->pix) {
    gdk_drawable_get_size(Mxlocal->pix, &w, &h);
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

  Mxlocal->scrollx = -d->cx + XPos;
  Mxlocal->scrolly = -d->cy + YPos;

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
    gdk_window_invalidate_rect(d->win, NULL, TRUE);
  }
}

void
CloseGC(void)
{
  int i;

  if (Mxlocal->gc == 0)
    return;
 
  if (Mxlocal->linetonum != 0) {
    gdk_draw_lines(Mxlocal->pix, Mxlocal->gc, Mxlocal->points, Mxlocal->linetonum);
    Mxlocal->linetonum = 0;
  }

  gdk_display_flush(Disp);
  g_object_unref(G_OBJECT(Mxlocal->gc));
  Mxlocal->gc = 0;

  memfree(Mxlocal->fontalias);
  for (i = 0; i < GTKFONTCASH; i++) {
    if (Mxlocal->font[i].fontalias != NULL) {
      memfree(Mxlocal->font[i].fontalias);
      pango_font_description_free(Mxlocal->font[i].font);
      Mxlocal->font[i].fontalias = NULL;
      Mxlocal->font[i].font = NULL;
    }
  }

  if (Mxlocal->region != NULL)
    gdk_region_destroy(Mxlocal->region);

  Mxlocal->region = NULL;

  UnFocus();
}

void
ReopenGC(void)
{
  if (Mxlocal->gc != 0) {
    if (Mxlocal->linetonum != 0) {
      gdk_draw_lines(Mxlocal->pix, Mxlocal->gc, Mxlocal->points, Mxlocal->linetonum);
      Mxlocal->linetonum = 0;
    }
    gdk_display_flush(Disp);
    g_object_unref(G_OBJECT(Mxlocal->gc));
  }
  Mxlocal->gc = gdk_gc_new(Mxlocal->win);
  Mxlocal->offsetx = 0;
  Mxlocal->offsety = 0;
  Mxlocal->cpx = 0;
  Mxlocal->cpy = 0;
  Mxlocal->pixel_dot = Mxlocal->windpi / 25.4 / 100;
  Mxlocal->loadfontf = -1;
  Mxlocal->linetonum = 0;

  if (Mxlocal->region != NULL)
    gdk_region_destroy(Mxlocal->region);

  Mxlocal->region = NULL;
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

  gc = gdk_gc_new(d->win);

  if (d->ShowFrame)
    ShowFocusFrame(gc);

  if (d->ShowLine)
    ShowFocusLine(gc, d->ChangePoint);

  if (d->ShowRect)
    ShowFrameRect(gc);

  if (d->ShowCross)
    ShowCrossGauge(gc);

  g_object_unref(G_OBJECT(gc));

  SShowFrame = d->ShowFrame;
  SShowLine = d->ShowLine;
  SShowRect = d->ShowRect;
  SShowCross = d->ShowCross;

  d->ShowFrame = d->ShowLine = d->ShowRect = d->ShowCross = FALSE;
  ReopenGC();
  if (region)
    gdk_region_destroy(region);

  region = NULL;

  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) != NULL) {
    _exeobj(Menulocal.GRAobj, "clear", Menulocal.GRAinst, 0, NULL);
    d->ignoreredraw = TRUE;
    //    ResetEvent();    /* XmUpdateDisplay(d->Win); */
    d->ignoreredraw = FALSE;
    _exeobj(Menulocal.GRAobj, "draw", Menulocal.GRAinst, 0, NULL);
    _exeobj(Menulocal.GRAobj, "flush", Menulocal.GRAinst, 0, NULL);
  }

  d->ShowFrame = SShowFrame;
  d->ShowLine = SShowLine;
  d->ShowRect = SShowRect;
  d->ShowCross = SShowCross;

  gc = gdk_gc_new(d->win);

  if (d->ShowFrame)
    ShowFocusFrame(gc);

  if (d->ShowLine)
    ShowFocusLine(gc, d->ChangePoint);

  if (d->ShowRect)
    ShowFrameRect(gc);

  if (d->ShowCross)
    ShowCrossGauge(gc);

  g_object_unref(G_OBJECT(gc));
  ResetStatusBar();

  ProgressDialogFinalize();

  gdk_window_invalidate_rect(d->win, NULL, TRUE);
}

void
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
  if (Menulock || GlobalLock)
    return;

  Draw(TRUE);

  FileWinUpdate(TRUE);
  AxisWinUpdate(TRUE);
}

void
CmViewerDrawB(GtkWidget *w, gpointer client_data)
{

  if (Menulock || GlobalLock)
    return;

  Draw(FALSE);

  FileWinUpdate(TRUE);
  AxisWinUpdate(TRUE);
}

void
CmViewerClear(void)
{
  if (Menulock || GlobalLock)
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
  struct focuslist *focus;
  struct objlist *obj, *dobj = NULL, *aobj;
  char *inst, *inst2, *dinst, *dfield;
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

  if (Menulock || GlobalLock)
    return;

  d = &(NgraphApp.Viewer);
  dc = gdk_gc_new(d->win);

  ShowFocusFrame(dc);
  d->ShowFrame = FALSE;

  num = arraynum(d->focusobj);
  axis = FALSE;
  PaintLock = TRUE;

  for (i = num - 1; i >= 0; i--) {
    focus = *(struct focuslist **) arraynget(d->focusobj, i);
    if (focus && ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL)) {
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

	    if ((group2 != NULL) && (group2[0] == type)) {
	      if (strcmp(group + 2, group2 + 2) == 0) {
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
	    }
	  }

	  if (((type == 's') || (type == 'f')) && findX && findY
	      && !_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray)
	      && ((snum = arraynum(sarray)) >= 0)) {
	    sdata = (char **) arraydata(sarray);

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
			  && (*(int *) arraylast(&iarray)
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
			  && (*(int *) arraylast(&iarray)
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
	      arrayndel2(d->focusobj, i);
	    }

	    if (!findG) {
	      if (idg != -1) {
		if ((dinst = chkobjinst(dobj, idg)) != NULL)
		  AddList(dobj, dinst);
	      }
	    }

	    if (ret != IDCANCEL)
	      NgraphApp.Changed = TRUE;
	  } else if ((type == 'c') && findX && findY) {
	    getobj(obj, "x", idx, 0, NULL, &x1);
	    getobj(obj, "y", idy, 0, NULL, &y1);
	    getobj(obj, "length", idx, 0, NULL, &lenx);
	    getobj(obj, "length", idy, 0, NULL, &leny);

	    CrossDialog(&DlgCross, x1, y1, lenx, leny, obj, idx, idy);

	    ret = DialogExecute(TopLevel, &DlgCross);

	    if (ret == IDDELETE) {
	      AxisDel(aid);
	      arrayndel2(d->focusobj, i);
	    }

	    if (ret != IDCANCEL)
	      NgraphApp.Changed = TRUE;
	  }
	} else {
	  AxisDialog(&DlgAxis, obj, id, TRUE);
	  ret = DialogExecute(TopLevel, &DlgAxis);

	  if (ret == IDDELETE) {
	    AxisDel(id);
	    arrayndel2(d->focusobj, i);
	  }

	  if (ret != IDCANCEL)
	    NgraphApp.Changed = TRUE;
	}
      } else {
	AddInvalidateRect(obj, inst);

	if (obj == chkobject("line")) {
	  LegendArrowDialog(&DlgLegendArrow, obj, id);
	  ret = DialogExecute(TopLevel, &DlgLegendArrow);
	} else if (obj == chkobject("curve")) {
	  LegendCurveDialog(&DlgLegendCurve, obj, id);
	  ret = DialogExecute(TopLevel, &DlgLegendCurve);
	} else if (obj == chkobject("polygon")) {
	  LegendPolyDialog(&DlgLegendPoly, obj, id);
	  ret = DialogExecute(TopLevel, &DlgLegendPoly);
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
	  delobj(obj, id);
	}

	if (ret == IDOK)
	  AddInvalidateRect(obj, inst);

	if (ret != IDCANCEL)
	  NgraphApp.Changed = TRUE;
      }
    }
  }
  PaintLock = FALSE;

  if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis)))
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
  struct focuslist *focus;
  struct objlist *obj;
  char *inst;
  GdkGC *dc;
  int axis;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if ((d->MouseMode == MOUSENONE)
      && ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB))) {

    dc = gdk_gc_new(d->win);
    ShowFocusFrame(dc);
    d->ShowFrame = FALSE;

    axis = FALSE;
    PaintLock = TRUE;

    num = arraynum(d->focusobj);

    for (i = num - 1; i >= 0; i--) {
      focus = *(struct focuslist **) arraynget(d->focusobj, i);
      obj = focus->obj;

      if ((inst = chkobjinstoid(obj, focus->oid)) != NULL) {
	AddInvalidateRect(obj, inst);
	DelList(obj, inst);
	_getobj(obj, "id", inst, &id);

	if (obj == chkobject("axis")) {
	  AxisDel(id);
	  axis = TRUE;
	} else {
	  delobj(obj, id);
	}
	NgraphApp.Changed = TRUE;
      }
    }
    PaintLock = FALSE;
    if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis)))
      d->allclear = FALSE;
    if (num != 0)
      UpdateAll();
  }
}

static void
ViewTop(void)
{
  int id, num;
  struct focuslist *focus;
  struct objlist *obj;
  char *inst;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if ((d->MouseMode == MOUSENONE)
      && ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB))) {
    num = arraynum(d->focusobj);

    if (num == 1) {
      focus = *(struct focuslist **) arraynget(d->focusobj, 0);
      obj = focus->obj;

      if (chkobjchild(chkobject("legend"), obj)) {
	if ((inst = chkobjinstoid(obj, focus->oid)) != NULL) {
	  DelList(obj, inst);
	  _getobj(obj, "id", inst, &id);
	  movetopobj(obj, id);
	  AddList(obj, inst);
	  NgraphApp.Changed = TRUE;
	  d->allclear = TRUE;
	  UpdateAll();
	}
      }
    }
  }
}

static void
ViewLast(void)
{
  int id, num;
  struct focuslist *focus;
  struct objlist *obj;
  char *inst;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);
  if ((d->MouseMode == MOUSENONE)
      && ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB))) {

    num = arraynum(d->focusobj);

    if (num == 1) {
      focus = *(struct focuslist **) arraynget(d->focusobj, 0);
      obj = focus->obj;

      if (chkobjchild(chkobject("legend"), obj)) {
	inst = chkobjinstoid(obj, focus->oid);
	if (inst) {
	  DelList(obj, inst);
	  _getobj(obj, "id", inst, &id);
	  movelastobj(obj, id);
	  AddList(obj, inst);
	  NgraphApp.Changed = TRUE;
	  d->allclear = TRUE;
	  UpdateAll();
	}
      }
    }
  }
}

static void
ncopyobj(struct objlist *obj, int id1, int id2)
{
  int j;
  char *field;
  int perm, type;

  for (j = 0; j < chkobjfieldnum(obj); j++) {
    field = chkobjfieldname(obj, j);
    perm = chkobjperm(obj, field);
    type = chkobjfieldtype(obj, field);
    if (((perm & NREAD) != 0) && ((perm & NWRITE) != 0) && (type < NVFUNC))
      copyobj(obj, field, id1, id2);
  }
}

static void
ViewCopy(void)
{
  int i, j, id, id2, did, num;
  struct focuslist *focus;
  struct objlist *obj, *dobj, *aobj;
  char *inst, *inst2, *dinst, *dfield;
  GdkGC *dc;
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
  int axis;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  if ((d->MouseMode == MOUSENONE)
      && ((d->Mode == PointB) || (d->Mode == LegendB) || (d->Mode == AxisB))) {

    dc = gdk_gc_new(d->win);

    ShowFocusFrame(dc);
    d->ShowFrame = FALSE;

    axis = FALSE;
    PaintLock = TRUE;

    num = arraynum(d->focusobj);

    for (i = 0; i < num; i++) {
      focus = *(struct focuslist **) arraynget(d->focusobj, i);

      if (focus && ((inst = chkobjinstoid(focus->obj, focus->oid)) != NULL)) {
	obj = focus->obj;
	_getobj(obj, "id", inst, &id);

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

	      if ((group2 != NULL) && (group2[0] == type)) {
		if (strcmp(group + 2, group2 + 2) == 0) {
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
	      }
	    }

	    if (((type == 's') || (type == 'f')) && findX && findY
		&& !_getobj(Menulocal.obj, "_list", Menulocal.inst, &sarray)
		&& ((snum = arraynum(sarray)) >= 0)) {
	      sdata = (char **) arraydata(sarray);
	      for (j = 1; j < snum; j++) {
		if (((dobj =
		      getobjlist(sdata[j], &did, &dfield, NULL)) != NULL)
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
			    && (*(int *)
				arraylast(&iarray) == idx))
			  matchx = TRUE;
		      }
		      arraydel(&iarray);
		    }
		    if (axisy != NULL) {
		      arrayinit(&iarray, sizeof(int));
		      if (!getobjilist(axisy, &aobj, &iarray, FALSE, NULL)) {
			if ((arraynum(&iarray) >= 1)
			    && (obj == aobj)
			    && (*(int *)
				arraylast(&iarray) == idy))
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
	      if ((idx2 = newobj(obj)) >= 0) {
		ncopyobj(obj, idx2, idx);
		inst2 = chkobjinst(obj, idx2);
		_getobj(obj, "oid", inst2, &oidx);
		AddList(obj, inst2);
		AddInvalidateRect(obj, inst2);
		NgraphApp.Changed = TRUE;
	      } else {
		AddInvalidateRect(obj, inst);
	      }

	      if ((idy2 = newobj(obj)) >= 0) {
		ncopyobj(obj, idy2, idy);
		inst2 = chkobjinst(obj, idy2);
		_getobj(obj, "oid", inst2, &oidy);
		AddList(obj, inst2);
		AddInvalidateRect(obj, inst2);
		NgraphApp.Changed = TRUE;
	      } else {
		AddInvalidateRect(obj, inst);
	      }

	      if ((idu2 = newobj(obj)) >= 0) {
		ncopyobj(obj, idu2, idu);
		inst2 = chkobjinst(obj, idu2);
		if (idx2 >= 0) {
		  axisx = (char *) memalloc(ID_BUF_SIZE);
		  if (axisx) {
		    snprintf(axisx, ID_BUF_SIZE, "axis:^%d", oidx);
		    putobj(obj, "reference", idu2, axisx);
		  }
		}
		AddList(obj, inst2);
		AddInvalidateRect(obj, inst2);
		NgraphApp.Changed = TRUE;
	      } else {
		AddInvalidateRect(obj, inst);
	      }

	      if ((idr2 = newobj(obj)) >= 0) {
		ncopyobj(obj, idr2, idr);
		inst2 = chkobjinst(obj, idr2);
		if (idy2 >= 0) {
		  axisy = (char *) memalloc(ID_BUF_SIZE);
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

		NgraphApp.Changed = TRUE;
	      } else {
		AddInvalidateRect(obj, inst);
	      }

	      if (findG) {
		dobj = chkobject("axisgrid");
		if ((idg2 = newobj(dobj)) >= 0) {
		  ncopyobj(dobj, idg2, idg);
		  inst2 = chkobjinst(dobj, idg2);
		  if (idx2 >= 0 && idu2 >= 0) {
		    axisx = (char *) memalloc(ID_BUF_SIZE);
		    if (axisx) {
		      snprintf(axisx, ID_BUF_SIZE, "axis:^%d", oidx);
		      putobj(dobj, "axis_x", idg2, axisx);
		    }
		  }
		  if (idy2 >= 0 && idr2 >= 0) {
		    axisy = (char *) memalloc(ID_BUF_SIZE);
		    if (axisy) {
		      snprintf(axisy, ID_BUF_SIZE, "axis:^%d", oidy);
		      putobj(dobj, "axis_y", idg2, axisy);
		    }
		  }
		  AddList(dobj, inst2);
		  NgraphApp.Changed = TRUE;
		}
	      }
	    } else if ((type == 'c') && findX && findY) {
	      if ((idx2 = newobj(obj)) >= 0) {
		ncopyobj(obj, idx2, idx);
		inst2 = chkobjinst(obj, idx2);
		_getobj(obj, "oid", inst2, &oidx);
		AddList(obj, inst2);
		AddInvalidateRect(obj, inst2);
		NgraphApp.Changed = TRUE;
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
		NgraphApp.Changed = TRUE;

	      } else {
		AddInvalidateRect(obj, inst);
	      }

	      if (idx2 >= 0 && idy2 >= 0) {
		axisy = (char *) memalloc(ID_BUF_SIZE);
		if (axisy) {
		  snprintf(axisy, ID_BUF_SIZE, "axis:^%d", oidy);
		  putobj(obj, "adjust_axis", idx2, axisy);
		}
		axisx = (char *) memalloc(ID_BUF_SIZE);
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
	      NgraphApp.Changed = TRUE;
	    } else {
	      AddInvalidateRect(obj, inst);
	    }
	  }
	} else {
	  if ((id2 = newobj(obj)) >= 0) {
	    ncopyobj(obj, id2, id);
	    inst2 = chkobjinst(obj, id2);
	    _getobj(obj, "oid", inst2, &(focus->oid));
	    AddList(obj, inst2);
	    AddInvalidateRect(obj, inst2);
	    NgraphApp.Changed = TRUE;
	  } else {
	    AddInvalidateRect(obj, inst);
	  }
	}
      }
    }
    PaintLock = FALSE;

    if ((d->Mode == LegendB) || ((d->Mode == PointB) && (!axis)))
      d->allclear = FALSE;

    UpdateAll();
    ShowFocusFrame(dc);
    d->ShowFrame = TRUE;
    g_object_unref(G_OBJECT(dc));
  }
}

static void
ViewCross(void)
{
  GdkGC *dc;
  struct Viewer *d;

  d = &(NgraphApp.Viewer);

  dc = gdk_gc_new(d->win);

  if (d->ShowCross) {
    ShowCrossGauge(dc);
    d->ShowCross = FALSE;
  } else {
    ShowCrossGauge(dc);
    d->ShowCross = TRUE;
  }

  g_object_unref(G_OBJECT(dc));
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
  case VIEW_COPY:
    ViewCopy();
    break;
  case VIEW_TOP:
    ViewTop();
    break;
  case VIEW_LAST:
    ViewLast();
    break;
  case VIEW_CROSS:
    ViewCross();
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

  UnFocus();

  Mode = NgraphApp.Viewer.Mode = (int) client_data;

  if ((Mode == PointB) || (Mode == LegendB) || (Mode == AxisB)
      || (Mode == TrimB) || (Mode == DataB) || (Mode == EvalB)) {
    SetCursor(GDK_LEFT_PTR);
  } else if (Mode == TextB) {
    SetCursor(GDK_XTERM);
  } else if (Mode == ZoomB) {
    SetCursor(GDK_TARGET);
  } else {
    SetCursor(GDK_CROSSHAIR);
  }
  NgraphApp.Viewer.Capture = FALSE;
  NgraphApp.Viewer.MouseMode = MOUSENONE;
}
