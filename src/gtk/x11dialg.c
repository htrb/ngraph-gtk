/* 
 * $Id: x11dialg.c,v 1.47 2009/12/10 02:57:27 hito Exp $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>

#include "ngraph.h"
#include "object.h"
#include "mathfn.h"
#include "nstring.h"

#include "ogra2cairo.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11dialg.h"

#include "gtk_liststore.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

void ResetEvent();

struct line_style FwLineStyle[] = {
  {N_("solid"),      "",                        0},
  {N_("dot"),        "100 100",                 2},
  {N_("short dash"), "150 150",                 2},
  {N_("dash"),       "450 150",                 2},
  {N_("dot dash"),   "450 150 150 150",         4},
  {N_("2-dot dash"), "450 150 150 150 150 150", 6},
  {N_("dot 2-dash"), "450 150 450 150 150 150", 6},
};
#define CLINESTYLE (sizeof(FwLineStyle) / sizeof(*FwLineStyle))

char *FwNumStyle[] =
  {N_("auto"), "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
int FwNumStyleNum = sizeof(FwNumStyle) / sizeof(*FwNumStyle);

struct FileDialog DlgFile;
struct FileDialog DlgFileDef;
struct EvalDialog DlgEval;
struct MathDialog DlgMath;
struct MathTextDialog DlgMathText;
struct FitDialog DlgFit;
struct FitLoadDialog DlgFitLoad;
struct FitSaveDialog DlgFitSave;
struct FileMoveDialog DlgFileMove;
struct FileMaskDialog DlgFileMask;
struct FileLoadDialog DlgFileLoad;
struct FileMathDialog DlgFileMath;
struct SectionDialog DlgSection;
struct CrossDialog DlgCross;
struct AxisDialog DlgAxis;
struct GridDialog DlgGrid;
struct ZoomDialog DlgZoom;
struct MergeDialog DlgMerge;
struct LegendDialog DlgLegendCurve;
struct LegendDialog DlgLegendPoly;
struct LegendDialog DlgLegendArrow;
struct LegendDialog DlgLegendRect;
struct LegendDialog DlgLegendArc;
struct LegendDialog DlgLegendMark;
struct LegendDialog DlgLegendText;
struct LegendDialog DlgLegendTextDef;
struct LegendGaussDialog DlgLegendGauss;
struct PageDialog DlgPage;
struct SwitchDialog DlgSwitch;
struct DirectoryDialog DlgDirectory;
struct LoadDialog DlgLoad;
struct PrmDialog DlgPrm;
struct SaveDialog DlgSave;
struct DriverDialog DlgDriver;
struct PrintDialog DlgPrinter;
struct OutputDataDialog DlgOutputData;
struct DefaultDialog DlgDefault;
struct SetScriptDialog DlgSetScript;
struct PrefScriptDialog DlgPrefScript;
struct SetDriverDialog DlgSetDriver;
struct PrefDriverDialog DlgPrefDriver;
struct PrefFontDialog DlgPrefFont;
struct MiscDialog DlgMisc;
struct ExViewerDialog DlgExViewer;
struct ViewerDialog DlgViewer;
struct SelectDialog DlgSelect;
struct CopyDialog DlgCopy;
struct OutputImageDialog DlgImageOut;

void
initdialog(void)
{
  DlgFile.widget = NULL;
  DlgFile.focus = NULL;
  DlgFile.resource = N_("data");
  DlgFile.mark.widget = NULL;
  DlgFile.mark.focus = NULL;
  DlgFile.mark.resource = N_("mark selection");
  DlgFileDef.widget = NULL;
  DlgFileDef.focus = NULL;
  DlgFileDef.resource = N_("data default");
  DlgFileDef.mark.widget = NULL;
  DlgFileDef.mark.focus = NULL;
  DlgFileDef.mark.resource = N_("mark selection");
  DlgEval.widget = NULL;
  DlgEval.focus = NULL;
  DlgEval.resource = N_("evaluation");
  DlgMath.widget = NULL;
  DlgMath.focus = NULL;
  DlgMath.resource = N_("math");
  DlgMathText.widget = NULL;
  DlgMathText.focus = NULL;
  DlgMathText.resource = N_("math text");
  DlgFit.widget = NULL;
  DlgFit.focus = NULL;
  DlgFit.resource = N_("fit");
  DlgFitLoad.widget = NULL;
  DlgFitLoad.focus = NULL;
  DlgFitLoad.resource = N_("fit load");
  DlgFitSave.widget = NULL;
  DlgFitSave.focus = NULL;
  DlgFitSave.Profile = NULL;
  DlgFitSave.resource = N_("fit save");
  DlgFileMove.widget = NULL;
  DlgFileMove.focus = NULL;
  DlgFileMove.resource = N_("data move");
  DlgFileMask.widget = NULL;
  DlgFileMask.focus = NULL;
  DlgFileMask.resource = N_("data mask");
  DlgFileLoad.widget = NULL;
  DlgFileLoad.focus = NULL;
  DlgFileLoad.resource = N_("data load");
  DlgFileMath.widget = NULL;
  DlgFileMath.focus = NULL;
  DlgFileMath.resource = N_("data math");
  DlgSection.widget = NULL;
  DlgSection.focus = NULL;
  DlgSection.resource = N_("Frame/Section Graph");
  DlgCross.widget = NULL;
  DlgCross.focus = NULL;
  DlgCross.resource = N_("Cross Graph");
  DlgAxis.widget = NULL;
  DlgAxis.focus = NULL;
  DlgAxis.resource = N_("axis");
  DlgGrid.widget = NULL;
  DlgGrid.focus = NULL;
  DlgGrid.resource = N_("grid");
  DlgZoom.widget = NULL;
  DlgZoom.focus = NULL;
  DlgZoom.resource = N_("Scale Zoom");
  DlgMerge.widget = NULL;
  DlgMerge.focus = NULL;
  DlgMerge.resource = N_("merge");
  DlgLegendCurve.widget = NULL;
  DlgLegendCurve.focus = NULL;
  DlgLegendCurve.resource = N_("legend curve");
  DlgLegendPoly.widget = NULL;
  DlgLegendPoly.focus = NULL;
  DlgLegendPoly.resource = N_("legend polygon");
  DlgLegendArrow.widget = NULL;
  DlgLegendArrow.focus = NULL;
  DlgLegendArrow.resource = N_("legend line");
  DlgLegendRect.widget = NULL;
  DlgLegendRect.focus = NULL;
  DlgLegendRect.resource = N_("legend rectangle");
  DlgLegendArc.widget = NULL;
  DlgLegendArc.focus = NULL;
  DlgLegendArc.resource = N_("legend arc");
  DlgLegendMark.widget = NULL;
  DlgLegendMark.focus = NULL;
  DlgLegendMark.resource = N_("legend mark");
  DlgLegendMark.mark.widget = NULL;
  DlgLegendMark.mark.focus = NULL;
  DlgLegendMark.mark.resource = N_("mark selection");
  DlgLegendText.widget = NULL;
  DlgLegendText.focus = NULL;
  DlgLegendText.resource = N_("legend text");
  DlgLegendTextDef.widget = NULL;
  DlgLegendTextDef.focus = NULL;
  DlgLegendTextDef.resource = N_("textdefault");
  DlgLegendGauss.widget = NULL;
  DlgLegendGauss.focus = NULL;
  DlgLegendGauss.resource = N_("legend gauss");
  DlgPage.widget = NULL;
  DlgPage.focus = NULL;
  DlgPage.resource = N_("page");
  DlgSwitch.widget = NULL;
  DlgSwitch.focus = NULL;
  DlgSwitch.resource = N_("drawobj");
  DlgDirectory.widget = NULL;
  DlgDirectory.focus = NULL;
  DlgDirectory.resource = N_("directory");
  DlgLoad.widget = NULL;
  DlgLoad.focus = NULL;
  DlgLoad.resource = N_("load");
  DlgPrm.widget = NULL;
  DlgPrm.focus = NULL;
  DlgPrm.resource = N_("loadprm");
  DlgSave.widget = NULL;
  DlgSave.focus = NULL;
  DlgSave.resource = N_("save");
  DlgDriver.widget = NULL;
  DlgDriver.focus = NULL;
  DlgDriver.resource = N_("driver");
  DlgPrinter.widget = NULL;
  DlgPrinter.focus = NULL;
  DlgPrinter.resource = N_("print");
  DlgOutputData.widget = NULL;
  DlgOutputData.focus = NULL;
  DlgOutputData.resource = N_("output data");
  DlgDefault.widget = NULL;
  DlgDefault.focus = NULL;
  DlgDefault.resource = N_("save default");
  DlgSetScript.widget = NULL;
  DlgSetScript.focus = NULL;
  DlgSetScript.resource = N_("Add-in script");
  DlgPrefScript.widget = NULL;
  DlgPrefScript.focus = NULL;
  DlgPrefScript.resource = N_("Add-in script setup");
  DlgSetDriver.widget = NULL;
  DlgSetDriver.focus = NULL;
  DlgSetDriver.resource = N_("External driver");
  DlgPrefDriver.widget = NULL;
  DlgPrefDriver.focus = NULL;
  DlgPrefDriver.resource = N_("External driver setup");
  DlgPrefFont.widget = NULL;
  DlgPrefFont.focus = NULL;
  DlgPrefFont.resource = N_("Font");
  DlgMisc.widget = NULL;
  DlgMisc.focus = NULL;
  DlgMisc.resource = N_("Miscellaneous");
  DlgExViewer.widget = NULL;
  DlgExViewer.focus = NULL;
  DlgExViewer.resource = N_("External viewer");
  DlgViewer.widget = NULL;
  DlgViewer.focus = NULL;
  DlgViewer.resource = N_("Viewer");
  DlgSelect.widget = NULL;
  DlgSelect.focus = NULL;
  DlgSelect.resource = N_("multi select");
  DlgCopy.widget = NULL;
  DlgCopy.focus = NULL;
  DlgCopy.resource = N_("single select");
  DlgImageOut.widget = NULL;
  DlgImageOut.focus = NULL;
  DlgImageOut.resource = N_("output image");
}

static gboolean
multi_list_default_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  GtkTreeSelection *sel;
  struct SelectDialog *d;
  int n;

  d = (struct SelectDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    n = gtk_tree_selection_count_selected_rows(sel);
    if (n < 1)
    return FALSE;

    gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);

    return TRUE;
  }
  return FALSE;
}

static int
search_id(GtkWidget *list, int id)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  int r, a, i;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
  r = gtk_tree_model_get_iter_first(model, &iter);

  i = 0;
  while (r) {
    gtk_tree_model_get(model, &iter, 0, &a, -1);
    if (a == id)
      return i;

    r = gtk_tree_model_iter_next(model, &iter);
    i++;
  }

  return -1;
}

static void
SelectDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  char *s;
  struct SelectDialog *d;
  int i, *seldata, selnum, a;
  GtkWidget *swin, *w, *hbox;
  GtkTreeIter iter;
  n_list_store list[] = {
    {"id",       G_TYPE_INT,    TRUE, FALSE, NULL, FALSE, 0, 0, 0, 0, PANGO_ELLIPSIZE_NONE, 0},
    {"property", G_TYPE_STRING, TRUE, FALSE, NULL, FALSE, 0, 0, 0, 0, PANGO_ELLIPSIZE_END, 0},
  } ;

  d = (struct SelectDialog *) data;

  if (makewidget) {
    d->list = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_all(d->list);
    list_store_set_selection_mode(d->list, GTK_SELECTION_MULTIPLE);
    g_signal_connect(d->list, "button-press-event", G_CALLBACK(multi_list_default_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(multi_list_default_cb), d);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), d->list);

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_All"), IDSALL);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }

  list_store_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    s = d->cb(d->Obj, i);
    if (s) {
      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, i);
      list_store_set_string(d->list, &iter, 1, CHK_STR(s));
      g_free(s);
    }
  }

  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(XtNameToWidget(w, "*list"), XmNwidth, 200, NULL);
  }
  */

  if (chkobjlastinst(d->Obj) == 0) {
    list_store_select_nth(d->list, 0);
  } else if (d->isel) {
    seldata = arraydata(d->isel);
    selnum = arraynum(d->isel);

    for (i = 0; i < selnum; i++) {
      a = search_id(d->list, seldata[i]);
      if (a >= 0) {
	list_store_multi_select_nth(d->list, a, a);
      }
    }
  }
}

