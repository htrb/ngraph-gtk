/*
 * $Id: x11graph.c,v 1.57 2010-03-04 08:30:17 hito Exp $
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "dir_defs.h"

#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "odraw.h"

#include "init.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11axis.h"
#include "x11print.h"
#include "x11commn.h"
#include "x11info.h"

#include "gtk_liststore.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#define PAPER_SIZE_MIN 1000

char *LoadPathStr[] = {
  N_("unchange"),
  N_("full"),
  N_("base"),
  NULL,
};

struct pagelisttype
{
  char *paper, *name;
  enum paper_id id;
  int langscape, width, height;
};

static struct pagelisttype pagelist[] = {
  {N_("Custom"),                   "custom",              PAPER_ID_CUSTOM, TRUE,  0,     0},
  {"A3 P (297.00x420.00)",     GTK_PAPER_NAME_A3,     PAPER_ID_A3,     FALSE, 29700, 42000},
  {"A4 P (210.00x297.00)",     GTK_PAPER_NAME_A4,     PAPER_ID_A4,     FALSE, 21000, 29700},
  {"A4 L (297.00x210.00)",     GTK_PAPER_NAME_A4,     PAPER_ID_A4,     TRUE,  29700, 21000},
  {"A5 P (148.00x210.00)",     GTK_PAPER_NAME_A5,     PAPER_ID_A5,     FALSE, 14800, 21000},
  {"A5 L (210.00x148.00)",     GTK_PAPER_NAME_A5,     PAPER_ID_A5,     TRUE,  21000, 14800},
  {"B4 P (257.00x364.00)",     "iso_b4",              PAPER_ID_B4,     FALSE, 25700, 36400},
  {"B5 P (182.00x257.00)",     GTK_PAPER_NAME_B5,     PAPER_ID_B5,     FALSE, 18200, 25700},
  {"B5 L (257.00x182.00)",     GTK_PAPER_NAME_B5,     PAPER_ID_B5,     TRUE,  25700, 18200},
  {N_("Letter P (215.90x279.40)"), GTK_PAPER_NAME_LETTER, PAPER_ID_LETTER, FALSE, 21590, 27940},
  {N_("Letter L (279.40x215.90)"), GTK_PAPER_NAME_LETTER, PAPER_ID_LETTER, TRUE,  27940, 21590},
  {N_("Legal  P (215.90x355.60)"), GTK_PAPER_NAME_LEGAL,  PAPER_ID_LEGAL,  FALSE, 21590, 35560},
  {N_("Legal  L (355.60x215.90)"), GTK_PAPER_NAME_LEGAL,  PAPER_ID_LEGAL,  TRUE,  35560, 21590},
};

#define PAGELISTNUM (sizeof(pagelist) / sizeof(*pagelist))
#define DEFAULT_PAPER_SIZE 0

int
set_paper_type(int w, int h)
{
  unsigned int j;

  if (w < 1 || h < 1)
    return 0;

  Menulocal.PaperWidth = w;
  Menulocal.PaperHeight = h;

  for (j = 0; j < PAGELISTNUM; j++) {
    if (w == pagelist[j].width &&  h == pagelist[j].height) {
      break;
    }
  }

  if (j == PAGELISTNUM) {
    j = DEFAULT_PAPER_SIZE;
  }

  Menulocal.PaperName = pagelist[j].name;
  Menulocal.PaperId = pagelist[j].id;
  Menulocal.PaperLandscape = pagelist[j].langscape;

  return j;
}

static void
PageDialogSetupItem(GtkWidget *w, struct PageDialog *d)
{
  int j;

  spin_entry_set_val(d->leftmargin, Menulocal.LeftMargin);
  spin_entry_set_val(d->topmargin, Menulocal.TopMargin);

  spin_entry_set_val(d->paperwidth, Menulocal.PaperWidth);
  spin_entry_set_val(d->paperheight, Menulocal.PaperHeight);

  spin_entry_set_val(d->paperzoom, Menulocal.PaperZoom);

  j = set_paper_type(Menulocal.PaperWidth, Menulocal.PaperHeight);

  combo_box_set_active(d->paper, j);
}

static void
PageDialogPage(GtkWidget *w, gpointer client_data)
{
  struct PageDialog *d;
  int a;

  d = (struct PageDialog *) client_data;

  a = combo_box_get_active(d->paper);

  if (a < 0)
    return;

  set_widget_sensitivity_with_label(d->paperwidth, a == 0);
  set_widget_sensitivity_with_label(d->paperheight, a == 0);

  if (a > 0) {
    spin_entry_set_val(d->paperwidth, pagelist[a].width);
    spin_entry_set_val(d->paperheight, pagelist[a].height);
  }
}

static void
PageDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  unsigned int j;
  struct PageDialog *d;
  int i;

  d = (struct PageDialog *) data;

  if (makewidget) {
#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(1, 2, FALSE);
#endif

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, PAPER_SIZE_MIN, G_MAXUSHORT);
    add_widget_to_table(table, w, _("paper _Width:"), FALSE, i++);
    d->paperwidth = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, PAPER_SIZE_MIN, G_MAXUSHORT);
    add_widget_to_table(table, w, _("paper _Height:"), FALSE, i++);
    d->paperheight = w;

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Paper:"), FALSE, i++);
    d->paper = w;
    g_signal_connect(w, "changed", G_CALLBACK(PageDialogPage), d);

    for (j = 0; j < PAGELISTNUM; j++) {
      combo_box_append_text(d->paper, _(pagelist[j].paper));
    }

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Left margin:"), FALSE, i++);
    d->leftmargin = w;


    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Top margin:"), FALSE, i++);
    d->topmargin = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, FALSE, TRUE);
    add_widget_to_table(table, w, _("paper _Zoom:"), FALSE, i++);
    d->paperzoom = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
  }
  PageDialogSetupItem(wi, d);
}

static void
PageDialogClose(GtkWidget *wi, void *data)
{
  struct PageDialog *d;
  int w, h;

  d = (struct PageDialog *) data;
  if (d->ret != IDOK)
    return;

  w = spin_entry_get_val(d->paperwidth);
  h = spin_entry_get_val(d->paperheight);

  if (w < PAPER_SIZE_MIN || h < PAPER_SIZE_MIN) {
    d->ret = IDLOOP;
    return;
  }

  set_paper_type(w, h);

  Menulocal.LeftMargin = spin_entry_get_val(d->leftmargin);
  Menulocal.TopMargin = spin_entry_get_val(d->topmargin);

  Menulocal.PaperZoom = spin_entry_get_val(d->paperzoom);
}

void
PageDialog(struct PageDialog *data)
{
  data->SetupWindow = PageDialogSetup;
  data->CloseWindow = PageDialogClose;
}

static void
set_objlist_btn_state(struct SwitchDialog *d, gboolean b)
{
  gtk_widget_set_sensitive(d->add, b);
  gtk_widget_set_sensitive(d->ins, b);
}

static void
set_drawlist_btn_state(struct SwitchDialog *d, gboolean b)
{
  gtk_widget_set_sensitive(d->top, b);
  gtk_widget_set_sensitive(d->bottom, b);
  gtk_widget_set_sensitive(d->up, b);
  gtk_widget_set_sensitive(d->down, b);
  gtk_widget_set_sensitive(d->del, b);
}

static void
SwitchDialogSetupItem(GtkWidget *w, struct SwitchDialog *d)
{
  int j, num;
  char **buf;
  GtkTreeIter iter;

  d->btn_lock = TRUE;

  list_store_clear(d->drawlist);
  num = arraynum(&(d->idrawrable));
  for (j = 0; j < num; j++) {
    buf = (char **) arraynget(&(d->drawrable),
			      arraynget_int(&(d->idrawrable), j));
    list_store_append(d->drawlist, &iter);
    list_store_set_string(d->drawlist, &iter, 0, _(*buf));
  }

  d->btn_lock = FALSE;
}

static void
SwitchDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list, *ptr;
  int duplicate, num, *data, i;

  d = (struct SwitchDialog *) client_data;

  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->objlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  data = arraydata(&(d->idrawrable));
  num = arraynum(&(d->idrawrable));

  for (ptr = list; ptr; ptr = ptr->next) {
    int *ary, a;

    duplicate = FALSE;
    ary = gtk_tree_path_get_indices((GtkTreePath *)(ptr->data));
    a = ary[0];
    for (i = 0; i < num; i++) {
      if (data[i] == a) {
	duplicate = TRUE;
	break;
      }
    }
    if ( !duplicate) {
      arrayadd(&(d->idrawrable), &a);
    }
  }

  if (list) {
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free(list);
  }

  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static void
SwitchDialogInsert(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  int i, a, pos, num2;
  int *data;
  GtkTreeSelection *selected;
  GList *list, *last;

  d = (struct SwitchDialog *) client_data;

  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);
  pos = 0;
  if (list) {
    last = g_list_last(list);
    if (last) {
      int *ptr;
      ptr = gtk_tree_path_get_indices((GtkTreePath *)(last->data));
      pos = ptr[0];
    }
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free(list);
  }

  data = arraydata(&(d->idrawrable));

  num2 = arraynum(&(d->idrawrable));
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->objlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  for (last = list; last; last = last->next) {
    int *ptr;
    ptr = gtk_tree_path_get_indices((GtkTreePath *)(last->data));
    a = ptr[0];
    for (i = 0; i < num2; i++) {
      if (data[i] == a) {
	break;
      }
    }
    if (i == num2) {
      arrayins(&(d->idrawrable), &a, pos);
    }
  }

  if (list) {
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free(list);
  }

  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static void
switch_dialog_top_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary, i, k;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  i = ary[0];
  k = arraynget_int(&(d->idrawrable), i);
  arrayndel(&(d->idrawrable), i);
  arrayins(&(d->idrawrable), &k, 0);
}

static void
switch_dialog_last_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary, i, k;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  i = ary[0];
  k = arraynget_int(&(d->idrawrable), i);
  arrayndel(&(d->idrawrable), i);
  arrayadd(&(d->idrawrable), &k);
}

static void
SwitchDialogUp(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list, *ptr;
  struct SwitchDialog *d;
  int i, k, modified, *ary;
  GtkTreePath *path;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list == NULL) {
    return;
  }

  modified = FALSE;
  for (ptr = list; ptr; ptr = g_list_next(ptr)) {
    path = (GtkTreePath *) ptr->data;
    ary = gtk_tree_path_get_indices(path);

    if (ary == NULL) {
      break;
    }

    i = ary[0];
    if (i <= 0) {
      break;
    }

    k = arraynget_int(&(d->idrawrable), i);
    arrayndel(&(d->idrawrable), i);
    i--;
    arrayins(&(d->idrawrable), &k, i);
    modified = TRUE;
  }

  if (modified) {
    SwitchDialogSetupItem(d->widget, d);

    for (ptr = list; ptr; ptr = g_list_next(ptr)) {
      path = (GtkTreePath *) ptr->data;
      ary = gtk_tree_path_get_indices(path);

      if (ary == NULL)
	break;

      list_store_select_nth(d->drawlist, ary[0] - 1);
    }
  }

  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);
}

static void
SwitchDialogDown(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list, *ptr;
  struct SwitchDialog *d;
  int i, k, num, modified, *ary;
  GtkTreePath *path;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list == NULL) {
    return;
  }

  num = list_store_get_num(d->drawlist);

  modified = FALSE;
  for (ptr = g_list_last(list); ptr; ptr = g_list_previous(ptr)) {
    path = (GtkTreePath *) ptr->data;
    ary = gtk_tree_path_get_indices(path);

    if (ary == NULL)
      break;

    i = ary[0];
    if (i >= num - 1) {
      break;
    }

    k = arraynget_int(&(d->idrawrable), i);
    arrayndel(&(d->idrawrable), i);
    i++;
    arrayins(&(d->idrawrable), &k, i);
    modified = TRUE;
  }

  if (modified) {
    SwitchDialogSetupItem(d->widget, d);

    for (ptr = g_list_last(list); ptr; ptr = g_list_previous(ptr)) {
      path = (GtkTreePath *) ptr->data;
      ary = gtk_tree_path_get_indices(path);

      if (ary == NULL)
	break;

      list_store_select_nth(d->drawlist, ary[0] + 1);
    }
  }

  g_list_foreach(list, free_tree_path_cb, NULL);
  g_list_free(list);
}

static void
SwitchDialogTop(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list;
  struct SwitchDialog *d;
  int i, num = 0;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    num = g_list_length(list);
    g_list_foreach(list, switch_dialog_top_cb, d);
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free(list);
  }
  SwitchDialogSetupItem(d->widget, d);
  for (i = 0; i < num; i++) {
    list_store_select_nth(d->drawlist, i);
  }
}

static void
SwitchDialogLast(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list;
  int n, i, num = 0;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    num = g_list_length(list);
    list = g_list_reverse(list);
    g_list_foreach(list, switch_dialog_last_cb, d);
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free (list);
  }
  SwitchDialogSetupItem(d->widget, d);
  n = list_store_get_num(d->drawlist);
  for (i = 0; i < num; i++) {
    list_store_select_nth(d->drawlist, n - i - 1);
  }
}

static void
switch_dialog_remove_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  arrayndel(&(d->idrawrable), ary[0]);
}

static void
SwitchDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    list = g_list_reverse(list);
    g_list_foreach(list, switch_dialog_remove_cb, d);
    g_list_foreach(list, free_tree_path_cb, NULL);
    g_list_free (list);
  }
  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static gboolean
drawlist_sel_cb(GtkTreeSelection *sel, gpointer user_data)
{
  int n;
  struct SwitchDialog *d;

  d = (struct SwitchDialog *) user_data;

  if (! d->btn_lock) {
    n = gtk_tree_selection_count_selected_rows(sel);
    set_drawlist_btn_state(d, n);
  }
  return FALSE;
}

static gboolean
objlist_sel_cb(GtkTreeSelection *sel, gpointer user_data)
{
  int n;
  struct SwitchDialog *d;

  d = (struct SwitchDialog *) user_data;

  if (! d->btn_lock) {
    n = gtk_tree_selection_count_selected_rows(sel);
    set_objlist_btn_state(d, n);
  }
  return FALSE;
}

static void
SwitchDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *hbox, *vbox, *vbox2, *label, *frame;
  GtkTreeIter iter;
  struct SwitchDialog *d;
  int num2, num1, j, k, *obj_check;
  char **buf;
  GtkTreeSelection *sel;
  static n_list_store list[] = {
    {N_("Object"), G_TYPE_STRING, TRUE, FALSE, NULL},
  };

  d = (struct SwitchDialog *) data;

  if (makewidget) {

#if GTK_CHECK_VERSION(3, 0, 0)
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    hbox = gtk_hbox_new(FALSE, 4);

    vbox = gtk_vbox_new(FALSE, 4);
#endif

    label = gtk_label_new_with_mnemonic(_("_Draw order"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->drawlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(drawlist_sel_cb), d);


    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), w);

    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif

    w = gtk_button_new_with_mnemonic(_("_Add"));
    set_button_icon(w, "list-add");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogAdd), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->add = w;

    w = gtk_button_new_with_mnemonic(_("_Insert"));
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogInsert), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->ins = w;

#if GTK_CHECK_VERSION(3, 2, 0)
    w = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#else
    w = gtk_hseparator_new();
#endif
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_button_new_with_mnemonic(_("_Top"));
    set_button_icon(w, "go-top");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogTop), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->top =w;

    w = gtk_button_new_with_mnemonic(_("_Up"));
    set_button_icon(w, "go-up");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogUp), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->up = w;

    w = gtk_button_new_with_mnemonic(_("_Down"));
    set_button_icon(w, "go-down");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogDown), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->down = w;

    w = gtk_button_new_with_mnemonic(_("_Bottom"));
    set_button_icon(w, "go-bottom");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogLast), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->bottom = w;

    w = gtk_button_new_with_mnemonic(("_Remove"));
    set_button_icon(w, "list-remove");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogRemove), d);
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
    d->del = w;

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox2 = gtk_vbox_new(FALSE, 4);
#endif
    gtk_box_pack_end(GTK_BOX(vbox2), vbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif

    label = gtk_label_new_with_mnemonic(_("_Objects"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->objlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(objlist_sel_cb), d);


    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), w);

    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));

    d->btn_lock = FALSE;
  }

  menuadddrawrable(chkobject("draw"), &(d->drawrable));
  list_store_clear(d->objlist);
  num2 = arraynum(&(d->drawrable));
  for (j = 0; j < num2; j++) {
    buf = (char **) arraynget(&(d->drawrable), j);
    list_store_append(d->objlist, &iter);
    list_store_set_string(d->objlist, &iter, 0, _(*buf));
  }
  num1 = arraynum(&(Menulocal.drawrable));
  obj_check = g_malloc0(sizeof(*obj_check) * num2);
  if (obj_check == NULL) {
    return;
  }
  for (j = 0; j < num1; j++) {
    struct objlist *obj;

    buf = (char **) arraynget(&(Menulocal.drawrable), j);
    obj = chkobject(*buf);
    if (obj == NULL) {
      continue;
    }
    for (k = 0; k < num2; k++) {
      if (strcmp0(arraynget_str(&(d->drawrable), k), obj->name) == 0) {
	break;
      }
    }
    if (k != num2 && obj_check[k] == 0) {
      obj_check[k] = 1;
      arrayadd(&(d->idrawrable), &k);
    }
  }
  g_free(obj_check);
  SwitchDialogSetupItem(wi, d);
  set_objlist_btn_state(d, FALSE);
  set_drawlist_btn_state(d, FALSE);
}

static void
SwitchDialogClose(GtkWidget *w, void *data)
{
  struct SwitchDialog *d;
  int j, num;
  char **buf;

  d = (struct SwitchDialog *) data;
  if (d->ret == IDOK) {
    arraydel2(&(Menulocal.drawrable));
    num = arraynum(&(d->idrawrable));
    for (j = 0; j < num; j++) {
      buf = (char **) arraynget(&(d->drawrable),
				arraynget_int(&(d->idrawrable), j));
      if ((*buf) != NULL)
	arrayadd2(&(Menulocal.drawrable), *buf);
    }
  }
  arraydel2(&(d->drawrable));
  arraydel(&(d->idrawrable));
}

void
SwitchDialog(struct SwitchDialog *data)
{
  data->SetupWindow = SwitchDialogSetup;
  data->CloseWindow = SwitchDialogClose;
  arrayinit(&(data->drawrable), sizeof(char *));
  arrayinit(&(data->idrawrable), sizeof(int));
}

static void
DirectoryDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *table;
  struct DirectoryDialog *d;
  char *cwd;

  d = (struct DirectoryDialog *) data;
  if (makewidget) {
#if GTK_CHECK_VERSION(3, 4, 0)
    table = gtk_grid_new();
#else
    table = gtk_table_new(2, 2, FALSE);
#endif

    w = gtk_file_chooser_button_new(_("directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    d->dir = w;
    add_widget_to_table(table, w, _("_Select Dir:"), TRUE, 0);

    w = gtk_label_new(_("Current Dir:"));
#if GTK_CHECK_VERSION(3, 4, 0)
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    g_object_set(w, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), w, 0, 1, 1, 1);
#else
    gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), w, 0, 1, 1, 2, 0, 0, 4, 4);
#endif

    w = gtk_label_new("");
    gtk_label_set_ellipsize(GTK_LABEL(w), PANGO_ELLIPSIZE_START);
    d->dir_label = w;
#if GTK_CHECK_VERSION(3, 4, 0)
    gtk_widget_set_hexpand(w, TRUE);
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    g_object_set(w, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), w, 1, 2, 1, 1);
#else
    gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), w, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 4, 4);
#endif

    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
  }

  cwd = ngetcwd();
  if (cwd) {
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d->dir), cwd);
    gtk_label_set_text(GTK_LABEL(d->dir_label), cwd);
    g_free(cwd);
  }
}

static void
DirectoryDialogClose(GtkWidget *w, void *data)
{
  struct DirectoryDialog *d;
  char *s;

  d = (struct DirectoryDialog *) data;
  if (d->ret == IDCANCEL)
    return;

  s = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d->dir));

  if (s && strlen(s) > 0) {
    if (nchdir(s)) {
      ErrorMessage();
    }
  }

  if (s)
    g_free(s);
}

void
DirectoryDialog(struct DirectoryDialog *data)
{
  data->SetupWindow = DirectoryDialogSetup;
  data->CloseWindow = DirectoryDialogClose;
}

static void
LoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *vbox;
  struct LoadDialog *d;
  int j;

  d = (struct LoadDialog *) data;
  if (makewidget) {
#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif

    w = gtk_check_button_new_with_mnemonic(_("_Expand included file"));
    d->expand_file = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = create_text_entry(FALSE, TRUE);
    item_setup(vbox, w, _("_Dir:"), FALSE);
    d->dir = w;

    w = combo_box_create();
    item_setup(vbox, w, _("_Path:"), FALSE);
    for (j = 0; LoadPathStr[j]; j++) {
      combo_box_append_text(w, _(LoadPathStr[j]));
    }
    d->load_path = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->expand_file), d->expand);
  combo_box_set_active(d->load_path, d->loadpath);
  gtk_entry_set_text(GTK_ENTRY(d->dir), d->exdir);
}

static void
LoadDialogClose(GtkWidget *w, void *data)
{
  struct LoadDialog *d;
  const char *s;

  d = (struct LoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
  d->expand = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->expand_file));
  s = gtk_entry_get_text(GTK_ENTRY(d->dir));
  g_free(d->exdir);
  d->exdir = g_strdup(s);
  d->loadpath = combo_box_get_active(d->load_path);
}

void
LoadDialog(struct LoadDialog *data)
{
  data->SetupWindow = LoadDialogSetup;
  data->CloseWindow = LoadDialogClose;
  data->expand = Menulocal.expand;
  data->exdir = g_strdup(Menulocal.expanddir);
  data->loadpath = Menulocal.loadpath;
}

static void
SaveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *vbox;
  int j;
  struct SaveDialog *d;

  d = (struct SaveDialog *) data;
  if (makewidget) {
#if GTK_CHECK_VERSION(3, 0, 0)
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
    vbox = gtk_vbox_new(FALSE, 4);
#endif

    w = combo_box_create();
    item_setup(vbox, w, _("_Path:"), FALSE);
    for (j = 0; pathchar[j] != NULL; j++) {
      combo_box_append_text(w, _(pathchar[j]));
    }
    d->path = w;

    w = gtk_check_button_new_with_mnemonic(_("_Include data file"));
    d->include_data = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Include merge file"));
    d->include_merge = w;
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
  }
  combo_box_set_active(d->path, Menulocal.savepath);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->include_data), Menulocal.savewithdata);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->include_merge), Menulocal.savewithmerge);
  d->focus = d->include_data;
}

static void
SaveDialogClose(GtkWidget *w, void *data)
{
  int num;
  struct SaveDialog *d;

  d = (struct SaveDialog *) data;
  if (d->ret == IDCANCEL)
    return;

  num = combo_box_get_active(d->path);
  if (num >= 0) {
    d->Path = num;
  }
  *(d->SaveData) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->include_data));
  *(d->SaveMerge) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->include_merge));
}

void
SaveDialog(struct SaveDialog *data, int *sdata, int *smerge)
{
  data->SetupWindow = SaveDialogSetup;
  data->CloseWindow = SaveDialogClose;
  data->SaveData = sdata;
  data->SaveMerge = smerge;
}

void
CmGraphNewMenu(GtkAction *w, gpointer client_data)
{
  int sel;

  if (Menulock || Globallock)
    return;

  if (!CheckSave())
    return;

  DeleteDrawable();

  sel = GPOINTER_TO_INT(client_data);
  switch (sel) {
  case MenuIdGraphNewFrame:
    CmAxisNewFrame(w, client_data);
    break;
  case MenuIdGraphNewSection:
    CmAxisNewSection(w, client_data);
    break;
  case MenuIdGraphNewCross:
    CmAxisNewCross(w, client_data);
    break;
  case MenuIdGraphAllClear:
  default:
    break;
  }

  SetFileName(NULL);
  set_axis_undo_button_sensitivity(FALSE);
  reset_graph_modified();

  UpdateAll();
  CmViewerDraw(NULL, GINT_TO_POINTER(TRUE));
  InfoWinClear();
}

void
CmGraphLoad(GtkAction *w, gpointer client_data)
{
  char *file;

  if (Menulock || Globallock)
    return;

  if (!CheckSave())
    return;

  if (nGetOpenFileName(TopLevel,
		       _("Load NGP file"), "ngp", &(Menulocal.graphloaddir),
		       NULL, &file, TRUE,
		       Menulocal.changedirectory) != IDOK) {
    return;
  }

  LoadDialog(&DlgLoad);
  if (DialogExecute(TopLevel, &DlgLoad) == IDOK) {
    LoadNgpFile(file, DlgLoad.loadpath, DlgLoad.expand,
		DlgLoad.exdir, Menulocal.scriptconsole, "-f");
  }
  g_free(DlgLoad.exdir);
  g_free(file);
}

void
CmGraphSave(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  GraphSave(FALSE);
}

void
CmGraphOverWrite(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  GraphSave(TRUE);
}

void
CmGraphSwitch(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  SwitchDialog(&DlgSwitch);
  if (DialogExecute(TopLevel, &DlgSwitch) == IDOK) {
    set_graph_modified();
    ChangePage();
  }
}

void
CmGraphPage(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  PageDialog(&DlgPage);
  if (DialogExecute(TopLevel, &DlgPage) == IDOK) {
    SetPageSettingsToGRA();
    ChangePage();
    GetPageSettingsFromGRA();
    set_graph_modified();
  }
}

void
CmGraphDirectory(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  DirectoryDialog(&DlgDirectory);
  DialogExecute(TopLevel, &DlgDirectory);
}

void
CmGraphShell(GtkAction *w, gpointer client_data)
{
  struct objlist *obj, *robj, *shell;
  N_VALUE *inst;
  int idn, allocnow, n;

  if (Menulock || Globallock)
    return;

  menu_lock(TRUE);

  obj = Menulocal.obj;
  inst = Menulocal.inst;
  idn = getobjtblpos(obj, "_evloop", &robj);
  registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, NULL);
  if ((shell = chkobject("shell")) != NULL) {
    n = chkobjlastinst(shell);
    if (n < 0) {
      newobj(shell);
    }
    allocnow = allocate_console();
    exeobj(shell, "shell", 0, 0, NULL);
    free_console(allocnow);
  }
  unregisterevloop(robj, idn, inst);
  menu_lock(FALSE);
  UpdateAll();
}

void
CmGraphQuit(GtkAction *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;

  QuitGUI();
}

void
CmGraphHistory(GtkRecentChooser *w, gpointer client_data)
{
  char *uri, *fname, *path;

  if (Menulock || Globallock)
    return;

  uri = gtk_recent_chooser_get_current_uri(w);
  if (uri == NULL) {
    return;
  }

  fname = g_filename_from_uri(uri, NULL, NULL);
  g_free(uri);
  if (fname == NULL) {
    return;
  }

  if (!CheckSave()) {
    g_free(fname);
    return;
  }

  path = g_path_get_dirname(fname);
  if (nchdir(path)) {
    ErrorMessage();
    g_free(path);
    g_free(fname);
    return;
  }
  g_free(path);

  LoadDialog(&DlgLoad);

  if (DialogExecute(TopLevel, &DlgLoad) == IDOK) {
    LoadNgpFile(fname, DlgLoad.loadpath, DlgLoad.expand,
		DlgLoad.exdir, Menulocal.scriptconsole, "-f");
  }
  g_free(DlgLoad.exdir);
  g_free(fname);
}

#if ! GTK_CHECK_VERSION(2, 24, 0)
static void
about_link_activated_cb(GtkAboutDialog *about, const gchar *link, gpointer data)
{
  char *cmd;

  cmd = g_strdup_printf("%s \"%s\"", Menulocal.browser, link);
  system_bg(cmd);

  g_free(cmd);
}
#endif

void
CmHelpAbout(GtkAction *w, gpointer client_data)
{
  struct objlist *obj;
  char *web, *copyright;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("system")) == NULL)
    return;

  getobj(obj, "copyright", 0, 0, NULL, &copyright);
  getobj(obj, "web", 0, 0, NULL, &web);

#if ! GTK_CHECK_VERSION(2, 24, 0)
  gtk_about_dialog_set_url_hook(about_link_activated_cb, NULL, NULL);
#endif
  gtk_show_about_dialog(GTK_WINDOW(TopLevel),
			"program-name", PACKAGE,
			"copyright", copyright,
			"version", VERSION,
			"website", web,
#if GTK_CHECK_VERSION(3, 0, 0)
			"license-type", GTK_LICENSE_GPL_2_0,
#else
			"license", License,
#endif
			"wrap-license", TRUE,
			"authors", Auther,
			"translator-credits", Translator,
			"documenters", Documenter,
			"comments", _("Ngraph is the program to create scientific 2-dimensional graphs for researchers and engineers."),
			NULL);
}

static char *
check_help_file(void)
{
  char *file, *ptr, *tmp;
  const char *locale;

  locale = n_getlocale();
  if (locale == NULL) {
    goto End;
  }

  tmp = g_strdup(locale);
  if (tmp == NULL){
    goto End;
  }

  ptr = strchr(tmp, '_');
  if (ptr == NULL) {
    g_free(tmp);
    goto End;
  }

  *ptr = '\0';
  file = g_strdup_printf("%s/html/%s/%s", DOCDIR, tmp, HELP_FILE);
  g_free(tmp);

  if (naccess(file, R_OK) == 0) {
    return file;
  }
  g_free(file);

 End:
  return g_strdup_printf("%s/html/ja/%s", DOCDIR, HELP_FILE); /* default language of the help file is Japanese. */
}

void
CmHelpHelp(GtkAction *w, gpointer client_data)
{
  char *cmd, *file;

  if (Menulock || Globallock)
    return;

  if (Menulocal.help_browser == NULL)
    return;

  file = check_help_file();

  cmd = g_strdup_printf("%s \"%s\"", Menulocal.help_browser, file);
  g_free(file);

  system_bg(cmd);

  g_free(cmd);
}
