/*
 * $Id: x11dialg.c,v 1.53 2010-03-04 08:30:17 hito Exp $
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

#include "object.h"
#include "mathfn.h"
#include "nstring.h"

#include "ogra2cairo.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11dialg.h"

#include "gtk_combo.h"
#include "gtk_widget.h"
#include "gtk_columnview.h"

#define LINE_ELEMENT_2(e1, e2)			#e1 " " #e2, {e1, e2}, 2
#define LINE_ELEMENT_4(e1, e2, e3, e4)		#e1 " " #e2 " " #e3 " " #e4, {e1, e2, e3, e4}, 4
#define LINE_ELEMENT_6(e1, e2, e3, e4, e5, e6)	#e1 " " #e2 " " #e3 " " #e4 " " #e5 " " #e6, {e1, e2, e3, e4, e5, e6}, 6

struct line_style FwLineStyle[] = {
  {N_("solid"),      "", {0}, 0},
  {N_("dot"),        LINE_ELEMENT_2(100, 100)},
  {N_("short dash"), LINE_ELEMENT_2(150, 150)},
  {N_("dash"),       LINE_ELEMENT_2(450, 150)},
  {N_("dot dash"),   LINE_ELEMENT_4(450, 150, 150, 150)},
  {N_("2-dot dash"), LINE_ELEMENT_6(450, 150, 150, 150, 150, 150)},
  {N_("dot 2-dash"), LINE_ELEMENT_6(450, 150, 450, 150, 150, 150)},
  {NULL, NULL, {0}, -1},
};
#define CLINESTYLE ((sizeof(FwLineStyle) / sizeof(*FwLineStyle)) - 1)

char *FwNumStyle[] =
  {N_("auto"), "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
int FwNumStyleNum = sizeof(FwNumStyle) / sizeof(*FwNumStyle);

struct FileDialog DlgFile;
struct FileDialog DlgRange;
struct FileDialog DlgArray;
struct FileDialog DlgFileDef;
struct EvalDialog DlgEval;
struct MathDialog DlgMath;
struct MathTextDialog DlgMathText;
struct FitDialog DlgFit;
struct FitSaveDialog DlgFitSave;
struct SectionDialog DlgSection;
struct CrossDialog DlgCross;
struct AxisDialog DlgAxis;
struct GridDialog DlgGrid;
struct GridDialog DlgGridDef;
struct ZoomDialog DlgZoom;
struct MergeDialog DlgMerge;
struct ParameterDialog DlgParameter;
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
struct SaveDialog DlgSave;
struct OutputDataDialog DlgOutputData;
struct DefaultDialog DlgDefault;
struct SetScriptDialog DlgSetScript;
struct PrefScriptDialog DlgPrefScript;
struct PrefFontDialog DlgPrefFont;
struct FontSettingDialog DlgFontSetting;
struct MiscDialog DlgMisc;
struct ExViewerDialog DlgExViewer;
struct ViewerDialog DlgViewer;
struct SelectDialog DlgSelect;
struct CopyDialog DlgCopy;
struct OutputImageDialog DlgImageOut;

static void SetTextFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static int SetObjFieldFromSpin(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static void SetSpinFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static int SetObjFieldFromToggle(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static void SetToggleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static int SetObjFieldFromList(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static void SetListFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
static int SetObjFieldFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field);

void
initdialog(void)
{
  DlgFile.widget = NULL;
  DlgFile.response_cb = NULL;
  DlgFile.focus = NULL;
  DlgFile.resource = N_("file");
  DlgRange.widget = NULL;
  DlgRange.response_cb = NULL;
  DlgRange.focus = NULL;
  DlgRange.resource = N_("function");
  DlgArray.widget = NULL;
  DlgArray.response_cb = NULL;
  DlgArray.focus = NULL;
  DlgArray.resource = N_("array");
  DlgFileDef.widget = NULL;
  DlgFileDef.response_cb = NULL;
  DlgFileDef.focus = NULL;
  DlgFileDef.resource = N_("data default");
  DlgEval.widget = NULL;
  DlgEval.response_cb = NULL;
  DlgEval.focus = NULL;
  DlgEval.resource = N_("evaluation");
  DlgMath.widget = NULL;
  DlgMath.response_cb = NULL;
  DlgMath.focus = NULL;
  DlgMath.resource = N_("math");
  DlgMathText.widget = NULL;
  DlgMathText.response_cb = NULL;
  DlgMathText.focus = NULL;
  DlgMathText.resource = N_("math text");
  DlgFit.widget = NULL;
  DlgFit.response_cb = NULL;
  DlgFit.focus = NULL;
  DlgFit.resource = N_("fit");
  DlgFitSave.widget = NULL;
  DlgFitSave.response_cb = NULL;
  DlgFitSave.focus = NULL;
  DlgFitSave.Profile = NULL;
  DlgFitSave.resource = N_("fit save");
  DlgSection.widget = NULL;
  DlgSection.response_cb = NULL;
  DlgSection.focus = NULL;
  DlgSection.resource = N_("Frame/Section Graph");
  DlgCross.widget = NULL;
  DlgCross.response_cb = NULL;
  DlgCross.focus = NULL;
  DlgCross.resource = N_("Cross Graph");
  DlgAxis.widget = NULL;
  DlgAxis.response_cb = NULL;
  DlgAxis.focus = NULL;
  DlgAxis.resource = N_("axis");
  DlgGrid.widget = NULL;
  DlgGrid.response_cb = NULL;
  DlgGrid.focus = NULL;
  DlgGrid.resource = N_("grid");
  DlgGridDef.widget = NULL;
  DlgGridDef.response_cb = NULL;
  DlgGridDef.focus = NULL;
  DlgGridDef.resource = N_("grid default");
  DlgZoom.widget = NULL;
  DlgZoom.response_cb = NULL;
  DlgZoom.focus = NULL;
  DlgZoom.resource = N_("Scale Zoom");
  DlgZoom.zoom = 20000;
  DlgMerge.widget = NULL;
  DlgMerge.response_cb = NULL;
  DlgMerge.focus = NULL;
  DlgMerge.resource = N_("merge");
  DlgParameter.widget = NULL;
  DlgParameter.response_cb = NULL;
  DlgParameter.focus = NULL;
  DlgParameter.resource = N_("parameter");
  DlgLegendArrow.widget = NULL;
  DlgLegendArrow.response_cb = NULL;
  DlgLegendArrow.focus = NULL;
  DlgLegendArrow.arrow_pixmap = NULL;
  DlgLegendArrow.resource = N_("legend line");
  DlgLegendRect.widget = NULL;
  DlgLegendRect.response_cb = NULL;
  DlgLegendRect.focus = NULL;
  DlgLegendRect.resource = N_("legend rectangle");
  DlgLegendArc.widget = NULL;
  DlgLegendArc.response_cb = NULL;
  DlgLegendArc.focus = NULL;
  DlgLegendArc.resource = N_("legend arc");
  DlgLegendMark.widget = NULL;
  DlgLegendMark.response_cb = NULL;
  DlgLegendMark.focus = NULL;
  DlgLegendMark.resource = N_("legend mark");
  DlgLegendText.widget = NULL;
  DlgLegendText.response_cb = NULL;
  DlgLegendText.focus = NULL;
  DlgLegendText.resource = N_("legend text");
  DlgLegendTextDef.widget = NULL;
  DlgLegendTextDef.response_cb = NULL;
  DlgLegendTextDef.focus = NULL;
  DlgLegendTextDef.resource = N_("text default");
  DlgLegendGauss.widget = NULL;
  DlgLegendGauss.response_cb = NULL;
  DlgLegendGauss.focus = NULL;
  DlgLegendGauss.resource = N_("legend gauss");
  DlgPage.widget = NULL;
  DlgPage.response_cb = NULL;
  DlgPage.focus = NULL;
  DlgPage.resource = N_("page");
  DlgSwitch.widget = NULL;
  DlgSwitch.response_cb = NULL;
  DlgSwitch.focus = NULL;
  DlgSwitch.resource = N_("drawobj");
  DlgDirectory.widget = NULL;
  DlgDirectory.response_cb = NULL;
  DlgDirectory.focus = NULL;
  DlgDirectory.resource = N_("directory");
  DlgLoad.widget = NULL;
  DlgLoad.response_cb = NULL;
  DlgLoad.focus = NULL;
  DlgLoad.exdir = NULL;
  DlgLoad.resource = N_("load");
  DlgSave.widget = NULL;
  DlgSave.response_cb = NULL;
  DlgSave.focus = NULL;
  DlgSave.resource = N_("save");
  DlgOutputData.widget = NULL;
  DlgOutputData.response_cb = NULL;
  DlgOutputData.focus = NULL;
  DlgOutputData.resource = N_("output data");
  DlgDefault.widget = NULL;
  DlgDefault.response_cb = NULL;
  DlgDefault.focus = NULL;
  DlgDefault.resource = N_("save default");
  DlgSetScript.widget = NULL;
  DlgSetScript.response_cb = NULL;
  DlgSetScript.focus = NULL;
  DlgSetScript.resource = N_("Add-in script");
  DlgPrefScript.widget = NULL;
  DlgPrefScript.response_cb = NULL;
  DlgPrefScript.focus = NULL;
  DlgPrefScript.resource = N_("Add-in script setup");
  DlgPrefFont.widget = NULL;
  DlgPrefFont.response_cb = NULL;
  DlgPrefFont.focus = NULL;
  DlgPrefFont.resource = N_("Font");
  DlgMisc.widget = NULL;
  DlgMisc.response_cb = NULL;
  DlgMisc.focus = NULL;
  DlgMisc.resource = N_("Miscellaneous");
  DlgExViewer.widget = NULL;
  DlgExViewer.response_cb = NULL;
  DlgExViewer.focus = NULL;
  DlgExViewer.resource = N_("External viewer");
  DlgViewer.widget = NULL;
  DlgViewer.response_cb = NULL;
  DlgViewer.focus = NULL;
  DlgViewer.resource = N_("Viewer");
  DlgSelect.widget = NULL;
  DlgSelect.response_cb = NULL;
  DlgSelect.focus = NULL;
  DlgSelect.resource = N_("multi select");
  DlgCopy.widget = NULL;
  DlgCopy.response_cb = NULL;
  DlgCopy.focus = NULL;
  DlgCopy.resource = N_("single select");
  DlgImageOut.widget = NULL;
  DlgImageOut.response_cb = NULL;
  DlgImageOut.focus = NULL;
  DlgImageOut.resource = N_("output image");
}

struct response_callback *
response_callback_new(response_callback_func func, response_callback_free_func free, gpointer data)
{
  struct response_callback *cb;
  cb = g_malloc0(sizeof(* cb));
  if (cb == NULL) {
    return NULL;
  }
  cb->cb = func;
  cb->free = free;
  cb->data = data;
  return cb;
}

void
response_callback_add(void *dialog, response_callback_func cb, response_callback_free_func free_cb, gpointer data)
{
  struct DialogType *d;

  d = (struct DialogType *) dialog;
  d->response_cb = response_callback_new(cb, free_cb, data);
}

static void
multi_list_default_cb(GtkWidget *view, guint pos, gpointer user_data)
{
  struct SelectDialog *d;
  d = (struct SelectDialog *) user_data;
  gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
}

static int
search_id(GtkWidget *columnview, int id)
{
  GListModel *list;
  int n, i;
  NInst *ni;

  list = G_LIST_MODEL (gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview)));
  if (list == NULL) {
    return -1;
  }

  n = g_list_model_get_n_items (list);
  for (i = 0; i < n; i++) {
    int target_id;
    ni = g_list_model_get_item (list, i);
    target_id = ni->id;
    g_object_unref (ni);
    if (target_id == id) {
      return i;
    }
  }

  return -1;
}

static void
setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), GPOINTER_TO_INT (user_data) ? 0.0 : 1.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_label_set_single_line_mode (GTK_LABEL (label), TRUE);
  gtk_list_item_set_child (list_item, label);
}

static void
bind_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_list_item_get_child (list_item);
  NInst *item = N_INST(gtk_list_item_get_item (list_item));

  if (GPOINTER_TO_INT (user_data)) {
    gtk_label_set_text(GTK_LABEL(label), item->name);
  } else {
    char text[20];

    snprintf(text, sizeof(text), "%d", item->id);
    gtk_label_set_text(GTK_LABEL(label), text);
  }
}

static char *
sort_column (NInst *item, gpointer user_data)
{
  return g_strdup (item->name);
}

static int
sort_by_id (NInst *item, gpointer user_data)
{
  return item->id;
}

static void
SelectDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct SelectDialog *d;
  int i, *seldata, selnum;

  d = (struct SelectDialog *) data;

  if (makewidget) {
    GtkWidget *swin, *w, *hbox;
    GtkColumnViewColumn *col;
    d->list = columnview_create(N_TYPE_INST, N_SELECTION_TYPE_MULTI);
    col = columnview_create_column(d->list, "id", G_CALLBACK(setup_column), G_CALLBACK(bind_column), NULL, GINT_TO_POINTER (0), FALSE);
    columnview_set_numeric_sorter(col, G_TYPE_INT, G_CALLBACK(sort_by_id), NULL);
    columnview_create_column(d->list, _("property"), G_CALLBACK(setup_column), G_CALLBACK(bind_column), G_CALLBACK(sort_column), GINT_TO_POINTER (1), TRUE);

    g_signal_connect(d->list, "activate", G_CALLBACK(multi_list_default_cb), d);

    swin = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), d->list);

    dialog_add_all_button((struct DialogType *) d);

    w = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(w), swin);
    gtk_box_append(GTK_BOX(d->vbox), w);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    w = gtk_button_new_with_mnemonic(_("Select _All"));
    g_signal_connect_swapped(w, "clicked", G_CALLBACK(columnview_select_all), d->list);
    gtk_box_append(GTK_BOX(hbox), w);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }
  gtk_window_set_title(GTK_WINDOW(d->widget), (d->title) ? d->title : d->resource);
  columnview_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    char *s;
    s = d->cb(d->Obj, i);
    if (s) {
      columnview_append_n_inst(d->list, s, i, d->Obj);
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
    columnview_set_active (d->list, 0, TRUE);
  } else if (d->isel) {
    seldata = arraydata(d->isel);
    selnum = arraynum(d->isel);

    columnview_unselect_all(d->list);
    for (i = 0; i < selnum; i++) {
      int a;
      a = search_id(d->list, seldata[i]);
      if (a >= 0) {
	columnview_select(d->list, a);
      }
    }
  }
}

static void
SelectDialogClose(GtkWidget *w, void *data)
{
  struct SelectDialog *d;
  GListModel *list;
  NInst *ni;
  int n, i;

  d = (struct SelectDialog *) data;
  list = G_LIST_MODEL(gtk_column_view_get_model (GTK_COLUMN_VIEW (d->list)));
  if (list == NULL) {
    d->ret = IDOK;
    return;
  }
  n = g_list_model_get_n_items (G_LIST_MODEL (list));

  if (d->ret == IDOK) {

    for (i = 0; i < n; i++) {
      if (gtk_selection_model_is_selected (GTK_SELECTION_MODEL (list), i)) {
	ni = g_list_model_get_item (list, i);
	arrayadd(d->sel, &ni->id);
	g_object_unref (ni);
      }
    }
  } else if (d->ret == IDSALL) {
    for (i = 0; i < n; i++) {
      ni = g_list_model_get_item (G_LIST_MODEL (list), i);
      arrayadd(d->sel, &ni->id);
      g_object_unref (ni);
    }
    d->ret = IDOK;
  }
}

void
SelectDialog(struct SelectDialog *data,
	     struct objlist *obj,
	     const char *title,
	     char *(*callback) (struct objlist * obj, int id),
	     struct narray *array, struct narray *iarray)
{
  data->SetupWindow = SelectDialogSetup;
  data->CloseWindow = SelectDialogClose;
  data->Obj = obj;
  data->cb = callback;
  data->sel = array;
  data->isel = iarray;
  data->title = title;
}

static void
single_list_default_cb(GtkColumnView *view, guint pos, gpointer user_data)
{
  struct CopyDialog *d;

  d = (struct CopyDialog *) user_data;
  gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
}

static void
CopyDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct CopyDialog *d;
  int i;

  d = (struct CopyDialog *) data;
  if (makewidget) {
    GtkWidget *swin, *w;
    GtkColumnViewColumn *col;
    d->list = columnview_create(N_TYPE_INST, N_SELECTION_TYPE_SINGLE);
    col = columnview_create_column(d->list, "id", G_CALLBACK(setup_column), G_CALLBACK(bind_column), G_CALLBACK(sort_column), GINT_TO_POINTER (0), FALSE);
    columnview_set_numeric_sorter(col, G_TYPE_INT, G_CALLBACK(sort_by_id), NULL);
    columnview_create_column(d->list, _("property"), G_CALLBACK(setup_column), G_CALLBACK(bind_column), G_CALLBACK(sort_column), GINT_TO_POINTER (1), TRUE);

    swin = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), d->list);

    g_signal_connect(d->list, "activate", G_CALLBACK(single_list_default_cb), d);

    w = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(w), swin);
    gtk_box_append(GTK_BOX(d->vbox), w);
    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }
  gtk_window_set_title(GTK_WINDOW(d->widget), (d->title) ? d->title : d->resource);
  columnview_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    char *s;
    s = d->cb(d->Obj, i);
    if (s) {
      columnview_append_n_inst(d->list, CHK_STR(s), i, d->Obj);
      g_free(s);
    }
  }

  if (chkobjlastinst(d->Obj) == 0) {
    columnview_set_active (d->list, 0, TRUE);
  } else if (d->Id >= 0) {
    int a;
    a = search_id(d->list, d->Id);
    if (a >= 0) {
      columnview_set_active (d->list, a, TRUE);
    }
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
  NInst *inst;


  d = (struct CopyDialog *) data;

  if (d->ret == IDCANCEL) {
    return;
  }

  inst = N_INST (columnview_get_active_item(d->list));
  d->sel = inst->id;
}

void
CopyDialog(struct CopyDialog *data,
	   struct objlist *obj, int id,
	   const char *title,
	   char *(*callback) (struct objlist * obj, int id))
{
  data->SetupWindow = CopyDialogSetup;
  data->CloseWindow = CopyDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->cb = callback;
  data->sel = id;
  data->title = title;
  data->rcb = NULL;
}

static void
copy_click_response(struct response_callback *cb)
{
  int sel;

  if (cb->return_value == IDOK) {
    sel = DlgCopy.sel;
  } else {
    sel = -1;
  }
  DlgCopy.rcb(sel, cb->data);
}
void
CopyClick(GtkWidget *parent, struct objlist *obj, int Id,
	  char *(*callback) (struct objlist * obj, int id),
          response_cb response_cb, gpointer user_data)
{
  CopyDialog(&DlgCopy, obj, Id, _("copy property (single select)"), callback);
  DlgCopy.rcb = response_cb;
  response_callback_add(&DlgCopy, copy_click_response, NULL, user_data);
  DialogExecute(parent, &DlgCopy);
}

int
SetObjPointsFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GListModel *list;
  struct narray *array;
  int  i, n;

  if (w == NULL) {
    return 0;
  }

  list = G_LIST_MODEL (columnview_get_list (w));
  array = arraynew(sizeof(int));
  if (array == NULL) {
    return -1;
  }

  n = g_list_model_get_n_items (list);
  for (i = 0; i < n; i++) {
    NPoint *point;
    point = g_list_model_get_item (list, i);
    arrayadd(array, &point->x);
    arrayadd(array, &point->y);
    g_object_unref (point);
  }

  if (get_graph_modified()) {
    if (putobj(Obj, field, Id, array) < 0) {
      goto ErrEnd;
    }
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

  return 0;


 ErrEnd:

  if (array) {
    arrayfree(array);
  }
  gtk_widget_grab_focus(w);

  return -1;
}

void
SetTextFromObjPoints(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GListStore *list;
  struct narray *array;
  int i, n, *points;

  if (w == NULL) {
    return;
  }

  list = columnview_get_list (w);
  g_list_store_remove_all (list);

  getobj(Obj, field, Id, 0, NULL, &array);
  n = arraynum(array);
  points = arraydata(array);
  for (i = 0; i < n / 2; i++) {
    list_store_append_n_point (list, points[i * 2], points[i * 2 + 1]);
  }
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

  if (w == NULL) {
    return 0;
  }

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_SPIN_BUTTON)) {
    r = SetObjFieldFromSpin(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_ENTRY)) {
    r = SetObjFieldFromText(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_DROP_DOWN)) {
    r = SetObjFieldFromList(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_TOGGLE_BUTTON)) {
    r = SetObjFieldFromToggle(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_CHECK_BUTTON)) {
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
  if (w == NULL) {
    return;
  }

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_SPIN_BUTTON)) {
    SetSpinFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_ENTRY)) {
    SetTextFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_DROP_DOWN)) {
    SetListFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_TOGGLE_BUTTON)) {
    SetToggleFromObjField(w, Obj, Id, field);
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_CHECK_BUTTON)) {
    SetToggleFromObjField(w, Obj, Id, field);
  }
}

static int
SetObjFieldFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  const char *tmp;
  char *buf;

  if (w == NULL) {
    return 0;
  }

  tmp = gtk_editable_get_text(GTK_EDITABLE(w));
  if (tmp == NULL) {
    return -1;
  }

  buf = g_strdup(tmp);
  if (buf == NULL) {
    return -1;
  }

  if (chk_sputobjfield(Obj, Id, field, buf)) {
    g_free(buf);
    return -1;
  }

  g_free(buf);
  return 0;
}

static void
SetTextFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GtkEntry *entry;
  char *buf;

  if (w == NULL) {
    return;
  }

  entry = GTK_ENTRY(w);

  sgetobjfield(Obj, Id, field, NULL, &buf, FALSE, FALSE, FALSE);

  if (buf == NULL) {
    editable_set_init_text(GTK_WIDGET(entry), "");
    return;
  }

  editable_set_init_text(GTK_WIDGET(entry), CHK_STR(buf));
  g_free(buf);
}

static int
SetObjFieldFromSpin(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int val, oval;

  if (w == NULL) {
    return 0;
  }

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

static void
SetSpinFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int val;

  if (w == NULL){
    return;
  }

  getobj(Obj, field, Id, 0, NULL, &val);
  spin_entry_set_val(w, val);
}

static int
SetObjFieldFromToggle(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  gboolean state;
  int a, oa;

  if (w == NULL) {
    return 0;
  }

  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_CHECK_BUTTON)) {
    state = gtk_check_button_get_active(GTK_CHECK_BUTTON(w));
  } else {
    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  }
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

static void
SetToggleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int a;

  if (w == NULL) {
    return;
  }

  getobj(Obj, field, Id, 0, NULL, &a);
  if (G_TYPE_CHECK_INSTANCE_TYPE(w, GTK_TYPE_CHECK_BUTTON)) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w), a);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), a);
  }
}

static int
set_obj_points_from_text(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  double d;
  int ip;
  char *buf, *ptr, *tmp, *eptr;
  const char *ctmp;
  struct narray *array = NULL, *atmp;

  if (w == NULL) {
    return 0;
  }

  ctmp = gtk_editable_get_text(GTK_EDITABLE(w));
  if (ctmp == NULL) {
    return -1;
  }

  buf = g_strdup(ctmp);
  if (buf == NULL) {
    return -1;
  }

  array = arraynew(sizeof(int));
  ptr = buf;
  while (1) {
    while (*ptr && isspace(*ptr))
      ptr++;

    if (*ptr == '\0') {
      break;
    }

    tmp = strchr(ptr, ' ');
    if (tmp) {
      *tmp = '\0';
    }

    d = strtod(ptr, &eptr);
    if (d != d || d == HUGE_VAL || d == - HUGE_VAL || eptr[0] != '\0')
      goto ErrEnd;

    ip = nround(d * 100);
    atmp = arrayadd(array, &ip);
    if (atmp == NULL) {
      goto ErrEnd;
    }

    array = atmp;

    if (tmp == NULL) {
      break;
    }

    ptr = tmp + 1;
  }

  if (get_graph_modified()) {
    if (putobj(Obj, field, Id, array) < 0) {
      goto ErrEnd;
    }
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
  if (buf) {
    g_free(buf);
  }

  if (array) {
    arrayfree(array);
  }

  return -1;
}

int
SetObjFieldFromStyle(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  unsigned int j;
  const char *ptr;

  if (w == NULL) {
    return 0;
  }

  ptr = gtk_editable_get_text(GTK_EDITABLE(w));
  if (ptr == NULL) {
    return -1;
  }

  for (j = 0; j < CLINESTYLE; j++) {
    if (strcmp(ptr, FwLineStyle[j].name) == 0 ||
	strcmp(ptr, _(FwLineStyle[j].name)) == 0) {
      if (chk_sputobjfield(Obj, Id, field, FwLineStyle[j].list) != 0) {
	gtk_widget_grab_focus(w);
	return -1;
      }
      break;
    }
  }

  if (j == CLINESTYLE) {
    if (set_obj_points_from_text(w, Obj, Id, field)) {
      gtk_widget_grab_focus(w);
      return -1;
    }
  }

  return 0;
}

int
get_style_index(struct objlist *obj, int id, char *field)
{
  unsigned int j;
  int i;
  struct narray *array;
  int stylenum;
  int *style, a;
  char *s;

  getobj(obj, field, id, 0, NULL, &array);
  stylenum = arraynum(array);
  style = arraydata(array);
  for (j = 0; j < CLINESTYLE; j++) {
    if (stylenum == FwLineStyle[j].num) {
      s = FwLineStyle[j].list;
      for (i = 0; i < FwLineStyle[j].num; i++) {
	a = strtol(s, &s, 10);
	if (style[i] != a) {
	  break;
	}
      }
      if (i == FwLineStyle[j].num) {
	return j;
      }
    }
  }

  return -1;
}

const char *
get_style_string(struct objlist *obj, int id, char *field)
{
  int i;

  i = get_style_index(obj, id, field);
  if (i < 0) {
    return NULL;
  }
  return FwLineStyle[i].name;
}

static void
set_entry_from_obj_point(GtkEntry *entry, struct objlist *Obj, int Id, char *field)
{
  struct narray *array;
  char buf[128];
  int i, n, *points, pos;
  GtkEntryBuffer *t_buf;

  t_buf = gtk_entry_get_buffer(entry);
  gtk_entry_buffer_set_text(t_buf, "", 0);

  getobj(Obj, field, Id, 0, NULL, &array);
  n = arraynum(array);
  points = arraydata(array);
  pos = 0;
  for (i = 0; i < n / 2; i++) {
    int l;
    l = snprintf(buf, sizeof(buf), "%.2f %.2f ", points[i * 2] / 100.0, points[i * 2 + 1] / 100.0);
    gtk_entry_buffer_insert_text(t_buf, pos, buf, l);
    pos += l;
  }
}

void
SetStyleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  GtkEntry *entry;
  int count;
  const char *s;

  if (w == NULL) {
    return;
  }

  entry = GTK_ENTRY(w);

  count = combo_box_get_num(w);
  if (count == 0) {
    unsigned int j;
    for (j = 0; j < CLINESTYLE; j++) {
      combo_box_append_text(w, _(FwLineStyle[j].name));
    }
  }

  s = get_style_string(Obj, Id, field);
  if (s) {
    editable_set_init_text(GTK_WIDGET(entry), _(s));
  } else {
    set_entry_from_obj_point(entry, Obj, Id, field);
  }
}

static int
SetObjFieldFromList(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int pos, opos;

  if (w == NULL) {
    return 0;
  }

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

static void
SetListFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int count, a;

  if (w == NULL) {
    return;
  }

  count = combo_box_get_num(w);
  if (count == 0) {
    char **enumlist;
    int j;
    enumlist = (char **) chkobjarglist(Obj, field);
    for (j = 0; enumlist[j] && enumlist[j][0]; j++) {
      combo_box_append_text(w, _(enumlist[j]));
    }
  }
  getobj(Obj, field, Id, 0, NULL, &a);
  combo_box_set_active(w, a);
}

struct compatible_font_info *
SetFontListFromObj(GtkWidget *w, struct objlist *obj, int id, const char *name)
{
  int j, selfont;
  struct fontmap *fcur;
  char *font;
  struct compatible_font_info *compatible;

  if (w == NULL) {
    return NULL;
  }

  compatible = NULL;

  getobj(obj, name, id, 0, NULL, &font);
  combo_box_clear(w);

  fcur = Gra2cairoConf->fontmap_list_root;
  j = 0;
  selfont = -1;
  while (fcur) {
    combo_box_append_text(w, fcur->fontalias);
    if (font && strcmp(font, fcur->fontalias) == 0) {
      selfont = j;
    }
    j++;
    fcur = fcur->next;
  }

  if (selfont < 0) {
    selfont = 0;
    compatible = gra2cairo_get_compatible_font_info(font);
    if (compatible) {
      fcur = Gra2cairoConf->fontmap_list_root;
      j = 0;
      while (fcur) {
	if (strcmp(compatible->name, fcur->fontalias) == 0) {
	  selfont = j;
	  break;
	}
	j++;
	fcur = fcur->next;
      }
    }
  }

  combo_box_set_active(w, selfont);

  return compatible;
}

void
SetObjFieldFromFontList(GtkWidget *w, struct objlist *obj, int id, char *name)
{
  struct fontmap *fcur;
  char *fontalias;

  if (w == NULL) {
    return;
  }

  fontalias = combo_box_get_active_text(w);
  if (fontalias == NULL) {
    return;
  }

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur)) {
    g_free(fontalias);
    return;
  }

  g_free(fontalias);

  chk_sputobjfield(obj, id, name, fcur->fontalias);
}

#define AXIS_COMBO_BOX_ID_LIST_KEY "AXIS_COMBO_BOX_ID"
#define AXIS_COMBO_BOX_FLAGS_KEY "AXIS_COMBO_BOX_FLAGS"
enum AXIS_COMBO_BOX_COLUMN {
  AXIS_COMBO_BOX_COLUMN_TITLE       = 0,
  AXIS_COMBO_BOX_COLUMN_ID          = 1,
  AXIS_COMBO_BOX_COLUMN_OID         = 2,
  AXIS_COMBO_BOX_COLUMN_SENSITIVITY = 3,
  AXIS_COMBO_BOX_COLUMN_NUM         = 4
};

static void
axis_combo_box_deleted(GtkWidget *cbox, gpointer user_data)
{
  struct narray *array;

  array = (struct narray *) user_data;
  arrayfree(array);
}

GtkWidget *
axis_combo_box_create(int flags)
{
  GtkWidget *cbox;
  struct narray *array;

  array = arraynew(sizeof(int));
  if (array == NULL) {
    return NULL;
  }
  cbox = combo_box_create();
  g_object_set_data(G_OBJECT(cbox), AXIS_COMBO_BOX_FLAGS_KEY, GINT_TO_POINTER(flags));
  g_object_set_data(G_OBJECT(cbox), AXIS_COMBO_BOX_ID_LIST_KEY, array);
  g_signal_connect(cbox, "destroy", G_CALLBACK(axis_combo_box_deleted), array);
  return cbox;
}

static void
axis_combo_box_clear(GtkWidget *cbox)
{
  combo_box_clear(cbox);
}

static int
axis_combo_box_get_flags(GtkWidget *cbox)
{
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cbox), AXIS_COMBO_BOX_FLAGS_KEY));
}

static struct narray *
axis_combo_box_get_id_array(GtkWidget *cbox)
{
  return g_object_get_data(G_OBJECT(cbox), AXIS_COMBO_BOX_ID_LIST_KEY);
}

struct axis_combo_box_each_data {
  GtkWidget *combo;
  int id;
};

static int
axis_combo_box_get_id(GtkWidget *cbox, struct objlist *obj, int id, const char *field)
{
  char *valstr;
  struct narray axis_array;
  struct objlist *aobj;
  int aid;

  combo_box_set_active(cbox, 0);
  sgetobjfield(obj, id, field, NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr == NULL) {
    return -1;
  }
  g_strstrip(valstr);
  if (! g_ascii_isalpha(valstr[0])) {
    g_free(valstr);
    return -1;
  }
  arrayinit(&axis_array, sizeof(int));
  if (getobjilist(valstr, &aobj, &axis_array, FALSE, NULL)) {
    g_free(valstr);
    return -1;
  }

  aid = arraylast_int(&axis_array);
  arraydel(&axis_array);
  g_free(valstr);
  return aid;
}

void
axis_combo_box_setup(GtkWidget *cbox, struct objlist *obj, int id, const char *field)
{
  struct objlist *aobj;
  const char *name;
  int default_aid, aid, lastinst, flag, self, selected, row;
  struct narray *array;

  if (cbox == NULL) {
    return;
  }

  array = axis_combo_box_get_id_array(cbox);
  if (array == NULL) {
    return;
  }

  arrayclear(array);
  axis_combo_box_clear(cbox);

  aobj = getobject("axis");
  flag = axis_combo_box_get_flags(cbox);
  lastinst = chkobjlastinst(aobj);
  row = 0;
   if (flag & AXIS_COMBO_BOX_ADD_NONE) {
    combo_box_append_text(cbox, _("none"));
    aid = -1;
    row++;
    arrayadd(array, &aid);
  }
  self = (aobj == obj) ? id : -1;
  default_aid = axis_combo_box_get_id(cbox, obj, id, field);
  selected = 0;
  for (aid = 0; aid <= lastinst; aid++) {
    getobj(aobj, "group", aid, 0, NULL, &name);
    if (aid == self) {
      continue;
    }
    if (flag & AXIS_COMBO_BOX_USE_OID) {
      int aoid;
      getobj(aobj, "oid", aid, 0, NULL, &aoid);
      arrayadd(array, &aoid);
    } else {
      arrayadd(array, &aid);
    }
    combo_box_append_text(cbox, name);
    if (aid == default_aid) {
      selected = row;
    }
    row++;
  }
  combo_box_set_active(cbox, selected);
}

int
SetObjAxisFieldFromWidget(GtkWidget *w, struct objlist *obj, int id, char *field)
{
  int r, aid, selected;
  struct narray *array;
  char *item;

  if (w == NULL) {
    return 0;
  }

  array = axis_combo_box_get_id_array(w);
  if (array == NULL) {
    return 0;
  }

  selected = combo_box_get_active(w);
  if (selected < 0) {
    return 0;
  }

  aid = arraynget_int(array, selected);
  if (aid < 0) {
    item = NULL;
  } else {
    int flag;
    flag = axis_combo_box_get_flags(w);
    if (flag & AXIS_COMBO_BOX_USE_OID) {
      item = g_strdup_printf("axis:^%d", aid);
    } else {
      item = g_strdup_printf("axis:%d", aid);
    }
  }
  r = chk_sputobjfield(obj, id, field, item);

  g_free(item);

  return r;
}

static void
_set_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, char *prefix, char *postfix)
{
  GdkRGBA color;
  int r, g, b, a;
  char buf[64];

  snprintf(buf, sizeof(buf), "%sR%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &r);

  snprintf(buf, sizeof(buf), "%sG%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &g);

  snprintf(buf, sizeof(buf), "%sB%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &b);

  snprintf(buf, sizeof(buf), "%sA%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &a);

  color.red = r / 255.0;
  color.green = g / 255.0;
  color.blue = b / 255.0;
  color.alpha = 1.0;

  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(w), &color);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (aw), a);

  snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
  gtk_widget_set_tooltip_text(w, buf);
}

void
set_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, char *prefix)
{
  _set_color(w, aw, obj, id, prefix, NULL);
}

void
set_color2(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  _set_color(w, aw, obj, id, NULL, "2");
}

void
set_fill_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  _set_color(w, aw, obj, id, "fill_", NULL);
}

void
set_stroke_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  _set_color(w, aw, obj, id, "stroke_", NULL);
}

static int
_putobj_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, char *prefix, char *postfix)
{
  GdkRGBA color;
  int r, g, b, a, o;
  char buf[64];

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &color);
  r = color.red * 255;
  g = color.green * 255;
  b = color.blue * 255;
  a = gtk_spin_button_get_value (GTK_SPIN_BUTTON (aw));

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

  snprintf(buf, sizeof(buf), "%sA%s", CHK_STR(prefix), CHK_STR(postfix));
  getobj(obj, buf, id, 0, NULL, &o);
  if (o != a) {
    if (putobj(obj, buf, id, &a) == -1) {
      return TRUE;
    }
    set_graph_modified();
  }

  return FALSE;
}

int
putobj_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, char *prefix)
{
  return _putobj_color(w, aw, obj, id, prefix, NULL);
}

int
putobj_color2(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  return _putobj_color(w, aw, obj, id, NULL, "2");
}

int
putobj_fill_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  return _putobj_color(w, aw, obj, id, "fill_", NULL);
}

int
putobj_stroke_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id)
{
  return _putobj_color(w, aw, obj, id, "stroke_", NULL);
}