static void
select_tree_selection_foreach_cb(GtkTreeModel *model, GtkTreePath *path,
				 GtkTreeIter *iter, gpointer data)
{
  int a;
  struct SelectDialog *d;

  d = (struct SelectDialog *) data;
  a = list_store_get_int(d->list, iter, 0);
  arrayadd(d->sel, &a);
}

static void
SelectDialogClose(GtkWidget *w, void *data)
{
  struct SelectDialog *d;

  d = (struct SelectDialog *) data;
  if (d->ret == IDOK) {
    GtkTreeSelection *gsel;

    gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    gtk_tree_selection_selected_foreach(gsel, select_tree_selection_foreach_cb, data);
  } else if (d->ret == IDSALL) {
    int r, id;
    GtkTreeIter iter;

    r = list_store_get_iter_first(d->list, &iter);
    while (r) {
      id = list_store_get_int(d->list, &iter, 0);
      arrayadd(d->sel, &id);
      r = list_store_iter_next(d->list, &iter);
    }
    d->ret = IDOK;
  }
}

void
SelectDialog(struct SelectDialog *data,
	     struct objlist *obj,
	     char *(*callback) (struct objlist * obj, int id),
	     struct narray *array, struct narray *iarray)
{
  data->SetupWindow = SelectDialogSetup;
  data->CloseWindow = SelectDialogClose;
  data->Obj = obj;
  data->cb = callback;
  arrayinit(array, sizeof(int));
  data->sel = array;
  data->isel = iarray;
}

