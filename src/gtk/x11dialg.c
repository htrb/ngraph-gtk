/* 
 * $Id: x11dialg.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "gtk_liststore.h"
#include "gtk_combo.h"

#include "ngraph.h"
#include "object.h"

#include "ox11menu.h"
#include "x11gui.h"
#include "x11dialg.h"

void ResetEvent();

struct line_style FwLineStyle[] = {
  {"solid",      "",                        0},
  {"dot",        "100 100",                 2},
  {"short dash", "150 150",                 2},
  {"dash",       "450 150",                 2},
  {"dot dash",   "450 150 150 150",         4},
  {"2-dot dash", "450 150 150 150 150 150", 6},
  {"dot 2-dash", "450 150 450 150 150 150", 6},
};
#define CLINESTYLE (sizeof(FwLineStyle) / sizeof(*FwLineStyle))

char *FwNumStyle[] =
  { "auto", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
int FwNumStyleNum = sizeof(FwNumStyle) / sizeof(*FwNumStyle);

char *CbLineWidth[] = { "10", "20", "40", "80", "120" };
int CbLineWidthNum = sizeof(CbLineWidth) / sizeof(*CbLineWidth);

char *CbTextPt[] = { "1200", "1600", "1800", "2000", "2400", "3000" };
int CbTextPtNum = sizeof(CbTextPt) / sizeof(*CbTextPt);

char *CbDirection[] = { "0", "9000", "18000", "27000" };
int CbDirectionNum = (sizeof(CbDirection) / sizeof(*CbDirection));

char *CbMarkSize[] = { "100", "200", "300", "400", "500" };
int CbMarkSizeNum = sizeof(CbMarkSize) / sizeof(*CbMarkSize);

gint8 Dashes[] = { 2, 2 };
int DashesNum = sizeof(Dashes) / sizeof(*Dashes);

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
struct AxisBaseDialog DlgAxisBase;
struct AxisPosDialog DlgAxisPos;
struct NumDialog DlgNum;
struct AxisFontDialog DlgAxisFont;
struct GaugeDialog DlgGauge;
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
struct ScriptDialog DlgScript;
struct DefaultDialog DlgDefault;
struct SetScriptDialog DlgSetScript;
struct PrefScriptDialog DlgPrefScript;
struct SetDriverDialog DlgSetDriver;
struct PrefDriverDialog DlgPrefDriver;
struct MiscDialog DlgMisc;
struct ExViewerDialog DlgExViewer;
struct ViewerDialog DlgViewer;
struct SelectDialog DlgSelect;
struct CopyDialog DlgCopy;

void
initdialog(void)
{
  DlgFile.widget = NULL;
  DlgFile.resource = N_("data");
  DlgFile.mark.widget = NULL;
  DlgFile.mark.resource = N_("mark selection");
  DlgFileDef.widget = NULL;
  DlgFileDef.resource = N_("data default");
  DlgFileDef.mark.widget = NULL;
  DlgFileDef.mark.resource = N_("mark selection");
  DlgEval.widget = NULL;
  DlgEval.resource = N_("evaluation");
  DlgMath.widget = NULL;
  DlgMath.resource = N_("math");
  DlgMathText.widget = NULL;
  DlgMathText.resource = N_("math text");
  DlgFit.widget = NULL;
  DlgFit.resource = N_("fit");
  DlgFitLoad.widget = NULL;
  DlgFitLoad.resource = N_("fit load");
  DlgFitSave.widget = NULL;
  DlgFitSave.resource = N_("fit save");
  DlgFileMove.widget = NULL;
  DlgFileMove.resource = N_("data move");
  DlgFileMask.widget = NULL;
  DlgFileMask.resource = N_("data mask");
  DlgFileLoad.widget = NULL;
  DlgFileLoad.resource = N_("data load");
  DlgFileMath.widget = NULL;
  DlgFileMath.resource = N_("data math");
  DlgSection.widget = NULL;
  DlgSection.resource = N_("Frame/Section Graph");
  DlgCross.widget = NULL;
  DlgCross.resource = N_("Cross Graph");
  DlgAxis.widget = NULL;
  DlgAxis.resource = N_("axis");
  DlgGrid.widget = NULL;
  DlgGrid.resource = N_("grid");
  DlgZoom.widget = NULL;
  DlgZoom.resource = N_("Scale Zoom");
  DlgAxisBase.widget = NULL;
  DlgAxisBase.resource = N_("Axis Baseine");
  DlgAxisPos.widget = NULL;
  DlgAxisPos.resource = N_("Axis Position");
  DlgNum.widget = NULL;
  DlgNum.resource = N_("Axis Numbering");
  DlgAxisFont.widget = NULL;
  DlgAxisFont.resource = N_("Axis Font");
  DlgGauge.widget = NULL;
  DlgGauge.resource = N_("Axis Gauge");
  DlgMerge.widget = NULL;
  DlgMerge.resource = N_("merge");
  DlgLegendCurve.widget = NULL;
  DlgLegendCurve.resource = N_("legend curve");
  DlgLegendPoly.widget = NULL;
  DlgLegendPoly.resource = N_("legend polygon");
  DlgLegendArrow.widget = NULL;
  DlgLegendArrow.resource = N_("legend line");
  DlgLegendRect.widget = NULL;
  DlgLegendRect.resource = N_("legend rectangle");
  DlgLegendArc.widget = NULL;
  DlgLegendArc.resource = N_("legend arc");
  DlgLegendMark.widget = NULL;
  DlgLegendMark.resource = N_("legend mark");
  DlgLegendMark.mark.widget = NULL;
  DlgLegendMark.mark.resource = N_("mark selection");
  DlgLegendText.widget = NULL;
  DlgLegendText.resource = N_("legend text");
  DlgLegendTextDef.widget = NULL;
  DlgLegendTextDef.resource = N_("textdefault");
  DlgLegendGauss.widget = NULL;
  DlgLegendGauss.resource = N_("legend gauss");
  DlgPage.widget = NULL;
  DlgPage.resource = N_("page");
  DlgSwitch.widget = NULL;
  DlgSwitch.resource = N_("drawobj");
  DlgDirectory.widget = NULL;
  DlgDirectory.resource = N_("directory");
  DlgLoad.widget = NULL;
  DlgLoad.resource = N_("load");
  DlgPrm.widget = NULL;
  DlgPrm.resource = N_("loadprm");
  DlgSave.widget = NULL;
  DlgSave.resource = N_("save");
  DlgDriver.widget = NULL;
  DlgDriver.resource = N_("driver");
  DlgPrinter.widget = NULL;
  DlgPrinter.resource = N_("print");
  DlgOutputData.widget = NULL;
  DlgOutputData.resource = N_("output data");
  DlgScript.widget = NULL;
  DlgScript.resource = N_("addin");
  DlgDefault.widget = NULL;
  DlgDefault.resource = N_("save default");
  DlgSetScript.widget = NULL;
  DlgSetScript.resource = N_("setscript");
  DlgPrefScript.widget = NULL;
  DlgPrefScript.resource = N_("prefscript");
  DlgSetDriver.widget = NULL;
  DlgSetDriver.resource = N_("setdriver");
  DlgPrefDriver.widget = NULL;
  DlgPrefDriver.resource = N_("prefdriver");
  DlgMisc.widget = NULL;
  DlgMisc.resource = N_("misc");
  DlgExViewer.widget = NULL;
  DlgExViewer.resource = N_("extviewer");
  DlgViewer.widget = NULL;
  DlgViewer.resource = N_("viewer");
  DlgSelect.widget = NULL;
  DlgSelect.resource = N_("multi select");
  DlgCopy.widget = NULL;
  DlgCopy.resource = N_("single select");
}

static gboolean
multi_list_default_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  GtkTreeSelection *sel;
  struct SelectDialog *d;
  GList *list;

  d = (struct SelectDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    list = gtk_tree_selection_get_selected_rows(sel, NULL);

    list = gtk_tree_selection_get_selected_rows(sel, NULL);
    if (list == NULL)
      return FALSE;

    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free(list);

    gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);

    return TRUE;
  }
  return FALSE;
}

static void
SelectDialogSetup(GtkWidget *w, void *data, int makewidget)
{
  char *s;
  struct SelectDialog *d;
  int i, *seldata, selnum;
  GtkWidget *swin, *b, *hbox;
  GtkTreeIter iter;
  n_list_store list[] = {
    {N_("multi select"), G_TYPE_STRING, TRUE, FALSE, NULL},
  } ;

  d = (struct SelectDialog *) data;

  if (makewidget) {
    d->list = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(d->list, GTK_SELECTION_MULTIPLE);
    g_signal_connect(d->list, "button-press-event", G_CALLBACK(multi_list_default_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(multi_list_default_cb), d);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), d->list);

    gtk_dialog_add_button(GTK_DIALOG(w), "_All", IDSALL);

    gtk_box_pack_start(GTK_BOX(d->vbox), swin, TRUE, TRUE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    b = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(b, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), b, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    gtk_window_set_default_size(GTK_WINDOW(w), -1, 300);
  }

  list_store_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    s = d->cb(d->Obj, i);
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, (s) ? s: "");
    memfree(s);
  }

  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(XtNameToWidget(w, "*list"), XmNwidth, 200, NULL);
  }
  */

  if (d->isel != NULL) {
    seldata = arraydata(d->isel);
    selnum = arraynum(d->isel);

    for (i = 0; i < selnum; i++) {
       list_store_multi_select_nth(d->list, seldata[i], seldata[i]);
    }
  }
}

