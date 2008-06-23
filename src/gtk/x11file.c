/* 
 * $Id: x11file.c,v 1.14 2008/06/23 02:18:25 hito Exp $
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "gtk_entry_completion.h"
#include "gtk_liststore.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathcode.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"
#include "nconfig.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11file.h"
#include "x11commn.h"

static n_list_store Flist[] = {
  {"",	        G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden",     FALSE},
  {"#",		G_TYPE_INT,     TRUE, FALSE, "id",         FALSE},
  {"file",	G_TYPE_STRING,  TRUE, FALSE, "file",       TRUE},
  {"x",		G_TYPE_INT,     TRUE, FALSE, "x",          FALSE},
  {"y",		G_TYPE_INT,     TRUE, FALSE, "y",          FALSE},
  {"ax",	G_TYPE_STRING,  TRUE, FALSE, "axis_x",     FALSE},
  {"ay",	G_TYPE_STRING,  TRUE, FALSE, "axis_y",     FALSE},
  {"type",	G_TYPE_OBJECT,  TRUE, FALSE, "type",       FALSE},
  {"size",	G_TYPE_INT,     TRUE, FALSE, "mark_size",  FALSE},
  {"width",	G_TYPE_INT,     TRUE, FALSE, "line_width", FALSE},
  {"skip",	G_TYPE_INT,     TRUE, FALSE, "head_skip",  FALSE},
  {"step",	G_TYPE_INT,     TRUE, FALSE, "read_step",  FALSE},
  {"final",	G_TYPE_INT,     TRUE, FALSE, "final_line", FALSE},
  {"num",	G_TYPE_INT,     TRUE, FALSE, "data_num",   FALSE},
  {"^#",	G_TYPE_INT,     TRUE, FALSE, "oid",        FALSE},
};

#define FILE_WIN_COL_NUM (sizeof(Flist)/sizeof(*Flist))
#define FILE_WIN_COL_OID (FILE_WIN_COL_NUM - 1)
#define FILE_WIN_COL_ID 1


static void file_delete_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy2_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_fit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_edit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_draw_popup_func(GtkMenuItem *w, gpointer client_data);

static struct subwin_popup_list Popup_list[] = {
  {GTK_STOCK_OPEN,        G_CALLBACK(CmFileOpenB), TRUE, NULL},
  {GTK_STOCK_PROPERTIES,  G_CALLBACK(list_sub_window_update), TRUE, NULL},
  {GTK_STOCK_CLOSE,       G_CALLBACK(file_delete_popup_func), TRUE, NULL},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(list_sub_window_move_top), TRUE, NULL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(list_sub_window_move_up), TRUE, NULL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(list_sub_window_move_down), TRUE, NULL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(list_sub_window_move_last), TRUE, NULL},
  {GTK_STOCK_EDIT,        G_CALLBACK(file_edit_popup_func), TRUE, NULL},
  {N_("_Duplicate"),          G_CALLBACK(file_copy_popup_func), FALSE, NULL},
  {N_("duplicate _Behind"),   G_CALLBACK(file_copy2_popup_func), FALSE, NULL},
  {N_("_Draw"),               G_CALLBACK(file_draw_popup_func), FALSE, NULL},
  {N_("_Hide"),               G_CALLBACK(list_sub_window_hide), FALSE, NULL},
  {N_("_Fit"),                G_CALLBACK(file_fit_popup_func), FALSE, NULL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))

#define FITSAVE "fit.ngp"
#define CB_BUF_SIZE 128

static char *FieldStr[] = {"math_x", "math_y", "func_f", "func_g", "func_h"};

static gboolean FileWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data);

static void
MathTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct MathTextDialog *d;
  static char *label[] = {N_("_Math X:"), N_("_Math Y:"), "_F(X,Y,Z):", "_G(X,Y,Z):", "_H(X,Y,Z):"};
  char **array;
  int num;

  d = (struct MathTextDialog *) data;
  d->math = NULL;
  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    d->label = item_setup(hbox, w, _("_Math:"), TRUE);
    d->list = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }
  num = arraynum(Menulocal.mathlist);
  array = (char **) arraydata(Menulocal.mathlist);

  switch (d->Mode) {
  case 0:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.x_math_list);
    break;
  case 1:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.y_math_list);
    break;
  case 2:
  case 3:
  case 4:
    gtk_entry_set_completion(GTK_ENTRY(d->list), NgraphApp.func_list);
    break;
  }

  gtk_label_set_text_with_mnemonic(GTK_LABEL(d->label), _(label[d->Mode]));
  gtk_entry_set_text(GTK_ENTRY(d->list), d->Text);
  gtk_window_set_default_size(GTK_WINDOW(wi), 400, -1);
}

static void
MathTextDialogClose(GtkWidget *w, void *data)
{
  struct MathTextDialog *d;
  const char *p;

  d = (struct MathTextDialog *) data;

  if (d->ret == IDCANCEL)
    return;

  p = gtk_entry_get_text(GTK_ENTRY(d->list));
  d->math = strdup(p);
  switch (d->Mode) {
  case 0:
    entry_completion_append(NgraphApp.x_math_list, p);
    break;
  case 1:
    entry_completion_append(NgraphApp.y_math_list, p);
    break;
  case 2:
  case 3:
  case 4:
    entry_completion_append(NgraphApp.func_list, p);
    break;
  }
  gtk_entry_set_completion(GTK_ENTRY(d->list), NULL);
}

void
MathTextDialog(struct MathTextDialog *data, char *text, int mode)
{
  if (mode < 0 || mode >= MATH_FNC_NUM)
    mode = 0;

  data->SetupWindow = MathTextDialogSetup;
  data->CloseWindow = MathTextDialogClose;
  data->Text = text;
  data->Mode = mode;
}

static void
MathDialogSetupItem(GtkWidget *w, struct MathDialog *d)
{
  int i;
  char *math, *field = NULL;
  GtkTreeIter iter;

  list_store_clear(d->list);

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    math = NULL;
    getobj(d->Obj, field, i, 0, NULL, &math);
    list_store_append(d->list, &iter);
    list_store_set_int(d->list, &iter, 0, i);
    list_store_set_string(d->list, &iter, 1, (math) ? math : "");
  }

  if (d->Mode >= 0 && d->Mode < MATH_FNC_NUM)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->func[d->Mode]), TRUE);
}

static void
MathDialogMode(GtkWidget *w, gpointer client_data)
{
  struct MathDialog *d;
  int i;
  
  if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    return;

  d = (struct MathDialog *) client_data;

  for (i = 0; i < MATH_FNC_NUM; i++) {
    if (w == d->func[i])
      d->Mode = i;
  }
  MathDialogSetupItem(d->widget, d);
}

static void
MathDialogList(GtkButton *w, gpointer client_data)
{
  struct MathDialog *d;
  int a, *ary;
  char *field = NULL, *buf;
  GtkTreeSelection *gsel;
  GtkTreePath *path;
  GList *list, *data;

  d = (struct MathDialog *) client_data;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  list = gtk_tree_selection_get_selected_rows(gsel, NULL);

  if (list == NULL)
    return;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(d->list), &path, NULL);

  if (path) {
    ary = gtk_tree_path_get_indices(path);
    a = ary[0];
    gtk_tree_path_free(path);
  } else {
    data = g_list_last(list);
    ary = gtk_tree_path_get_indices(data->data);
    a = ary[0];
  }

  if (ary == NULL)
    goto END;

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  sgetobjfield(d->Obj, a, field, NULL, &buf, FALSE, FALSE, FALSE);
  if (buf == NULL)
    goto END;

  MathTextDialog(&DlgMathText, buf, d->Mode);

  if (DialogExecute(d->widget, &DlgMathText) == IDOK) {
    for (data = list; data; data = data->next) {
      ary = gtk_tree_path_get_indices(data->data);
      if (ary == NULL)
	continue;

      sputobjfield(d->Obj, ary[0], field, DlgMathText.math);
      AddMathList(DlgMathText.math);
      NgraphApp.Changed = TRUE;
    }
    free(DlgMathText.math);
  }
  memfree(buf);

  MathDialogSetupItem(d->widget, d);

  for (data = list; data; data = data->next) {
    ary = gtk_tree_path_get_indices(data->data);
    if (ary == NULL)
      continue;

    gtk_tree_selection_select_path(gsel, data->data);
  }

 END:
  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);
}

static gboolean
math_dialog_key_pressed_cb(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct MathDialog *d;
  GtkTreeSelection *gsel;
  GList *list;

  d = (struct MathDialog *) user_data;

  if (e->keyval != GDK_Return)
    return FALSE;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  list = gtk_tree_selection_get_selected_rows(gsel, NULL);

  if (list == NULL)
    return FALSE;

  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);

  MathDialogList(NULL, d);

  return TRUE;
}

static gboolean
math_dialog_butten_pressed_cb(GtkWidget *w, GdkEventButton *e, gpointer user_data)
{
  struct MathDialog *d;

  d = (struct MathDialog *) user_data;

  if (e->type != GDK_2BUTTON_PRESS)
    return FALSE;

  MathDialogList(NULL, d);

  return TRUE;
}

static void
MathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *vbox, *hbox;
  struct MathDialog *d;
  static n_list_store list[] = {
    {"id",       G_TYPE_INT,    TRUE, FALSE, NULL, FALSE},
    {N_("math"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };
  int i;

  d = (struct MathDialog *) data;

  if (makewidget) {
    char *button_str[] = {
      N_("_X math"),
      N_("_Y math"),
      "_F(X, Y, Z)",
      "_G(X, Y, Z)",
      "_H(X, Y, Z)",
    };

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(math_dialog_key_pressed_cb), d);
    g_signal_connect(w, "button-press-event", G_CALLBACK(math_dialog_butten_pressed_cb), d);
    d->list = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);

    w = NULL;
    for (i = 0; i < MATH_FNC_NUM; i++) {
      w = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(w), _(button_str[i]));
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
      d->func[i] = w;
      g_signal_connect(w, "toggled", G_CALLBACK(MathDialogMode), d);
    }

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_from_stock(GTK_STOCK_EDIT);
    g_signal_connect(w, "clicked", G_CALLBACK(MathDialogList), d);
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 4);

    d->show_cancel = FALSE;

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
  }

  MathDialogSetupItem(wi, d);
}

static void
MathDialogClose(GtkWidget *w, void *data)
{
}

void
MathDialog(struct MathDialog *data, struct objlist *obj)
{
  data->SetupWindow = MathDialogSetup;
  data->CloseWindow = MathDialogClose;
  data->Obj = obj;
  data->Mode = 0;
}

static gboolean
fit_load_dialog_default_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct FitLoadDialog *d;

  d = (struct FitLoadDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)) {
    gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
    return TRUE;
  }

  return FALSE;
}

static void
FitLoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  char *s;
  struct FitLoadDialog *d;
  int i;
  GtkWidget *w;
  GtkTreeIter iter;
  n_list_store list[] = {
    {N_("Fit setting"), G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct FitLoadDialog *) data;
  if (makewidget) {
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    d->list = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "button-press-event", G_CALLBACK(fit_load_dialog_default_cb), d);
    g_signal_connect(w, "key-press-event", G_CALLBACK(fit_load_dialog_default_cb), d);
  }
  list_store_clear(d->list);
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, (s)? s: "");
  }
  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(d->list, XmNwidth, 200, NULL);
  }
  */
}