static gboolean
single_list_default_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct CopyDialog *d;
  int i;

  d = (struct CopyDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    i = list_store_get_selected_index(d->list);
    if (i < 0)
      return FALSE;

    gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);

    return TRUE;
  }

  return FALSE;
}

static void
CopyDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  char *s;
  struct CopyDialog *d;
  int i, a;
  GtkWidget *swin, *w;
  GtkTreeIter iter;
  n_list_store copy_list[] = {
    {"id",       G_TYPE_INT,    TRUE, FALSE, NULL, FALSE, 0, 0, 0, 0, PANGO_ELLIPSIZE_NONE, 0},
    {"property", G_TYPE_STRING, TRUE, FALSE, NULL, FALSE, 0, 0, 0, 0, PANGO_ELLIPSIZE_END, 0},
  };

  d = (struct CopyDialog *) data;
  if (makewidget) {
    d->list = list_store_create(sizeof(copy_list) / sizeof(*copy_list), copy_list);
    list_store_set_sort_all(d->list);
    list_store_set_selection_mode(d->list, GTK_SELECTION_SINGLE);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), d->list);

    g_signal_connect(d->list, "button-press-event", G_CALLBACK(single_list_default_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(single_list_default_cb), d);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->vbox), w, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }

  list_store_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    s = d->cb(d->Obj, i);
    if (s) {
      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, i);
      list_store_set_string(d->list, &iter, 1, CHK_STR(s));
      g_free(s);
    }
  }

  if (chkobjlastinst(d->Obj) == 0) {
    list_store_select_nth(d->list, 0);
  } else if (d->Id >= 0) {
    a = search_id(d->list, d->Id);
    if (a >= 0)
      list_store_select_nth(d->list, a);
  }
  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(XtNameToWidget(w, "*list"), XmNwidth, 200, NULL);
  }
  */
}

