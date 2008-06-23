/* 
 * $Id: x11merge.c,v 1.5 2008/06/23 02:18:25 hito Exp $
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

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "ioutil.h"

#include "gtk_liststore.h"
#include "gtk_subwin.h"

#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11file.h"
#include "x11merge.h"
#include "x11commn.h"

static n_list_store Mlist[] = {
  {"",     G_TYPE_BOOLEAN, TRUE, FALSE, "hidden",      FALSE},
  {"#",    G_TYPE_INT,     TRUE, FALSE, "id",          FALSE},
  {"file", G_TYPE_STRING,  TRUE, FALSE, "file",        FALSE},
  {"tm",   G_TYPE_INT,     TRUE, FALSE, "top_margin",  FALSE},
  {"lm",   G_TYPE_INT,     TRUE, FALSE, "left_margin", FALSE},
  {"zm",   G_TYPE_INT,     TRUE, FALSE, "zoom",        FALSE},
  {"^#",   G_TYPE_INT,     TRUE, FALSE, "oid",         FALSE},
};

#define MERG_WIN_COL_NUM (sizeof(Mlist)/sizeof(*Mlist))
#define MERG_WIN_COL_OID (MERG_WIN_COL_NUM - 1)
#define MERG_WIN_COL_ID 1

static void MergeWinMergeOpen(GtkMenuItem *w, gpointer client_data);
static gboolean MergeWinExpose(GtkWidget *w, GdkEvent *event, gpointer client_data);
static void merge_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row);

static struct subwin_popup_list Popup_list[] = {
  {GTK_STOCK_OPEN,        G_CALLBACK(MergeWinMergeOpen), TRUE, NULL},
  {"_Focus",              G_CALLBACK(list_sub_window_focus), FALSE, NULL},
  {GTK_STOCK_PREFERENCES, G_CALLBACK(list_sub_window_update), TRUE, NULL},
  {GTK_STOCK_CLOSE,       G_CALLBACK(list_sub_window_delete), TRUE, NULL},
  {GTK_STOCK_GOTO_TOP,    G_CALLBACK(list_sub_window_move_top), TRUE, NULL},
  {GTK_STOCK_GO_UP,       G_CALLBACK(list_sub_window_move_up), TRUE, NULL},
  {GTK_STOCK_GO_DOWN,     G_CALLBACK(list_sub_window_move_down), TRUE, NULL},
  {GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(list_sub_window_move_last), TRUE, NULL},
  {N_("_Duplicate"),      G_CALLBACK(list_sub_window_copy), FALSE, NULL},
  {N_("_Hide"),           G_CALLBACK(list_sub_window_hide), FALSE, NULL},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list))


static void
MergeDialogSetupItem(struct MergeDialog *d, int file, int id)
{
  if (file) {
    SetTextFromObjField(d->file, d->Obj, id, "file");
  }
  SetTextFromObjField(d->topmargin, d->Obj, id, "top_margin");
  SetTextFromObjField(d->leftmargin, d->Obj, id, "left_margin");
  SetTextFromObjField(d->zoom, d->Obj, id, "zoom");
#ifdef JAPANESE
  SetToggleFromObjField(d->greeksymbol, d->Obj, id, "symbol_greek");
#endif
}

static void
MergeDialogCopy(struct MergeDialog *d)
{
  int sel;

  if ((sel = CopyClick(d->widget, d->Obj, d->Id, FileCB)) != -1)
    MergeDialogSetupItem(d, FALSE, sel);
}

static void
MergeDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox;
  struct MergeDialog *d;
  char title[64];

  d = (struct MergeDialog *) data;

  snprintf(title, sizeof(title), _("Merge %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   GTK_STOCK_CLOSE, IDDELETE,
			   GTK_STOCK_COPY, IDCOPY,
			   NULL);

    hbox = gtk_hbox_new(FALSE, 2);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_File:"), TRUE);
    d->file = w;
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    hbox = gtk_hbox_new(FALSE, 2);
    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Top Margin:"), TRUE);
    d->topmargin = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Left Margin:"), TRUE);
    d->leftmargin = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Zoom:"), TRUE);
    d->zoom = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

#ifdef JAPANESE
    hbox = gtk_hbox_new(FALSE, 2);
    w = gtk_check_button_new_with_label(_("GreekSymbol"));
    d->greeksymbol = w;
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
#endif
  }
  MergeDialogSetupItem(d, TRUE, d->Id);
}

static void
MergeDialogClose(GtkWidget *w, void *data)
{
  struct MergeDialog *d;
  int ret;

  d = (struct MergeDialog *) data;

  switch(d->ret) {
  case IDOK:
    break;
  case IDCOPY:
    MergeDialogCopy(d);
    d->ret = IDLOOP;
    return;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (SetObjFieldFromText(d->file, d->Obj, d->Id, "file"))
    return;
  if (SetObjFieldFromText(d->topmargin, d->Obj, d->Id, "top_margin"))
    return;
  if (SetObjFieldFromText(d->leftmargin, d->Obj, d->Id, "left_margin"))
    return;
  if (SetObjFieldFromText(d->zoom, d->Obj, d->Id, "zoom"))
    return;
#ifdef JAPANESE
  if (SetObjFieldFromToggle(d->greeksymbol, d->Obj, d->Id, "symbol_greek"))
    return;
#endif
  d->ret = ret;
}

void
MergeDialog(void *data, struct objlist *obj, int id, int sub_id)
{
  struct MergeDialog *d;

  d = (struct MergeDialog *) data;

  d->SetupWindow = MergeDialogSetup;
  d->CloseWindow = MergeDialogClose;
  d->Obj = obj;
  d->Id = id;
}

void
CmMergeOpen(void)
{
  struct objlist *obj;
  char *name = NULL;
  int id, ret;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (nGetOpenFileName(TopLevel, _("Merge open"), "gra", NULL, NULL, &name,
		       "*.gra", TRUE, Menulocal.changedirectory) != IDOK || ! name)
    return;

  id = newobj(obj);
  if (id >= 0) {
    changefilename(name);
    putobj(obj, "file", id, name);
    MergeDialog(&DlgMerge, obj, id, -1);
    ret = DialogExecute(TopLevel, &DlgMerge);
    if ((ret == IDDELETE) || (ret == IDCANCEL)) {
      delobj(obj, id);
    } else {
      NgraphApp.Changed = TRUE;
    }
  } else {
    free(name);
  }
  MergeWinUpdate(TRUE);
}

void
CmMergeClose(void)
{
  struct narray farray;
  struct objlist *obj;
  int i;
  int num, *array;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delobj(obj, array[i]);
      NgraphApp.Changed = TRUE;
    }
    MergeWinUpdate(TRUE);
  }
  arraydel(&farray);
}

void
CmMergeUpdate(void)
{
  struct narray farray;
  struct objlist *obj;
  int i, j, ret;
  int *array, num;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("merge")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    num = arraynum(&farray);
    array = (int *) arraydata(&farray);
    for (i = 0; i < num; i++) {
      MergeDialog(&DlgMerge, obj, array[i], -1);
      if ((ret = DialogExecute(TopLevel, &DlgMerge)) == IDDELETE) {
	delobj(obj, array[i]);
	for (j = i + 1; j < num; j++)
	  array[j]--;
      }
      if (ret != IDCANCEL)
	NgraphApp.Changed = TRUE;
    }
    MergeWinUpdate(TRUE);
  }
  arraydel(&farray);
}


void
CmMergeMenu(GtkMenuItem *w, gpointer client_data)
{
  switch ((int) client_data) {
  case MenuIdMergeOpen:
    CmMergeOpen();
    break;
  case MenuIdMergeUpdate:
    CmMergeUpdate();
    break;
  case MenuIdMergeClose:
    CmMergeClose();
    break;
  }
}

static void
MergeWinMergeOpen(GtkMenuItem *w, gpointer client_data)
{
  CmMergeOpen();
}

void
MergeWinUpdate(int clear)
{
  struct SubWin *d;

  d = &(NgraphApp.MergeWin);

  MergeWinExpose(NULL, NULL, NULL);

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), MERG_WIN_COL_ID, d->select);
  }
}

static void
merge_list_set_val(struct SubWin *d, GtkTreeIter *iter, int row)
{
  int cx;
  int i = 0;
  char *file, *bfile;

  for (i = 0; i < MERG_WIN_COL_NUM; i++) {
    if (strcmp(Mlist[i].name, "file") == 0) {
      getobj(d->obj, "file", row, 0, NULL, &file);
      bfile = getbasename(file);
      if (bfile != NULL) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, bfile);
	memfree(bfile);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "....................");
      }
    } else if (strcmp(Mlist[i].name, "hidden") == 0) {
      getobj(d->obj, Mlist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Mlist[i].type, &cx);
    } else {
      getobj(d->obj, Mlist[i].name, row, 0, NULL, &cx);
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Mlist[i].type, &cx);
    }
  }
}

static gboolean
MergeWinExpose(GtkWidget *w, GdkEvent *event, gpointer client_data)
{
  struct SubWin *d;

  d = &(NgraphApp.MergeWin);
  if (GTK_WIDGET(d->text) == NULL)
    return FALSE;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d, merge_list_set_val);
  } else {
    list_sub_window_set(d, merge_list_set_val);
  }

  return FALSE;
}

/*
void
MergeWindowUnmap(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;
  Position x, y, x0, y0;
  Dimension w0, h0;

  d = &(NgraphApp.MergeWin);
  if (d->Win != NULL) {
    XtVaGetValues(d->Win, XmNx, &x, XmNy, &y,
		  XmNwidth, &w0, XmNheight, &h0, NULL);
    Menulocal.mergewidth = w0;
    Menulocal.mergeheight = h0;
    XtTranslateCoords(TopLevel, 0, 0, &x0, &y0);
    Menulocal.mergex = x - x0;
    Menulocal.mergey = y - y0;
    XtDestroyWidget(d->Win);
    d->Win = NULL;
    GTK_WIDGET(d->text) = NULL;
    XmToggleButtonSetState(XtNameToWidget
			   (TopLevel, "*windowmenu.button_3"), FALSE, FALSE);
  }
}
*/

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
CmMergeWindow(GtkWidget *w, gpointer client_data)
{
  struct SubWin *d;

  d = &(NgraphApp.MergeWin);
  d ->type = TypeMergeWin;

  if (d->Win) {
    if (GTK_WIDGET_VISIBLE(d->Win)) { 
      gtk_widget_hide(d->Win);
    } else {
      gtk_widget_show_all(d->Win);
    }
  } else {
    GtkWidget *dlg;

    d->update = MergeWinUpdate;
    d->setup_dialog = MergeDialog;
    d->dialog = &DlgMerge;

    dlg = list_sub_window_create(d, "Merge Window", MERG_WIN_COL_NUM, Mlist, Mergewin_xpm);

    g_signal_connect(dlg, "expose-event", G_CALLBACK(MergeWinExpose), NULL);

    d->obj = chkobject("merge");
    d->num = chkobjlastinst(d->obj);

    d->select = -1;
    sub_win_create_popup_menu(d, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));
    gtk_widget_show_all(dlg);
 }
}