static void
FitLoadDialogClose(GtkWidget *w, void *data)
{
  struct FitLoadDialog *d;

  d = (struct FitLoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
  d->sel = list_store_get_selected_index(d->list);
}

void
FitLoadDialog(struct FitLoadDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitLoadDialogSetup;
  data->CloseWindow = FitLoadDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->sel = -1;
}

static void
FitSaveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct FitSaveDialog *d;
  int i, j;
  char *s;

  d = (struct FitSaveDialog *) data;
  if (makewidget) {
    w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

    hbox = gtk_hbox_new(FALSE, 4);
    item_setup(hbox, w, _("_Profile:"), TRUE);
    d->profile = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
  }
  combo_box_clear(d->profile);
  j = 0;
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    combo_box_append_text(d->profile, (s)? s: "&");
  }
}

static void
FitSaveDialogClose(GtkWidget *w, void *data)
{
  struct FitSaveDialog *d;
  const char *s;

  d = (struct FitSaveDialog *) data;

  if (d->ret != IDOK)
    return;

  s = combo_box_entry_get_text(d->profile);
  if (s) {
    d->Profile = nstrdup(s);
  }
}

void
FitSaveDialog(struct FitSaveDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitSaveDialogSetup;
  data->CloseWindow = FitSaveDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->Profile = NULL;
}

static void
FitDialogSetupItem(GtkWidget *w, struct FitDialog *d, int id)
{
  int a, i;

  SetListFromObjField(d->type, d->Obj, id, "type");

  getobj(d->Obj, "poly_dimension", id, 0, NULL, &a);
  combo_box_set_active(d->dim, a);

  SetTextFromObjField(d->weight, d->Obj, id, "weight_func");

  SetToggleFromObjField(d->through_point, d->Obj, id, "through_point");

  SetTextFromObjField(d->x, d->Obj, id, "point_x");

  SetTextFromObjField(d->y, d->Obj, id, "point_y");

  SetTextFromObjField(d->min, d->Obj, id, "min");

  SetTextFromObjField(d->max, d->Obj, id, "max");

  SetTextFromObjField(d->div, d->Obj, id, "div");

  SetToggleFromObjField(d->interpolation, d->Obj, id, "interpolation");

  SetTextFromObjField(d->formula, d->Obj, id, "user_func");

  SetTextFromObjField(d->converge, d->Obj, id, "converge");

  SetToggleFromObjField(d->derivatives, d->Obj, id, "derivative");

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    SetTextFromObjField(d->p[i], d->Obj, id, p);
    SetTextFromObjField(d->d[i], d->Obj, id, dd);
  }
}

static char *
FitCB(struct objlist *obj, int id)
{
  char *valstr, *s;

  s = (char *) memalloc(CB_BUF_SIZE);
  if (s == NULL)
    return NULL;

  sgetobjfield(obj, id, "type", NULL, &valstr, FALSE, FALSE, FALSE);
  snprintf(s, CB_BUF_SIZE, "%-5d %.100s", id, valstr);
  memfree(valstr);
  return s;
}

static void
FitDialogCopy(  struct FitDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FitCB)) != -1)
    FitDialogSetupItem(d->widget, d, sel);
}

static int
FitDialogLoadConfig(struct FitDialog *d, int errmes)
{
  int lastid;
  int newid;
  struct objlist *shell;
  struct narray sarray;
  char *argv[2];
  char *file;

  lastid = chkobjlastinst(d->Obj);
  if (lastid == d->Lastid) {
    if ((file = searchscript(FITSAVE)) == NULL) {
      if (errmes)
	MessageBox(TopLevel, "Setting file not found.", FITSAVE, MB_OK);
      return FALSE;
    }
    if ((shell = chkobject("shell")) == NULL)
      return FALSE;
    newid = newobj(shell);
    if (newid < 0) {
      memfree(file);
      return FALSE;
    }
    arrayinit(&sarray, sizeof(char *));
    changefilename(file);
    if (arrayadd(&sarray, &file) == NULL) {
      memfree(file);
      arraydel2(&sarray);
      return FALSE;
    }
    argv[0] = (char *) &sarray;
    argv[1] = NULL;
    exeobj(shell, "shell", newid, 1, argv);
    arraydel2(&sarray);
    delobj(shell, newid);
  }
  return TRUE;
}

static void
FitDialogLoad(struct FitDialog *d)
{
  int lastid, id;

  if (!FitDialogLoadConfig(d, TRUE))
    return;
  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    MessageBox(TopLevel, "No settings.", FITSAVE, MB_OK);
    return;
  }
  FitLoadDialog(&DlgFitLoad, d->Obj, d->Lastid + 1);
  if ((DialogExecute(d->widget, &DlgFitLoad) == IDOK)
      && (DlgFitLoad.sel >= 0)) {
    id = DlgFitLoad.sel + d->Lastid + 1;
    FitDialogSetupItem(d->widget, d, id);
  }
}

static void
FitDialogSave(struct FitDialog *d)
{
  int i, id, len;
  char *s;
  char *ngpfile;
  int error;
  HANDLE hFile;
  int num;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  if (DialogExecute(d->widget, &DlgFitSave) != IDOK)
    return;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (strcmp(s, DlgFitSave.Profile) == 0) {
      if (MessageBox(TopLevel, "Overwrite existing setting?", "Confirm",
		     MB_YESNO) != IDYES) {
	return;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    id = newobj(d->Obj);
  } else {
    id = i;
  }

  if (putobj(d->Obj, "profile", id, DlgFitSave.Profile) == -1)
    return;

  if (SetObjFieldFromList(d->type, d->Obj, id, "type"))
    return;

  num = combo_box_get_active(d->dim);
  if (num >= 0 && putobj(d->Obj, "poly_dimension", id, &num) == -1)
    return;

  if (SetObjFieldFromText(d->weight, d->Obj, id, "weight_func"))
    return;

  if (SetObjFieldFromToggle
      (d->through_point, d->Obj, id, "through_point"))
    return;

  if (SetObjFieldFromText(d->x, d->Obj, id, "point_x"))
    return;

  if (SetObjFieldFromText(d->y, d->Obj, id, "point_y"))
    return;

  if (SetObjFieldFromText(d->min, d->Obj, id, "min"))
    return;

  if (SetObjFieldFromText(d->max, d->Obj, id, "max"))
    return;

  if (SetObjFieldFromText(d->div, d->Obj, id, "div"))
    return;

  if (SetObjFieldFromToggle(d->interpolation, d->Obj, id,
			    "interpolation"))
    return;
  if (SetObjFieldFromText(d->formula, d->Obj, id, "user_func"))
    return;

  if (SetObjFieldFromToggle(d->derivatives, d->Obj, id, "derivative"))
    return;

  if (SetObjFieldFromText(d->converge, d->Obj, id, "converge"))
    return;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromText(d->p[i], d->Obj, id, p))
      return;

    if (SetObjFieldFromText(d->d[i], d->Obj, id, dd))
      return;
  }

  if ((ngpfile = getscriptname(FITSAVE)) == NULL)
    return;

  error = FALSE;

  hFile = nopen(ngpfile, O_CREAT | O_TRUNC | O_RDWR, NFMODE);

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "save", i, 0, NULL, &s);
    len = strlen(s);
    if (len != nwrite(hFile, s, len))
      error = TRUE;
    if (nwrite(hFile, "\n", 1) != 1)
      error = TRUE;
  }
  nclose(hFile);
  if (error)
    MessageBox(TopLevel, "I/O error: Write", "Error:", MB_OK);
  memfree(ngpfile);
}

static void
FitDialogResult(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;
  double derror, correlation, coe[10];
  char *equation, *math, *code, *inst, buf[1024];
  int i, j, dim, dimension, maxdim, need2pass, *needdata, tbl[10], type, num;
  struct narray needarray;

  d = (struct FitDialog *) client_data;

  if ((inst = chkobjinst(d->Obj, d->Id)) == NULL)
    return;

  if (_getobj(d->Obj, "type", inst, &type))
    return;

  if (_getobj(d->Obj, "poly_dimension", inst, &dimension))
    return;

  if (_getobj(d->Obj, "number", inst, &num))
    return;

  if (_getobj(d->Obj, "error", inst, &derror))
    return;

  if (_getobj(d->Obj, "correlation", inst, &correlation))
    return;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%00";

    p[sizeof(p) - 2] += i;

    if (_getobj(d->Obj, p, inst, coe + i)) {
      return;
    }
  }

  if (_getobj(d->Obj, "equation", inst, &equation))
    return;

  if (_getobj(d->Obj, "user_func", inst, &math))
    return;

  if (equation == NULL) {
    snprintf(buf, sizeof(buf), "Undefined");
  } else if (type != 4) {
    i = 0;

    if (type == 0) {
      dim = dimension + 1;
    } else {
      dim = 2;
    }

    if (type == 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: %%0i*X^i (i=0-%d)\n\n", dim - 1);
    } else if (type == 1) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: exp(%%00)*X^%%01\n\n");
    } else if (type == 2) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: exp(%%01*X+%%00)\n\n");
    } else if (type == 3) {
      i += snprintf(buf + i, sizeof(buf) - i, "Eq: %%01*Ln(X)+%%00\n\n");
    }

    for (j = 0; j < dim; j++) {
      i += snprintf(buf + i, sizeof(buf) - i, "       %%0%d = %.7e\n", j, coe[j]);
    }
    i += snprintf(buf + i, sizeof(buf) - i, "\n");
    i += snprintf(buf + i, sizeof(buf) - i, "    points = %d\n", num);
    i += snprintf(buf + i, sizeof(buf) - i, "    <DY^2> = %.7e\n", derror);

    if (correlation >= 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = %.7e\n", correlation);
    } else {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = -------------\n");
    }
  } else {
    arrayinit(&needarray, sizeof(int));
    mathcode(math, &code, &needarray, NULL, &maxdim, &need2pass,
	     TRUE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,
	     FALSE, FALSE);
    memfree(code);
    dim = arraynum(&needarray);
    needdata = (int *) arraydata(&needarray);
    for (i = 0; i < dim; i++) {
      tbl[i] = needdata[i];
    }
    arraydel(&needarray);

    i = 0;
    i += snprintf(buf + i, sizeof(buf) - i, "Eq: User defined\n\n");

    for (j = 0; j < dim; j++) {
      i += snprintf(buf + i, sizeof(buf) - i, "       %%0%d = %.7e\n", tbl[j], coe[tbl[j]]);
    }
    i += snprintf(buf + i, sizeof(buf) - i, "\n");
    i += snprintf(buf + i, sizeof(buf) - i, "    points = %d\n", num);
    i += snprintf(buf + i, sizeof(buf) - i, "    <DY^2> = %.7e\n", derror);

    if (correlation >= 0) {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = %.7e\n", correlation);
    } else {
      i += snprintf(buf + i, sizeof(buf) - i, "|r| or |R| = -------------\n");
    }
  }
  MessageBox(d->widget, buf, _("Fitting Results"), MB_OK);
}