static void
CopyDialogClose(GtkWidget *w, void *data)
{
  struct CopyDialog *d;
  

  d = (struct CopyDialog *) data;

  if (d->ret == IDCANCEL)
    return;

  d->sel = list_store_get_selected_int(d->list, 0);
}

void
CopyDialog(struct CopyDialog *data,
	   struct objlist *obj, int id,
	   char *(*callback) (struct objlist * obj, int id))
{
  data->SetupWindow = CopyDialogSetup;
  data->CloseWindow = CopyDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->cb = callback;
  data->sel = id;
}

int
CopyClick(GtkWidget *parent, struct objlist *obj, int Id,
	  char *(*callback) (struct objlist * obj, int id))
{
  int sel;

  CopyDialog(&DlgCopy, obj, Id, callback);

  if (DialogExecute(parent, &DlgCopy) == IDOK) {
    sel = DlgCopy.sel;
  } else {
    sel = -1;
  }

  return sel;
}

int
SetObjPointsFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  double d;
  int ip;
  char *buf, *ptr, *tmp, *eptr;
  const char *ctmp;
  struct narray *array = NULL, *atmp;

  if (w == NULL)
    return 0;

  ctmp = gtk_entry_get_text(GTK_ENTRY(w));
  if (ctmp == NULL)
    return -1;

  buf = g_strdup(ctmp);
  if (buf == NULL)
    return -1;

  array = arraynew(sizeof(int));
  ptr = buf;
  while (1) {
    while (ptr && isspace(*ptr))
      ptr++;

    if (*ptr == '\0')
      break;

    tmp = strchr(ptr, ' ');
    if (tmp)
      *tmp = '\0';

    d = strtod(ptr, &eptr);
    if (d != d || d == HUGE_VAL || d == - HUGE_VAL || eptr[0] != '\0')
      goto ErrEnd;

    ip = nround(d * 100);
    atmp = arrayadd(array, &ip);
    if (atmp == NULL)
      goto ErrEnd;

    array = atmp;

    if (tmp == NULL)
      break;

    ptr = tmp + 1;
  }

  if (get_graph_modified()) {
    if (putobj(Obj, field, Id, array) < 0)
      goto ErrEnd;
  } else {
    char *str1, *str2;

    sgetobjfield(Obj, Id, field, NULL, &str1, FALSE, FALSE, FALSE);
    if (putobj(Obj, field, Id, array) < 0) {
      g_free(str1);
      goto ErrEnd;
    }
    sgetobjfield(Obj, Id, field, NULL, &str2, FALSE, FALSE, FALSE);
    if (str1 && str2 && strcmp(str1, str2)) {
      set_graph_modified();
    }
    g_free(str2);
    g_free(str1);
  }

  g_free(buf);
  return 0;


 ErrEnd:
  if (buf)
    g_free(buf);

  if (array)
    arrayfree(array);

  return -1;
}