static void
select_tree_selection_foreach_cb(GtkTreeModel *model, GtkTreePath *path,
			  GtkTreeIter *iter, gpointer data)
{
  int a;
  char *s;
  struct SelectDialog *d;

  d = (struct SelectDialog *) data;
  s = gtk_tree_path_to_string(path);
  a = atoi(s);
  g_free(s);
  arrayadd(d->sel, &a);
}

static void
SelectDialogClose(GtkWidget *w, void *data)
{
  int i;
  struct SelectDialog *d;

  d = (struct SelectDialog *) data;
  if (d->ret == IDOK) {
    GtkTreeSelection *gsel;

    gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
    gtk_tree_selection_selected_foreach(gsel, select_tree_selection_foreach_cb, data);
  } else if (d->ret == IDSALL) {
    for (i = 0; i <= chkobjlastinst(d->Obj); i++)
      arrayadd(d->sel, &i);
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
CopyDialogSetup(GtkWidget *w, void *data, int makewidget)
{
  char *s;
  struct CopyDialog *d;
  int i;
  GtkWidget *swin;
  GtkTreeIter iter;
  n_list_store copy_list[] = {
    {N_("single select"), G_TYPE_STRING, TRUE, FALSE, NULL},
  } ;

  d = (struct CopyDialog *) data;
  if (makewidget) {
    d->list = list_store_create(sizeof(copy_list) / sizeof(*copy_list), copy_list);
    list_store_set_selection_mode(d->list, GTK_SELECTION_SINGLE);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), d->list);

    g_signal_connect(d->list, "button-press-event", G_CALLBACK(single_list_default_cb), d);
    g_signal_connect(d->list, "key-press-event", G_CALLBACK(single_list_default_cb), d);
    gtk_container_add(GTK_CONTAINER (GTK_DIALOG(w)->vbox), swin);
    gtk_window_set_default_size(GTK_WINDOW(w), -1, 300);
  }

  list_store_clear(d->list);

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    s = d->cb(d->Obj, i);
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, (s) ? s: "");
    memfree(s);
  }

  if (d->Id >= 0) {
    list_store_select_nth(d->list, d->Id);
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

  d->sel = list_store_get_selected_index(d->list);
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
SetObjFieldFromText(GtkWidget *w, struct objlist *Obj, int Id,
		    char *field)
{
  GtkEntry *entry;
  const char *tmp;
  char *buf;

  if (w == NULL)
    return 0;

  entry = GTK_ENTRY(w);
  tmp = gtk_entry_get_text(entry);

  if (tmp == NULL)
    return -1;

  buf = strdup(tmp);

  if (buf == NULL)
    return -1;

  if (sputobjfield(Obj, Id, field, buf) != 0) {
    free(buf);
    return -1;
  }

  free(buf);
  return 0;
}

void
SetTextFromObjField(GtkWidget *w, struct objlist *Obj, int Id,
		    char *field)
{
  GtkEntry *entry;
  char *buf;

  if (w == NULL)
    return;

  entry = GTK_ENTRY(w);

  sgetobjfield(Obj, Id, field, NULL, &buf, FALSE, FALSE, FALSE);
  gtk_entry_set_text(entry, buf);
  memfree(buf);
}

int
SetObjFieldFromToggle(GtkWidget *w, struct objlist *Obj, int Id,
		      char *field)
{
  gboolean state;
  int a;

  if (w == NULL)
    return 0;

  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
  a = state ? TRUE : FALSE;
  if (putobj(Obj, field, Id, &a) == -1)
    return -1;

  return 0;
}

void
SetToggleFromObjField(GtkWidget *w, struct objlist *Obj, int Id,
		      char *field)
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
  int j;
  char *buf;
  const char *ptr;

  if (w == NULL)
    return 0;

  ptr = gtk_entry_get_text(GTK_ENTRY(GTK_BIN(w)->child));

  if (ptr == NULL) {
    return -1;
  }

  for (j = 0; j < CLINESTYLE; j++) {
    if (strcmp(ptr, FwLineStyle[j].name) == 0) {
      if (sputobjfield(Obj, Id, field, FwLineStyle[j].list) != 0) {
	return -1;
      }
      break;
    }
  }

  buf = strdup(ptr);
  if (buf == NULL)
    return -1;

  if (j == CLINESTYLE) {
    if (sputobjfield(Obj, Id, field, buf) != 0) {
      free(buf);
      return -1;
    }
  }

  free(buf);
  return 0;
}