static int
FitDialogApply(GtkWidget *w, struct FitDialog *d)
{
  int i;
  int num;

  if (SetObjFieldFromList(d->type, d->Obj, d->Id, "type"))
    return FALSE;

  num = combo_box_get_active(d->dim);
  if (num >= 0 && putobj(d->Obj, "poly_dimension", d->Id, &num) == -1)
      return FALSE;

  if (SetObjFieldFromText(d->weight, d->Obj, d->Id, "weight_func"))
    return FALSE;

  if (SetObjFieldFromToggle(d->through_point, d->Obj, d->Id, "through_point"))
    return FALSE;

  if (SetObjFieldFromText(d->x, d->Obj, d->Id, "point_x"))
    return FALSE;

  if (SetObjFieldFromText(d->y, d->Obj, d->Id, "point_y"))
    return FALSE;

  if (SetObjFieldFromText(d->min, d->Obj, d->Id, "min"))
    return FALSE;

  if (SetObjFieldFromText(d->max, d->Obj, d->Id, "max"))
    return FALSE;

  if (SetObjFieldFromText(d->div, d->Obj, d->Id, "div"))
    return FALSE;

  if (SetObjFieldFromToggle(d->interpolation, d->Obj, d->Id, "interpolation"))
    return FALSE;

  if (SetObjFieldFromText(d->formula, d->Obj, d->Id, "user_func"))
    return FALSE;

  if (SetObjFieldFromToggle(d->derivatives, d->Obj, d->Id, "derivative"))
    return FALSE;

  if (SetObjFieldFromText(d->converge, d->Obj, d->Id, "converge"))
    return FALSE;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromText(d->p[i], d->Obj, d->Id, p))
      return FALSE;

    if (SetObjFieldFromText(d->d[i], d->Obj, d->Id, dd))
      return FALSE;
  }

  NgraphApp.Changed = TRUE;

  return TRUE;
}

static void
FitDialogDraw(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;
  if (!FitDialogApply(d->widget, d))
    return;
  FitDialogSetupItem(d->widget, d, d->Id);
  Draw(FALSE);
}

static void
FitDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *frame;
  struct FitDialog *d;
  char title[20], **enumlist, mes[10];
  int i;

  d = (struct FitDialog *) data;
  snprintf(title, sizeof(title), _("Fit %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_DELETE, IDDELETE,
			   GTK_STOCK_COPY, IDCOPY,
			   _("_Load"), IDLOAD,
			   GTK_STOCK_SAVE, IDSAVE,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Type:"), FALSE);
    d->type = w;

    w = gtk_check_button_new_with_mnemonic(_("_Through"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->through_point = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, "_X:", TRUE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, "_Y:", TRUE);
    d->y = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = combo_box_create();
    item_setup(hbox, w, _("_Dim:"), FALSE);
    d->dim = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Weight:"), TRUE);
    d->weight = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Div:"), TRUE);
    d->div = w;


    w = gtk_check_button_new_with_mnemonic(_("_Interpolation"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->interpolation = w;

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Formula:"), TRUE);
    d->formula = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Converge (%):"), TRUE);
    d->converge = w;

    w = gtk_check_button_new_with_mnemonic(_("_Derivatives"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->derivatives = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    for (i = 0; i < FIT_PARM_NUM; i++) {
      char p[] = "_P0", dd[] = "_D0";
    
      p[sizeof(p) - 2] += i;
      dd[sizeof(dd) - 2] += i;

      hbox = gtk_hbox_new(FALSE, 4);

      w = create_text_entry(TRUE, TRUE);

      item_setup(hbox, w, p, TRUE);
      d->p[i] = w;

      w = create_text_entry(TRUE, TRUE);

      item_setup(hbox, w, dd, TRUE);
      d->d[i] = w;

      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    }

    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);

    enumlist = (char **) chkobjarglist(d->Obj, "type");
    for (i = 0; enumlist[i] != NULL; i++) {
      combo_box_append_text(d->type, _(enumlist[i]));
    }

    for (i = 0; i < FIT_PARM_NUM; i++) {
      snprintf(mes, sizeof(mes), "%d", i + 1);
      combo_box_append_text(d->dim, mes);
    }

    hbox = gtk_hbox_new(FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Result"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogResult), d);

    w = gtk_button_new_with_mnemonic(_("_Draw"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogDraw), d);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
  }


  FitDialogSetupItem(wi, d, d->Id);
}

static void
FitDialogClose(GtkWidget *w, void *data)
{
  struct FitDialog *d;
  int ret;
  int i, lastid;

  d = (struct FitDialog *) data;
  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FitDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDLOAD:
    FitDialogLoad(d);
    d->ret = IDLOOP;
    return;
  case IDSAVE:
    FitDialogSave(d);
    d->ret = IDLOOP;
    return;
  case IDDELETE:
    return;
  case IDCANCEL:
    return;
  default:
    d->ret = IDLOOP;
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;
  if (!FitDialogApply(w, d))
    return;
  d->ret = ret;
  lastid = chkobjlastinst(d->Obj);
  for (i = lastid; i > d->Lastid; i--)
    delobj(d->Obj, i);
}

void
FitDialog(struct FitDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FitDialogSetup;
  data->CloseWindow = FitDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->Lastid = chkobjlastinst(obj);
}

static void
FileMoveDialogSetupItem(GtkWidget *w, struct FileMoveDialog *d, int id)
{
  int j, movenum, line;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  char buf[64];

  list_store_clear(d->list);

  getobj(d->Obj, "move_data", d->Id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", d->Id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", d->Id, 0, NULL, &movey);

  movenum = arraynum(move);
  
  if (arraynum(movex) < movenum) {
    movenum = arraynum(movex);
  }

  if (arraynum(movey) < movenum) {
    movenum = arraynum(movey);
  }

  if (movenum > 0) {
    for (j = 0; j < movenum; j++) {
      line = *(int *) arraynget(move, j);
      x = *(double *) arraynget(movex, j);
      y = *(double *) arraynget(movey, j);

      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, line);

      snprintf(buf, sizeof(buf), "%+.15e", x);
      list_store_set_string(d->list, &iter, 1, buf);

      snprintf(buf, sizeof(buf), "%+.15e", y);
      list_store_set_string(d->list, &iter, 2, buf);
    }
  }
}

static void
FileMoveDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileMoveDialog *d;
  int a;
  double x, y;
  const char *buf;
  char *endptr, buf2[64];
  GtkTreeIter iter;

  d = (struct FileMoveDialog *) client_data;

  buf = gtk_entry_get_text(GTK_ENTRY(d->line));
  if (buf[0] == '\0') return;

  a = strtol(buf, &endptr, 10);
  if (endptr[0] != '\0') return;

  buf = gtk_entry_get_text(GTK_ENTRY(d->x));
  if (buf[0] == '\0') return;

  x = strtod(buf, &endptr);
  if (endptr[0] != '\0') return;

  buf = gtk_entry_get_text(GTK_ENTRY(d->y));
  if (buf[0] == '\0') return;

  y = strtod(buf, &endptr);
  if (endptr[0] != '\0') return;

  list_store_append(d->list, &iter);
  list_store_set_int(d->list, &iter, 0, a);

  snprintf(buf2, sizeof(buf2), "%+.15e", x);
  list_store_set_string(d->list, &iter, 1, buf2);

  snprintf(buf2, sizeof(buf2), "%+.15e", y);
  list_store_set_string(d->list, &iter, 2, buf2);

  gtk_entry_set_text(GTK_ENTRY(d->line), "");
  gtk_entry_set_text(GTK_ENTRY(d->x), "");
  gtk_entry_set_text(GTK_ENTRY(d->y), "");
}


static gboolean
move_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileMoveDialog *d;

  d = (struct FileMoveDialog *) user_data;
  if (e->keyval != GDK_Return)
    return FALSE;

  FileMoveDialogAdd(NULL, d);

  return TRUE;
}

static void
FileMoveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  struct FileMoveDialog *d;
  n_list_store list[] = {
    {N_("Line No."), G_TYPE_INT,    TRUE, FALSE, NULL, FALSE},
    {"X",            G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
    {"Y",            G_TYPE_STRING, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct FileMoveDialog *) data;

  if (makewidget) {
    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);


    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, _("_Line:"), TRUE);
    d->line = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, "_X:", TRUE);
    d->x = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(move_dialog_key_pressed), d);
    item_setup(hbox, w, "_Y:", TRUE);
    d->y = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogAdd), d);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_remove_selected_cb), d->list);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), 640, 400);
  }

  FileMoveDialogSetupItem(wi, d, d->Id);

  if (makewidget)
    d->widget = NULL;
}

static void
FileMoveDialogClose(GtkWidget *w, void *data)
{
  struct FileMoveDialog *d;
  int ret, j, movenum, line, a;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  gboolean state;
  char *ptr, *endptr;

  d = (struct FileMoveDialog *) data;
  if (d->ret != IDOK)
    return;
  ret = d->ret;
  d->ret = IDLOOP;
  getobj(d->Obj, "move_data", d->Id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", d->Id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", d->Id, 0, NULL, &movey);
  if (move != NULL) {
    putobj(d->Obj, "move_data", d->Id, NULL);
    move = NULL;
  }
  if (movex != NULL) {
    putobj(d->Obj, "move_data_x", d->Id, NULL);
    movex = NULL;
  }
  if (movey != NULL) {
    putobj(d->Obj, "move_data_y", d->Id, NULL);
    movey = NULL;
  }

  state = list_store_get_iter_first(d->list, &iter);
  while (state) {
    a = list_store_get_int(d->list, &iter, 0); 

    ptr = list_store_get_string(d->list, &iter, 1); 
    x = strtod(ptr, &endptr);

    ptr = list_store_get_string(d->list, &iter, 2); 
    y = strtod(ptr, &endptr);

    if (move == NULL)
      move = arraynew(sizeof(int));

    if (movex == NULL)
      movex = arraynew(sizeof(double));

    if (movey == NULL)
      movey = arraynew(sizeof(double));

    movenum = arraynum(move);
    if (arraynum(movex) < movenum)
      movenum = arraynum(movex);

    if (arraynum(movey) < movenum)
      movenum = arraynum(movey);

    for (j = 0; j < movenum; j++) {
      line = *(int *) arraynget(move, j);

      if (line == a)
	break;
    }

    if (j == movenum) {
      arrayadd(move, &a);
      arrayadd(movex, &x);
      arrayadd(movey, &y);
    }

    state = list_store_iter_next(d->list, &iter);
  }
  putobj(d->Obj, "move_data", d->Id, move);
  putobj(d->Obj, "move_data_x", d->Id, movex);
  putobj(d->Obj, "move_data_y", d->Id, movey);
  d->ret = ret;
}

void
FileMoveDialog(struct FileMoveDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMoveDialogSetup;
  data->CloseWindow = FileMoveDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
FileMaskDialogSetupItem(GtkWidget *w, struct FileMaskDialog *d, int id)
{
  int line, j, masknum;
  struct narray *mask;
  GtkTreeIter iter;

  list_store_clear(d->list);
  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if ((masknum = arraynum(mask)) > 0) {
    for (j = 0; j < masknum; j++) {
      line = *(int *) arraynget(mask, j);
      list_store_append(d->list, &iter);
      list_store_set_int(d->list, &iter, 0, line);
    }
  }
}

static void
FileMaskDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileMaskDialog *d;
  int a;
  const char *buf;
  char *endptr;
  GtkTreeIter iter;

  d = (struct FileMaskDialog *) client_data;
  buf = gtk_entry_get_text(GTK_ENTRY(d->line));
  if (buf[0] == '\0') return;

  a = strtol(buf, &endptr, 10);
  if (endptr[0] == '\0') {
    list_store_append(d->list, &iter);
    list_store_set_int(d->list, &iter, 0, a);
  }

  gtk_entry_set_text(GTK_ENTRY(d->line), "");
}