void
SetTextFromObjPoints(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GtkEntry *entry;
  struct narray *array;
  char *str, buf[128], *tmp;
  int i, n, *points;

  if (w == NULL)
    return;

  str = nstrnew();
  if (str == NULL)
    return;

  entry = GTK_ENTRY(w);
  getobj(Obj, field, Id, 0, NULL, &array);
  n = arraynum(array);
  points = (int *) arraydata(array);
  for (i = 0; i < n; i++) {
    snprintf(buf, sizeof(buf), "%.2f ", points[i] / 100.0);
    tmp = nstrcat(str, buf);
    if (tmp == NULL)
      goto END;
    str = tmp;
  }

  gtk_entry_set_text(entry, str);

 END:
  g_free(str);
}

int
chk_sputobjfield(struct objlist *obj, int id, char *field, char *str)
{
  if (get_graph_modified()) {
    if (sputobjfield(obj, id, field, str)) {
      return 1;
    }
  } else {
    char *ptr, *org;

    sgetobjfield(obj, id, field, NULL, &org, FALSE, FALSE, FALSE);

    if (sputobjfield(obj, id, field, str)) {
      g_free(org);
      return 1;
    }

    sgetobjfield(obj, id, field, NULL, &ptr, FALSE, FALSE, FALSE);
    if ((ptr == NULL && org) ||
	(ptr && org == NULL) ||
	(ptr && org && strcmp(ptr, org))) {
      set_graph_modified();
    }
    g_free(ptr);
    g_free(org);
  }

  return 0;
}