void
SetStyleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field)
{
  int i, j;
  GtkEntry *entry;
  struct narray *array;
  int stylenum;
  int *style, a, count;
  char *s;

  if (w == NULL)
    return;

  entry = GTK_ENTRY(GTK_BIN(w)->child);

  count = combo_box_get_num(w);
  if (count == 0) {
    for (j = 0; j < CLINESTYLE; j++) {
      combo_box_append_text(w, FwLineStyle[j].name);
    }
  }

  getobj(Obj, field, Id, 0, NULL, &array);
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
	goto match;
    }
  }
  SetTextFromObjField(GTK_WIDGET(entry), Obj, Id, field);
  return;

match:
  gtk_entry_set_text(entry, FwLineStyle[j].name);
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
SetObjFieldFromList(GtkWidget *w, struct objlist *Obj, int Id,
		    char *field)
{
  int pos;

  if (w == NULL)
    return 0;

  pos = combo_box_get_active(w);

  if (pos < 0 || putobj(Obj, field, Id, &pos) == -1) {
    return -1;
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
    for (j = 0; enumlist[j] != NULL; j++) {
      combo_box_append_text(w, enumlist[j]);
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

  fcur = Mxlocal->fontmaproot;
  j = 0;
  selfont = -1;
  while (fcur != NULL) {
    if ((jfont && fcur->twobyte) || (! jfont && !(fcur->twobyte))) {
      combo_box_append_text(w, fcur->fontalias);
      if (strcmp(font, fcur->fontalias) == 0)
	selfont = j;
      j++;
    }
    fcur = fcur->next;
  }

  if (selfont != -1)
    combo_box_set_active(w, selfont);
}

void
SetObjFieldFromFontList(GtkWidget *w, struct objlist *obj, int id, char *name, int jfont)
{
  struct fontmap *fcur;
  int pos, j;

  if (w == NULL)
    return;

  pos = combo_box_get_active(w);

  if (pos < 0)
    return;

  j = 0;
  for (fcur = Mxlocal->fontmaproot; fcur; fcur = fcur->next) {
    if ((! jfont || ! fcur->twobyte) && (jfont || fcur->twobyte))
	continue;

    if (j == pos) {
      sputobjfield(obj, id, name, fcur->fontalias);
      break;
    }
    j++;
  }
}

static void
_set_color(GtkWidget *w, struct objlist *obj, int id, char *prefix, char *postfix)
{
  GdkColor color;
  int r, g, b;
  char buf[64];

  snprintf(buf, sizeof(buf), "%sR%s", (prefix)? prefix: "", (postfix)? postfix: "");
  getobj(obj, buf, id, 0, NULL, &r);

  snprintf(buf, sizeof(buf), "%sG%s", (prefix)? prefix: "", (postfix)? postfix: "");
  getobj(obj, buf, id, 0, NULL, &g);

  snprintf(buf, sizeof(buf), "%sB%s", (prefix)? prefix: "", (postfix)? postfix: "");
  getobj(obj, buf, id, 0, NULL, &b);

  color.red = r * 256;
  color.green = g * 256;
  color.blue = b * 256;

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
  int r, g, b;
  char buf[64];

  gtk_color_button_get_color(GTK_COLOR_BUTTON(w), &color);
  r = color.red / 256;
  g = color.green / 256;
  b = color.blue / 256;

  snprintf(buf, sizeof(buf), "%sR%s", (prefix)? prefix: "", (postfix)? postfix: "");
  if (putobj(obj, buf, id, &r) == -1)
    return TRUE;

  snprintf(buf, sizeof(buf), "%sG%s", (prefix)? prefix: "", (postfix)? postfix: "");
  if (putobj(obj, buf, id, &g) == -1)
    return TRUE;

  snprintf(buf, sizeof(buf), "%sB%s", (prefix)? prefix: "", (postfix)? postfix: "");
  if (putobj(obj, buf, id, &b) == -1)
    return TRUE;

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