static gboolean
mask_dialog_key_pressed(GtkWidget *w, GdkEventKey *e, gpointer user_data)
{
  struct FileMaskDialog *d;

  d = (struct FileMaskDialog *) user_data;
  if (e->keyval != GDK_Return)
    return FALSE;

  FileMaskDialogAdd(NULL, d);

  return TRUE;
}

static void
FileMaskDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *hbox, *vbox;
  struct FileMaskDialog *d;
  n_list_store list[] = {
    {_("Line No."), G_TYPE_INT, TRUE, FALSE, NULL, FALSE},
  };

  d = (struct FileMaskDialog *) data;

  if (makewidget) {
    hbox = gtk_hbox_new(FALSE, 4);
    vbox = gtk_vbox_new(FALSE, 4);

    w = create_text_entry(TRUE, FALSE);
    g_signal_connect(w, "key-press-event", G_CALLBACK(mask_dialog_key_pressed), d);

    item_setup(vbox, w, _("_Line:"), FALSE);
    d->line = w;

    swin = gtk_scrolled_window_new(NULL, NULL);
    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->list = w;
    gtk_container_add(GTK_CONTAINER(swin), w);

    w = gtk_button_new_from_stock(GTK_STOCK_ADD);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogAdd), d);

    w = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_remove_selected_cb), d->list);

    w = gtk_button_new_from_stock(GTK_STOCK_SELECT_ALL);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), 300, 400);
  }

  FileMaskDialogSetupItem(wi, d, d->Id);
}

static void
FileMaskDialogClose(GtkWidget *w, void *data)
{
  struct FileMaskDialog *d;
  int ret, j, masknum, line, a;
  struct narray *mask;
  GtkTreeIter iter;
  gboolean state;

  d = (struct FileMaskDialog *) data;
  if (d->ret != IDOK)
    return;

  ret = d->ret;
  d->ret = IDLOOP;

  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if (mask != NULL) {
    putobj(d->Obj, "mask", d->Id, NULL);
    mask = NULL;
  }
  
  state = list_store_get_iter_first(d->list, &iter);
  while (state) {
    a = list_store_get_int(d->list, &iter, 0); 
    if (mask == NULL)
      mask = arraynew(sizeof(int));

    masknum = arraynum(mask);
    for (j = 0; j < masknum; j++) {
      line = *(int *) arraynget(mask, j);
      if (line == a)
	break;
    }
    if (j == masknum)
      arrayadd(mask, &a);

    state = list_store_iter_next(d->list, &iter);
  }
  putobj(d->Obj, "mask", d->Id, mask);
  d->ret = ret;
}

void
FileMaskDialog(struct FileMaskDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMaskDialogSetup;
  data->CloseWindow = FileMaskDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
FileLoadDialogSetupItem(GtkWidget *w, struct FileLoadDialog *d, int id)
{
  char *ifs, *s;
  int i;

  SetTextFromObjField(d->headskip, d->Obj, id, "head_skip");
  SetTextFromObjField(d->readstep, d->Obj, id, "read_step");
  SetTextFromObjField(d->finalline, d->Obj, id, "final_line");
  SetTextFromObjField(d->remark, d->Obj, id, "remark");
  sgetobjfield(d->Obj, id, "ifs", NULL, &ifs, FALSE, FALSE, FALSE);
  s = nstrnew();
  for (i = 0; i < strlen(ifs); i++) {
    if (ifs[i] == '\t') {
      s = nstrccat(s, '\\');
      s = nstrccat(s, 't');
    } else if (ifs[i] == '\\') {
      s = nstrccat(s, '\\');
      s = nstrccat(s, '\\');
    } else
      s = nstrccat(s, ifs[i]);
  }
  gtk_entry_set_text(GTK_ENTRY(d->ifs), s);
  memfree(s);
  memfree(ifs);
  SetToggleFromObjField(d->csv, d->Obj, id, "csv");
}

static void
FileLoadDialogCopy(struct FileLoadDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FileCB)) != -1)
    FileLoadDialogSetupItem(d->widget, d, sel);
}

static void
FileLoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox;
  struct FileLoadDialog *d;

  d = (struct FileLoadDialog *) data;

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Head skip:"), TRUE);
    d->headskip = w;

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Read step:"), TRUE);
    d->readstep = w;

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Final line:"), TRUE);
    d->finalline = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Remark:"), TRUE);
    d->remark = w;

    w = create_text_entry(TRUE, TRUE);

    item_setup(hbox, w, _("_Ifs:"), TRUE);
    d->ifs = w;

    w = gtk_check_button_new_with_mnemonic(_("_CSV"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->csv = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }

  FileLoadDialogSetupItem(wi, d, d->Id);
}

static void
FileLoadDialogClose(GtkWidget *w, void *data)
{
  struct FileLoadDialog *d;
  int ret;
  const char *ifs;
  char *s;
  int i;

  d = (struct FileLoadDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileLoadDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromText(d->headskip, d->Obj, d->Id, "head_skip"))
    return;

  if (SetObjFieldFromText(d->readstep, d->Obj, d->Id, "read_step"))
    return;

  if (SetObjFieldFromText(d->finalline, d->Obj, d->Id, "final_line"))
    return;

  if (SetObjFieldFromText(d->remark, d->Obj, d->Id, "remark"))
    return;

  ifs = gtk_entry_get_text(GTK_ENTRY(d->ifs));
  s = nstrnew();

  for (i = 0; i < strlen(ifs); i++) {
    if ((ifs[i] == '\\') && (ifs[i + 1] == 't')) {
      s = nstrccat(s, 0x09);
      i++;
    } else if (ifs[i] == '\\') {
      s = nstrccat(s, '\\');
      i++;
    } else {
      s = nstrccat(s, ifs[i]);
    }
  }

  if (sputobjfield(d->Obj, d->Id, "ifs", s) != 0) {
    memfree(s);
    return;
  }

  memfree(s);

  if (SetObjFieldFromToggle(d->csv, d->Obj, d->Id, "csv"))
    return;

  d->ret = ret;
}

void
FileLoadDialog(struct FileLoadDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileLoadDialogSetup;
  data->CloseWindow = FileLoadDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
FileMathDialogSetupItem(GtkWidget *w, struct FileMathDialog *d, int id)
{
  SetTextFromObjField(d->xsmooth, d->Obj, id, "smooth_x");
  SetTextFromObjField(d->ysmooth, d->Obj, id, "smooth_y");
  SetTextFromObjField(d->xmath, d->Obj, id, "math_x");
  SetTextFromObjField(d->ymath, d->Obj, id, "math_y");
  SetTextFromObjField(d->fmath, d->Obj, id, "func_f");
  SetTextFromObjField(d->gmath, d->Obj, id, "func_g");
  SetTextFromObjField(d->hmath, d->Obj, id, "func_h");
}

static void
FileMathDialogCopy(struct FileMathDialog *d)
{
  int sel;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    FileMathDialogSetupItem(d->widget, d, sel);
  }
}

static gboolean
func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data)
{
  struct FileMathDialog *d;

  d = (struct FileMathDialog *) user_data;
  gtk_entry_set_completion(GTK_ENTRY(d->fmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->gmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->hmath), NULL);

  gtk_entry_set_completion(GTK_ENTRY(w), NgraphApp.func_list);

  return FALSE;
}

static void
FileMathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox;
  struct FileMathDialog *d;

  d = (struct FileMathDialog *) data;
  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    vbox = gtk_vbox_new(FALSE, 4);
    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_X smooth:"), FALSE);
    d->xsmooth = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_X math:"), TRUE);
    d->xmath = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Y smooth:"), FALSE);
    d->ysmooth = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Y math:"), TRUE);
    d->ymath = w;

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_F(X,Y,Z):", TRUE);
    d->fmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_G(X,Y,Z):", TRUE);
    d->gmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(TRUE, TRUE);
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), d);
    item_setup(hbox, w, "_H(X,Y,Z):", TRUE);
    d->hmath = w;
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
  }

  FileMathDialogSetupItem(wi, d, d->Id);

  gtk_entry_set_completion(GTK_ENTRY(d->xmath), NgraphApp.x_math_list);
  gtk_entry_set_completion(GTK_ENTRY(d->ymath), NgraphApp.y_math_list);
}

static void
FileMathDialogClose(GtkWidget *w, void *data)
{
  struct FileMathDialog *d;
  int ret;
  const char *s;

  d = (struct FileMathDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    FileMathDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  s = gtk_entry_get_text(GTK_ENTRY(d->ymath));
  entry_completion_append(NgraphApp.y_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->xmath));
  entry_completion_append(NgraphApp.x_math_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->fmath));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->gmath));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_entry_get_text(GTK_ENTRY(d->hmath));
  entry_completion_append(NgraphApp.func_list, s);

  if (SetObjFieldFromText(d->xsmooth, d->Obj, d->Id, "smooth_x"))
    return;

  if (SetObjFieldFromText(d->xmath, d->Obj, d->Id, "math_x"))
    return;

  if (SetObjFieldFromText(d->ysmooth, d->Obj, d->Id, "smooth_y"))
    return;

  if (SetObjFieldFromText(d->ymath, d->Obj, d->Id, "math_y"))
    return;

  if (SetObjFieldFromText(d->fmath, d->Obj, d->Id, "func_f"))
    return;

  if (SetObjFieldFromText(d->gmath, d->Obj, d->Id, "func_g"))
    return;

  if (SetObjFieldFromText(d->hmath, d->Obj, d->Id, "func_h"))
    return;

  gtk_entry_set_completion(GTK_ENTRY(d->ymath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->xmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->fmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->gmath), NULL);
  gtk_entry_set_completion(GTK_ENTRY(d->hmath), NULL);

  d->ret = ret;
}

void
FileMathDialog(struct FileMathDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileMathDialogSetup;
  data->CloseWindow = FileMathDialogClose;
  data->Obj = obj;
  data->Id = id;
}

static void
MarkDialogCB(GtkWidget *w, gpointer client_data)
{
  int i;
  struct MarkDialog *d;

  d = (struct MarkDialog *) client_data;

  if (! d->cb_respond)
    return;

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    if (w == d->toggle[i])
      break;
  }

  d->Type = i;
  d->ret = IDOK;
  gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
}

static void
MarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox,*vbox, *img;
  struct MarkDialog *d;
  int type;
#define COL 10

  d = (struct MarkDialog *) data;

  if (makewidget) {
    hbox = NULL;
    vbox = gtk_vbox_new(FALSE, 4);

    for (type = 0; type < MARK_TYPE_NUM; type++) {
      w = gtk_toggle_button_new();
      img = gtk_image_new_from_pixmap(NgraphApp.markpix[type], NULL);
      gtk_button_set_image(GTK_BUTTON(w), img);
      g_signal_connect(w, "clicked", G_CALLBACK(MarkDialogCB), d);
      d->toggle[type] = w;
      if (type % COL == 0) {
	if (hbox) {
	  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	}
	hbox = gtk_hbox_new(FALSE, 4);
      }
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    }
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    //    d->show_buttons = FALSE;
  }

  d->cb_respond = FALSE;
  for (type = 0; type < MARK_TYPE_NUM; type++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[type]), FALSE);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[d->Type]), TRUE);
  gtk_widget_grab_focus(d->toggle[d->Type]);
  d->cb_respond = TRUE;
}