int
SetObjFieldFromWidget(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int r = 0;

  if (w == NULL)
    return 0;

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_SPIN_BUTTON)) {
    r = SetObjFieldFromSpin(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_ENTRY)) {
    r = SetObjFieldFromText(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_COMBO_BOX_ENTRY)) {
    r = SetObjFieldFromText(GTK_BIN(w)->child, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_COMBO_BOX)) {
    r = SetObjFieldFromList(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_TOGGLE_BUTTON)) {
    r = SetObjFieldFromToggle(w, Obj, Id, field);
  }

  if (r) {
    gtk_widget_grab_focus(w);
  }

  return r;
}

void
SetWidgetFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  if (w == NULL)
    return;

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_SPIN_BUTTON)) {
    SetSpinFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_ENTRY)) {
    SetTextFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_COMBO_BOX_ENTRY)) {
    SetTextFromObjField(GTK_BIN(w)->child, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_COMBO_BOX)) {
    SetListFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_TOGGLE_BUTTON)) {
    SetToggleFromObjField(w, Obj, Id, field);
  }
}

int
SetObjFieldFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  const char *tmp;
  char *buf;

  if (w == NULL)
    return 0;

  tmp = gtk_entry_get_text(GTK_ENTRY(w));
  if (tmp == NULL)
    return -1;

  if (strcmp(field, "file") == 0) {
    buf = filename_from_utf8(tmp);
    if (buf == NULL) {
      MessageBox(NULL, _("Couldn't convert filename from UTF-8."), NULL, MB_OK);
      return -1;
    }
  } else {
    buf = g_strdup(tmp);
  }

  if (buf == NULL)
    return -1;

  if (chk_sputobjfield(Obj, Id, field, buf)) {
    g_free(buf);
    return -1;
  }

  g_free(buf);
  return 0;
}

void
SetTextFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GtkEntry *entry;
  char *buf;

  if (w == NULL)
    return;

  entry = GTK_ENTRY(w);

  sgetobjfield(Obj, Id, field, NULL, &buf, FALSE, FALSE, FALSE);
  
  if (buf == NULL) {
    gtk_entry_set_text(entry, "");
    return;
  }

  if (strcmp(field, "file") == 0) {
    char *ptr;

    ptr = filename_to_utf8(buf);
    if (ptr == NULL) {
      MessageBox(NULL, _("Couldn't convert filename to UTF-8."), NULL, MB_OK);
      return;
    }

    g_free(buf);
    buf = ptr;
  }

  gtk_entry_set_text(entry, CHK_STR(buf));
  g_free(buf);
}

int
SetObjFieldFromSpin(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int val, oval;

  if (w == NULL)
    return 0;

  val = spin_entry_get_val(w);

  if (get_graph_modified()) {
    if (putobj(Obj, field, Id, &val) < 0) {
      return -1;
    }
  } else {
    getobj(Obj, field, Id, 0, NULL, &oval);
    if (putobj(Obj, field, Id, &val) < 0) {
      return -1;
    }
    getobj(Obj, field, Id, 0, NULL, &val);
    if (val != oval) {
      set_graph_modified();
    }
  }

  return 0;
}

void
SetSpinFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int val;

  if (w == NULL)
    return;

  getobj(Obj, field, Id, 0, NULL, &val);
  spin_entry_set_val(w, val);
}

int
SetObjFieldFromToggle(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  gboolean state;
  int a, oa;

  if (w == NULL)
    return 0;

  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  a = state ? TRUE : FALSE;

  getobj(Obj, field, Id, 0, NULL, &oa);
  oa = oa ? TRUE : FALSE;

  if (a != oa) {
    if (putobj(Obj, field, Id, &a) == -1){
      return -1;
    }
    set_graph_modified();
  }

  return 0;
}

void
SetToggleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int a;

  if (w == NULL)
    return;

  getobj(Obj, field, Id, 0, NULL, &a);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), a);
}