static void
MarkDialogClose(GtkWidget *w, void *data)
{
}

void
MarkDialog(struct MarkDialog *data, int type)
{
  data->SetupWindow = MarkDialogSetup;
  data->CloseWindow = MarkDialogClose;
  data->Type = type;
}

static void
FileDialogSetupItemCommon(GtkWidget *w, struct FileDialog *d, int id)
{
  int i, j, a, lastinst;
  struct objlist *aobj;
  char *name, *valstr;
  GtkWidget *img;

  combo_box_clear(d->xaxis);
  combo_box_clear(d->yaxis);

  aobj = getobject("axis");

  lastinst = chkobjlastinst(aobj);
  for (j = 0; j <= lastinst; j++) {
    getobj(aobj, "group", j, 0, NULL, &name);
    if (name == NULL) {
      name = "";
    }
    combo_box_append_text(d->xaxis, name);
    combo_box_append_text(d->yaxis, name);
  }

  SetTextFromObjField(d->xcol, d->Obj, id, "x");

  sgetobjfield(d->Obj, id, "axis_x", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->xaxis, valstr + i);;
  memfree(valstr);

  SetTextFromObjField(d->ycol, d->Obj, id, "y");

  sgetobjfield(d->Obj, id, "axis_y", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  combo_box_entry_set_text(d->yaxis, valstr + i);;
  memfree(valstr);

  SetListFromObjField(d->type, d->Obj, id, "type");

  SetListFromObjField(d->curve, d->Obj, id, "interpolation");

  getobj(d->Obj, "mark_type", id, 0, NULL, &a);
  if (a < 0 || a >= MARK_TYPE_NUM)
    a = 0;

  img = gtk_image_new_from_pixmap(NgraphApp.markpix[a], NULL);
  gtk_button_set_image(GTK_BUTTON(d->mark_btn), img);

  MarkDialog(&(d->mark), a);

  SetComboList(d->size, CbMarkSize, CbMarkSizeNum);
  SetTextFromObjField(GTK_BIN(d->size)->child, d->Obj, id, "mark_size");

  SetComboList(d->width, CbLineWidth, CbLineWidthNum);
  SetTextFromObjField(GTK_BIN(d->width)->child, d->Obj, id, "line_width");

  SetStyleFromObjField(d->style, d->Obj, id, "line_style");

  SetListFromObjField(d->join, d->Obj, id, "line_join");

  SetTextFromObjField(d->miter, d->Obj, id, "line_miter_limit");

  SetToggleFromObjField(d->clip, d->Obj, id, "data_clip");

  set_color(d->col1, d->Obj, id, NULL);
  set_color2(d->col2, d->Obj, id);
}

static void
FileDialogSetupItem(GtkWidget *w, struct FileDialog *d, int file, int id)
{
  int i;
  char *valstr;

  FileDialogSetupItemCommon(w, d, id);

  if (file) {
    SetTextFromObjField(d->file, d->Obj, id, "file");
  }

  if (id == d->Id) {
    sgetobjfield(d->Obj, id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
    for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
    if (valstr[i] == ':')
      i++;
    gtk_label_set_text(GTK_LABEL(d->fitid), valstr + i);
  }
}

static void
FileDialogAxis(GtkWidget *w, gpointer client_data)
{
  char buf[10];
  int a;

  a = combo_box_get_active(w);

  if (a < 0)
    return;

  snprintf(buf, sizeof(buf), "%d", a);
  combo_box_entry_set_text(w, buf);
}

static void
FileDialogMark(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  GtkWidget *img;

  d = (struct FileDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  img = gtk_image_new_from_pixmap(NgraphApp.markpix[d->mark.Type], NULL);
  gtk_button_set_image(GTK_BUTTON(w), img);
}

static void
FileDialogFit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  struct objlist *fitobj, *obj;
  char *fit, *inst;
  int i, idnum, fitid = 0, fitoid, ret;
  struct narray iarray;
  char *valstr;

  d = (struct FileDialog *) client_data;

  if ((fitobj = chkobject("fit")) == NULL)
    return;

  if (getobj(d->Obj, "fit", d->Id, 0, NULL, &fit) == -1)
    return;

  if (fit != NULL) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &obj, &iarray, FALSE, NULL))
      return;

    idnum = arraynum(&iarray);
    if ((obj != fitobj) || (idnum < 1)) {
      if (putobj(d->Obj, "fit", d->Id, NULL) == -1) {
	arraydel(&iarray);
	return;
      }
    } else {
      fitid = *(int *) arraylast(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL) {
    fitid = newobj(fitobj);
    inst = getobjinst(fitobj, fitid);

    _getobj(fitobj, "oid", inst, &fitoid);

    if ((fit = mkobjlist(fitobj, NULL, fitoid, NULL, TRUE)) == NULL)
      return;

    if (putobj(d->Obj, "fit", d->Id, fit) == -1) {
      memfree(fit);
      return;
    }
  }

  FitDialog(&DlgFit, fitobj, fitid);
  ret = DialogExecute(d->widget, &DlgFit);

  if (ret == IDDELETE) {
    delobj(fitobj, fitid);
    putobj(d->Obj, "fit", d->Id, NULL);
  } else if (ret == IDOK) {
    combo_box_set_active(d->type, 19);
  }

  sgetobjfield(d->Obj, d->Id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':')
    i++;
  gtk_label_set_text(GTK_LABEL(d->fitid), valstr);
  memfree(valstr);
}

static void
FileDialogCopy(struct FileDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FileCB)) != -1)
    FileDialogSetupItem(d->widget, d, FALSE, sel);
}

static void
FileDialogCopyAll(struct FileDialog *d)
{
  int sel;
  int j, perm, type;
  char *field;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel == -1) 
    return;

  for (j = 0; j < chkobjfieldnum(d->Obj); j++) {
    field = chkobjfieldname(d->Obj, j);
    perm = chkobjperm(d->Obj, field);
    type = chkobjfieldtype(d->Obj, field);
    if ((strcmp2(field, "name") != 0) && (strcmp2(field, "file") != 0)
	&& (strcmp2(field, "fit") != 0)
	&& ((perm & NREAD) != 0) && ((perm & NWRITE) != 0)
	&& (type < NVFUNC))
      copyobj(d->Obj, field, d->Id, sel);
  }
  FitCopy(d->Obj, d->Id, sel);
  FileDialogSetupItem(d->widget, d, FALSE, d->Id);
  NgraphApp.Changed = TRUE;
}

static void
FileDialogOption(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  exeobj(d->Obj, "load_settings", d->Id, 0, NULL);
  FileDialogSetupItem(d->widget, d, FALSE, d->Id);
}

static void
FileDialogMath(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMathDialog(&DlgFileMath, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMath);
}

static void
FileDialogLoad(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileLoadDialog(&DlgFileLoad, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileLoad);
}

static void
FileDialogMask(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMaskDialog(&DlgFileMask, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMask);
}

static void
FileDialogMove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  FileMoveDialog(&DlgFileMove, d->Obj, d->Id);
  DialogExecute(d->widget, &DlgFileMove);
}

static void
FileDialogEdit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  const char *tmp;
  char *name;
  char *argv[3];
  pid_t pid;

  d = (struct FileDialog *) client_data;

  if (Menulocal.editor == NULL)
    return;

  argv[0] = Menulocal.editor;
  argv[2] = NULL;

  tmp = gtk_entry_get_text(GTK_ENTRY(d->file));

  if (tmp == NULL)
    return;

  name = strdup(tmp);
  if (name == NULL)
    return;

  if ((pid = fork()) >= 0) {
    argv[1] = name;
    if (pid) {
      arrayadd(&ChildList, &pid);
    } else {
      execvp(argv[0], argv);
      exit(1);
    }
  }
  free(name);
}

static void
FileDialogType(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int type;

  d = (struct FileDialog *) client_data;

  gtk_widget_set_sensitive(d->mark_btn, TRUE);
  gtk_widget_set_sensitive(d->curve, TRUE);
  gtk_widget_set_sensitive(d->col1, TRUE);
  gtk_widget_set_sensitive(d->col2, TRUE);
  gtk_widget_set_sensitive(d->style, TRUE);
  gtk_widget_set_sensitive(d->width, TRUE);
  gtk_widget_set_sensitive(d->size, TRUE);
  gtk_widget_set_sensitive(d->miter, TRUE);
  gtk_widget_set_sensitive(d->join, TRUE);

  type = combo_box_get_active(w);

  switch (type) {
  case 0:
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 1:
  case 2:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    break;
  case 3:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    break;
  case 4:
  case 6:
  case 8:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 5:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 7:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 9:
  case 10:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 11:
  case 12:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    break;
  case 13:
  case 14:
  case 17:
  case 18:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 15:
  case 16:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->miter, FALSE);
    gtk_widget_set_sensitive(d->join, FALSE);
    break;
  case 19:
    gtk_widget_set_sensitive(d->mark_btn, FALSE);
    gtk_widget_set_sensitive(d->curve, FALSE);
    gtk_widget_set_sensitive(d->col2, FALSE);
    gtk_widget_set_sensitive(d->size, FALSE);
    break;
  }
}

static void
FileDialogSetupCommon(GtkWidget *wi, struct FileDialog *d)
{
  GtkWidget *w, *hbox, *vbox, *vbox2, *frame;

  vbox = gtk_vbox_new(FALSE, 4);

  vbox2 = gtk_vbox_new(FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);

  w = create_text_entry(TRUE, TRUE);

  item_setup(hbox, w, _("_X column:"), TRUE);
  d->xcol = w;

  w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

  item_setup(hbox, w, _("_X axis:"), TRUE);
  d->xaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);


  hbox = gtk_hbox_new(FALSE, 4);

  w = create_text_entry(TRUE, TRUE);

  item_setup(hbox, w, _("_Y column:"), TRUE);
  d->ycol = w;

  w = combo_box_entry_create();
    gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);

  item_setup(hbox, w, _("_Y axis:"), TRUE);
  d->yaxis = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogAxis), d);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 4);
  d->comment_box = hbox;

  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);


  hbox = gtk_hbox_new(FALSE, 4);

  w = combo_box_create();
  item_setup(hbox, w, _("_Type:"), FALSE);
  d->type = w;
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogType), d);


  vbox2 = gtk_vbox_new(FALSE, 4);
  w = gtk_button_new();
  item_setup(vbox2, w, _("_Mark:"), FALSE);
  d->mark_btn = w;
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMark), d);

  w = combo_box_create();
  item_setup(vbox2, w, _("_Curve:"), FALSE);
  d->curve = w;

  d->fit_box = vbox2;

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), vbox2);


  vbox2 = gtk_vbox_new(FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox2), frame, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  w = create_color_button(wi);
  item_setup(hbox, w, _("_Color 1:"), FALSE);
  d->col1 = w;

  w = create_color_button(wi);
  item_setup(hbox, w, _("_Color 2:"), FALSE);
  d->col2 = w;

  w = gtk_check_button_new_with_mnemonic(_("_Clip"));
  d->clip = w;
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);


  vbox2 = gtk_vbox_new(FALSE, 4);

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  item_setup(vbox2, w, _("Line _Style:"), TRUE);
  d->style = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  item_setup(vbox2, w, _("_Line Width:"), TRUE);
  d->width = w;

  w = combo_box_entry_create();
  gtk_widget_set_size_request(w, NUM_ENTRY_WIDTH * 1.5, -1);
  item_setup(vbox2, w, _("_Size:"), TRUE);
  d->size = w;

  w = create_text_entry(TRUE, TRUE);
  item_setup(vbox2, w, _("_Miter:"), TRUE);
  d->miter = w;

  w = combo_box_create();
  item_setup(vbox2, w, _("_Join:"), TRUE);
  d->join = w;

  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);


  vbox2 = gtk_vbox_new(FALSE, 4);

  w = gtk_button_new_with_mnemonic(_("_Math"));
  gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 4);
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMath), d);

  w = gtk_button_new_with_mnemonic(_("_Load"));
  gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 4);
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogLoad), d);

  d->button_box = vbox2;

  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 4);
}