int
SetObjFieldFromStyle(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  unsigned int j;
  const char *ptr;

  if (w == NULL)
    return 0;

  ptr = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(w)->child));

  if (ptr == NULL) {
    return -1;
  }

  for (j = 0; j < CLINESTYLE; j++) {
    if (strcmp(ptr, FwLineStyle[j].name) == 0 || 
	strcmp(ptr, _(FwLineStyle[j].name)) == 0) {
      if (chk_sputobjfield(Obj, Id, field, FwLineStyle[j].list) != 0) {
	return -1;
      }
      break;
    }
  }

  if (j == CLINESTYLE) {
    if (SetObjPointsFromText(GTK_BIN(w)->child, Obj, Id, field)) {
      return -1;
    }
  }

  return 0;
}

const char *
get_style_string(struct objlist *obj, int id, char *field)
{
  unsigned int j;
  int i;
  struct narray *array;
  int stylenum;
  int *style, a;
  char *s;

  getobj(obj, field, id, 0, NULL, &array);
  stylenum = arraynum(array);
  style = (int *) arraydata(array);
  for (j = 0; j < CLINESTYLE; j++) {
    if (stylenum == FwLineStyle[j].num) {
      s = FwLineStyle[j].list;
      for (i = 0; i < FwLineStyle[j].num; i++) {
	a = strtol(s, &s, 10);
	if (style[i] != a)
	  break;
      }
      if (i == FwLineStyle[j].num)
	return FwLineStyle[j].name;
    }
  }

  return NULL;
}

void
SetStyleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  unsigned int j;
  GtkEntry *entry;
  int count;
  const char *s;

  if (w == NULL)
    return;

  entry = GTK_ENTRY(GTK_BIN(w)->child);

  count = combo_box_get_num(w);
  if (count == 0) {
    for (j = 0; j < CLINESTYLE; j++) {
      combo_box_append_text(w, _(FwLineStyle[j].name));
    }
  }

  s = get_style_string(Obj, Id, field);
  if (s) {
    gtk_entry_set_text(entry, _(s));
  } else {
    SetTextFromObjPoints(GTK_WIDGET(entry), Obj, Id, field);
  }
}

int 
get_radio_index(GSList *top)
{
  int i, n;
  GtkToggleButton *btn;
  GSList *list;
  
  n = g_slist_length(top);
  for (i = 0, list = top; i < n; i++, list = list->next) {
    btn = GTK_TOGGLE_BUTTON(list->data);
    if (gtk_toggle_button_get_active(btn)) {
      return i;
    }
  }
  return -1;
}

int
SetObjFieldFromList(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int pos, opos;

  if (w == NULL)
    return 0;

  pos = combo_box_get_active(w);

  if (pos < 0) {
    return -1;
  }

  getobj(Obj, field, Id, 0, NULL, &opos);

  if (pos != opos) {
    if (putobj(Obj, field, Id, &pos) == -1) {
      return -1;
    }
    set_graph_modified();
  }

  return 0;
}

void
SetListFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  char **enumlist;
  int j, a, count;

  if (w == NULL)
    return;

  count = combo_box_get_num(w);
  if (count == 0) {
    enumlist = (char **) chkobjarglist(Obj, field);
    for (j = 0; enumlist[j]; j++) {
      combo_box_append_text(w, _(enumlist[j]));
    }
  }
  getobj(Obj, field, Id, 0, NULL, &a);
  combo_box_set_active(w, a);
}

void
SetComboList(GtkWidget *w, char **list, int num)
{
  int count;

  if (w == NULL)
    return;

  count = combo_box_get_num(w);
  if (count == 0) {
    SetComboList2(w, list, num);
  }
}

void
SetComboList2(GtkWidget *w, char **list, int num)
{
  int j;

  if (w == NULL)
    return;

  for (j = 0; j < num; j++) {
    combo_box_append_text(w, list[j]);
  }
}

void
SetFontListFromObj(GtkWidget *w, struct objlist *obj, int id, char *name, int jfont)
{
  int j, selfont;
  struct fontmap *fcur;
  char *font;

  if (w == NULL)
    return;

  getobj(obj, name, id, 0, NULL, &font);
  combo_box_clear(w);

  fcur = Gra2cairoConf->fontmap_list_root;
  j = 0;
  selfont = 0;
  while (fcur) {
    if ((jfont && fcur->twobyte) || (! jfont && !(fcur->twobyte))) {
      combo_box_append_text(w, fcur->fontalias);
      if (font && strcmp(font, fcur->fontalias) == 0)
	selfont = j;
      j++;
    }
    fcur = fcur->next;
  }

  combo_box_set_active(w, selfont);
}

void
SetObjFieldFromFontList(GtkWidget *w, struct objlist *obj, int id, char *name, int jfont)
{
  struct fontmap *fcur;
  char *fontalias;

  if (w == NULL)
    return;

  fontalias = combo_box_get_active_text(w);

  if (fontalias == NULL)
    return;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur)) {
    g_free(fontalias);
    return;
  }

  g_free(fontalias);

  if ((! jfont || ! fcur->twobyte) && (jfont || fcur->twobyte))
    return;

  chk_sputobjfield(obj, id, name, fcur->fontalias);
}

int
SetObjAxisFieldFromWidget(GtkWidget *w, struct objlist *obj, int id, char *field)
{
  const char *s;
  char *buf;
  int r;

  s = combo_box_entry_get_text(w);
  if (s == NULL)
    return 0;

  if (s[0] == '\0') {
    buf = NULL;
  } else {
    buf = g_strdup_printf("axis:%s", CHK_STR(s));
  }

  r = chk_sputobjfield(obj, id, field, buf);

  if (buf)
    g_free(buf);

  return r;
}

static void
_set_color(GtkWidget *w, struct objlist *obj, int id, char *prefix, char *postfix)
{
  GdkColor color;
  int r, g, b;
  char buf[64];

  snprintf(buf, sizeof(buf), "%sR%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &r);

  snprintf(buf, sizeof(buf), "%sG%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &g);

  snprintf(buf, sizeof(buf), "%sB%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &b);

  color.red = (r & 0xffU) * 257;
  color.green = (g & 0xffU) * 257;
  color.blue = (b & 0xffU) * 257;

  gtk_color_button_set_color(GTK_COLOR_BUTTON(w), &color);
}

void
set_color(GtkWidget *w, struct objlist *obj, int id, char *prefix)
{
  _set_color(w, obj, id, prefix, NULL);
}

void
set_color2(GtkWidget *w, struct objlist *obj, int id)
{
  _set_color(w, obj, id, NULL, "2");
}

static int
_putobj_color(GtkWidget *w, struct objlist *obj, int id, char *prefix, char *postfix)
{
  GdkColor color;
  int r, g, b, o;
  char buf[64];

  gtk_color_button_get_color(GTK_COLOR_BUTTON(w), &color);
  r = (color.red >> 8);
  g = (color.green >> 8);
  b = (color.blue >> 8);

  snprintf(buf, sizeof(buf), "%sR%s", CHK_STR(prefix), CHK_STR(postfix));

  getobj(obj, buf, id, 0, NULL, &o);
  if (o != r) {
    if (putobj(obj, buf, id, &r) == -1) {
      return TRUE;
    }
    set_graph_modified();
  }


  snprintf(buf, sizeof(buf), "%sG%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &o);
  if (o != g) {
    if (putobj(obj, buf, id, &g) == -1) {
      return TRUE;
    }
    set_graph_modified();
  }

  snprintf(buf, sizeof(buf), "%sB%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &o);
  if (o != b) {
    if (putobj(obj, buf, id, &b) == -1) {
      return TRUE;
    }
    set_graph_modified();
  }

  return FALSE;
}

int
putobj_color(GtkWidget *w, struct objlist *obj, int id, char *prefix)
{
  return _putobj_color(w, obj, id, prefix, NULL);
}

int
putobj_color2(GtkWidget *w, struct objlist *obj, int id)
{
  return _putobj_color(w, obj, id, NULL, "2");
}