static void
FileDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *swin, *view;
  struct FileDialog *d;
  int line, rcode;
  char title[20], *argv[2], *s;

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   _("_Apply all"), IDFAPPLY,
			   GTK_STOCK_CLOSE, IDDELETE,
			   GTK_STOCK_COPY, IDCOPY,
			   _("_Copy all"), IDCOPYALL,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_File:"), TRUE);
    d->file = w;

    w = gtk_button_new_with_mnemonic(_("_Load settings"));
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    d->load_settings = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogOption), d);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
    FileDialogSetupCommon(wi, d);



    swin = gtk_scrolled_window_new(NULL, NULL);
    view = gtk_text_view_new_with_buffer(NULL);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    d->comment = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_container_add(GTK_CONTAINER(swin), view);

    w = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(d->comment_box), w, TRUE, TRUE, 4);


    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_button_new_with_mnemonic(_("_Fit"));
    d->fit = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);

    w = gtk_label_new("");
    d->fitid = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->fit_box), hbox, FALSE, FALSE, 4);


    w = gtk_button_new_with_mnemonic(_("_Mask"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMask), d);

    w = gtk_button_new_with_mnemonic(_("_Move"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMove), d);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    gtk_box_pack_start(GTK_BOX(d->button_box), w, FALSE, FALSE, 4);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogEdit), d);
  }

  line = 10;
  argv[0] = (char *) &line;
  argv[1] = NULL;
  rcode = getobj(d->Obj, "head_lines", d->Id, 1, argv, &s);
  if (s) {
    gboolean valid;
    const gchar *ptr;

    valid = g_utf8_validate(s, -1, &ptr);
    if (valid) {
      gtk_text_buffer_set_text(d->comment, s, -1);
    } else {
      gtk_text_buffer_set_text(d->comment, _("This file contain invalid UTF-8 strings."), -1);
    }
  }
  FileDialogSetupItem(wi, d, TRUE, d->Id);

  /*
  if (makewidget)
    d->widget = NULL;
  */
}

static int
FileDialogCloseCommon(GtkWidget *w, struct FileDialog *d)
{
  const char *s;
  char *buf;
  int len;

  if (SetObjFieldFromText(d->xcol, d->Obj, d->Id, "x"))
    return TRUE;

  s = combo_box_entry_get_text(d->xaxis);

  if (s)
    len = strlen(s);
  else
    len = 0;

  len += 6;
  buf = (char *) memalloc(len);
  if (buf) {
    strcpy(buf, "axis:");
    if (s)
      strcat(buf, s);
    if (sputobjfield(d->Obj, d->Id, "axis_x", buf) != 0) {
      memfree(buf);
      return TRUE;
    }
    memfree(buf);
  }

  if (SetObjFieldFromText(d->ycol, d->Obj, d->Id, "y"))
    return TRUE;

  s = combo_box_entry_get_text(d->yaxis);

  if (s)
    len = strlen(s);
  else
    len = 0;

  len += 6;
  buf = (char *) memalloc(len);
  if (buf) {
    strcpy(buf, "axis:");
    if (s)
      strcat(buf, s);
    if (sputobjfield(d->Obj, d->Id, "axis_y", buf) != 0) {
      memfree(buf);
      return TRUE;
    }
    memfree(buf);
  }

  if (SetObjFieldFromList(d->type, d->Obj, d->Id, "type"))
    return TRUE;

  if (SetObjFieldFromList(d->curve, d->Obj, d->Id, "interpolation"))
    return TRUE;

  if (putobj(d->Obj, "mark_type", d->Id, &(d->mark.Type)) == -1)
    return TRUE;

  if (SetObjFieldFromText(GTK_BIN(d->size)->child, d->Obj, d->Id, "mark_size"))
    return TRUE;

  if (SetObjFieldFromText(GTK_BIN(d->width)->child, d->Obj, d->Id, "line_width"))
    return TRUE;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "line_style"))
    return TRUE;

  if (SetObjFieldFromList(d->join, d->Obj, d->Id, "line_join"))
    return TRUE;

  if (SetObjFieldFromText(d->miter, d->Obj, d->Id, "line_miter_limit"))
    return TRUE;

  if (SetObjFieldFromToggle(d->clip, d->Obj, d->Id, "data_clip"))
    return TRUE;

  if (putobj_color(d->col1, d->Obj, d->Id, NULL))
    return TRUE;

  if (putobj_color2(d->col2, d->Obj, d->Id))
    return TRUE;

  return FALSE;
}

static void
FileDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;
  const char *s;
  char *buf;
  int len;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
  case IDFAPPLY:
    break;
  case IDCOPY:
    FileDialogCopy(d);
    d->ret = IDLOOP;
    return;
  case IDCOPYALL:
    FileDialogCopyAll(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromText(d->file, d->Obj, d->Id, "file"))
    return;

  if (FileDialogCloseCommon(w, d))
    return;

  if (SetObjFieldFromText(d->xcol, d->Obj, d->Id, "x"))
    return;

  s = combo_box_entry_get_text(d->xaxis);

  if (s)
    len = strlen(s);
  else
    len = 0;

  len += 6;
  buf = (char *) memalloc(len);
  if (buf) {
    strcpy(buf, "axis:");
    if (s)
      strcat(buf, s);
    if (sputobjfield(d->Obj, d->Id, "axis_x", buf) != 0) {
      memfree(buf);
      return;
    }
    memfree(buf);
  }

  if (SetObjFieldFromText(d->ycol, d->Obj, d->Id, "y"))
    return;

  s = combo_box_entry_get_text(d->yaxis);

  if (s)
    len = strlen(s);
  else
    len = 0;

  len += 6;
  buf = (char *) memalloc(len);
  if (buf) {
    strcpy(buf, "axis:");
    if (s)
      strcat(buf, s);
    if (sputobjfield(d->Obj, d->Id, "axis_y", buf) != 0) {
      memfree(buf);
      return;
    }
    memfree(buf);
  }

  if (SetObjFieldFromList(d->type, d->Obj, d->Id, "type"))
    return;

  if (SetObjFieldFromList(d->curve, d->Obj, d->Id, "interpolation"))
    return;

  if (putobj(d->Obj, "mark_type", d->Id, &(d->mark.Type)) == -1)
    return;

  if (SetObjFieldFromText(GTK_BIN(d->size)->child, d->Obj, d->Id, "mark_size"))
    return;

  if (SetObjFieldFromText(GTK_BIN(d->width)->child, d->Obj, d->Id, "line_width"))
    return;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "line_style"))
    return;

  if (SetObjFieldFromList(d->join, d->Obj, d->Id, "line_join"))
    return;

  if (SetObjFieldFromText(d->miter, d->Obj, d->Id, "line_miter_limit"))
    return;

  if (SetObjFieldFromToggle(d->clip, d->Obj, d->Id, "data_clip"))
    return;

  putobj_color(d->col1, d->Obj, d->Id, NULL);
  putobj_color2(d->col2, d->Obj, d->Id);

  d->ret = ret;
}

void
FileDialog(void *data, struct objlist *obj, int id, int candel)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  d->SetupWindow = FileDialogSetup;
  d->CloseWindow = FileDialogClose;
  d->Obj = obj;
  d->Id = id;
}

static void
FileDialogDefSetupItem(GtkWidget *w, struct FileDialog *d, int id)
{
  FileDialogSetupItemCommon(w, d, id);
}

static void
FileDefDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  if (makewidget) {
    FileDialogSetupCommon(wi, d);
  }
  FileDialogDefSetupItem(wi, d, d->Id);
}

static void
FileDefDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (FileDialogCloseCommon(w, d))
    return;

  d->ret = ret;
}

void
FileDefDialog(struct FileDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileDefDialogSetup;
  data->CloseWindow = FileDefDialogClose;
  data->Obj = obj;
  data->Id = id;
}

void
CmFileHistory(GtkWidget *w, gpointer client_data)
{
  int i, fil, num;
  char **data;
  struct narray *datafilelist;
  int ret;
  char *name;
  int len, id;
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (w == NgraphApp.fhistory[i]) break;
  }
  fil = i;
  datafilelist = Menulocal.datafilelist;
  num = arraynum(datafilelist);
  data = (char **) arraydata(datafilelist);
  if ((fil < 0) || (fil >= num) || (data[fil] == NULL))
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    len = strlen(data[fil]);
    if ((name = (char *) memalloc(len + 1)) != NULL) {
      strcpy(name, data[fil]);
      putobj(obj, "file", id, name);
      FileDialog(&DlgFile, obj, id, 0);
      ret = DialogExecute(TopLevel, &DlgFile);
      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	FitDel(obj, id);
	delobj(obj, id);
      } else {
	NgraphApp.Changed = TRUE;
      }
    }
  }
  AddDataFileList(data[fil]);
  FileWinUpdate(TRUE);
}

void
CmFileNew(void)
{
  char *name, *file;
  int id, ret;
  struct objlist *obj;


  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if (nGetOpenFileName(TopLevel, _("Data new"), NULL, NULL,
		       NULL, &file, NULL, FALSE,
		       Menulocal.changedirectory) == IDOK)
  {
    if ((id = newobj(obj)) >= 0) {
      name = nstrdup(file);
      if (name == NULL) {
	free(file);
	return;
      }
      changefilename(name);
      AddDataFileList(name);
      putobj(obj, "file", id, name);
      FileDialog(&DlgFile, obj, id, 0);
      ret = DialogExecute(TopLevel, &DlgFile);
      if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	FitDel(obj, id);
	delobj(obj, id);
      } else
	NgraphApp.Changed = TRUE;
      FileWinUpdate(TRUE);
    }
    free(file);
  }
}


void
CmFileOpen(void)
{
  int id, id0, ids, ide, j, perm, type, ret;
  char *field, *name, *tmp;;
  char **file = NULL, **ptr;
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;

  if ((obj = chkobject("file")) == NULL)
    return;

  ids = -1;
  ret = nGetOpenFileNameMulti(TopLevel, _("Data open"), NULL,
			      &(Menulocal.fileopendir), NULL,
			      &file, NULL, Menulocal.changedirectory);

  if (ret == IDOK && file) {
    for (ptr = file; *ptr; ptr++) {
      name = *ptr;
      id = newobj(obj);
      if (id >= 0) {
	if (ids == -1)
	  ids = id;
	tmp = nstrdup(name);
	changefilename(tmp);
	AddDataFileList(tmp);
	putobj(obj, "file", id, tmp);
      }
      free(name);
    }
    free(file);
  }

  FileWinUpdate(TRUE);

  if (ids != -1) {
    ide = chkobjlastinst(obj);
    id0 = -1;
    id = ids;
    while (id <= ide) {
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
	id++;
      } else {
	FileDialog(&DlgFile, obj, id, 0);
	ret = DialogExecute(TopLevel, &DlgFile);
	if ((ret == IDDELETE) || (ret == IDCANCEL)) {
	  FitDel(obj, id);
	  delobj(obj, id);
	  ide--;
	} else if (ret == IDFAPPLY) {
	  id0 = id;
	  id++;
	  NgraphApp.Changed = TRUE;
	} else {
	  id++;
	  NgraphApp.Changed = TRUE;
	}
      }
      FileWinUpdate(TRUE);
    }
  }
}

void
CmFileClose(void)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int *array, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      FitDel(obj, array[i]);
      delobj(obj, array[i]);
      NgraphApp.Changed = TRUE;
    }
    FileWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmFileUpdate(void)
{
  struct objlist *obj;
  int i, j;
  int *array, num;
  int id0, perm, type, ret;
  char *field;
  struct narray farray;
  int last;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  last = chkobjlastinst(obj);
  if (last == -1)
    return;
  else if (last == 0) {
    arrayinit(&farray, sizeof(int));
    arrayadd(&farray, &last);
    ret = IDOK;
  } else {
    SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
    ret = DialogExecute(TopLevel, &DlgSelect);
  }
  if (ret == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    id0 = -1;
    for (i = 0; i < num; i++) {
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
	    copyobj(obj, field, array[i], array[id0]);
	}
	FitCopy(obj, array[i], array[id0]);
      } else {
	FileDialog(&DlgFile, obj, array[i], 0);
	ret = DialogExecute(TopLevel, &DlgFile);
	if (ret == IDDELETE) {
	  FitDel(obj, array[i]);
	  delobj(obj, array[i]);
	  for (j = i + 1; j < num; j++)
	    array[j]--;
	} else if (ret == IDFAPPLY)
	  id0 = i;
	if (ret != IDCANCEL)
	  NgraphApp.Changed = TRUE;
      }
    }
    FileWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmFileEdit(void)
{
  struct objlist *obj;
  int i;
  char *name;
  char *argv[3];
  pid_t pid;
  int last;

  if (Menulock || GlobalLock)
    return;
  if (Menulocal.editor == NULL)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  last = chkobjlastinst(obj);
  if (last == -1)
    return;
  else if (last == 0)
    i = 0;
  else {
    CopyDialog(&DlgCopy, obj, -1, FileCB);
    if (DialogExecute(TopLevel, &DlgCopy) == IDOK)
      i = DlgCopy.sel;
    else
      return;
  }
  if (i < 0)
    return;
  argv[0] = Menulocal.editor;
  argv[2] = NULL;
  if (getobj(obj, "file", i, 0, NULL, &name) == -1)
    return;
  if (name != NULL) {
    argv[1] = name;
    if ((pid = fork()) >= 0) {
      if (pid == 0) {
	execvp(argv[0], argv);
	exit(1);
      } else
	arrayadd(&ChildList, &pid);
    }
  }
}

void
CmFileOpenB(GtkWidget *w, gpointer p)
{
  CmFileOpen();
}

void
CmFileMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdFileNew:
    CmFileNew();
    break;
  case MenuIdFileOpen:
    CmFileOpen();
    break;
  case MenuIdFileUpdate:
    CmFileUpdate();
    break;
  case MenuIdFileClose:
    CmFileClose();
    break;
  case MenuIdFileEdit:
    CmFileEdit();
    break;
  }
}

void
CmOptionFileDef(void)
{
  struct objlist *obj;
  int id;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if ((id = newobj(obj)) >= 0) {
    FileDefDialog(&DlgFileDef, obj, id);
    if (DialogExecute(TopLevel, &DlgFileDef) == IDOK) {
      if (CheckIniFile()) {
	exeobj(obj, "save_config", id, 0, NULL);
      }
    }
    delobj(obj, id);
    UpdateAll2();
  }
}

static void
FileWinFileEdit(struct SubWin *d)
{
  int sel;
  char *argv[3], *name;
  pid_t pid;

  if (Menulock || GlobalLock)
    return;

  if (Menulocal.editor == NULL)
    return;

  sel = d->select;

  if (sel < 0 || sel > d->num)
    return;

  argv[0] = Menulocal.editor;
  argv[2] = NULL;

  if (getobj(d->obj, "file", sel, 0, NULL, &name) == -1)
    return;

  if (name == NULL)
    return;

  argv[1] = name;
  pid = fork();
  if (pid < 0)
    return;

  if (pid == 0) {
    execvp(argv[0], argv);
    exit(1);
  } else {
    arrayadd(&ChildList, &pid);
  }
}

static void
file_edit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileEdit(d);
}

static void
FileWinFileDelete(struct SubWin *d)
{
  int sel, update;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);

  if ((sel >= 0) && (sel <= d->num)) {
    FitDel(d->obj, sel);
    delobj(d->obj, sel);
    d->num--;
    update = FALSE;
    if (d->num < 0) {
      d->select = -1;
      update = TRUE;
    } else if (sel > d->num) {
      d->select = d->num;
    } else {
      d->select = sel;
    }
    FileWinUpdate(update);
    NgraphApp.Changed = TRUE;
  }
}

static void
file_delete_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileDelete(d);
}

static int
file_obj_copy(struct SubWin *d)
{
  int sel;
  int j, id, perm, type;
  char *field;

  if (Menulock || GlobalLock)
    return -1;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);

  if ((sel < 0) || (sel > d->num))
    return -1;

  id = newobj(d->obj);

  if (id < 0)
    return -1;

  for (j = 0; j < chkobjfieldnum(d->obj); j++) {
    field = chkobjfieldname(d->obj, j);
    perm = chkobjperm(d->obj, field);
    type = chkobjfieldtype(d->obj, field);
    if ((strcmp2(field, "name") != 0) 
	&& (strcmp2(field, "fit") != 0) && ((perm & NREAD) != 0)
	&& ((perm & NWRITE) != 0) && (type < NVFUNC)) {
      copyobj(d->obj, field, id, sel);
    }
  }
  FitCopy(d->obj, id, sel);
  d->num++;
  NgraphApp.Changed = TRUE;

  return id;
}

static void
FileWinFileCopy(struct SubWin *d)
{
  d->select = file_obj_copy(d);
  FileWinUpdate(FALSE);
}

static void
file_copy_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileCopy(d);
}

static void
FileWinFileCopy2(struct SubWin *d)
{
  int id, sel, j;

  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  id = file_obj_copy(d);

  if (id < 0) {
    d->select = sel;
    FileWinUpdate(TRUE);
    return;
  }

  for (j = d->num; j > sel + 1; j--) {
    moveupobj(d->obj, j);
  }

  d->select = sel + 1;
  FileWinUpdate(FALSE);
}

static void
file_copy2_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileCopy2(d);
}

static void
FileWinFileUpdate(struct SubWin *d)
{
  int sel, ret;

  if (Menulock || GlobalLock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
 
  if ((sel >= 0) && (sel <= d->num)) {
    d->setup_dialog(d->dialog, d->obj, sel, -1);
    d->select = sel;
    if ((ret = DialogExecute(TopLevel, d->dialog)) == IDDELETE) {
      FitDel(d->obj, sel);
      delobj(d->obj, sel);
      d->select = -1;
    }
    if (ret != IDCANCEL)
      NgraphApp.Changed = TRUE;
    d->update(FALSE);
  }
}

static void
FileWinFileDraw(struct SubWin *d)
{
  int i, sel;
  int hidden;
  
  if (Menulock || GlobalLock)
    return;

  sel = list_store_get_selected_index(GTK_WIDGET(d->text));

  if ((sel >= 0) && (sel <= d->num)) {
    for (i = 0; i <= d->num; i++) {
      if (i == sel)
	hidden = FALSE;
      else
	hidden = TRUE;
      putobj(d->obj, "hidden", i, &hidden);
    }
    d->select = sel;
  } else {
    hidden = FALSE;
    for (i = 0; i <= d->num; i++) {
      putobj(d->obj, "hidden", i, &hidden);
    }
    d->select = -1;
  }
  CmViewerDrawB(NULL, NULL);
  FileWinUpdate(FALSE);
}

static void
file_draw_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFileDraw(d);
}

void
FileWinUpdate(int clear)
{
  struct SubWin *d;

  d = &(NgraphApp.FileWin);

  FileWinExpose(NULL, NULL, NULL);

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID, d->select);
  }
}

static void
FileWinFit(struct SubWin *d)
{
  struct objlist *fitobj, *obj2;
  char *fit;
  int sel, idnum, fitid = 0, ret;
  struct narray iarray;

  if (Menulock || GlobalLock)
    return;

  sel = d->select;

  if (sel < 0 || sel > d->num)
    return;

  if ((fitobj = chkobject("fit")) == NULL)
    return;

  if (getobj(d->obj, "fit", sel, 0, NULL, &fit) == -1)
    return;

  if (fit != NULL) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &obj2, &iarray, FALSE, NULL)) {
      arraydel(&iarray);
      return;
    }
    idnum = arraynum(&iarray);

    if ((obj2 != fitobj) || (idnum < 1)) {
      if (putobj(d->obj, "fit", sel, NULL) == -1) {
	arraydel(&iarray);
	return;
      }
    } else {
      fitid = *(int *) arraylast(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL)
    return;

  FitDialog(&DlgFit, fitobj, fitid);

  ret = DialogExecute(TopLevel, &DlgFit);
  if (ret == IDDELETE) {
    delobj(fitobj, fitid);
    putobj(d->obj, "fit", sel, NULL);
  }
}

static void
file_fit_popup_func(GtkMenuItem *w, gpointer client_data)
{
  struct SubWin *d;

  d = (struct SubWin *) client_data;
  FileWinFit(d);
}


static GdkPixbuf *
draw_type_pixbuf(struct objlist *obj, int i)
{
  int j, k, ggc, dpi, fr, fg, fb, fr2, fg2, fb2,
    type, width = 40, height = 20, poly[14], marktype,
    intp, spcond, spnum, lockstate;
  double spx[7], spy[7], spz[7], spc[6][7], spc2[6];
  struct mxlocal mxsave;
  GdkPixmap *pix;
  GdkPixbuf *pixbuf;
  static GdkGC *gc = NULL;
  GdkColor color;

  lockstate = GlobalLock;
  GlobalLock = TRUE;

  if (gc == NULL) {
    gc = gdk_gc_new(TopLevel->window);
  }


  pix = gdk_pixmap_new(TopLevel->window, width, height, -1);

  color.red = Menulocal.bg_r * 0xff;
  color.green = Menulocal.bg_g * 0xff;
  color.blue = Menulocal.bg_b * 0xff;
  gdk_gc_set_rgb_fg_color(gc, &color);
  gdk_draw_rectangle(pix, gc, TRUE, 0, 0, width, height);

  getobj(obj, "type", i, 0, NULL, &type);
  getobj(obj, "R", i, 0, NULL, &fr);
  getobj(obj, "G", i, 0, NULL, &fg);
  getobj(obj, "B", i, 0, NULL, &fb);

  getobj(obj, "R2", i, 0, NULL, &fr2);
  getobj(obj, "G2", i, 0, NULL, &fg2);
  getobj(obj, "B2", i, 0, NULL, &fb2);

  dpi = height * 254;
  mxsaveGC(gc, pix, NULL, 0, 0, &mxsave, dpi, NULL);
  ggc = _GRAopen(chkobjectname(Menulocal.obj), "_output",
		 Menulocal.outputobj, Menulocal.inst, Menulocal.output, -1,
		 -1, -1, NULL, Mxlocal);
  if (ggc < 0) {
    _GRAclose(ggc);
    mxrestoreGC(&mxsave);
    return NULL;
  }
  GRAview(ggc, 0, 0, width, height, 0);

  switch (type) {
  case 0:
    getobj(obj, "mark_type", i, 0, NULL, &marktype);
    GRAlinestyle(ggc, 0, NULL, 0, 0, 0, 1000);
    GRAmark(ggc, marktype, 5, 5, 8, fr, fg, fb, fr2, fg2, fb2);
    break;
  case 1:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    GRAline(ggc, 0, 5, 10, 5);
    break;
  case 2:
    poly[0] = 0;
    poly[1] = 5;
    poly[2] = 3;
    poly[3] = 2;
    poly[4] = 7;
    poly[5] = 8;
    poly[6] = 10;
    poly[7] = 5;
    poly[8] = 7;
    poly[9] = 2;
    poly[10] = 3;
    poly[11] = 8;
    poly[12] = 0;
    poly[13] = 5;
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    GRAdrawpoly(ggc, 7, poly, 0);
    break;
  case 3:
    spx[0] = 0;
    spx[1] = 3;
    spx[2] = 7;
    spx[3] = 10;
    spx[4] = 7;
    spx[5] = 3;
    spx[6] = 0;
    spy[0] = 5;
    spy[1] = 2;
    spy[2] = 8;
    spy[3] = 5;
    spy[4] = 2;
    spy[5] = 8;
    spy[6] = 5;
    for (j = 0; j < 7; j++)
      spz[j] = j;
    getobj(obj, "interpolation", i, 0, NULL, &intp);
    if ((intp == 0) || (intp == 2)) {
      spcond = SPLCND2NDDIF;
      spnum = 4;
    } else {
      spcond = SPLCNDPERIODIC;
      spnum = 7;
    }
    spline(spz, spx, spc[0], spc[1], spc[2], spnum, spcond, spcond, 0, 0);
    spline(spz, spy, spc[3], spc[4], spc[5], spnum, spcond, spcond, 0, 0);
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (intp >= 2) {
      GRAmoveto(ggc, 12, 8);
      GRAtextstyle(ggc, "Times", 28, 0, 0);
      GRAouttext(ggc, "B");
    }
    GRAcurvefirst(ggc, 0, NULL, NULL, NULL, splinedif, splineint, NULL, spx[0], spy[0]);
    for (j = 0; j < spnum - 1; j++) {
      for (k = 0; k < 6; k++) {
	spc2[k] = spc[k][j];
      }
      if (!GRAcurve(ggc, spc2, spx[j], spy[j])) break;
    }
    break;
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
    GRAcolor(ggc, fr, fg, fb);
    if ((type == 4) || (type == 5))
      GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    else
      GRAlinestyle(ggc, 0, NULL, 1, 2, 0, 1000);
    spx[0] = 0;
    spy[0] = 7;
    spx[1] = 10;
    spy[1] = 3;
    if ((type == 4) || (type == 5))
      GRAline(ggc, spx[0], spy[0], spx[1], spy[1]);
    if (type == 5) {
      poly[0] = 9;
      poly[1] = 3;
      poly[2] = spx[1];
      poly[3] = spy[1];
      poly[4] = 10;
      poly[5] = 4;
      GRAdrawpoly(ggc, 3, poly, 0);
    }
    if ((type == 7) || (type == 8)) {
      if (type == 7)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, spx[0], spy[0], spx[1], spy[1], 1);
      if (type == 7)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == 6) || (type == 7)) {
      GRAline(ggc, spx[0], spy[0], spx[0], spy[1]);
      GRAline(ggc, spx[0], spy[1], spx[1], spy[1]);
      GRAline(ggc, spx[1], spy[1], spx[1], spy[0]);
      GRAline(ggc, spx[1], spy[0], spx[0], spy[0]);
    }
    break;
  case 9:
  case 10:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (type == 9) {
      GRAline(ggc, 0, 5, 10, 5);
      GRAline(ggc, 0, 3, 0, 7);
      GRAline(ggc, 10, 3, 10, 7);
    } else {
      GRAline(ggc, 5, 1, 5, 9);
      GRAline(ggc, 3, 1, 7, 1);
      GRAline(ggc, 3, 9, 7, 9);
    }
    break;
  case 11:
  case 12:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 0, 0, 1000);
    if (type == 11) {
      GRAmoveto(ggc, 0, 9);
      GRAlineto(ggc, 3, 9);
      GRAlineto(ggc, 3, 5);
      GRAlineto(ggc, 7, 5);
      GRAlineto(ggc, 7, 1);
      GRAlineto(ggc, 10, 1);
    } else {
      GRAmoveto(ggc, 0, 9);
      GRAlineto(ggc, 0, 6);
      GRAlineto(ggc, 5, 6);
      GRAlineto(ggc, 5, 3);
      GRAlineto(ggc, 10, 3);
      GRAlineto(ggc, 10, 1);
    }
    break;
  case 13:
  case 14:
  case 15:
  case 16:
  case 17:
  case 18:
    GRAcolor(ggc, fr, fg, fb);
    GRAlinestyle(ggc, 0, NULL, 1, 2, 0, 1000);
    if ((type == 15) || (type == 17)) {
      if (type == 15)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, 0, 4, 10, 6, 1);
      if (type == 15)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == 16) || (type == 18)) {
      if (type == 16)
	GRAcolor(ggc, fr2, fg2, fb2);
      GRArectangle(ggc, 4, 1, 6, 9, 1);
      if (type == 16)
	GRAcolor(ggc, fr, fg, fb);
    }
    if ((type == 13) || (type == 15)) {
      GRAline(ggc, 0, 4, 10, 4);
      GRAline(ggc, 10, 4, 10, 6);
      GRAline(ggc, 10, 6, 0, 6);
      GRAline(ggc, 0, 6, 0, 4);
    }
    if ((type == 14) || (type == 16)) {
      GRAline(ggc, 4, 1, 6, 1);
      GRAline(ggc, 6, 1, 6, 9);
      GRAline(ggc, 6, 9, 4, 9);
      GRAline(ggc, 4, 9, 4, 1);
    }
    break;
  case 19:
    GRAcolor(ggc, fr, fg, fb);
    GRAmoveto(ggc, 0, 8);
    GRAtextstyle(ggc, "Times", 28, 0, 0);
    GRAouttext(ggc, "fit");
    break;
  }

  _GRAclose(ggc);
  mxrestoreGC(&mxsave);
  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pix, NULL, 0, 0, 0, 0, width, height);

  //  g_object_unref(G_OBJECT(gc));
  g_object_unref(G_OBJECT(pix));

  GlobalLock = lockstate;

  return pixbuf;
}

static void
file_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row)
{
  int cx, i, j, len;
  char buf[256], *color, *valstr;
  struct narray *mask, *move;
  char *file, *bfile;

  for (i = 0; i < FILE_WIN_COL_NUM; i++) {
    if (strcmp(Flist[i].name, "file") == 0) {
      getobj(d->obj, "mask", row, 0, NULL, &mask);
      getobj(d->obj, "move_data", row, 0, NULL, &move);
      getobj(d->obj, "file", row, 0, NULL, &file);
      if ((arraynum(mask) != 0) || (arraynum(move) != 0)) {
	color = "blue";
      } else {
	color = "black";
      }
      bfile = getbasename(file);
      if (bfile != NULL) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, bfile);
	memfree(bfile);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
      }
      list_store_set_string(GTK_WIDGET(d->text), iter, FILE_WIN_COL_NUM, color);
    } else if (strcmp(Flist[i].name, "type") == 0) {
      GdkPixbuf *pixbuf = NULL;

      pixbuf = draw_type_pixbuf(d->obj, row);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
    } else if (strncmp(Flist[i].name, "axis_", 5) == 0) {
      sgetobjfield(d->obj, row, Flist[i].name, NULL, &valstr, FALSE, FALSE, FALSE);
      for (j = 0; (valstr[j] != '\0') && (valstr[j] != ':'); j++);
      if (valstr[j] == ':') j++;
      len = snprintf(buf, sizeof(buf), "%3s", valstr + j);
      list_store_set_string(GTK_WIDGET(d->text), iter, i, buf);
    } else if (strcmp(Flist[i].name, "hidden") == 0) {
      getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
    } else {
      getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
    }
  }
}

static gboolean
FileWinExpose(GtkWidget *wi, GdkEvent *event, gpointer client_data)
{
  struct SubWin *d;

  if (Menulock || GlobalLock)
    return TRUE;

  d = &(NgraphApp.FileWin);

  if (GTK_WIDGET(d->text) == NULL)
    return FALSE;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, file_list_set_val);
  } else {
    list_sub_window_set(d, file_list_set_val);
  }

  return FALSE;
}

void
CmFileWinMath(GtkWidget *w, gpointer p)
{
  struct objlist *obj;

  if (Menulock || GlobalLock)
    return;
  obj = chkobject("file");
  MathDialog(&DlgMath, obj);
  DialogExecute(TopLevel, &DlgMath);
}

static gboolean
ev_key_down(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  struct SubWin *d;
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  if (Menulock || GlobalLock)
    return TRUE;

  d = (struct SubWin *) user_data;
  e = (GdkEventKey *)event;

  switch (e->keyval) {
  case GDK_Delete:
    FileWinFileDelete(d);
    break;
  case GDK_Return:
    FileWinFileUpdate(d);
    break;
  case GDK_Insert:
    if (e->state & GDK_SHIFT_MASK)
      FileWinFileCopy2(d);
    else
      FileWinFileCopy(d);
    break;
  case GDK_space:
    FileWinFileDraw(d);
    break;
  case GDK_F:
    FileWinFit(d);
    break;
  default:
    return FALSE;
  }
  return TRUE;
}


static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, i;
  struct SubWin *d;

  d = (struct SubWin *) user_data;

  sel = d->select;
  for (i = 1; i < POPUP_ITEM_NUM; i++) {
    gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= d->num);
  }
}

void
CmFileWindow(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  d = &(NgraphApp.FileWin);
  d ->type = TypeFileWin;

  if (d->Win) {
    if (GTK_WIDGET_VISIBLE(d->Win)) { 
      gtk_widget_hide(d->Win);
    } else {
      gtk_widget_show_all(d->Win);
    }
  } else {
    GtkWidget *dlg;

    d->update = FileWinUpdate;
    d->setup_dialog = FileDialog;
    d->dialog = &DlgFile;
    d->ev_key = ev_key_down;

    dlg = list_sub_window_create(d, "File Window", FILE_WIN_COL_NUM, Flist, Filewin_xpm);

    g_signal_connect(dlg, "expose-event", G_CALLBACK(FileWinExpose), NULL);

    d->obj = chkobject("file");
    d->num = chkobjlastinst(d->obj);

    d->select = -1;
    sub_win_create_popup_menu(d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
    gtk_widget_show_all(dlg);
  }
}
